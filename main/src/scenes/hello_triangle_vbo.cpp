#include "hello_triangle_vbo.h"
#include "file_utility.h"

#include <iostream>
#include <array>

void HelloTriangleVBO::Begin() {
  pipeline_.Begin("data/shaders/hello_triangle_vao/triangle_vao.vert",
                  "data/shaders/hello_triangle_vao/triangle_vao.frag");

  my_vao_.Create();
  my_vbo_.Create();

  vertices_.reserve(kPositionCount_);
  vertices_ = {
      // positions                 
      glm::vec3(0.0f, 0.5f, 0.0f), 
      glm::vec3(0.5f, -0.5f, 0.0f), 
      glm::vec3(-0.5f, -0.5f, 0.0f)
  };

  my_vao_.AttachVBO(my_vbo_);
  my_vbo_.SetData(vertices_.data(), vertices_.size(), GL_STATIC_DRAW);

  vertex_layout_.PushAttribute(VertexAttribute(3, GL_FLOAT, GL_FALSE));
  my_vao_.SetupVertexAttributes(vertex_layout_);
}

void HelloTriangleVBO::End() {
  // Unload program/pipeline
  pipeline_.End();

  my_vao_.Destroy();
  my_vbo_.Destroy();

  vertex_layout_.Clear();
}

void HelloTriangleVBO::Update(float dt) {
  // Draw program
  pipeline_.Bind();
  my_vao_.Bind();
  glDrawArrays(GL_TRIANGLES, 0, vertices_.size());
}