//
// Created by Jonas Zell on 2019-01-17.
//

#ifndef MINEKAMPF_BASICTEXTURE_H
#define MINEKAMPF_BASICTEXTURE_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <SFML/Graphics.hpp>

#include <llvm/ADT/FoldingSet.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringRef.h>

#include <string>

namespace mc {

class Context;

class BasicTexture: public llvm::FoldingSetNode {
public:
   enum Kind {
      DIFFUSE, SPECULAR, NORMAL, HEIGHT,
   };

private:
   explicit BasicTexture(Kind K, GLuint textureID, std::string &&File,
                         unsigned glTextureKind = GL_TEXTURE_2D);

   GLuint textureID;
   Kind TextureKind;
   std::string File;
   unsigned glTextureKind;

public:
   BasicTexture();
   ~BasicTexture();

   BasicTexture(BasicTexture &&Tex) noexcept = default;
   BasicTexture &operator=(BasicTexture &&Tex) noexcept = default;

   BasicTexture(const BasicTexture&) = delete;
   BasicTexture &operator=(const BasicTexture&) = delete;

   friend class Context;

   void Profile(llvm::FoldingSetNodeID &ID);
   static void Profile(llvm::FoldingSetNodeID &ID, Kind K,
                       llvm::StringRef File);

   void bindTexture() const;

   GLuint getTextureID() const { return textureID; }
   Kind getKind() const { return TextureKind; }
   unsigned getGLTextureKind() const { return glTextureKind; }

   operator bool() const { return textureID != 0; }
};

struct Material {
   float shininess;
};

struct Texture {
   BasicTexture *texture;
   Material material;

   Texture(BasicTexture *texture, Material material)
      : texture(texture), material(material)
   { }
};

} // namespace mc

#endif //MINEKAMPF_BASICTEXTURE_H
