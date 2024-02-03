#pragma once

#include "scene.h"
#include "mesh.h"

#include <array>

class UniformQuad final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;

 private:
  static constexpr int kPositionCount_ = 24;
  static constexpr int kIndiceBeginCount_ = 6;

  static constexpr float kTimeLimit_ = 60.f;

  Pipeline pipeline_;
  GLuint vao_, vbo_, ebo_;

  float color_coef_ = 0.f;
  float time_ = 0.f;

  std::vector<float> vertices_;
  std::vector<GLuint> indices_;
};
