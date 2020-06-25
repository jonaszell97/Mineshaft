#ifndef MINESHAFT_MULTITEXTURE_H
#define MINESHAFT_MULTITEXTURE_H

#include "mineshaft/Texture/BasicTexture.h"

namespace mc {

class TextureAtlas {
   TextureAtlas();
   explicit TextureAtlas(BasicTexture::Kind textureKind,
                         GLuint textureID,
                         std::string &&FileName,
                         sf::Image &&Img,
                         float textureWidth,
                         float textureHeight);

   // The image containing all textures.
   sf::Image Img;

   /// Name of the file this texture was loaded from.
   std::string fileName;

   /// Texture ID of the joined texture.
   GLuint textureID;

   /// Texture kind of the joined texture.
   BasicTexture::Kind textureKind;

   /// Width of a single texture, normalized to 1.
   float textureWidth = 0.0f;

   /// Height of a single texture, normalized to 1.
   float textureHeight = 0.0f;

public:
   static llvm::Optional<TextureAtlas> fromFile(BasicTexture::Kind textureKind,
                                                llvm::StringRef FileName,
                                                float textureWidth,
                                                float textureHeight);

   friend class Application;

   TextureAtlas(const TextureAtlas&) = delete;
   TextureAtlas &operator=(const TextureAtlas&) = delete;

   TextureAtlas(TextureAtlas&&) = default;
   TextureAtlas &operator=(TextureAtlas&&) = default;

   BasicTexture *getTexture(Application &Ctx,
                            BasicTexture::Kind kind,
                            GLuint BeginX, GLuint BeginY,
                            GLuint EndX = -1, GLuint EndY = -1) const;

   sf::Image getTextureImg(GLuint BeginX, GLuint BeginY,
                           GLuint EndX = -1, GLuint EndY = -1) const;

   glm::vec2 getTextureUVCoords(GLuint BeginX, GLuint BeginY);

   void bind() const;

   GLuint getTextureID() const { return textureID; }
   BasicTexture::Kind getKind() const { return textureKind; }
   const sf::Image &getImage() const { return Img; }
   float getTextureWidth() const { return textureWidth; }
   float getTextureHeight() const { return textureHeight; }
};

} // namespace mc

#endif //MINESHAFT_MULTITEXTURE_H
