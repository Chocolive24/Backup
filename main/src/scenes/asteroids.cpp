#include "asteroids.h"
#include "engine.h"


#include <iostream>

// TODO: 
// Faire une méthode DrawInstancedModel && DrawInstancedMesh dans Pipeline.
// Faire une method model.SetModelMatrixBufferSubData qui appelle la même méthod mais pour chaque mesh.

void Asteroids::Begin() {
  asteroid_pipeline_.Begin("data/shaders/instancing.vert",
                           "data/shaders/hello_model/model.frag");
  planet_pipeline_.Begin("data/shaders/transform.vert",
                         "data/shaders/hello_model/model.frag");
  screen_texture_pipe_.Begin("data/shaders/screen_texture.vert",
                        "data/shaders/texture/texture.frag");
  debug_mesh_pipe_.Begin("data/shaders/transform.vert",
                       "data/shaders/uni_color.frag");

  debug_cube_.CreateCube();

  asteroid_model_.Load("data/models/asteroid/rock.obj");

  asteroid_models_mat_.resize(kAsteroidCount_, glm::mat4(1.f));
  visible_asteroids_mat_.reserve(kAsteroidCount_);
  srand(42);
  for (std::size_t i = 0; i < kAsteroidCount_; i++) {
    // 1. Set position around radius.
    const float angle = i / static_cast<float>(kAsteroidCount_) * 360.f;
    float displacement = (rand() % (int)(2 * radius_offset_ * 100)) / 
                               100.0f - radius_offset_;
    const float pos_x = sin(angle) * kRotationRadius_ + displacement;
    displacement = (rand() % (int)(2 * radius_offset_ * 100)) / 
                   100.0f - radius_offset_;
    const float pos_y = displacement * 0.4f;
    displacement = (rand() % (int)(2 * radius_offset_ * 100)) / 
                   100.0f - radius_offset_;
    const float pos_z = cos(angle) * kRotationRadius_ + displacement;

    glm::mat4 model(1.f);
    model = glm::translate(model, glm::vec3(pos_x, 0, pos_z));

    // 2. Scale the asteroid randomly between 0.05 and 0.25;
    float scale = (rand() % 20) / 100.0f + 0.05;
    model = glm::scale(model, glm::vec3(scale));

    // 3. Rotate the asteroid with a random angle around a weird vector.
    float rotAngle = (rand() % 360);
    model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

    // 4. now add to list of matrices
    asteroid_models_mat_[i] = model;
  }

  asteroid_model_.SetupModelMatrixBuffer(asteroid_models_mat_.data(), 
                                         asteroid_models_mat_.size(), 
                                         GL_DYNAMIC_DRAW);
  asteroid_model_.GenerateModelSphereBoundingVolume();

  planet_model_.Load("data/models/planet/planet.obj");
  planet_model_.GenerateModelSphereBoundingVolume();

  // Create screen_quad vao data.
  constexpr GLuint kVertexCount = 4;
  std::vector<Vertex> vertices;
  vertices.reserve(kVertexCount);
  vertices = {
      Vertex{glm::vec3(1.f, 1.f, 0.0f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(1.0f, 1.0f)},
      Vertex{glm::vec3(1.f, 0.2f, 0.0f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(1.0f, 0.0f)},
      Vertex{glm::vec3(0.3f, 0.2f, 0.0f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(0.0f, 0.0f)},
      Vertex{glm::vec3(0.3f, 1.f, 0.0f), glm::vec3(0.f, 0.f, 1.0f),
             glm::vec2(0.0f, 1.0f)},
  };

  // Create indices.
  constexpr GLuint kIndiceCount = 6;

  debug_quad_indices_.reserve(kIndiceCount);
  debug_quad_indices_ = {0, 1, 3, 1, 2, 3};

  glGenVertexArrays(1, &screen_quad_vao_);
  glGenBuffers(1, &screen_quad_vbo_);
  glGenBuffers(1, &screen_quad_ebo_);

  glBindVertexArray(screen_quad_vao_);

  // Bind vbo.
  glBindBuffer(GL_ARRAY_BUFFER, screen_quad_vbo_);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
               vertices.data(), GL_STATIC_DRAW);

  // Bind ebo.
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screen_quad_ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               debug_quad_indices_.size() * sizeof(GLuint),
               debug_quad_indices_.data(), GL_STATIC_DRAW);

  // vertex positions
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

  // vertex texture coords
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void*)offsetof(Vertex, uv));

  glBindVertexArray(0);

  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

  glGenTextures(1, &screen_texture_);
  glBindTexture(GL_TEXTURE_2D, screen_texture_);

  const auto window_size = Engine::window_size();
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_size.x, window_size.y, 
               0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         screen_texture_, 0);

  glGenRenderbuffers(1, &rbo_);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_size.x,
                        window_size.y);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, rbo_);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  camera_.Begin(glm::vec3(0, 0, 0), 45.f, 0.1f, 100.f);
  debug_camera_.Begin(glm::vec3(0, 150, 0), 45.f, 0.1f, 300.f, -90.f, -90.f);

  const auto cam_pos = camera_.position();
  const auto& frustum = camera_.CalculateFrustum(Engine::window_aspect());

  std::array<glm::vec3, 8> corners = {};

  frustum_indices_ = {
    // Connecting planes
    0, 4, 1,
    1, 4, 5,
    2, 6, 3,
    3, 6, 7,

    // Additional connecting triangles
    0, 2, 4,
    2, 6, 4,
    1, 5, 3,
    5, 7, 3,

    // Near plane
    0, 1, 2,
    1, 3, 2,

    // Far plane
    4, 5, 6,
    5, 7, 6,
  };

  // Create frustum Mesh.
  glGenVertexArrays(1, &frustum_vao_);
  glGenBuffers(1, &frustum_vbo_);
  glGenBuffers(1, &frustum_ebo_);

  glBindVertexArray(frustum_vao_);

  // Bind vbo.
  glBindBuffer(GL_ARRAY_BUFFER, frustum_vbo_);
  glBufferData(GL_ARRAY_BUFFER, frustum.corners.size() * sizeof(glm::vec3),
               frustum.corners.data(), GL_DYNAMIC_DRAW);

  // Bind ebo.
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, frustum_ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, frustum_indices_.size() * sizeof(GLuint),
               frustum_indices_.data(), GL_STATIC_DRAW);

  // vertex positions
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
   
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
}

