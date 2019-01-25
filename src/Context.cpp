//
// Created by Jonas Zell on 2019-01-19.
//

#include "mineshaft/Context.h"
#include "mineshaft/World/Block.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/MemoryBuffer.h>

#include <cstdio>

using namespace llvm;
using namespace mc;

Context::Context()
   : Cam(*this),
     blockTextures(*TextureAtlas::fromFile("blocks.png")),
     blockTextureArray()
{

}

Context::~Context()
{
   for (auto &T : LoadedTextures) {
      T.~BasicTexture();
   }

   glfwTerminate();
}

bool Context::initialize()
{
#ifndef NDEBUG
   srand(time(nullptr));
#endif

   // Initialise GLFW
   glewExperimental = GL_TRUE; // Needed for core profile

   if (!glfwInit()) {
      fprintf(stderr, "Failed to initialize GLFW\n");
      return true;
   }

   glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

   // Open a window and create its OpenGL context
   Window = glfwCreateWindow(1024, 768, "Tutorial 01", nullptr, nullptr);
   Cam.setWindow(Window);

   if (!Window) {
      fprintf(stderr, "Failed to open GLFW window.\n" );
      glfwTerminate();

      return true;
   }

   glfwMakeContextCurrent(Window); // Initialize GLEW
   glewExperimental = GL_TRUE; // Needed in core profile

   if (glewInit() != GLEW_OK) {
      fprintf(stderr, "Failed to initialize GLEW\n");
      return true;
   }

   // Ensure we can capture the escape key being pressed below
   glfwSetInputMode(Window, GLFW_STICKY_KEYS, GL_TRUE);

   // Hide the mouse and enable unlimited mouvement
   glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

   // Set the mouse at the center of the screen
   glfwPollEvents();
   glfwSetCursorPos(Window, 1024/2, 768/2);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_BLEND);

   // Enable depth test
   glEnable(GL_DEPTH_TEST);

   // Accept fragment if it closer to the camera than the former one
   glDepthFunc(GL_LESS);

   // Cull triangles whose normal is not towards the camera
   glEnable(GL_CULL_FACE);

   // Enable antisotropic filtering.
   GLfloat largest_supported_anisotropy;
   glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largest_supported_anisotropy);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largest_supported_anisotropy);

   blockTextureArray = Block::loadBlockTextures();
   blockTextureCubemapArray = Block::loadBlockCubeMapTextures();

   return false;
}

glm::vec4 Context::setBackgroundColor(const glm::vec4 &newColor) const
{
   glm::vec4 color = getBackgroundColor();
   glClearColor(newColor.x, newColor.y, newColor.z, newColor.w);

   return color;
}

glm::vec4 Context::getBackgroundColor() const
{
   glm::vec4 currentColor;
   glGetFloatv(GL_CURRENT_COLOR, reinterpret_cast<GLfloat*>(&currentColor));

   return currentColor;
}

Model* Context::getOrLoadModel(llvm::StringRef fileName)
{
   auto It = loadedModels.find(fileName);
   if (It != loadedModels.end()) {
      return It->getValue();
   }

   auto optModel = Model::loadFromFile(*this, fileName);
   if (!optModel) {
      return nullptr;
   }

   Model *model = new(*this) Model(std::move(*optModel));
   loadedModels[fileName] = model;

   return model;
}

bool Context::isModelLoaded(llvm::StringRef modelName)
{
   return loadedModels.count(modelName) != 0;
}

Model *Context::internModel(llvm::StringRef modelName, mc::Model &&model)
{
   Model *internedModel = new(*this) Model(std::move(model));
   loadedModels[modelName] = internedModel;

   return internedModel;
}

BasicTexture* Context::loadTexture(BasicTexture::Kind K,
                                   llvm::StringRef File) {
   llvm::FoldingSetNodeID ID;
   BasicTexture::Profile(ID, K, File);

   void *InsertPos;
   if (auto *T = LoadedTextures.FindNodeOrInsertPos(ID, InsertPos)) {
      return T;
   }

   llvm::SmallString<64> fileName;
   fileName += "../assets/textures/";
   fileName += File;

   sf::Image Img;
   if (!Img.loadFromFile(fileName.str())) {
      return nullptr;
   }

   auto *T = loadTexture(K, Img, fileName.str());
   LoadedTextures.InsertNode(T, InsertPos);

   return T;
}

