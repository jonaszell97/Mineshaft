#include "mineshaft/Camera.h"
#include "mineshaft/Application.h"
#include "mineshaft/Entity/Player.h"
#include "mineshaft/Model/Model.h"
#include "mineshaft/utils.h"
#include "mineshaft/World/Chunk.h"
#include "mineshaft/World/World.h"
#include "mineshaft/World/WorldGenerator.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <llvm/Support/Format.h>

#include <cstdio>

using namespace glm;
using namespace mc;

Camera::Camera(Application &Ctx, GLFWwindow *window, glm::vec3 position,
               float FOV)
   : app(Ctx), window(window),
     currentTime((float)glfwGetTime()), lastTime(currentTime), deltaTime(0.0f),
     position(position), FOV(FOV)
{ }

void Camera::initialize(GLFWwindow *window)
{
   glfwMakeContextCurrent(window);

   this->window = window;
   glfwGetFramebufferSize(window, &viewportWidth, &viewportHeight);
   glViewport(0, 0, viewportWidth, viewportHeight);

   glfwGetWindowSize(window, &windowWidth, &windowHeight);
   aspectRatio = (float)windowWidth / (float)windowHeight;
}

void Camera::updateCurrentTime()
{
   currentTime = (float)glfwGetTime();
   deltaTime = currentTime - lastTime;
}

void Camera::updateLastTime()
{
   lastTime = currentTime;
}

void Camera::computeMatricesFromInputs()
{
   auto *player = app.getPlayer();

   // Direction : Spherical coordinates to Cartesian coordinates conversion
   direction = player->getDirection();
   position = player->getPosition();
   right = player->getRightVector();
   up = player->getUpVector();

   switch (cameraMode) {
   case FirstPerson:
      position.x += player->getWidth() / 2;
      position.y += (player->getHeight() / 2) + 1.5f;
      position.z += player->getDepth() / 2;
      break;
   case ThirdPerson:
      position.y += 7.0f;
      position.z += 4.2f;
      break;
   case ThirdPersonInverted:
      position.y += 7.0f;
      position.z -= 4.2f;
      break;
   }

   viewProjectionMatrices.Projection = glm::perspective(
      glm::radians(FOV),
      (float)viewportWidth / (float)viewportHeight,
      NEAR_CLIPPING_DISTANCE,
      FAR_CLIPPING_DISTANCE);

   viewProjectionMatrices.View = glm::lookAt(
      position,
      position + direction,
      up);

   viewFrustum = calculateViewFrustum();

   // Render frustum
#ifndef NDEBUG
   if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
      if (!renderFrustumPressed) {
         renderFrustumPressed = true;
         frustumToRender = viewFrustum;
      }

      renderFrustum(frustumToRender);
   }
   else {
      renderFrustumPressed = false;
   }
#endif
}

void Camera::cycleCameraMode()
{
   switch (cameraMode) {
   case FirstPerson:
      cameraMode = ThirdPerson;
      break;
   case ThirdPerson:
      cameraMode = ThirdPersonInverted;
      break;
   case ThirdPersonInverted:
      cameraMode = FirstPerson;
      break;
   }
}

glm::vec3 Camera::getRayVector(float x, float y)
{
   glm::vec3 view = normalize(direction);
   glm::vec3 h = normalize(cross(view, up));
   glm::vec3 v = normalize(cross(h, view));

   float rad = glm::radians(FOV);

   int width, height;
   glfwGetWindowSize(window, &width, &height);

   float vLength = tan(rad / 2) * NEAR_CLIPPING_DISTANCE;
   float hLength = vLength * ((float)width / (float)height);

   v *= vLength;
   h *= hLength;

   // translate mouse coordinates so that the origin lies in the center
   // of the view port
   x -= width / 2.0f;
   y -= height / 2.0f;

   // scale mouse coordinates so that half the view port width and height
   // becomes 1
   y /= (height / 2.0f);
   x /= (width / 2.0f);

   // linear combination to compute intersection of picking ray with
   // view port plane
   auto pos = position + view * NEAR_CLIPPING_DISTANCE + h * x + v * y;

   // compute direction of picking ray by subtracting intersection point
   // with camera position
   return pos - position;
}

