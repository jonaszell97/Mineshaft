#include "mineshaft/World/Chunk.h"

#include "mineshaft/Application.h"
#include "mineshaft/World/World.h"

using namespace mc;

ChunkSegment::ChunkSegment()
{
   std::memset(blockStorage, 0, sizeof(blockStorage));
}

Block &ChunkSegment::getBlockAt(const BlockPositionChunk &pos)
{
   int x = pos.x, y = pos.y, z = pos.z;
   y += MC_CHUNK_HEIGHT / 2;
   y %= MC_CHUNK_SEGMENT_HEIGHT;

   auto *blockPtr = reinterpret_cast<Block*>(&blockStorage);
   return blockPtr[x + MC_CHUNK_WIDTH * (y + MC_CHUNK_SEGMENT_HEIGHT * z)];
}

const Block &ChunkSegment::getBlockAt(const BlockPositionChunk &pos) const
{
   return const_cast<ChunkSegment*>(this)->getBlockAt(pos);
}

Chunk::Chunk()
   : world(nullptr), chunkSegments{}, x(0), z(0)
{
   std::memset(chunkSegments, 0, sizeof(chunkSegments));
}

Chunk::Chunk(World *world, int x, int z)
   : Chunk()
{
   initialize(world, x, z);
}

Chunk::Chunk(mc::Chunk &&Other) noexcept
   : Chunk()
{
   std::swap(world, Other.world);
   std::swap(x, Other.x);
   std::swap(z, Other.z);
   std::swap(chunkMesh, Other.chunkMesh);
   std::swap(boundingBox, Other.boundingBox);

   for (unsigned i = 0; i < (MC_CHUNK_HEIGHT / MC_CHUNK_SEGMENT_HEIGHT); ++i) {
      std::swap(chunkSegments[i], Other.chunkSegments[i]);
   }
}

Chunk& Chunk::operator=(Chunk &&Other) noexcept
{
   std::swap(world, Other.world);
   std::swap(x, Other.x);
   std::swap(z, Other.z);
   std::swap(chunkMesh, Other.chunkMesh);
   std::swap(boundingBox, Other.boundingBox);

   for (unsigned i = 0; i < (MC_CHUNK_HEIGHT / MC_CHUNK_SEGMENT_HEIGHT); ++i) {
      std::swap(chunkSegments[i], Other.chunkSegments[i]);
   }

   return *this;
}

void Chunk::initialize(World *world, int x, int z)
{
   this->world = world;
   this->x = x;
   this->z = z;

   static constexpr float width = MC_CHUNK_WIDTH * MC_BLOCK_SCALE;
   static constexpr float height = MC_CHUNK_HEIGHT * MC_BLOCK_SCALE;
   static constexpr float depth = MC_CHUNK_DEPTH * MC_BLOCK_SCALE;

   static constexpr float hwidth = width / 2.0f;
   static constexpr float hheight = height / 2.0f;
   static constexpr float hdepth = depth / 2.0f;

   boundingBox.minX = -hwidth;
   boundingBox.maxX = hwidth;

   boundingBox.minY = -hheight;
   boundingBox.maxY = hheight;

   boundingBox.minZ = -hdepth;
   boundingBox.maxZ = hdepth;

   boundingBox.applyOffset(glm::vec3(x * width + hwidth, 0.0f, z * depth + hdepth));
}

const Block *Chunk::getBlockAt(const WorldPosition &pos) const
{
   return const_cast<Chunk*>(this)->getBlockAt(pos);
}

Block *Chunk::getBlockAt(const WorldPosition &pos)
{
   if (pos.x >= (this->x + 1) * MC_CHUNK_WIDTH
   || pos.x < this->x * MC_CHUNK_WIDTH
   || pos.z >= (this->z + 1) * MC_CHUNK_DEPTH
   || pos.z < this->z * MC_CHUNK_DEPTH) {
      return nullptr;
   }

   auto *seg = getSegmentForYCoord(pos.y);
   if (!seg) {
      return nullptr;
   }

   return &seg->getBlockAt(getPositionInChunk(pos));
}

void Chunk::updateBlock(const mc::WorldPosition &pos,
                        mc::Block &&block,
                        bool recheckVisibility) {
   if (pos.x >= (this->x + 1) * MC_CHUNK_WIDTH
       || pos.x < this->x * MC_CHUNK_WIDTH
       || pos.z >= (this->z + 1) * MC_CHUNK_DEPTH
       || pos.z < this->z * MC_CHUNK_DEPTH) {
      return;
   }

   auto *seg = getSegmentForYCoord(pos.y);
   if (!seg) {
      return;
   }

   seg->airOnly &= block.is(Block::Air);

   Block &blockRef = seg->getBlockAt(getPositionInChunk(pos));
   blockRef = std::move(block);

   if (recheckVisibility) {
      modifiedBlock(getPositionInChunk(pos));
   }
}

void Chunk::modifiedBlock(const mc::BlockPositionChunk &pos)
{
   visibilityCalculated = false;

   // Check bordering chunks.
   if (pos.x == 0) {
      auto *other = world->getChunk(ChunkPosition(this->x - 1, this->z), false);
      if (other) {
         other->visibilityCalculated = false;
      }
   }
   if (pos.x == 15) {
      auto *other = world->getChunk(ChunkPosition(this->x + 1, this->z), false);
      if (other) {
         other->visibilityCalculated = false;
      }
   }

   if (pos.z == 0) {
      auto *other = world->getChunk(ChunkPosition(this->x, this->z - 1), false);
      if (other) {
         other->visibilityCalculated = false;
      }
   }
   if (pos.z == 15) {
      auto *other = world->getChunk(ChunkPosition(this->x, this->z + 1), false);
      if (other) {
         other->visibilityCalculated = false;
      }
   }
}

