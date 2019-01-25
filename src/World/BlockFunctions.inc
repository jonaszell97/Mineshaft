bool Block::isTransparent(BlockID blockID)
{
   switch (blockID) {
   case BlockID::Air: return true;
   case BlockID::Dirt: return false;
   case BlockID::Grass: return false;
   case BlockID::Stone: return false;
   case BlockID::Bedrock: return false;
   case BlockID::MoonStone: return false;
   }
}

Block Block::createAir(Context &C, glm::vec3 position,
                          glm::vec3 direction) {
   Model *m;
   if (C.isModelLoaded("Air")) {
      m = C.getOrLoadModel("Air");
   }
   else {
      Mesh mesh = Mesh::createCube(nullptr);
      m = C.internModel("Air", Model(mesh));
   }

   return Block(BlockID::Air, m, position, direction);
}

Block Block::createDirt(Context &C, glm::vec3 position,
                          glm::vec3 direction) {
   Model *m;
   if (C.isModelLoaded("Dirt")) {
      m = C.getOrLoadModel("Dirt");
   }
   else {
      Mesh mesh = Mesh::createCube(nullptr);
      m = C.internModel("Dirt", Model(mesh));
   }

   return Block(BlockID::Dirt, m, position, direction);
}

Block Block::createGrass(Context &C, glm::vec3 position,
                          glm::vec3 direction) {
   Model *m;
   if (C.isModelLoaded("Grass")) {
      m = C.getOrLoadModel("Grass");
   }
   else {
      Mesh mesh = Mesh::createCube(nullptr);
      m = C.internModel("Grass", Model(mesh));
   }

   return Block(BlockID::Grass, m, position, direction);
}

Block Block::createStone(Context &C, glm::vec3 position,
                          glm::vec3 direction) {
   Model *m;
   if (C.isModelLoaded("Stone")) {
      m = C.getOrLoadModel("Stone");
   }
   else {
      Mesh mesh = Mesh::createCube(nullptr);
      m = C.internModel("Stone", Model(mesh));
   }

   return Block(BlockID::Stone, m, position, direction);
}

Block Block::createBedrock(Context &C, glm::vec3 position,
                          glm::vec3 direction) {
   Model *m;
   if (C.isModelLoaded("Bedrock")) {
      m = C.getOrLoadModel("Bedrock");
   }
   else {
      Mesh mesh = Mesh::createCube(nullptr);
      m = C.internModel("Bedrock", Model(mesh));
   }

   return Block(BlockID::Bedrock, m, position, direction);
}

Block Block::createMoonStone(Context &C, glm::vec3 position,
                          glm::vec3 direction) {
   Model *m;
   if (C.isModelLoaded("MoonStone")) {
      m = C.getOrLoadModel("MoonStone");
   }
   else {
      Mesh mesh = Mesh::createCube(nullptr);
      m = C.internModel("MoonStone", Model(mesh));
   }

   return Block(BlockID::MoonStone, m, position, direction);
}
bool Block::usesCubeMap(BlockID blockID)
{
   switch (blockID) {
   case BlockID::Air: return false;
   case BlockID::Dirt: return false;
   case BlockID::Grass: return true;
   case BlockID::Stone: return false;
   case BlockID::Bedrock: return false;
   case BlockID::MoonStone: return false;
   }
}
unsigned Block::getTextureLayer(BlockID blockID)
{
   switch (blockID) {
   case BlockID::Air: return 0;
   case BlockID::Dirt: return 1;
   case BlockID::Grass: return 0;
   case BlockID::Stone: return 2;
   case BlockID::Bedrock: return 3;
   case BlockID::MoonStone: return 4;
   }
}

TextureArray Block::loadBlockTextures()
{
   TextureArray texArray = TextureArray::create(BasicTexture::DIFFUSE,
                                               16,
                                               16,
                                               5);

   llvm::SmallString<64> fileName;
   fileName += "../assets/textures/";

   size_t initialSize = fileName.size();

   {
      fileName += "air.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, 0);
      }

      fileName.resize(initialSize);
   }

   {
      fileName += "dirt.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, 1);
      }

      fileName.resize(initialSize);
   }

   {
      fileName += "stone.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, 2);
      }

      fileName.resize(initialSize);
   }

   {
      fileName += "bedrock.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, 3);
      }

      fileName.resize(initialSize);
   }

   {
      fileName += "moon_stone.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, 4);
      }

      fileName.resize(initialSize);
   }

   texArray.finalize();
   return texArray;
}


TextureArray Block::loadBlockCubeMapTextures()
{
   TextureArray texArray = TextureArray::createCubemap(BasicTexture::DIFFUSE,
                                                        16,
                                                        16,
                                                        5);

   llvm::SmallString<64> fileName;
   fileName += "../assets/textures/";

   size_t initialSize = fileName.size();

   {
      fileName += "grass_side.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, 0, 0);
      }

      fileName.resize(initialSize);
   }

   {
      fileName += "grass_side.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, 0, 1);
      }

      fileName.resize(initialSize);
   }

   {
      fileName += "grass_top.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, 0, 2);
      }

      fileName.resize(initialSize);
   }

   {
      fileName += "dirt.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, 0, 3);
      }

      fileName.resize(initialSize);
   }

   {
      fileName += "grass_side.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, 0, 4);
      }

      fileName.resize(initialSize);
   }

   {
      fileName += "grass_side.png";

      sf::Image Img;
      if (Img.loadFromFile(fileName.str())) {
         texArray.addTexture(Img, 0, 5);
      }

      fileName.resize(initialSize);
   }

   texArray.finalize();
   return texArray;
}

