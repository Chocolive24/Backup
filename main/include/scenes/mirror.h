#pragma once

#include "scene.h"
#include "mesh.h"
#include "camera.h"

class Mirror final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  Pipeline cube_pipeline_;
  Pipeline mirror_pipeline_;
  Pipeline reflection_pipeline_;

  Camera camera_;

  Mesh cube_;
  Mesh quad_;
  Texture texture_;

  static constexpr glm::vec3 cube_pos_ = glm::vec3(0.f, 0.f, 2.f);
  static constexpr glm::vec3 mirror_pos_ = glm::vec3(0.f);
  static constexpr glm::vec3 mirror_color_ = glm::vec3(0.2f);

  glm::mat4 model_, view_, projection_;
};
