#pragma once

#include "scene.h"
#include "texture.h"
#include "mesh.h"

#include <array>

class TextureQuad final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;

 private:
  static constexpr int kPositionCount_ = 4;
  static constexpr int kIndiceBeginCount_ = 6;

  Pipeline pipeline_;

  Mesh quad_;

  Texture diffuse_map_;
};
