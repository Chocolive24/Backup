#include "hello_quad.h"
#include "file_utility.h"

#include <iostream>
#include <array>

void HelloQuad::Begin() {
  pipeline_.Begin("data/shaders/hello_quad/quad.vert",
                  "data/shaders/hello_quad/quad.frag");

  vao_.Create();
  vbo_.Create();
  ebo_.Create();

  std::vector<float> vertices(kVertexFloatCount);
  vertices = {
      // positions              
      0.5f, 0.5f,  0.0f,  1.0f, 0.0f,  0.0f,      
      0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,       
      -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f,       
      -0.5f, 0.5f, 0.0f,  0.5f,  0.5f, 0.5f      
  };
      
  vao_.AttachVBO(vbo_);
  vbo_.SetData(vertices.data(), vertices.size(), GL_STATIC_DRAW);

  std::vector<GLuint> indices(kIndiceBeginCount_);
  indices = {0, 1, 3, 1, 2, 3};

  vao_.AttachEBO(ebo_);
  ebo_.SetData(indices);

  vertex_layout_.PushAttribute(VertexAttribute(3, GL_FLOAT, GL_FALSE)); // positions.
  vertex_layout_.PushAttribute(VertexAttribute(3, GL_FLOAT, GL_FALSE)); // colors.

  vao_.SetupVertexAttributes(vertex_layout_);
}

void HelloQuad::End() {
  // Unload program/pipeline
  pipeline_.End();

  vao_.Destroy();
  vbo_.Destroy();
  vertex_layout_.Clear();
  ebo_.Destroy();
}

void HelloQuad::Update(float dt) {
  // Draw program
  pipeline_.Bind();
  vao_.Bind();
  glDrawElements(GL_TRIANGLES, ebo_.element_count(), GL_UNSIGNED_INT, 0);
}