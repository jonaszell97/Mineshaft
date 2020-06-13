#ifndef MINESHAFT_WORLDGENERATOR_H
#define MINESHAFT_WORLDGENERATOR_H

#include "mineshaft/Support/Noise/SimplexNoise.h"
#include "mineshaft/World/World.h"

#include <random>
#include <llvm/ADT/STLExtras.h>

namespace mc {

enum class Biome : uint8_t {
   Undefined = 0,
   Plains,
   Forest,
   Mountains,
};

class WorldGenerator {
protected:
   explicit WorldGenerator(World *world, WorldGenOptions options);

   /// The world we are generating terrain for.
   World *world;

   /// The world generation options.
   WorldGenOptions options;

public:
   /// Virtual destructor to guarantee vtable generation.
   virtual ~WorldGenerator() = default;

   /// Generate the terrain for a chunk according to the generation strategy.
   virtual void generateTerrain(Chunk &chunk) = 0;
};

class DefaultTerrainGenerator: public WorldGenerator {
private:
   /// The noise generator.
   FastNoise noiseGenerator;

   /// Random number generator.
   std::mt19937 rng;

   /// Generate noise with the specified parameters.
   float getNoise(int x, int z,
                  float frequency = 0.01f,
                  float lacunarity = 2.0f,
                  float gain = 0.5f,
                  FastNoise::FractalType type = FastNoise::FBM);

   /// Configure the noise generator based on the biome.
   float getNoise(Biome b, int x, int z);

   /// Configure the noise generator based on the biome.
   float getTreeNoise(Biome b, int x, int z);
   int getTreeNoiseFrequency(Biome b);

   /// Configure the noise generator based on the biome.
   float getBiomeNoise(ChunkPosition chunkPos);

   /// Configure the noise generator based on the biome.
   Biome getBiome(const Chunk &chunk);

   /// Visualize the generated noise.
   void visualizeNoise(const llvm::function_ref<float(int, int)> &noiseGen,
                       llvm::StringRef name = "noise",
                       int width = 128, int depth = 128);

public:
   DefaultTerrainGenerator(World *world, WorldGenOptions &options);

   /// \inherit
   void generateTerrain(Chunk &chunk) override;

   /// Generate a tree at the specified position.
   void generateTree(Chunk &chunk, const WorldPosition &pos);
};

} // namespace mc

#endif //MINESHAFT_WORLDGENERATOR_H
