#include "mineshaft/Application.h"
#include "mineshaft/Entity/Entity.h"
#include "mineshaft/Entity/Player.h"
#include "mineshaft/utils.h"
#include "mineshaft/World/World.h"
#include "mineshaft/World/WorldGenerator.h"

using namespace mc;

WorldSegment::WorldSegment(World *world, int x, int z)
{
   x *= MC_WORLD_SEGMENT_WIDTH;
   z *= MC_WORLD_SEGMENT_DEPTH;

   for (int i = x; i < x + MC_WORLD_SEGMENT_WIDTH; ++i) {
      for (int j = z; j < z + MC_WORLD_SEGMENT_DEPTH; ++j) {
         chunks[i - x][j - z].initialize(world, i, j);
      }
   }
}

World::World(Application &app)
   : app(app), numChunksToRender(
   (2*app.gameOptions.renderDistance+1)*(2*app.gameOptions.renderDistance+1))
{
   chunksToRender = app.Allocate<Chunk*>(numChunksToRender);
   chunkUpdateDistanceThreshold = app.gameOptions.renderDistance * 15.0f;
}

World::World(World &&w) noexcept
   : app(w.app), loadedSegments(w.loadedSegments),
     chunksToRender(w.chunksToRender), numChunksToRender(w.numChunksToRender),
     chunkUpdateDistanceThreshold(w.chunkUpdateDistanceThreshold),
     centerChunk(w.centerChunk), focusedBlock(w.focusedBlock),
     entities(std::move(w.entities)), activeEntities(std::move(w.activeEntities)),
     minX(w.minX), maxX(w.maxX), minZ(w.minZ), maxZ(w.maxZ)
{
   w.loadedSegments = nullptr;
   w.chunksToRender = nullptr;
   w.minX = 0;
   w.maxX = 0;
   w.minZ = 0;
   w.maxZ = 0;
}

World::~World()
{
   int xSize = maxX - minX;
   for (int i = 0; i < xSize; ++i) {
      free(loadedSegments[i]);
   }

   free(loadedSegments);
}

World& World::operator=(mc::World &&w) noexcept
{
   std::swap(w.loadedSegments, loadedSegments);
   std::swap(w.chunksToRender, chunksToRender);
   std::swap(w.numChunksToRender, numChunksToRender);
   std::swap(w.chunkUpdateDistanceThreshold, chunkUpdateDistanceThreshold);
   std::swap(w.centerChunk, centerChunk);
   std::swap(w.focusedBlock, focusedBlock);
   std::swap(w.entities, entities);
   std::swap(w.activeEntities, activeEntities);
   std::swap(w.minX, minX);
   std::swap(w.maxX, maxX);
   std::swap(w.minZ, minZ);
   std::swap(w.maxZ, maxZ);

   return *this;
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
      seg = new(app) WorldSegment(this, x, z);
   }
   {
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

const Block *World::getBlock(const WorldPosition &pos) const
{
   auto chunkPos = getChunkPosition(pos);
   const Chunk *chunk = const_cast<World*>(this)->getChunk(chunkPos, false);

   if (!chunk) {
      return nullptr;
   }

   return chunk->getBlockAt(pos);
}

void World::updateBlock(const mc::WorldPosition &pos,
                        mc::Block &&block,
                        bool delayIfNecessary) {
   auto chunkPos = getChunkPosition(pos);
   auto *chunk = const_cast<World*>(this)->getChunk(chunkPos, false);

   if (!chunk) {
      if (delayIfNecessary) {
         blockUpdates[chunkPos].emplace_back(pos, std::move(block));
      }

      return;
   }

   chunk->updateBlock(pos, std::move(block));
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

void World::updatePlayerPosition()
{
   glm::vec3 pos = app.getPlayer()->getPosition();

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

   unsigned renderDistance = app.gameOptions.renderDistance;
   unsigned k = 0;

//   int i = chunkX - renderDistance;
//   int maxi = chunkX + renderDistance;
//
//   for (; i < maxi; ++i) {
//      int j = chunkZ - renderDistance;
//      int maxz = chunkZ + renderDistance;
//
//      for (; j < maxz; ++j) {
//         chunksToRender[k++] = getChunk(ChunkPosition(i, j));
//      }
//   }

   chunksToRender[k++] = chunk;
   centerChunk = chunk;

   for (int i = 1; i <= renderDistance; ++i) {
      for (int x = -i; x <= i; x += 2*i) {
         for (int z = -i; z <= i; ++z) {
            chunksToRender[k++] = getChunk(ChunkPosition(chunkX + x, chunkZ + z));
         }
      }
      for (int z = -i; z <= i; z += 2*i) {
         for (int x = -i+1; x < i; ++x) {
            chunksToRender[k++] = getChunk(ChunkPosition(chunkX + x, chunkZ + z));
         }
      }
   }

   assert(k == numChunksToRender);

   // Update active entities.
   activeEntities.clear();

   for (auto *e : entities) {
      auto chunkPos = getChunkPosition(e->getPosition());
      if (isChunkVisible(chunkPos)) {
         activeEntities.insert(e);
      }
   }
}

llvm::ArrayRef<Chunk*> World::getChunksToRender() const
{
   return llvm::ArrayRef<Chunk*>(chunksToRender, numChunksToRender);
}

void World::registerEntity(Entity *e)
{
   entities.push_back(e);

   auto chunkPos = getChunkPosition(e->getPosition());
   if (isChunkVisible(chunkPos)) {
      activeEntities.insert(e);
   }
}

bool World::isChunkVisible(const ChunkPosition &pos) const
{
   if (!centerChunk) {
      return false;
   }

   unsigned renderDistance = app.gameOptions.renderDistance;
   auto centerPos = centerChunk->getChunkPosition();

   return pos.x >= centerPos.x - renderDistance
      && pos.x < centerPos.x + renderDistance
      && pos.z >= centerPos.z - renderDistance
      && pos.z < centerPos.z + renderDistance;
}

void World::genWorld(WorldSegment &seg)
{
   for (auto &chunk : seg.getChunks()) {
      // Perform delayed block updates.
      auto it = blockUpdates.find(chunk.getChunkPosition());
      if (it != blockUpdates.end()) {
         for (DelayedBlockUpdate &update : it->second) {
            updateBlock(update.pos, std::move(update.block));
         }

         it->second.clear();
      }

      worldGenerator->generateTerrain(chunk);
      chunk.setModified();
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