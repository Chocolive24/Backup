#include "shadow_scene.h"
#include "engine.h"

#include <iostream>

// instancing du model pour voir + de shadow o.o

void ShadowScene::Begin() {
  scene_pipeline_.Begin("data/shaders/shadow/transform_shadow.vert",
                        "data/shaders/shadow/shadow.frag");
  normal_map_scene_pipe_.Begin("data/shaders/shadow/tangent_transform_shadow.vert",
                        "data/shaders/shadow/tangent_shadow.frag");
  hdr_pipeline_.Begin("data/shaders/screen_texture.vert",
                      "data/shaders/hdr/hdr.frag");
  shadow_mapping_pipe_.Begin("data/shaders/shadow/simple_depth.vert",
                            "data/shaders/shadow/simple_depth.frag");
  shadow_map_debug_pipe_.Begin("data/shaders/screen_texture.vert",
                               "data/shaders/shadow/depth_map_rendering.frag");

  constexpr bool hdr = true;

  cube_.CreateCube();
  screen_quad_.CreateScreenQuad();

  wood_map_ = LoadTexture("data/textures/wood.png", GL_CLAMP_TO_EDGE, 
                          GL_LINEAR, hdr);
  nanosuit_.Load("data/models/backpack/backpack.obj", hdr, false);

  const auto window_size = Engine::window_size();

  // Generate HDR framebuffer
  glGenFramebuffers(1, &hdr_fbo_);
  glGenTextures(1, &hdr_color_buffer_);

  // Bind the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);

  // Bind the texture
  glBindTexture(GL_TEXTURE_2D, hdr_color_buffer_);
  
  // Specify texture storage and parameters
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_size.x, window_size.y, 0,
                  GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  // Attach texture to framebuffer
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                        GL_TEXTURE_2D, hdr_color_buffer_, 0);

  // Create depth buffer (renderbuffer)
  glGenRenderbuffers(1, &depth_rbo_);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_size.x,
                        window_size.y);

  // Attach renderbuffer to framebuffer
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depth_rbo_);

  // Check framebuffer completeness
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Framebuffer not complete!" << std::endl;
  }

  // Unbind the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);




  // configure depth map FBO
  // -----------------------
  glGenFramebuffers(1, &depth_map_fbo_);
  // create depth texture
  glGenTextures(1, &depth_map_);
  glBindTexture(GL_TEXTURE_2D, depth_map_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, kShadowMapWidth_,
               kShadowMapHeight_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  constexpr std::array<float, 4> border_colors_ = {1.0f, 1.0f, 1.0f, 1.0f};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_colors_.data());

  // attach depth texture as FBO's depth buffer
  glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         depth_map_, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // shader configuration
  // --------------------
  scene_pipeline_.Bind();
  scene_pipeline_.SetInt("diffuseTexture", 0);
  scene_pipeline_.SetInt("shadowMap", 1);

  normal_map_scene_pipe_.Bind();
  normal_map_scene_pipe_.SetInt("shadowMap", 1);

  shadow_map_debug_pipe_.Bind();
  shadow_map_debug_pipe_.SetInt("depthMap", 0);

  glEnable(GL_DEPTH_TEST);

  camera_.Begin(glm::vec3(0.f, 0.f, 3.f));
}

void ShadowScene::End() {
  // Unload program/pipeline
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  Engine::set_clear_color(glm::vec3(0.f));

  camera_.End();

  scene_pipeline_.End();
  normal_map_scene_pipe_.End();
  hdr_pipeline_.End();
  shadow_mapping_pipe_.End();
  shadow_map_debug_pipe_.End();

  nanosuit_.Destroy();
  cube_.Destroy();
  screen_quad_.Destroy();

  glDeleteTextures(1, &wood_map_);
}

