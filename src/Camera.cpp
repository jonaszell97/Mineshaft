//
// Created by Jonas Zell on 2019-01-19.
//

#include "mineshaft/Camera.h"
#include "mineshaft/Context.h"
#include "mineshaft/Model/Model.h"
#include "mineshaft/utils.h"
#include "mineshaft/World/Block.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>

using namespace glm;
using namespace mc;

#define NEAR_CLIPPING_DISTANCE 0.1f
#define FAR_CLIPPING_DISTANCE  100.0f

Camera::Camera(Context &Ctx, GLFWwindow *window, glm::vec3 position,
               float FOV, float horizontalAngle,
               float verticalAngle, float speed, float mouseSpeed)
   : Ctx(Ctx), window(window), position(position),
     FOV(FOV), horizontalAngle(horizontalAngle),
     verticalAngle(verticalAngle), speed(speed), mouseSpeed(mouseSpeed),
     lastTime(glfwGetTime())
{ }

void Camera::computeMatricesFromInputs()
{
   // Compute time difference between current and last frame
   double currentTime = glfwGetTime();
   float deltaTime = float(currentTime - lastTime);

   // Get mouse position
   double xpos, ypos;
   glfwGetCursorPos(window, &xpos, &ypos);

   // Reset mouse position for next frame
   glfwSetCursorPos(window, 1024/2, 768/2);

   // Compute new orientation
   horizontalAngle += mouseSpeed * float(1024/2 - xpos);
   verticalAngle   += mouseSpeed * float( 768/2 - ypos);

   if (verticalAngle >= 3.14f) {
      verticalAngle = 3.14f;
   }
   if (verticalAngle <= -3.14f) {
      verticalAngle = -3.14f;
   }

   // Direction : Spherical coordinates to Cartesian coordinates conversion
   direction = vec3(
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

   // Up vector
   up = glm::cross(right, direction);

   // Move forward
   if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
      position += direction * deltaTime * speed;
   }

   // Move backward
   if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      position -= direction * deltaTime * speed;
   }

   // Strafe right
   if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
      position += right * deltaTime * speed;
   }

   // Strafe left
   if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
      position -= right * deltaTime * speed;
   }

   // Move up
   if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
      position += up * deltaTime * speed;
   }

   // Move down
   if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      position -= up * deltaTime * speed;
   }

   // Increase FoV
   if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
      FOV += 5.0f;
   }

   // Decrease FoV
   if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
      FOV -= 5.0f;
   }

   int width, height;
   glfwGetWindowSize(window, &width, &height);

   viewProjectionMatrices.Projection = glm::perspective(glm::radians(FOV),
                                                  (float)width / (float)height,
                                                  NEAR_CLIPPING_DISTANCE,
                                                  FAR_CLIPPING_DISTANCE);

   // Camera matrix
   viewProjectionMatrices.View = glm::lookAt(
      position,
      position + direction,
      up
   );

   // For the next frame, the "last time" will be "now"
   lastTime = currentTime;
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