BasicTexture* Context::loadTexture(BasicTexture::Kind K,
                                   const sf::Image &Img,
                                   llvm::StringRef File) {
   if (!Img.getPixelsPtr()) {
      return nullptr;
   }

   GLuint textureID;
   glGenTextures(1, &textureID);
   glBindTexture(GL_TEXTURE_2D, textureID);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Img.getSize().x, Img.getSize().y,
                0, GL_RGBA, GL_UNSIGNED_BYTE, Img.getPixelsPtr());

   glGenerateMipmap(GL_TEXTURE_2D);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   return new(*this) BasicTexture(K, textureID, File);
}

BasicTexture*
Context::loadCubeMapTexture(BasicTexture::Kind K,
                            const std::array<sf::Image, 6> &textures) {
   GLuint textureID;
   glGenTextures(1, &textureID);
   glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

   static constexpr unsigned faceOrder[] = {
      // top, bottom, left, right, front, back
      GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,

      GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
      GL_TEXTURE_CUBE_MAP_POSITIVE_X,

      GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
      GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
   };

   unsigned i = 0;
   for (auto &Img : textures) {
      glTexImage2D(faceOrder[i],
                   0, GL_RGBA, Img.getSize().x, Img.getSize().y,
                   0, GL_RGBA, GL_UNSIGNED_BYTE, Img.getPixelsPtr());

      ++i;
   }

   glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

   return new(*this) BasicTexture(K, textureID, "", GL_TEXTURE_CUBE_MAP);
}

const Shader& Context::getShader(ShaderKind K)
{
   if (!Shaders[K]) {
      initializeShader(K);
   }

   return *Shaders[K];
}

void Context::initializeShader(ShaderKind K)
{
   llvm::SmallString<64> VertexName;
   llvm::SmallString<64> FragmentName;
   llvm::SmallString<64> GeometryName;

   switch (K) {
   case BASIC_SHADER: {
      VertexName += "../src/Shader/Shaders/BasicShader";
      FragmentName += "../src/Shader/Shaders/BasicShader";
      break;
   }
   case BASIC_SHADER_INSTANCED: {
      VertexName += "../src/Shader/Shaders/BasicShaderInstanced";
      FragmentName += "../src/Shader/Shaders/BasicShader";
      break;
   }
   case CUBE_SHADER: {
      VertexName += "../src/Shader/Shaders/CubeShader";
      FragmentName += "../src/Shader/Shaders/CubeShader";
      break;
   }
   case CUBE_SHADER_INSTANCED: {
      VertexName += "../src/Shader/Shaders/CubeShaderInstanced";
      FragmentName += "../src/Shader/Shaders/CubeShader";
      break;
   }
   case TEXTURE_ARRAY_SHADER_INSTANCED: {
      VertexName += "../src/Shader/Shaders/BasicShaderInstanced";
      FragmentName += "../src/Shader/Shaders/BasicShaderTextureArray";
      break;
   }
   case BORDER_SHADER: {
      VertexName += "../src/Shader/Shaders/BorderShader";
      FragmentName += "../src/Shader/Shaders/BorderShader";
      break;
   }
   case CROSSHAIR_SHADER: {
      VertexName += "../src/Shader/Shaders/CrosshairShader";
      FragmentName += "../src/Shader/Shaders/CrosshairShader";
      break;
   }
   case SINGLE_COLOR_SHADER: {
      VertexName += "../src/Shader/Shaders/SingleColorShader";
      FragmentName += "../src/Shader/Shaders/SingleColorShader";
      break;
   }
   case NORMAL_SHADER: {
      VertexName += "../src/Shader/Shaders/NormalShader";
      FragmentName += "../src/Shader/Shaders/SingleColorShader";
      GeometryName += "../src/Shader/Shaders/NormalShader";
      break;
   }
   default:
      llvm_unreachable("invalid shader kind!");
   }

   VertexName += ".vertexshader";
   FragmentName += ".fragmentshader";

   if (!GeometryName.empty()) {
      GeometryName += ".geometryshader";
   }

   Shaders[K] = loadShader(VertexName, FragmentName, GeometryName);
}

