#include "mineshaft/Application.h"
#include "mineshaft/Model/Model.h"
#include "mineshaft/utils.h"
#include "mineshaft/World/Block.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SaveAndRestore.h>

using namespace mc;

BoundingBox BoundingBox::unitCube()
{
   static BoundingBox boundingBox;
   static bool initialized = false;
   if (!initialized) {
      boundingBox.minX = 0;
      boundingBox.maxX = MC_BLOCK_SCALE;

      boundingBox.minY = 0;
      boundingBox.maxY = MC_BLOCK_SCALE;

      boundingBox.minZ = 0;
      boundingBox.maxZ = MC_BLOCK_SCALE;
   }

   return boundingBox;
}

void BoundingBox::applyOffset(const glm::vec3 &vec)
{
   minX += vec.x;
   maxX += vec.x;

   minY += vec.y;
   maxY += vec.y;

   minZ += vec.z;
   maxZ += vec.z;
}

BoundingBox BoundingBox::offsetBy(const glm::vec3 &vec) const
{
   BoundingBox copy = *this;
   copy.applyOffset(vec);

   return copy;
}

glm::vec3 BoundingBox::center() const
{
   return glm::vec3(minX + ((maxX - minX) / 2.0f),
                    minY + ((maxY - minY) / 2.0f),
                    minZ + ((maxZ - minZ) / 2.0f));
}

glm::vec3 BoundingBox::fbl() const
{
   return glm::vec3(minX, minY, maxZ);
}

glm::vec3 BoundingBox::bbl() const
{
   return glm::vec3(minX, minY, minZ);
}

glm::vec3 BoundingBox::fbr() const
{
   return glm::vec3(maxX, minY, maxZ);
}

glm::vec3 BoundingBox::bbr() const
{
   return glm::vec3(maxX, minY, minZ);
}

glm::vec3 BoundingBox::ftl() const
{
   return glm::vec3(minX, maxY, maxZ);
}

glm::vec3 BoundingBox::btl() const
{
   return glm::vec3(minX, maxY, minZ);
}

glm::vec3 BoundingBox::ftr() const
{
   return glm::vec3(maxX, maxY, maxZ);
}

glm::vec3 BoundingBox::btr() const
{
   return glm::vec3(maxX, maxY, minZ);
}

bool BoundingBox::contains(const glm::vec3 &point) const
{
   return (point.x >= minX && point.x <= maxX) &&
          (point.y >= minY && point.y <= maxY) &&
          (point.z >= minZ && point.z <= maxZ);
}

std::pair<bool, glm::vec3> BoundingBox::intersects(const glm::vec3 &rayOrigin,
                                                   const glm::vec3 &rayDir,
                                                   float minDist, float maxDist) const {
   glm::vec3 invDir = 1.f / rayDir;

   bool signDirX = invDir.x < 0;
   bool signDirY = invDir.y < 0;
   bool signDirZ = invDir.z < 0;

   glm::vec3 min = glm::vec3(minX, minY, minZ);
   glm::vec3 max = glm::vec3(maxX, maxY, maxZ);
   glm::vec3 bbox = signDirX ? min : max;

   float tmin = (bbox.x - rayOrigin.x) * invDir.x;
   bbox = signDirX ? min : max;
   float tmax = (bbox.x - rayOrigin.x) * invDir.x;
   bbox = signDirY ? max : min;
   float tymin = (bbox.y - rayOrigin.y) * invDir.y;
   bbox = signDirY ? min : max;
   float tymax = (bbox.y - rayOrigin.y) * invDir.y;

   if ((tmin > tymax) || (tymin > tmax)) {
      return {false, {}};
   }
   if (tymin > tmin) {
      tmin = tymin;
   }
   if (tymax < tmax) {
      tmax = tymax;
   }

   bbox = signDirZ ? max : min;
   float tzmin = (bbox.z - rayOrigin.z) * invDir.z;
   bbox = signDirZ ? min : max;
   float tzmax = (bbox.z - rayOrigin.z) * invDir.z;

   if ((tmin > tzmax) || (tzmin > tmax)) {
      return {false, {}};
   }
   if (tzmin > tmin) {
      tmin = tzmin;
   }
   if (tzmax < tmax) {
      tmax = tzmax;
   }

   if ((tmin < maxDist) && (tmax > minDist)) {
      return {true, (rayOrigin + (rayDir * tmin) )};
   }

   return {false, {}};
}

