#include "bloom_scene.h"
#include "engine.h"

void BloomScene::Begin() {
  container_cube_pipe_.Begin("data/shaders/transform.vert",
                             "data/shaders/hdr/hdr_lighting.frag");
  light_box_pipe_.Begin("data/shaders/transform.vert",
                         "data/shaders/light_box.frag");
  gaussian_blur_pipe_.Begin("data/shaders/screen_texture.vert",
                            "data/shaders/gaussian_blur.frag");
  bloom_pipeline_.Begin("data/shaders/screen_texture.vert",
                        "data/shaders/hdr/bloom.frag");

  cube_.CreateCube();
  ground_quad_.CreateCube();
  screen_quad_.CreateScreenQuad();

  container_map_ = LoadTexture("data/textures/container_diffuse.png",
                        GL_CLAMP_TO_EDGE, GL_LINEAR, true);
  wood_map_ = LoadTexture("data/textures/wood.png", GL_CLAMP_TO_EDGE,
                        GL_LINEAR, true);

  const auto window_size = Engine::window_size();

  // The HDR framebuffer as two render targets (2 color buffers).
  // The first one contains the scene.
  // The second one contains only the brightness colors to apply bloom on it. 
  CreateHdrFrameBuffer(window_size);

  // Ping pong framebuffers are use to apply the gaussian blur on the 
  // brightness texture.
  CreatePingPongFrameBuffers(window_size);

  // shader configuration
  container_cube_pipe_.Bind();
  container_cube_pipe_.SetInt("diffuseTexture", 0);
  gaussian_blur_pipe_.Bind();
  gaussian_blur_pipe_.SetInt("image", 0);
  bloom_pipeline_.Bind();
  bloom_pipeline_.SetInt("scene", 0);
  bloom_pipeline_.SetInt("bloomBlur", 1);

  camera_.Begin(glm::vec3(0.f, 0.f, 3.f));
}

void BloomScene::CreateHdrFrameBuffer(const glm::vec2& window_size) {
  // Generate framebuffer and textures
  glGenFramebuffers(1, &hdr_fbo_);
  glGenTextures(2, color_buffers_.data());

  // Bind the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);

  // Create the two color buffers.
  for (int i = 0; i < color_buffers_.size(); i++) {
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, color_buffers_[i]);

    // Specify texture storage and parameters
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_size.x, window_size.y, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                           GL_TEXTURE_2D, color_buffers_[i], 0);
  }

  // Create depth buffer (renderbuffer)
  glGenRenderbuffers(1, &depth_rbo_);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_size.x,
                        window_size.y);

  // Attach renderbuffer to framebuffer
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depth_rbo_);

  // Specify the color attachments to be drawn
  constexpr std::array<GLuint, 2> attachments = {GL_COLOR_ATTACHMENT0,
                                                 GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, attachments.data());

  // Check framebuffer completeness
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "Framebuffer not complete!" << std::endl;
  }

  // Unbind the framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BloomScene::CreatePingPongFrameBuffers(const glm::vec2& window_size) {
  glGenFramebuffers(2, ping_pong_fbo_.data());
  glGenTextures(2, ping_pong_color_buffers_.data());

  for (int i = 0; i < ping_pong_fbo_.size(); i++) {
    glBindFramebuffer(GL_FRAMEBUFFER, ping_pong_fbo_[i]);
    glBindTexture(GL_TEXTURE_2D, ping_pong_color_buffers_[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window_size.x, window_size.y, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           ping_pong_color_buffers_[i], 0);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void BloomScene::End() {
  // Unload program/pipeline
  container_cube_pipe_.End();
  light_box_pipe_.End();
  gaussian_blur_pipe_.End();
  bloom_pipeline_.End();

  cube_.Destroy();
  ground_quad_.Destroy();
  screen_quad_.Destroy();

  glDeleteTextures(1, &container_map_);
  glDeleteTextures(1, &wood_map_);
  glDeleteTextures(2, color_buffers_.data());

  camera_.End();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
}

void BloomScene::Update(float dt) {
  const auto aspect = Engine::window_aspect();

  camera_.Update(dt);
  view_ = camera_.CalculateViewMatrix();
  projection_ = camera_.CalculateProjectionMatrix(aspect);

  // First pass, one framebuffer, two render targets:
  // Render target 1 : the scene.
  // Render target 2 : the brightness parts of the scene.
  glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo_);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);

  container_cube_pipe_.Bind();

  // Constant uniforms for each vertex.
  container_cube_pipe_.SetMatrix4("transform.view", view_);
  container_cube_pipe_.SetMatrix4("transform.projection", projection_);
  container_cube_pipe_.SetVec3("viewPos", camera_.position());

  for (int i = 0; i < light_positions_.size(); i++) {
    container_cube_pipe_.SetVec3("lights[" + std::to_string(i) + "].Position",
                                 light_positions_[i]);
    container_cube_pipe_.SetVec3("lights[" + std::to_string(i) + "].Color",
                                 light_colors_[i]);
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, container_map_);
  DrawContainerCubes();

  glBindTexture(GL_TEXTURE_2D, wood_map_);
  DrawGroundCube();

  light_box_pipe_.Bind();

  // Constant uniforms for each vertex.
  light_box_pipe_.SetMatrix4("transform.view", view_);
  light_box_pipe_.SetMatrix4("transform.projection", projection_);

  DrawLightBoxes();

  // Second pass, two framebuffers, 1 render target per framebuffer.
  // This pass blur the brightness texture by swapping between two framebuffers
  // (drawing between 2 color buffers).
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  gaussian_blur_pipe_.Bind();
  bool horizontal = true, first_iteration = true;
  int amount = 10;

  for (int i = 0; i < amount; i++) {
    glBindFramebuffer(GL_FRAMEBUFFER, ping_pong_fbo_[horizontal]);
    gaussian_blur_pipe_.SetInt("horizontal", horizontal);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, first_iteration
                                     ? color_buffers_[1]
                                     : ping_pong_color_buffers_[!horizontal]);

    gaussian_blur_pipe_.DrawMesh(screen_quad_);

    horizontal = !horizontal;

    if (first_iteration) {
      first_iteration = false;
    }
  }

  // Third pass, 1 framebuffer, 1 render target.
  // Draw the scene texture with gamma correction and HDR and draw additively
  // the bloom texture.
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  bloom_pipeline_.Bind();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, color_buffers_[0]);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, ping_pong_color_buffers_[!horizontal]);
  bloom_pipeline_.SetInt("hdr", hdr_);
  bloom_pipeline_.SetFloat("exposure", exposure_);

  bloom_pipeline_.DrawMesh(screen_quad_);
}

