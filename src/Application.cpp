#include "mineshaft/Application.h"
#include "mineshaft/GameSave.h"
#include "mineshaft/Entity/Player.h"
#include "mineshaft/World/Block.h"
#include "mineshaft/World/World.h"

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

Application::Application()
   : camera(*this),
     events(*this),
     blockTextures(),
     defaultFont(*this)
{

}

Application::~Application()
{
   for (auto &T : loadedTextures) {
      T.~BasicTexture();
   }

   glfwTerminate();
}

static Application *ctxPtr = nullptr;

static void keyPressed(GLFWwindow *window,
                       int key, int scancode,
                       int action, int mods) {
   if (action == GLFW_PRESS) {
      ctxPtr->events.keyPressed(key);
   }
   else if (action == GLFW_RELEASE) {
      ctxPtr->events.keyReleased(key);
   }
}

static void mouseButtonPressed(GLFWwindow *window,
                               int button, int action,
                               int mods) {
   if (action == GLFW_PRESS) {
      ctxPtr->events.mouseButtonPressed(button);
   }
   else if (action == GLFW_RELEASE) {
      ctxPtr->events.mouseButtonReleased(button);
   }
}

static float lastWClick = 0.0f;
static float lastSpaceClick = 0.0f;
static float lastEscapeClick = 0.0f;

static void handleKeyPressed(EventDispatcher &events, const Event &event)
{
   auto &data = event.getKeyboardEventData();
   int key = data.pressedKey;

   auto &app = events.getContext();
   auto *player = app.getPlayer();

   bool reschedule = true;
   switch (key) {
   case GLFW_KEY_W: {
      float time = app.getCamera().getCurrentTime();
      if (lastWClick != 0.0f && !data.rescheduled && (time - lastWClick) <= 0.3f) {
         app.controlOptions.movementSpeed = 24.0f;
         lastWClick = 0;
      }
      else {
         lastWClick = time;
      }

      player->strafeForward(app, app.controlOptions.movementSpeed);
      break;
   }
   case GLFW_KEY_A:
      player->strafeLeft(app, app.controlOptions.movementSpeed);
      break;
   case GLFW_KEY_S:
      player->strafeBackward(app, app.controlOptions.movementSpeed);
      break;
   case GLFW_KEY_D:
      player->strafeRight(app, app.controlOptions.movementSpeed);
      break;
   case GLFW_KEY_ESCAPE: {
      float time = app.getCamera().getCurrentTime();
      if (lastEscapeClick != 0.0f && !data.rescheduled && (time - lastEscapeClick) <= 0.3f) {
         app.shouldQuit = true;
         lastEscapeClick = 0;
      }
      else {
         lastEscapeClick = time;
      }

      switch (app.getGameSate()) {
      case Application::GameState::Running:
         app.setGameState(Application::GameState::Paused);
         break;
      case Application::GameState::Paused:
         app.setGameState(Application::GameState::Running);
         break;
      case Application::GameState::MainMenu:
         app.shouldQuit = true;
         break;
      }

      reschedule = false;
      break;
   }
   case GLFW_KEY_SPACE: {
      float time = app.getCamera().getCurrentTime();
      if (lastSpaceClick != 0.0f && !data.rescheduled
      && (time - lastSpaceClick) <= 0.3f) {
         switch (player->getMovementKind()) {
         case MovableEntity::MovementKind::Walking:
            player->setMovementKind(MovableEntity::MovementKind::Free);
            break;
         case MovableEntity::MovementKind::Free:
            player->setMovementKind(MovableEntity::MovementKind::Walking);
            break;
         }

         lastSpaceClick = 0.0f;
         reschedule = false;

         break;
      }
      else {
         lastSpaceClick = time;
      }

      switch (player->getMovementKind()) {
      case Player::MovementKind::Walking:
         player->initiateJump(app, 2.0f, app.controlOptions.movementSpeed);
         reschedule = false;

         break;
      case Player::MovementKind::Free:
         player->moveUp(app, app.controlOptions.movementSpeed);
         reschedule = true;
         break;
      }

      break;
   }
   case GLFW_KEY_LEFT_SHIFT:
      reschedule = player->moveDown(app, app.controlOptions.movementSpeed);
      break;
   case GLFW_KEY_UP:
      app.getCamera().setFOV(app.getCamera().getFOV() + 5.0f);
      reschedule = false;
      break;
   case GLFW_KEY_DOWN:
      app.getCamera().setFOV(app.getCamera().getFOV() - 5.0f);
      reschedule = false;
      break;
   case GLFW_KEY_F3:
      app.renderDebugInfo = !app.renderDebugInfo;
      reschedule = false;
      break;
   case GLFW_KEY_F5:
      app.getCamera().cycleCameraMode();
      reschedule = false;
      break;
#ifndef NDEBUG
   case GLFW_KEY_F9:
      app.breakpointsActive = !app.breakpointsActive;
      reschedule = false;
      break;
#endif
   default:
      reschedule = false;
      break;
   }

   auto *window = events.getContext().getWindow();
   if (reschedule && glfwGetKey(window, key) == GLFW_PRESS) {
      events.keyPressed(key, true);
   }
}

