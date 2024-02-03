#pragma once

#include "scene.h"

class HelloTriangle final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;

  private:
  Pipeline pipeline_;
  GLuint vao_ = 0;
};