float Camera::Plane::distance(const glm::vec3 &p) const
{
   return dot(normal, (p - point));
}

Camera::ViewFrustum Camera::calculateViewFrustum()
{
   int width, height;
   glfwGetWindowSize(window, &width, &height);

   glm::vec3 d = normalize(direction);
   float ratio = (float)width / (float) height;

   float tang = tan(glm::radians(FOV) / 2);
   float nh = NEAR_CLIPPING_DISTANCE * tang;
   float nw = nh * ratio;
   float fh = FAR_CLIPPING_DISTANCE  * tang;
   float fw = fh * ratio;

   ViewFrustum Frustum{};

   glm::vec3 nc = position + (d * NEAR_CLIPPING_DISTANCE);
   glm::vec3 fc = position + (d * FAR_CLIPPING_DISTANCE);

#ifndef NDEBUG
   Frustum.ftl = fc + (up * fh) - (right * fw);
   Frustum.ftr = fc + (up * fh) + (right * fw);
   Frustum.fbl = fc - (up * fh) - (right * fw);
   Frustum.fbr = fc - (up * fh) + (right * fw);

   Frustum.ntl = nc + (up * nh) - (right * nw);
   Frustum.ntr = nc + (up * nh) + (right * nw);
   Frustum.nbl = nc - (up * nh) - (right * nw);
   Frustum.nbr = nc - (up * nh) + (right * nw);
#endif

   // Near
   Frustum.planes.near.point = nc;
   Frustum.planes.near.normal = direction;

   // Far
   Frustum.planes.far.point = fc;
   Frustum.planes.far.normal = -direction;

   // Right
   glm::vec3 a = normalize((nc + right * nw) - position);
   Frustum.planes.right.point = position;
   Frustum.planes.right.normal = normalize(cross(up, a));

   // Left
   a = normalize((nc - right * nw) - position);
   Frustum.planes.left.point = position;
   Frustum.planes.left.normal = normalize(cross(a, up));

   // Top
   a = normalize((nc + up * nh) - position);
   Frustum.planes.top.point = position;
   Frustum.planes.top.normal = normalize(cross(a, right));

   // Bottom
   a = normalize((nc - up * nh) - position);
   Frustum.planes.bottom.point = position;
   Frustum.planes.bottom.normal = normalize(cross(right, a));

   return Frustum;
}

Camera::ViewCullingTestResult
Camera::pointInFrustum(const glm::vec3 &p)
{
   for (auto &plane : viewFrustum.allPlanes) {
      float dist = plane.distance(p);
      if (dist < 0.0f) {
         return Outside;
      }
   }

   return Inside;
}

Camera::ViewCullingTestResult
Camera::sphereInFrustum(const BoundingSphere &sphere)
{
   ViewCullingTestResult result = Inside;

   for (auto &plane : viewFrustum.allPlanes) {
      float dist = plane.distance(sphere.center);
      if (dist < -sphere.radius) {
         return Outside;
      }
      else if (dist < sphere.radius) {
         result = Intersect;
      }
   }

   return result;
}

Camera::ViewCullingTestResult
Camera::boxInFrustum(const BoundingBox &box)
{
   ViewCullingTestResult result = Inside;

   for (auto &plane : viewFrustum.allPlanes) {
      int out = 0;
      int in = 0;

      for (const glm::vec3 &p : box.corners()) {
         if (in != 0 && out != 0) {
            break;
         }

         float dist = plane.distance(p);
         if (dist < 0.0f) {
            ++out;
         }
         else {
            ++in;
         }
      }

      if (!in) {
         return Outside;
      }
      else if (out) {
         result = Intersect;
      }
   }

   return result;
}

