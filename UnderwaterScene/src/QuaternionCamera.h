#pragma once
//
// QuaternionCamera.h - kamera na kwaternionach (MRZ-02, metoda obowiazkowa M3).
//
// Orientacja trzymana jest jako kwaternion jednostkowy, a obroty skladamy w
// lokalnym ukladzie kamery. Dzieki temu nie ma gimbal locka i nie ma
// wyroznionej osi "gora" - czyli dokladnie to, czego chcemy do swobodnego
// plywania pod woda (6DOF).
//
// Nazwa klasy to celowo "Camera" (globalna), zeby nie mylic z funkcjami z
// Core/Camera.h (Core::createViewMatrix / createPerspectiveMatrix).
//
#include "glm.hpp"
#include "ext.hpp"
#include "gtc/matrix_transform.hpp"   // glm::lookAt
#include "gtc/quaternion.hpp"         // glm::quat, angleAxis

class Camera
{
public:
    glm::vec3 position = glm::vec3(0.0f, 1.0f, 6.0f);

    Camera() = default;
    explicit Camera(const glm::vec3& pos) : position(pos) {}

    // Wektory bazowe wyliczone z orientacji.
    glm::vec3 front() const { return orientation * glm::vec3(0.0f, 0.0f, -1.0f); }
    glm::vec3 right() const { return orientation * glm::vec3(1.0f, 0.0f, 0.0f); }
    glm::vec3 up()    const { return orientation * glm::vec3(0.0f, 1.0f, 0.0f); }

    glm::mat4 viewMatrix() const
    {
        return glm::lookAt(position, position + front(), up());
    }

    // Mysz: yaw wokol lokalnej gory, pitch wokol lokalnego prawa.
    // Skladanie w lokalnym ukladzie (orientation * delta) = brak gimbal locka.
    void addYawPitch(float yawDeg, float pitchDeg)
    {
        glm::quat qYaw   = glm::angleAxis(glm::radians(yawDeg),   glm::vec3(0.0f, 1.0f, 0.0f));
        glm::quat qPitch = glm::angleAxis(glm::radians(pitchDeg), glm::vec3(1.0f, 0.0f, 0.0f));
        orientation = glm::normalize(orientation * qYaw * qPitch);
    }

    // Przechyl (roll) wokol lokalnego "do przodu" - klawisze Q/E.
    void addRoll(float rollDeg)
    {
        orientation = glm::normalize(orientation * glm::angleAxis(glm::radians(rollDeg), glm::vec3(0.0f, 0.0f, -1.0f)));
    }

    // Ruch.
    void moveForward(float d) { position += front() * d; }
    void moveRight(float d)   { position += right() * d; }
    void moveWorldUp(float d) { position += glm::vec3(0.0f, 1.0f, 0.0f) * d; } // gora wzgledem swiata (ku powierzchni)

    const glm::quat& getOrientation() const { return orientation; }

private:
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identycznosc (w, x, y, z)
};
