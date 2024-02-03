#pragma once

#include "scene.h"
#include "texture.h"
#include "camera.h"
#include "mesh.h"
#include "material.h"
#include "engine.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>

class LightCasters final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

 private:
  static constexpr std::array<glm::vec3, 8> cube_positions_ = {
    glm::vec3( 0.0f,  0.0f,  0.0f), 
    glm::vec3( 2.0f,  5.0f, -15.0f), 
    glm::vec3(-1.5f, -2.2f, -2.5f),  
    glm::vec3(-3.8f, -2.0f, -12.3f),  
    glm::vec3( 2.4f, -0.4f, -3.5f),  
    glm::vec3(-1.7f,  3.0f, -7.5f),  
    glm::vec3( 1.3f, -2.0f, -2.5f),  
    glm::vec3( 1.5f,  2.0f, -2.5f),
  };

  float rotation_factor_ = 0.f;

  Pipeline pipeline_;
  Pipeline dir_light_pipeline_;
  Pipeline point_light_pipeline_;
  Pipeline flashlight_pipeline_;

  Pipeline lamp_pipeline_;

  LightType current_light_type_ = LightType::kDirectional;

  Mesh cube_;
  PhongMaterial material_ = {glm::vec3(0),  glm::vec3(0), glm::vec3(0), {}, {},
                        PhongShininess::k64};

  DirectionalLight dir_light_ = DirectionalLight(glm::vec3(-0.2f, -1.0f, -0.3f),
                                                 glm::vec3(0.2f), 
                                                 glm::vec3(1.f), 
                                                 glm::vec3(1.f));

  PointLight point_light_ = PointLight(glm::vec3(0.f, 0.f, -5.f), 
                                       glm::vec3(0.2f), 
                                       glm::vec3(1.f), 
                                       glm::vec3(1.f),
                                       1.f, 0.09, 0.032f );

  SpotLight flashlight_ = SpotLight(glm::vec3(0.f), 
                                    glm::vec3(0.f),
                                    glm::vec3(0.2f), 
                                    glm::vec3(1.f), 
                                    glm::vec3(1.f),  
                                    1.f, 0.09, 0.032f,
                                    glm::cos(glm::radians(6.f)), 
                                    glm::cos(glm::radians(10.f)));

  Camera camera_;
  //TODO: windowSize accessible dans les scenes.
  glm::mat4 projection_;

  bool must_appply_textures_ = true;
  bool use_blinn_phong_ = true;
};
