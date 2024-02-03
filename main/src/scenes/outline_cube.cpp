#include "outline_cube.h"
#include "engine.h"
#include "texture.h"

void OutlineCube::Begin() {
  map_cube_pipe_.Begin("data/shaders/transform.vert",
                  "data/shaders/texture/texture.frag");

  outline_cube_pipe_.Begin("data/shaders/transform.vert",
                           "data/shaders/phong_light/lamp.frag");

  texture_.Create("data/textures/container.jpg", GL_REPEAT, GL_LINEAR);

  camera_.Begin(glm::vec3(0.0f, 0.0f, 3.f));
  cube_.CreateCube();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_STENCIL_TEST);
}

void OutlineCube::End() {
  // Unload program/pipeline
  glDisable(GL_STENCIL_TEST);
  glStencilFunc(GL_ALWAYS, 0, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
  glStencilMask(0xFF);
  map_cube_pipe_.End();
  outline_cube_pipe_.End();

  glDeleteTextures(1, &texture_.id);

  camera_.End();
  cube_.Destroy();
}

void OutlineCube::Update(float dt) {
  camera_.Update(dt);

  
  projection_ = glm::perspective(glm::radians(camera_.fov()),
                                 Engine::window_aspect(), 0.1f, 100.0f);
 

  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilFunc(GL_ALWAYS, 1, 0xFF); // All fragments should pass the stencil test.
  glStencilMask(0xFF);  // Enable writing to the stencil buffer.
  map_cube_pipe_.Bind();

  map_cube_pipe_.SetMatrix4("transform.view", camera_.CalculateViewMatrix());
  map_cube_pipe_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, cube_pos_);
  map_cube_pipe_.SetMatrix4("transform.model", model_);

  map_cube_pipe_.SetVec3("light_color", glm::vec3(0.2f, 0.5f, 0.8f));

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_.id);
  map_cube_pipe_.DrawMesh(cube_);

  glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
  glStencilMask(0x00);  // disable writing to the stencil buffer
  glDisable(GL_DEPTH_TEST);

  outline_cube_pipe_.Bind();

  model_ = glm::mat4(1.f);
  model_ = glm::scale(model_, glm::vec3(1.2f));

  outline_cube_pipe_.SetMatrix4("transform.model", model_);
  outline_cube_pipe_.SetMatrix4("transform.view", camera_.CalculateViewMatrix());
  outline_cube_pipe_.SetMatrix4("transform.projection", projection_);

  outline_cube_pipe_.SetVec3("light_color", glm::vec3(0.1f, 0.5f, 0.8f));

  outline_cube_pipe_.DrawMesh(cube_);

  glStencilMask(0xFF);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glEnable(GL_DEPTH_TEST);
}

void OutlineCube::OnEvent(const SDL_Event& event) { camera_.OnEvent(event); }
