//
// Created by Jonas Zell on 2019-01-23.
//

#include "mineshaft/Context.h"
#include "mineshaft/utils.h"
#include "mineshaft/World/World.h"

using namespace mc;

WorldSegment::WorldSegment(World *world, int x, int z)
{
   x *= MC_WORLD_SEGMENT_WIDTH;
   z *= MC_WORLD_SEGMENT_DEPTH;

   for (int i = x; i < x + MC_WORLD_SEGMENT_WIDTH; ++i) {
      for (int j = z; j < z + MC_WORLD_SEGMENT_DEPTH; ++j) {
         chunks[i - x][j - z].initialize(world, i, j);
         llvm::errs() << "initializing chunk (" << i << ", " << j << ")\n";
      }
   }
}

World::World(mc::Context &Ctx, const WorldGenOptions &options)
   : Ctx(Ctx), numChunksToRender(
   Ctx.gameOptions.renderDistance * Ctx.gameOptions.renderDistance * 4),
     options(options)
{
   chunksToRender = new Chunk*[numChunksToRender];
   chunkUpdateDistanceThreshold = Ctx.gameOptions.renderDistance * 15.0f;
}

World::~World()
{
   int xSize = maxX - minX;
   int zSize = maxZ - minZ;

   for (int i = 0; i < xSize; ++i) {
      for (int j = 0; j < zSize; ++j) {
         if (!loadedSegments[i][j]) {
            continue;
         }

         loadedSegments[i][j]->~WorldSegment();
      }

      free(loadedSegments[i]);
   }

   delete[] chunksToRender;
}

World::ChunkIndex World::getLocalChunkCoordinate(const ChunkPosition &chunkPos)
{
   ChunkIndex result;

   if (chunkPos.x < 0) {
      result.segmentX = (int)std::floor((float)chunkPos.x / (float)MC_WORLD_SEGMENT_WIDTH);
      result.localChunkX = chunkPos.x % MC_WORLD_SEGMENT_WIDTH;

      if (result.localChunkX < 0) {
         result.localChunkX += MC_WORLD_SEGMENT_WIDTH;
      }
   }
   else {
      result.segmentX = chunkPos.x / MC_WORLD_SEGMENT_WIDTH;
      result.localChunkX = (chunkPos.x % MC_WORLD_SEGMENT_WIDTH);
   }

   if (chunkPos.z < 0) {
      result.segmentZ = (int)std::floor((float)chunkPos.z / (float)MC_WORLD_SEGMENT_DEPTH);
      result.localChunkZ = chunkPos.z % MC_WORLD_SEGMENT_DEPTH;

      if (result.localChunkZ < 0) {
         result.localChunkZ += MC_WORLD_SEGMENT_DEPTH;
      }
   }
   else {
      result.segmentZ = chunkPos.z / MC_WORLD_SEGMENT_DEPTH;
      result.localChunkZ = (chunkPos.z % MC_WORLD_SEGMENT_DEPTH);
   }

   return result;
}

std::pair<int, int>
World::getLocalSegmentCoordinate(int segmentX, int segmentZ)
{
   return { segmentX - minX, segmentZ - minZ };
}

WorldSegment* World::getSegment(int x, int z, bool initialize)
{
   int neededMinX = 0;
   int neededMaxX = 0;
   int neededMinZ = 0;
   int neededMaxZ = 0;

   if (x < minX) {
      if (!initialize) {
         return nullptr;
      }

      neededMinX = x;
   }
   else if (x >= maxX) {
      if (!initialize) {
         return nullptr;
      }

      neededMaxX = x;
   }

   if (z < minZ) {
      if (!initialize) {
         return nullptr;
      }

      neededMinZ = z;
   }
   else if (z >= maxX) {
      if (!initialize) {
         return nullptr;
      }

      neededMaxZ = z;
   }

   growWorld(neededMinX, neededMaxX + 1, neededMinZ, neededMaxZ + 1);

   auto coords = getLocalSegmentCoordinate(x, z);
   WorldSegment *&seg = loadedSegments[coords.first][coords.second];

   if (seg || !initialize) {
      return seg;
   }

   {
      Timer t("init_chunk");
      seg = new(Ctx) WorldSegment(this, x, z);
   }
   {
      Timer t("gen_world_segment");
      genWorld(*seg);
   }

   return seg;
}

