#include "light.h"

DirectionalLight::DirectionalLight(glm::vec3 direction, glm::vec3 ambient,
                                   glm::vec3 diffuse, glm::vec3 specular)
    : Light{glm::vec3(0.0f), direction, ambient, diffuse, specular} 
{
}

PointLight::PointLight(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse,
                       glm::vec3 specular, float constant, float linear,
                       float quadratic)
    : Light{position, glm::vec3(0.0f), ambient, diffuse, specular},
      constant(constant),
      linear(linear),
      quadratic(quadratic) 
{
}

SpotLight::SpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 ambient,
                     glm::vec3 diffuse, glm::vec3 specular, float constant,
                     float linear, float quadratic, float cutoff,
                     float outer_cutoff)
    : Light{position, direction, ambient, diffuse, specular},
      constant(constant),
      linear(linear),
      quadratic(quadratic), 
      cutoff(cutoff),
      outer_cutoff(outer_cutoff) 
{}
