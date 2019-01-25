//
// Created by Jonas Zell on 2019-01-23.
//

#ifndef MINESHAFT_WORLD_H
#define MINESHAFT_WORLD_H

#include "mineshaft/World/Chunk.h"

namespace mc {

/// Stores options for world and terrain generation.
struct WorldGenOptions {
   enum WorldType {
      /// A completely flat world.
      FLAT,
   };

   /// The world type.
   WorldType type = FLAT;

   /// The y coordinate of sea level.
   int seaY = 0;

   /// The y coordinate of the beginning of the dirt level.
   int dirtY = -5;

   /// The y coordinate of the cloud layer.
   int cloudY = 100;
};

struct WorldSegment {
   WorldSegment(World *world, int x, int z);

   /// \return An array ref containing all chunks in the segment.
   llvm::ArrayRef<Chunk> getChunks() const
   {
      return llvm::ArrayRef<Chunk>(
         reinterpret_cast<const Chunk*>(&chunks),
         MC_WORLD_SEGMENT_WIDTH * MC_WORLD_SEGMENT_DEPTH);
   }

   /// \return An array ref containing all chunks in the segment.
   llvm::MutableArrayRef<Chunk> getChunks()
   {
      return llvm::MutableArrayRef<Chunk>(
         reinterpret_cast<Chunk*>(&chunks),
         MC_WORLD_SEGMENT_WIDTH * MC_WORLD_SEGMENT_DEPTH);
   }

   /// Storage for Chunks in this segment.
   Chunk chunks[MC_WORLD_SEGMENT_WIDTH][MC_WORLD_SEGMENT_DEPTH];
};

class World {
   /// Reference to the context instance.
   Context &Ctx;

   /// Storage for currently loaded world segments. This is structured like a
   /// two-dimensional array.
   WorldSegment ***loadedSegments = nullptr;

   /// Chunks that will be rendered based on the current player position.
   Chunk **chunksToRender = nullptr;

   /// The number of chunks that are rendered at a time.
   unsigned numChunksToRender = 0;

   /// The distance after which new chunks are loaded into memory.
   float chunkUpdateDistanceThreshold = 0.0f;

   /// The chunk that rendered area is centered around.
   Chunk *centerChunk = nullptr;

   /// The lowest loaded x segment coordinate.
   int minX = 0;

   /// The highest loaded x segment coordinate.
   int maxX = 0;

   /// The lowest loaded z segment coordinate.
   int minZ = 0;

   /// The highest loaded z segment coordinate.
   int maxZ = 0;

   /// Options for world generation.
   WorldGenOptions options;

   /// Generate the terrain of the given segment.
   void genWorld(WorldSegment &seg);

   /// Generate a flat world in the given segment.
   void genFlat(WorldSegment &seg);
   void genFlatChunk(Chunk &chunk);

   struct ChunkIndex {
      int segmentX;
      int segmentZ;
      int localChunkX;
      int localChunkZ;
   };

   /// Get the unsigned vector index for a signed chunk coordinate.
   ChunkIndex getLocalChunkCoordinate(const ChunkPosition &chunkPos);

   /// Get the unsigned vector index for a signed segment coordinate.
   std::pair<int, int> getLocalSegmentCoordinate(int segmentX, int segmentZ);

   /// Load new chunks and generate their terrain.
   void growWorld(int neededMinX, int neededMaxX,
                  int neededMinZ, int neededMaxZ);

   /// Load a chunk as the center of the visible area.
   void loadChunk(Chunk *chunk);

public:
   /// C'tor. Does not generate any terrain.
   explicit World(Context &Ctx, const WorldGenOptions &options);
   ~World();

   /// \return A chunk at the specified coordinates.
   Chunk *getChunk(const ChunkPosition &chunkPos, bool initialize = true);

   /// \return A block, if its corresponding chunk is loaded.
   const Block *getBlock(const WorldPosition &pos);

   enum BlockNeighbour {
      RightNeighbour,
      LeftNeighbour,
      TopNeighbour,
      BottomNeighbour,
      FrontNeighbour,
      BackNeighbour,
   };

   /// \return The neighbour of a block, if its corresponding chunk is loaded.
   const Block *getBlockNeighbour(const Block &block, BlockNeighbour neighbour);

   /// \return The neighbour of a block, if its corresponding chunk is loaded.
   void getBlockNeighbours(const Block &block,
                           std::array<const Block*, 6> &neighbours);

   /// \return A segment at the specified coordinates.
   WorldSegment *getSegment(int x, int z, bool initialize = true);

   /// Potentially update the rendered chunks based on the players position.
   void updatePlayerPosition(const glm::vec3 &pos);

   /// Update the visibility of chunks.
   void updateVisibility();

   /// Get the currently rendered chunks.
   llvm::ArrayRef<Chunk*> getChunksToRender() const;

#ifndef NDEBUG
   void print(llvm::raw_ostream &OS) const;
   void dump();
#endif
};

} // namespace mc

#endif //MINESHAFT_WORLD_H
