//
// Created by Jonas Zell on 2019-01-17.
//

#include "mineshaft/Texture/TextureAtlas.h"
#include "mineshaft/Context.h"

#include <llvm/ADT/SmallString.h>

using namespace mc;

llvm::Optional<TextureAtlas>
TextureAtlas::fromFile(llvm::StringRef FileName)
{
   llvm::SmallString<64> fileName;
   fileName += "../assets/textures/";
   fileName += FileName;

   std::string str = fileName.str();

   sf::Image Img;
   if (!Img.loadFromFile(str)) {
      return llvm::None;
   }

   return TextureAtlas(std::move(str), std::move(Img));
}

TextureAtlas::TextureAtlas(std::string &&FileName, sf::Image &&Img)
   : Img(std::move(Img)), fileName(std::move(FileName))
{

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

BasicTexture *TextureAtlas::getTexture(Context &Ctx,
                                       BasicTexture::Kind kind,
                                       GLuint BeginX, GLuint BeginY,
                                       GLuint EndX, GLuint EndY) const {
   auto SubImg = getTextureImg(BeginX, BeginY, EndX, EndY);
   return Ctx.loadTexture(kind, SubImg, fileName);
}