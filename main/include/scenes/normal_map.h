#pragma once

#include "scene.h"
#include "camera.h"
#include "mesh.h"
#include "model.h"
#include "texture.h"
#include "light.h"

class NormalMap final : public Scene {
public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  Pipeline wall_norm_map_pipe_;
  Pipeline wall_basic_map_pipe;
  Pipeline model_norm_map_pipe_;
  Pipeline model_basic_map_pipe;
  Pipeline light_cube_pipeline_;

  Camera camera_;

  Model nanosuit_;

  Mesh cube_;
  Texture diffuse_map;
  Texture normal_map_;
  int shininess = 64;

  glm::vec3 light_pos_;
  glm::vec3 light_color_ = glm::vec3(1.f);

  glm::mat4 model_, view_, projection_;

  void DrawLightCubes(float dt);
  void DrawNormalMapPipeline();
  void DrawBasicMapPipeline();
};
