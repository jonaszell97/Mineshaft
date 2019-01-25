//
// Created by Jonas Zell on 2019-01-19.
//

#ifndef MINEKAMPF_CAMERA_H
#define MINEKAMPF_CAMERA_H

#include "mineshaft/Model/Model.h"

#include <glm/glm.hpp>

#include <array>

class GLFWwindow;

namespace mc {

class Block;
class Model;

struct ControlMatrices {
   glm::mat4 View;
   glm::mat4 Projection;

   glm::mat4 getMatrix() const { return Projection * View; }
};

class Camera {
   /// The context of this camera.
   Context &Ctx;

   /// The window this camera operates on.
   GLFWwindow *window;

   /// The current position of the camera.
   glm::vec3 position;

   /// The direction we're looking at.
   glm::vec3 direction;

   /// The current up vector.
   glm::vec3 up;

   /// The current right vector.
   glm::vec3 right;

   /// The current field of view.
   float FOV;

   /// The current horizontal angle.
   float horizontalAngle;

   /// The current vertical angle.
   float verticalAngle;

   /// The current movement speed.
   float speed;

   /// The current mouse speed.
   float mouseSpeed;

   /// Time of the last camera update.
   double lastTime;

public:
   struct Plane {
      glm::vec3 point;
      glm::vec3 normal;

      float distance(const glm::vec3 &p) const;
   };

   struct ViewFrustum {
      union {
         struct {
            Plane near;
            Plane far;
            Plane top;
            Plane bottom;
            Plane left;
            Plane right;
         } planes;

         Plane allPlanes[6];
      };

      glm::vec3 ftl, ftr, fbl, fbr, ntl, ntr, nbl, nbr;

      bool operator==(const ViewFrustum &Other)
      {
         return ftl == Other.ftl
            && ftr == Other.ftr
            && fbl == Other.fbl
            && fbr == Other.fbr
            && ntl == Other.ntl
            && ntr == Other.ntr
            && nbl == Other.nbl
            && nbr == Other.nbr;
      }

      bool operator!=(const ViewFrustum &Other)
      {
         return !(*this == Other);
      }
   };

private:
   ViewFrustum viewFrustum;
   ControlMatrices viewProjectionMatrices;

   /// The VAO of the crosshair model.
   unsigned CrosshairVAO = 0;

   /// The VBO of the crosshair model.
   unsigned CrosshairVBO = 0;

#ifndef NDEBUG
   bool renderFrustumPressed = false;
   ViewFrustum frustumToRender;
#endif

public:
   explicit Camera(Context &Ctx,
                   GLFWwindow *window = nullptr,
                   glm::vec3 position = glm::vec3(),
                   float FOV = 45.0f,
                   float horizontalAngle = 3.14f,
                   float verticalAngle = 0.0f,
                   float speed = 30.0f,
                   float mouseSpeed = 0.005f);

   /// Update this camera's window.
   void setWindow(GLFWwindow *window);

   /// \return the position of the camera.
   const glm::vec3 &getPosition() const;

   /// Manually set the position of the camera.
   void setPosition(const glm::vec3 &position);

   /// \return the current field of view.
   float getFOV() const;

   /// Update the field of view.
   void setFOV(float FOV);

   /// \return the current horizontal viewing angle.
   float getHorizontalAngle() const;

   /// Update the horizontal viewing angle.
   void setHorizontalAngle(float horizontalAngle);

   /// \return the current vertical viewing angle.
   float getVerticalAngle() const;

   /// Update the vertical viewing angle.
   void setVerticalAngle(float verticalAngle);

   /// \return the current movement speed.
   float getSpeed() const;

   /// Update the movement speed.
   void setSpeed(float speed);

   /// \return the current mouse speed.
   float getMouseSpeed() const;

   /// Update the mouse speed.
   void setMouseSpeed(float mouseSpeed);

   /// \return A ray vector from the given coordinate.
   glm::vec3 getRayVector(float x, float y);

   /// Calculate the six planes that form the view frustum.
   ViewFrustum calculateViewFrustum();

   /// \return The current view frustum.
   const ViewFrustum &getViewFrustum() const { return viewFrustum; }

   /// \return The current view and projection matrices.
   const ControlMatrices &getViewProjectionMatrices() const
   {
      return viewProjectionMatrices;
   }

   enum ViewCullingTestResult {
      Inside,
      Outside,
      Intersect
   };

   /// Check if a point is contained in the view frustum.
   ViewCullingTestResult pointInFrustum(const glm::vec3 &p);

   /// Check if a sphere is contained in the view frustum.
   ViewCullingTestResult sphereInFrustum(const BoundingSphere &sphere);

   /// Check if a box is contained in the view frustum.
   ViewCullingTestResult boxInFrustum(const BoundingBox &box);

   /// Draw the current view fractum.
   void renderFrustum(const ViewFrustum &viewFrustum);

   /// Render a bounding box.
   void renderBoundingBox(const BoundingBox &boundingBox,
                          const glm::vec4 &color);

   /// Render a plane.
   std::array<glm::vec3, 4> getPlaneCorners(const Plane &plane);

   /// Draw a coordinate system at the center of the world.
   void renderCoordinateSystem();

   /// Draw a crosshair.
   void renderCrosshair();

   /// \return true iff the camera currently points at the given object.
   std::pair<bool, float> pointsAt(const Model &M,
                                   const glm::mat4 &modelMatrix);

   /// Update viewing angle based on mouse inpit.
   void computeMatricesFromInputs();

   /// Render the given array of blocks.
   void renderBlocks(llvm::ArrayRef<const Block*> blocks, bool changed = true);
};

} // namespace mc

#endif //MINEKAMPF_CAMERA_H
