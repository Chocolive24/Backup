#pragma once

#include "scene.h"
#include "texture.h"
#include "camera.h"
#include "mesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>

class PhongLight final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

 private:
  static constexpr glm::vec3 kRed_ = glm::vec3(1.0f, 0.0f, 0.0f);
  static constexpr glm::vec3 kBlue_ = glm::vec3(0.0f, 0.0f, 1.0f);
  static constexpr glm::vec3 kWhite_ = glm::vec3(1.0f, 1.0f, 1.0f);

  glm::vec3 cube_pos_ = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 cube_map_pos_ = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 cube_blinn_pos = glm::vec3(5.0f, 0.0f, 0.0f);


  std::array<glm::vec3, 2> moving_light_pos_ = {glm::vec3(1.2f, 0.0f, 2.0f),
                                               glm::vec3(1.2f, 1.0f, 2.0f)};

  glm::vec3 static_light_pos_ = glm::vec3(5.f, 1.f, 0.f);

  Pipeline pipeline_;
  Pipeline phong_pipeline_;
  Pipeline blinn_phong_pipeline_;
  Pipeline light_pipeline_;
  Pipeline cube_map_pipeline_;

  Mesh cube_;

  Texture diffuse_map_;
  Texture specular_map_;

  Camera camera_;
  //TODO: windowSize accessible dans les scenes.
  glm::mat4 model_ = glm::mat4(1.f);
  glm::mat4 view_ = glm::mat4(1.f);
  glm::mat4 projection_ = glm::perspective(glm::radians(45.0f),
                                           (float)1280 / (float)720, 0.1f, 100.0f);
};
