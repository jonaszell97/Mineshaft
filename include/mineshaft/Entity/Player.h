#ifndef MINESHAFT_PLAYER_H
#define MINESHAFT_PLAYER_H

#include "mineshaft/Entity/Entity.h"

namespace mc {

class Application;

class Player: public MovableEntity {
public:
   Player(const glm::vec3 &position,
          const glm::vec3 &direction,
          const glm::vec3 &size,
          Model *model);

   /// isa / cast implementation.
   static bool classof(Entity *e)
   {
      return e->getKind() == Entity::Player;
   }

   /// Update viewing direction based on mouse movement.
   void updateViewingDirection(Application &Ctx);
};

} // namespace mc

#endif //MINESHAFT_PLAYER_H
