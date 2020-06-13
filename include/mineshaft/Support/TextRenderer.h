#ifndef MINESHAFT_TEXTRENDERER_H
#define MINESHAFT_TEXTRENDERER_H

#include "mineshaft/Model/Model.h"
#include "mineshaft/Texture/BasicTexture.h"

#include <unordered_map>

namespace mc {

class Application;

class TextRenderer {
private:
   /// Reference to the context object.
   Application &Ctx;

   /// The font texture.
   BasicTexture *fontTexture;

   /// Width of the font texture.
   unsigned width;

   /// Height of the font texture.
   unsigned height;

   /// Height of a single character.
   unsigned charHeight;

   /// The projection matrix for the current screen dimensions.
   glm::mat4 projectionMatrix;

   struct CharacterData {
      CharacterData() = default;

      unsigned x;
      unsigned y;
      unsigned height;
      unsigned width;
      unsigned xoffset;
      unsigned yoffset;
      unsigned xadvance;

      Mesh mesh;
   };

   /// Stored character data.
   std::unordered_map<unsigned, CharacterData> characterData;

   explicit TextRenderer(Application &Ctx);

   /// Private constructor.
   TextRenderer(Application &Ctx, BasicTexture *fontTexture,
                unsigned int width, unsigned int height,
                std::unordered_map<unsigned, CharacterData> &&characterData);

   void initialize(llvm::StringRef bmfontFile);

public:
   friend class Application;

   /// Move construction / assignment.
   TextRenderer(TextRenderer &&other) noexcept = default;
   TextRenderer &operator=(TextRenderer &&other) noexcept = delete;

   /// Display a single character.
   void renderCharacter(CharacterData &data, const glm::vec2 &pos,
                        float scale = 4.0f);

   /// Display text at the specified screen coordinate.
   void renderText(llvm::StringRef text, const glm::vec2 &pos,
                   float scale = 4.0f);

   /// Display text at the specified screen coordinate.
   void renderText(llvm::StringRef text, const BoundingBox &textBox);
};

} // namespace mc

#endif //MINESHAFT_TEXTRENDERER_H