void ShadowScene::Update(float dt) {
  // 1. render depth of scene to texture (from light's perspective)
  // --------------------------------------------------------------
  // render scene from light's point of view
  glViewport(0, 0, kShadowMapWidth_, kShadowMapHeight_);
  glBindFramebuffer(GL_FRAMEBUFFER, depth_map_fbo_);
  glClear(GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(shadow_map_cull_face);

  static float time = 0.f;
  time += dt;

  //light_pos_.x = cos(time) * 5.f;
  //light_pos_.z = sin(time) * 5.f;

  light_dir_ = glm::normalize(glm::vec3(0) - light_pos_);

  glm::mat4 lightProjection, lightView;
  float near_plane = 1.0f, far_plane = 20.f;
  lightProjection =
      glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
  lightView = glm::lookAt(light_pos_, light_pos_ + light_dir_,
                          glm::vec3(0.0, 1.0, 0.0));
  light_space_matrix_ = lightProjection * lightView;

  shadow_mapping_pipe_.Bind();
  shadow_mapping_pipe_.SetMatrix4("lightSpaceMatrix", light_space_matrix_);

  DrawScene(ShadowPipelineType::kShadowMapping);

  // Render scene with shadow calculation in HDR color buffer.
  const auto window_size = Engine::window_size();
  glViewport(0, 0, window_size.x, window_size.y);

  glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  camera_.Update(dt);
  view_ = camera_.CalculateViewMatrix();
  projection_ = camera_.CalculateProjectionMatrix(Engine::window_aspect());

  normal_map_scene_pipe_.Bind();

  // Bind the shadow map.
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_2D, depth_map_);
  normal_map_scene_pipe_.SetInt("shadowMap", 3);

  // Common vert shader uniforms.
  normal_map_scene_pipe_.SetMatrix4("transform.view", view_);
  normal_map_scene_pipe_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(-2.0f, 1.75f, -3.0));
  model_ = glm::scale(model_, glm::vec3(1.f));
  normal_map_scene_pipe_.SetMatrix4("transform.model", model_);
  normal_map_scene_pipe_.SetMatrix4("normalMatrix",
                              glm::mat4(glm::transpose(glm::inverse(model_))));

  // Normal mapping uniforms.
  normal_map_scene_pipe_.SetVec3("viewPos", camera_.position());
  normal_map_scene_pipe_.SetVec3("lightDir", light_dir_);

  // Shadow uniforms.
  normal_map_scene_pipe_.SetMatrix4("lightSpaceMatrix", light_space_matrix_);

  normal_map_scene_pipe_.DrawModel(nanosuit_);

  scene_pipeline_.Bind();

  scene_pipeline_.SetMatrix4("transform.view", view_);
  scene_pipeline_.SetMatrix4("transform.projection", projection_);
  scene_pipeline_.SetMatrix4("lightSpaceMatrix", light_space_matrix_);

  scene_pipeline_.SetVec3("viewPos", camera_.position());
  scene_pipeline_.SetVec3("lightDir", light_dir_);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, depth_map_);
  scene_pipeline_.SetInt("shadowMap", 1);

  DrawScene(ShadowPipelineType::kScene);

  // Render the scene with HDR and gamma correction:
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  hdr_pipeline_.Bind();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, hdr_color_buffer_);

  hdr_pipeline_.SetFloat("exposure", 1.f);

  hdr_pipeline_.DrawMesh(screen_quad_);

  // render Depth map to quad for visual debugging
  // ---------------------------------------------

  if (debug_depth_map_) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    shadow_map_debug_pipe_.Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth_map_);

    shadow_map_debug_pipe_.DrawMesh(screen_quad_);
  }
}

void ShadowScene::DrawScene(ShadowPipelineType pipe_type) {

  bool use_normal_mapping = false;
  bool use_normal_mat = false;

  switch (pipe_type) { 
    case ShadowPipelineType::kScene:
      current_pipe_ = &scene_pipeline_;
      use_normal_mat = true;
      use_normal_mapping = true;
      break;
    case ShadowPipelineType::kShadowMapping:
      current_pipe_ = &shadow_mapping_pipe_;
      break;
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, wood_map_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(0.0f, 1.5f, 0.0));
  current_pipe_->SetMatrix4("transform.model", model_);

  if (use_normal_mat) {
    current_pipe_->SetMatrix4("normalMatrix",
                              glm::mat4(glm::transpose(glm::inverse(model_))));
  }

  current_pipe_->DrawMesh(cube_);

  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, glm::vec3(2.0f, 0.5f, 1.0));
  model_ = glm::scale(model_, glm::vec3(0.5f));
  current_pipe_->SetMatrix4("transform.model", model_);

  if (use_normal_mat) {
    current_pipe_->SetMatrix4("normalMatrix",
                              glm::mat4(glm::transpose(glm::inverse(model_))));
  }

  current_pipe_->DrawMesh(cube_);

  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, glm::vec3(-1.0f, 1.0f, 2.0));
  model_ = glm::rotate(model_, glm::radians(60.0f),
                       glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
  model_ = glm::scale(model_, glm::vec3(0.5));
  current_pipe_->SetMatrix4("transform.model", model_);

  if (use_normal_mat) {
    current_pipe_->SetMatrix4("normalMatrix",
                              glm::mat4(glm::transpose(glm::inverse(model_))));
  }

  current_pipe_->DrawMesh(cube_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(0.f, -0.5f, 0.f));
  model_ = glm::scale(model_, glm::vec3(25.f, 1.f, 25.f));
  model_ = glm::rotate(model_, glm::radians(90.f), glm::vec3(1, 0, 0));
  current_pipe_->SetMatrix4("transform.model", model_);

  if (use_normal_mat) {
    current_pipe_->SetMatrix4("normalMatrix",
                              glm::mat4(glm::transpose(glm::inverse(model_))));
  }

  current_pipe_->DrawMesh(cube_);

  if (use_normal_mapping) {
    return;
  }

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(-2.0f, 1.75f, -3.0));
  model_ = glm::scale(model_, glm::vec3(1.f));
  current_pipe_->SetMatrix4("transform.model", model_);

  if (use_normal_mat) {
    current_pipe_->SetMatrix4("normalMatrix",
                              glm::mat4(glm::transpose(glm::inverse(model_))));
  }

  current_pipe_->DrawModel(nanosuit_);
}

void ShadowScene::OnEvent(const SDL_Event& event) { 
    camera_.OnEvent(event);

    switch (event.type) {
    case SDL_KEYDOWN:
      switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_E:
          debug_depth_map_ = !debug_depth_map_;
          break;
        case SDL_SCANCODE_C:
          shadow_map_cull_face = shadow_map_cull_face == GL_BACK ? GL_FRONT : GL_BACK;
          break;
      }
      break;

    default:
      break;
    }
}
