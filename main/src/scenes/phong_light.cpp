#include "phong_light.h"

#include <array>
#include <iostream>

#include "engine.h"
#include "file_utility.h"

void PhongLight::Begin() {
  phong_pipeline_.Begin("data/shaders/transform.vert",
                        "data/shaders/phong_light/phong_light.frag");
  blinn_phong_pipeline_.Begin("data/shaders/transform.vert",
                              "data/shaders/phong_light/blinn_phong_light.frag");

  cube_map_pipeline_.Begin("data/shaders/transform.vert",
                           "data/shaders/phong_light/phong_light_map.frag");
  light_pipeline_.Begin("data/shaders/transform.vert",
                        "data/shaders/phong_light/lamp.frag");

  diffuse_map_.Create("data/textures/container_diffuse.png", GL_REPEAT, GL_LINEAR);
  specular_map_.Create("data/textures/container_specular.png", GL_REPEAT, GL_LINEAR);

  pipeline_ = phong_pipeline_;

  cube_.CreateCube();
  camera_.Begin(glm::vec3(0.0f, 0.0f, 3.f));

  glEnable(GL_DEPTH_TEST);
}

void PhongLight::End() {
  // Unload program/pipeline
  pipeline_.End();
  phong_pipeline_.End();
  blinn_phong_pipeline_.End();
  light_pipeline_.End();
  cube_map_pipeline_.End();

  cube_.Destroy();

  diffuse_map_.Destroy();
  specular_map_.Destroy();

  camera_.End();
  projection_ = glm::perspective(glm::radians(45.0f), (float)1280 / (float)720,
                                 0.1f, 100.0f);
}

void PhongLight::Update(float dt) {
  camera_.Update(dt);

  pipeline_.Bind();
  // Vertex shader unfirom updates.
  view_ = camera_.CalculateViewMatrix();
  projection_ = glm::perspective(glm::radians(camera_.fov()),
                                 Engine::window_aspect(), 0.1f, 100.0f);
  pipeline_.SetMatrix4("transform.projection", projection_);
  pipeline_.SetMatrix4("transform.view", view_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, cube_pos_);
  model_ = glm::scale(model_, glm::vec3(0.75, 0.75, 0.75));
  pipeline_.SetMatrix4("transform.model", model_);
  pipeline_.SetMatrix4("normalMatrix",
                       glm::mat4(glm::transpose(glm::inverse(model_))));

  // Fragment shader unfirom updates.
  pipeline_.SetVec3("material.ambient", kRed_);
  pipeline_.SetVec3("material.diffuse", kRed_);
  pipeline_.SetVec3("material.specular", glm::vec3(0.5f, 0.5f, 0.5f));
  pipeline_.SetFloat("material.shininess", 64);

  pipeline_.SetVec3("light.position", moving_light_pos_[0]);
  pipeline_.SetVec3("light.ambient", kWhite_ * 0.1f);
  pipeline_.SetVec3("light.diffuse", kWhite_ * 0.6f);
  pipeline_.SetVec3("light.specular", kWhite_ * 0.5f);

  pipeline_.SetVec3("viewPos", glm::vec3(camera_.position()));

  pipeline_.DrawMesh(cube_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, cube_blinn_pos);
  model_ = glm::scale(model_, glm::vec3(2.f, 0.3f, 2.f));
  pipeline_.SetMatrix4("transform.model", model_);
  pipeline_.SetMatrix4("normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

  // Fragment shader unfirom updates.
  pipeline_.SetVec3("material.ambient", kBlue_);
  pipeline_.SetVec3("material.diffuse", kBlue_);
  pipeline_.SetVec3("material.specular", glm::vec3(0.5f, 0.5f, 0.5f));
  pipeline_.SetFloat("material.shininess", 8);

  pipeline_.SetVec3("light.position", static_light_pos_);
  pipeline_.SetVec3("light.ambient", kWhite_ * 0.1f);
  pipeline_.SetVec3("light.diffuse", kWhite_ * 0.6f);
  pipeline_.SetVec3("light.specular", kWhite_ * 0.5f);

  pipeline_.SetVec3("viewPos", glm::vec3(camera_.position()));

  pipeline_.DrawMesh(cube_);

  cube_map_pipeline_.Bind();

  // Vertex shader unfirom updates.
  cube_map_pipeline_.SetMatrix4("transform.projection", projection_);
  cube_map_pipeline_.SetMatrix4("transform.view", view_);

  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, cube_map_pos_);
  model_ = glm::scale(model_, glm::vec3(0.75, 0.75, 0.75));
  cube_map_pipeline_.SetMatrix4("transform.model", model_);
  cube_map_pipeline_.SetMatrix4(
      "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

  // Fragment shader unfirom updates.
  cube_map_pipeline_.SetInt("material.diffuse_map", 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, diffuse_map_.id);

  cube_map_pipeline_.SetInt("material.specular_map", 1);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, specular_map_.id);

  cube_map_pipeline_.SetFloat("material.shininess", 64);

  cube_map_pipeline_.SetVec3("light.position", moving_light_pos_[1]);
  cube_map_pipeline_.SetVec3("light.ambient", kWhite_ * 0.1f);
  cube_map_pipeline_.SetVec3("light.diffuse", kWhite_ * 0.6f);
  cube_map_pipeline_.SetVec3("light.specular", kWhite_ * 0.5f);

  cube_map_pipeline_.SetVec3("viewPos", glm::vec3(camera_.position()));

  cube_map_pipeline_.DrawMesh(cube_);

  light_pipeline_.Bind();

  light_pipeline_.SetMatrix4("transform.projection", projection_);
  light_pipeline_.SetMatrix4("transform.view", view_);

  static constexpr float radius = 3.f;
  static float time = 0.f;
  time += dt;

  for (auto& light_pos : moving_light_pos_) {
    model_ = glm::mat4(1.0f);
    light_pos.x = std::cos(time) * radius;
    light_pos.z = std::sin(time) * radius;
    model_ = glm::translate(model_, light_pos);
    model_ = glm::scale(model_, glm::vec3(0.3, 0.3, 0.3));
    light_pipeline_.SetMatrix4("transform.model", model_);

    light_pipeline_.SetVec3("light_color", kWhite_);

    light_pipeline_.DrawMesh(cube_);
  }

  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, static_light_pos_);
  model_ = glm::scale(model_, glm::vec3(0.3, 0.3, 0.3));
  light_pipeline_.SetMatrix4("transform.model", model_);

  light_pipeline_.SetVec3("light_color", kWhite_);

  light_pipeline_.DrawMesh(cube_);
}

void PhongLight::OnEvent(const SDL_Event& event) {
  camera_.OnEvent(event);

  switch (event.type) {
    case SDL_KEYDOWN:
      switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_Q:
          pipeline_ = blinn_phong_pipeline_;
          break;
        case SDL_SCANCODE_E:
          pipeline_ = phong_pipeline_;
          break;
        default:
          break;
      }
      break;

    default:
      break;
  }
}