void BloomScene::DrawContainerCubes() {
  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, glm::vec3(0.0f, 1.5f, 0.0));
  container_cube_pipe_.SetMatrix4("transform.model", model_);
  container_cube_pipe_.DrawMesh(cube_);

  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, glm::vec3(2.0f, 0.0f, 1.0));
  container_cube_pipe_.SetMatrix4("transform.model", model_);
  container_cube_pipe_.SetMatrix4("normalMatrix", 
      glm::mat4(glm::transpose(glm::inverse(model_))));
  container_cube_pipe_.DrawMesh(cube_);

  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, glm::vec3(-1.0f, -1.0f, 2.0));
  model_ = glm::rotate(model_, glm::radians(60.0f),
                       glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
  container_cube_pipe_.SetMatrix4("transform.model", model_);
  container_cube_pipe_.SetMatrix4("normalMatrix",
                       glm::mat4(glm::transpose(glm::inverse(model_))));
  container_cube_pipe_.DrawMesh(cube_);

  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, glm::vec3(0.0f, 2.7f, 4.0));
  model_ = glm::scale(model_, glm::vec3(1.25f));
  model_ = glm::rotate(model_, glm::radians(23.0f),
                       glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
  container_cube_pipe_.SetMatrix4("transform.model", model_);
  container_cube_pipe_.SetMatrix4("normalMatrix",
                       glm::mat4(glm::transpose(glm::inverse(model_))));
  container_cube_pipe_.DrawMesh(cube_);

  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, glm::vec3(-2.0f, 1.0f, -3.0));
  model_ = glm::rotate(model_, glm::radians(124.0f),
                       glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
  container_cube_pipe_.SetMatrix4("transform.model", model_);
  container_cube_pipe_.SetMatrix4("normalMatrix",
                       glm::mat4(glm::transpose(glm::inverse(model_))));
  container_cube_pipe_.DrawMesh(cube_);

  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, glm::vec3(-3.0f, 0.0f, 0.0));
  container_cube_pipe_.SetMatrix4("transform.model", model_);
  container_cube_pipe_.SetMatrix4("normalMatrix",
                       glm::mat4(glm::transpose(glm::inverse(model_))));
  container_cube_pipe_.DrawMesh(cube_);
}

void BloomScene::DrawGroundCube() {
  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, glm::vec3(0.f, -1.f, 0.f));
  model_ = glm::scale(model_, glm::vec3(12.5f, 0.5f, 12.5f));
  model_ = glm::rotate(model_, glm::radians(90.f), glm::vec3(1, 0, 0));

  container_cube_pipe_.SetMatrix4("transform.model", model_);
  container_cube_pipe_.SetMatrix4(
      "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

  container_cube_pipe_.DrawMesh(ground_quad_);
}

void BloomScene::DrawLightBoxes() {
  for (int i = 0; i < light_positions_.size(); i++) {
    model_ = glm::mat4(1.f);
    model_ = glm::translate(model_, light_positions_[i]);
    model_ = glm::scale(model_, glm::vec3(0.75f));
    light_box_pipe_.SetMatrix4("transform.model", model_);

    light_box_pipe_.SetVec3("lightColor", light_colors_[i]);
    light_box_pipe_.DrawMesh(cube_);
  }
}

void BloomScene::OnEvent(const SDL_Event& event) {
  camera_.OnEvent(event);

  switch (event.type) {
    case SDL_KEYDOWN:
      switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_H:
          hdr_ = !hdr_;
          break;
        case SDL_SCANCODE_E:
          exposure_ += 0.1f;
          break;
        case SDL_SCANCODE_R:
          if (exposure_ > 0) {
            exposure_ -= 0.1f;
          } 
          else {
            exposure_ = 0.0f;
          }
          break;
      }
      break;

    default:
      break;
  }
}
