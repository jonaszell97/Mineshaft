//
// Created by Jonas Zell on 2019-01-17.
//

#ifndef MINEKAMPF_UTILS_H
#define MINEKAMPF_UTILS_H

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

#include <string>

template <class HashTy, class T>
inline void hash_combine(HashTy& seed, const T& v)
{
   std::hash<T> hasher;
   seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

namespace std {

template <>
struct hash<glm::vec2>
{
   std::size_t operator()(const glm::vec2& k) const noexcept
   {
      std::size_t hashVal = 0;
      hash_combine(hashVal, k.x);
      hash_combine(hashVal, k.y);

      return hashVal;
   }
};

template <>
struct hash<glm::vec3>
{
   std::size_t operator()(const glm::vec3& k) const noexcept
   {
      std::size_t hashVal = 0;
      hash_combine(hashVal, k.x);
      hash_combine(hashVal, k.y);
      hash_combine(hashVal, k.z);

      return hashVal;
   }
};

} // namespace std

namespace mc {

#ifdef _WIN32
static char PATH_SEPARATOR = '\\';
#else
static char PathSeperator = '/';
#endif

llvm::StringRef getPath(llvm::StringRef fullPath);
llvm::StringRef getFileNameAndExtension(llvm::StringRef fullPath);

bool fileExists(llvm::StringRef name);

std::string findFileInDirectories(llvm::StringRef fileName,
                                  llvm::ArrayRef<std::string> directories);

inline void enableWireframe()
{
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

inline void disableWireframe()
{
   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

template<glm::length_t L, typename T, glm::qualifier Q>
glm::vec<L, T, Q> triIntersect(const glm::vec<L, T, Q> &ro,
                               const glm::vec<L, T, Q> &rd,
                               const glm::vec<L, T, Q> &v0,
                               const glm::vec<L, T, Q> &v1,
                               const glm::vec<L, T, Q> &v2) {
   auto v1v0 = v1 - v0;
   auto v2v0 = v2 - v0;
   auto rov0 = ro - v0;

   auto n = cross(v1v0, v2v0);
   auto q = cross(rov0, rd);

   float d = 1.0 / dot(rd, n);
   float u = d * dot(-q, v2v0);
   float v = d * dot(q, v1v0);
   float t = d * dot(-n, rov0);

   if (u < 0.0 || u > 1.0 || v < 0.0 || (u+v) > 1.0) {
      t = -1.0f;
   }

   return glm::vec<L, T, Q>(t, u, v);
}

float float_rand(float min, float max);

glm::vec4 randomColor();

class Timer {
   llvm::StringRef taskName;
   long long beginTime;
   llvm::raw_ostream &OS;

   long long getTime() const;

public:
   explicit Timer(llvm::StringRef taskName, llvm::raw_ostream *OS = nullptr);
   ~Timer();
};

} // namespace mc

#endif //MINEKAMPF_UTILS_H
