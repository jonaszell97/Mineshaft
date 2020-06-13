#include "mineshaft/Shader/Shader.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace mc;

Shader::Shader(unsigned ProgramID)
   : ProgramID(ProgramID)
{

}

Shader::Shader(Shader &&Other) noexcept
   : ProgramID(Other.ProgramID)
{
   Other.ProgramID = 0;
}

Shader::~Shader()
{
   glDeleteProgram(ProgramID);
}

Shader& Shader::operator=(Shader &&Other) noexcept
{
   this->~Shader();

   ProgramID = Other.ProgramID;
   Other.ProgramID = 0;

   return *this;
}

void Shader::useShader() const
{
   glUseProgram(ProgramID);
}

GLint Shader::getUniformLocation(const char *Name) const
{
   return glGetUniformLocation(ProgramID, Name);
}

void Shader::setUniform(GLint Location, GLint Val) const
{
   glUniform1i(Location, Val);
}

void Shader::setUniform(GLint Location, GLuint Val) const
{
   glUniform1ui(Location, Val);
}

void Shader::setUniform(GLint Location, float Val) const
{
   glUniform1f(Location, Val);
}

void Shader::setUniform(GLint Location, glm::vec3 Val) const
{
   glUniform3f(Location, Val.x, Val.y, Val.z);
}

void Shader::setUniform(GLint Location, glm::vec4 Val) const
{
   glUniform4f(Location, Val.x, Val.y, Val.z, Val.w);
}

void Shader::setUniform(GLint Location, glm::mat4 Val) const
{
   glUniformMatrix4fv(Location, 1, GL_FALSE, glm::value_ptr(Val));
}

void Shader::setUniform(const char *Name, GLint Val) const
{
   setUniform(getUniformLocation(Name), Val);
}

void Shader::setUniform(const char *Name, GLuint Val) const
{
   setUniform(getUniformLocation(Name), Val);
}

void Shader::setUniform(const char *Name, float Val) const
{
   setUniform(getUniformLocation(Name), Val);
}

void Shader::setUniform(const char *Name, glm::vec3 Val) const
{
   setUniform(getUniformLocation(Name), Val);
}

void Shader::setUniform(const char *Name, glm::vec4 Val) const
{
   setUniform(getUniformLocation(Name), Val);
}

void Shader::setUniform(const char *Name, glm::mat4 Val) const
{
   setUniform(getUniformLocation(Name), Val);
}