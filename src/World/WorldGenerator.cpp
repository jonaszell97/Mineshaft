#include "mineshaft/World/WorldGenerator.h"

#include <SFML/Graphics/Image.hpp>

using namespace mc;

WorldGenerator::WorldGenerator(World *world, WorldGenOptions options)
   : world(world), options(options)
{
   world->setWorldGenerator(this);
}

DefaultTerrainGenerator::DefaultTerrainGenerator(World *world,
                                                 WorldGenOptions &options)
   : WorldGenerator(world, options), noiseGenerator(options.seed),
     rng((unsigned)options.seed)
{
   visualizeNoise([&](int x, int z) {
      return getBiomeNoise(ChunkPosition(x, z));
   }, "biome_noise", 512, 512);
//
//   visualizeNoise([&](int x, int z) {
//      return getTreeNoise(Biome::Forest, x, z);
//   }, "forest_tree_noise", 512, 512);
//
//   visualizeNoise([&](int x, int z) {
//      return getTreeNoise(Biome::Plains, x, z);
//   }, "plains_tree_noise", 512, 512);
}

float DefaultTerrainGenerator::getNoise(int x, int z, float frequency,
                                        float lacunarity,
                                        float gain,
                                        FastNoise::FractalType type) {
   noiseGenerator.SetFrequency(frequency);
   noiseGenerator.SetFractalLacunarity(lacunarity);
   noiseGenerator.SetFractalGain(gain);
   noiseGenerator.SetFractalType(type);

   return noiseGenerator.GetSimplexFractal(x, z);
}

float DefaultTerrainGenerator::getNoise(Biome b, int x, int z)
{
   switch (b) {
   case Biome::Plains:
   case Biome::Forest: {
      // Generate high hills with a low frequency.
      float mountainNoise = getNoise(x, z, 0.01f, 1.0f);

      // Generate lower hills with a low frequency.
      float terrainNoise = getNoise(x, z, 0.03f, 2.0f);

      // Combine the two noise levels.
      float rawNoise = 0.2f * mountainNoise + 0.8f * terrainNoise;

      // Exponentiate noise to flatten valleys and mountains.
      return pow(rawNoise, 5.0f);
   }
   case Biome::Mountains: {
      return getNoise(x, z);
   }
   default:
      return 0.0f;
   }
}

float DefaultTerrainGenerator::getTreeNoise(Biome b, int x, int z)
{
   int R;
   switch (b) {
   case Biome::Plains:
   case Biome::Mountains: {
      R = 3;
      break;
   }
   case Biome::Forest:
      R = 1;
      break;
   default:
      R = 3;
      break;
   }

   // Generate peaks with a medium frequency.
   float peakNoise = getNoise(x, z, 0.03f, 3.0f);

   bool shouldGenerate = true;
   for (int xn = x - R; xn <= x + R; ++xn) {
      for (int zn = z - R; zn <= z + R; ++zn) {
         if (xn == x && zn == z) {
            continue;
         }

         float neighborNoise = getNoise(xn, zn, 0.03f, 3.0f);
         if (neighborNoise >= peakNoise) {
            shouldGenerate = false;
            break;
         }
      }
   }

   // Return either -1 or 1, depending on a threshold.
   return shouldGenerate ? -1.0f : 1.0f;
}

float DefaultTerrainGenerator::getBiomeNoise(ChunkPosition chunkPos)
{
   noiseGenerator.SetCellularDistanceFunction(FastNoise::Natural);
   noiseGenerator.SetFrequency(0.05f);
   noiseGenerator.SetCellularReturnType(FastNoise::CellValue);
   noiseGenerator.SetCellularJitter(0.8f);

   return noiseGenerator.GetCellular(chunkPos.x, chunkPos.z);
}

Biome DefaultTerrainGenerator::getBiome(const Chunk &chunk)
{
   float noise = getBiomeNoise(chunk.getChunkPosition());
   noise += 1.0f;

   if (noise >= 1.2f) {
      return Biome::Plains;
   }

   if (noise >= 0.5f) {
      return Biome::Forest;
   }

   return Biome::Mountains;
}

