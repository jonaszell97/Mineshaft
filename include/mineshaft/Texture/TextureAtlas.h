//
// Created by Jonas Zell on 2019-01-17.
//

#ifndef MINEKAMPF_MULTITEXTURE_H
#define MINEKAMPF_MULTITEXTURE_H

#include "mineshaft/Texture/BasicTexture.h"

namespace mc {

class TextureAtlas {
   explicit TextureAtlas(std::string &&FileName,
                         sf::Image &&Img);

   // The image containing all textures.
   sf::Image Img;

   /// Name of the file this texture was loaded from.
   std::string fileName;

public:
   static llvm::Optional<TextureAtlas> fromFile(llvm::StringRef FileName);

   TextureAtlas(const TextureAtlas&) = delete;
   TextureAtlas &operator=(const TextureAtlas&) = delete;

   TextureAtlas(TextureAtlas&&) = default;
   TextureAtlas &operator=(TextureAtlas&&) = default;

   BasicTexture *getTexture(Context &Ctx,
                            BasicTexture::Kind kind,
                            GLuint BeginX, GLuint BeginY,
                            GLuint EndX = -1, GLuint EndY = -1) const;

   sf::Image getTextureImg(GLuint BeginX, GLuint BeginY,
                           GLuint EndX = -1, GLuint EndY = -1) const;
};

} // namespace mc

#endif //MINEKAMPF_MULTITEXTURE_H
