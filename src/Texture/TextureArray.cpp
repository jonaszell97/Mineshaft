//
// Created by Jonas Zell on 2019-01-24.
//

#include "mineshaft/Texture/TextureArray.h"

using namespace mc;

TextureArray::TextureArray(BasicTexture::Kind textureKind, GLuint textureID,
                           GLuint glTextureKind)
   : textureKind(textureKind), textureID(textureID),
     glTextureKind(glTextureKind)
{ }

TextureArray::TextureArray() : TextureArray(BasicTexture::DIFFUSE, 0, 0)
{

}

TextureArray TextureArray::create(BasicTexture::Kind textureKind,
                                  unsigned height, unsigned width,
                                  unsigned layers) {
   glActiveTexture(GL_TEXTURE0);

   GLuint textureID;
   glGenTextures(1, &textureID);

   glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
   glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA,
                width, height, layers,
                0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

   glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
   return TextureArray(textureKind, textureID, GL_TEXTURE_2D_ARRAY);
}

TextureArray TextureArray::createCubemap(mc::BasicTexture::Kind textureKind,
                                         unsigned height, unsigned width,
                                         unsigned layers) {
   glActiveTexture(GL_TEXTURE1);

   GLuint textureID;
   glGenTextures(1, &textureID);

   glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textureID);
   glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA,
                width, height, layers * 6,
                0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

   glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, 0);
   return TextureArray(textureKind, textureID, GL_TEXTURE_CUBE_MAP_ARRAY);
}

void TextureArray::bind() const
{
   if (isCubeMap()) {
      glActiveTexture(GL_TEXTURE1);
   }
   else {
      glActiveTexture(GL_TEXTURE0);
   }

   glBindTexture(glTextureKind, textureID);
}

void TextureArray::addTexture(const sf::Image &Img, unsigned layer,
                              unsigned face) {
   unsigned factor = 1;
   if (isCubeMap()) {
      factor = 6;
   }

   bind();
   glTexSubImage3D(glTextureKind, 0, 0, 0, (layer * factor) + face,
                   Img.getSize().x, Img.getSize().y,
                   1, GL_RGBA, GL_UNSIGNED_BYTE,
                   Img.getPixelsPtr());
}

void TextureArray::finalize() const
{
   bind();

   glGenerateMipmap(glTextureKind);
   glTexParameteri(glTextureKind, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
   glTexParameteri(glTextureKind, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(glTextureKind, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(glTextureKind, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}