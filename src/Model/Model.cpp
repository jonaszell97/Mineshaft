//
// Created by Jonas Zell on 2019-01-17.
//

#include "mineshaft/Model/Model.h"
#include "mineshaft/Context.h"
#include "mineshaft/utils.h"

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

Mesh::Mesh(std::vector<Vertex> &&Vertices,
           std::vector<unsigned> &&Indices,
           std::vector<Texture> &&Textures)
   : Vertices(std::move(Vertices)), Indices(std::move(Indices)),
     Textures(std::move(Textures))
{
   initializeMesh();
}

Mesh& Mesh::operator=(Mesh &&Other) noexcept
{
   Vertices = std::move(Other.Vertices);
   Indices = std::move(Other.Indices);
   Textures = std::move(Other.Textures);
   VAO = Other.VAO;
   EBO = Other.EBO;
   VBO = Other.VBO;

   Other.VAO = 0;
   Other.VBO = 0;
   Other.EBO = 0;

   return *this;
}

Mesh Mesh::createTriangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3)
{
   std::vector<Vertex> Vertices;
   Vertices.emplace_back(p1, glm::vec2(), glm::vec3());
   Vertices.emplace_back(p2, glm::vec2(), glm::vec3());
   Vertices.emplace_back(p3, glm::vec2(), glm::vec3());

   return Mesh(move(Vertices), { 0, 1, 2 }, {});
}

Mesh Mesh::createCube(Context &Ctx, llvm::StringRef texture)
{
   return createCube(Ctx.loadTexture(BasicTexture::DIFFUSE, texture));
}

