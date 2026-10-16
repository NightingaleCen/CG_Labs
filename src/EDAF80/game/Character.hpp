#pragma once

#include "core/helpers.hpp"
#include "../../core/InputHandler.h"

#include <functional>

namespace edaf80::game
{

class Character
{
    public:
        Character(float eyeHeight = 1.0f, float radius = 0.25f) :
            eyeHeight(eyeHeight), mRadius(radius) {};

        void move(float deltaTimeSec, glm::vec3 forwardDirection, InputHandler &ih);
        glm::vec2 moveAroundTrees(const glm::vec2& start, const glm::vec2& target,
                                 const std::function<bool(const glm::vec2&)>& isBlocked) const;

        glm::vec2 getPosition() const {return glm::vec2(mPosition.x, mPosition.y);};
        void setPosition(float x, float z) {mPosition.x = x; mPosition.y = z;};
        float getEyeHeight() const {return eyeHeight;};
        float getRadius() const {return mRadius;};
        float getSpeed() const {return mSpeed;};
        void setSpeed(float speed) {mSpeed = speed;};

    private:
        glm::vec2 mPosition = glm::vec2(0.0f, 0.0f);
        float mSpeed = 3.0f;
        float eyeHeight;
        float mRadius;
};

}