Chunk* World::getChunk(const ChunkPosition &chunkPos, bool initialize)
{
   auto coords = getLocalChunkCoordinate(chunkPos);
   auto *segment = getSegment(coords.segmentX, coords.segmentZ, initialize);

   if (!segment) {
      assert(!initialize);
      return nullptr;
   }

   return &segment->chunks[coords.localChunkX][coords.localChunkZ];
}

const Block *World::getBlock(const WorldPosition &pos)
{
   auto chunkPos = getChunkPosition(pos);
   auto *chunk = getChunk(chunkPos, false);

   if (!chunk) {
      return nullptr;
   }

   return &chunk->getBlockAt(pos);
}

const Block* World::getBlockNeighbour(const Block &block,
                                      BlockNeighbour neighbour) {
   WorldPosition neighbourPos = block.getPosition();
   switch (neighbour) {
   case RightNeighbour:
      ++neighbourPos.x;
      break;
   case LeftNeighbour:
      --neighbourPos.x;
      break;
   case TopNeighbour:
      ++neighbourPos.y;
      break;
   case BottomNeighbour:
      --neighbourPos.y;
      break;
   case FrontNeighbour:
      ++neighbourPos.z;
      break;
   case BackNeighbour:
      --neighbourPos.z;
      break;
   }

   return getBlock(neighbourPos);
}

void World::getBlockNeighbours(const Block &block,
                               std::array<const Block *, 6> &neighbours) {
   neighbours[0] = getBlockNeighbour(block, RightNeighbour);
   neighbours[1] = getBlockNeighbour(block, LeftNeighbour);
   neighbours[2] = getBlockNeighbour(block, TopNeighbour);
   neighbours[3] = getBlockNeighbour(block, BottomNeighbour);
   neighbours[4] = getBlockNeighbour(block, FrontNeighbour);
   neighbours[5] = getBlockNeighbour(block, BackNeighbour);
}

void World::growWorld(int neededMinX, int neededMaxX,
                      int neededMinZ, int neededMaxZ) {
   assert(neededMinX <= neededMaxX);
   assert(neededMinZ <= neededMaxZ);

   int totalMinX = std::min(minX, neededMinX);
   int totalMinZ = std::min(minZ, neededMinZ);

   int totalMaxX = std::max(maxX, neededMaxX);
   int totalMaxZ = std::max(maxZ, neededMaxZ);

   bool growX = neededMinX < minX || neededMaxX > maxX;
   int xSize = totalMaxX - totalMinX;
   if (growX) {
      int negoffset = -totalMinX;

      // Allocate enough space for the new x values.
      loadedSegments =
         (WorldSegment***)realloc(loadedSegments,
                                  xSize * sizeof(WorldSegment**));

      // Shift values to the right if necessary.
      int negXdiff = minX - neededMinX;
      if (negXdiff > 0) {
         std::memmove(loadedSegments + negXdiff, loadedSegments,
                      (maxX - minX) * sizeof(WorldSegment**));

         // Fill new values with nullptr.
         std::memset(loadedSegments, 0, negXdiff * sizeof(WorldSegment**));
      }

      int posXdiff = neededMaxX - maxX;
      if (posXdiff > 0) {
         // Fill new values with nullptr.
         std::memset(loadedSegments + negoffset + maxX, 0,
                     posXdiff * sizeof(WorldSegment**));
      }
   }

   bool growZ = neededMinZ < minZ || neededMaxZ > maxZ;
   if (growX || growZ) {
      int negoffset = -totalMinZ;
      int zSize = totalMaxZ - totalMinZ;
      int posZdiff = neededMaxZ - maxZ;
      int negZdiff = minZ - neededMinZ;

      for (int i = 0; i < xSize; ++i) {
         WorldSegment **&ptr = loadedSegments[i];
         bool loaded = ptr != nullptr;

         if (!growZ && loaded) {
            continue;
         }

         ptr = (WorldSegment**)realloc(ptr, zSize * sizeof(WorldSegment*));

         // We need to set all values to null.
         if (!loaded) {
            std::memset(ptr, 0, zSize * sizeof(WorldSegment*));
         }
         else {
            if (negZdiff > 0) {
               // Shift old values to the right.
               std::memmove((void *) (ptr + negZdiff),
                            (void *) ptr,
                            (maxZ - minZ) * sizeof(WorldSegment*));

               // Set new negative values to nullptr.
               std::memset(ptr, 0, negZdiff * sizeof(WorldSegment*));
            }

            if (posZdiff > 0) {
               // Set new negative values to nullptr.
               std::memset(ptr + negoffset + maxZ, 0,
                           posZdiff * sizeof(WorldSegment*));
            }
         }
      }
   }

   minX = totalMinX;
   maxX = totalMaxX;
   minZ = totalMinZ;
   maxZ = totalMaxZ;
}

