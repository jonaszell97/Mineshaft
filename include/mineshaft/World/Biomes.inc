float DefaultTerrainGenerator::getNoise(Biome b, int x, int z)
{
   switch (b) {
   case Biome::Forest: {
      float height = 0.f;

      {
         noiseGenerator.SetFrequency(8.912896e+04);
         noiseGenerator.SetFractalLacunarity(0.000000e+00);
         noiseGenerator.SetFractalGain(0.000000e+00);

         height += -1.588187e-23 * noiseGenerator.GetSimplexFractal(x, z);
      }

      {
         noiseGenerator.SetFrequency(-3.218644e+26);
         noiseGenerator.SetFractalLacunarity(0.000000e+00);
         noiseGenerator.SetFractalGain(0.000000e+00);

         height += -1.588187e-23 * noiseGenerator.GetSimplexFractal(x, z);
      }
   return height;
   }
   case Biome::Plains: {
      float height = 0.f;

      {
         noiseGenerator.SetFrequency(8.912896e+04);
         noiseGenerator.SetFractalLacunarity(0.000000e+00);
         noiseGenerator.SetFractalGain(0.000000e+00);

         height += -1.588187e-23 * noiseGenerator.GetSimplexFractal(x, z);
      }

      {
         noiseGenerator.SetFrequency(-3.218644e+26);
         noiseGenerator.SetFractalLacunarity(0.000000e+00);
         noiseGenerator.SetFractalGain(0.000000e+00);

         height += -1.588187e-23 * noiseGenerator.GetSimplexFractal(x, z);
      }
   return height;
   }
   case Biome::Mountains: {
      float height = 0.f;

      {
         noiseGenerator.SetFrequency(8.912896e+04);
         noiseGenerator.SetFractalLacunarity(0.000000e+00);
         noiseGenerator.SetFractalGain(0.000000e+00);

         height += 0.000000e+00 * noiseGenerator.GetSimplexFractal(x, z);
      }
   return height;
   }
   }
}

int DefaultTerrainGenerator::getTreeNoiseFrequency(Biome b)
{
   switch (b) {
   case Biome::Forest: return 1;
   case Biome::Plains: return 3;
   case Biome::Mountains: return 3;
   }
}

