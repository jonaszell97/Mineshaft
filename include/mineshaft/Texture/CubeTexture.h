#ifndef MINESHAFT_CUBETEXTURE_H
#define MINESHAFT_CUBETEXTURE_H

#include <GL/glew.h>
#include <SFML/Graphics.hpp>
#include <llvm/ADT/Optional.h>

#include <array>
#include <string>

class CubeTexture {
   explicit CubeTexture(GLuint textureID);

   GLuint textureID;

public:
   using CubeTextureArray = std::array<std::string, 6>;
   static llvm::Optional<CubeTexture> fromFiles(const CubeTextureArray  &Files);

   ~CubeTexture();

   void bindTexture() const;
   GLuint getTextureID() const { return textureID; }
};

#endif //MINESHAFT_CUBETEXTURE_H
