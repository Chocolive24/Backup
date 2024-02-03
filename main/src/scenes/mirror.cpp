#include "mirror.h"
#include "engine.h"
#include "texture.h"

void Mirror::Begin() {
  cube_pipeline_.Begin("data/shaders/transform.vert",
                  "data/shaders/texture/texture.frag");
  mirror_pipeline_.Begin("data/shaders/transform.vert",
                         "data/shaders/phong_light/lamp.frag");
  reflection_pipeline_.Begin("data/shaders/transform.vert",
                           "data/shaders/colored_texture.frag");

  texture_.Create("data/textures/monkey.png", GL_CLAMP_TO_EDGE, GL_LINEAR);

  camera_.Begin(glm::vec3(1.f, 1.f, 5.f));
  cube_.CreateCube();
  quad_.CreateQuad();
}

void Mirror::End() {
  // Unload program/pipeline
  glDisable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glStencilMask(0xFF);

  cube_pipeline_.End();
  mirror_pipeline_.End();
  reflection_pipeline_.End();

  glDeleteTextures(1, &texture_.id);

  camera_.End();
  cube_.Destroy();
  quad_.Destroy();
}

void Mirror::Update(float dt) {
  camera_.Update(dt);
  
  projection_ = glm::perspective(glm::radians(camera_.fov()),
                                 Engine::window_aspect(), 0.1f, 100.0f);

  cube_pipeline_.Bind();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);

  glDisable(GL_STENCIL_TEST);

  cube_pipeline_.SetMatrix4("transform.view", camera_.CalculateViewMatrix());
  cube_pipeline_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, cube_pos_);
  cube_pipeline_.SetMatrix4("transform.model", model_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_.id);
  cube_pipeline_.DrawMesh(cube_);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_FALSE);

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 1, 0xFF); // All fragments should pass the stencil test.
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilMask(0xFF);  // Enable writing to the stencil buffer.

  mirror_pipeline_.Bind();

  mirror_pipeline_.SetMatrix4("transform.view", camera_.CalculateViewMatrix());
  mirror_pipeline_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, mirror_pos_);
  model_ = glm::scale(model_, glm::vec3(5.f));
  mirror_pipeline_.SetMatrix4("transform.model", model_);

  mirror_pipeline_.SetVec3("light_color", mirror_color_);
 
  mirror_pipeline_.DrawMesh(quad_);

  reflection_pipeline_.Bind();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);

  glEnable(GL_STENCIL_TEST);
  glStencilFunc(GL_EQUAL, 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilMask(0xFF);  // enable writing to the stencil buffer

  reflection_pipeline_.SetMatrix4("transform.view", camera_.CalculateViewMatrix());
  reflection_pipeline_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, -cube_pos_);
  //model_ = glm::rotate(model_, glm::radians(180.f), glm::vec3(0.f, 1.f, 0.f));
  model_ = glm::scale(model_, glm::vec3(1.f, 1.f, -1.f));
  reflection_pipeline_.SetMatrix4("transform.model", model_);

  reflection_pipeline_.SetVec3("texture_color", mirror_color_);

  reflection_pipeline_.DrawMesh(cube_);
}

void Mirror::OnEvent(const SDL_Event& event) { camera_.OnEvent(event); }
