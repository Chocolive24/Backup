#pragma once

#include "scene.h"
#include "model.h"
#include "camera.h"

class DeferredShadingScene final : public Scene {
public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  Pipeline geometry_pipeline_;
  Pipeline lighting_pipeline_;
  Pipeline light_box_pipeline_;

  Camera camera_;
  glm::mat4 model_, view_, projection_;
  Model backpack_;

  Mesh cube_;
  Mesh screen_quad_;

  GLuint g_buffer_;
  // color buffers.
  GLuint pos_map_, normal_map_, albedo_spec_map_;

  GLuint depth_rbo_;

  void CreateGBuffer(const glm::vec2& screen_size);
};
