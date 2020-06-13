#ifndef MINESHAFT_SAVEFILE_H
#define MINESHAFT_SAVEFILE_H

#include "mineshaft/World/World.h"

namespace mc {

class Player;
class WorldGenerator;

struct GameSave {
private:
private:
   GameSave(World &&world,
            WorldGenerator *worldGenerator,
            Player *player);

public:
   GameSave(Application &app, WorldGenOptions options);

   /// The overworld.
   World overworld;

   /// The world generator.
   WorldGenerator *worldGenerator;

   /// The player entity.
   Player *player;

   static GameSave createNew(Application &app, WorldGenOptions options);
   static llvm::Optional<GameSave> loadFromFile(Application &app,
                                                llvm::StringRef fileName);
};

} // namespace mc

#endif //MINESHAFT_SAVEFILE_H
