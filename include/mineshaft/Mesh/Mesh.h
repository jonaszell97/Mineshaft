#ifndef MINESHAFT_MESH_H
#define MINESHAFT_MESH_H

#include "Texture/BasicTexture.h"

#include <llvm/Support/raw_ostream.h>

#include <glm/glm.hpp>
#include <vector>

struct Mesh {
private:
   static Mesh Cube;

public:
   std::vector<glm::vec2> UVs;
   std::vector<glm::vec3> Normals;
   std::vector<glm::vec3> Coordinates;
   std::vector<unsigned> Indices;
   BasicTexture Texture;

   /// Create a cube mesh without loading an .obj file.
   static Mesh createCube(BasicTexture &&Texture = BasicTexture());

   /// Print this mesh for debugging.
   void print(llvm::raw_ostream &OS) const;
   void dump() const;
};

#endif //MINESHAFT_MESH_H
