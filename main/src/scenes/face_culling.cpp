#include "face_culling.h"
#include "engine.h"
#include <iostream>

void FaceCulling::Begin() {
  pipeline_.Begin("data/shaders/transform.vert",
                  "data/shaders/texture/texture.frag");

  container_map_.Create("data/textures/container.jpg", 
                        GL_CLAMP_TO_EDGE, GL_LINEAR);

  camera_.Begin(glm::vec3(0.f, 0.f, 3.f));

  cube_.CreateCube();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
}

void FaceCulling::End() {
  // Unload program/pipeline
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  pipeline_.End();

  camera_.End();

  cube_.Destroy();

  container_map_.Destroy();
}

void FaceCulling::Update(float dt) {
  // Draw program
  camera_.Update(dt);

  const auto cull_face = use_back_culling_ ? GL_BACK : GL_FRONT;
  glCullFace(cull_face);

  const auto winding_order = use_ccw_winding_ ? GL_CCW : GL_CW;
  glFrontFace(winding_order);

  pipeline_.Bind();

  view_ = camera_.CalculateViewMatrix();
  projection_ = glm::perspective(glm::radians(camera_.fov()),
                                 Engine::window_aspect(), 0.1f, 100.0f);

  pipeline_.SetMatrix4("transform.view", view_);
  pipeline_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  pipeline_.SetMatrix4("transform.model", model_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, container_map_.id);

  pipeline_.DrawMesh(cube_);
}

void FaceCulling::OnEvent(const SDL_Event& event) { 
    camera_.OnEvent(event);

    switch (event.type) {
      case SDL_KEYDOWN:
        switch (event.key.keysym.scancode) {
          case SDL_SCANCODE_Q:
            use_back_culling_ = !use_back_culling_;
            break;
          case SDL_SCANCODE_E:
            use_ccw_winding_ = !use_ccw_winding_;
            break;
          default:
            break;
        }
        break;

      default:
        break;
    }
}