bool BoundingBox::collidesWith(const mc::BoundingBox &other) const
{
   return (minX <= other.maxX && maxX >= other.minX) &&
          (minY <= other.maxY && maxY >= other.minY) &&
          (minZ <= other.maxZ && maxZ >= other.minZ);
}

Mesh::Mesh(std::vector<Vertex> &&Vertices,
           std::vector<unsigned> &&Indices,
           std::vector<Texture> &&Textures)
   : Vertices(std::move(Vertices)), Indices(std::move(Indices)),
     Textures(std::move(Textures))
{
   initializeMesh();
}

Mesh::Mesh(mc::Mesh &&other) noexcept
   : Vertices(move(other.Vertices)),
     Indices(move(other.Indices)),
     Textures(move(other.Textures)),
     VAO(other.VAO), VBO(other.VBO), EBO(other.EBO)
{
   other.VAO = 0;
   other.VBO = 0;
   other.EBO = 0;
}

Mesh& Mesh::operator=(Mesh &&other) noexcept
{
   std::swap(Vertices, other.Vertices);
   std::swap(Indices, other.Indices);
   std::swap(Textures, other.Textures);
   std::swap(VAO, other.VAO);
   std::swap(VBO, other.VBO);
   std::swap(EBO, other.EBO);

   return *this;
}

Mesh::~Mesh()
{
   glDeleteVertexArrays(1, &VAO);
   glDeleteBuffers(1, &EBO);
   glDeleteBuffers(1, &VBO);
}

Mesh Mesh::createTriangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
{
   std::vector<Vertex> Vertices;
   Vertices.emplace_back(p1, glm::vec2(), glm::vec3());
   Vertices.emplace_back(p2, glm::vec2(), glm::vec3());
   Vertices.emplace_back(p3, glm::vec2(), glm::vec3());

   return Mesh(move(Vertices), { 0, 1, 2 }, {});
}

Mesh Mesh::createQuad(glm::vec3 bl, glm::vec3 tl, glm::vec3 tr, glm::vec3 br,
                      glm::vec2 uv, float textureWidth, float textureHeight,
                      unsigned windingOrder) {
   if (windingOrder == GL_CCW) {
      std::vector<Vertex> Vertices;
      Vertices.emplace_back(bl, glm::vec2(uv.x, uv.y + textureHeight), glm::vec3());
      Vertices.emplace_back(tl, uv, glm::vec3());
      Vertices.emplace_back(tr, glm::vec2(uv.x + textureWidth, uv.y), glm::vec3());
      Vertices.emplace_back(br, glm::vec2(uv.x + textureWidth, uv.y + textureHeight), glm::vec3());

      return Mesh(move(Vertices), { 0, 3, 2, 0, 2, 1 }, { });
   }

   std::vector<Vertex> Vertices;
   Vertices.emplace_back(bl, uv, glm::vec3());
   Vertices.emplace_back(tl, glm::vec2(uv.x, uv.y + textureHeight), glm::vec3());
   Vertices.emplace_back(tr, glm::vec2(uv.x + textureWidth, uv.y + textureHeight), glm::vec3());
   Vertices.emplace_back(br, glm::vec2(uv.x + textureWidth, uv.y), glm::vec3());

   assert(windingOrder == GL_CW && "unknown winding order!");
   return Mesh(move(Vertices), { 0, 2, 3, 0, 1, 2 }, { });
}

Mesh Mesh::createCube(Application &Ctx, llvm::StringRef texture)
{
   return createCube(Ctx.loadTexture(BasicTexture::DIFFUSE, texture));
}

