//
// Created by Jonas Zell on 2019-01-22.
//

#ifndef MINESHAFT_CHUNK_H
#define MINESHAFT_CHUNK_H

#include "mineshaft/Config.h"
#include "mineshaft/World/Block.h"

namespace mc {

class Chunk;
class World;

class ChunkSegment {
   /// The blocks in this chunk segment.
   unsigned char blockStorage[MC_BLOCKS_PER_CHUNK_SEGMENT * sizeof(Block)];

public:
   ChunkSegment();

   /// \return The blocks contained in this chunk segment.
   llvm::ArrayRef<Block> getBlocks() const
   {
      return { reinterpret_cast<const Block*>(blockStorage),
         MC_BLOCKS_PER_CHUNK_SEGMENT };
   }

   /// \return The blocks contained in this chunk segment.
   llvm::MutableArrayRef<Block> getBlocks()
   {
      return { reinterpret_cast<Block*>(blockStorage),
         MC_BLOCKS_PER_CHUNK_SEGMENT };
   }

   /// \return The block at the specified chunk-local coordinate.
   const Block &getBlockAt(const BlockPositionChunk &pos) const;
   Block &getBlockAt(const BlockPositionChunk &pos);

   const Block &getBlockAt(const WorldPosition &pos) const
   {
      return getBlockAt(getPositionInChunk(pos));
   }

   Block &getBlockAt(const WorldPosition &pos)
   {
      return getBlockAt(getPositionInChunk(pos));
   }
};

class Chunk {
   /// Reference to the world instance.
   World *world;

   /// The segments of this chunk.
   ChunkSegment *chunkSegments[MC_CHUNK_HEIGHT / MC_CHUNK_SEGMENT_HEIGHT];

   /// X position of this chunk with respect to the world.
   int x;

   /// Z position of this chunk with respect to the world.
   int z;

   /// True if the visibility of block faces in this chunk has been calculated.
   bool visibilityCalculated = false;

   /// The bounding box of this chunk.
   BoundingBox boundingBox;

public:
   Chunk();
   Chunk(World *world, int x, int z);

   ~Chunk();

   Chunk(const Chunk&) = delete;
   Chunk &operator=(const Chunk&) = delete;

   Chunk(Chunk&&) noexcept;
   Chunk &operator=(Chunk&&) noexcept;

   /// Initialize the chunk with the given coordinates.
   void initialize(World *world, int x, int z);

   /// \return The block at the specified chunk-local coordinate.
   const Block &getBlockAt(const WorldPosition &pos) const;
   Block &getBlockAt(const WorldPosition &pos);

   /// \return The bounding box of this chunk.
   const BoundingBox &getBoundingBox() { return boundingBox; }

   /// Update the visibility of blocks in this chunk.
   void updateVisibility();

   /// Update visibility of the blocks surrounding the given one.
   void updateVisibility(Block &b);

   /// \return true iff this chunk was modified.
   bool wasModified() const { return !visibilityCalculated; }

   template<class segment_type, class block_type>
   struct block_iterator_t {
   private:
      segment_type segmentPtr;
      segment_type endPtr;
      unsigned indexInSegment;

      void advance()
      {
         if (segmentPtr == endPtr) {
            return;
         }

         // Move on to a valid segment.
         while (!*segmentPtr || indexInSegment >= MC_BLOCKS_PER_CHUNK_SEGMENT) {
            if (++segmentPtr == endPtr) {
               return;
            }

            indexInSegment = 0;
         }

         // Move on to a non-air block within the segment.
         while (true) {
            auto &block = **this;
            if (!block.is(Block::Air)) {
               break;
            }

            if (++indexInSegment >= MC_BLOCKS_PER_CHUNK_SEGMENT) {
               return advance();
            }
         }
      }

   public:
      explicit block_iterator_t(segment_type chunkSegments,
                                segment_type endPtr)
         : segmentPtr(chunkSegments), endPtr(endPtr), indexInSegment(0)
      {
         advance();
      }

      using reference = block_type&;
      using pointer   = block_type*;

      reference operator*() const
      {
         return (*segmentPtr)->getBlocks()[indexInSegment];
      }

      pointer operator->() const
      {
         return &(*segmentPtr)->getBlocks()[indexInSegment];
      }

      block_iterator_t &operator++()
      {
         ++indexInSegment;
         advance();
         return *this;
      }

      const block_iterator_t operator++(int)
      {
         auto copy = *this;
         ++indexInSegment;
         advance();

         return copy;
      }

      bool operator==(const block_iterator_t &rhs) const
      {
         return segmentPtr == rhs.segmentPtr
            && endPtr == rhs.endPtr
            && indexInSegment == rhs.indexInSegment;
      }

      bool operator!=(const block_iterator_t &rhs) const
      {
         return !(*this == rhs);
      }
   };

   using const_block_iterator = block_iterator_t<ChunkSegment *const*, const Block>;
   using block_range = llvm::iterator_range<const_block_iterator>;

   const_block_iterator block_begin() const
   {
      return const_block_iterator(
         chunkSegments,
         chunkSegments + (MC_CHUNK_HEIGHT / MC_CHUNK_SEGMENT_HEIGHT));
   }

   const_block_iterator block_end() const
   {
      return const_block_iterator(
         chunkSegments + (MC_CHUNK_HEIGHT / MC_CHUNK_SEGMENT_HEIGHT),
         chunkSegments + (MC_CHUNK_HEIGHT / MC_CHUNK_SEGMENT_HEIGHT));
   }

   block_range getBlocks() const
   {
      return block_range(block_begin(), block_end());
   }

   /// Iterate through the non-air blocks of this chunk.
   template<class Callback>
   void forAllNonAirBlocks(const Callback &Fn)
   {
      for (auto *seg : chunkSegments) {
         if (!seg) {
            continue;
         }

         for (auto &block : seg->getBlocks()) {
            if (block.is(Block::Air)) {
               continue;
            }

            if (!Fn(block)) {
               return;
            }
         }
      }
   }

   /// \return the segment that the y-coordinate is contained in.
   ChunkSegment *getSegmentForYCoord(int y, bool initialize = true);

   /// \return The world coordinate for a block in this chunk.
   WorldPosition getWorldPosition(const BlockPositionChunk &pos);

   /// \return The center coordinate of this block.
   WorldPosition getCenterWorldPosition() const;

   /// \return The chunk position.
   ChunkPosition getChunkPosition() const;

   void fillLayerWith(Context &Ctx, int y, const Block &block);
};

} // namespace mc

#endif //MINESHAFT_CHUNK_H