void DefaultTerrainGenerator::generateTerrain(Chunk &chunk)
{
   auto &app = world->getApplication();

   static auto grass = Block::createGrass(app, glm::vec3(0.0f));
   static auto dirt = Block::createDirt(app, glm::vec3(0.0f));
   static auto stone = Block::createStone(app, glm::vec3(0.0f));
   static auto water = Block::createWater(app, glm::vec3(0.0f));

   Biome biome = getBiome(chunk);
   chunk.setBiome(biome);

   for (unsigned x = 0; x < MC_CHUNK_WIDTH; ++x) {
      for (unsigned z = 0; z < MC_CHUNK_DEPTH; ++z) {
         BlockPositionChunk pos(x, 0, z);
         WorldPosition worldPos = chunk.getWorldPosition(pos);

         float noise = getNoise(biome, worldPos.x, worldPos.z);
         int height = (int)((noise * MC_CHUNK_HEIGHT / 2.0f));

         // Fill with water
         if (height < options.seaY) {
            worldPos.y = height;
            while (worldPos.y <= options.seaY) {
               chunk.updateBlock(worldPos, Block(water, getScenePosition(worldPos)),
                                 false);

               ++worldPos.y;
            }
         }
         else {
            worldPos.y = height;
            chunk.updateBlock(worldPos,
                              Block(grass, getScenePosition(worldPos)),
                              false);

            // Generate trees.
            if (getTreeNoise(biome, worldPos.x, worldPos.z) == -1.0f) {
               generateTree(chunk, worldPos);
            }
         }

         // Create dirt for the blocks below.
         int y = height - 1;
         for (; y >= height - 1 - options.dirtLayers; --y) {
            worldPos.y = y;
            chunk.updateBlock(worldPos, Block(dirt, getScenePosition(worldPos)),
                              false);
         }

         // Create dirt for the blocks below.
         for (; y > -(MC_CHUNK_HEIGHT / 2); --y) {
            worldPos.y = y;
            chunk.updateBlock(worldPos, Block(stone, getScenePosition(worldPos)),
                              false);
         }
      }
   }
}

void DefaultTerrainGenerator::generateTree(Chunk &chunk, const WorldPosition &pos)
{
   unsigned rd = rng();
   int height = 3 + (rd % 3);

   static Block trunk = Block::createOakWood(world->getApplication(), glm::vec3(0.0f));
   static Block leaf = Block::createLeaf(world->getApplication(), glm::vec3(0.0f));

   WorldPosition worldPos = pos;
   for (int y = pos.y + 1; y <= pos.y + height; ++y) {
      worldPos.y = y;
      chunk.updateBlock(worldPos, Block(trunk, getScenePosition(worldPos)),
                        false);
   }

   // Place leaves around top block.
   for (int x = pos.x - 1; x <= pos.x + 1; ++x) {
      worldPos.x = x;

      for (int z = pos.z - 1; z <= pos.z + 1; ++z) {
         worldPos.z = z;

         for (int y = pos.y + height - 1; y <= pos.y + height + 1; ++y) {
            if (x == pos.x && z == pos.z && y <= pos.y + height) {
               continue;
            }

            worldPos.y = y;
            world->updateBlock(worldPos,
                               Block(leaf, getScenePosition(worldPos)));
         }
      }
   }

   // Place some amount of random leaves.
   for (int x = pos.x - 2; x <= pos.x + 2; x += 1) {
      for (int z = pos.z - 2; z <= pos.z + 2; z += 1) {
         if ((x >= pos.x - 1 && x <= pos.x + 1)
         && (z >= pos.z - 1 && z <= pos.z + 1)) {
            continue;
         }

         WorldPosition leafPos(x, pos.y + height - 1, z);
         world->updateBlock(leafPos,
                            Block(leaf, getScenePosition(leafPos)));

         if (rng() < (UINT_MAX / 3)) {
            ++leafPos.y;
            world->updateBlock(leafPos,
                               Block(leaf, getScenePosition(leafPos)));
         }
      }
   }
}

void DefaultTerrainGenerator::visualizeNoise(const llvm::function_ref<float(int, int)> &noiseGen,
                                             llvm::StringRef name,
                                             int width, int depth) {
   sf::Image Img;
   Img.create(width, depth);

   for (int x = 0; x < width; ++x) {
      for (int z = 0; z < depth; ++z) {
         float noise = noiseGen(x, z);

         float normalized = (noise + 1.0f) / 2.0f;
         uint8_t color = (uint8_t)(normalized * 255);

         Img.setPixel(
            x, z, sf::Color(color, color, color));
      }
   }

   Img.saveToFile(std::string("/Users/Jonas/Downloads/") + name.str() + ".png");
}