static void addCubeFace(Block::FaceMask face, std::vector<unsigned> &Indices,
                        std::vector<Vertex> &Vertices,
                        const BoundingBox &boundingBox,
                        glm::vec2 uv = glm::vec2(0.0f, 0.0f),
                        float textureWidth = 1.0f,
                        float textureHeight = 1.0f) {
   glm::vec3 points[4];
   glm::vec3 normal;

   // bottom left, top left, top right, bottom right
   switch (face) {
   case Block::F_Right:
      // right face
      points[0] = boundingBox.fbr();
      points[1] = boundingBox.ftr();
      points[2] = boundingBox.btr();
      points[3] = boundingBox.bbr();

      normal = glm::vec3(1.0f, 0.0f, 0.0f);

      break;
   case Block::F_Left:
      // left face
      points[0] = boundingBox.bbl();
      points[1] = boundingBox.btl();
      points[2] = boundingBox.ftl();
      points[3] = boundingBox.fbl();

      normal = glm::vec3(-1.0f, 0.0f, 0.0f);

      break;
   case Block::F_Top:
      // top face
      points[0] = boundingBox.ftl();
      points[1] = boundingBox.btl();
      points[2] = boundingBox.btr();
      points[3] = boundingBox.ftr();

      normal = glm::vec3(0.0f, 1.0f, 0.0f);

      break;
   case Block::F_Bottom:
      // bottom face
      points[0] = boundingBox.bbl();
      points[1] = boundingBox.fbl();
      points[2] = boundingBox.fbr();
      points[3] = boundingBox.bbr();

      normal = glm::vec3(0.0f, -1.0f, 0.0f);

      break;
   case Block::F_Front:
      // front face
      points[0] = boundingBox.fbl();
      points[1] = boundingBox.ftl();
      points[2] = boundingBox.ftr();
      points[3] = boundingBox.fbr();

      normal = glm::vec3(0.0f, 0.0f, 1.0f);

      break;
   case Block::F_Back:
      // back face
      points[0] = boundingBox.bbr();
      points[1] = boundingBox.btr();
      points[2] = boundingBox.btl();
      points[3] = boundingBox.bbl();

      normal = glm::vec3(0.0f, 0.0f, -1.0f);

      break;
   default:
      llvm_unreachable("bad face index");
   }

   // bottom left
   size_t idx0 = Vertices.size();
   Vertices.emplace_back(points[0], glm::vec2(uv.x, uv.y + textureHeight), normal);

   // top left
   size_t idx1 = Vertices.size();
   Vertices.emplace_back(points[1], glm::vec2(uv.x, uv.y), normal);

   // top right
   size_t idx2 = Vertices.size();
   Vertices.emplace_back(points[2], glm::vec2(uv.x + textureWidth, uv.y), normal);

   // bottom right
   size_t idx3 = Vertices.size();
   Vertices.emplace_back(points[3], glm::vec2(uv.x + textureWidth, uv.y + textureHeight), normal);

   // first triangle (top left - bottom left - bottom right)
   Indices.emplace_back(idx1);
   Indices.emplace_back(idx0);
   Indices.emplace_back(idx3);

   // second triangle (top left - bottom right - top right)
   Indices.emplace_back(idx1);
   Indices.emplace_back(idx3);
   Indices.emplace_back(idx2);
}

Mesh Mesh::createBoundingBox(mc::Application &Ctx,
                             const mc::BoundingBox &boundingBox) {
   std::vector<unsigned> Indices;
   std::vector<Vertex> Vertices;

   for (unsigned i = 0; i < 6; ++i) {
      addCubeFace(Block::face(i), Indices, Vertices, boundingBox);
   }

   return Mesh(move(Vertices), move(Indices), {});
}

Mesh Mesh::createCube(BasicTexture *texture)
{
   static Mesh Cube;
   if (Cube.Indices.empty()) {
      std::vector<unsigned> Indices;
      Indices.reserve(36);

      std::vector<Vertex> Vertices;
      Vertices.reserve(24);

      BoundingBox boundingBox = BoundingBox::unitCube();
      for (unsigned i = 0; i < 6; ++i) {
         addCubeFace(Block::face(i), Indices, Vertices, boundingBox);
      }

      Cube = Mesh(move(Vertices), move(Indices), {});
   }

   return Mesh(std::vector<Vertex>(Cube.Vertices),
               std::vector<unsigned>(Cube.Indices),
               std::vector<Texture>{ Texture(texture, Material()) });
}

