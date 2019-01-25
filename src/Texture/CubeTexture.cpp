//
// Created by Jonas Zell on 2019-01-17.
//

#include "CubeTexture.h"

llvm::Optional<CubeTexture>
CubeTexture::fromFiles(const CubeTextureArray &Files)
{
   GLuint textureID;
   glGenTextures(1, &textureID);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

   unsigned i = 0;
   for (auto &File : Files) {
      sf::Image image;
      if (!image.loadFromFile(File)) {
         return llvm::None;
      }

      auto param  = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
      auto width  = image.getSize().x;
      auto height = image.getSize().y;

      glTexImage2D(param, 0, GL_RGBA, width, height,
                   0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelsPtr());

      ++i;
   }

   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   return CubeTexture(textureID);
}

CubeTexture::CubeTexture(GLuint textureID) : textureID(textureID)
{

}

CubeTexture::~CubeTexture()
{
   glDeleteTextures(1, &textureID);
}

void CubeTexture::bindTexture() const
{
   glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
}