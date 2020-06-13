#ifndef OPENGLTEST_BLOCK_H
#define OPENGLTEST_BLOCK_H

#include "mineshaft/Config.h"
#include "mineshaft/Model/Model.h"
#include "mineshaft/Shader/Shader.h"
#include "mineshaft/Texture/TextureArray.h"

namespace mc {

class Camera;
class Application;
class Chunk;

class Block {
public:
   enum BlockID {
#  define MC_BLOCK(NAME) NAME,
#  include "mineshaft/World/Blocks.def"
   };

private:
   /// The ID of this block.
   BlockID blockID : 24;

   /// Whether or not the bounding box of this block has been computed.
   mutable bool boundingBoxComputed : 1;

   /// The model matrix of this block, including its position and rotation.
   glm::mat4 modelMatrix;

   /// This block's position.
   WorldPosition position;

   /// The bounding box of this block.
   mutable BoundingBox boundingBox;

public:
   /// Default air block.
   explicit Block(glm::vec3 position);

   /// Create a block.
   Block(BlockID blockID,
         glm::vec3 position,
         glm::vec3 direction);

   /// Create a copy of a block at a different position.
   Block(const Block &block,
         glm::vec3 position,
         glm::vec3 direction = glm::vec3(0.0f,1.0f,0.0f));

   friend class Camera;
   friend class Chunk;

#  define MC_BLOCK(NAME)                                                   \
   static Block create##NAME(Application &app,                             \
                             glm::vec3 position,                           \
                             glm::vec3 direction = glm::vec3(0.0f,1.0f,0.0f));
#  include "mineshaft/World/Blocks.def"

   /// \return true iff blocks of this kind are transparent.
   static bool isTransparent(BlockID ID);

   /// \return true iff this block is transparent.
   bool isTransparent() const { return isTransparent(blockID); }

   /// \return true iff blocks of this kind are solid.
   static bool isSolid(BlockID ID);

   /// \return true iff this block is solid.
   bool isSolid() const { return isSolid(blockID); }

   /// \return true iff this block uses different textures for each face.
   static bool usesCubeMap(BlockID ID);

   /// \return true iff this block uses different textures for each face.
   bool usesCubeMap() const { return usesCubeMap(blockID); }

   /// Describes the faces of a block that should be rendered.
   enum FaceMask {
      F_None    = 0x0,
      F_Right   = 0x1,
      F_Left    = 0x2,
      F_Top     = 0x4,
      F_Bottom  = 0x8,
      F_Front   = 0x10,
      F_Back    = 0x20,

      F_All     = F_Top | F_Bottom | F_Left | F_Right | F_Front | F_Back,
   };

   /// \return The face at index i.
   static FaceMask face(unsigned i) { return FaceMask(1 << i); }

   /// \return the texture UV coordinates.
   static glm::vec2 getTextureUV(BlockID ID, FaceMask face);

   /// \return the texture UV coordinates.
   glm::vec2 getTextureUV(FaceMask face) const { return getTextureUV(blockID, face); }

   /// \return the model matrix of this block.
   const glm::mat4 &getModelMatrix() const { return modelMatrix; }

   /// \return The bounding box of this block.
   const BoundingBox &getBoundingBox() const;

   /// \return This block type's ID.
   BlockID getBlockID() const { return blockID; }

   /// \return true iff this block has the given ID.
   bool is(BlockID blockID) const { return this->blockID == blockID; }

   /// \return The world position of this block.
   WorldPosition getPosition() const { return position; }
};

} // namespace mc

#endif //OPENGLTEST_BLOCK_H
