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

class MultipleLights final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

 private:
  static constexpr std::array<glm::vec3, 8> cube_positions_ = {
    glm::vec3( 0.0f,  0.0f,  0.0f), 
    glm::vec3( 2.0f,  3.0f, -5.0f), 
    glm::vec3(-1.5f, -2.2f, -2.5f),  
    glm::vec3(-3.8f, -2.0f, -5.3f),  
    glm::vec3( 2.4f, -0.4f, -3.5f),  
    glm::vec3(-1.7f,  3.0f, -4.5f),  
    glm::vec3( 1.3f, -2.0f, -2.5f),  
    glm::vec3( 1.5f,  2.0f, -2.5f),
  };

  float rotation_factor_ = 0.f;

  Pipeline pipeline_;
  Pipeline container_cubes_pipe_;
  Pipeline point_lights_pipe_;

  Mesh cube_;
  PhongMaterial material_ = {glm::vec3(0),  glm::vec3(0), glm::vec3(0), {}, {},
                        PhongShininess::k64};

  static constexpr glm::vec3 kOrange_ = glm::vec3(1.f, 0.6f, 0.2f);
  static constexpr glm::vec3 kRed_ = glm::vec3(1.f, 0.0f, 0.0f);
  static constexpr glm::vec3 kCyan_ = glm::vec3(0.f, 1.f, 1.f);

  DirectionalLight dir_light_ = DirectionalLight(glm::vec3(-0.2f, -1.0f, -0.3f),
                                                 glm::vec3(kOrange_) * 0.2f, 
                                                 glm::vec3(kOrange_), 
                                                 glm::vec3(kOrange_));

  std::array<PointLight, 2> point_lights_ = {
      PointLight(glm::vec3(0.f, -1.f, -4.f), 
                 glm::vec3(0.2f), 
                 glm::vec3(1.f),
                 glm::vec3(1.f), 
                 1.f, 0.09, 0.032f),
      PointLight(glm::vec3(0.f, 1.f, -4.f), 
                 glm::vec3(kRed_) * 0.2f, 
                 glm::vec3(kRed_),
                 glm::vec3(kRed_), 
                 1.f, 0.09, 0.032f)
  };


  SpotLight flashlight_ = SpotLight(glm::vec3(0.f), 
                                    glm::vec3(0.f),
                                    glm::vec3(kCyan_) * 0.2f, 
                                    glm::vec3(kCyan_), 
                                    glm::vec3(kCyan_),     
                                    1.f, 0.09, 0.032f,
                                    glm::cos(glm::radians(6.f)), 
                                    glm::cos(glm::radians(10.f)));

  Camera camera_;
  glm::mat4 model_, view_, projection_;
};
