#pragma once

#include "scene.h"
#include "model.h"
#include "camera.h"
#include "material.h"
#include "renderer.h"

#include <array>

class PBR_Scene final : public Scene {
public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  Renderer renderer_;

  Pipeline pbr_pipeline_;
  Pipeline textured_pbr_pipeline_;
  Pipeline equirect_to_cubemap_pipe_;
  Pipeline cubemap_pipeline_;
  Pipeline irradiance_pipeline_;
  Pipeline prefilter_pipeline_;
  Pipeline brdf_pipeline_;
  Pipeline hdr_pipeline_;
  Pipeline* current_pipeline_ = nullptr;

  Camera camera_;

  glm::mat4 model_, view_, projection_;

  Model backpack_;
  Mesh sphere_;
  Mesh cube_;
  Mesh cubemap_mesh_;
  Mesh screen_quad_;

  Material rusted_iron_mat_;

  GLuint equirectangular_map_;

  GLuint capture_fbo_;
  GLuint capture_rbo_;

  GLuint env_cubemap_;
  GLuint irradiance_cubemap_;
  GLuint prefilter_cubemap_;
  GLuint brdf_lut_; // lut = look up texture.

  GLuint hdr_fbo_;
  GLuint hdr_color_buffer_;
  GLuint depth_rbo_;

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

  // Sphere placement variables.
  // ---------------------------
  static constexpr int kRowCount = 7;
  static constexpr int kColumnCount = 7;
  static constexpr float kSpacing = 2.5f;

  // Lights variables.
  // -----------------
  static constexpr int kLightCount = 4;
  static constexpr float kLightsRotRadius = 20.f;

  std::array<glm::vec3, kLightCount> light_positions = {
      glm::vec3(-10.0f, 10.0f, 10.0f),
      glm::vec3(10.0f, 10.0f, 10.0f),
      glm::vec3(-10.0f, -10.0f, 10.0f),
      glm::vec3(10.0f, -10.0f, 10.0f),
  };

  static constexpr std::array<glm::vec3, kLightCount> light_colors = {
      glm::vec3(300.0f, 300.0f, 300.0f), 
      glm::vec3(300.0f, 300.0f, 300.0f),
      glm::vec3(300.0f, 300.0f, 300.0f), 
      glm::vec3(300.0f, 300.0f, 300.0f)
  };

  void CreateHDR_Cubemap();
  void CreateIrradianceCubeMap();
  void CreatePrefilterCubeMap();
  void CreateBRDF_LUT_Texture();
};
