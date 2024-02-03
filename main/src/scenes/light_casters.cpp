#include "light_casters.h"
#include "file_utility.h"

#include <iostream>

void LightCasters::Begin() {
  dir_light_pipeline_.Begin("data/shaders/transform.vert",
                            "data/shaders/directional_light.frag");
  point_light_pipeline_.Begin("data/shaders/transform.vert",
                              "data/shaders/point_light.frag");
  flashlight_pipeline_.Begin("data/shaders/transform.vert",
                              "data/shaders/spot_light.frag");

  lamp_pipeline_.Begin("data/shaders/transform.vert",
                       "data/shaders/phong_light/lamp.frag");
  // load texture
  material_.diffuse_map.Create("data/textures/container_diffuse.png", GL_REPEAT, GL_LINEAR);
  material_.specular_map.Create("data/textures/container_specular.png", GL_REPEAT, GL_LINEAR);

  pipeline_ = dir_light_pipeline_;
  current_light_type_ = LightType::kDirectional;

  cube_.CreateCube();
  camera_.Begin(glm::vec3(0.0f, 0.0f, 3.f));

  Engine::set_clear_color(glm::vec3(0.2, 0.2, 0.5));

  glEnable(GL_DEPTH_TEST);
}

void LightCasters::End() {
  // Unload program/pipeline
  pipeline_.End();
  dir_light_pipeline_.End();
  point_light_pipeline_.End();
  flashlight_pipeline_.End();
  lamp_pipeline_.End();

  cube_.Destroy();
  glDeleteTextures(1, &material_.diffuse_map.id);
  glDeleteTextures(1, &material_.specular_map.id);

  camera_.End();

  Engine::set_clear_color(glm::vec3(0));
}

void LightCasters::Update(float dt) {
  pipeline_.Bind();

  camera_.Update(dt);
  projection_ = glm::perspective(glm::radians(camera_.fov()),
                                 Engine::window_aspect(), 0.1f, 100.0f);

  pipeline_.SetMatrix4("transform.projection", projection_);
  pipeline_.SetMatrix4("transform.view", camera_.CalculateViewMatrix());

  pipeline_.SetVec3("viewPos", glm::vec3(camera_.position()));

  if (rotation_factor_ >= 360.f) {
    rotation_factor_ = 0.f;
  }

  rotation_factor_ += dt;

  for (std::size_t i = 0; i < cube_positions_.size(); i++) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, cube_positions_[i]);
    model = glm::scale(model, glm::vec3(0.75, 0.75, 0.75));
    const auto angle = rotation_factor_ * (i * 0.5f);
    model = glm::rotate(model, angle, glm::vec3(1.0f, 0.3f, 0.5f));
    pipeline_.SetMatrix4("transform.model", model);

    pipeline_.SetMatrix4("normalMatrix",
                         glm::mat4(glm::transpose(glm::inverse(model))));

    if (must_appply_textures_) {
      pipeline_.SetBool("has_textures", 1);

      pipeline_.SetInt("material.diffuse_map", 0);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, material_.diffuse_map.id);

      pipeline_.SetInt("material.specular_map", 1);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, material_.specular_map.id);
    } else {
      pipeline_.SetBool("has_textures", 0);
      pipeline_.SetVec3("material.diffuse", glm::vec3(1.f, 0.0f, 0.0f));
      pipeline_.SetVec3("material.specular", glm::vec3(0.5f));
    }

    pipeline_.SetFloat("material.shininess",
                       static_cast<int>(material_.shininess));

    pipeline_.SetBool("use_blinn_phong", use_blinn_phong_);

    switch (current_light_type_) {
      case LightType::kDirectional:
        pipeline_.SetLight("dir_light", dir_light_);
        break;
      case LightType::kPoint:
        pipeline_.SetLight("point_light", point_light_);
        break;
      case LightType::kSpot:
        flashlight_.position = camera_.position();
        flashlight_.direction = camera_.front();
        pipeline_.SetLight("spot_light", flashlight_);
        break;
      default:
        break;
    }

    pipeline_.DrawMesh(cube_);
  }

  if (current_light_type_ == LightType::kPoint)
  {
    lamp_pipeline_.Bind();

    lamp_pipeline_.SetMatrix4("transform.projection", projection_);
    lamp_pipeline_.SetMatrix4("transform.view", camera_.CalculateViewMatrix());
    lamp_pipeline_.SetVec3("viewPos", glm::vec3(camera_.position()));

    glm::mat4 lamp_model = glm::mat4(1.0f);
    lamp_model = glm::translate(lamp_model, point_light_.position);
    lamp_model = glm::scale(lamp_model, glm::vec3(0.5));
    lamp_pipeline_.SetMatrix4("transform.model", lamp_model);

    lamp_pipeline_.SetVec3("light_color", glm::vec3(point_light_.diffuse));

    lamp_pipeline_.DrawMesh(cube_);
  }
}

void LightCasters::OnEvent(const SDL_Event& event) { 
    camera_.OnEvent(event); 

    switch (event.type) {
    case SDL_KEYDOWN:
      switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_R:
          pipeline_ = dir_light_pipeline_;
          current_light_type_ = LightType::kDirectional;
          Engine::set_clear_color(glm::vec3(0.2, 0.2, 0.5));
          break;
        case SDL_SCANCODE_Q:
          pipeline_ = point_light_pipeline_;
          current_light_type_ = LightType::kPoint;
          Engine::set_clear_color(glm::vec3(0.05f));
          break;
        case SDL_SCANCODE_F:
          pipeline_ = flashlight_pipeline_;
          current_light_type_ = LightType::kSpot;
          flashlight_.outer_cutoff = flashlight_.cutoff;
          Engine::set_clear_color(glm::vec3(0.05f));
          break;
        case SDL_SCANCODE_G:
          pipeline_ = flashlight_pipeline_;
          current_light_type_ = LightType::kSpot;
          flashlight_.outer_cutoff = glm::cos(glm::radians(10.f));
          Engine::set_clear_color(glm::vec3(0.05f));
          break;
        case SDL_SCANCODE_X:
          must_appply_textures_ = !must_appply_textures_;
          break;
        case SDL_SCANCODE_C:
          use_blinn_phong_ = !use_blinn_phong_;
          break;
        default:
          break;
      }
      break;

    default:
      break;
    }
}