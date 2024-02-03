#pragma once

#include "glm/vec3.hpp"

enum class LightType { kDirectional, kPoint, kSpot };

// Base Light struct
struct Light {
  glm::vec3 position;
  glm::vec3 direction;

  // Color values.
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
};

struct DirectionalLight : Light {
  DirectionalLight(glm::vec3 direction, glm::vec3 ambient, glm::vec3 diffuse,
                   glm::vec3 specular);
};

struct PointLight : Light {
  float constant;
  float linear;
  float quadratic;

  PointLight(glm::vec3 position, glm::vec3 ambient, glm::vec3 diffuse,
             glm::vec3 specular, float constant, float linear, float quadratic);
};

struct SpotLight : Light {
  float constant;
  float linear;
  float quadratic;

  float cutoff;
  float outer_cutoff;

  SpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 ambient,
            glm::vec3 diffuse, glm::vec3 specular, float constant, float linear,
            float quadratic, float cutoff,
            float outer_cutoff);
};