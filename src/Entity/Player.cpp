#include "mineshaft/Entity/Player.h"

#include "mineshaft/Application.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace mc;

Player::Player(const glm::vec3 &position,
               const glm::vec3 &direction,
               const glm::vec3 &size,
               mc::Model *model)
   : MovableEntity(Entity::Player, position, direction, size, model)
{

}

void Player::updateViewingDirection(Application &Ctx)
{
   auto *window = Ctx.getWindow();

   // Get mouse position
   double xpos, ypos;
   glfwGetCursorPos(window, &xpos, &ypos);

   auto &camera = Ctx.getCamera();
   unsigned windowWidth = camera.getViewportWidth();
   unsigned windowHeight = camera.getViewportHeight();

   // Reset mouse position for next frame
   glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);

   // Compute new orientation
   horizontalAngle += Ctx.controlOptions.mouseSpeed * float(windowWidth/2 - xpos);
   verticalAngle += Ctx.controlOptions.mouseSpeed * float(windowHeight/2 - ypos);

   if (verticalAngle >= 3.14f) {
      verticalAngle = 3.14f;
   }
   if (verticalAngle <= -3.14f) {
      verticalAngle = -3.14f;
   }

   MovableEntity::updateVectors(Ctx);
}