std::pair<bool, float> Camera::pointsAt(const Model &M,
                                        const glm::mat4 &modelMatrix) {
   int width, height;
   glfwGetWindowSize(window, &width, &height);

   glm::vec3 rd = getRayVector(width / 2.0f, height / 2.0f);
   float shortestDistance = -1.0f;

   for (auto &mesh : M.getMeshes()) {
      unsigned NumVertices = (unsigned)mesh.Indices.size();
      assert((NumVertices % 3) == 0);

      for (unsigned i = 0; i < NumVertices; i += 3) {
         glm::vec3 intersection = triIntersect(
            position, rd,
            glm::vec3(modelMatrix *
               glm::vec4(mesh.Vertices[mesh.Indices[i]].Position, 1.0f)),
            glm::vec3(modelMatrix *
               glm::vec4(mesh.Vertices[mesh.Indices[i+1]].Position, 1.0f)),
            glm::vec3(modelMatrix *
               glm::vec4(mesh.Vertices[mesh.Indices[i+2]].Position, 1.0f)));

         if (intersection.x == -1.0f) {
            continue;
         }

         if (shortestDistance == -1.0f || intersection.x < shortestDistance) {
            shortestDistance = intersection.x;
         }
      }
   }

   return { shortestDistance != -1.0f, shortestDistance };
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

      for (const glm::vec3 &p : box.corners) {
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

   auto &shader = Ctx.getShader(Context::SINGLE_COLOR_SHADER);
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

   auto &shader = Ctx.getShader(Context::SINGLE_COLOR_SHADER);
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

   auto &shader = Ctx.getShader(Context::CROSSHAIR_SHADER);
   shader.useShader();

   // Render crosshair
   glBindVertexArray(CrosshairVAO);
   glDrawArrays(GL_LINES, 0, 8);

   // Reset values.
   glBindVertexArray(0);
}

void Camera::renderBoundingBox(const mc::BoundingBox &boundingBox,
                               const glm::vec4 &color) {
   struct Data {
      GLuint VAO = 0, VBO = 0;
   };

   static std::unordered_map<glm::vec3, Data> dataMap;
   auto &VAO = dataMap[boundingBox.center].VAO;
   auto &VBO = dataMap[boundingBox.center].VBO;

   if (!VAO) {
      glGenVertexArrays(1, &VAO);
      glGenBuffers(1, &VBO);

      glBindVertexArray(VAO);
      glBindBuffer(GL_ARRAY_BUFFER, VBO);

      std::vector<glm::vec3> vertices{
         // Top quad
         boundingBox.corners[0],
         boundingBox.corners[1],

         boundingBox.corners[0],
         boundingBox.corners[2],

         boundingBox.corners[1],
         boundingBox.corners[3],

         boundingBox.corners[2],
         boundingBox.corners[3],

         // Bottom quad
         boundingBox.corners[4],
         boundingBox.corners[5],

         boundingBox.corners[4],
         boundingBox.corners[6],

         boundingBox.corners[5],
         boundingBox.corners[7],

         boundingBox.corners[6],
         boundingBox.corners[7],

         // Sides
         boundingBox.corners[0],
         boundingBox.corners[4],

         boundingBox.corners[1],
         boundingBox.corners[5],

         boundingBox.corners[2],
         boundingBox.corners[6],

         boundingBox.corners[3],
         boundingBox.corners[7],
      };

      glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
                   vertices.data(), GL_STATIC_DRAW);

      // vertex positions
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
   }

   auto &shader = Ctx.getShader(Context::SINGLE_COLOR_SHADER);

   glBindVertexArray(VAO);
   glEnableVertexAttribArray(0);

   shader.useShader();
   shader.setUniform("singleColor", color);

   glDrawArrays(GL_LINES, 0, 12 * 3 * 2);

   glBindVertexArray(0);
   glDisableVertexAttribArray(0);
}

void Camera::renderBlocks(llvm::ArrayRef<const Block*> blocks, bool changed)
{
   if (blocks.empty()) {
      return;
   }

   const Shader *shader = &Ctx.getShader(Context::TEXTURE_ARRAY_SHADER_INSTANCED);

   shader->useShader();
   shader->setUniform("viewProjectionMatrix", viewProjectionMatrices.getMatrix());
   shader->setUniform("textureArray", 0);
   shader->setUniform("cubeArray", 1);

   Ctx.blockTextureArray.bind();
   Ctx.blockTextureCubemapArray.bind();

   struct RenderData {
      glm::mat4 modelMatrix;
      GLint layer;

      RenderData(const mat4 &modelMatrix, GLint layer, bool cubemap)
         : modelMatrix(modelMatrix),
           layer(cubemap ? -layer - 1 : layer)
      { }
   };

   static GLuint buffer = 0;
   if (!buffer) {
      glGenBuffers(1, &buffer);
   }

   glBindBuffer(GL_ARRAY_BUFFER, buffer);

   if (changed) {
      std::vector<RenderData> renderData;
      for (auto *block : blocks) {
         renderData.emplace_back(block->getModelMatrix(),
                                 block->getTextureLayer(),
                                 block->usesCubeMap());

         for (const Mesh &mesh : block->getModel()->getMeshes()) {
            glBindVertexArray(mesh.VAO);

            static constexpr GLsizei stride = sizeof(RenderData);
            static constexpr GLsizei vec4Size = sizeof(glm::vec4);

            // model matrix
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, nullptr);
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride,
                                  (void *) (vec4Size));
            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride,
                                  (void *) (2 * vec4Size));
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride,
                                  (void *) (3 * vec4Size));

            // layer
            glEnableVertexAttribArray(7);
            glVertexAttribIPointer(7, 1, GL_INT, stride,
                                   (void *) (4 * vec4Size));

            glVertexAttribDivisor(3, 1);
            glVertexAttribDivisor(4, 1);
            glVertexAttribDivisor(5, 1);
            glVertexAttribDivisor(6, 1);
            glVertexAttribDivisor(7, 1);

            glBindVertexArray(0);
         }
      }

      glBufferData(GL_ARRAY_BUFFER, renderData.size() * sizeof(RenderData),
                   renderData.data(), GL_DYNAMIC_DRAW);
   }

   for (const Mesh &mesh : blocks.front()->getModel()->getMeshes()) {
      glBindVertexArray(mesh.VAO);
      glDrawElementsInstanced(GL_TRIANGLES, mesh.Indices.size(),
                              GL_UNSIGNED_INT, nullptr, blocks.size());
   }

   // Reset values.
   glBindVertexArray(0);
   glActiveTexture(GL_TEXTURE0);

