//
// Created by Jonas Zell on 2019-01-22.
//

#ifndef MINESHAFT_CONFIG_H
#define MINESHAFT_CONFIG_H

#include <glm/glm.hpp>

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

/// A point in the rendered scene.
using ScenePosition = glm::vec3;

/// Position of a block within the entire world.
struct WorldPosition {
   int x = 0;
   int y = 0;
   int z = 0;

   WorldPosition(int x, int y, int z)
      : x(x), y(y), z(z)
   { }
};

/// Position of a block within its chunk.
struct BlockPositionChunk {
   int x = 0;
   int y = 0;
   int z = 0;

   BlockPositionChunk(int x, int y, int z)
      : x(x), y(y), z(z)
   { }
};

/// Position of a chunk within the entire world.
struct ChunkPosition {
   int x = 0;
   int z = 0;

   ChunkPosition(int x, int z)
      : x(x), z(z)
   { }
};

inline ScenePosition getScenePosition(const WorldPosition &worldCoord)
{
   return ScenePosition(worldCoord.x * MC_BLOCK_SCALE,
                        worldCoord.y * MC_BLOCK_SCALE,
                        worldCoord.z * MC_BLOCK_SCALE);
}

inline WorldPosition getWorldPosition(const ScenePosition &pos)
{
   return WorldPosition((int)(pos.x / MC_BLOCK_SCALE),
                        (int)(pos.y / MC_BLOCK_SCALE),
                        (int)(pos.z / MC_BLOCK_SCALE));
}

inline ChunkPosition getChunkPosition(const WorldPosition &worldCoord)
{
   return ChunkPosition(worldCoord.x / MC_CHUNK_WIDTH,
                        worldCoord.z / MC_CHUNK_DEPTH);
}

inline ChunkPosition getChunkPosition(const ScenePosition &pos)
{
   return getChunkPosition(getWorldPosition(pos));
}

inline BlockPositionChunk getPositionInChunk(const WorldPosition &pos)
{
   int x = pos.x, y = pos.y, z = pos.z;
   if (x < 0) {
      x = MC_CHUNK_WIDTH - x;
   }
   if (z < 0) {
      z = MC_CHUNK_DEPTH - z;
   }

   return BlockPositionChunk(x % MC_CHUNK_WIDTH, y, z % MC_CHUNK_DEPTH);
}

#endif //MINESHAFT_CONFIG_H
