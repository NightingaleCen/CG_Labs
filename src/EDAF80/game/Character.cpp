#include "Character.hpp"

#include <algorithm>
#include <cmath>

void edaf80::game::Character::move(float deltaTimeSec, glm::vec3 forwardDirection, InputHandler &ih)
{
    if (!ih.IsKeyboardCapturedByUI()) {
        bool forward = ih.GetKeycodeState(GLFW_KEY_W) & PRESSED;
        bool backward = ih.GetKeycodeState(GLFW_KEY_S) & PRESSED;
        bool left = ih.GetKeycodeState(GLFW_KEY_A) & PRESSED;
        bool right = ih.GetKeycodeState(GLFW_KEY_D) & PRESSED;

        glm::vec2 forwardDirectionXZ(forwardDirection.x, forwardDirection.z);
        glm::vec2 direction(0.0f);

        if (glm::length(forwardDirectionXZ) > 0.0f)
        {
            forwardDirectionXZ = glm::normalize(forwardDirectionXZ);
            glm::vec2 rightDirection(forwardDirectionXZ.y, -forwardDirectionXZ.x);

            if (forward) direction += forwardDirectionXZ;
            if (backward) direction -= forwardDirectionXZ;
            if (left) direction += rightDirection;
            if (right) direction -= rightDirection;
        }

        if (glm::length(direction) > 0.0f)
        {
            direction = glm::normalize(direction);
            mPosition += direction * mSpeed * deltaTimeSec;
        }
    }
}

glm::vec2 edaf80::game::Character::moveAroundTrees(const glm::vec2& start,
                                                  const glm::vec2& target,
                                                  const std::function<bool(const glm::vec2&)>& isBlocked) const
{
    // slide along blocked directions
    const auto movement = target - start;
    auto position = start;
    const glm::vec2 next_x(position.x + movement.x, position.y);
    if (!isBlocked(next_x))
        position.x = next_x.x;
    const glm::vec2 next_z(position.x, position.y + movement.y);
    if (!isBlocked(next_z))
        position.y = next_z.y;
    return position;
}