Mesh Mesh::createCube(BasicTexture *texture)
{
   static unsigned cube_indices[] = {
      // 0 - front bottom right
      // 1 - back bottom left
      // 2 - back bottom right
      // > bottom back
      0, 1, 2,

      // 3 - back top left
      // 4 - front top right
      // 5 - back top right
      // > top back
      3, 4, 5,

      // 6 - back top right
      // 7 - front bottom right
      // 8 - back bottom right
      6, 7, 8,

      // 9 - front top right
      // 10 - front bottom left
      // 11 - front bottom right
      9, 10, 11,

      // 12 - front bottom left
      // 13 - back top left
      // 14 - back bottom left
      12, 13, 14,

      // 15 - back bottom right
      // 16 - back top left
      // 17 - back top right
      15, 16, 17,

      // 0 - front bottom right
      // 18 - front bottom left
      // 1 - back bottom left
      0, 18, 1,

      // 3 - back top left
      // 19 - front top left
      // 4 - front top right
      3, 19, 4,

      // 6 - back top right
      // 20 - front top right
      // 7 - front bottom right
      6, 20, 7,

      // 9 - front top right
      // 21 - front top left
      // 10 - front bottom left
      9, 21, 10,

      // 12 - front bottom left
      // 22 - front top left
      // 13 - back top left
      12, 22, 13,

      // 15 - back bottom right
      // 23 - back bottom left
      // 16 - back top left
      15, 23, 16,
   };

   static Mesh Cube;
   if (Cube.Indices.empty()) {
      std::vector<unsigned> Indices;
      Indices.resize(sizeof(cube_indices) / sizeof(unsigned));
      std::copy(cube_indices,
                cube_indices + (sizeof(cube_indices) / sizeof(unsigned)),
                Indices.data());

      std::vector<Vertex> Vertices;
      Vertices.reserve(24);

      // 0 - front bottom right
      Vertices.emplace_back(glm::vec3(1.0f, -1.0f, 1.0f),
                            glm::vec2(1.0f, 0.0f),
                            glm::vec3(0.0f, -1.0f, 0.0f));

      // 1 - back bottom left
      Vertices.emplace_back(glm::vec3(-1.0f, -1.0f, -1.0f),
                            glm::vec2(0.0f, 1.0f),
                            glm::vec3(0.0f, -1.0f, 0.0f));

      // 2 - back bottom right
      Vertices.emplace_back(glm::vec3(1.0f, -1.0f, -1.0f),
                            glm::vec2(0.0f, 0.0f),
                            glm::vec3(0.0f, -1.0f, 0.0f));

      // 3 - back top left
      Vertices.emplace_back(glm::vec3(-1.0f, 1.0f, -1.0f),
                            glm::vec2(1.0f, 0.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));

      // 4 - front top right
      Vertices.emplace_back(glm::vec3(1.0f, 1.0f, 1.0f),
                            glm::vec2(0.0f, 1.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));

      // 5 - back top right
      Vertices.emplace_back(glm::vec3(1.0f, 1.0f, -1.0f),
                            glm::vec2(0.0f, 0.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));

      // 6 - back top right
      Vertices.emplace_back(glm::vec3(1.0f, 1.0f, -1.0f),
                            glm::vec2(1.0f, 0.0f),
                            glm::vec3(1.0f, -0.0f, 0.0f));

      // 7 - front bottom right
      Vertices.emplace_back(glm::vec3(1.0f, -1.0f, 1.0f),
                            glm::vec2(0.0f, 1.0f),
                            glm::vec3(1.0f, -0.0f, 0.0f));

      // 8 - back bottom right
      Vertices.emplace_back(glm::vec3(1.0f, -1.0f, -1.0f),
                            glm::vec2(0.0f, 0.0f),
                            glm::vec3(1.0f, -0.0f, 0.0f));

      // 9 - front top right
      Vertices.emplace_back(glm::vec3(1.0f, 1.0f, 1.0f),
                            glm::vec2(1.0f, 0.0f),
                            glm::vec3(0.0f, -0.0f, 1.0f));

      // 10 - front bottom left
      Vertices.emplace_back(glm::vec3(-1.0f, -1.0f, 1.0f),
                            glm::vec2(0.0f, 1.0f),
                            glm::vec3(0.0f, -0.0f, 1.0f));

      // 11 - front bottom right
      Vertices.emplace_back(glm::vec3(1.0f, -1.0f, 1.0f),
                            glm::vec2(0.0f, 0.0f),
                            glm::vec3(0.0f, -0.0f, 1.0f));

      // 12 - front bottom left
      Vertices.emplace_back(glm::vec3(-1.0f, -1.0f, 1.0f),
                            glm::vec2(0.0f, 0.0f),
                            glm::vec3(-1.0f, -0.0f, -0.0f));

      // 13 - back top left
      Vertices.emplace_back(glm::vec3(-1.0f, 1.0f, -1.0f),
                            glm::vec2(1.0f, 1.0f),
                            glm::vec3(-1.0f, -0.0f, -0.0f));

      // 14 - back bottom left
      Vertices.emplace_back(glm::vec3(-1.0f, -1.0f, -1.0f),
                            glm::vec2(0.0f, 1.0f),
                            glm::vec3(-1.0f, -0.0f, -0.0f));

      // 15 - back bottom right
      Vertices.emplace_back(glm::vec3(1.0f, -1.0f, -1.0f),
                            glm::vec2(1.0f, 0.0f),
                            glm::vec3(0.0f, 0.0f, -1.0f));

      // 16 - back top left
      Vertices.emplace_back(glm::vec3(-1.0f, 1.0f, -1.0f),
                            glm::vec2(0.0f, 1.0f),
                            glm::vec3(0.0f, 0.0f, -1.0f));

      // 17 - back top right
      Vertices.emplace_back(glm::vec3(1.0f, 1.0f, -1.0f),
                            glm::vec2(0.0f, 0.0f),
                            glm::vec3(0.0f, 0.0f, -1.0f));

      // 18 - front bottom left
      Vertices.emplace_back(glm::vec3(-1.0f, -1.0f, 1.0f),
                            glm::vec2(1.0f, 1.0f),
                            glm::vec3(0.0f, -1.0f, 0.0f));

      // 19 - front top left
      Vertices.emplace_back(glm::vec3(-1.0f, 1.0f, 1.0f),
                            glm::vec2(1.0f, 1.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));

      // 20 - front top right
      Vertices.emplace_back(glm::vec3(1.0f, 1.0f, 1.0f),
                            glm::vec2(1.0f, 1.0f),
                            glm::vec3(1.0f, -0.0f, 0.0f));

      // 21 - front top left
      Vertices.emplace_back(glm::vec3(-1.0f, 1.0f, 1.0f),
                            glm::vec2(1.0f, 1.0f),
                            glm::vec3(0.0f, -0.0f, 1.0f));

      // 22 - front top left
      Vertices.emplace_back(glm::vec3(-1.0f, 1.0f, 1.0f),
                            glm::vec2(1.0f, 0.0f),
                            glm::vec3(-1.0f, -0.0f, -0.0f));

      // 23 - back bottom left
      Vertices.emplace_back(glm::vec3(-1.0f, -1.0f, -1.0f),
                            glm::vec2(1.0f, 1.0f),
                            glm::vec3(0.0f, 0.0f, -1.0f));

      Cube = Mesh(move(Vertices), move(Indices), {});
   }

   return Mesh(std::vector<Vertex>(Cube.Vertices),
               std::vector<unsigned>(Cube.Indices),
               std::vector<Texture>{ Texture(texture, Material()) });
}

