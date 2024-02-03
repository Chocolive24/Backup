#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <string>
#include <vector>
#include <memory>

struct Transform {
  // Local space information.
  // ------------------------
  glm::vec3 position = {0.0f, 0.0f, 0.0f};
  // In euleur angles.
  glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
  glm::vec3 scale = {1.0f, 1.0f, 1.0f};

  // Global space information concatenate in matrix.
  // -----------------------------------------------
  glm::mat4 model_matrix = glm::mat4(1.0f);
};

class Entity {
 public:
   Transform transform;

   std::vector<std::unique_ptr<Entity>> children;
   Entity* parent = nullptr;

   void InitFromModel(std::string_view path, bool gamma = false, bool flip_y = true) 
	   noexcept;
};