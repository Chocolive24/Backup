#pragma once

#include "scene.h"
#include "model.h"
#include "camera.h"
#include "material.h"
#include "renderer.h"
#include "frame_buffer_object.h"
#include "bloom_frame_buffer_object.h"

#include <array>

enum class GeometryPipelineType { 
  kDeferredShading, 
  kShadowMapping, 
};

class FinalScene final : public Scene {
public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  Renderer renderer_;
  Camera camera_;

  Frustum camera_frustum_;

  glm::mat4 model_, view_, projection_;

  // IBL textures creation pipelines.
  // --------------------------------
  Pipeline equirect_to_cubemap_pipe_;
  Pipeline irradiance_pipeline_;
  Pipeline prefilter_pipeline_;
  Pipeline brdf_pipeline_;

  // Geometry pipelines.
  // -------------------
  Pipeline geometry_pipeline_;
  Pipeline instanced_geometry_pipeline_;
  Pipeline three_channels_geometry_pipe_;
  Pipeline ssao_pipeline_;
  Pipeline ssao_blur_pipeline_;
  Pipeline shadow_mapping_pipe_;
  Pipeline instanced_shadow_mapping_pipe_;

  // Drawing and lighting pipelines.
  // -------------------------------
  Pipeline pbr_lighting_pipeline_;
  Pipeline debug_lights_pipeline_;
  Pipeline cubemap_pipeline_;

  // Postprocessing pipelines.
  // -------------------------
  Pipeline down_sample_pipeline_;
  Pipeline up_sample_pipeline_;
  Pipeline bloom_hdr_pipeline_;

  // Meshes.
  // -------
  Mesh sphere_;
  Mesh cube_;
  Mesh cubemap_mesh_;
  Mesh screen_quad_;

  // Models.
  // -------
  Model leo_magnus_;
  Model sword_;

  // Materials.
  // ----------
  Material rusted_iron_mat_;
  Material bambo_mat;

  std::vector<GLuint> leo_magnus_textures_{};
  std::vector<GLuint> sword_textures_{};

  // Frame buffers.
  // --------------
  FrameBufferObject capture_fbo_;
  FrameBufferObject g_buffer_;
  FrameBufferObject ssao_fbo_;
  FrameBufferObject ssao_blur_fbo_;
  GLuint shadow_map_fbo_, shadow_map_;
  FrameBufferObject hdr_fbo_;

  // IBL textures data.
  // ------------------
  static constexpr std::uint16_t kSkyboxResolution = 4096;
  static constexpr std::uint8_t kIrradianceMapResolution = 32;
  static constexpr std::uint8_t kPrefilterMapResolution = 128;
  static constexpr std::uint16_t kBrdfLutResolution = 512;

  GLuint env_cubemap_;
  GLuint irradiance_cubemap_;
  GLuint prefilter_cubemap_;
  GLuint brdf_lut_;

  // Capture matrices for pre-computing IBL textures.
  // ------------------------------------------------
  const glm::mat4 capture_projection_ = glm::perspective(glm::radians(90.0f), 1.0f, 
                                                         0.1f, 10.f);
  const std::array<glm::mat4, 6> capture_views_ = {
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f))
  };

  // Shadow mapping data.
  // --------------------
  static constexpr int kShadowMapWidth_ = 2048, kShadowMapHeight_ = 2048;

  glm::mat4 light_space_matrix_ = glm::mat4(1.f);

  glm::vec3 light_pos_ = glm::vec3(15.0f, 20.f, 10.0f);
  glm::vec3 light_dir_ = glm::normalize(glm::vec3(0) - light_pos_);
  glm::vec3 light_color = glm::vec3(1.f);

  // SSAO variables.
  // ---------------
  static constexpr std::uint8_t kSsaoKernelSampleCount_ = 64;
  static constexpr std::uint8_t kSsaoNoiseDimensionX_ = 4;
  static constexpr std::uint8_t kSsaoNoiseDimensionY_ = 4;
  static constexpr float kSsaoRadius = 0.5f;
  static constexpr float kSsaoBiais = 0.025f;
  static constexpr float kCombiendAoFactor = 1.f;

  GLuint noise_texture_;

  std::array<glm::vec3, kSsaoKernelSampleCount_> ssao_kernel_{};

  // Bloom variables.
  // ----------------
  BloomFrameBufferObject bloom_fbo_;
  static constexpr std::uint8_t kBloomMipsCount_ = 5;
  static constexpr float kbloomFilterRadius_ = 0.005f;
  static constexpr float kBloomStrength_ = 0.04f;  // range (0.03, 0.15) works really well.

  // Instancing variables.
  // ---------------------------
  static constexpr std::uint8_t kRowCount_ = 7;
  static constexpr std::uint8_t kColumnCount_ = 7;
  static constexpr std::uint16_t kSphereCount_ = kRowCount_ * kColumnCount_;
  static constexpr float kSpacing_ = 2.5f;

  std::vector<glm::mat4> sphere_model_matrices_{};
  std::vector<glm::mat4> visible_sphere_model_matrices_{};

  // Lights variables.
  // -----------------
  static constexpr std::uint8_t kLightCount = 4;

  static constexpr std::array<glm::vec3, kLightCount> light_positions_ = {
      glm::vec3(-10.0f, 10.0f, 10.0f),
      glm::vec3(10.0f, 10.0f, 10.0f),
      glm::vec3(-10.0f, -10.0f, 10.0f),
      glm::vec3(10.0f, -10.0f, 10.0f),
  };

  static constexpr std::array<glm::vec3, kLightCount> light_colors_ = {
      glm::vec3(100.f), 
      glm::vec3(100.f),
      glm::vec3(100.f), 
      glm::vec3(100.f)
  };

  // Begin methods.
  // --------------
  void CreatePipelines() noexcept;

  void CreateMeshes() noexcept;
  void CreateModels() noexcept;
  void CreateMaterials() noexcept;

  void CreateFrameBuffers() noexcept;

  void CreateSsaoData() noexcept;

  void CreateHdrCubemap() noexcept;
  void CreateIrradianceCubeMap() noexcept;
  void CreatePrefilterCubeMap() noexcept;
  void CreateBrdfLut() noexcept;

  // Render passes.
  // --------------
  void ApplyGeometryPass() noexcept;
  void ApplySsaoPass() noexcept;
  void ApplyShadowMappingPass() noexcept;
  void ApplyDeferredPbrLightingPass() noexcept;
  void ApplyFrontShadingPass() noexcept;
  void ApplyBloomPass() noexcept;
  void ApplyHdrPass() noexcept;

  void DrawObjectGeometry(GeometryPipelineType geometry_type) noexcept;
  void DrawInstancedObjectGeometry(GeometryPipelineType geometry_type) noexcept;

  // End methods.
  // ------------
  void DestroyPipelines() noexcept;

  void DestroyMeshes() noexcept;
  void DestroyModels() noexcept;
  void DestroyMaterials() noexcept;

  void DestroyIblPreComputedCubeMaps() noexcept;

  void DestroyFrameBuffers() noexcept;
};