void Asteroids::End() {
  // Unload program/pipeline
  asteroid_pipeline_.End();
  planet_pipeline_.End();
  screen_texture_pipe_.End();
  debug_mesh_pipe_.End();

  glDeleteFramebuffers(1, &fbo_);
  glDeleteRenderbuffers(1, &rbo_);

  debug_cube_.Destroy();

  asteroid_model_.Destroy();
  planet_model_.Destroy();

  glDeleteVertexArrays(1, &screen_quad_vao_);
  glDeleteBuffers(1, &screen_quad_vbo_);
  glDeleteBuffers(1, &screen_quad_ebo_);

  camera_.End();
  debug_camera_.End();

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
}

void Asteroids::Update(float dt) {
  const auto window_size = Engine::window_size();
  const auto aspect = Engine::window_aspect();

  camera_.Update(dt);

  view_ = camera_.CalculateViewMatrix();
  projection_ = camera_.CalculateProjectionMatrix(aspect);
  const auto& frustum = camera_.CalculateFrustum(aspect);

  // First pass (draw scene from user camera).
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // we're not using the stencil buffer now

  asteroid_pipeline_.Bind();

  asteroid_pipeline_.SetMatrix4("view", view_);
  asteroid_pipeline_.SetMatrix4("projection", projection_);

  visible_asteroids_mat_.clear();
  for (const auto& mesh : asteroid_model_.meshes()) {
    for (std::size_t i = 0; i < kAsteroidCount_; i++) {
      if (mesh.bounding_sphere().IsOnFrustum(frustum,
                                             asteroid_models_mat_[i])) {
        visible_asteroids_mat_.push_back(asteroid_models_mat_[i]);
      }
    }
  }

  asteroid_model_.SetModelMatrixBufferSubData(visible_asteroids_mat_.data(), 
                                              visible_asteroids_mat_.size());

  for (auto& mesh : asteroid_model_.meshes()) {
    asteroid_pipeline_.SetInt("texture_diffuse1", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mesh.textures()[0].id);

    renderer_.DrawInstancedMesh(mesh, visible_asteroids_mat_.size());
  }

  planet_pipeline_.Bind();

  planet_pipeline_.SetMatrix4("transform.view", view_);
  planet_pipeline_.SetMatrix4("transform.projection", projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, kPlanetPos_);
  model_ = glm::scale(model_, glm::vec3(0.3f));
  planet_pipeline_.SetMatrix4("transform.model", model_);

  bool is_planet_on_frustum = false;

  for (const auto& mesh : planet_model_.meshes()) {
    if (mesh.bounding_sphere().IsOnFrustum(frustum, model_)) {
      is_planet_on_frustum = true;
    }
  }
  
  if (is_planet_on_frustum) {
    planet_pipeline_.DrawModel(planet_model_);
  }

  // Second pass (draw scene from debug POV in the FBO).
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, window_size.x,
                        window_size.y);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glFrontFace(GL_CCW);
  glCullFace(GL_BACK);
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // we're not using the stencil buffer now

  debug_view_ = debug_camera_.CalculateViewMatrix();
  debug_projection_ = debug_camera_.CalculateProjectionMatrix(aspect);

  asteroid_pipeline_.Bind();

  asteroid_pipeline_.SetMatrix4("view", debug_view_);
  asteroid_pipeline_.SetMatrix4("projection", debug_projection_);

  for (const auto& mesh : asteroid_model_.meshes()) {
    asteroid_pipeline_.SetInt("texture_diffuse1", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mesh.textures()[0].id);

    glBindVertexArray(mesh.vao().id());
    glDrawElementsInstanced(GL_TRIANGLES, mesh.elementCount(), GL_UNSIGNED_INT,
                            0, visible_asteroids_mat_.size());
    glBindVertexArray(0);
  }

  planet_pipeline_.Bind();

  planet_pipeline_.SetMatrix4("transform.view", debug_view_);
  planet_pipeline_.SetMatrix4("transform.projection", debug_projection_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, kPlanetPos_);
  model_ = glm::scale(model_, glm::vec3(0.3f));
  planet_pipeline_.SetMatrix4("transform.model", model_);

  if (is_planet_on_frustum) {
    planet_pipeline_.DrawModel(planet_model_);
  }

  glDisable(GL_DEPTH);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  debug_mesh_pipe_.Bind();

  debug_mesh_pipe_.SetMatrix4("transform.projection", debug_projection_);
  debug_mesh_pipe_.SetMatrix4("transform.view", debug_view_);

  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, camera_.position());
  model_ = glm::scale(model_, glm::vec3(2.f));
  debug_mesh_pipe_.SetMatrix4("transform.model", model_);

  debug_mesh_pipe_.SetVec4("fragColor", glm::vec4(1.f, 0.f, 0.f, 1.f));

  debug_mesh_pipe_.DrawMesh(debug_cube_);

  glBindBuffer(GL_ARRAY_BUFFER, frustum_vbo_);
  glBufferSubData(GL_ARRAY_BUFFER, 0,
                frustum.corners.size() * sizeof(glm::vec3),
                frustum.corners.data());

  model_ = glm::mat4(1.f);
  debug_mesh_pipe_.SetMatrix4("transform.model", model_);

  debug_mesh_pipe_.SetVec4("fragColor", glm::vec4(0.f, 0.f, 1.f, 0.2f));

  glBindVertexArray(frustum_vao_);
  glDrawElements(GL_TRIANGLES, frustum_indices_.size(), GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);

  // Third pass (draw the screen_texture (the FBO)).
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);

  screen_texture_pipe_.Bind();

  glBindTexture(GL_TEXTURE_2D, screen_texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_size.x, window_size.y, 0,
               GL_RGB, GL_UNSIGNED_BYTE, NULL);

  glBindVertexArray(screen_quad_vao_);
  glDrawElements(GL_TRIANGLES, debug_quad_indices_.size(), GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void Asteroids::OnEvent(const SDL_Event& event) { camera_.OnEvent(event); }