ChunkSegment* Chunk::getSegmentForYCoord(int y, bool initialize)
{
   if (y >= (MC_CHUNK_HEIGHT / 2) || y < -(MC_CHUNK_HEIGHT / 2)) {
      return nullptr;
   }

   // Account for negative y coordinates.
   y += MC_CHUNK_HEIGHT / 2;

   unsigned idx = std::floor(y / MC_CHUNK_SEGMENT_HEIGHT);
   ChunkSegment *&seg = chunkSegments[idx];

   if (!initialize || seg) {
      return seg;
   }

   seg = new(world->getApplication()) ChunkSegment;
   return seg;
}

WorldPosition Chunk::getWorldPosition(const BlockPositionChunk &pos)
{
   return WorldPosition((pos.x + (this->x * MC_CHUNK_WIDTH)),
                        pos.y,
                        (pos.z + (this->z * MC_CHUNK_DEPTH)));
}

WorldPosition Chunk::getCenterWorldPosition() const
{
   return WorldPosition((x * MC_CHUNK_WIDTH) + (MC_CHUNK_WIDTH / 2),
                        0,
                        (z * MC_CHUNK_DEPTH) + (MC_CHUNK_DEPTH / 2));
}

ChunkPosition Chunk::getChunkPosition() const
{
   return ChunkPosition(x, z);
}

void Chunk::fillLayerWith(Application &Ctx, int y, const Block &block, unsigned holeFrequency)
{
   ChunkSegment *seg = getSegmentForYCoord(y);

   for (unsigned x = 0; x < MC_CHUNK_WIDTH; ++x) {
      for (unsigned z = 0; z < MC_CHUNK_DEPTH; ++z) {
         BlockPositionChunk pos(x, y, z);

         if (!holeFrequency || rand() > RAND_MAX / holeFrequency) {
            new(&seg->getBlockAt(pos))
               Block(block, getScenePosition(getWorldPosition(pos)));
         }
      }
   }
}

static void visitBlockNeighbours(World *world, Chunk *chunk,
                                 Block *blockPtr, int segmentMax, int segmentMin,
                                 int ox, int oy, int oz,
                                 bool isWater, bool &foundTransparentBlock,
                                 unsigned &faceMask) {
   static constexpr int offsets[][3] = {
      { 1, 0, 0 },  // Right
      { -1, 0, 0 }, // Left
      { 0, 1, 0 },  // Top
      { 0, -1, 0 }, // Bottom
      { 0, 0, 1 },  // Front
      { 0, 0, -1 }, // Back
   };

   unsigned i = 0;
   for (auto &offset : offsets) {
      int x = ox + offset[0], y = oy + offset[1], z = oz + offset[2];

      int indexY = y;
      indexY += MC_CHUNK_HEIGHT / 2;
      indexY %= MC_CHUNK_SEGMENT_HEIGHT;

      const Block *neighbour;
      if (x < 0 || x >= MC_CHUNK_WIDTH
          || z < 0 || z >= MC_CHUNK_WIDTH
          || y < segmentMin || y >= segmentMax) {
         auto worldPos = chunk->getWorldPosition(BlockPositionChunk(x, y, z));
         neighbour = world->getBlock(worldPos);
      }
      else {
         neighbour = &blockPtr[x + MC_CHUNK_WIDTH * (indexY + MC_CHUNK_SEGMENT_HEIGHT * z)];
      }

      if (neighbour && neighbour->isTransparent()) {
         // Don't render water blocks next to other water blocks.
         if (!isWater || !neighbour->is(Block::Water)) {
            faceMask |= Block::face(i);
         }

         foundTransparentBlock = true;
      }
      else if (!neighbour) {
         foundTransparentBlock = true;
      }

      ++i;
   }
}

void Chunk::updateVisibility()
{
   if (visibilityCalculated) {
      return;
   }

   auto &app = world->getApplication();

   visibilityCalculated = true;
   chunkMesh = ChunkMesh();

//   Timer timer("Creating chunk mesh");

   int y = (MC_CHUNK_HEIGHT / 2) - 1;
   bool done = false;

   for (int segNo = (MC_CHUNK_HEIGHT / MC_CHUNK_SEGMENT_HEIGHT) - 1; segNo >= 0; --segNo) {
      auto *seg = chunkSegments[segNo];
      if (!seg || seg->airOnly) {
         y -= MC_CHUNK_SEGMENT_HEIGHT;
         continue;
      }

      Block *blockPtr = seg->getBlocks().data();

      int segmentMax = y;
      int endY = y - MC_CHUNK_SEGMENT_HEIGHT;
      while (y > endY) {
         bool foundTransparentBlock = false;

         int indexY = y;
         indexY += MC_CHUNK_HEIGHT / 2;
         indexY %= MC_CHUNK_SEGMENT_HEIGHT;

         for (int x = 0; x < MC_CHUNK_WIDTH; ++x) {
            for (int z = 0; z < MC_CHUNK_DEPTH; ++z) {
               auto &block = blockPtr[x + MC_CHUNK_WIDTH * (indexY + MC_CHUNK_SEGMENT_HEIGHT * z)];
               if (block.is(Block::Air)) {
                  foundTransparentBlock = true;
                  continue;
               }

               foundTransparentBlock |= block.isTransparent();

               unsigned faceMask = Block::F_None;
               visitBlockNeighbours(world, this, blockPtr, segmentMax, endY, x,
                                    y, z, block.is(Block::Water),
                                    foundTransparentBlock, faceMask);

               if (faceMask != Block::F_None) {
                  chunkMesh.addFace(app, block, faceMask);
               }
            }
         }

         if (!foundTransparentBlock) {
            done = true;
            break;
         }

         --y;
      }

      if (done) {
         break;
      }
   }
}