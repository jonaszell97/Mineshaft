//
// Created by Jonas Zell on 2019-01-22.
//

#include "mineshaft/World/Block.h"
#include "mineshaft/Context.h"

#include <llvm/ADT/SmallString.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace mc;

Block::Block(glm::vec3 position)
   : blockID(Air), visible(true), boundingBoxComputed(true),
     model(nullptr),
     modelMatrix(glm::translate(glm::mat4(1.0f), position)),
     position(getWorldPosition(position)), boundingBox()
{
   boundingBox.center = glm::vec3(
      modelMatrix * glm::vec4(boundingBox.center, 1.0f));

   boundingBox.corners[0] = glm::vec3(-1.0f, -1.0f, 1.0f);
   boundingBox.corners[1] = glm::vec3(-1.0f, -1.0f, -1.0f);
   boundingBox.corners[2] = glm::vec3(1.0f, -1.0f, 1.0f);
   boundingBox.corners[3] = glm::vec3(1.0f, -1.0f, -1.0f);

   boundingBox.corners[4] = glm::vec3(-1.0f, 1.0f, 1.0f);
   boundingBox.corners[5] = glm::vec3(-1.0f, 1.0f, -1.0f);
   boundingBox.corners[6] = glm::vec3(1.0f, 1.0f, 1.0f);
   boundingBox.corners[7] = glm::vec3(1.0f, 1.0f, -1.0f);

   for (auto &corner : boundingBox.corners) {
      corner = glm::vec3(
         modelMatrix * glm::vec4(corner, 1.0f));
   }
}

Block::Block(BlockID blockID,
             Model *model,
             glm::vec3 position,
             glm::vec3 direction)
   : blockID(blockID), visible(true), boundingBoxComputed(false), model(model),
     modelMatrix(glm::translate(glm::mat4(1.0f), position)),
     position(getWorldPosition(position)), boundingBox(model->getBoundingBox())
{

}

Block::Block(const Block &block, glm::vec3 position, glm::vec3 direction)
   : blockID(block.blockID), visible(true), boundingBoxComputed(false),
     model(block.model), modelMatrix(glm::translate(glm::mat4(1.0f), position)),
     position(getWorldPosition(position)), boundingBox(model->getBoundingBox())
{

}

const BoundingBox& Block::getBoundingBox() const
{
   if (!boundingBoxComputed) {
      boundingBox.center = glm::vec3(
         modelMatrix * glm::vec4(boundingBox.center, 1.0f));

      for (auto &corner : boundingBox.corners) {
         corner = glm::vec3(
            modelMatrix * glm::vec4(corner, 1.0f));
      }

      boundingBoxComputed = true;
   }

   return boundingBox;
}

void Block::render(Context &C,
                   const glm::mat4 &viewProjectionMatrix) const {
   if (is(Block::Air)) {
      return;
   }

   if (isTransparent()) {
      glDisable(GL_DEPTH_TEST);
   }

   const Shader *shader;
   if (usesCubeMap()) {
      shader = &C.getShader(Context::CUBE_SHADER);
   }
   else {
      shader = &C.getShader(Context::BASIC_SHADER);
   }

   model->render(*shader, modelMatrix, viewProjectionMatrix);

   if (isTransparent()) {
      glEnable(GL_DEPTH_TEST);
   }
}

void Block::renderWithBorders(Context &C,
                              const glm::mat4 &viewProjectionMatrix,
                              glm::vec4 borderColor) const {
   if (is(Block::Air)) {
      return;
   }

   const Shader *shader;
   if (usesCubeMap()) {
      shader = &C.getShader(Context::CUBE_SHADER);
   }
   else {
      shader = &C.getShader(Context::BASIC_SHADER);
   }

   model->renderWithBorders(*shader, C.getShader(Context::BORDER_SHADER),
                            borderColor, modelMatrix,
                            viewProjectionMatrix);
}

void Block::renderNormals(mc::Context &C,
                          const glm::mat4 &viewProjectionMatrix,
                          const glm::vec4 &normalColor) const {
   model->renderNormals(C, normalColor, modelMatrix, viewProjectionMatrix);
}

#include "BlockFunctions.inc"