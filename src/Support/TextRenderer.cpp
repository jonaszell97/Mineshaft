#include "mineshaft/Support/TextRenderer.h"

#include "mineshaft/Application.h"

#include <json.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/MemoryBuffer.h>
#include <GLFW/glfw3.h>

using namespace mc;
using json = nlohmann::json;

TextRenderer::TextRenderer(mc::Application &Ctx)
   : Ctx(Ctx), fontTexture(nullptr), width(0), height(0), charHeight(0),
     characterData()
{

}

TextRenderer::TextRenderer(Application &Ctx, BasicTexture *fontTexture,
                           unsigned int width, unsigned int height,
                           std::unordered_map<unsigned, CharacterData> &&characterData)
   : Ctx(Ctx), fontTexture(fontTexture), width(width), height(height),
     characterData(std::move(characterData))
{ }

void TextRenderer::initialize(llvm::StringRef bmfontFile)
{
   llvm::SmallString<64> fileName;
   fileName += "../assets/fonts/";
   fileName += bmfontFile;

   auto optBuffer = llvm::MemoryBuffer::getFile(fileName);
   if (!optBuffer) {
      return;
   }

   auto *buffer = optBuffer.get().get();
   json data = json::parse({ buffer->getBufferStart(), buffer->getBufferSize() });
   json &config = data["config"];
   json &symbols = data["symbols"];

   height = config["textureHeight"];
   width = config["textureWidth"];
   charHeight = config["charHeight"];

   std::string textureFile = config["textureFile"];
   fontTexture = Ctx.loadTexture(BasicTexture::DIFFUSE, textureFile);

   for (json &symbol : symbols) {
      CharacterData &c = characterData[symbol["id"]];
      c.x = symbol["x"];
      c.y = symbol["y"];
      c.height = symbol["height"];
      c.width = symbol["width"];
      c.xoffset = symbol["xoffset"];
      c.yoffset = symbol["yoffset"];
      c.xadvance = symbol["xadvance"];
      c.x = symbol["x"];
      c.x = symbol["x"];
      c.x = symbol["x"];
   }

   projectionMatrix = glm::ortho(0.0f, (float) Ctx.getCamera().getViewportWidth(),
                                 (float) Ctx.getCamera().getViewportHeight(), 0.0f);
}

void TextRenderer::renderCharacter(CharacterData &data,
                                   const glm::vec2 &pos,
                                   float scale) {
   if (!data.mesh.VAO) {
      glm::vec2 uv;
      uv.x = (float)data.x / (float)width;
      uv.y = (float)data.y / (float)height;

      float x = (float)data.xoffset;
      float y = (float)data.yoffset;

      data.mesh = Mesh::createQuad(glm::vec3(x, y, 0.0f),
                                   glm::vec3(x, y + data.height, 0.0f),
                                   glm::vec3(x + data.width, y + data.height, 0.0f),
                                   glm::vec3(x + data.width, y, 0.0f),
                                   uv, (float)data.width / (float)width,
                                   (float)data.height / (float)height,
                                   GL_CW);

      data.mesh.Textures.emplace_back(fontTexture, Material());
   }

   glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, 0.0f));
   modelMatrix = glm::translate(modelMatrix, glm::vec3(pos, 0.0f));

   auto &shader = Ctx.getShader(Application::BASIC_SHADER);
   data.mesh.render(shader, projectionMatrix, modelMatrix);
}

void TextRenderer::renderText(llvm::StringRef text,
                              const glm::vec2 &pos,
                              float scale) {
   glm::vec2 cursor = pos;
   for (auto &c : text) {
      if (c == '\n') {
         cursor.x = pos.x;
         cursor.y += charHeight;

         continue;
      }

      auto &data = characterData[c];
      renderCharacter(data, cursor, scale);

      cursor.x += data.xadvance;
   }
}