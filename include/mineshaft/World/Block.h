//
// Created by Jonas Zell on 2019-01-22.
//

#ifndef OPENGLTEST_BLOCK_H
#define OPENGLTEST_BLOCK_H

#include "mineshaft/Config.h"
#include "mineshaft/Model/Model.h"
#include "mineshaft/Shader/Shader.h"
#include "mineshaft/Texture/TextureArray.h"

namespace mc {

class Camera;
class Context;
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

   /// Whether or not this block is currently visible.
   bool visible : 1;

   /// Whether or not the bounding box of this block has been computed.
   mutable bool boundingBoxComputed : 1;

   /// The model of this block.
   Model *model;

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
         Model *model,
         glm::vec3 position,
         glm::vec3 direction);

   /// Create a copy of a block at a different position.
   Block(const Block &block,
         glm::vec3 position,
         glm::vec3 direction = glm::vec3(0.0f,1.0f,0.0f));

   friend class Camera;
   friend class Chunk;

#  define MC_BLOCK(NAME)                                                   \
   static Block create##NAME(Context &C,                                   \
                             glm::vec3 position,                           \
                             glm::vec3 direction = glm::vec3(0.0f,1.0f,0.0f));
#  include "mineshaft/World/Blocks.def"

   /// \return true iff blocks of this kind are transparent.
   static bool isTransparent(BlockID ID);

   /// \return true iff this block is transparent.
   bool isTransparent() const { return isTransparent(blockID); }

   /// \return true iff this block uses different textures for each face.
   static bool usesCubeMap(BlockID ID);

   /// \return true iff this block uses different textures for each face.
   bool usesCubeMap() const { return usesCubeMap(blockID); }

   /// \return the texture index of the block.
   static unsigned getTextureLayer(BlockID ID);

   /// \return true iff this block uses different textures for each face.
   unsigned getTextureLayer() const { return getTextureLayer(blockID); }

   /// Load the block textures.
   static TextureArray loadBlockTextures();

   /// Load the block textures.
   static TextureArray loadBlockCubeMapTextures();

   /// \return true iff this block is visible.
   bool isVisible() const { return visible; }

   /// Set if this block should be visible.
   void setVisible(bool v = true) { visible = v; }

   /// Describes the faces of a block that should be rendered.
   enum FaceMask {
      F_None    = 0x0,
      F_Top     = 0x1,
      F_Bottom  = 0x2,
      F_Left    = 0x4,
      F_Right   = 0x8,
      F_Front   = 0x10,
      F_Back    = 0x20,

      F_All     = F_Top | F_Bottom | F_Left | F_Right | F_Front | F_Back,
   };

   /// Render this block.
   void render(Context &C,
               const glm::mat4 &viewProjectionMatrix) const;

   /// Render this block with borders.
   void renderWithBorders(Context &C,
                          const glm::mat4 &viewProjectionMatrix,
                          glm::vec4 borderColor) const;

   /// Render this block's normals
   void renderNormals(Context &C,
                      const glm::mat4 &viewProjectionMatrix,
                      const glm::vec4 &normalColor) const;

   /// Render the specified faces of this block.
   void render(const Shader &shader,
               const glm::mat4 &viewProjectionMatrix,
               unsigned faceMask) const;

   /// \return the model of this block.
   Model *getModel() const { return model; }

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
