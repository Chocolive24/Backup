#pragma once

#include "scene.h"
#include "mesh.h"
#include "camera.h"

class OutlineCube final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  Pipeline map_cube_pipe_;
  Pipeline outline_cube_pipe_;

  Camera camera_;

  Mesh cube_;
  Texture texture_;

  glm::vec3 cube_pos_ = glm::vec3(0.f);

  glm::mat4 model_, view_, projection_;
};
