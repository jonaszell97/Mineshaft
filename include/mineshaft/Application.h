#ifndef MINESHAFT_CONTEXT_H
#define MINESHAFT_CONTEXT_H

#include "mineshaft/Camera.h"
#include "mineshaft/Config.h"
#include "mineshaft/Event/EventDispatcher.h"
#include "mineshaft/Texture/BasicTexture.h"
#include "mineshaft/Texture/TextureAtlas.h"
#include "mineshaft/Shader/Shader.h"
#include "mineshaft/Support/TextRenderer.h"
#include "mineshaft/Support/Worker.h"

#include <SFML/Graphics.hpp>
#include <llvm/ADT/FoldingSet.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/Support/Allocator.h>

#include <unordered_map>

class GLFWwindow;

namespace mc {

struct GameSave;
class Player;

struct GameOptions {
   /// The maximum render distance (in chunks per direction).
   unsigned renderDistance = 2;

   /// The maximum interaction distance.
   unsigned interactionDistance = 10;
};

struct ControlOptions {
   /// The movement speed.
   float movementSpeed = 12.0f;

   /// The mouse movement speed.
   float mouseSpeed = 0.005f;
};

class Application {
   /// The glfw window.
   GLFWwindow *window;

   /// The camera object.
   Camera camera;

   /// Map of loaded textures for quick access.
   llvm::FoldingSet<BasicTexture> loadedTextures;

   /// Map of loaded models
   llvm::StringMap<Model*> loadedModels;

   /// The loaded save file.
   std::unique_ptr<GameSave> loadedSave;

public:
   enum ShaderKind {
      BASIC_SHADER = 0,
      BASIC_SHADER_INSTANCED,
      CUBE_SHADER,
      CUBE_SHADER_INSTANCED,
      TEXTURE_ARRAY_SHADER_INSTANCED,
      BORDER_SHADER,
      CROSSHAIR_SHADER,
      SINGLE_COLOR_SHADER,
      NORMAL_SHADER,
      WATER_SHADER,

      __NUM_SHADERS
   };

   /// Allocator used in this context.
   mutable llvm::BumpPtrAllocator Allocator;

   /// The game options.
   GameOptions gameOptions;

   /// The control options.
   ControlOptions controlOptions;

   /// The event dispatcher.
   EventDispatcher events;

   /// Thread used for world generation.
   Worker worldGenWorker;

   /// The currently active world.
   World *activeWorld = nullptr;

   /// Chunks to render in the current frame.
   llvm::SmallVector<Chunk*, 16> chunksToRender;

   enum class GameState {
      /// The game is in the main menu.
      MainMenu,

      /// The game is running.
      Running,

      /// The game is paused.
      Paused,
   };

   /// The current game state.
   GameState gameState = GameState::MainMenu;

private:
   /// Loaded shaders.
   Shader *Shaders[__NUM_SHADERS] = { nullptr };

public:
   /// The block texture atlas.
   TextureAtlas blockTextures;

   /// The default font.
   TextRenderer defaultFont;

#ifndef NDEBUG
   bool breakpointsActive = false;
#endif

   bool renderDebugInfo = false;

   /// If true, the game will be exited after this frame.
   bool shouldQuit = false;

private:
   /// Initialize a shader.
   void initializeShader(ShaderKind K);

   /// Load a shader.
   Shader *loadShader(llvm::StringRef VertexShader,
                      llvm::StringRef FragmentShader,
                      llvm::StringRef GeometryShader = "");

   /// Run the main menu.
   int handleMainMenu();

   /// Run the game.
   int handleMainGame();

   /// Run the pause screen.
   int handlePause();

public:
   /// Creates a context without initializing it.
   Application();

   /// Tears down the context.
   ~Application();

   void *Allocate(size_t size, size_t alignment = 8) const
   {
      return Allocator.Allocate(size, alignment);
   }

   template<typename T>
   T *Allocate(size_t Num = 1) const
   {
      return static_cast<T *>(Allocate(Num * sizeof(T), alignof(T)));
   }

   void Deallocate(void *Ptr) const
   { }

   /// Initialize the OpenGL context.
   bool initialize();

   /// Run the main application loop.
   int runGameLoop();

   /// \return The current game state.
   GameState getGameSate() const { return gameState; }

   /// Set the current game state.
   void setGameState(GameState state) { gameState = state; }

   /// Set the background color of the scene.
   /// \return The current background color.
   glm::vec4 setBackgroundColor(const glm::vec4 &newColor) const;

   /// \return The current background color.
   glm::vec4 getBackgroundColor() const;

   /// \return The current player entity.
   Player *getPlayer() const;

   /// Load a model from a file, or return a previously loaded model of the
   /// same name.
   Model *getOrLoadModel(llvm::StringRef fileName);

   /// \return true iff a model with the given name is loaded.
   bool isModelLoaded(llvm::StringRef modelName);

   /// Intern a manually created model.
   Model *internModel(llvm::StringRef modelName, Model &&model);

   /// Load a texture.
   BasicTexture *loadTexture(BasicTexture::Kind K, llvm::StringRef File);

   /// Load a texture.
   BasicTexture *loadTexture(BasicTexture::Kind K, const sf::Image &Img,
                             llvm::StringRef File);

   /// Load a cube texture.
   BasicTexture *loadCubeMapTexture(BasicTexture::Kind K,
                                    const std::array<sf::Image, 6> &textures);

   /// \return This context's window.
   GLFWwindow *getWindow() const { return window; }

   /// \return The camera object.
   Camera &getCamera() { return camera; }

   /// \return The shader of the specified type.
   const Shader &getShader(ShaderKind K = BASIC_SHADER);
};

} // namespace mc

inline void *operator new(size_t size, const mc::Application& Ctx,
                          size_t alignment = 8) {
   return Ctx.Allocate(size, alignment);
}

inline void operator delete(void *ptr, const mc::Application& Ctx,
                            size_t) {
   return Ctx.Deallocate(ptr);
}

inline void *operator new[](size_t size, const mc::Application& Ctx,
                            size_t alignment = 8) {
   return Ctx.Allocate(size, alignment);
}

inline void operator delete[](void *ptr, const mc::Application& Ctx,
                              size_t) {
   return Ctx.Deallocate(ptr);
}

#endif //MINESHAFT_CONTEXT_H
