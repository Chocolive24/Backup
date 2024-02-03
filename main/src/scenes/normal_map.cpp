#include "normal_map.h"
#include "engine.h"

void NormalMap::Begin() {
  wall_norm_map_pipe_.Begin("data/shaders/transform_tangent.vert",
                             "data/shaders/light_tangent.frag");
  wall_basic_map_pipe.Begin("data/shaders/transform.vert",
                             "data/shaders/phong_light/blinn_phong_light_map.frag");
  model_basic_map_pipe.Begin("data/shaders/transform.vert",
                             "data/shaders/phong_light/blinn_phong_light_model.frag");
  model_norm_map_pipe_.Begin("data/shaders/transform_tangent_model.vert",
                             "data/shaders/light_tangent_model.frag");
  light_cube_pipeline_.Begin("data/shaders/transform.vert",
                              "data/shaders/phong_light/lamp.frag");

  diffuse_map.Create("data/textures/brickwall.jpg", GL_CLAMP_TO_EDGE, GL_LINEAR);
  normal_map_.Create("data/textures/brickwall_normal.jpg", GL_CLAMP_TO_EDGE, GL_LINEAR);

  nanosuit_.Load("data/models/leo_magnus/leo_magnus.obj", false, false);

  cube_.CreateCube();
  
  camera_.Begin(glm::vec3(1.5f, 0.3f, 2.f));

  Engine::set_clear_color(glm::vec3(0.1f));

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
}

void NormalMap::End() {
  Engine::set_clear_color(glm::vec3(0.f));

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  // Unload program/pipeline
  wall_norm_map_pipe_.End();
  wall_basic_map_pipe.End();
  model_basic_map_pipe.End();
  model_norm_map_pipe_.End();
  light_cube_pipeline_.End();

  diffuse_map.Destroy();
  normal_map_.Destroy();

  nanosuit_.Destroy();

  cube_.Destroy();

  camera_.End();
}

void NormalMap::Update(float dt) { 
  camera_.Update(dt);
  view_ = camera_.CalculateViewMatrix();
  projection_ = glm::perspective(glm::radians(camera_.fov()),
                                 Engine::window_aspect(), 0.1f, 100.f);

  DrawLightCubes(dt);

  DrawNormalMapPipeline();

  model_basic_map_pipe.Bind();

  model_basic_map_pipe.SetMatrix4("transform.view", view_);
  model_basic_map_pipe.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(0.5f, 0.f, 0.f));
  model_ = glm::scale(model_, glm::vec3(0.3f));
  model_basic_map_pipe.SetMatrix4("transform.model", model_);

  model_basic_map_pipe.SetVec3("viewPos", glm::vec3(camera_.position()));
  model_basic_map_pipe.SetVec3("lightPos", light_pos_);
  model_basic_map_pipe.SetMatrix4("normalMatrix", 
      glm::mat4(glm::transpose(glm::inverse(model_))));


  model_basic_map_pipe.SetFloat("material.shininess", shininess);

  model_basic_map_pipe.DrawModel(nanosuit_);

  DrawBasicMapPipeline();

  model_norm_map_pipe_.Bind();

  model_norm_map_pipe_.SetMatrix4("transform.view", view_);
  model_norm_map_pipe_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(1.5f, 0.f, 0.f));
  model_ = glm::scale(model_, glm::vec3(0.3f));
  model_norm_map_pipe_.SetMatrix4("transform.model", model_);

  model_norm_map_pipe_.SetVec3("viewPos", glm::vec3(camera_.position()));
  model_norm_map_pipe_.SetVec3("lightPos", light_pos_);
  model_norm_map_pipe_.SetMatrix4(
      "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

  model_norm_map_pipe_.SetFloat("material.shininess", shininess);

  model_norm_map_pipe_.DrawModel(nanosuit_);
}

void NormalMap::OnEvent(const SDL_Event& event) { camera_.OnEvent(event); }

void NormalMap::DrawLightCubes(float dt) {
  light_cube_pipeline_.Bind();

  light_cube_pipeline_.SetMatrix4("transform.projection", projection_);
  light_cube_pipeline_.SetMatrix4("transform.view", view_);

  static constexpr float radius = 3.f;
  static float time;
  time += dt;

  glm::mat4 lamp_model = glm::mat4(1.0f);
  light_pos_.x = std::cos(time) * radius;
  light_pos_.z = std::sin(time) * radius;

  lamp_model = glm::translate(lamp_model, light_pos_);
  lamp_model = glm::scale(lamp_model, glm::vec3(0.3));
  light_cube_pipeline_.SetMatrix4("transform.model", lamp_model);

  light_cube_pipeline_.SetVec3("light_color", light_color_);

  light_cube_pipeline_.DrawMesh(cube_);
}

void NormalMap::DrawNormalMapPipeline() {
  wall_norm_map_pipe_.Bind();

  wall_norm_map_pipe_.SetMatrix4("transform.view", view_);
  wall_norm_map_pipe_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(-0.5f, 0.f, 0.f));
  wall_norm_map_pipe_.SetMatrix4("transform.model", model_);

  wall_norm_map_pipe_.SetVec3("viewPos", glm::vec3(camera_.position()));
  wall_norm_map_pipe_.SetVec3("lightPos", light_pos_);
  wall_norm_map_pipe_.SetMatrix4(
      "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, diffuse_map.id);
  wall_norm_map_pipe_.SetInt("material.texture_diffuse1", 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, normal_map_.id);
  wall_norm_map_pipe_.SetInt("material.texture_normal1", 1);

  wall_norm_map_pipe_.SetFloat("material.shininess", shininess);

  wall_norm_map_pipe_.DrawMesh(cube_);
}

void NormalMap::DrawBasicMapPipeline() {
  wall_basic_map_pipe.Bind();

  wall_basic_map_pipe.SetMatrix4("transform.view", view_);
  wall_basic_map_pipe.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(-1.6f, 0.f, 0.f));
  wall_basic_map_pipe.SetMatrix4("transform.model", model_);

  wall_basic_map_pipe.SetVec3("viewPos", glm::vec3(camera_.position()));
  wall_basic_map_pipe.SetVec3("lightPos", light_pos_);
  wall_basic_map_pipe.SetMatrix4(
      "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, diffuse_map.id);
  wall_basic_map_pipe.SetInt("material.texture_diffuse1", 0);

  wall_basic_map_pipe.SetFloat("material.shininess", shininess);

  wall_basic_map_pipe.DrawMesh(cube_);
}