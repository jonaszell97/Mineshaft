//
// Created by Jonas Zell on 2019-01-18.
//

#include "Mesh.h"

Mesh Mesh::Cube;

static unsigned cube_indices[] = {
   0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,0,18,1,3,
   19,4,6,20,7,9,21,10,12,22,13,15,23,16,
};

Mesh Mesh::createCube(BasicTexture &&Texture)
{
   if (Cube.Indices.empty()) {
      Cube.Indices.resize(sizeof(cube_indices) / sizeof(unsigned));
      std::copy(cube_indices,
                cube_indices + (sizeof(cube_indices) / sizeof(unsigned)),
                Cube.Indices.data());

      Cube.Coordinates.reserve(24);
      Cube.Coordinates.emplace_back(1.0f, -1.0f, 1.0f);
      Cube.Coordinates.emplace_back(-1.0f, -1.0f, -1.0f);
      Cube.Coordinates.emplace_back(1.0f, -1.0f, -1.0f);
      Cube.Coordinates.emplace_back(-1.0f, 1.0f, -1.0f);
      Cube.Coordinates.emplace_back(1.0f, 1.0f, 1.0f);
      Cube.Coordinates.emplace_back(1.0f, 1.0f, -1.0f);
      Cube.Coordinates.emplace_back(1.0f, 1.0f, -1.0f);
      Cube.Coordinates.emplace_back(1.0f, -1.0f, 1.0f);
      Cube.Coordinates.emplace_back(1.0f, -1.0f, -1.0f);
      Cube.Coordinates.emplace_back(1.0f, 1.0f, 1.0f);
      Cube.Coordinates.emplace_back(-1.0f, -1.0f, 1.0f);
      Cube.Coordinates.emplace_back(1.0f, -1.0f, 1.0f);
      Cube.Coordinates.emplace_back(-1.0f, -1.0f, 1.0f);
      Cube.Coordinates.emplace_back(-1.0f, 1.0f, -1.0f);
      Cube.Coordinates.emplace_back(-1.0f, -1.0f, -1.0f);
      Cube.Coordinates.emplace_back(1.0f, -1.0f, -1.0f);
      Cube.Coordinates.emplace_back(-1.0f, 1.0f, -1.0f);
      Cube.Coordinates.emplace_back(1.0f, 1.0f, -1.0f);
      Cube.Coordinates.emplace_back(-1.0f, -1.0f, 1.0f);
      Cube.Coordinates.emplace_back(-1.0f, 1.0f, 1.0f);
      Cube.Coordinates.emplace_back(1.0f, 1.0f, 1.0f);
      Cube.Coordinates.emplace_back(-1.0f, 1.0f, 1.0f);
      Cube.Coordinates.emplace_back(-1.0f, 1.0f, 1.0f);
      Cube.Coordinates.emplace_back(-1.0f, -1.0f, -1.0f);

      Cube.UVs.reserve(23);
      Cube.UVs.emplace_back(1.0f, 0.0f);
      Cube.UVs.emplace_back(0.0f, 1.0f);
      Cube.UVs.emplace_back(0.0f, 0.0f);
      Cube.UVs.emplace_back(1.0f, 0.0f);
      Cube.UVs.emplace_back(0.0f, 1.0f);
      Cube.UVs.emplace_back(0.0f, 0.0f);
      Cube.UVs.emplace_back(1.0f, 0.0f);
      Cube.UVs.emplace_back(0.0f, 1.0f);
      Cube.UVs.emplace_back(0.0f, 0.0f);
      Cube.UVs.emplace_back(1.0f, 0.0f);
      Cube.UVs.emplace_back(0.0f, 1.0f);
      Cube.UVs.emplace_back(0.0f, 0.0f);
      Cube.UVs.emplace_back(0.0f, 0.0f);
      Cube.UVs.emplace_back(1.0f, 1.0f);
      Cube.UVs.emplace_back(0.0f, 1.0f);
      Cube.UVs.emplace_back(1.0f, 0.0f);
      Cube.UVs.emplace_back(0.0f, 1.0f);
      Cube.UVs.emplace_back(0.0f, 0.0f);
      Cube.UVs.emplace_back(1.0f, 1.0f);
      Cube.UVs.emplace_back(1.0f, 1.0f);
      Cube.UVs.emplace_back(1.0f, 1.0f);
      Cube.UVs.emplace_back(1.0f, 1.0f);
      Cube.UVs.emplace_back(1.0f, 0.0f);
      Cube.UVs.emplace_back(1.0f, 1.0f);

      Cube.Normals.reserve(23);
      Cube.Normals.emplace_back(0.0f, -1.0f, 0.0f);
      Cube.Normals.emplace_back(0.0f, -1.0f, 0.0f);
      Cube.Normals.emplace_back(0.0f, -1.0f, 0.0f);
      Cube.Normals.emplace_back(0.0f, 1.0f, 0.0f);
      Cube.Normals.emplace_back(0.0f, 1.0f, 0.0f);
      Cube.Normals.emplace_back(0.0f, 1.0f, 0.0f);
      Cube.Normals.emplace_back(1.0f, -0.0f, 0.0f);
      Cube.Normals.emplace_back(1.0f, -0.0f, 0.0f);
      Cube.Normals.emplace_back(1.0f, -0.0f, 0.0f);
      Cube.Normals.emplace_back(0.0f, -0.0f, 1.0f);
      Cube.Normals.emplace_back(0.0f, -0.0f, 1.0f);
      Cube.Normals.emplace_back(0.0f, -0.0f, 1.0f);
      Cube.Normals.emplace_back(-1.0f, -0.0f, -0.0f);
      Cube.Normals.emplace_back(-1.0f, -0.0f, -0.0f);
      Cube.Normals.emplace_back(-1.0f, -0.0f, -0.0f);
      Cube.Normals.emplace_back(0.0f, 0.0f, -1.0f);
      Cube.Normals.emplace_back(0.0f, 0.0f, -1.0f);
      Cube.Normals.emplace_back(0.0f, 0.0f, -1.0f);
      Cube.Normals.emplace_back(0.0f, -1.0f, 0.0f);
      Cube.Normals.emplace_back(0.0f, 1.0f, 0.0f);
      Cube.Normals.emplace_back(1.0f, -0.0f, 0.0f);
      Cube.Normals.emplace_back(0.0f, -0.0f, 1.0f);
      Cube.Normals.emplace_back(-1.0f, -0.0f, -0.0f);
      Cube.Normals.emplace_back(0.0f, 0.0f, -1.0f);
   }

   Mesh M;
   M.Indices = Cube.Indices;
   M.Coordinates = Cube.Coordinates;
   M.UVs = Cube.UVs;
   M.Normals = Cube.Normals;
   M.Texture = std::move(Texture);

   return M;
}

void Mesh::dump() const
{
   print(llvm::errs());
}

void Mesh::print(llvm::raw_ostream &OS) const
{
   OS << "Indices: \n";
   for (unsigned Idx : Indices) {
      OS << Idx << ",";
   }

   OS << "\n\nCoordinates: \n";
   for (auto &Coord : Coordinates) {
      OS << Coord.x << "f, "
         << Coord.y << "f, "
         << Coord.z << "f,\n";
   }

   OS << "\n\nUVs: \n";
   for (auto &UV : UVs) {
      OS << UV.x << "f, "
         << UV.y << "f,\n";
   }

   OS << "\n\nNormals: \n";
   for (auto &Norm : Normals) {
      OS << Norm.x << "f, "
         << Norm.y << "f, "
         << Norm.z << "f,\n";
   }
}