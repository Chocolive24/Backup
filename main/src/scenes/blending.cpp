#include "blending.h"
#include "texture.h"
#include "engine.h"

#include <algorithm>

void Blending::Begin() {
  cube_pipeline_.Begin("data/shaders/transform.vert",
                       "data/shaders/blending.frag");
  window_pipeline_.Begin("data/shaders/transform.vert",
                         "data/shaders/blending.frag");

  container_map_.Create("data/textures/container.jpg", GL_CLAMP_TO_EDGE, GL_LINEAR);
  window_map_.Create("data/textures/window.png", GL_CLAMP_TO_EDGE, GL_LINEAR);

  cube_.CreateCube();
  quad_.CreateQuad();

  camera_.Begin(glm::vec3(0.f, 0.f, 5.f));

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Blending::End() {
  // Unload program/pipeline
  glDisable(GL_BLEND);

  cube_pipeline_.End();
  window_pipeline_.End();

  glDeleteTextures(1, &container_map_.id);
  glDeleteTextures(1, &window_map_.id);

  cube_.Destroy();
  quad_.Destroy();
}

void Blending::Update(float dt) {
  camera_.Update(dt);
  view_ = camera_.CalculateViewMatrix();
  projection_ = glm::perspective(glm::radians(camera_.fov()),
                                 Engine::window_aspect(), 0.1f, 100.0f);

  cube_pipeline_.Bind();

  cube_pipeline_.SetMatrix4("transform.view", view_);
  cube_pipeline_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, cube_pos_);
  cube_pipeline_.SetMatrix4("transform.model", model_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, container_map_.id);

  cube_pipeline_.DrawMesh(cube_);

  window_pipeline_.Bind();

  window_pipeline_.SetMatrix4("transform.view", view_);
  window_pipeline_.SetMatrix4("transform.projection", projection_);

  glBindTexture(GL_TEXTURE_2D, window_map_.id);

  const auto cam_pos = camera_.position();
  std::sort(windows.begin(), windows.end(),
            [&cam_pos](const Window& a, const Window& b) {
                return glm::length(cam_pos - a.pos) >
                       glm::length(cam_pos - b.pos);
            });

  for (const auto window : windows)
  {
    model_ = glm::mat4(1.f);
    model_ = glm::translate(model_, window.pos);
    model_ = glm::rotate(model_, glm::radians(window.x_rotation),
                         glm::vec3(0.f, 1.f, 0.f));

    window_pipeline_.SetMatrix4("transform.model", model_);

    window_pipeline_.DrawMesh(quad_);
  }
}

void Blending::OnEvent(const SDL_Event& event) { camera_.OnEvent(event); }