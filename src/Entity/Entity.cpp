#include "mineshaft/Entity/Entity.h"

#include "mineshaft/Application.h"
#include "mineshaft/World/World.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace mc;

Entity::Entity(EntityKind entityKind,
               const glm::vec3 &position,
               const glm::vec3 &direction,
               const glm::vec3 &size,
               Model *model)
   : entityKind(entityKind), position(position), direction(direction),
     model(model)
{
   float width = size.x;
   float height = size.y;
   float depth = size.z;

   float hwidth = width / 2.0f;
   float hheight = height / 2.0f;
   float hdepth = depth / 2.0f;

   boundingBox.minX = -hwidth;
   boundingBox.maxX = hwidth;

   boundingBox.minY = -hheight;
   boundingBox.maxY = hheight;

   boundingBox.minZ = -hdepth;
   boundingBox.maxZ = hdepth;
}

MovableEntity::MovableEntity(mc::Entity::EntityKind entityKind,
                             const glm::vec3 &position,
                             const glm::vec3 &direction,
                             const glm::vec3 &size,
                             mc::Model *model,
                             float horizontalAngle,
                             float verticalAngle)
   : Entity(entityKind, position, direction, size, model),
     horizontalAngle(horizontalAngle), verticalAngle(verticalAngle)
{

}

glm::mat4 Entity::getModelMatrix() const
{
   return glm::translate(glm::mat4(1.0f), position);
}

bool MovableEntity::wouldCollide(Application &Ctx, const glm::vec3 &newPos) const
{
   if (!collides) {
      return false;
   }

   BoundingBox newBoundingBox = boundingBox.offsetBy(newPos);

   // Check surrounding blocks.
   int blockWidth = std::ceil(boundingBox.maxX - boundingBox.minX / MC_BLOCK_SCALE) + 1;
   int blockHeight = std::ceil((boundingBox.maxY - boundingBox.minY) / MC_BLOCK_SCALE) + 1;
   int blockDepth = std::ceil((boundingBox.maxZ - boundingBox.minZ) / MC_BLOCK_SCALE) + 1;

   auto worldPos = getWorldPosition(newPos);

   for (int x = worldPos.x - blockWidth; x < worldPos.x + blockWidth; ++x) {
      for (int y = worldPos.y - blockHeight; y < worldPos.y + blockHeight; ++y) {
         for (int z = worldPos.z - blockDepth; z < worldPos.z + blockDepth; ++z) {
            auto *block = Ctx.activeWorld->getBlock(WorldPosition(x, y, z));

            if (block && !block->isTransparent()) {
               if (newBoundingBox.collidesWith(block->getBoundingBox())) {
                  return true;
               }
            }
         }
      }
   }

   // TODO consider other entities.
   return false;
}

bool MovableEntity::strafeForward(Application &Ctx, float speed)
{
   float deltaTime = Ctx.getCamera().getElapsedTime();
   glm::vec3 newPos = position;
   switch (movementKind) {
   case MovementKind::Walking:
      newPos.x += direction.x * deltaTime * speed;
      newPos.z += direction.z * deltaTime * speed;
      break;
   case MovementKind::Free:
      newPos += direction * deltaTime * speed;
      break;
   }

   if (wouldCollide(Ctx, newPos)) {
      return false;
   }

   position = newPos;
   return true;
}

bool MovableEntity::strafeBackward(Application &Ctx, float speed)
{
   float deltaTime = Ctx.getCamera().getElapsedTime();
   glm::vec3 newPos = position;
   switch (movementKind) {
   case MovementKind::Walking:
      newPos.x -= direction.x * deltaTime * speed;
      newPos.z -= direction.z * deltaTime * speed;
      break;
   case MovementKind::Free:
      newPos -= direction * deltaTime * speed;
      break;
   }


   if (wouldCollide(Ctx, newPos)) {
      return false;
   }

   position = newPos;
   return true;
}

