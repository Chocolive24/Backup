#include "deferred_shading_scene.h"
#include "engine.h"
#include "file_utility.h"

#include <iostream>
#include <vector>

// TODO:
// Clean and refactor.
// Add HDR.
// Add Bloom effect.

static std::vector<glm::vec3> objectPositions;
static const unsigned int NR_LIGHTS = 32;
static std::vector<glm::vec3> lightPositions;
static std::vector<glm::vec3> lightColors;

void DeferredShadingScene::Begin() {
  geometry_pipeline_.Begin("data/shaders/transform.vert",
                           "data/shaders/deferred_shading/g_buffer.frag");
  lighting_pipeline_.Begin("data/shaders/screen_texture.vert",
                           "data/shaders/deferred_shading/deferred_shading.frag");
  light_box_pipeline_.Begin("data/shaders/transform.vert",
                            "data/shaders/light_box.frag");

  backpack_.Load("data/models/backpack/backpack.obj");
  cube_.CreateCube();
  screen_quad_.CreateScreenQuad();

  const auto screen_size = Engine::window_size();

  CreateGBuffer(screen_size);

  objectPositions.push_back(glm::vec3(-3.0, -0.5, -3.0));
  objectPositions.push_back(glm::vec3(0.0, -0.5, -3.0));
  objectPositions.push_back(glm::vec3(3.0, -0.5, -3.0));
  objectPositions.push_back(glm::vec3(-3.0, -0.5, 0.0));
  objectPositions.push_back(glm::vec3(0.0, -0.5, 0.0));
  objectPositions.push_back(glm::vec3(3.0, -0.5, 0.0));
  objectPositions.push_back(glm::vec3(-3.0, -0.5, 3.0));
  objectPositions.push_back(glm::vec3(0.0, -0.5, 3.0));
  objectPositions.push_back(glm::vec3(3.0, -0.5, 3.0));

  // lighting info
  // -------------
  srand(13);
  for (unsigned int i = 0; i < NR_LIGHTS; i++) {
    // calculate slightly random offsets
    float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
    float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
    float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
    lightPositions.push_back(glm::vec3(xPos, yPos, zPos));
    // also calculate random color
    float rColor = static_cast<float>(((rand() % 100) / 200.0f) +
                                      0.5);  // between 0.5 and 1.)
    float gColor = static_cast<float>(((rand() % 100) / 200.0f) +
                                      0.5);  // between 0.5 and 1.)
    float bColor = static_cast<float>(((rand() % 100) / 200.0f) +
                                      0.5);  // between 0.5 and 1.)
    lightColors.push_back(glm::vec3(rColor, gColor, bColor));
  }

  // shader configuration
  // --------------------
  lighting_pipeline_.Bind();
  lighting_pipeline_.SetInt("gPosition", 0);
  lighting_pipeline_.SetInt("gNormal", 1);
  lighting_pipeline_.SetInt("gAlbedoSpec", 2);

  camera_.Begin(glm::vec3(0.f, 0.f, 3.f));
  glEnable(GL_DEPTH_TEST);
}

void DeferredShadingScene::End() {
  geometry_pipeline_.End();
  lighting_pipeline_.End();
  light_box_pipeline_.End();
  camera_.End();
  backpack_.Destroy();
  cube_.Destroy();
  screen_quad_.Destroy();
}

