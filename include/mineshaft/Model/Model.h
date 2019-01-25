//
// Created by Jonas Zell on 2019-01-17.
//

#ifndef MINEKAMPF_MODEL_H
#define MINEKAMPF_MODEL_H

#include "mineshaft/Texture/BasicTexture.h"
#include "mineshaft/Shader/Shader.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/Optional.h>
#include <llvm/Support/raw_ostream.h>

class aiScene;
class aiNode;
class aiMesh;

namespace mc {

class TextureAtlas;

struct BoundingBox {
   std::array<glm::vec3, 8> corners;
   glm::vec3 center;
};

struct BoundingSphere {
   glm::vec3 center;
   float radius;
};

struct Vertex {
   glm::vec3 Position;
   glm::vec2 Texture;
   glm::vec3 Normal;

   Vertex(glm::vec3 Position,
          glm::vec2 Texture,
          glm::vec3 Normal)
      : Position(Position), Texture(Texture), Normal(Normal)
   { }

   Vertex() = default;
};

struct Mesh {
   std::vector<Vertex> Vertices;
   std::vector<unsigned> Indices;
   std::vector<Texture> Textures;

   /// The VAO for this mesh.
   GLuint VAO = 0;

   /// The VBO for this mesh.
   GLuint VBO = 0;

   /// The EBO for this mesh.
   GLuint EBO = 0;

private:
   /// Initialize the mesh.
   void initializeMesh();

public:
   /// Memberwise C'tor.
   Mesh(std::vector<Vertex> &&Vertices,
        std::vector<unsigned> &&Indices,
        std::vector<Texture> &&Textures);

   /// Disable copy construction.
   Mesh(const Mesh&) = delete;
   Mesh &operator=(const Mesh&) = delete;

   Mesh(Mesh&&) noexcept = default;
   Mesh &operator=(Mesh&&) noexcept;

   /// Default C'tor.
   Mesh() = default;

   /// Create a triangle mesh.
   static Mesh createTriangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);

   /// Create a cube mesh with the same texture on all sides.
   static Mesh createCube(Context &Ctx, llvm::StringRef texture);

   /// Create a cube mesh with the same texture on all sides.
   static Mesh createCube(BasicTexture *texture);

   static Mesh createCube(const TextureAtlas &texture,
                          unsigned textureX,
                          unsigned textureY);

   /// Create a cube mesh with the same texture on all sides.
   static Mesh createCube(const std::array<BasicTexture*, 6> &texture);

   /// Render this mesh using the given shader.
   void render(const Shader &shader,
               glm::mat4 viewProjectionMatrix,
               glm::mat4 modelMatrix = glm::mat4(1.0f)) const;

   /// Bind this meshes textures and VAO.
   void bind(const Shader &shader) const;

   /// Print this mesh for debugging.
   void print(llvm::raw_ostream &OS) const;
   void dump() const;
};

class Context;

class Model {
   /// Number of meshes in this model.
   unsigned NumMeshes : 24;
   bool boundingBoxCalculated : 1;
   bool boundingSphereCalculated : 1;

   /// The bounding box of this model.
   BoundingBox boundingBox;

   /// The bounding sphere of this model.
   BoundingSphere boundingSphere;

   union {
      Mesh SingleMesh;
      Mesh *MultipleMeshes;
   };

public:
   /// Construct from an array of meshes.
   explicit Model(llvm::MutableArrayRef<Mesh> Meshes);

   /// D'tor. Deals with deconstructing the meshes.
   ~Model();

   /// Disallow copy construction.
   Model(const Model&) = delete;
   Model &operator=(const Model&) = delete;

   Model(Model &&Other) noexcept;
   Model &operator=(Model &&Other) noexcept;

   /// Load a model from a file.
   static llvm::Optional<Model> loadFromFile(Context &Ctx,
                                             llvm::StringRef FileName);

   /// Render the model using a shader.
   void render(const Shader &shader,
               glm::mat4 modelMatrix,
               glm::mat4 viewProjectionMatrix) const;

   /// Render the model with borders.
   void renderWithBorders(const Shader &shader,
                          const Shader &borderShader,
                          glm::vec4 borderColor,
                          glm::mat4 modelMatrix,
                          glm::mat4 viewProjectionMatrix) const;

   /// Render the model with borders.
   void renderNormals(Context &Ctx,
                      glm::vec4 normalColor,
                      glm::mat4 modelMatrix,
                      glm::mat4 viewProjectionMatrix) const;

   /// \return The meshes that make up this mode.
   llvm::ArrayRef<Mesh> getMeshes() const;

   /// \return The meshes that make up this mode.
   llvm::MutableArrayRef<Mesh> getMeshes();

   /// \return The bounding box of this model.
   const BoundingBox &getBoundingBox();

   /// \return The bounding sphere of this model.
   const BoundingSphere &getBoundingSphere();
};

} // namespace mc

#endif //MINEKAMPF_MODEL_H
