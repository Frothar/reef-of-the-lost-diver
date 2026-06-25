#pragma once
//
// Spline.h - splajn Catmull-Rom (NED-01) + Parallel Transport Frames (NED-02).
//
// Gladka, ciagla krzywa przechodzaca przez zadane punkty kontrolne. Sluzy jako
// sciezka ruchu (ryby, meduzy). PTF daje stabilne ramki orientacji bez skrecania
// (Frenet-Serret skrecalby sie przy prostych odcinkach i w punktach przegiec).
//
// Parametryzacja: evaluate(t) dla t w [0,1] obchodzi cala zapetlona sciezke
// (zawijane modulo). evaluate(0) = pierwszy punkt kontrolny. Krzywa C1-ciagla.
//
// Uzycie PTF:
//   spline.buildFrames(256);          // raz po dodaniu punktow kontrolnych
//   SplineFrame f = spline.getFrame(t); // w petli animacji
//   glm::mat4 model = f.toMatrix();    // gotowa macierz modelu dla ryby
//
#include "glm.hpp"
#include "gtc/quaternion.hpp"
#include <vector>

// ---------------------------------------------------------------------------
// SplineFrame - jedna ramka PTF wzdluz splajnu
// ---------------------------------------------------------------------------
struct SplineFrame
{
    glm::vec3 position;  // punkt na krzywej
    glm::vec3 T;         // tangent (do przodu), znormalizowany
    glm::vec3 N;         // normalna (lokalna "gora"), znormalizowana, stabilna
    glm::vec3 B;         // binormalna = cross(T, N), znormalizowana

    // Macierz modelu gotowa do uzycia jako uniform "model":
    //   kolumny: B, N, -T, position  (obrot + translacja, skala=1)
    // Ryba patrzy wzdluz -T (OpenGL: os Z skierowana "do tylu").
    glm::mat4 toMatrix() const;
};

// ---------------------------------------------------------------------------
// Spline
// ---------------------------------------------------------------------------
class Spline
{
public:
    void addControlPoint(const glm::vec3& p);
    void clear();

    // Pozycja na krzywej. t w [0,1] po calej zapetlonej sciezce (zawijane modulo).
    glm::vec3 evaluate(float t) const;

    // Pochodna pozycji po t - kierunek "do przodu" wzdluz krzywej.
    // Niezerowa (dla >=2 punktow) i nieznormalizowana.
    glm::vec3 evaluateTangent(float t) const;

    // Probkowanie do rysowania jako GL_LINE_STRIP (podglad debugowy NED-01).
    // samplesPerSegment punktow na segment; ostatni punkt domyka petle.
    std::vector<glm::vec3> sampleLine(int samplesPerSegment = 24) const;

    // --- PTF (NED-02) -------------------------------------------------------

    // Wstepne obliczenie ramek PTF. Wywolaj raz po ustawieniu punktow.
    // samples: liczba probek na cala zapetlona sciezke (wieksza = gladsze PTF).
    void buildFrames(int samples = 256);

    // Ramka PTF w dowolnym t in [0,1] (interpolacja miedzy sasiednimi probkami).
    // Wymaga wczesniejszego buildFrames().
    SplineFrame getFrame(float t) const;

    // Prebuilt frames (do debugowego renderowania osi).
    const std::vector<SplineFrame>& frames() const { return ptfFrames; }

    size_t pointCount() const { return controlPoints.size(); }
    const std::vector<glm::vec3>& points() const { return controlPoints; }

private:
    std::vector<glm::vec3>  controlPoints;
    std::vector<SplineFrame> ptfFrames;    // wyniki buildFrames()

    const glm::vec3& wrap(int i) const;
    void locate(float t, int& seg, float& s) const;

    static glm::vec3 catmullRom(const glm::vec3& p0, const glm::vec3& p1,
                                const glm::vec3& p2, const glm::vec3& p3, float s);
    static glm::vec3 catmullRomTangent(const glm::vec3& p0, const glm::vec3& p1,
                                       const glm::vec3& p2, const glm::vec3& p3, float s);
};
