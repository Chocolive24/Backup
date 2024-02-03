#pragma once

#include "scene.h"
#include "model.h"
#include "camera.h"

class SSAO_Scene final : public Scene {
public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  Pipeline geometry_pipeline_;
  Pipeline ground_pipeline_;
  Pipeline ssao_pipeline_;
  Pipeline ssao_blur_pipeline_;
  Pipeline lighting_pipeline_;
  Pipeline ssao_debug_pipeline_;
  Camera camera_;

  Model backpack_;
  Mesh cube_;
  Mesh screen_quad_;

  GLuint g_buffer_;
  GLuint depth_rbo_;
  GLuint g_pos_map_, g_normal_map_, g_albedo_spec_map_;

  GLuint ssao_fbo_;
  GLuint ssao_color_buffer_;
  GLuint noise_texture_;
  
  GLuint ssao_blur_fbo_;
  GLuint ssao_blur_color_buffer_;

  std::vector<glm::vec3> ssao_kernel_;

  // lighting info.
  // -------------
  static constexpr glm::vec3 lightPos_ = glm::vec3(2.0, 4.0, -2.0);
  static constexpr glm::vec3 lightColor_ = glm::vec3(0.2, 0.2, 0.7);

  glm::mat4 model_, view_, projection_;
};