void Camera::renderFrustum(const ViewFrustum &viewFrustum)
{
   static GLuint VAO = 0, VBO = 0;
   static glm::vec4 colors[6];

   std::vector<glm::vec3> vertices;

   // Near
   vertices.emplace_back(viewFrustum.ntl);
   vertices.emplace_back(viewFrustum.ntr);
   vertices.emplace_back(viewFrustum.nbl);

   vertices.emplace_back(viewFrustum.nbl);
   vertices.emplace_back(viewFrustum.nbr);
   vertices.emplace_back(viewFrustum.ntr);

   // Far
   vertices.emplace_back(viewFrustum.ftl);
   vertices.emplace_back(viewFrustum.ftr);
   vertices.emplace_back(viewFrustum.fbl);

   vertices.emplace_back(viewFrustum.fbl);
   vertices.emplace_back(viewFrustum.fbr);
   vertices.emplace_back(viewFrustum.ftr);

   // Left
   vertices.emplace_back(viewFrustum.nbl);
   vertices.emplace_back(viewFrustum.ntl);
   vertices.emplace_back(viewFrustum.ftl);

   vertices.emplace_back(viewFrustum.nbl);
   vertices.emplace_back(viewFrustum.ftl);
   vertices.emplace_back(viewFrustum.fbl);

   // Right
   vertices.emplace_back(viewFrustum.nbr);
   vertices.emplace_back(viewFrustum.ntr);
   vertices.emplace_back(viewFrustum.ftr);

   vertices.emplace_back(viewFrustum.nbr);
   vertices.emplace_back(viewFrustum.ftr);
   vertices.emplace_back(viewFrustum.fbr);

   // Top
   vertices.emplace_back(viewFrustum.ntl);
   vertices.emplace_back(viewFrustum.ntr);
   vertices.emplace_back(viewFrustum.ftl);

   vertices.emplace_back(viewFrustum.ftl);
   vertices.emplace_back(viewFrustum.ftr);
   vertices.emplace_back(viewFrustum.ntr);

   // Bottom
   vertices.emplace_back(viewFrustum.nbl);
   vertices.emplace_back(viewFrustum.nbr);
   vertices.emplace_back(viewFrustum.fbl);

   vertices.emplace_back(viewFrustum.fbl);
   vertices.emplace_back(viewFrustum.fbr);
   vertices.emplace_back(viewFrustum.nbr);

   if (VAO == 0) {
      for (auto &c : colors) {
         c = randomColor();
      }

      glGenVertexArrays(1, &VAO);
      glBindVertexArray(VAO);

      glGenBuffers(1, &VBO);
      glBindBuffer(GL_ARRAY_BUFFER, VBO);

      // vertex positions
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                            nullptr);
   }

   glBindVertexArray(VAO);
   glDisable(GL_CULL_FACE);

   glBindBuffer(GL_ARRAY_BUFFER, VBO);
   glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
                vertices.data(), GL_DYNAMIC_DRAW);

   auto &shader = app.getShader(Application::SINGLE_COLOR_SHADER);
   shader.useShader();
   shader.setUniform("MVP", viewProjectionMatrices.getMatrix());

   for (int i = 0; i < vertices.size(); i += 6) {
      shader.setUniform("singleColor", colors[i / 6]);
      glDrawArrays(GL_TRIANGLES, i, 6);
   }

   glBindVertexArray(0);
   glDisableVertexAttribArray(0);
   glEnable(GL_CULL_FACE);
}

std::array<glm::vec3, 4> Camera::getPlaneCorners(const Plane &plane)
{
   glm::vec3 P1 = plane.point;

   // Get a random second point on the plane.
   glm::vec3 P2 = plane.point + dot(plane.normal, plane.normal);

   glm::vec3 tangent = normalize(P2 - P1);
   glm::vec3 bitangent = cross(tangent, plane.normal);

   float D = 5.0f;
   glm::vec3 v1 = P1 - tangent*D - bitangent*D;
   glm::vec3 v2 = P1 + tangent*D - bitangent*D;
   glm::vec3 v3 = P1 + tangent*D + bitangent*D;
   glm::vec3 v4 = P1 - tangent*D + bitangent*D;

   return { v1, v2, v3, v4 };
}

