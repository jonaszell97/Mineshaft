#include "mineshaft/World/Block.h"
#include "mineshaft/Application.h"

#include <llvm/ADT/SmallString.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace mc;

Block::Block(glm::vec3 position)
   : blockID(Air), boundingBoxComputed(true),
     modelMatrix(glm::translate(glm::mat4(1.0f), position)),
     position(getWorldPosition(position)), boundingBox()
{
   boundingBox.minX += position.x;
   boundingBox.maxX += position.x;

   boundingBox.minY += position.y;
   boundingBox.maxY += position.y;

   boundingBox.minZ += position.z;
   boundingBox.maxZ += position.z;
}

Block::Block(BlockID blockID,
             glm::vec3 position,
             glm::vec3 direction)
   : blockID(blockID), boundingBoxComputed(false),
     modelMatrix(glm::translate(glm::mat4(1.0f), position)),
     position(getWorldPosition(position)),
     boundingBox(BoundingBox::unitCube())
{

}

Block::Block(const Block &block, glm::vec3 position, glm::vec3 direction)
   : blockID(block.blockID), boundingBoxComputed(false),
     modelMatrix(glm::translate(glm::mat4(1.0f), position)),
     position(getWorldPosition(position)), boundingBox(block.boundingBox)
{

}

const BoundingBox& Block::getBoundingBox() const
{
   if (!boundingBoxComputed) {
      auto scenePos = getScenePosition(position);
      boundingBox.minX += scenePos.x;
      boundingBox.maxX += scenePos.x;

      boundingBox.minY += scenePos.y;
      boundingBox.maxY += scenePos.y;

      boundingBox.minZ += scenePos.z;
      boundingBox.maxZ += scenePos.z;

      boundingBoxComputed = true;
   }

   return boundingBox;
}

#include "BlockFunctions.inc"