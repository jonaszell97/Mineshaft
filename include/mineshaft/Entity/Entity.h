#ifndef MINESHAFT_ENTITY_H
#define MINESHAFT_ENTITY_H

#include "mineshaft/Config.h"
#include "mineshaft/Model/Model.h"

namespace mc {

class Model;

class Entity {
public:
   /// The entity kind.
   enum EntityKind {
#  define MC_ENTITY(NAME) NAME,
#  include "Entities.inc"
   };

protected:
   Entity(EntityKind entityKind,
          const glm::vec3 &position,
          const glm::vec3 &direction,
          const glm::vec3 &size,
          Model *model);

   /// The kind of entity.
   EntityKind entityKind;

   /// The current position of the entity.
   ScenePosition position;

   /// The current direction of the entity.
   glm::vec3 direction;

   /// The model of this entity.
   Model *model;

   /// The bounding box of this entity.
   BoundingBox boundingBox;

public:
   /// isa / cast implementation.
   static bool classof(Entity*) { return true; }

   /// \return The entity kind.
   EntityKind getKind() const { return entityKind; }

   /// \return The model matrix of this entity.
   glm::mat4 getModelMatrix() const;

   /// \return The current position of the entity.
   const glm::vec3 &getPosition() const { return position; }

   /// \return The current direction of the entity.
   const glm::vec3 &getDirection() const { return direction; }

   /// \return The model of the entity.
   Model *getModel() const { return model; }

   /// \return The bounding box of this model.
   const BoundingBox &getBoundingBox() { return boundingBox; }

   /// \return The width of the entity.
   float getWidth() const { return boundingBox.maxX - boundingBox.minX; }

   /// \return The height of the entity.
   float getHeight() const { return boundingBox.maxY - boundingBox.minY; }

   /// \return The depth of the entity.
   float getDepth() const { return boundingBox.maxZ - boundingBox.minZ; }
};

class MovableEntity: public Entity {
protected:
   MovableEntity(EntityKind entityKind,
                 const glm::vec3 &position,
                 const glm::vec3 &direction,
                 const glm::vec3 &size,
                 Model *model,
                 float horizontalAngle = 3.14f,
                 float verticalAngle = 0.0f);

   /// The current up vector.
   glm::vec3 up;

   /// The current right vector.
   glm::vec3 right;

   /// The current horizontal angle.
   float horizontalAngle;

   /// The current vertical angle.
   float verticalAngle;

   /// The current vertical velocity.
   float verticalVelocity = 0.0f;

   /// Whether or not this entity collides with other objects.
   bool collides = true;

   /// Update the directional vectors.
   void updateVectors(Application &Ctx);

public:
   enum class MovementKind {
      Walking,
      Free,
   };

protected:
   /// The kind of movement that is currently active.
   MovementKind movementKind = MovementKind::Walking;

public:
   /// Update player position and direction.
   void setPosition(const glm::vec3 &position) { this->position = position; }
   void setDirection(const glm::vec3 &dir) { this->direction = dir; }

   /// \return The current movement kind.
   MovementKind getMovementKind() const { return movementKind; }

   /// Set the movement kind.
   void setMovementKind(MovementKind k) { movementKind = k; }

   /// \return The current right vector of the player.
   const glm::vec3 &getRightVector() const { return right; }

   /// \return The current up vector of the player.
   const glm::vec3 &getUpVector() const { return up; }

   /// \return true iff the given position would collide with another object.
   bool wouldCollide(Application &Ctx, const glm::vec3 &newPos) const;

   /// Move forward along the direction vector.
   bool strafeForward(Application &Ctx, float speed);

   /// Move backward along the direction vector.
   bool strafeBackward(Application &Ctx, float speed);

   /// Move left along the direction vector.
   bool strafeLeft(Application &Ctx, float speed);

   /// Move forward along the direction vector.
   bool strafeRight(Application &Ctx, float speed);

   /// Move straight up.
   bool moveUp(Application &Ctx, float speed);

   /// Move straight down.
   bool moveDown(Application &Ctx, float speed);

   /// Initiate a jump to the specified height.
   void initiateJump(Application &Ctx, float height, float speed);
};

} // namespace mc

#endif //MINESHAFT_ENTITY_H