void Mesh::initializeMesh()
{
   if (VAO) {
      return;
   }

   glGenVertexArrays(1, &VAO);

   glGenBuffers(1, &VBO);
   glGenBuffers(1, &EBO);

   glBindVertexArray(VAO);
   glBindBuffer(GL_ARRAY_BUFFER, VBO);

   glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(Vertex),
                Vertices.data(), GL_STATIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, Indices.size() * sizeof(unsigned),
                Indices.data(), GL_STATIC_DRAW);

   // vertex positions
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);

   // vertex texture coords
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                         (void*)offsetof(Vertex, Texture));

   // vertex normals
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                         (void*)offsetof(Vertex, Normal));

   glBindVertexArray(0);
}

void Mesh::bind(const mc::Shader &shader) const
{
   shader.useShader();

   unsigned int diffuseNr = 1;
   unsigned int specularNr = 1;
   unsigned int normalNr = 1;
   unsigned int heightNr = 1;

   unsigned NumTextures = (unsigned)Textures.size();

   for (unsigned int i = 0; i < NumTextures; i++) {
      glActiveTexture(GL_TEXTURE0 + i);

      const Texture &T = Textures[i];
      if (T.texture->getGLTextureKind() == GL_TEXTURE_CUBE_MAP) {
         shader.setUniform("cubemap", (GLint)i);
      }
      else {
         assert(T.texture->getGLTextureKind() == GL_TEXTURE_2D);

         std::string name;
         switch (T.texture->getKind()) {
         case BasicTexture::DIFFUSE:
            name += "textureDiffuse";
            name += std::to_string(diffuseNr++);
            break;
         case BasicTexture::SPECULAR:
            name += "textureSpecular";
            name += std::to_string(specularNr++);
            break;
         case BasicTexture::NORMAL:
            name += "textureNormal";
            name += std::to_string(normalNr++);
            break;
         case BasicTexture::HEIGHT:
            name += "textureHeight";
            name += std::to_string(heightNr++);
            break;
         }

         shader.setUniform(name.c_str(), (GLint)i);
      }

      glBindTexture(T.texture->getGLTextureKind(), T.texture->getTextureID());
   }

   glBindVertexArray(VAO);
}

void Mesh::render(const Shader &shader,
                  glm::mat4 viewProjectionMatrix,
                  glm::mat4 modelMatrix) const {
   bind(shader);
   shader.setUniform("MVP", viewProjectionMatrix * modelMatrix);

   // Render mesh
   glDrawElements(GL_TRIANGLES, Indices.size(), GL_UNSIGNED_INT, nullptr);

   // Reset values.
   glBindVertexArray(0);
   glActiveTexture(GL_TEXTURE0);
}

void Mesh::dump() const
{
   print(llvm::errs());
}

void Mesh::print(llvm::raw_ostream &OS) const
{
   OS << "Vertices:";

   for (unsigned i = 0; i < Indices.size(); i += 3) {
      OS << "\n   "
         << "(" << Vertices[Indices[i]].Position.x << ", "
         << Vertices[Indices[i]].Position.y << ", "
         << Vertices[Indices[i]].Position.z << ") "
         << "(" << Vertices[Indices[i + 1]].Position.x << ", "
         << Vertices[Indices[i + 1]].Position.y << ", "
         << Vertices[Indices[i + 1]].Position.z << ") "
         << "(" << Vertices[Indices[i + 2]].Position.x << ", "
         << Vertices[Indices[i + 2]].Position.y << ", "
         << Vertices[Indices[i + 2]].Position.z << ") ";
   }

   OS << "\nUVs:";

   for (unsigned i = 0; i < Indices.size(); i += 3) {
      OS << "\n   "
         << "(" << Vertices[Indices[i]].Texture.x << ", "
         << Vertices[Indices[i]].Texture.y << ") "
         << "(" << Vertices[Indices[i + 1]].Texture.x << ", "
         << Vertices[Indices[i + 1]].Texture.y << ") "
         << "(" << Vertices[Indices[i + 2]].Texture.x << ", "
         << Vertices[Indices[i + 2]].Texture.y << ") ";
   }

   OS << "\nNormals:";

   for (unsigned i = 0; i < Indices.size(); i += 3) {
      OS << "\n   "
         << "(" << Vertices[Indices[i]].Normal.x << ", "
         << Vertices[Indices[i]].Normal.y << ", "
         << Vertices[Indices[i]].Normal.z << ") "
         << "(" << Vertices[Indices[i + 1]].Normal.x << ", "
         << Vertices[Indices[i + 1]].Normal.y << ", "
         << Vertices[Indices[i + 1]].Normal.z << ") "
         << "(" << Vertices[Indices[i + 2]].Normal.x << ", "
         << Vertices[Indices[i + 2]].Normal.y << ", "
         << Vertices[Indices[i + 2]].Normal.z << ") ";
   }

   OS << "\n";
}

