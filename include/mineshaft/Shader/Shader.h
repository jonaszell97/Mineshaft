//
// Created by Jonas Zell on 2019-01-19.
//

#ifndef MINEKAMPF_SHADER_H
#define MINEKAMPF_SHADER_H

#include <GL/glew.h>
#include <glm/glm.hpp>

namespace mc {

class Context;

class Shader {
   /// Program ID of this shader.
   unsigned ProgramID = 0;

   /// Private C'tor.
   explicit Shader(unsigned ProgramID);

   friend class Context;

public:
   /// Cleans up the shader program.
   ~Shader();

   /// Shaders are move-only.
   Shader(const Shader&) = delete;
   Shader &operator=(const Shader&) = delete;

   Shader(Shader &&Other) noexcept;
   Shader &operator=(Shader &&Other) noexcept;

   /// Use this shader.
   void useShader() const;

   /// \return a uniform location.
   GLint getUniformLocation(const char *Name) const;

   /// Setters for uniform int values.
   void setUniform(const char *Name, GLint Val) const;

   /// Setters for uniform uint values.
   void setUniform(const char *Name, GLuint Val) const;

   /// Setters for uniform float values.
   void setUniform(const char *Name, float Val) const;

   /// Setters for uniform vec3 values.
   void setUniform(const char *Name, glm::vec3 Val) const;

   /// Setters for uniform vec4 values.
   void setUniform(const char *Name, glm::vec4 Val) const;

   /// Setters for uniform mat4 values.
   void setUniform(const char *Name, glm::mat4 Val) const;

   /// Setters for uniform int values.
   void setUniform(GLint Location, GLint Val) const;

   /// Setters for uniform uint values.
   void setUniform(GLint Location, GLuint Val) const;

   /// Setters for uniform float values.
   void setUniform(GLint Location, float Val) const;

   /// Setters for uniform vec3 values.
   void setUniform(GLint Location, glm::vec3 Val) const;

   /// Setters for uniform vec4 values.
   void setUniform(GLint Location, glm::vec4 Val) const;

   /// Setters for uniform mat4 values.
   void setUniform(GLint Location, glm::mat4 Val) const;
};

} // namespace mc

#endif //MINEKAMPF_SHADER_H