Shader* Context::loadShader(llvm::StringRef VertexShader,
                            llvm::StringRef FragmentShader,
                            llvm::StringRef GeometryShader) {
   auto VertexBuf = MemoryBuffer::getFile(VertexShader);
   if (!VertexBuf) {
      return nullptr;
   }

   auto FragmentBuf = MemoryBuffer::getFile(FragmentShader);
   if (!FragmentBuf) {
      return nullptr;
   }

   GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
   GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
   GLuint GeometryShaderID = 0;

   GLint Result = GL_FALSE;
   GLint InfoLogLength;

   // Compile Vertex Shader
   printf("Compiling shader : %s\n", VertexShader.str().c_str());
   char const *VertexSourcePointer = VertexBuf.get()->getBufferStart();
   glShaderSource(VertexShaderID, 1, &VertexSourcePointer , nullptr);
   glCompileShader(VertexShaderID);

   // Check Vertex Shader
   glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
   glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);

   if (InfoLogLength > 0) {
      std::vector<char> VertexShaderErrorMessage((unsigned)InfoLogLength + 1);
      glGetShaderInfoLog(VertexShaderID, InfoLogLength, nullptr,
                         &VertexShaderErrorMessage[0]);

      printf("%s\n", &VertexShaderErrorMessage[0]);
      return nullptr;
   }

   // Compile Fragment Shader
   printf("Compiling shader : %s\n", FragmentShader.str().c_str());
   char const *FragmentSourcePointer = FragmentBuf.get()->getBufferStart();
   glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , nullptr);
   glCompileShader(FragmentShaderID);

   // Check Fragment Shader
   glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
   glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);

   if (InfoLogLength > 0) {
      std::vector<char> FragmentShaderErrorMessage((unsigned)InfoLogLength + 1);
      glGetShaderInfoLog(FragmentShaderID, InfoLogLength, nullptr,
                         &FragmentShaderErrorMessage[0]);

      printf("%s\n", &FragmentShaderErrorMessage[0]);
      return nullptr;
   }

   // Link the program
   printf("Linking program\n");

   GLuint ProgramID = glCreateProgram();
   glAttachShader(ProgramID, VertexShaderID);
   glAttachShader(ProgramID, FragmentShaderID);

   // Compile geometry shader.
   if (!GeometryShader.empty()) {
      auto GeometryBuf = MemoryBuffer::getFile(GeometryShader);
      if (!GeometryBuf) {
         return nullptr;
      }

      GeometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);

      printf("Compiling shader : %s\n", GeometryShader.str().c_str());
      char const *GeometrySourcePointer = GeometryBuf.get()->getBufferStart();
      glShaderSource(GeometryShaderID, 1, &GeometrySourcePointer , nullptr);
      glCompileShader(GeometryShaderID);

      // Check Vertex Shader
      glGetShaderiv(GeometryShaderID, GL_COMPILE_STATUS, &Result);
      glGetShaderiv(GeometryShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);

      if (InfoLogLength > 0) {
         std::vector<char> ShaderErrorMessage((unsigned)InfoLogLength + 1);
         glGetShaderInfoLog(GeometryShaderID, InfoLogLength, nullptr,
                            &ShaderErrorMessage[0]);

         printf("%s\n", &ShaderErrorMessage[0]);
         return nullptr;
      }

      glAttachShader(ProgramID, GeometryShaderID);
   }

   // Check the program
   glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
   glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
   glLinkProgram(ProgramID);

   if (InfoLogLength > 0) {
      std::vector<char> ProgramErrorMessage((unsigned)InfoLogLength + 1);
      glGetProgramInfoLog(ProgramID, InfoLogLength, nullptr,
                          &ProgramErrorMessage[0]);

      printf("%s\n", &ProgramErrorMessage[0]);
      return nullptr;
   }

   glDetachShader(ProgramID, VertexShaderID);
   glDetachShader(ProgramID, FragmentShaderID);

   glDeleteShader(VertexShaderID);
   glDeleteShader(FragmentShaderID);

   if (GeometryShaderID) {
      glDetachShader(ProgramID, GeometryShaderID);
      glDeleteShader(GeometryShaderID);
   }

   return new(*this) Shader(ProgramID);
}