void World::updatePlayerPosition(const glm::vec3 &pos)
{
   // Get the position of the player in world coordinates.
   auto playerPosW = getWorldPosition(pos);

   // Get the chunk that the player is located in.
   auto *playerChunk = getChunk(getChunkPosition(playerPosW));

   // If this chunk is already loaded, we're done.
   if (playerChunk == centerChunk) {
      return;
   }

   if (!centerChunk) {
      return loadChunk(playerChunk);
   }

   // Check if the player crossed the threshold for loading new chunks.
   float distance = glm::distance(
      pos, getScenePosition(centerChunk->getCenterWorldPosition()));

   if (distance > chunkUpdateDistanceThreshold) {
      loadChunk(playerChunk);
   }
}

void World::loadChunk(Chunk *chunk)
{
   auto chunkPos = chunk->getChunkPosition();
   int chunkX = chunkPos.x;
   int chunkZ = chunkPos.z;

   unsigned renderDistance = Ctx.gameOptions.renderDistance;
   unsigned k = 0;

   int i = chunkX - renderDistance;
   int maxi = chunkX + renderDistance;

   for (; i < maxi; ++i) {
      int j = chunkZ - renderDistance;
      int maxz = chunkZ + renderDistance;

      for (; j < maxz; ++j) {
         chunksToRender[k++] = getChunk(ChunkPosition(i, j));
      }
   }

   centerChunk = chunk;
   updateVisibility();
}

llvm::ArrayRef<Chunk*> World::getChunksToRender() const
{
   return llvm::ArrayRef<Chunk*>(chunksToRender, numChunksToRender);
}

void World::genWorld(mc::WorldSegment &seg)
{
   switch (options.type) {
   case WorldGenOptions::FLAT:
      genFlat(seg);
      break;
   }
}

void World::genFlat(mc::WorldSegment &seg)
{
   for (auto &chunk : seg.getChunks()) {
      genFlatChunk(chunk);
   }
}

void World::genFlatChunk(mc::Chunk &chunk)
{
   static Block grass = Block::createGrass(Ctx, glm::vec3(0.0f));
   static Block dirt = Block::createGrass(Ctx, glm::vec3(0.0f));
   static Block stone = Block::createGrass(Ctx, glm::vec3(0.0f));

   chunk.fillLayerWith(Ctx, options.seaY, grass);

   for (int layer = options.seaY - 1; layer > -(MC_CHUNK_HEIGHT / 2); --layer) {
      if (layer > options.dirtY) {
         chunk.fillLayerWith(Ctx, layer, dirt);
      }
      else {
         chunk.fillLayerWith(Ctx, layer, stone);
      }
   }
}

void World::updateVisibility()
{
   for (auto *chunk : getChunksToRender()) {
      chunk->updateVisibility();
   }
}

void World::dump()
{
   print(llvm::errs());
}

void World::print(llvm::raw_ostream &OS) const
{
   int i = 0;
   for (int x = minX; x < maxX; ++x) {
      if (i++ > 0) {
         OS << "\n";
      }

      int j = 0;
      for (int z = minZ; z < maxZ; ++z) {
         if (j++ > 0) {
            OS << " ";
         }

         auto *chunk = const_cast<World*>(this)->getChunk(ChunkPosition(x, z),
                                                          false);
         assert(chunk && "unloaded chunk");

         OS << "(" << chunk->getChunkPosition().x << ", "
            << chunk->getChunkPosition().z << ")";
      }
   }
}