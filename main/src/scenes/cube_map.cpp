#include "cube_map.h"
#include "engine.h"

#include <array>

void CubeMap::Begin() {
  skybox_pipeline_.Begin("data/shaders/cube_map.vert",
                         "data/shaders/cube_map.frag");
  reflection_pipeline_.Begin("data/shaders/transform.vert",
                             "data/shaders/reflection.frag");
  refraction_pipeline_.Begin("data/shaders/transform.vert",
                             "data/shaders/refraction.frag");

  std::array<std::string, 6> faces = {
      "data/textures/cube_maps/sky_and_water/right.jpg",
      "data/textures/cube_maps/sky_and_water/left.jpg",
      "data/textures/cube_maps/sky_and_water/top.jpg",
      "data/textures/cube_maps/sky_and_water/bottom.jpg",
      "data/textures/cube_maps/sky_and_water/front.jpg",
      "data/textures/cube_maps/sky_and_water/back.jpg",
  };

  cube_map_ = LoadCubeMap(faces, GL_CLAMP_TO_EDGE, GL_LINEAR);
  monkey_map_ = LoadTexture("data/textures/monkey.png", GL_CLAMP_TO_EDGE, GL_LINEAR);

  backpack_.Load("data/models/backpack/backpack.obj");

  skybox_.CreateCubeMap();
  cube_.CreateCube();

  camera_.Begin(glm::vec3(0.f, 0.f, 5.f));

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
}

void CubeMap::End() {
  // Unload program/pipeline
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  skybox_pipeline_.End();
  reflection_pipeline_.End();
  refraction_pipeline_.End();

  glDeleteTextures(1, &cube_map_);
  glDeleteTextures(1, &monkey_map_);

  backpack_.Destroy();

  skybox_.Destroy();
  cube_.Destroy();

  camera_.End();
}

void CubeMap::Update(float dt) {
  camera_.Update(dt);

  view_ = camera_.CalculateViewMatrix();
  projection_ = glm::perspective(glm::radians(45.f), Engine::window_aspect(),
                                 0.1f, 100.f);

  glDepthFunc(GL_LESS);
  glCullFace(GL_BACK);
  reflection_pipeline_.Bind();

  reflection_pipeline_.SetMatrix4("transform.view", view_);
  reflection_pipeline_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, monkey_cube_pos_);
  reflection_pipeline_.SetMatrix4("transform.model", model_);

  reflection_pipeline_.SetMatrix4("normalMatrix", 
                                  glm::mat4(glm::transpose(glm::inverse(model_))));
  reflection_pipeline_.SetVec3("camera_pos", camera_.position());

  reflection_pipeline_.SetFloat("reflection_factor", reflection_factor);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map_);
  reflection_pipeline_.SetInt("cube_map", 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, monkey_map_);
  reflection_pipeline_.SetInt("material.texture_diffuse1", 1);

  reflection_pipeline_.DrawMesh(cube_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, backpack_pos_);
  reflection_pipeline_.SetMatrix4("transform.model", model_);

  reflection_pipeline_.DrawModel(backpack_, 1);

  // Refraction pipeline.
  refraction_pipeline_.Bind();

  refraction_pipeline_.SetMatrix4("transform.view", view_);
  refraction_pipeline_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, -monkey_cube_pos_);
  refraction_pipeline_.SetMatrix4("transform.model", model_);

  refraction_pipeline_.SetMatrix4(
      "normalMatrix", glm::mat4(glm::transpose(glm::inverse(model_))));
  refraction_pipeline_.SetVec3("camera_pos", camera_.position());

  refraction_pipeline_.SetFloat("reflection_factor", reflection_factor);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map_);
  refraction_pipeline_.SetInt("cube_map", 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, monkey_map_);
  refraction_pipeline_.SetInt("material.texture_diffuse1", 1);

  refraction_pipeline_.DrawMesh(cube_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, -backpack_pos_);
  refraction_pipeline_.SetMatrix4("transform.model", model_);

  refraction_pipeline_.SetFloat("refraction_ratio", 1.f / refractive_index);
  refraction_pipeline_.SetFloat("reflection_factor", reflection_factor);

  refraction_pipeline_.DrawModel(backpack_, 1);

  glDepthFunc(GL_LEQUAL);
  // Cull the front faces to be able to see the back faces of the cube_map.
  glCullFace(GL_FRONT); 
  skybox_pipeline_.Bind();

  skybox_pipeline_.SetMatrix4("transform.view", glm::mat4(glm::mat3(view_)));
  skybox_pipeline_.SetMatrix4("transform.projection", projection_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map_);
  skybox_pipeline_.DrawMesh(skybox_);
}

void CubeMap::OnEvent(const SDL_Event& event) { 
  camera_.OnEvent(event);
  switch (event.type) {
    case SDL_KEYDOWN:
      switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_Q:
          reflection_factor -= 0.1f;
          if (reflection_factor <= 0.f) reflection_factor = 0.f;
          break;
        case SDL_SCANCODE_E:
          reflection_factor += 0.1f;
          if (reflection_factor >= 1.f) reflection_factor = 1.f;
          break;
        case SDL_SCANCODE_X:
          refractive_index -= 0.1f;
          if (refractive_index <= 1.f) refractive_index = 1.f;
          break;
        case SDL_SCANCODE_C:
          refractive_index += 0.1f;
          if (refractive_index >= 2.42f) refractive_index = 2.42f;
          break;
        default:
          break;
      }
      break;

    default:
      break;
  }
}