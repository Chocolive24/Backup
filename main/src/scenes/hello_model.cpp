#include "hello_model.h"
#include "engine.h"
#include "file_utility.h"

#include <iostream>

void HelloModel::Begin() {
  pipeline_.Begin("data/shaders/hello_model/model.vert",
                  "data/shaders/hello_model/model.frag");

  backpack_.Load("data/models/leo_magnus/leo_magnus.obj", false, false);

  camera_.Begin(glm::vec3(0.f, 0.f, 3.f));
  glEnable(GL_DEPTH_TEST);
}

void HelloModel::End() {
  // Unload program/pipeline
  pipeline_.End();
  camera_.End();
  backpack_.Destroy();
}

void HelloModel::Update(float dt) {
  // Draw program
  pipeline_.Bind();

  // view/projection transformations
  camera_.Update(dt);
  view_ = camera_.CalculateViewMatrix();
  projection_ = glm::perspective(glm::radians(camera_.fov()),
                                 Engine::window_aspect(), 0.1f, 100.0f);

  pipeline_.SetMatrix4("transform.projection", projection_);
  pipeline_.SetMatrix4("transform.view", view_);

  // render the loaded model
  model_ = glm::mat4(1.0f);
  pipeline_.SetMatrix4("transform.model", model_);

  pipeline_.DrawModel(backpack_);
}

void HelloModel::OnEvent(const SDL_Event& event) { camera_.OnEvent(event); }