void Mesh::initializeMesh()
{
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

   float width = xMax - xMin;
   float height = yMax - yMin;
   float depth = zMax - zMin;

   float halfWidth = width / 2.0f;
   float halfHeight = height / 2.0f;
   float halfDepth = depth / 2.0f;

   auto &BB = boundingBox;
   BB.center = glm::vec3(xMin + halfWidth,
                         yMin + halfHeight,
                         zMin + halfDepth);

   BB.corners[0] = BB.center + glm::vec3(-halfWidth, -halfHeight, halfDepth);
   BB.corners[1] = BB.center + glm::vec3(-halfWidth, -halfHeight, -halfDepth);
   BB.corners[2] = BB.center + glm::vec3(halfWidth, -halfHeight, halfDepth);
   BB.corners[3] = BB.center + glm::vec3(halfWidth, -halfHeight, -halfDepth);

   BB.corners[4] = BB.center + glm::vec3(-halfWidth, halfHeight, halfDepth);
   BB.corners[5] = BB.center + glm::vec3(-halfWidth, halfHeight, -halfDepth);
   BB.corners[6] = BB.center + glm::vec3(halfWidth, halfHeight, halfDepth);
   BB.corners[7] = BB.center + glm::vec3(halfWidth, halfHeight, -halfDepth);

   boundingBoxCalculated = true;
   return BB;
}

const BoundingSphere &Model::getBoundingSphere()
{
   if (boundingSphereCalculated) {
      return boundingSphere;
   }

   const BoundingBox &boundingBox = getBoundingBox();
   float maxDistance = 0.0f;

   for (auto &corner : boundingBox.corners) {
      float dist = distance(boundingBox.center, corner);
      if (dist > maxDistance) {
         maxDistance = dist;
      }
   }

   boundingSphere.center = boundingBox.center;
   boundingSphere.radius = maxDistance;
   boundingSphereCalculated = true;

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

void Model::renderNormals(mc::Context &Ctx, glm::vec4 normalColor,
                          glm::mat4 modelMatrix,
                          glm::mat4 viewProjectionMatrix) const {
   auto &shader = Ctx.getShader(Context::NORMAL_SHADER);
   shader.setUniform("singleColor", normalColor);

   render(shader, modelMatrix, viewProjectionMatrix);
}

static void loadTexture(Context &Ctx, aiMaterial *Mat, aiTextureType Kind,
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

static void processMesh(Context &Ctx, aiMesh *M, const aiScene *Scene,
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

static void processNode(Context &Ctx, aiNode *Node, const aiScene *Scene,
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

llvm::Optional<Model> Model::loadFromFile(Context &Ctx,
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