#include "hello_triangle.h"

void HelloTriangle::Begin() {
  pipeline_.Begin("data/shaders/hello_triangle/triangle.vert",
                  "data/shaders/hello_triangle/triangle.frag");

  // Empty vao
  glCreateVertexArrays(1, &vao_);
}

void HelloTriangle::End() {
  // Unload program/pipeline
  pipeline_.End();

  glDeleteVertexArrays(1, &vao_);
}

void HelloTriangle::Update(float dt) {
  // Draw program
  pipeline_.Bind();
  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}