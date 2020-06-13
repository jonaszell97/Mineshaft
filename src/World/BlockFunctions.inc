bool Block::isTransparent(BlockID blockID)
{
   switch (blockID) {
   case BlockID::Air: return true;
   case BlockID::Dirt: return false;
   case BlockID::Grass: return false;
   case BlockID::Stone: return false;
   case BlockID::CobbleStone: return false;
   case BlockID::Bedrock: return false;
   case BlockID::MoonStone: return false;
   case BlockID::Sand: return false;
   case BlockID::OakWood: return false;
   case BlockID::Leaf: return true;
   case BlockID::Water: return true;
   }
}
bool Block::isSolid(BlockID blockID)
{
   switch (blockID) {
   case BlockID::Air: return false;
   case BlockID::Dirt: return true;
   case BlockID::Grass: return true;
   case BlockID::Stone: return true;
   case BlockID::CobbleStone: return true;
   case BlockID::Bedrock: return true;
   case BlockID::MoonStone: return true;
   case BlockID::Sand: return true;
   case BlockID::OakWood: return true;
   case BlockID::Leaf: return true;
   case BlockID::Water: return false;
   }
}

Block Block::createAir(Application &app, glm::vec3 position,
                          glm::vec3 direction) {
   return Block(BlockID::Air, position, direction);
}

Block Block::createDirt(Application &app, glm::vec3 position,
                          glm::vec3 direction) {
   return Block(BlockID::Dirt, position, direction);
}

Block Block::createGrass(Application &app, glm::vec3 position,
                          glm::vec3 direction) {
   return Block(BlockID::Grass, position, direction);
}

Block Block::createStone(Application &app, glm::vec3 position,
                          glm::vec3 direction) {
   return Block(BlockID::Stone, position, direction);
}

Block Block::createCobbleStone(Application &app, glm::vec3 position,
                          glm::vec3 direction) {
   return Block(BlockID::CobbleStone, position, direction);
}

Block Block::createBedrock(Application &app, glm::vec3 position,
                          glm::vec3 direction) {
   return Block(BlockID::Bedrock, position, direction);
}

Block Block::createMoonStone(Application &app, glm::vec3 position,
                          glm::vec3 direction) {
   return Block(BlockID::MoonStone, position, direction);
}

Block Block::createSand(Application &app, glm::vec3 position,
                          glm::vec3 direction) {
   return Block(BlockID::Sand, position, direction);
}

Block Block::createOakWood(Application &app, glm::vec3 position,
                          glm::vec3 direction) {
   return Block(BlockID::OakWood, position, direction);
}

Block Block::createLeaf(Application &app, glm::vec3 position,
                          glm::vec3 direction) {
   return Block(BlockID::Leaf, position, direction);
}

Block Block::createWater(Application &app, glm::vec3 position,
                          glm::vec3 direction) {
   return Block(BlockID::Water, position, direction);
}
bool Block::usesCubeMap(BlockID blockID)
{
   switch (blockID) {
   case BlockID::Air: return false;
   case BlockID::Dirt: return false;
   case BlockID::Grass: return true;
   case BlockID::Stone: return false;
   case BlockID::CobbleStone: return false;
   case BlockID::Bedrock: return false;
   case BlockID::MoonStone: return false;
   case BlockID::Sand: return false;
   case BlockID::OakWood: return true;
   case BlockID::Leaf: return false;
   case BlockID::Water: return false;
   }
}
glm::vec2 Block::getTextureUV(BlockID blockID, FaceMask face)
{
   switch (blockID) {
   case BlockID::Air: return glm::vec2(3.750000e-01, 5.000000e-01);
   case BlockID::Dirt: return glm::vec2(0.000000e+00, 5.000000e-01);
   case BlockID::Grass:       switch (face) {
      case F_Right: return glm::vec2(2.500000e-01, 0.000000e+00);
      case F_Left: return glm::vec2(2.500000e-01, 0.000000e+00);
      case F_Top: return glm::vec2(8.750000e-01, 0.000000e+00);
      case F_Bottom: return glm::vec2(0.000000e+00, 5.000000e-01);
      case F_Front: return glm::vec2(2.500000e-01, 0.000000e+00);
      case F_Back: return glm::vec2(2.500000e-01, 0.000000e+00);
      default: llvm_unreachable("not a valid cube face");
      }
   case BlockID::Stone: return glm::vec2(5.000000e-01, 0.000000e+00);
   case BlockID::CobbleStone: return glm::vec2(7.500000e-01, 0.000000e+00);
   case BlockID::Bedrock: return glm::vec2(2.500000e-01, 5.000000e-01);
   case BlockID::MoonStone: return glm::vec2(0.000000e+00, 0.000000e+00);
   case BlockID::Sand: return glm::vec2(3.750000e-01, 0.000000e+00);
   case BlockID::OakWood:       switch (face) {
      case F_Right: return glm::vec2(5.000000e-01, 5.000000e-01);
      case F_Left: return glm::vec2(5.000000e-01, 5.000000e-01);
      case F_Top: return glm::vec2(6.250000e-01, 0.000000e+00);
      case F_Bottom: return glm::vec2(6.250000e-01, 0.000000e+00);
      case F_Front: return glm::vec2(5.000000e-01, 5.000000e-01);
      case F_Back: return glm::vec2(5.000000e-01, 5.000000e-01);
      default: llvm_unreachable("not a valid cube face");
      }
   case BlockID::Leaf: return glm::vec2(1.250000e-01, 5.000000e-01);
   case BlockID::Water: return glm::vec2(1.250000e-01, 0.000000e+00);
   }
}
