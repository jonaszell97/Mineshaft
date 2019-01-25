//
// Created by Jonas Zell on 2019-01-22.
//

#include "mineshaft/World/Chunk.h"
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
   : world(nullptr), chunkSegments{nullptr}, x(0), z(0)
{
   std::memset(chunkSegments, 0, sizeof(chunkSegments));
}

Chunk::Chunk(World *world, int x, int z)
   : Chunk()
{
   initialize(world, x, z);
}

Chunk::Chunk(mc::Chunk &&Other) noexcept
   : world(Other.world), chunkSegments{}, x(Other.x), z(Other.z)
{
   for (unsigned i = 0; i < (MC_CHUNK_HEIGHT / MC_CHUNK_SEGMENT_HEIGHT); ++i) {
      chunkSegments[i] = Other.chunkSegments[i];
      Other.chunkSegments[i] = nullptr;
   }
}

Chunk& Chunk::operator=(Chunk &&Other) noexcept
{
   this->~Chunk();

   world = Other.world;
   x = Other.x;
   z = Other.z;

   for (unsigned i = 0; i < (MC_CHUNK_HEIGHT / MC_CHUNK_SEGMENT_HEIGHT); ++i) {
      chunkSegments[i] = Other.chunkSegments[i];
      Other.chunkSegments[i] = nullptr;
   }

   return *this;
}

Chunk::~Chunk()
{
   for (auto *seg : chunkSegments) {
      delete seg;
   }
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

   boundingBox.center = glm::vec3(x * width + hwidth, 0.0f, z * width + hdepth);

   boundingBox.corners[0] = boundingBox.center + glm::vec3(-hwidth, -hheight, hdepth);
   boundingBox.corners[1] = boundingBox.center + glm::vec3(-hwidth, -hheight, -hdepth);
   boundingBox.corners[2] = boundingBox.center + glm::vec3(hwidth, -hheight, hdepth);
   boundingBox.corners[3] = boundingBox.center + glm::vec3(hwidth, -hheight, -hdepth);

   boundingBox.corners[4] = boundingBox.center + glm::vec3(-hwidth, hheight, hdepth);
   boundingBox.corners[5] = boundingBox.center + glm::vec3(-hwidth, hheight, -hdepth);
   boundingBox.corners[6] = boundingBox.center + glm::vec3(hwidth, hheight, hdepth);
   boundingBox.corners[7] = boundingBox.center + glm::vec3(hwidth, hheight, -hdepth);
}

const Block &Chunk::getBlockAt(const WorldPosition &pos) const
{
   return const_cast<Chunk*>(this)->getBlockAt(pos);
}

Block &Chunk::getBlockAt(const WorldPosition &pos)
{
   auto *seg = getSegmentForYCoord(pos.y);
   return seg->getBlockAt(getPositionInChunk(pos));
}

ChunkSegment* Chunk::getSegmentForYCoord(int y, bool initialize)
{
   // Account for negative y coordinates.
   y += MC_CHUNK_HEIGHT / 2;

   unsigned idx = std::floor(y / MC_CHUNK_SEGMENT_HEIGHT);
   ChunkSegment *&seg = chunkSegments[idx];

   if (!initialize || seg) {
      return seg;
   }

   seg = new ChunkSegment;
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

void Chunk::fillLayerWith(Context &Ctx, int y, const Block &block)
{
   ChunkSegment *seg = getSegmentForYCoord(y);

   for (unsigned x = 0; x < MC_CHUNK_WIDTH; ++x) {
      for (unsigned z = 0; z < MC_CHUNK_DEPTH; ++z) {
         BlockPositionChunk pos(x, y, z);
         new (&seg->getBlockAt(pos))
            Block(block, getScenePosition(getWorldPosition(pos)));
      }
   }
}

void Chunk::updateVisibility()
{
   if (visibilityCalculated) {
      return;
   }

   std::array<const Block*, 6> neighbours{};
   visibilityCalculated = true;

   unsigned segmentY = 0;
   for (auto *seg : chunkSegments) {
      if (!seg) {
         ++segmentY;
         continue;
      }

      for (auto &block : seg->getBlocks()) {
         // A block is visible if any of its neighbours are transparent.
         world->getBlockNeighbours(block, neighbours);

         bool visible = false;
         for (auto &neighbour : neighbours) {
            if (neighbour && neighbour->isTransparent()) {
               visible = true;
               break;
            }
         }

         block.setVisible(visible);
      }
   }
}

void Chunk::updateVisibility(mc::Block &block)
{
   std::array<const Block*, 6> neighbours{};

   // A block is visible if any of its neighbours are transparent.
   world->getBlockNeighbours(block, neighbours);

   bool visible = false;
   for (auto &neighbour : neighbours) {
      if (neighbour && neighbour->isTransparent()) {
         visible = true;
         break;
      }
   }

   block.setVisible(visible);
}