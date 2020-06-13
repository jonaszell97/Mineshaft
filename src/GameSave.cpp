#include "mineshaft/GameSave.h"

#include "mineshaft/Application.h"
#include "mineshaft/Entity/Player.h"
#include "mineshaft/World/WorldGenerator.h"

using namespace mc;

GameSave::GameSave(World &&world,
                   WorldGenerator *worldGenerator,
                   Player *player)
   : overworld(std::move(world)), worldGenerator(worldGenerator), player(player)
{ }

GameSave::GameSave(Application &app, WorldGenOptions options)
   : overworld(app),
     worldGenerator(new(app) DefaultTerrainGenerator(&overworld, options)),
     player(new(app) Player(glm::vec3(-992.f, 48.5f, -232.f),
                            glm::vec3(1.0f, 0.0f, 0.0f),
                            glm::vec3(1.6f, 3.5f, 0.8f),
                            nullptr))
{
   overworld.registerEntity(player);
}

GameSave GameSave::createNew(Application &app, WorldGenOptions options)
{
   return GameSave(app, options);
}