static void handleKeyReleased(EventDispatcher &events, const Event &event)
{
   auto &data = event.getKeyboardEventData();
   int key = data.pressedKey;

   switch (key) {
   case GLFW_KEY_W: {
      events.getContext().controlOptions.movementSpeed = 12.0f;
      break;
   }
   default:
      break;
   }
}

static void handleMouseButtonPressed(EventDispatcher &events, const Event &event)
{
   int button = event.getMouseButtonEventData().pressedButton;
   auto &Ctx = events.getContext();

   switch (button) {
   case GLFW_MOUSE_BUTTON_LEFT: {
      auto *world = Ctx.activeWorld;
      auto *block = world->getFocusedBlock();

      if (block) {
         world->updateBlock(block->getPosition(), Block::createAir(
            Ctx, getScenePosition(block->getPosition())));
      }

      break;
   }
   default:
      break;
   }
}

bool Application::initialize()
{
   ctxPtr = this;

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
   glfwWindowHint(GLFW_DECORATED, false);

   // Open a window and create its OpenGL context
   auto *monitor = glfwGetPrimaryMonitor();
   const GLFWvidmode* mode = glfwGetVideoMode(monitor);
   glfwWindowHint(GLFW_RED_BITS, mode->redBits);
   glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
   glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
   glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

   window = glfwCreateWindow(900, 600, "Mineshaft", nullptr, nullptr);
   if (!window) {
      fprintf(stderr, "Failed to open GLFW window.\n" );
      glfwTerminate();

      return true;
   }

   glewExperimental = GL_TRUE; // Needed in core profile
   camera.initialize(window);

   if (glewInit() != GLEW_OK) {
      fprintf(stderr, "Failed to initialize GLEW\n");
      return true;
   }

   // Ensure we can capture the escape key being pressed below
   glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

   // Hide the mouse and enable unlimited mouvement
   glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

   // Set the mouse at the center of the screen
   glfwPollEvents();
   glfwSetCursorPos(window, camera.getViewportWidth() / 2,
                    camera.getViewportHeight() / 2);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

   blockTextures = std::move(*TextureAtlas::fromFile(BasicTexture::DIFFUSE, "blocks.png", 16.0f, 16.0f));
   defaultFont.initialize("minecraft_regular_5.json");

   glfwSetKeyCallback(window, &keyPressed);
   glfwSetMouseButtonCallback(window, &mouseButtonPressed);

   events.registerEventHandler(Event::KeyPressed, &handleKeyPressed);
   events.registerEventHandler(Event::KeyReleased, &handleKeyReleased);
   events.registerEventHandler(Event::MouseButtonPressed, &handleMouseButtonPressed);

   return false;
}

int Application::runGameLoop()
{
   worldGenWorker.start();

   int errorCode = 0;
   bool firstFrame = true;

   do {
      // Clear the screen.
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

      // Update camera time.
      camera.updateCurrentTime();

      switch (gameState) {
      case GameState::MainMenu:
         errorCode = handleMainMenu();
         break;
      case GameState::Running:
         errorCode = handleMainGame();
         break;
      case GameState::Paused:
         errorCode = handlePause();
         break;
      }

      // Dispatch events.
      events.dispatchEvents();

      // Render and poll events.
      glfwSwapBuffers(window);
      glfwPollEvents();

      // Update camera time.
      camera.updateLastTime();

#ifdef __APPLE__
      if (firstFrame) {
         int x, y;
         glfwGetWindowPos(window, &x, &y);
         glfwSetWindowPos(window, x + 1, y);
         glfwSetWindowPos(window, x - 1, y);
      }
#endif

      firstFrame = false;
   }
   while (glfwWindowShouldClose(window) == 0
      && errorCode == 0
      && !shouldQuit);

   return errorCode;
}

