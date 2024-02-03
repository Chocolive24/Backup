#pragma once

#include "pipeline.h"

#include <GL/glew.h>
#include <glm/vec3.hpp>

// Material abstractions for PBR scenes.
// -------------------------------------

class Material {
 public:
  Material() noexcept = default;
  ~Material() noexcept;

  void Create(const GLuint& albedo_map, const GLuint& normal_map, const GLuint& metallic_map,
              const GLuint& roughness_map, const GLuint& ao_map);
  void Bind(GLenum gl_texture_idx) const noexcept;

  void Destroy();

 private:
  GLuint albedo_map_ = 0;
  GLuint normal_map_ = 0;
  GLuint metallic_map_ = 0;
  GLuint roughness_map_ = 0;
  GLuint ao_map_ = 0;
};

// Material abstractions for Phong light scenes.
// --------------------------------------------

enum class PhongShininess { 
	k2 = 2,
	k4 = 4,
	k8 = 8,
	k16 = 16,
	k32 = 32,
	k64 = 64,
	k128 = 128,
	k256 = 256
};

struct PhongMaterial {
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
  Texture diffuse_map;
  Texture specular_map;
  PhongShininess shininess = PhongShininess::k32;
};