#ifndef MINESHAFT_CONFIG_H
#define MINESHAFT_CONFIG_H

#include <glm/glm.hpp>

#define NEAR_CLIPPING_DISTANCE 0.1f
#define FAR_CLIPPING_DISTANCE 200.0f
#define MC_BLOCK_SCALE 2.0f

#define MC_TEXTURE_WIDTH       16
#define MC_TEXTURE_HEIGHT      16
#define MC_TEXTURE_ATLAS_WIDTH 8

#define MC_WORLD_SEGMENT_WIDTH  5
#define MC_WORLD_SEGMENT_DEPTH  5

#define MC_CHUNK_WIDTH          16
#define MC_CHUNK_DEPTH          16
#define MC_CHUNK_SEGMENT_HEIGHT 16
#define MC_CHUNK_HEIGHT         256
#define MC_BLOCKS_PER_CHUNK_SEGMENT (MC_CHUNK_WIDTH * MC_CHUNK_SEGMENT_HEIGHT * MC_CHUNK_DEPTH)

#define MC_LOADED_CHUNKS_HORIZONTAL 10
#define MC_LOADED_CHUNKS_VERTICAL   10

namespace mc {

/// A point in the rendered scene.
using ScenePosition = glm::vec3;

/// Position of a block within the entire world.
struct WorldPosition {
   int x = 0;
   int y = 0;
   int z = 0;

   WorldPosition(int x = 0, int y = 0, int z = 0)
      : x(x), y(y), z(z)
   { }


   bool operator==(const WorldPosition &rhs) const
   {
      return x == rhs.x && y == rhs.y && z == rhs.z;
   }

   bool operator!=(const WorldPosition &rhs) const
   {
      return !(*this == rhs);
   }
};

/// Position of a block within its chunk.
struct BlockPositionChunk {
   int x = 0;
   int y = 0;
   int z = 0;

   BlockPositionChunk(int x = 0, int y = 0, int z = 0)
      : x(x), y(y), z(z)
   { }

   bool operator==(const BlockPositionChunk &rhs) const
   {
      return x == rhs.x && y == rhs.y && z == rhs.z;
   }

   bool operator!=(const BlockPositionChunk &rhs) const
   {
      return !(*this == rhs);
   }
};

/// Position of a chunk within the entire world.
struct ChunkPosition {
   int x = 0;
   int z = 0;

   ChunkPosition(int x = 0, int z = 0)
      : x(x), z(z)
   { }

   bool operator==(const ChunkPosition &rhs) const
   {
      return x == rhs.x && z == rhs.z;
   }

   bool operator!=(const ChunkPosition &rhs) const
   {
      return !(*this == rhs);
   }
};

inline ScenePosition getScenePosition(const WorldPosition &worldCoord)
{
   return ScenePosition(worldCoord.x * MC_BLOCK_SCALE,
                        worldCoord.y * MC_BLOCK_SCALE,
                        worldCoord.z * MC_BLOCK_SCALE);
}

inline WorldPosition getWorldPosition(const ScenePosition &pos)
{
   int x, y, z;
   if (pos.x < 0) {
      x = (int)std::floor(pos.x / MC_BLOCK_SCALE);
   }
   else {
      x = (int)(pos.x / MC_BLOCK_SCALE);
   }
   if (pos.y < 0) {
      y = (int)std::floor(pos.y / MC_BLOCK_SCALE);
   }
   else {
      y = (int)(pos.y / MC_BLOCK_SCALE);
   }
   if (pos.z < 0) {
      z = (int)std::floor(pos.z / MC_BLOCK_SCALE);
   }
   else {
      z = (int)(pos.z / MC_BLOCK_SCALE);
   }

   return WorldPosition(x, y, z);
}

inline ChunkPosition getChunkPosition(const WorldPosition &worldCoord)
{
   int x, z;
   if (worldCoord.x < 0) {
      x = (int) std::floor((float) worldCoord.x / MC_CHUNK_WIDTH);
   }
   else {
      x = worldCoord.x / MC_CHUNK_WIDTH;
   }

   if (worldCoord.z < 0) {
      z = (int) std::floor((float) worldCoord.z / MC_CHUNK_DEPTH);
   }
   else {
      z = worldCoord.z / MC_CHUNK_DEPTH;
   }

   return ChunkPosition(x, z);
}

inline ChunkPosition getChunkPosition(const ScenePosition &pos)
{
   return getChunkPosition(getWorldPosition(pos));
}

inline BlockPositionChunk getPositionInChunk(const WorldPosition &pos)
{
   int x = pos.x, y = pos.y, z = pos.z;
   if (x < 0) {
      x = MC_CHUNK_WIDTH + (pos.x % MC_CHUNK_WIDTH);
   }
   else {
      x = pos.x;
   }
   if (z < 0) {
      z = MC_CHUNK_DEPTH + (pos.z % MC_CHUNK_DEPTH);
   }
   else {
      z = pos.z;
   }

   return BlockPositionChunk(x % MC_CHUNK_WIDTH, y, z % MC_CHUNK_DEPTH);
}

} // namespace mc

#endif //MINESHAFT_CONFIG_H
