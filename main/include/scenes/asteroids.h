#pragma once

#include "scene.h"
#include "camera.h"
#include "model.h"
#include "renderer.h"

class Asteroids final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  Pipeline asteroid_pipeline_;
  Pipeline planet_pipeline_;
  Pipeline screen_texture_pipe_;
  Pipeline debug_mesh_pipe_;

  Renderer renderer_;

  GLuint fbo_, rbo_;
  GLuint screen_texture_;

  Camera camera_;
  Camera debug_camera_;

  Mesh debug_cube_;

  Model asteroid_model_;
  Model planet_model_;

  GLuint frustum_vao_ = 0, frustum_vbo_ = 0, frustum_ebo_ = 0;
  std::array<GLuint, 36> frustum_indices_;

  GLuint screen_quad_vao_ = 0, screen_quad_vbo_ = 0, screen_quad_ebo_;
  std::vector<GLuint> debug_quad_indices_;

  static constexpr glm::vec3 kPlanetPos_ = glm::vec3(0.f, 0.f, 0.f);
  static constexpr std::uint32_t kAsteroidCount_ = 100'000;
  static constexpr float kRotationRadius_ = 40.f;
  static constexpr float radius_offset_ = 25.f;
  static constexpr float asteroid_speed_ = 5.f;

  std::vector<glm::mat4> asteroid_models_mat_;
  std::vector<glm::mat4> visible_asteroids_mat_;

  glm::mat4 model_ = glm::mat4(1.f), view_, projection_;
  glm::mat4 debug_view_, debug_projection_;
};