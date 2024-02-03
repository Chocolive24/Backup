#pragma once

#include "scene.h"
#include "camera.h"
#include "texture.h"
#include "mesh.h"
#include "model.h"

class CubeMap final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  static constexpr glm::vec3 monkey_cube_pos_ = glm::vec3(2.f, 0.f, 0.f);
  static constexpr glm::vec3 backpack_pos_ = glm::vec3(6.f, 0.f, 0.f);

  Pipeline skybox_pipeline_;
  Pipeline reflection_pipeline_;
  Pipeline refraction_pipeline_;

  Model backpack_;

  Mesh skybox_;
  Mesh cube_;

  GLuint cube_map_ = 0;
  GLuint monkey_map_ = 0;

  Camera camera_;

  glm::mat4 model_, view_, projection_;

  float reflection_factor = 1.f;
  float refractive_index = 1.52f;
};
