#include "mineshaft/Config.h"
#include "mineshaft/World/Block.h"

#include <tblgen/Record.h>
#include <tblgen/Value.h>
#include <tblgen/Support/Casting.h>

#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/raw_ostream.h>
#include <SFML/Graphics/Image.hpp>

using namespace tblgen;
using namespace tblgen::support;

namespace {

class BiomeDefinitionEmitter {
   llvm::raw_ostream &OS;
   RecordKeeper &RK;

public:
   BiomeDefinitionEmitter(llvm::raw_ostream &OS, RecordKeeper &RK)
      : OS(OS), RK(RK)
   { }

   void Emit();
};

} // anonymous namespace

void BiomeDefinitionEmitter::Emit()
{
   llvm::SmallVector<Record*, 64> biomes;
   RK.getAllDefinitionsOf("Biome", biomes);

   OS << "#ifndef MC_BIOME\n"
      << "   #define MC_BIOME(NAME)\n"
      << "#endif\n\n";

   for (auto *biome : biomes) {
      OS << "MC_BIOME(" << biome->getName() << ")\n";
   }

   OS << "\n\n#undef MC_BIOME";
}

namespace {

class BiomeFunctionEmitter {
   llvm::raw_ostream &OS;
   RecordKeeper &RK;

   struct Noise {
      enum Kind {
         Cellular, Simplex,
      };

      Noise(Kind kind, float frequency, float lacunarity, float gain)
         : kind(kind), frequency(frequency), lacunarity(lacunarity), gain(gain)
      { }

      Kind kind;
      float frequency;
      float lacunarity;
      float gain;
   };

   static llvm::StringRef noiseKindToString(Noise::Kind kind)
   {
      switch (kind) {
      case Noise::Cellular: return "CellularFractal";
      case Noise::Simplex: return "SimplexFractal";
      }
   }

   struct Biome {
      std::vector<std::pair<Noise, float>> noise;
      int treeFrequency = 0;
      int frequency = 0;
   };

   llvm::DenseMap<Record*, Biome> biomes;
   int frequencyTotal = 0;

   void emitGetNoise();
   void emitGetTreeNoise();

public:
   BiomeFunctionEmitter(llvm::raw_ostream &OS, RecordKeeper &RK)
      : OS(OS), RK(RK)
   { }

   void Emit();
};

} // anonymous namespace

void BiomeFunctionEmitter::Emit()
{
   llvm::SmallVector<Record*, 64> biomes;
   RK.getAllDefinitionsOf("Biome", biomes);

   for (auto *biome : biomes) {
      auto &b = this->biomes[biome];
      auto *noises = cast<ListLiteral>(biome->getFieldValue("noise"));
      for (auto *noise : noises->getValues()) {
         auto *weightedNoiseRec = cast<RecordVal>(noise)->getRecord();

         auto *noiseVal = cast<RecordVal>(weightedNoiseRec->getFieldValue("noise"))->getRecord();
         auto *weight = cast<FPLiteral>(weightedNoiseRec->getFieldValue("weight"));

         auto *frequency = cast<FPLiteral>(noiseVal->getFieldValue("frequency"));
         auto *lacunarity = cast<FPLiteral>(noiseVal->getFieldValue("lacunarity"));
         auto *gain = cast<FPLiteral>(noiseVal->getFieldValue("gain"));

         Noise::Kind k = llvm::StringSwitch<Noise::Kind>(noiseVal->getBases().front().getBase()->getName())
            .Case("Cellular", Noise::Cellular)
            .Case("Simplex", Noise::Simplex)
            .Default(Noise::Cellular);

         b.noise.emplace_back(Noise(k, frequency->getVal().convertToFloat(),
                                    lacunarity->getVal().convertToFloat(),
                                    gain->getVal().convertToFloat()),
                              weight->getVal().convertToFloat());
      }

      b.treeFrequency = (int)cast<IntegerLiteral>(
         biome->getFieldValue("treeFrequency"))->getVal().getZExtValue();
      b.frequency = (int)cast<IntegerLiteral>(
         biome->getFieldValue("frequency"))->getVal().getZExtValue();

      frequencyTotal += b.frequency;
   }

   emitGetNoise();
   emitGetTreeNoise();
}

void BiomeFunctionEmitter::emitGetNoise()
{
   OS << "float DefaultTerrainGenerator::getNoise(Biome b, int x, int z)\n{\n";
   OS << "   switch (b) {\n";

   for (auto &pair : biomes) {
      auto &biome = pair.getSecond();
      OS << "   case Biome::" << pair.getFirst()->getName() << ": {\n"
         << "      float height = 0.f;\n";

      for (auto &noise : biome.noise) {
         OS << R"__(
      {
         noiseGenerator.SetFrequency()__" << noise.first.frequency << R"__();
         noiseGenerator.SetFractalLacunarity()__" << noise.first.lacunarity << R"__();
         noiseGenerator.SetFractalGain()__" << noise.first.gain << R"__();

         height += )__" << noise.second << R"__( * noiseGenerator.Get)__"
           << noiseKindToString(noise.first.kind) << R"__((x, z);
      }
)__";
      }

      OS << "   return height;\n   }\n";
   }

   OS << "   }\n";
   OS << "}\n\n";
}

void BiomeFunctionEmitter::emitGetTreeNoise()
{
   OS << "int DefaultTerrainGenerator::getTreeNoiseFrequency(Biome b)\n{\n";
   OS << "   switch (b) {\n";

   for (auto &pair : biomes) {
      auto &biome = pair.getSecond();
      OS << "   case Biome::" << pair.getFirst()->getName() << ": return "
         << biome.treeFrequency << ";\n";
   }

   OS << "   }\n";
   OS << "}\n\n";
}

extern "C" {

void EmitBiomeDefinitions(llvm::raw_ostream &OS, RecordKeeper &RK)
{
   BiomeDefinitionEmitter(OS, RK).Emit();
}

void EmitBiomeFunctions(llvm::raw_ostream &OS, RecordKeeper &RK)
{
   BiomeFunctionEmitter(OS, RK).Emit();
}

} // extern "C"