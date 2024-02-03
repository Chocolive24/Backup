#include "ssao_scene.h"
#include "engine.h"
#include "file_utility.h"

#include <iostream>
#include <random>

float lerp(float a, float b, float f) { return a + f * (b - a); }

void SSAO_Scene::Begin() {
  geometry_pipeline_.Begin("data/shaders/ssao/view_space_transform.vert",
                           "data/shaders/ssao/ssao_g_buffer.frag");
  ground_pipeline_.Begin("data/shaders/transform.vert",
                         "data/shaders/texture/texture.frag");
  ssao_pipeline_.Begin("data/shaders/screen_texture.vert",
                       "data/shaders/ssao/ssao.frag");
  ssao_blur_pipeline_.Begin("data/shaders/screen_texture.vert",
                            "data/shaders/ssao/ssao_blur.frag");
  lighting_pipeline_.Begin("data/shaders/screen_texture.vert",
                           "data/shaders/ssao/ssao_lighting.frag");
  ssao_debug_pipeline_.Begin("data/shaders/screen_texture.vert",
                             "data/shaders/texture/texture.frag");

  backpack_.Load("data/models/backpack/backpack.obj");

  cube_.CreateCube();
  screen_quad_.CreateScreenQuad();

  const auto screen_size = Engine::window_size();

  // configure g-buffer framebuffer
  // ------------------------------
  glGenFramebuffers(1, &g_buffer_);
  glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_);

  // position color buffer
  glGenTextures(1, &g_pos_map_);
  glBindTexture(GL_TEXTURE_2D, g_pos_map_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_size.x, screen_size.y, 0,
               GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         g_pos_map_, 0);

  // normal color buffer
  glGenTextures(1, &g_normal_map_);
  glBindTexture(GL_TEXTURE_2D, g_normal_map_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_size.x, screen_size.y, 0,
               GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         g_normal_map_, 0);

  // color + specular color buffer
  glGenTextures(1, &g_albedo_spec_map_);
  glBindTexture(GL_TEXTURE_2D, g_albedo_spec_map_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screen_size.x, screen_size.y, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                         g_albedo_spec_map_, 0);

  // tell OpenGL which color attachments we'll use (of this framebuffer) for
  // rendering
  static constexpr std::array<GLuint, 3> attachments = {
      GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
  glDrawBuffers(3, attachments.data());

  // create and attach depth buffer (renderbuffer)
  glGenRenderbuffers(1, &depth_rbo_);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screen_size.x,
                        screen_size.y);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depth_rbo_);

  // finally check if framebuffer is complete
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "Framebuffer not complete!" << std::endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // generate sample kernel
  // ----------------------
  std::uniform_real_distribution<GLfloat> random_floats(0.0, 1.0);  
  // generates random floats between 0.0 and 1.0
  std::default_random_engine generator;

  for (GLuint i = 0; i < 64; i++) {
    glm::vec3 sample(random_floats(generator) * 2.0 - 1.0,
                     random_floats(generator) * 2.0 - 1.0,
                     random_floats(generator));
    sample = glm::normalize(sample);
    sample *= random_floats(generator);
    float scale = float(i) / 64.0f;

    // scale samples s.t. they're more aligned to center of kernel
    scale = lerp(0.1f, 1.0f, scale * scale);
    sample *= scale;
    ssao_kernel_.push_back(sample);
  }

  std::vector<glm::vec3> ssao_noise;
  for (GLuint i = 0; i < 16; i++) {
    // As the sample kernel is oriented along the positive z direction in tangent space, 
    // we leave the z component at 0.0 so we rotate around the z axis.
    glm::vec3 noise(random_floats(generator) * 2.0 - 1.0,
                    random_floats(generator) * 2.0 - 1.0, 0.0f);
    ssao_noise.push_back(noise);
  }  

  glGenTextures(1, &noise_texture_);
  glBindTexture(GL_TEXTURE_2D, noise_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT,
               ssao_noise.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glGenFramebuffers(1, &ssao_fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo_);

  glGenTextures(1, &ssao_color_buffer_);
  glBindTexture(GL_TEXTURE_2D, ssao_color_buffer_);
  // As the ambient occlusion result is a single grayscale value we'll only need a 
  // texture's red component, so we set the color buffer's internal format to GL_RED.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screen_size.x, screen_size.y, 0, GL_RED,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         ssao_color_buffer_, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glGenFramebuffers(1, &ssao_blur_fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, ssao_blur_fbo_);
  glGenTextures(1, &ssao_blur_color_buffer_);
  glBindTexture(GL_TEXTURE_2D, ssao_blur_color_buffer_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, screen_size.x, screen_size.y, 0, GL_RED,
               GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         ssao_blur_color_buffer_, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Configure shaders.
  // ------------------
  lighting_pipeline_.Bind();
  lighting_pipeline_.SetInt("gPosition", 0);
  lighting_pipeline_.SetInt("gNormal", 1);
  lighting_pipeline_.SetInt("gAlbedo", 2);
  lighting_pipeline_.SetInt("ssao", 3);

  ssao_pipeline_.Bind();
  ssao_pipeline_.SetInt("gPosition", 0);
  ssao_pipeline_.SetInt("gNormal", 1);
  ssao_pipeline_.SetInt("texNoise", 2);

  ssao_blur_pipeline_.Bind();
  ssao_blur_pipeline_.SetInt("ssaoInput", 0);

  camera_.Begin(glm::vec3(0.f, 0.f, 3.f));
  glEnable(GL_DEPTH_TEST);
}

void SSAO_Scene::End() {
  glDisable(GL_DEPTH_TEST);

  geometry_pipeline_.End();
  ground_pipeline_.End();
  ssao_pipeline_.End();
  ssao_blur_pipeline_.End();
  lighting_pipeline_.End();
  ssao_debug_pipeline_.End();

  camera_.End();

  backpack_.Destroy();
  cube_.Destroy();
  screen_quad_.Destroy();
}

void SSAO_Scene::Update(float dt) {
  const auto screen_size = Engine::window_size();

  camera_.Update(dt);
  view_ = camera_.CalculateViewMatrix();
  projection_ = glm::perspective(glm::radians(camera_.fov()),
                                 Engine::window_aspect(), 0.1f, 100.0f);

  // 1. Geometry pass, write scene geometry data into the G-buffer.
  // --------------------------------------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_);
  glClearColor(0.0, 0.0, 0.0, 1.0);  // keep it black so it doesn't leak into g-buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  geometry_pipeline_.Bind();

  geometry_pipeline_.SetMatrix4("transform.projection", projection_);
  geometry_pipeline_.SetMatrix4("transform.view", view_);

  // render the loaded model
  model_ = glm::mat4(1.0f);
  model_ = glm::translate(model_, glm::vec3(0.f, 1.5f, 0.f));
  model_ = glm::rotate(model_, glm::radians(90.f), glm::vec3(-1.f, 0.f, 0.f));
  geometry_pipeline_.SetMatrix4("transform.model", model_);
  geometry_pipeline_.SetMatrix4("normalMatrix", 
      glm::mat4(glm::transpose(glm::inverse(view_ * model_))));

  geometry_pipeline_.DrawModel(backpack_);

  model_ = glm::mat4(1.0f);
  model_ = glm::scale(model_, glm::vec3(25.f, 1.f, 25.f));
  geometry_pipeline_.SetMatrix4("transform.model", model_);
  geometry_pipeline_.SetMatrix4("normalMatrix", 
      glm::mat4(glm::transpose(glm::inverse(view_ * model_))));
  geometry_pipeline_.DrawMesh(cube_);

  // 2. SSAO texture creation pass.
  // ------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo_);
  glClear(GL_COLOR_BUFFER_BIT);
  ssao_pipeline_.Bind();
  // Send kernel + rotation
  for (unsigned int i = 0; i < 64; ++i) {
    ssao_pipeline_.SetVec3("samples[" + std::to_string(i) + "]", ssao_kernel_[i]);
  }
    
  ssao_pipeline_.SetMatrix4("projection", projection_);
  ssao_pipeline_.SetVec2("noiseScale", glm::vec2(screen_size.x / 4.f, 
                                                 screen_size.y / 4.f));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, g_pos_map_);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, g_normal_map_);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, noise_texture_);
  
  ssao_pipeline_.DrawMesh(screen_quad_);

  // 3. SSAO blur pass.
  // ------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER, ssao_blur_fbo_);
  glClear(GL_COLOR_BUFFER_BIT);
  ssao_blur_pipeline_.Bind();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, ssao_color_buffer_);
  ssao_blur_pipeline_.DrawMesh(screen_quad_);

  // 4. Lighting pass with ssao.
  // ------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  lighting_pipeline_.Bind();
  // Send light relevant uniforms.
  glm::vec3 light_pos_view =
      glm::vec3(view_ * glm::vec4(lightPos_, 1.0));
  lighting_pipeline_.SetVec3("light.Position", light_pos_view);
  lighting_pipeline_.SetVec3("light.Color", lightColor_);
  // Update attenuation parameters
  const float linear = 0.09f;
  const float quadratic = 0.032f;
  lighting_pipeline_.SetFloat("light.Linear", linear);
  lighting_pipeline_.SetFloat("light.Quadratic", quadratic);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, g_pos_map_);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, g_normal_map_);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, g_albedo_spec_map_);
  glActiveTexture(GL_TEXTURE3);  // add extra SSAO texture to lighting pass
  glBindTexture(GL_TEXTURE_2D, ssao_blur_color_buffer_);
  
  lighting_pipeline_.DrawMesh(screen_quad_);

  //ssao_debug_pipeline_.Bind();

  //glActiveTexture(GL_TEXTURE4);
  //glBindTexture(GL_TEXTURE_2D, ssao_color_buffer_);
  //ssao_debug_pipeline_.DrawMesh(screen_quad_);
}

void SSAO_Scene::OnEvent(const SDL_Event& event) { camera_.OnEvent(event); }
