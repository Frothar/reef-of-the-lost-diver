//
// FishAnimation.cpp - implementacja ruchu ryby po splajnie (NED-04).
//
#include "FishAnimation.h"
#include "gtc/matrix_transform.hpp"
#include <cmath>

void FishAnimation::update(float dt)
{
    if (!path) return;
    t += speed * dt;
    t -= std::floor(t); // utrzymaj w [0,1) - sciezka jest zapetlona
}

glm::mat4 FishAnimation::modelMatrix() const
{
    if (!path) return glm::scale(glm::mat4(1.0f), glm::vec3(scale));

    SplineFrame f = path->getFrame(t);
    // f.toMatrix() niesie obrot (B,N,-T) + translacje; doskalowujemy jednolicie.
    return f.toMatrix() * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
}

glm::mat4 FishAnimation::frameMatrix() const
{
    if (!path) return glm::mat4(1.0f);
    return path->getFrame(t).toMatrix();
}

glm::vec3 FishAnimation::position() const
{
    if (!path) return glm::vec3(0.0f);
    return path->getFrame(t).position;
}
