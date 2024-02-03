#pragma once

#include "scene.h"
#include "camera.h"
#include "mesh.h"
#include "model.h"

#include "glm/mat4x4.hpp"

#include <memory>

enum class ShadowPipelineType {
  kNone,
  kScene,
  kShadowMapping,
  kShadowMapDebug,
};

class ShadowScene final : public Scene {
public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void DrawScene(ShadowPipelineType pipe_type);
  void OnEvent(const SDL_Event& event) override;

private:
  static constexpr int kShadowMapWidth_ = 1024, kShadowMapHeight_ = 1024;

  Pipeline scene_pipeline_;
  Pipeline normal_map_scene_pipe_;
  Pipeline hdr_pipeline_;
  Pipeline shadow_mapping_pipe_;
  Pipeline shadow_map_debug_pipe_;
  Pipeline* current_pipe_;

  Camera camera_;

  Model nanosuit_;

  Mesh cube_;
  Mesh screen_quad_;

  GLuint wood_map_;

  GLuint hdr_fbo_;
  GLuint hdr_color_buffer_;
  GLuint depth_rbo_;

  GLuint depth_map_fbo_;
  GLuint depth_map_;

  glm::mat4 model_, view_, projection_;
  glm::mat4 light_space_matrix_;

  glm::vec3 light_pos_ = glm::vec3(-5.0f, 7.5f, 3.0f);
  glm::vec3 light_dir_ = glm::normalize(glm::vec3(0) - light_pos_);

  GLenum shadow_map_cull_face = GL_BACK;
  bool debug_depth_map_ = false;
};
