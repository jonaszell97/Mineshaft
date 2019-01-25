//
// Created by Jonas Zell on 2019-01-17.
//

#include "mineshaft/Texture/BasicTexture.h"

using namespace mc;

BasicTexture::BasicTexture()
   : textureID(0), TextureKind(DIFFUSE), glTextureKind(GL_TEXTURE_2D)
{

}

BasicTexture::~BasicTexture()
{
   glDeleteTextures(1, &textureID);
}

void BasicTexture::Profile(llvm::FoldingSetNodeID &ID)
{
   Profile(ID, TextureKind, File);
}

void BasicTexture::Profile(llvm::FoldingSetNodeID &ID,
                           BasicTexture::Kind K,
                           llvm::StringRef File) {
   ID.AddInteger(K);
   ID.AddString(File);
}

BasicTexture::BasicTexture(Kind K, GLuint textureID, std::string &&File,
                           unsigned glTextureKind)
   : textureID(textureID), TextureKind(K), File(std::move(File)),
     glTextureKind(glTextureKind)
{

}

void BasicTexture::bindTexture() const
{
   glBindTexture(GL_TEXTURE_2D, textureID);
}