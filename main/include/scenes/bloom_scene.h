#pragma once

#include "scene.h"
#include "camera.h"
#include "mesh.h"
#include "texture.h"
#include "vertex_array_object.h"
#include "vertex_buffer_object.h"
#include "element_buffer_object.h"

#include <array>

class BloomScene final : public Scene {
 public:
  void Begin() override;
  void CreatePingPongFrameBuffers(const glm::vec2& window_size);
  void CreateHdrFrameBuffer(const glm::vec2& window_size);
  void End() override;
  void Update(float dt) override;
  void OnEvent(const SDL_Event& event) override;

 private:
  // Scene pipelines with HDR.
  Pipeline container_cube_pipe_;
  Pipeline light_box_pipe_;

  // Bloom effect pipelines.
  Pipeline gaussian_blur_pipe_;
  Pipeline bloom_pipeline_;

  Mesh cube_;
  Mesh ground_quad_;
  Mesh screen_quad_;

  GLuint container_map_;
  GLuint wood_map_;

  // Hdr buffers to draw the scene and exctract the high brightness pixels.
  GLuint hdr_fbo_, depth_rbo_;
  std::array<GLuint, 2> color_buffers_ = { 0, 0 };

  // Buffer to apply the blur to create a final bloom effect.
  std::array<GLuint, 2> ping_pong_fbo_ = { 0, 0 };
  std::array<GLuint, 2> ping_pong_color_buffers_ = { 0, 0 };

  Camera camera_;

  glm::mat4 model_, view_, projection_;

  static constexpr std::array<glm::vec3, 4> light_positions_ = {
    glm::vec3( 0.0f, 0.5f,  1.5f),
    glm::vec3(-4.0f, 0.5f, -3.0f),
    glm::vec3(3.0f, 0.5f, 1.0f),
    glm::vec3(-.8f,  2.4f, -1.0f)
  };

   static constexpr std::array<glm::vec3, 4> light_colors_ = {
     glm::vec3(10.0f,   10.0f,  10.0f),
     glm::vec3(10.0f, 0.0f, 0.0f),
     glm::vec3(0.0f, 0.0f, 15.0f),
     glm::vec3(0.0f,   5.0f,  0.0f)
  };

   bool hdr_ = true;
   float exposure_ = 1.f;

   void DrawContainerCubes();
   void DrawGroundCube();
   void DrawLightBoxes();
};
