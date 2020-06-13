#include "mineshaft/Light/Light.h"

using namespace mc;

Light::Light(Kind lightKind) : lightKind(lightKind)
{

}

Light Light::createDirectionalLight(glm::vec3 direction,
                                    glm::vec3 ambient,
                                    glm::vec3 diffuse,
                                    glm::vec3 specular) {
   Light L(Directional);
   L.directionalData.direction = direction;
   L.directionalData.ambient = ambient;
   L.directionalData.diffuse = diffuse;
   L.directionalData.specular = specular;

   return L;
}

Light Light::createPointLight(glm::vec3 position,
                              float constant,
                              float linear,
                              float quadratic,
                              glm::vec3 ambient,
                              glm::vec3 diffuse,
                              glm::vec3 specular) {
   Light L(Point);
   L.pointData.position = position;
   L.pointData.ambient = ambient;
   L.pointData.diffuse = diffuse;
   L.pointData.specular = specular;
   L.pointData.constant = constant;
   L.pointData.linear = linear;
   L.pointData.quadratic = quadratic;

   return L;
}

struct DistanceValues {
   float linear;
   float quadratic;
};

static DistanceValues getLinearAndQuadratic(int distance)
{
   float linear;
   float quadratic;

   if (distance <= 7) {
      linear = 0.7;
      quadratic = 1.8;
   }
   else if (distance <= 13) {
      linear = 0.35;
      quadratic = 0.44;
   }
   else if (distance <= 20) {
      linear = 0.22;
      quadratic = 0.20;
   }
   else if (distance <= 32) {
      linear = 0.14;
      quadratic = 0.07;
   }
   else if (distance <= 50) {
      linear = 0.09;
      quadratic = 0.032;
   }
   else if (distance <= 65) {
      linear = 0.07;
      quadratic = 0.017;
   }
   else if (distance <= 100) {
      linear = 0.045;
      quadratic = 0.0075;
   }
   else if (distance <= 160) {
      linear = 0.027;
      quadratic = 0.0028;
   }
   else if (distance <= 200) {
      linear = 0.022;
      quadratic = 0.0019;
   }
   else if (distance <= 325) {
      linear = 0.014;
      quadratic = 0.0007;
   }
   else if (distance <= 600) {
      linear = 0.007;
      quadratic = 0.0002;
   }
   else {
      linear = 0.0014;
      quadratic = 0.000007;
   }

   return { linear, quadratic };
}

Light Light::createPointLight(glm::vec3 position,
                              int distance,
                              glm::vec3 ambient,
                              glm::vec3 diffuse,
                              glm::vec3 specular) {
   auto values = getLinearAndQuadratic(distance);
   return createPointLight(position, 1.0f, values.linear, values.quadratic,
                           ambient, diffuse, specular);
}

Light Light::createSpotlight(glm::vec3 position,
                             glm::vec3 direction,
                             float cutoff,
                             float outerCutoff,
                             float constant,
                             float linear,
                             float quadratic) {
   Light L(Spot);
   L.spotData.position = position;
   L.spotData.direction = direction;
   L.spotData.cutoff = cutoff;
   L.spotData.outerCutoff = outerCutoff;
   L.spotData.constant = constant;
   L.spotData.linear = linear;
   L.spotData.quadratic = quadratic;

   return L;
}

Light Light::createSpotlight(glm::vec3 position,
                             glm::vec3 direction,
                             float cutoff,
                             float outerCutoff,
                             int distance) {
   auto values = getLinearAndQuadratic(distance);
   return createSpotlight(position, direction, cutoff, outerCutoff,
                          1.0f, values.linear, values.quadratic);
}