int Application::handleMainMenu()
{
   loadedSave = std::make_unique<GameSave>(*this, WorldGenOptions());
   gameState = GameState::Running;

   activeWorld = &loadedSave->overworld;
   setBackgroundColor(glm::vec4(0.686f, 0.933f, 0.933f, 1.0f));

   // Initialize the world around the player on the main thread.
   activeWorld->updatePlayerPosition();

   return 0;
}

LLVM_ATTRIBUTE_UNUSED
static void worldGenTask(World *world)
{
   world->updatePlayerPosition();
}

int Application::handleMainGame()
{
   auto *player = getPlayer();

   // Update player and camera positions.
   activeWorld->updatePlayerPosition();
//   worldGenWorker.push_task(&worldGenTask, activeWorld);
   activeWorld->updateVisibility();

   player->updateViewingDirection(*this);
   camera.computeMatricesFromInputs();

   // Render UI.
   camera.renderCrosshair();
   camera.renderCoordinateSystem();

   // Render chunks.
   for (Chunk *chunk : activeWorld->getChunksToRender()) {
      if (camera.boxInFrustum(chunk->getBoundingBox()) != Camera::Outside) {
         chunk->updateVisibility();
         chunksToRender.push_back(chunk);
      }
   }

   camera.renderChunks(chunksToRender);

   auto *activeBlock = camera.getPointedAtBlock(*activeWorld);
   if (activeBlock) {
      camera.renderBorders(*activeBlock);
      activeWorld->setFocusedBlock(activeBlock);
   }

   if (renderDebugInfo) {
      camera.renderDebugOverlay();
   }

   chunksToRender.clear();
   return 0;
}

int Application::handlePause()
{
   return 0;
}

glm::vec4 Application::setBackgroundColor(const glm::vec4 &newColor) const
{
   glm::vec4 color = getBackgroundColor();
   glClearColor(newColor.x, newColor.y, newColor.z, newColor.w);

   return color;
}

glm::vec4 Application::getBackgroundColor() const
{
   glm::vec4 currentColor;
   glGetFloatv(GL_CURRENT_COLOR, reinterpret_cast<GLfloat*>(&currentColor));

   return currentColor;
}

Player* Application::getPlayer() const
{
   return loadedSave->player;
}

Model* Application::getOrLoadModel(llvm::StringRef fileName)
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

bool Application::isModelLoaded(llvm::StringRef modelName)
{
   return loadedModels.count(modelName) != 0;
}

Model *Application::internModel(llvm::StringRef modelName, mc::Model &&model)
{
   Model *internedModel = new(*this) Model(std::move(model));
   loadedModels[modelName] = internedModel;

   return internedModel;
}

BasicTexture* Application::loadTexture(BasicTexture::Kind K,
                                   llvm::StringRef File) {
   llvm::FoldingSetNodeID ID;
   BasicTexture::Profile(ID, K, File);

   void *InsertPos;
   if (auto *T = loadedTextures.FindNodeOrInsertPos(ID, InsertPos)) {
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
   loadedTextures.InsertNode(T, InsertPos);

   return T;
}

BasicTexture* Application::loadTexture(BasicTexture::Kind K,
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
Application::loadCubeMapTexture(BasicTexture::Kind K,
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

const Shader& Application::getShader(ShaderKind K)
{
   if (!Shaders[K]) {
      initializeShader(K);
   }

   return *Shaders[K];
}

void Application::initializeShader(ShaderKind K)
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
   case WATER_SHADER: {
      VertexName += "../src/Shader/Shaders/WaterShader";
      FragmentName += "../src/Shader/Shaders/WaterShader";
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

Shader* Application::loadShader(llvm::StringRef VertexShader,
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