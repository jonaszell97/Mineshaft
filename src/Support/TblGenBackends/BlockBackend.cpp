//
// Created by Jonas Zell on 2019-01-22.
//

#include "mineshaft/Config.h"

#include <tblgen/Record.h>
#include <tblgen/Value.h>
#include <tblgen/Support/Casting.h>

#include <llvm/Support/raw_ostream.h>

using namespace tblgen;
using namespace tblgen::support;

namespace {

class BlockDefinitionEmitter {
   llvm::raw_ostream &OS;
   RecordKeeper &RK;

public:
   BlockDefinitionEmitter(llvm::raw_ostream &OS, RecordKeeper &RK)
      : OS(OS), RK(RK)
   { }

   void Emit();
};

} // anonymous namespace

void BlockDefinitionEmitter::Emit()
{
   llvm::SmallVector<Record*, 64> blocks;
   RK.getAllDefinitionsOf("Block", blocks);

   OS << "#ifndef MC_BLOCK\n"
      << "   #define MC_BLOCK(NAME)\n"
      << "#endif\n\n";

   for (auto *block : blocks) {
      OS << "MC_BLOCK(" << block->getName() << ")\n";
   }

   OS << "\n\n#undef MC_BLOCK";
}

namespace {

class BlockFunctionEmitter {
   llvm::raw_ostream &OS;
   RecordKeeper &RK;

   unsigned numTextures = 0;
   unsigned numCubemapTextures = 0;

   llvm::StringMap<unsigned> textureIDs;
   llvm::StringMap<unsigned> cubemapTextureIDs;
   llvm::DenseMap<Record*, bool> usesCubeMap;
   llvm::DenseMap<Record*, std::string> textureNames;

   llvm::StringRef getTextureName(Record *block);

   void emitIsTransparent(llvm::ArrayRef<Record*> blocks);
   void emitUseCubeMap(llvm::ArrayRef<Record*> blocks);
   void emitGetTextureLayer(llvm::ArrayRef<Record *> blocks);
   void emitLoadTextures(llvm::ArrayRef<Record *> blocks);
   void emitCreate(Record *block);

public:
   BlockFunctionEmitter(llvm::raw_ostream &OS, RecordKeeper &RK)
      : OS(OS), RK(RK)
   { }

   void Emit();
};

} // anonymous namespace

void BlockFunctionEmitter::Emit()
{
   llvm::SmallVector<Record*, 64> blocks;
   RK.getAllDefinitionsOf("Block", blocks);

   emitIsTransparent(blocks);

   for (auto *block : blocks) {
      emitCreate(block);
   }

   emitUseCubeMap(blocks);
   emitGetTextureLayer(blocks);
   emitLoadTextures(blocks);
}

llvm::StringRef BlockFunctionEmitter::getTextureName(Record *block)
{
   auto &textureName = textureNames[block];
   if (!textureName.empty()) {
      return textureName;
   }

   textureName = block->getName();
   textureName.front() = std::tolower(textureName.front());

   for (unsigned i = 0; i < textureName.size(); ++i) {
      if (::isupper(textureName[i])) {
         textureName.insert(textureName.begin() + i, '_');
         textureName[i + 1] = ::tolower(textureName[i + 1]);
         i++;
      }
   }

   return textureName;
}

void BlockFunctionEmitter::emitIsTransparent(llvm::ArrayRef<Record *> blocks)
{
   OS << "bool Block::isTransparent(BlockID blockID)\n{\n";
   OS << "   switch (blockID) {\n";

   for (auto *block : blocks) {
      bool transparent = support::cast<IntegerLiteral>(
         block->getFieldValue("transparent"))->getVal().getBoolValue();

      OS << "   case BlockID::" << block->getName() << ": return "
         << (transparent ? "true" : "false") << ";\n";
   }

   OS << "   }\n}\n";
}

void BlockFunctionEmitter::emitUseCubeMap(llvm::ArrayRef<Record *> blocks)
{
   OS << "bool Block::usesCubeMap(BlockID blockID)\n{\n";
   OS << "   switch (blockID) {\n";

   for (auto *block : blocks) {
      OS << "   case BlockID::" << block->getName() << ": return "
         << (usesCubeMap[block] ? "true" : "false") << ";\n";
   }

   OS << "   }\n}\n";
}

