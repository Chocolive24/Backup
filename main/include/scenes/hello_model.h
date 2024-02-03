#pragma once

#include "scene.h"
#include "model.h"
#include "camera.h"

class HelloModel final : public Scene {
public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  Pipeline pipeline_;
  Camera camera_;
  glm::mat4 model_, view_, projection_;
  Model backpack_;
};
