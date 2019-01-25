//
// Created by Jonas Zell on 2019-01-19.
//

#ifndef MINEKAMPF_CONTEXT_H
#define MINEKAMPF_CONTEXT_H

#include "mineshaft/Camera.h"
#include "mineshaft/Config.h"
#include "mineshaft/Texture/BasicTexture.h"
#include "mineshaft/Texture/TextureAtlas.h"
#include "mineshaft/Texture/TextureArray.h"
#include "mineshaft/Shader/Shader.h"

#include <SFML/Graphics.hpp>
#include <llvm/ADT/FoldingSet.h>
#include <llvm/Support/Allocator.h>

#include <unordered_map>

class GLFWwindow;

namespace mc {

struct GameOptions {
   /// The maximum render distance (in chunks per direction).
   unsigned renderDistance = 2;
};

class Context {
   /// The glfw window.
   GLFWwindow *Window;

   /// The camera object.
   Camera Cam;

   /// Map of loaded textures for quick access.
   llvm::FoldingSet<BasicTexture> LoadedTextures;

   /// Map of loaded models
   llvm::StringMap<Model*> loadedModels;

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

      __NUM_SHADERS
   };

   /// Allocator used in this context.
   mutable llvm::BumpPtrAllocator Allocator;

   /// The game options.
   GameOptions gameOptions;

private:
   /// Loaded shaders.
   Shader *Shaders[__NUM_SHADERS] = { nullptr };

public:
   /// The block texture atlas.
   const TextureAtlas blockTextures;
   TextureArray blockTextureArray;
   TextureArray blockTextureCubemapArray;

private:
   /// Initialize a shader.
   void initializeShader(ShaderKind K);

   /// Load a shader.
   Shader *loadShader(llvm::StringRef VertexShader,
                      llvm::StringRef FragmentShader,
                      llvm::StringRef GeometryShader = "");

public:
   /// Creates a context without initializing it.
   Context();

   /// Tears down the context.
   ~Context();

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

   /// Set the background color of the scene.
   /// \return The current background color.
   glm::vec4 setBackgroundColor(const glm::vec4 &newColor) const;

   /// \return The current background color.
   glm::vec4 getBackgroundColor() const;

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

   /// Load a texture atlas.
   TextureAtlas loadTextureAtlas(BasicTexture::Kind K,
                                 llvm::StringRef File);

   /// Load a cube texture.
   BasicTexture *loadCubeMapTexture(BasicTexture::Kind K,
                                    const std::array<sf::Image, 6> &textures);

   /// \return This context's window.
   GLFWwindow *getWindow() const { return Window; }

   /// \return The camera object.
   Camera &getCamera() { return Cam; }

   /// \return The shader of the specified type.
   const Shader &getShader(ShaderKind K = BASIC_SHADER);
};

} // namespace mc

inline void *operator new(size_t size, const mc::Context& Ctx,
                          size_t alignment = 8) {
   return Ctx.Allocate(size, alignment);
}

inline void operator delete(void *ptr, const mc::Context& Ctx,
                            size_t) {
   return Ctx.Deallocate(ptr);
}

inline void *operator new[](size_t size, const mc::Context& Ctx,
                            size_t alignment = 8) {
   return Ctx.Allocate(size, alignment);
}

inline void operator delete[](void *ptr, const mc::Context& Ctx,
                              size_t) {
   return Ctx.Deallocate(ptr);
}

#endif //MINEKAMPF_CONTEXT_H
