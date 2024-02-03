#include "texture_quad.h"
#include "file_utility.h"

#include <iostream>
#include <array>

void TextureQuad::Begin() {
  pipeline_.Begin("data/shaders/texture/texture.vert",
                  "data/shaders/texture/texture.frag");

  //load texture
  diffuse_map_.Create("data/textures/monkey.png", GL_REPEAT, GL_LINEAR);

  quad_.CreateQuad();
}

void TextureQuad::End() {
  // Unload program/pipeline
  pipeline_.End();

  diffuse_map_.Destroy();

  quad_.Destroy();
}

void TextureQuad::Update(float dt) {
  // Draw program
  pipeline_.Bind();

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, diffuse_map_.id);
  
  pipeline_.DrawMesh(quad_);
}