bool MovableEntity::strafeLeft(Application &Ctx, float speed)
{
   float deltaTime = Ctx.getCamera().getElapsedTime();
   glm::vec3 newPos = position;
   switch (movementKind) {
   case MovementKind::Walking:
      newPos.x -= right.x * deltaTime * speed;
      newPos.z -= right.z * deltaTime * speed;
      break;
   case MovementKind::Free:
      newPos -= right * deltaTime * speed;
      break;
   }

   if (wouldCollide(Ctx, newPos)) {
      return false;
   }

   position = newPos;
   return true;
}

bool MovableEntity::strafeRight(Application &Ctx, float speed)
{
   float deltaTime = Ctx.getCamera().getElapsedTime();
   glm::vec3 newPos = position;
   switch (movementKind) {
   case MovementKind::Walking:
      newPos.x += right.x * deltaTime * speed;
      newPos.z += right.z * deltaTime * speed;
      break;
   case MovementKind::Free:
      newPos += right * deltaTime * speed;
      break;
   }

   if (wouldCollide(Ctx, newPos)) {
      return false;
   }

   position = newPos;
   return true;
}

bool MovableEntity::moveUp(Application &Ctx, float speed)
{
   float deltaTime = Ctx.getCamera().getElapsedTime();
   auto newPos = position + (glm::vec3(0.0f, 1.0f, 0.0f) * deltaTime * speed);
   if (wouldCollide(Ctx, newPos)) {
      return false;
   }

   position = newPos;
   return true;
}

bool MovableEntity::moveDown(Application &Ctx, float speed)
{
   float deltaTime = Ctx.getCamera().getElapsedTime();
   auto newPos = position - (glm::vec3(0.0f, 1.0f, 0.0f) * deltaTime * speed);
   if (wouldCollide(Ctx, newPos)) {
      return false;
   }

   position = newPos;
   return true;
}

void MovableEntity::initiateJump(Application &Ctx, float height, float speed)
{
   verticalVelocity = height * speed;
}

void MovableEntity::updateVectors(Application &Ctx)
{
   // Compute time difference between current and last frame
   float deltaTime = Ctx.getCamera().getElapsedTime();

   // Amount of vertical velocity lost in a second.
   static constexpr float gravity = 9.81f;
   static constexpr float maxVelocity = gravity * 10;
   static constexpr float velocityDecrease = 95.0f;

   if (verticalVelocity == 0) {
      if (movementKind == MovementKind::Walking) {
         verticalVelocity = -gravity;
      }
   }
   else {
      auto newPos = position;
      newPos.y += verticalVelocity * deltaTime;

      if (wouldCollide(Ctx, newPos)) {
         // Restore default negative velocity.
         if (movementKind == MovementKind::Walking) {
            verticalVelocity = -gravity;
         }
         else {
            verticalVelocity = 0.0f;
         }
      }
      else {
         position = newPos;
         verticalVelocity -= velocityDecrease * deltaTime;

         if (verticalVelocity > maxVelocity) {
            verticalVelocity = maxVelocity;
         }
         else if (verticalVelocity < -maxVelocity) {
            verticalVelocity = -maxVelocity;
         }
      }
   }

   switch (movementKind) {
   case MovementKind::Walking: {
      // When walking, only the x and z directions can be chaned.
      direction = glm::vec3(
         cos(verticalAngle) * sin(horizontalAngle),
         sin(verticalAngle),
         cos(verticalAngle) * cos(horizontalAngle)
      );

      // Right vector
      right = glm::vec3(
         sin(horizontalAngle - 3.14f/2.0f),
         0.0f,
         cos(horizontalAngle - 3.14f/2.0f)
      );

      break;
   }
   case MovementKind::Free: {
      // Direction : Spherical coordinates to Cartesian coordinates conversion
      direction = glm::vec3(
         cos(verticalAngle) * sin(horizontalAngle),
         sin(verticalAngle),
         cos(verticalAngle) * cos(horizontalAngle)
      );

      // Right vector
      right = glm::vec3(
         sin(horizontalAngle - 3.14f/2.0f),
         0,
         cos(horizontalAngle - 3.14f/2.0f)
      );

      break;
   }
   }

   // Up vector
   up = glm::cross(right, direction);
}