#pragma once

#include "scene.h"
#include "camera.h"
#include "mesh.h"

#include <array>

struct Window
{
  glm::vec3 pos;
  float x_rotation;
};

class Blending final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  Pipeline cube_pipeline_;
  Pipeline window_pipeline_;

  Mesh cube_;
  Mesh quad_;

  Texture window_map_;
  Texture container_map_;

  Camera camera_;

  glm::mat4 model_, view_, projection_;

  static constexpr glm::vec3 cube_pos_ = glm::vec3(0.f);

  std::array<Window, 3> windows = {
    Window{ glm::vec3(2.f, 0.f, 2.f), 45.f},
    Window{ glm::vec3(-2.f, 0.f, 2.f), -45.f},
    Window{ glm::vec3(0.f, 0.f, -2.f), 0.f},
  };
};