void Camera::renderCoordinateSystem()
{
   static GLuint VAO = 0, VBO = 0;
   if (VAO == 0) {
      static constexpr float vertices[] = {
         // x axis
         0.0f, 0.0f, 0.0f,
         0.5f, 0.0f, 0.0f,

         // y axis
         0.0f, 0.0f, 0.0f,
         0.0f, 0.5f, 0.0f,

         // z axis
         0.0f, 0.0f, 0.0f,
         0.0f, 0.0f, 0.5f,
      };

      glGenVertexArrays(1, &VAO);
      glBindVertexArray(VAO);

      glGenBuffers(1, &VBO);
      glBindBuffer(GL_ARRAY_BUFFER, VBO);

      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

      // vertex positions
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                            nullptr);
   }

   glBindVertexArray(VAO);

   auto &shader = app.getShader(Application::SINGLE_COLOR_SHADER);
   shader.useShader();
   shader.setUniform("MVP", viewProjectionMatrices.getMatrix());

   // x axis
   shader.setUniform("singleColor", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
   glDrawArrays(GL_LINES, 0, 2);

   // y axis
   shader.setUniform("singleColor", glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
   glDrawArrays(GL_LINES, 2, 2);

   // z axis
   shader.setUniform("singleColor", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
   glDrawArrays(GL_LINES, 4, 2);

   glBindVertexArray(0);
   glDisableVertexAttribArray(0);
}

void Camera::renderCrosshair()
{
   static constexpr float FACTOR_SMALL = 0.01f;
   static constexpr float FACTOR_LARGE = 0.045f;
   static constexpr float ASPECT_RATIO = 4.0f / 3.0f;

   static constexpr float vertices[] = {
      // Left line
      -FACTOR_LARGE / ASPECT_RATIO, 0.0f, 0.0f,
      -FACTOR_SMALL / ASPECT_RATIO, 0.0f, 0.0f,

      // Top line
      0.0f, FACTOR_LARGE, 0.0f,
      0.0f, FACTOR_SMALL, 0.0f,

      // Right line
      FACTOR_LARGE / ASPECT_RATIO, 0.0f, 0.0f,
      FACTOR_SMALL / ASPECT_RATIO, 0.0f, 0.0f,

      // Bottom line
      0.0f, -FACTOR_LARGE, 0.0f,
      0.0f, -FACTOR_SMALL, 0.0f,
   };

   if (!CrosshairVAO) {
      glGenVertexArrays(1, &CrosshairVAO);
      glBindVertexArray(CrosshairVAO);

      glGenBuffers(1, &CrosshairVBO);
      glBindBuffer(GL_ARRAY_BUFFER, CrosshairVBO);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

      // vertex positions
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), nullptr);
   }

   auto &shader = app.getShader(Application::CROSSHAIR_SHADER);
   shader.useShader();

   // Render crosshair
   glBindVertexArray(CrosshairVAO);
   glDrawArrays(GL_LINES, 0, 8);

   // Reset values.
   glBindVertexArray(0);
}

void Camera::renderBoundingBox(const mc::BoundingBox &boundingBox,
                               const glm::vec4 &color) {
   static std::unordered_map<BoundingBox, Mesh> map;
   if (!map.count(boundingBox)) {
      map.emplace(boundingBox, Mesh::createBoundingBox(app, boundingBox));
   }

   auto &mesh = map.find(boundingBox)->second;
   auto &shader = app.getShader(Application::SINGLE_COLOR_SHADER);
   shader.setUniform("singleColor", color);

   mesh.render(shader, viewProjectionMatrices.getMatrix());
}

