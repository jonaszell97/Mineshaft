//
// Created by Jonas Zell on 2019-01-24.
//

#ifndef MINESHAFT_TEXTUREARRAY_H
#define MINESHAFT_TEXTUREARRAY_H

#include "mineshaft/Texture/BasicTexture.h"

namespace mc {

class TextureArray {
public:
   TextureArray(BasicTexture::Kind textureKind, GLuint textureID,
                GLuint glTextureKind);

private:
   TextureArray();

   /// The texture kind.
   BasicTexture::Kind textureKind;

   /// The OpenGL texture id.
   GLuint textureID;

   /// The OpenGL texture kind.
   GLuint glTextureKind;

public:
   friend class Context;

   /// Create a texture array.
   static TextureArray create(BasicTexture::Kind textureKind,
                              unsigned height, unsigned width, unsigned layers);

   /// Create a texture cubemap.
   static TextureArray createCubemap(BasicTexture::Kind textureKind,
                                     unsigned height, unsigned width,
                                     unsigned layers);

   /// Add a new texture to the texture array.
   void addTexture(const sf::Image &Img, unsigned layer,
                   unsigned face = 0);

   /// Finalize the texture array.
   void finalize() const;

   /// Bind the texture.
   void bind() const;

   /// \return true iff this texture array is a cube map.
   bool isCubeMap() const { return glTextureKind == GL_TEXTURE_CUBE_MAP_ARRAY; }
};

} // namespace mc

#endif //MINESHAFT_TEXTUREARRAY_H
