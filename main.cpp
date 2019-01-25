
#include "mineshaft/Context.h"
#include "mineshaft/Model/Model.h"
#include "mineshaft/utils.h"
#include "mineshaft/Texture/TextureAtlas.h"
#include "mineshaft/World/Block.h"
#include "mineshaft/World/Chunk.h"
#include "mineshaft/World/World.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <ctime>

using namespace glm;
using namespace mc;

int main()
{
   Context Ctx;
   if (Ctx.initialize()) {
      return -1;
   }

   GLFWwindow *Win = Ctx.getWindow();
   Camera &Cam = Ctx.getCamera();
   Cam.setPosition(glm::vec3(0.f, 0.f, 5.f));

   WorldGenOptions options{};
   World w(Ctx, options);

   // Set default background color.
   Ctx.setBackgroundColor(glm::vec4(0.686f, 0.933f, 0.933f, 1.0f));

   llvm::SmallVector<const Block*, 4> BlocksToRender;

   bool firstFrame = true;
   do {
      // Clear the screen.
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

      // Compute the (M)VP matrix from keyboard and mouse input
      Cam.computeMatricesFromInputs();
      Cam.renderCrosshair();
      Cam.renderCoordinateSystem();

      w.updatePlayerPosition(Cam.getPosition());

      bool foundChanges = firstFrame;
      for (Chunk *chunk : w.getChunksToRender()) {
         if (Cam.boxInFrustum(chunk->getBoundingBox()) != Camera::Outside) {
            foundChanges |= chunk->wasModified();
            chunk->updateVisibility();

            for (auto &block : chunk->getBlocks()) {
               if (!block.isVisible()) {
                  continue;
               }
               if (Cam.boxInFrustum(block.getBoundingBox()) == Camera::Outside) {
                  continue;
               }

               BlocksToRender.push_back(&block);
            }
         }
      }

      llvm::outs() << "drawing " << BlocksToRender.size() << " blocks\n";
      Cam.renderBlocks(BlocksToRender, foundChanges);

//      unsigned borderIndex = -1;
//      float smallestDistance = FLT_MAX;
//
//      unsigned i = 0;
//      for (const Block *block : BlocksToRender) {
//         if (block->is(Block::Air)) {
//            ++i;
//            continue;
//         }
//
//         auto pointsAt = Cam.pointsAt(*block->getModel(),
//                                      block->getModelMatrix());
//
//         if (pointsAt.first
//         && pointsAt.second <= 125.0f
//         && pointsAt.second < smallestDistance) {
//            smallestDistance = pointsAt.second;
//            borderIndex = i;
//         }
//
//         ++i;
//      }
//
//      i = 0;
//      for (const Block *block : BlocksToRender) {
//         if (i++ != borderIndex) {
//            block->render(Ctx, VP);
//         }
//      }
//
//      if (borderIndex != -1) {
//         const Block *block = BlocksToRender[borderIndex];
//         block->renderWithBorders(Ctx, VP,
//                                  glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
//      }

      // Swap buffers
      glfwSwapBuffers(Win);
      glfwPollEvents();
      BlocksToRender.clear();

#ifdef __APPLE__
      if (firstFrame) {
         int x, y;
         glfwGetWindowPos(Win, &x, &y);
         glfwSetWindowPos(Win, x + 1, y);
         glfwSetWindowPos(Win, x - 1, y);
      }
#endif

      firstFrame = false;

   } // Check if the ESC key was pressed or the window was closed
   while (glfwGetKey(Win, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
          glfwWindowShouldClose(Win) == 0);

   return 0;
}