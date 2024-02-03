#pragma once

#include "scene.h"
#include "camera.h"
#include "mesh.h"
#include "model.h"

#include "glm/mat4x4.hpp"

#include <memory>

enum class CsPipelineType {
  kNone,
  kScene,
  kShadowMapping,
  kShadowMapDebug,
};

class CascadedShadowScene final : public Scene {
public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void ShadowMappingPass(const float& camera_near, const float& camera_far);
  void OnEvent(const SDL_Event& event) override;

private:
  static constexpr int kShadowMapWidth_ = 4096, kShadowMapHeight_ = 4096;

  Pipeline ground_pipeline_;
  Pipeline backpack_pipeline_;
  Pipeline hdr_pipeline_;
  Pipeline shadow_mapping_pipe_;
  Pipeline shadow_map_debug_pipe_;

  Camera camera_;

  Model backpack_;

  Mesh cube_;
  Mesh screen_quad_;

  GLuint wood_map_;

  GLuint hdr_fbo_;
  GLuint hdr_color_buffer_;
  GLuint depth_rbo_;

  GLuint depth_map_fbo_;
  std::array<GLuint, 3> depth_maps_;

  glm::mat4 model_, view_, projection_;

  glm::vec3 light_dir_ = glm::normalize(glm::vec3(5.0f, -7.5f, -3.0f));

  GLenum shadow_map_cull_face = GL_BACK;
  int debug_depth_map_idx_ = 0;
  bool debug_depth_map_ = false;
  bool debug_color = false;

  float cascaded_near_ratio_ = 0.2f;
  float cascaded_mid_ratio_ = 0.6f;

  std::array<glm::mat4, 3> light_space_matrices_ = {};

  glm::mat4 CalculateLightSpaceMatrix(float near, float far);
  std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& proj,
                                                     const glm::mat4& view);
};
