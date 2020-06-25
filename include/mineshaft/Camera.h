#ifndef MINESHAFT_CAMERA_H
#define MINESHAFT_CAMERA_H

#include "mineshaft/Model/Model.h"

#include <glm/glm.hpp>

#include <array>

class GLFWwindow;

namespace mc {

class Block;
class Chunk;
class Event;
class EventDispatcher;
class Model;
class World;

struct ControlMatrices {
   glm::mat4 View;
   glm::mat4 Projection;

   glm::mat4 getMatrix() const { return Projection * View; }
};

class Camera {
   /// The context of this camera.
   Application &app;

   /// The window this camera operates on.
   GLFWwindow *window;

   /// Current viewport width.
   int viewportWidth = 0;

   /// Current viewport height.
   int viewportHeight = 0;

   /// Current window width.
   int windowWidth = 0;

   /// Current window height.
   int windowHeight = 0;

   /// The current aspect ratio of the window.
   float aspectRatio = 0.0f;

   /// Time of the last frame.
   float currentTime = 0.0f;

   /// Time of the last frame.
   float lastTime = 0.0f;

   /// Elapsed time since the last frame.
   float deltaTime = 0.0f;

   /// The current position of the camera.
   glm::vec3 position;

   /// The direction we're looking at.
   glm::vec3 direction;

   /// The current right vector.
   glm::vec3 right;

   /// The current up vector.
   glm::vec3 up;

   /// The current field of view.
   float FOV;

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

   enum CameraMode {
      FirstPerson,
      ThirdPerson,
      ThirdPersonInverted,
   };

private:
   ViewFrustum viewFrustum;
   ControlMatrices viewProjectionMatrices;

   /// The VAO of the crosshair model.
   unsigned CrosshairVAO = 0;

   /// The VBO of the crosshair model.
   unsigned CrosshairVBO = 0;

   /// The current camera mode.
   CameraMode cameraMode = FirstPerson;

#ifndef NDEBUG
   bool renderFrustumPressed = false;
   ViewFrustum frustumToRender;
#endif

public:
   explicit Camera(Application &Ctx,
                   GLFWwindow *window = nullptr,
                   glm::vec3 position = glm::vec3(0.0f, 4.0f, 0.0f),
                   float FOV = 45.0f);

   /// Initialize the camera event handlers.
   void initialize(GLFWwindow *window);

   /// \return the position of the camera.
   const glm::vec3 &getPosition() const;

   /// Manually set the position of the camera.
   void setPosition(const glm::vec3 &position);

   /// \return the current field of view.
   float getFOV() const;

   /// Update the field of view.
   void setFOV(float FOV);

   /// \return The elapsed time since the last frame.
   float getElapsedTime() const { return deltaTime; }

   /// \return The time stamp of the current frame.
   float getCurrentTime() const { return currentTime; }

   /// \return The time stamp of the last frame.
   float getLastTime() const { return lastTime; }

   /// Update the elapsed time.
   void updateCurrentTime();

   /// Update the elapsed time.
   void updateLastTime();

   /// \return The current window height in pixels.
   int getViewportHeight() const { return viewportHeight; }

   /// \return The current window width in pixels.
   int getViewportWidth() const { return viewportWidth; }

   /// \return The current window height in pixels.
   int getWindowHeight() const { return windowHeight; }

   /// \return The current window width in pixels.
   int getWindowWidth() const { return windowWidth; }

   /// \return The current window aspect ratio.
   float getAspectRatio() const { return aspectRatio; }

   /// \return The current camera mode.
   CameraMode getCameraMode() const { return cameraMode; }

   /// Set the camera mode.
   void setCameraMode(CameraMode mode) { cameraMode = mode; }

   /// Cycle through to the next camera mode.
   void cycleCameraMode();

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
   std::pair<bool, float> pointsAt(const BoundingBox &boundingBox,
                                   const glm::mat4 &modelMatrix,
                                   const glm::vec3 &rayVector);

   /// Update viewing angle based on mouse inpit.
   void computeMatricesFromInputs();

   /// Render the given array of blocks.
   void renderBlocks(llvm::ArrayRef<const Block*> blocks, bool changed = true);
   void renderChunks(llvm::ArrayRef<const Chunk*> chunks);

   /// Find the block the player currently points at.
   const Block *getPointedAtBlock(World &world);

   /// Render the borders around a model.
   void renderBorders(const Block &block);

   /// Render the debug overlay.
   void renderDebugOverlay();
};

} // namespace mc

#endif //MINESHAFT_CAMERA_H
