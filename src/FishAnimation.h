#pragma once
//
// FishAnimation.h - ryba jadaca po splajnie z orientacja PTF (NED-04).
//
// Spina razem NED-01 (splajn), NED-02 (PTF) i NED-03 (deformacja pływania):
//   - parametr t pelznie po zapetlonym splajnie ze stala predkoscia,
//   - w kazdej klatce ramka PTF (getFrame) daje pozycje + orientacje,
//   - toMatrix() ramki -> macierz modelu (ryba patrzy nosem wzdluz krzywej),
//   - swimPhase desynchronizuje machanie ogonem miedzy rybami (uniform time).
//
// Sama deformacja ciala dzieje sie w fish.vert; tu liczymy tylko gdzie ryba
// jest i jak zorientowana.
//
#include "glm.hpp"
#include "Spline.h"

class FishAnimation
{
public:
    FishAnimation() = default;
    FishAnimation(const Spline* path, float speed, float tOffset, float scale, float swimPhase)
        : path(path), speed(speed), t(tOffset), scale(scale), swimPhaseOffset(swimPhase) {}

    // Przesuwa rybe po sciezce. dt w sekundach. t zawijane do [0,1).
    void update(float dt);

    // Macierz modelu z ramki PTF: kolumny (B, N, -T, pozycja) * skala.
    // Ryba (glowa w lokalnym -Z) patrzy wzdluz tangentu krzywej.
    glm::mat4 modelMatrix() const;

    // Sama macierz ramki PTF (obrot + translacja, bez skali). Uzywana przez ryby
    // Sketchfab, ktore doklejaja wlasna korekte lokalna (skala/centrowanie/os ciala).
    glm::mat4 frameMatrix() const;

    glm::vec3 position() const;
    float     swimPhase() const { return swimPhaseOffset; }
    float     paramT()    const { return t; }

private:
    const Spline* path = nullptr;
    float speed           = 0.05f; // jednostki t na sekunde (cala petla = 1.0)
    float t               = 0.0f;  // aktualny parametr na splajnie [0,1)
    float scale           = 1.0f;
    float swimPhaseOffset = 0.0f;  // offset do uniformu time (rozne machanie ogonem)
};