void ChunkMesh::addFace(Application &C, const Block &block, unsigned faceMask)
{
   auto &boundingBox = block.getBoundingBox();

   float blockWidth = C.blockTextures.getTextureWidth();
   float blockHeight = C.blockTextures.getTextureHeight();

   Mesh *mesh;
   if (block.is(Block::Water)) {
      mesh = &waterMesh;
   }
   else if (block.isTransparent()) {
      mesh = &translucentMesh;
   }
   else {
      mesh = &terrainMesh;
   }

   for (unsigned i = 0; i < 6; ++i) {
      if ((faceMask & (1 << i)) == 0) {
         continue;
      }

      Block::FaceMask face = Block::face(i);
      addCubeFace(face, mesh->Indices, mesh->Vertices, boundingBox,
                  block.getTextureUV(face),
                  blockWidth, blockHeight);
   }
}

void ChunkMesh::finalize() const
{
   terrainMesh.initializeMesh();
   translucentMesh.initializeMesh();
   waterMesh.initializeMesh();
}

Model::Model(llvm::MutableArrayRef<Mesh> Meshes)
   : NumMeshes((unsigned)Meshes.size()),
     boundingBoxCalculated(false), boundingSphereCalculated(false)
{
   assert(!Meshes.empty());

   if (Meshes.size() == 1) {
      new(&SingleMesh) Mesh(std::move(Meshes.front()));
   }
   else {
      MultipleMeshes = new Mesh[NumMeshes];
      for (unsigned i = 0; i < NumMeshes; ++i) {
         MultipleMeshes[i] = std::move(Meshes[i]);
      }
   }
}

Model::Model(Model &&Other) noexcept
   : NumMeshes(Other.NumMeshes),
     boundingBoxCalculated(Other.boundingBoxCalculated),
     boundingSphereCalculated(Other.boundingSphereCalculated),
     boundingBox(Other.boundingBox),
     boundingSphere(Other.boundingSphere)
{
   if (Other.NumMeshes == 1) {
      new(&SingleMesh) Mesh(std::move(Other.SingleMesh));
   }
   else {
      MultipleMeshes = Other.MultipleMeshes;
      Other.MultipleMeshes = nullptr;
   }

   Other.NumMeshes = 0;
}

Model::~Model()
{
   if (NumMeshes != 1) {
      delete[] MultipleMeshes;
   }
   else {
      SingleMesh.~Mesh();
   }
}

Model& Model::operator=(Model &&Other) noexcept
{
   this->~Model();
   NumMeshes = Other.NumMeshes;
   boundingBoxCalculated = Other.boundingBoxCalculated;
   boundingSphereCalculated = Other.boundingSphereCalculated;
   boundingBox = Other.boundingBox;
   boundingSphere = Other.boundingSphere;

   if (Other.NumMeshes == 1) {
      new(&SingleMesh) Mesh(std::move(Other.SingleMesh));
   }
   else {
      MultipleMeshes = Other.MultipleMeshes;
      Other.MultipleMeshes = nullptr;
   }

   Other.NumMeshes = 0;
   return *this;
}

llvm::ArrayRef<Mesh> Model::getMeshes() const
{
   if (NumMeshes == 1) {
      return { &SingleMesh, 1 };
   }

   return { MultipleMeshes, NumMeshes };
}

llvm::MutableArrayRef<Mesh> Model::getMeshes()
{
   if (NumMeshes == 1) {
      return { &SingleMesh, 1 };
   }

   return { MultipleMeshes, NumMeshes };
}

