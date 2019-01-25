//
// Created by Jonas Zell on 2019-01-20.
//

#ifndef OPENGLTEST_LIGHT_H
#define OPENGLTEST_LIGHT_H

#include <glm/glm.hpp>

namespace mc {

class Shader;

class Light {
public:
   enum Kind {
      /// \brief A directional light source.
      Directional,

      /// \brief A point light source.
      Point,

      /// \brief A spotlight.
      Spot,
   };

private:
   struct DirectionalData {
      glm::vec3 direction;
      glm::vec3 ambient;
      glm::vec3 diffuse;
      glm::vec3 specular;
   };

   struct PointData {
      glm::vec3 position;
      glm::vec3 ambient;
      glm::vec3 diffuse;
      glm::vec3 specular;

      float constant;
      float linear;
      float quadratic;
   };

   struct SpotData {
      glm::vec3 position;
      glm::vec3 direction;
      float cutoff;
      float outerCutoff;
      float constant;
      float linear;
      float quadratic;
   };

   /// The kind of light source this is.
   Kind lightKind;

   union {
      DirectionalData directionalData;
      PointData pointData;
      SpotData spotData;
   };

   /// Private C'tor.
   explicit Light(Kind lightKind);

public:
   static Light createDirectionalLight(glm::vec3 direction,
                                       glm::vec3 ambient,
                                       glm::vec3 diffuse,
                                       glm::vec3 specular);

   static Light createPointLight(glm::vec3 position,
                                 float constant,
                                 float linear,
                                 float quadratic,
                                 glm::vec3 ambient,
                                 glm::vec3 diffuse,
                                 glm::vec3 specular);

   static Light createPointLight(glm::vec3 position,
                                 int distance,
                                 glm::vec3 ambient,
                                 glm::vec3 diffuse,
                                 glm::vec3 specular);

   static Light createSpotlight(glm::vec3 position,
                                glm::vec3 direction,
                                float cutoff,
                                float outerCutoff,
                                float constant,
                                float linear,
                                float quadratic);

   static Light createSpotlight(glm::vec3 position,
                                glm::vec3 direction,
                                float cutoff,
                                float outerCutoff,
                                int distance);

   /// Apply the light to the given shader.
   void apply(const Shader &S) const;
};

} // namespace mc

#endif //OPENGLTEST_LIGHT_H
