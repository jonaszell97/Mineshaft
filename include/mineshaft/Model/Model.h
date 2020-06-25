#ifndef MINESHAFT_MODEL_H
#define MINESHAFT_MODEL_H

#include "mineshaft/Texture/BasicTexture.h"
#include "mineshaft/Shader/Shader.h"
#include "mineshaft/utils.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/Optional.h>
#include <llvm/Support/raw_ostream.h>

class aiScene;
class aiNode;
class aiMesh;

namespace mc {

class Block;
class TextureAtlas;

struct BoundingBox {
   float minX = 0;
   float maxX = 0;

   float minY = 0;
   float maxY = 0;

   float minZ = 0;
   float maxZ = 0;

   void applyOffset(const glm::vec3 &vec);
   BoundingBox offsetBy(const glm::vec3 &vec) const;

   glm::vec3 center() const;

   // front bottom left
   // back bottom left
   // front bottom right
   // back bottom right
   glm::vec3 fbl() const;
   glm::vec3 bbl() const;
   glm::vec3 fbr() const;
   glm::vec3 bbr() const;

   // front top left
   // back top left
   // front top right
   // back top right
   glm::vec3 ftl() const;
   glm::vec3 btl() const;
   glm::vec3 ftr() const;
   glm::vec3 btr() const;

   static BoundingBox unitCube();

   std::array<glm::vec3, 8> corners() const
   {
      return { fbl(), bbl(), fbr(), bbr(), ftl(), btl(), ftr(), btr() };
   }

   /// \return true iff this bounding box collides with the given bounding box.
   bool collidesWith(const BoundingBox &other) const;

   /// \return true iff the point is within the bounding box.
   bool contains(const glm::vec3 &point) const;

   /// \return true iff the ray intersects the bounding box.
   std::pair<bool, glm::vec3> intersects(const glm::vec3 &rayOrigin,
                                         const glm::vec3 &rayDir,
                                         float minDist, float maxDist) const;

   bool operator==(const BoundingBox &other) const
   {
      return minX == other.minX
         && maxX == other.maxX
         && minY == other.minY
         && maxY == other.maxY
         && minZ == other.minZ
         && maxZ == other.maxZ;
   }

   bool operator!=(const BoundingBox &other) const
   {
      return !(*this == other);
   }
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

struct ChunkMesh;

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

protected:
   /// Initialize the mesh.
   void initializeMesh();

public:
   /// Memberwise C'tor.
   Mesh(std::vector<Vertex> &&Vertices,
        std::vector<unsigned> &&Indices,
        std::vector<Texture> &&Textures);

   ~Mesh();

   /// Disable copy construction.
   Mesh(const Mesh&) = delete;
   Mesh &operator=(const Mesh&) = delete;

   Mesh(Mesh&&) noexcept;
   Mesh &operator=(Mesh&&) noexcept;

   /// Default C'tor.
   Mesh() = default;

   friend ChunkMesh;

   /// Create a triangle mesh.
   static Mesh createTriangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);

   /// Create a quad mesh.
   static Mesh createQuad(glm::vec3 bl, glm::vec3 tl, glm::vec3 tr, glm::vec3 br,
                          glm::vec2 uv = glm::vec2(0.0f, 0.0f),
                          float textureWidth = 1.0f,
                          float textureHeight = 1.0f,
                          unsigned windingOrder = GL_CCW);

   /// Create a cube mesh with the same texture on all sides.
   static Mesh createCube(Application &Ctx, llvm::StringRef texture);

   /// Create a cube mesh with the same texture on all sides.
   static Mesh createBoundingBox(Application &Ctx, const BoundingBox &boundingBox);

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

struct ChunkMesh {
   /// The mesh containg the water in the chunk.
   mutable Mesh terrainMesh;

   /// The mesh containg translucent blocks.
   mutable Mesh translucentMesh;

   /// The mesh containg the water in the chunk.
   mutable Mesh waterMesh;

   /// Default C'tor, initializes an empty mesh.
   ChunkMesh() = default;

   /// Add a cube face to this chunk mesh.
   void addFace(Application &C, const Block &block, unsigned faceMask);

   /// Finalize the chunk mesh.
   void finalize() const;
};

class Application;

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
   static llvm::Optional<Model> loadFromFile(Application &Ctx,
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
   void renderNormals(Application &Ctx,
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

namespace std {

template <>
struct hash<::mc::BoundingBox>
{
   std::size_t operator()(const ::mc::BoundingBox& k) const noexcept
   {
      std::size_t hashVal = 0;
      hash_combine(hashVal, k.minX);
      hash_combine(hashVal, k.maxX);
      hash_combine(hashVal, k.minY);
      hash_combine(hashVal, k.maxY);
      hash_combine(hashVal, k.minZ);
      hash_combine(hashVal, k.maxZ);

      return hashVal;
   }
};

} // namespace std

#endif //MINESHAFT_MODEL_H