const BoundingBox &Model::getBoundingBox()
{
   if (boundingBoxCalculated) {
      return boundingBox;
   }

   bool first = true;
   float xMax = 0.0f, xMin = 0.0f,
      yMax = 0.0f, yMin = 0.0f,
      zMax = 0.0f, zMin = 0.0f;

   for (auto &mesh : getMeshes()) {
      for (auto &vert : mesh.Vertices) {
         if (first) {
            xMax = vert.Position.x;
            xMin = vert.Position.x;
            yMax = vert.Position.y;
            yMin = vert.Position.y;
            zMax = vert.Position.z;
            zMin = vert.Position.z;

            first = false;
         }
         else {
            if (vert.Position.x > xMax) {
               xMax = vert.Position.x;
            }
            else if (vert.Position.x < xMin) {
               xMin = vert.Position.x;
            }

            if (vert.Position.y > yMax) {
               yMax = vert.Position.y;
            }
            else if (vert.Position.y < yMin) {
               yMin = vert.Position.y;
            }

            if (vert.Position.z > zMax) {
               zMax = vert.Position.z;
            }
            else if (vert.Position.z < zMin) {
               zMin = vert.Position.z;
            }
         }
      }
   }

   auto &BB = boundingBox;
   BB.minX = xMin;
   BB.maxX = xMax;
   BB.minY = yMin;
   BB.maxY = yMax;
   BB.minZ = zMin;
   BB.maxZ = zMax;

   boundingBoxCalculated = true;
   return BB;
}

const BoundingSphere &Model::getBoundingSphere()
{
   if (boundingSphereCalculated) {
      return boundingSphere;
   }

//   const BoundingBox &boundingBox = getBoundingBox();
//   float maxDistance = 0.0f;
//
//   for (auto &corner : boundingBox.corners) {
//      float dist = distance(boundingBox.center, corner);
//      if (dist > maxDistance) {
//         maxDistance = dist;
//      }
//   }
//
//   boundingSphere.center = boundingBox.center();
//   boundingSphere.radius = maxDistance;
//   boundingSphereCalculated = true;

   return boundingSphere;
}

void Model::render(const Shader &S,
                   glm::mat4 modelMatrix,
                   glm::mat4 viewProjectionMatrix) const {
   for (auto &M : getMeshes()) {
      M.render(S, viewProjectionMatrix, modelMatrix);
   }
}

void Model::renderWithBorders(const Shader &shader,
                              const Shader &borderShader,
                              glm::vec4 borderColor,
                              glm::mat4 modelMatrix,
                              glm::mat4 viewProjectionMatrix) const {
   // Render normally while recording in the stencil buffer.
   glEnable(GL_STENCIL_TEST);
   glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
   glStencilFunc(GL_ALWAYS, 1, 0xFF);
   glStencilMask(0xFF);

   render(shader, modelMatrix, viewProjectionMatrix);

   // Draw again with the border shader.
   borderShader.useShader();
   borderShader.setUniform("borderColor", borderColor);

   glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
   glStencilMask(0x00);

   glm::mat4 scaledMatrix = glm::scale(modelMatrix, glm::vec3(1.02f));
   llvm::SaveAndRestore<glm::mat4> SAR(modelMatrix, scaledMatrix);

   render(borderShader, modelMatrix, viewProjectionMatrix);

   glStencilMask(0xFF);
   glEnable(GL_DEPTH_TEST);
   glDisable(GL_STENCIL_TEST);
}

void Model::renderNormals(mc::Application &Ctx, glm::vec4 normalColor,
                          glm::mat4 modelMatrix,
                          glm::mat4 viewProjectionMatrix) const {
   auto &shader = Ctx.getShader(Application::NORMAL_SHADER);
   shader.setUniform("singleColor", normalColor);

   render(shader, modelMatrix, viewProjectionMatrix);
}

