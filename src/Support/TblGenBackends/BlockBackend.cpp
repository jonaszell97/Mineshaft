#include "mineshaft/Config.h"
#include "mineshaft/World/Block.h"

#include <tblgen/Record.h>
#include <tblgen/Value.h>
#include <tblgen/Support/Casting.h>

#include <llvm/Support/raw_ostream.h>
#include <SFML/Graphics/Image.hpp>
#include <llvm/ADT/SmallString.h>

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
   llvm::StringMap<unsigned> textureIDs;

   llvm::DenseMap<Record*, bool> usesCubeMap;
   llvm::DenseMap<Record*, std::string> textureNames;

   float atlasWidth = 0.0f;
   float atlasHeight = 0.0f;

   llvm::StringRef getTextureName(Record *block);
   glm::vec2 getTextureUV(Record *block, unsigned face);

   void emitIsTransparent(llvm::ArrayRef<Record*> blocks);
   void emitIsSolid(llvm::ArrayRef<Record*> blocks);
   void emitUseCubeMap(llvm::ArrayRef<Record*> blocks);
   void emitGetTextureUV(llvm::ArrayRef<Record *> blocks);
   void emitCreate(Record *block);

   void emitTextureAtlas();

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
   emitIsSolid(blocks);

   for (auto *block : blocks) {
      emitCreate(block);
   }

   emitUseCubeMap(blocks);
   emitTextureAtlas();
   emitGetTextureUV(blocks);
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

glm::vec2 BlockFunctionEmitter::getTextureUV(Record *block, unsigned face)
{
   llvm::StringRef textureName;
   if (usesCubeMap[block]) {
      auto *givenTextures = cast<ListLiteral>(block->getFieldValue("textures"));
      textureName = cast<StringLiteral>(givenTextures->getValues()[face])->getVal();
   }
   else {
      textureName = getTextureName(block);
   }

   float x = 0;
   float z = 0;

   for (auto &textureIDPair : textureIDs) {
      if (textureIDPair.getKey() == textureName) {
         break;
      }

      x += 16.f;
      if (x == (8.f * 16.f)) {
         z += 16.f;
         x = 0.f;
      }
   }

   return glm::vec2(x / atlasWidth, z / atlasHeight);
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

void BlockFunctionEmitter::emitIsSolid(llvm::ArrayRef<Record *> blocks)
{
   OS << "bool Block::isSolid(BlockID blockID)\n{\n";
   OS << "   switch (blockID) {\n";

   for (auto *block : blocks) {
      bool transparent = support::cast<IntegerLiteral>(
         block->getFieldValue("solid"))->getVal().getBoolValue();

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

void BlockFunctionEmitter::emitGetTextureUV(llvm::ArrayRef<Record*> blocks)
{
   OS << "glm::vec2 Block::getTextureUV(BlockID blockID, FaceMask face)\n{\n";
   OS << "   switch (blockID) {\n";

   for (auto *block : blocks) {
      OS << "   case BlockID::" << block->getName() << ": ";

      if (usesCubeMap[block]) {
         OS << "      switch (face) {\n";

         static const char *names[] = {
            "F_Right", "F_Left",
            "F_Top", "F_Bottom",
            "F_Front", "F_Back",
         };

         for (unsigned i = 0; i < 6; ++i) {
            OS << "      case " << names[i] << ": return ";

            auto UV = getTextureUV(block, i);
            OS << "glm::vec2(" << UV.x << ", " << UV.y << ");\n";
         }

         OS << "      default: llvm_unreachable(\"not a valid cube face\");\n";
         OS << "      }\n";
      }
      else {
         auto UV = getTextureUV(block, 0);
         OS << "return glm::vec2(" << UV.x << ", " << UV.y << ");\n";
      }
   }

   OS << "   }\n}\n";
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
      for (auto *tex : givenTextures->getValues()) {
         auto defaultTexture = cast<StringLiteral>(tex)->getVal();
         auto It = textureIDs.find(defaultTexture);

         unsigned id;
         if (It != textureIDs.end()) {
            id = It->getValue();
         }
         else {
            id = numTextures++;
            textureIDs[defaultTexture] = id;
         }
      }

      usesCubeMap[block] = true;
   }

   OS << R"__(
Block Block::create)__" << name << R"__((Application &app, glm::vec3 position,
                          glm::vec3 direction) {
   return Block(BlockID::)__" << name << R"__(, position, direction);
}
)__";
}

void BlockFunctionEmitter::emitTextureAtlas()
{
   llvm::SmallString<128> textureName;
   textureName += "/Users/Jonas/mineshaft/assets/textures/";

   atlasWidth = 8.f * 16.f;
   atlasHeight = (unsigned)(std::ceil((float)textureIDs.size() / 8.0f)) * 16;

   sf::Image joined;
   joined.create(atlasWidth, atlasHeight);

   unsigned x = 0;
   unsigned z = 0;

   size_t initialSize = textureName.size();
   for (auto &textureIDPair : textureIDs) {
      textureName += textureIDPair.getKey();
      textureName += ".png";

      sf::Image img;
      if (img.loadFromFile(textureName.str())) {
         joined.copy(img, x, z, sf::IntRect(0, 0, 16, 16), false);
      }

      x += 16;
      if (x == (8 * 16)) {
         z += 16;
         x = 0;
      }

      textureName.resize(initialSize);
   }

   textureName += "blocks.png";
   joined.saveToFile(textureName.str());
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