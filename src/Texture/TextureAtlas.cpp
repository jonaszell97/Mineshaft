#include "mineshaft/Texture/TextureAtlas.h"
#include "mineshaft/Application.h"

#include <llvm/ADT/SmallString.h>

using namespace mc;

llvm::Optional<TextureAtlas>
TextureAtlas::fromFile(BasicTexture::Kind textureKind,
                       llvm::StringRef FileName,
                       float textureWidth,
                       float textureHeight) {
   llvm::SmallString<64> fileName;
   fileName += "../assets/textures/";
   fileName += FileName;

   std::string str = fileName.str();

   sf::Image Img;
   if (!Img.loadFromFile(str)) {
      return llvm::None;
   }

   GLuint textureID;
   glGenTextures(1, &textureID);
   glBindTexture(GL_TEXTURE_2D, textureID);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Img.getSize().x, Img.getSize().y,
                0, GL_RGBA, GL_UNSIGNED_BYTE, Img.getPixelsPtr());

   glGenerateMipmap(GL_TEXTURE_2D);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   return TextureAtlas(textureKind, textureID, std::move(str), std::move(Img),
                       textureWidth, textureHeight);
}

TextureAtlas::TextureAtlas()
   : Img(), fileName(),
     textureID(0), textureKind(BasicTexture::DIFFUSE)
{

}

TextureAtlas::TextureAtlas(BasicTexture::Kind textureKind,
                           GLuint textureID,
                           std::string &&FileName, sf::Image &&Img,
                           float textureWidth,
                           float textureHeight)
   : Img(std::move(Img)), fileName(std::move(FileName)),
     textureID(textureID), textureKind(textureKind)
{
   auto size = this->Img.getSize();

   float numBlocksHorizontal = size.x / textureWidth;
   float numBlocksVertical = size.y / textureHeight;

   this->textureWidth = 1.0f / numBlocksHorizontal;
   this->textureHeight = 1.0f / numBlocksVertical;
}

sf::Image TextureAtlas::getTextureImg(GLuint BeginX, GLuint BeginY,
                                      GLuint EndX, GLuint EndY) const {
   if (EndX == -1) {
      EndX = BeginX + 1;
   }
   if (EndY == -1) {
      EndY = BeginY + 1;
   }

   BeginX *= MC_TEXTURE_WIDTH;
   BeginY *= MC_TEXTURE_HEIGHT;

   EndX *= MC_TEXTURE_WIDTH;
   EndY *= MC_TEXTURE_HEIGHT;

   // We need coordinates in column-major order.
   std::swap(BeginX, BeginY);
   std::swap(EndX, EndY);

   if (BeginX >= EndX || EndX > Img.getSize().x) {
      llvm_unreachable("invalid texture indices");
   }
   if (BeginY >= EndY || EndY > Img.getSize().y) {
      llvm_unreachable("invalid texture indices");
   }

   sf::Image SubImg;
   SubImg.create(EndX - BeginX, EndY - BeginY);
   SubImg.copy(Img, 0, 0,
               sf::IntRect(BeginX, BeginY, EndX - BeginX, EndY -BeginY), false);

   return SubImg;
}

glm::vec2 TextureAtlas::getTextureUVCoords(GLuint BeginX, GLuint BeginY)
{
   return glm::vec2((BeginX * MC_TEXTURE_WIDTH) / Img.getSize().x,
                    (BeginY * MC_TEXTURE_HEIGHT) / Img.getSize().y);
}

void TextureAtlas::bind() const
{
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, textureID);
}

BasicTexture *TextureAtlas::getTexture(Application &Ctx,
                                       BasicTexture::Kind kind,
                                       GLuint BeginX, GLuint BeginY,
                                       GLuint EndX, GLuint EndY) const {
   auto SubImg = getTextureImg(BeginX, BeginY, EndX, EndY);
   return Ctx.loadTexture(kind, SubImg, fileName);
}