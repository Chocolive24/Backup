#pragma once

#include "scene.h"
#include "vertex_array_object.h"
#include "vertex_buffer_object.h"
#include "vertex_attribute.h"

#include <array>

class HelloTriangleVBO final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;

private:
  Pipeline pipeline_;
  VertexArrayObject my_vao_;
  VertexBufferObject<glm::vec3> my_vbo_;
  VertexAttributeLayout vertex_layout_;

  static constexpr int kVertexElemCount_ = 3;
  static constexpr int kPositionCount_ = 3;

  std::vector<glm::vec3> vertices_;
};