void BlockFunctionEmitter::emitGetTextureLayer(llvm::ArrayRef<Record *> blocks)
{
   OS << "unsigned Block::getTextureLayer(BlockID blockID)\n{\n";
   OS << "   switch (blockID) {\n";

   for (auto *block : blocks) {
      OS << "   case BlockID::" << block->getName() << ": return ";

      if (usesCubeMap[block]) {
         OS << "0;\n";
      }
      else {
         unsigned id = textureIDs[getTextureName(block)];
         OS << id << ";\n";
      }
   }

   OS << "   }\n}\n";
}

void BlockFunctionEmitter::emitLoadTextures(llvm::ArrayRef<Record*> blocks)
{
   unsigned height = MC_TEXTURE_HEIGHT;
   unsigned width = MC_TEXTURE_WIDTH;
   unsigned layers = textureIDs.size();

   OS << R"__(
TextureArray Block::loadBlockTextures()
{
   TextureArray texArray = TextureArray::create(BasicTexture::DIFFUSE,
                                               )__" << height << R"__(,
                                               )__" << width << R"__(,
                                               )__" << layers << R"__();

   llvm::SmallString<64> fileName;
   fileName += "../assets/textures/";

   size_t initialSize = fileName.size();
)__";

   for (auto *block : blocks) {
      if (usesCubeMap[block]) {
         continue;
      }

      auto defaultTexture = getTextureName(block);
      unsigned id = textureIDs[defaultTexture];

      OS << R"__(
   {
      fileName += ")__" << defaultTexture << R"__(.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, )__" << id << R"__();
      }

      fileName.resize(initialSize);
   }
)__";
   }

   OS << R"__(
   texArray.finalize();
   return texArray;
}

)__";

   OS << R"__(
TextureArray Block::loadBlockCubeMapTextures()
{
   TextureArray texArray = TextureArray::createCubemap(BasicTexture::DIFFUSE,
                                                        )__" << height << R"__(,
                                                        )__" << width << R"__(,
                                                        )__" << layers << R"__();

   llvm::SmallString<64> fileName;
   fileName += "../assets/textures/";

   size_t initialSize = fileName.size();
)__";

   for (auto *block : blocks) {
      if (!usesCubeMap[block]) {
         continue;
      }

      auto *givenTextures = cast<ListLiteral>(block->getFieldValue("textures"));
      auto defaultTexture = getTextureName(block);

      unsigned id = cubemapTextureIDs[defaultTexture];

      unsigned i = 0;
      for (Value *t : givenTextures->getValues()) {
         llvm::StringRef tex = cast<StringLiteral>(t)->getVal();

         OS << R"__(
   {
      fileName += ")__" << tex << R"__(.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, )__" << id << R"__(, )__" << i++ << R"__();
      }

      fileName.resize(initialSize);
   }
)__";
      }
   }

   OS << R"__(
   texArray.finalize();
   return texArray;
}

)__";
}

void BlockFunctionEmitter::emitCreate(Record *block)
{
   llvm::StringRef name = block->getName();

   auto *givenTextures = cast<ListLiteral>(block->getFieldValue("textures"));
   if (givenTextures->getValues().empty()) {
      auto defaultTexture = getTextureName(block);
      auto It = textureIDs.find(defaultTexture);

      unsigned id;
      if (It != textureIDs.end()) {
         id = It->getValue();
      }
      else {
         id = numTextures++;
         textureIDs[defaultTexture] = id;
      }

      usesCubeMap[block] = false;
   }
   else {
      auto defaultTexture = getTextureName(block);
      auto It = cubemapTextureIDs.find(defaultTexture);

      unsigned id;
      if (It != cubemapTextureIDs.end()) {
         id = It->getValue();
      }
      else {
         id = numCubemapTextures++;
         cubemapTextureIDs[defaultTexture] = id;
      }

      usesCubeMap[block] = true;
   }

   OS << R"__(
Block Block::create)__" << name << R"__((Context &C, glm::vec3 position,
                          glm::vec3 direction) {
   Model *m;
   if (C.isModelLoaded(")__" << name << R"__(")) {
      m = C.getOrLoadModel(")__" << name << R"__(");
   }
   else {
      Mesh mesh = Mesh::createCube(nullptr);
      m = C.internModel(")__" << name << R"__(", Model(mesh));
   }

   return Block(BlockID::)__" << name << R"__(, m, position, direction);
}
)__";
}

extern "C" {

void EmitBlockDefinitions(llvm::raw_ostream &OS, RecordKeeper &RK)
{
   BlockDefinitionEmitter(OS, RK).Emit();
}

void EmitBlockFunctions(llvm::raw_ostream &OS, RecordKeeper &RK)
{
   BlockFunctionEmitter(OS, RK).Emit();
}

} // extern "C"