void DeferredShadingScene::Update(float dt) {
  const auto screen_size = Engine::window_size();

  camera_.Update(dt);
  view_ = camera_.CalculateViewMatrix();
  projection_ = camera_.CalculateProjectionMatrix(Engine::window_aspect());

  // 1. geometry pass: render all geometric/color data to g-buffer
  glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_);
  glClearColor(0.0, 0.0, 0.0, 1.0);  // keep it black so it doesn't leak into g-buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  geometry_pipeline_.Bind();

  geometry_pipeline_.SetMatrix4("transform.projection", projection_);
  geometry_pipeline_.SetMatrix4("transform.view", view_);

  for (unsigned int i = 0; i < objectPositions.size(); i++) {
    model_ = glm::mat4(1.0f);
    model_ = glm::translate(model_, objectPositions[i]);
    model_ = glm::scale(model_, glm::vec3(0.25f));
    geometry_pipeline_.SetMatrix4("transform.model", model_);
    geometry_pipeline_.SetMatrix4("normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));

    geometry_pipeline_.DrawModel(backpack_);
  }

  // 2. lighting pass: use g-buffer to calculate the scene's lighting
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  lighting_pipeline_.Bind();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, pos_map_);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, normal_map_);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, albedo_spec_map_);

  // send light relevant uniforms
  for (unsigned int i = 0; i < lightPositions.size(); i++) {
    lighting_pipeline_.SetVec3("lights[" + std::to_string(i) + "].position",
                               lightPositions[i]);
    lighting_pipeline_.SetVec3("lights[" + std::to_string(i) + "].color",
                               lightColors[i]);

    // update attenuation parameters and calculate radius
    const float constant = 1.0f;  // note that we don't send this to the shader,
                                  // we assume it is always 1.0 (in our case)
    const float linear = 0.7f;
    const float quadratic = 1.8f;
    lighting_pipeline_.SetFloat("lights[" + std::to_string(i) + "].linear",
                                linear);
    lighting_pipeline_.SetFloat("lights[" + std::to_string(i) + "].quadratic",
                                quadratic);

    // then calculate radius of light volume/sphere
    const float maxBrightness = std::fmaxf(
        std::fmaxf(lightColors[i].r, lightColors[i].g), lightColors[i].b);
    float radius = (-linear + std::sqrt(linear * linear - 4 * quadratic *
                   (constant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * quadratic);

    lighting_pipeline_.SetFloat("lights[" + std::to_string(i) + "].radius",
                                radius);
  }

  lighting_pipeline_.SetVec3("viewPos", camera_.position());

  // finally render quad
  lighting_pipeline_.DrawMesh(screen_quad_);

  glBindFramebuffer(GL_READ_FRAMEBUFFER, g_buffer_);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);  // write to default framebuffer
  glBlitFramebuffer(0, 0, screen_size.x, screen_size.y, 0, 0, screen_size.x,
                    screen_size.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Render all light cubes with forward rendering.
  light_box_pipeline_.Bind();

  light_box_pipeline_.SetMatrix4("transform.projection", projection_);
  light_box_pipeline_.SetMatrix4("transform.view", view_);

  for (unsigned int i = 0; i < lightPositions.size(); i++) {
    model_ = glm::mat4(1.0f);
    model_ = glm::translate(model_, lightPositions[i]);
    model_ = glm::scale(model_, glm::vec3(0.25f));
    light_box_pipeline_.SetMatrix4("transform.model", model_);
    light_box_pipeline_.SetVec3("lightColor", lightColors[i]);
    
    light_box_pipeline_.DrawMesh(cube_);
  }

}

void DeferredShadingScene::OnEvent(const SDL_Event& event) { camera_.OnEvent(event); }


void DeferredShadingScene::CreateGBuffer(const glm::vec2& screen_size) {
  // configure g-buffer framebuffer
  // ------------------------------
  glGenFramebuffers(1, &g_buffer_);
  glBindFramebuffer(GL_FRAMEBUFFER, g_buffer_);

  // position color buffer
  glGenTextures(1, &pos_map_);
  glBindTexture(GL_TEXTURE_2D, pos_map_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_size.x, screen_size.y, 0,
               GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         pos_map_, 0);

  // normal color buffer
  glGenTextures(1, &normal_map_);
  glBindTexture(GL_TEXTURE_2D, normal_map_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_size.x, screen_size.y, 0,
               GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         normal_map_, 0);

  // color + specular color buffer
  glGenTextures(1, &albedo_spec_map_);
  glBindTexture(GL_TEXTURE_2D, albedo_spec_map_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screen_size.x, screen_size.y, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
                         albedo_spec_map_, 0);

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
}