//   std::unordered_map<Block::BlockID, std::vector<const Block*>> blockMap;
//   for (auto *block : blocks) {
//      blockMap[block->getBlockID()].push_back(block);
//   }
//
//   for (auto &blockIDPair : blockMap) {
//      const Block &first = *blockIDPair.second.front();
//      if (first.isTransparent()) {
//         glDisable(GL_DEPTH_TEST);
//      }
//
//      const Shader *shader;
//      if (first.usesCubeMap()) {
//         shader = &Ctx.getShader(Context::CUBE_SHADER_INSTANCED);
//      }
//      else {
//         shader = &Ctx.getShader(Context::BASIC_SHADER_INSTANCED);
//      }
//
//      shader->useShader();
//      shader->setUniform("viewProjectionMatrix", viewProjectionMatrices.getMatrix());
//
//      first.getModel()->getMeshes().front().bind(*shader);
//
//      std::vector<glm::mat4> modelMatrices;
//      for (auto *block : blockIDPair.second) {
//         modelMatrices.push_back(block->getModelMatrix());
//      }
//
//      GLuint buffer;
//      glGenBuffers(1, &buffer);
//      glBindBuffer(GL_ARRAY_BUFFER, buffer);
//      glBufferData(GL_ARRAY_BUFFER, modelMatrices.size() * sizeof(glm::mat4),
//                   modelMatrices.data(), GL_STATIC_DRAW);
//
//      for (auto *block : blockIDPair.second) {
//         for (const Mesh &mesh : block->getModel()->getMeshes()) {
//            glBindVertexArray(mesh.VAO);
//
//            static constexpr GLsizei vec4Size = sizeof(glm::vec4);
//            glEnableVertexAttribArray(3);
//            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, nullptr);
//            glEnableVertexAttribArray(4);
//            glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(vec4Size));
//            glEnableVertexAttribArray(5);
//            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(2 * vec4Size));
//            glEnableVertexAttribArray(6);
//            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(3 * vec4Size));
//
//            glVertexAttribDivisor(3, 1);
//            glVertexAttribDivisor(4, 1);
//            glVertexAttribDivisor(5, 1);
//            glVertexAttribDivisor(6, 1);
//
//            glBindVertexArray(0);
//         }
//      }
//
//      for (const Mesh &mesh : first.getModel()->getMeshes()) {
//         glBindVertexArray(mesh.VAO);
//         glDrawElementsInstanced(GL_TRIANGLES, mesh.Indices.size(),
//                                 GL_UNSIGNED_INT, 0, modelMatrices.size());
//      }
//
//      // Reset values.
//      glBindVertexArray(0);
//      glActiveTexture(GL_TEXTURE0);
//
//      if (first.isTransparent()) {
//         glEnable(GL_DEPTH_TEST);
//      }
//   }
}

void Camera::setWindow(GLFWwindow *window)
{
   this->window = window;
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

float Camera::getHorizontalAngle() const
{
   return horizontalAngle;
}

void Camera::setHorizontalAngle(float horizontalAngle)
{
   Camera::horizontalAngle = horizontalAngle;
}

float Camera::getVerticalAngle() const
{
   return verticalAngle;
}

void Camera::setVerticalAngle(float verticalAngle)
{
   Camera::verticalAngle = verticalAngle;
}

float Camera::getSpeed() const
{
   return speed;
}

void Camera::setSpeed(float speed)
{
   Camera::speed = speed;
}

float Camera::getMouseSpeed() const
{
   return mouseSpeed;
}

void Camera::setMouseSpeed(float mouseSpeed)
{
   Camera::mouseSpeed = mouseSpeed;
}
