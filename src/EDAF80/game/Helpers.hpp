#pragma once

#include "core/node.hpp"

#include <vector>

namespace edaf80::game
{

bonobo::mesh_data createRevolvedShape(const std::vector<glm::vec2>& profile,
                                     unsigned int resolution);

// traverse the scene graph and render all nodes
// copied from assignment 1
void renderSubtree(const Node& root, const glm::mat4& viewProjection,
                   const glm::mat4& parentTransform = glm::mat4(1.0f));

}
