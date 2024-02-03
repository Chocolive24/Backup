#include "uniform_quad.h"
#include "file_utility.h"

#include <iostream>
#include <array>

void UniformQuad::Begin() {
  pipeline_.Begin("data/shaders/uniform_quad/uniform_quad.vert",
                   "data/shaders/uniform_quad/uniform_quad.frag");

  glCreateVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

  // Create vbo.
  vertices_.reserve(kPositionCount_);
  vertices_.reserve(kPositionCount_);
  vertices_ = {
      // positions              // colors
      0.5f, 0.5f, 0.0f,         1.0f, 0.0f, 0.0f, 
      0.5f, -0.5f, 0.0f,        0.0f, 1.0f, 0.0f, 
      -0.5f, -0.5f, 0.0f,       0.0f, 0.0f, 1.0f, 
      -0.5f, 0.5f, 0.0f,        0.5f, 0.5f, 0.5f
  };

  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices_.size(),
               vertices_.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void*)(0));
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // Create ebo.
  indices_.reserve(kIndiceBeginCount_);
  indices_ = {0, 1, 3, 1, 2, 3};
  glGenBuffers(1, &ebo_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices_.size(),
               indices_.data(), GL_STATIC_DRAW);
}

void UniformQuad::End() {
  // Unload program/pipeline
  pipeline_.End();

  glDeleteVertexArrays(1, &vao_);
  glDeleteBuffers(1, &vbo_);
  glDeleteBuffers(1, &ebo_);

  color_coef_ = 0;
  time_ = 0;
}

void UniformQuad::Update(float dt) {
  // Update color coefficient value.
  if (time_ >= kTimeLimit_) {
    time_ = 0.f;
  }
  time_ += dt;
  color_coef_ = (std::sin(time_) + 1) / 2;
  
  // Update uniform value.
  pipeline_.SetFloat("color_coef", color_coef_);

  // Use program.
  pipeline_.Bind();
  glBindVertexArray(vao_);
  glDrawElements(GL_TRIANGLES, indices_.size(), GL_UNSIGNED_INT, 0);
}