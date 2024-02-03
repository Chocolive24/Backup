#include "post_processing.h"
#include "engine.h"
#include "error.h"

#include <iostream>

void PostProcessing::Begin() {
  cube_pipeline_.Begin("data/shaders/transform.vert",
                       "data/shaders/texture/texture.frag");
  reversed_color_pipeline_.Begin("data/shaders/screen_texture.vert",
                                 "data/shaders/reversed_colors.frag");
  gray_scale_pipeline_.Begin("data/shaders/screen_texture.vert",
                             "data/shaders/gray_scale.frag");
  kernel_pipeline_.Begin("data/shaders/screen_texture.vert",
                         "data/shaders/kernel.frag");
  blur_pipeline_.Begin("data/shaders/screen_texture.vert",
                       "data/shaders/blur.frag");
  edge_detection_pipeline_.Begin("data/shaders/screen_texture.vert",
                                 "data/shaders/edge_detection.frag");

  post_process_pipeline_ = edge_detection_pipeline_;

  container_map_.Create("data/textures/container.jpg", 
                        GL_CLAMP_TO_EDGE, GL_LINEAR);
  monkey_map.Create("data/textures/monkey.png", 
                    GL_CLAMP_TO_EDGE, GL_LINEAR);

  camera_.Begin(glm::vec3(0.f, 0.f, 3.f));

  nanosuit_.Load("data/models/nanosuit/nanosuit.obj", false);

  cube_.CreateCube();
  screen_quad.CreateScreenQuad();

  const auto window_size = Engine::window_size();

  ColorAttachment color_attachment(GL_RGB, GL_RGB, GL_LINEAR,
                                   GL_CLAMP_TO_EDGE);
  DepthStencilAttachment depth_stencil_attachment(GL_DEPTH24_STENCIL8,
                                                  GL_DEPTH_STENCIL_ATTACHMENT);
  FrameBufferSpecification specification;
  specification.SetSize(window_size);
  specification.PushColorAttachment(color_attachment);
  specification.SetDepthStencilAttachment(depth_stencil_attachment);

  fbo_.Create(specification);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
}

void PostProcessing::End() {
  // Unload program/pipeline
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  cube_pipeline_.End();
  post_process_pipeline_.End();
  reversed_color_pipeline_.End();
  gray_scale_pipeline_.End();
  kernel_pipeline_.End();
  blur_pipeline_.End();
  edge_detection_pipeline_.End();

  fbo_.Destroy();

  camera_.End();

  nanosuit_.Destroy();

  cube_.Destroy();
  screen_quad.Destroy();

  container_map_.Destroy();
  monkey_map.Destroy();
}

void PostProcessing::Update(float dt) {
  camera_.Update(dt);

  const auto window_size = Engine::window_size();

  // first pass
  fbo_.Bind();

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // we're not using the stencil buffer now

  cube_pipeline_.Bind();

  view_ = camera_.CalculateViewMatrix();
  projection_ = glm::perspective(glm::radians(camera_.fov()),
                                 Engine::window_aspect(), 0.1f, 100.0f);

  cube_pipeline_.SetMatrix4("transform.view", view_);
  cube_pipeline_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(-1.f, 0.f, 0.f));
  cube_pipeline_.SetMatrix4("transform.model", model_);

  glBindTexture(GL_TEXTURE_2D, container_map_.id);
  cube_pipeline_.DrawMesh(cube_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(1.f, -2.f, 0.f));
  model_ = glm::scale(model_, glm::vec3(0.2f, 0.2f, 0.2f));
  cube_pipeline_.SetMatrix4("transform.model", model_);

  cube_pipeline_.DrawModel(nanosuit_);

  fbo_.UnBind();

  // second pass
  glBindFramebuffer(GL_FRAMEBUFFER, 0);  // back to default
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glClearColor(1.f, 1.f, 1.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);

  post_process_pipeline_.Bind();

  glActiveTexture(GL_TEXTURE0);
  fbo_.BindColorBuffer(0);
  glViewport(0, 0, window_size.x, window_size.y);

  post_process_pipeline_.DrawMesh(screen_quad);
}

void PostProcessing::OnEvent(const SDL_Event& event) { 
    camera_.OnEvent(event);

    switch (event.type) {
      case SDL_KEYDOWN:
        switch (event.key.keysym.scancode) {
          case SDL_SCANCODE_1:
            post_process_pipeline_ = reversed_color_pipeline_;
            break;
          case SDL_SCANCODE_2:
            post_process_pipeline_ = gray_scale_pipeline_;
            break;
          case SDL_SCANCODE_3:
            post_process_pipeline_ = kernel_pipeline_;
            break;
          case SDL_SCANCODE_4:
            post_process_pipeline_ = blur_pipeline_;
            break;
          case SDL_SCANCODE_5:
            post_process_pipeline_ = edge_detection_pipeline_;
            break;
          default:
            break;
        }
        break;
      case SDL_WINDOWEVENT: {
        switch (event.window.event) {
          case SDL_WINDOWEVENT_RESIZED: {
            fbo_.Resize(glm::uvec2(event.window.data1, event.window.data2));
            break;                
          }
          default:
            break;
        }
        break;
      }

      default:
        break;
    }
}