void Camera::renderChunks(llvm::ArrayRef<const Chunk*> chunks)
{
   if (chunks.empty()) {
      return;
   }

   const Shader &shader = app.getShader(Application::BASIC_SHADER);
   const Shader &waterShader = app.getShader(Application::WATER_SHADER);

   waterShader.useShader();
   waterShader.setUniform("globalTime", currentTime);

   auto vpMatrix = viewProjectionMatrices.getMatrix();
   app.blockTextures.bind();

   for (auto it = chunks.rbegin(), end_it = chunks.rend(); it != end_it; ++it) {
      const Chunk *chunk = *it;

      auto &chunkMesh = chunk->getChunkMesh();
      chunkMesh.finalize();

      // Render terrain.
      shader.useShader();
      chunkMesh.terrainMesh.render(shader, vpMatrix);
   }

   for (auto it = chunks.rbegin(), end_it = chunks.rend(); it != end_it; ++it) {
      const Chunk *chunk = *it;
      auto &chunkMesh = chunk->getChunkMesh();

      // Render translucent block faces.
      chunkMesh.translucentMesh.render(shader, vpMatrix);

      // Render water.
      if (!chunkMesh.waterMesh.Indices.empty()) {
         waterShader.useShader();
         chunkMesh.waterMesh.render(waterShader, vpMatrix);
      }
   }

   // Reset values.
   glBindVertexArray(0);
   glActiveTexture(GL_TEXTURE0);
}

const Block *Camera::getPointedAtBlock(World &world)
{
   unsigned distance = app.gameOptions.interactionDistance;
   glm::vec3 rd = glm::normalize(direction);

   // Follow the ray from the camera position until we hit a block.
   float length = 0.5f;
   WorldPosition lastPos;

   while (length < (float)distance) {
      WorldPosition pos = getWorldPosition(position + (rd * length));
      if (pos == lastPos) {
         length += 0.5f;
         continue;
      }

      auto *block = world.getBlock(pos);
      if (block && block->isSolid()) {
         return block;
      }

      length += 0.5f;
      lastPos = pos;
   }

   return nullptr;
}

void Camera::renderBorders(const mc::Block &block)
{
   auto *texture = app.loadTexture(BasicTexture::DIFFUSE, "block_border.png");
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texture->getTextureID());

   auto &shader = app.getShader(Application::BASIC_SHADER);
   shader.setUniform("textureDiffuse1", 0);

   BoundingBox boundingBox = BoundingBox::unitCube();
   auto scaledMatrix = glm::scale(
      glm::translate(glm::mat4(1.0f),
         getScenePosition(block.getPosition()) + MC_BLOCK_SCALE / 2.f),
      glm::vec3(1.01f));

   scaledMatrix = glm::translate(scaledMatrix, glm::vec3(-(MC_BLOCK_SCALE/2.f)));

   Mesh m = Mesh::createBoundingBox(app, boundingBox);
   m.render(shader, viewProjectionMatrices.getMatrix(), scaledMatrix);
}

llvm::StringRef getBiomeName(Biome b)
{
   switch (b) {
   case Biome::Plains:
      return "Plains";
   case Biome::Mountains:
      return "Mountains";
   case Biome::Forest:
      return "Forest";
   default:
      return "Unknown";
   }
}

void Camera::renderDebugOverlay()
{
   auto playerPos = app.getPlayer()->getPosition();
   auto chunkPos = getChunkPosition(playerPos);

   std::string debugInfo;
   llvm::raw_string_ostream OS(debugInfo);
   auto biomeName = getBiomeName(app.activeWorld->getChunk(chunkPos)->getBiome());

   OS << "Mineshaft v0.01a" << "\n";
   OS << "Player x: " << llvm::format("%0.2f", playerPos.x)
      << " y: " << llvm::format("%0.2f", playerPos.y)
      << " z: " << llvm::format("%0.2f", playerPos.z) << "\n";
   OS << "Chunk x: " << chunkPos.x
      << " z: " << chunkPos.z << " (" << biomeName << ")\n";

   app.defaultFont.renderText(OS.str(), glm::vec2(2.0f, 2.0f),
                              3.0f);
}

const glm::vec3 &Camera::getPosition() const
{
   return position;
}

void Camera::setPosition(const glm::vec3 &position)
{
   Camera::position = position;
}

float Camera::getFOV() const
{
   return FOV;
}

void Camera::setFOV(float FOV)
{
   Camera::FOV = FOV;
}