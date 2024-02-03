#pragma once

#include "scene.h"
#include "camera.h"
#include "mesh.h"
#include "model.h"
#include "frame_buffer_object.h"

#include "glm/mat4x4.hpp"

class PostProcessing final : public Scene {
public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

private:
  Pipeline reversed_color_pipeline_;
  Pipeline gray_scale_pipeline_;
  Pipeline kernel_pipeline_;
  Pipeline blur_pipeline_;
  Pipeline edge_detection_pipeline_;
  Pipeline post_process_pipeline_;
  Pipeline cube_pipeline_;

  FrameBufferObject fbo_;

  Camera camera_;

  Model nanosuit_;

  Mesh cube_;
  Mesh screen_quad;

  Texture container_map_;
  Texture monkey_map;

  glm::mat4 model_, view_, projection_;


};
