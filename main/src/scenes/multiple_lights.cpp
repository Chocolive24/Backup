#include <iostream>

#include "file_utility.h"
#include "multiple_lights.h"

void MultipleLights::Begin() {
  container_cubes_pipe_.Begin("data/shaders/transform.vert",
                              "data/shaders/multiple_lights.frag");

  point_lights_pipe_.Begin("data/shaders/transform.vert",
                           "data/shaders/phong_light/lamp.frag");
  // load texture
  material_.diffuse_map.Create("data/textures/container_diffuse.png", GL_REPEAT, GL_LINEAR);
  material_.specular_map.Create("data/textures/container_specular.png", GL_REPEAT, GL_LINEAR);

  cube_.CreateCube();
  camera_.Begin(glm::vec3(0.0f, 0.0f, 3.f));

  Engine::set_clear_color(glm::vec3(1.f, 0.6f, 0.2f));

  glEnable(GL_DEPTH_TEST);
}

void MultipleLights::End() {
  // Unload program/pipeline
  pipeline_.End();
  container_cubes_pipe_.End();
  point_lights_pipe_.End();

  cube_.Destroy();
  glDeleteTextures(1, &material_.diffuse_map.id);
  glDeleteTextures(1, &material_.specular_map.id);

  camera_.End();

  Engine::set_clear_color(glm::vec3(0));
}

void MultipleLights::Update(float dt) {
  container_cubes_pipe_.Bind();

  camera_.Update(dt);
  view_ = camera_.CalculateViewMatrix();
  projection_ = glm::perspective(glm::radians(camera_.fov()),
                                 Engine::window_aspect(), 0.1f, 100.0f);

  container_cubes_pipe_.SetMatrix4("transform.projection", projection_);
  container_cubes_pipe_.SetMatrix4("transform.view", view_);

  container_cubes_pipe_.SetVec3("viewPos", glm::vec3(camera_.position()));

  container_cubes_pipe_.SetLight("dir_light", dir_light_);

  for (std::size_t i = 0; i < point_lights_.size(); i++)
  {
    container_cubes_pipe_.SetLight("point_lights["+ std::to_string(i) + "]", point_lights_[i]);
  }

  flashlight_.position = camera_.position();
  flashlight_.direction = camera_.front();
  container_cubes_pipe_.SetLight("spot_light", flashlight_);


  if (rotation_factor_ >= 360.f) {
    rotation_factor_ = 0.f;
  }

  rotation_factor_ += dt;

  for (std::size_t i = 0; i < cube_positions_.size(); i++) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, cube_positions_[i]);
    model = glm::scale(model, glm::vec3(0.75, 0.75, 0.75));
    const auto angle = rotation_factor_ * (i * 0.5f);
    model = glm::rotate(model, angle, glm::vec3(1.0f, 0.3f, 0.5f));
    container_cubes_pipe_.SetMatrix4("transform.model", model);

    container_cubes_pipe_.SetMatrix4("normalMatrix",
                         glm::mat4(glm::transpose(glm::inverse(model))));

    container_cubes_pipe_.SetInt("material.diffuse_map", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material_.diffuse_map.id);

    container_cubes_pipe_.SetInt("material.specular_map", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, material_.specular_map.id);

    container_cubes_pipe_.SetFloat("material.shininess",
                       static_cast<int>(material_.shininess));

    container_cubes_pipe_.DrawMesh(cube_);
  }

  point_lights_pipe_.Bind();

  point_lights_pipe_.SetMatrix4("transform.projection", projection_);
  point_lights_pipe_.SetMatrix4("transform.view", view_);
       
  for (std::size_t i = 0; i < point_lights_.size(); i++) {
    const auto point_light = point_lights_[i];
    model_ = glm::mat4(1.0f);
    model_ = glm::translate(model_, point_light.position);
    model_ = glm::scale(model_, glm::vec3(0.3f, 0.3f, 0.3f));

    point_lights_pipe_.SetMatrix4("transform.model", model_);

    point_lights_pipe_.SetVec3("light_color", point_light.diffuse);

    point_lights_pipe_.DrawMesh(cube_);
  }
}

void MultipleLights::OnEvent(const SDL_Event& event) {
  camera_.OnEvent(event);
}