static void loadTexture(Application &Ctx, aiMaterial *Mat, aiTextureType Kind,
                        std::vector<Texture> &Textures,
                        llvm::StringRef Path) {
   float shininess;
   if (Mat->Get(AI_MATKEY_SHININESS, shininess) != AI_SUCCESS) {
      shininess = 0.0f;
   }

   unsigned NumTextures = Mat->GetTextureCount(Kind);

   BasicTexture::Kind TextureKind;
   switch (Kind) {
   case aiTextureType_DIFFUSE:
      TextureKind = BasicTexture::DIFFUSE;
      break;
   case aiTextureType_SPECULAR:
      TextureKind = BasicTexture::SPECULAR;
      break;
   case aiTextureType_NORMALS:
      TextureKind = BasicTexture::NORMAL;
      break;
   case aiTextureType_HEIGHT:
      TextureKind = BasicTexture::HEIGHT;
      break;
   default:
      TextureKind = BasicTexture::DIFFUSE;
      break;
   }

   std::string pathStr = Path;

   for (unsigned i = 0; i < NumTextures; ++i) {
      aiString str;
      Mat->GetTexture(Kind, i, &str);

      std::string fileName(str.C_Str(), str.length);
      std::string realFile = findFileInDirectories(fileName, pathStr);

      if (realFile.empty()) {
         continue;
      }

      auto TexPtr = Ctx.loadTexture(TextureKind, realFile);
      if (TexPtr) {
         Textures.emplace_back(TexPtr, Material{shininess});
      }
   }
}

static void processMesh(Application &Ctx, aiMesh *M, const aiScene *Scene,
                        llvm::SmallVectorImpl<Mesh> &Meshes,
                        llvm::StringRef Path) {
   std::vector<Vertex> vertices;
   std::vector<unsigned> indices;
   std::vector<Texture> textures;

   bool hasTexture = M->mTextureCoords[0];
   for (unsigned i = 0; i < M->mNumVertices; i++) {
      Vertex &vertex = vertices.emplace_back();

      auto &V = M->mVertices[i];
      vertex.Position = glm::vec3(V.x, V.y, V.z);

      if (hasTexture) {
         auto &T = M->mTextureCoords[0][i];
         vertex.Texture = glm::vec2(T.x, T.y);
      }
      else {
         vertex.Texture = glm::vec2(0.0f, 0.0f);
      }

      auto &N = M->mNormals[i];
      vertex.Normal = glm::vec3(N.x, N.y, N.z);
   }

   for (unsigned int i = 0; i < M->mNumFaces; i++) {
      aiFace &face = M->mFaces[i];
      for (unsigned int j = 0; j < face.mNumIndices; j++) {
         indices.push_back(face.mIndices[j]);
      }
   }

   // process material
   if (M->mMaterialIndex >= 0) {
      aiMaterial *material = Scene->mMaterials[M->mMaterialIndex];
      loadTexture(Ctx, material, aiTextureType_DIFFUSE, textures, Path);
      loadTexture(Ctx, material, aiTextureType_SPECULAR, textures, Path);
      loadTexture(Ctx, material, aiTextureType_NORMALS, textures, Path);
      loadTexture(Ctx, material, aiTextureType_HEIGHT, textures, Path);
   }

   Meshes.emplace_back(move(vertices), move(indices), move(textures));
}

static void processNode(Application &Ctx, aiNode *Node, const aiScene *Scene,
                        llvm::SmallVectorImpl<Mesh> &Meshes,
                        llvm::StringRef Path) {
   // process all the node's meshes (if any)
   for (unsigned int i = 0; i < Node->mNumMeshes; i++) {
      processMesh(Ctx, Scene->mMeshes[Node->mMeshes[i]], Scene, Meshes, Path);
   }

   // then do the same for each of its children
   for (unsigned int i = 0; i < Node->mNumChildren; i++) {
      processNode(Ctx, Node->mChildren[i], Scene, Meshes, Path);
   }
}

llvm::Optional<Model> Model::loadFromFile(Application &Ctx,
                                          llvm::StringRef FileName) {
   Assimp::Importer Importer;
   const aiScene *scene = Importer.ReadFile(FileName.str(),
      aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

   if (!scene
   || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
   || !scene->mRootNode) {
      llvm::errs() << "ERROR::ASSIMP::" << Importer.GetErrorString() << "\n";
      return llvm::None;
   }

   llvm::StringRef Path = getPath(FileName);

   llvm::SmallVector<Mesh, 2> Meshes;
   processNode(Ctx, scene->mRootNode, scene, Meshes, Path);

   return Model(Meshes);
}