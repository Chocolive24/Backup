#pragma once

#include "scene.h"
#include "vertex_array_object.h"
#include "vertex_buffer_object.h"
#include "vertex_attribute.h"

#include <array>

class HelloQuad final : public Scene {
 public:
  void Begin() override;
  void End() override;
  void Update(float dt) override;

 private:
  static constexpr int kVertexFloatCount = 24;
  static constexpr int kIndiceBeginCount_ = 6;

  Pipeline pipeline_;

  VertexArrayObject vao_;
  VertexBufferObject<float> vbo_;
  VertexAttributeLayout vertex_layout_;
  ElementBufferObject ebo_;
};
