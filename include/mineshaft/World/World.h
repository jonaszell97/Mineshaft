#ifndef MINESHAFT_WORLD_H
#define MINESHAFT_WORLD_H

#include "mineshaft/World/Chunk.h"

#include <unordered_map>
#include <unordered_set>

namespace mc {

class Entity;
class WorldGenerator;

/// Stores options for world and terrain generation.
struct WorldGenOptions {
   enum WorldType {
      /// A completely flat world.
      FLAT,

      /// A default world.
      DEFAULT,
   };

   /// The world type.
   WorldType type = DEFAULT;

   /// The seed.
   int seed = 69;

   /// The y coordinate of sea level.
   int seaY = 0;

   /// The y coordinate of the beginning of the dirt level.
   int dirtLayers = 5;

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
   Application &app;

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

   /// The block that is currently looked at.
   const Block *focusedBlock = nullptr;

   /// The terrain generated used in this world.
   WorldGenerator *worldGenerator = nullptr;

   /// The entities contained in the world.
   std::vector<Entity*> entities;

   /// The entities in the currently loaded chunks.
   std::unordered_set<Entity*> activeEntities;

   /// The lowest loaded x segment coordinate.
   int minX = 0;

   /// The highest loaded x segment coordinate.
   int maxX = 0;

   /// The lowest loaded z segment coordinate.
   int minZ = 0;

   /// The highest loaded z segment coordinate.
   int maxZ = 0;

   /// Generate the terrain of the given segment.
   void genWorld(WorldSegment &seg);

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

   // Update to a block that should be performed when the corresponding
   // chunk is generated.
   struct DelayedBlockUpdate {
      WorldPosition pos;
      Block block;

      DelayedBlockUpdate(const WorldPosition &pos, Block &&block)
         : pos(pos), block(std::move(block))
      { }
   };

   // Block updates that should be performed when a chunk is generated.
   std::unordered_map<ChunkPosition, std::vector<DelayedBlockUpdate>> blockUpdates;

public:
   /// C'tor. Does not generate any terrain.
   explicit World(Application &app);
   ~World();

   /// Disable copying.
   World(const World &w) = delete;
   World &operator=(const World &w) = delete;

   World(World &&w) noexcept;
   World &operator=(World &&w) noexcept;

   /// \return A chunk at the specified coordinates.
   Chunk *getChunk(const ChunkPosition &chunkPos, bool initialize = true);

   /// \return A block, if its corresponding chunk is loaded.
   void updateBlock(const WorldPosition &pos, Block &&block,
                    bool delayIfNecessary = true);

   /// \return A block, if its corresponding chunk is loaded.
   const Block *getBlock(const WorldPosition &pos) const;

   /// \return The world generator.
   WorldGenerator *getWorldGenerator() const { return worldGenerator; }

   /// Set the world generator.
   void setWorldGenerator(WorldGenerator *gen) { worldGenerator = gen; }

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
   void updatePlayerPosition();

   /// Update the visibility of chunks and entities.
   void updateVisibility();

   /// Get the currently rendered chunks.
   llvm::ArrayRef<Chunk*> getChunksToRender() const;

   /// Register an entity.
   void registerEntity(Entity *e);

   /// \return The currently active entities.
   const std::unordered_set<Entity*> &getActiveEntities() const { return activeEntities; }

   /// \return The context instance.
   Application &getApplication() const { return app; }

   /// \return The block that is currently looked at.
   const Block *getFocusedBlock() const { return focusedBlock; }

   /// Set the currently focused block.
   void setFocusedBlock(const Block *b) { focusedBlock = b; }

   /// \return True if the given chunk is visible.
   bool isChunkVisible(const ChunkPosition &pos) const;

#ifndef NDEBUG
   void print(llvm::raw_ostream &OS) const;
   void dump();
#endif
};

} // namespace mc

#endif //MINESHAFT_WORLD_H
