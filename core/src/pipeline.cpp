#include "pipeline.h"
#include "mesh.h"
#include "model.h"

#include <iostream>
#include <string>

#include "error.h"
#include "file_utility.h"

Pipeline::~Pipeline() {
  if (program_ != 0) {
    LOG_ERROR("Program was not destroyed");
  }
}

void Pipeline::Begin(std::string_view vertex_path,
                     std::string_view fragment_path) noexcept {
  // Load vertex shader.
  const auto vertexContent = file_utility::LoadFile(vertex_path);
  const auto* ptr = vertexContent.data();
  auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &ptr, nullptr);
  glCompileShader(vertex_shader);

  // Check success status of vertex shader compilation
  GLint success;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    std::cerr << "Error while loading vertex shader\n";
  }

  // Load fragment shader.
  const auto fragmentContent = file_utility::LoadFile(fragment_path);
  ptr = fragmentContent.data();
  auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &ptr, nullptr);
  glCompileShader(fragment_shader);

  // Check success status of fragment shader compilation
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    std::cerr << "Error while loading fragment shader\n";
  }

  // Load program/pipeline
  program_ = glCreateProgram();
  glAttachShader(program_, vertex_shader);
  glAttachShader(program_, fragment_shader);
  glLinkProgram(program_);

  // Check if shader program was linked correctly
  glGetProgramiv(program_, GL_LINK_STATUS, &success);
  if (!success) {
    std::cerr << "Error while linking shader program\n";
  }

  // Delete the shaders as they're linked into our program now and no longer
  // necessary.
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
}

void Pipeline::Bind() const noexcept {
  current_program_ = program_;
  glUseProgram(program_);
}

void Pipeline::DrawMesh(const Mesh& mesh,
                        GLenum mode, GLuint start_tex_unit) noexcept {
  GLuint diffuseNr = 1;
  GLuint specularNr = 1;
  GLuint normalNr = 1;
  const auto textures = mesh.textures();
  for (GLuint i = 0; i < textures.size(); i++) {
    // activate proper texture unit before binding
    glActiveTexture(GL_TEXTURE0 + i + start_tex_unit);

    // Retrieve texture number (the N in diffuse_textureN).
    const auto& texture = textures[i];
    int number = 0;
    std::string name = texture.type;
    if (name == "texture_diffuse")
    {
      number = diffuseNr;
      diffuseNr++;
    } 
    else if (name == "texture_specular")
    {
      number = specularNr;
      specularNr++;
    } 
    else if (name == "texture_normal") {
      number = normalNr;
      normalNr++;
    }

    SetInt("material." + name + std::to_string(number), i + start_tex_unit);
    glBindTexture(GL_TEXTURE_2D, texture.id);
  }

  // Draw mesh.
  glBindVertexArray(mesh.vao().id());
  glDrawElements(mode, mesh.elementCount(), GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void Pipeline::DrawModel(const Model& model, GLuint start_tex_unit) noexcept {
  for (const auto& mesh : model.meshes()) {
    DrawMesh(mesh, GL_TRIANGLES, start_tex_unit);
  }
}

void Pipeline::End() noexcept {
  glDeleteProgram(program_);
  if (current_program_ == program_) {
    current_program_ = 0;
  }
  program_ = 0;
}

void Pipeline::SetInt(std::string_view name, int value) const noexcept {
  glUniform1i(glGetUniformLocation(program_, name.data()), value);
};

void Pipeline::SetFloat(std::string_view name, float value) const noexcept {
  glUniform1f(glGetUniformLocation(program_, name.data()), value);
};

void Pipeline::SetBool(std::string_view name, bool value) const noexcept {
  glUniform1i(glGetUniformLocation(program_, name.data()), value);
}
void Pipeline::SetVec2(std::string_view name, glm::vec2 vec2) const noexcept {
  glUniform2f(glGetUniformLocation(program_, name.data()), vec2.x, vec2.y);
};

void Pipeline::SetVec3(std::string_view name, glm::vec3 vec3) const noexcept {
  glUniform3f(glGetUniformLocation(program_, name.data()), vec3.x, vec3.y,
              vec3.z);
}

void Pipeline::SetVec4(std::string_view name, glm::vec4 vec4) const noexcept {
  glUniform4f(glGetUniformLocation(program_, name.data()), vec4.x, vec4.y,
              vec4.z, vec4.w);
}

void Pipeline::SetMatrix3(std::string_view name,
                          const glm::mat3& mat) const noexcept {
  glUniformMatrix3fv(glGetUniformLocation(program_, name.data()), 1, GL_FALSE,
                     glm::value_ptr(mat));
};

void Pipeline::SetMatrix4(std::string_view name,
                          const glm::mat4& mat) const noexcept {
  glUniformMatrix4fv(glGetUniformLocation(program_, name.data()), 1, GL_FALSE,
                     glm::value_ptr(mat));
};

void Pipeline::SetLight(std::string_view name,
                        const DirectionalLight& light) const noexcept {
  const auto str_name = std::string(name.data());
  SetVec3(str_name + ".direction", light.direction);

  SetVec3(str_name + ".ambient", light.ambient);
  SetVec3(str_name + ".diffuse", light.diffuse);
  SetVec3(str_name + ".specular", light.specular);
}

void Pipeline::SetLight(std::string_view name,
                        const PointLight& light) const noexcept {
  const auto str_name = std::string(name.data());
  SetVec3(str_name + ".position", light.position);

  SetVec3(str_name + ".ambient", light.ambient);
  SetVec3(str_name + ".diffuse", light.diffuse);
  SetVec3(str_name + ".specular", light.specular);

  SetFloat(str_name + ".constant", light.constant);
  SetFloat(str_name + ".linear", light.linear);
  SetFloat(str_name + ".quadratic", light.quadratic);
}

void Pipeline::SetLight(std::string_view name,
                        const SpotLight& light) const noexcept {
  const auto str_name = std::string(name.data());
  SetVec3(str_name + ".position", light.position);
  SetVec3(str_name + ".direction", light.direction);

  SetVec3(str_name + ".ambient", light.ambient);
  SetVec3(str_name + ".diffuse", light.diffuse);
  SetVec3(str_name + ".specular", light.specular);

  SetFloat(str_name + ".constant", light.constant);
  SetFloat(str_name + ".linear", light.linear);
  SetFloat(str_name + ".quadratic", light.quadratic);

  SetFloat(str_name + ".cutoff", light.cutoff);
  SetFloat(str_name + ".outer_cutoff", light.outer_cutoff);
}