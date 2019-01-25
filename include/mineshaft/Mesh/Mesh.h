//
// Created by Jonas Zell on 2019-01-17.
//

#ifndef MINEKAMPF_MESH_H
#define MINEKAMPF_MESH_H

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

#endif //MINEKAMPF_MESH_H
