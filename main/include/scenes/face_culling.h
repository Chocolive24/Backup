#pragma once

#include "scene.h"
#include "camera.h"
#include "mesh.h"

#include "glm/mat4x4.hpp"

class FaceCulling final : public Scene {
public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  Pipeline pipeline_;

  Camera camera_;

  Mesh cube_;

  Texture container_map_;

  glm::mat4 model_, view_, projection_;

  bool use_back_culling_ = true;
  bool use_ccw_winding_ = true;
};
