#pragma once
//
// Spline.h - splajn Catmull-Rom (NED-01).
//
// Gladka, ciagla krzywa przechodzaca przez zadane punkty kontrolne. Sluzy jako
// sciezka ruchu (ryby, meduzy): na niej NED-02 zbuduje Parallel Transport
// Frames, a NED-04 pusci po niej ryby.
//
// Parametryzacja: evaluate(t) dla t w [0,1] obchodzi cala zapetlona sciezke
// (dowolne t jest zawijane modulo, wiec t=1 wraca do startu). evaluate(0)
// zwraca pierwszy punkt kontrolny. Krzywa jest C1-ciagla, czyli tangenty sa
// gladkie i niezerowe rowniez na laczeniach segmentow.
//
#include "glm.hpp"
#include <vector>

class Spline
{
public:
    void addControlPoint(const glm::vec3& p);
    void clear();

    // Pozycja na krzywej. t w [0,1] po calej zapetlonej sciezce (zawijane modulo).
    glm::vec3 evaluate(float t) const;

    // Pochodna pozycji po t - kierunek "do przodu" wzdluz krzywej.
    // Niezerowa (dla >=2 punktow) i nieznormalizowana; znormalizuj u siebie jak trzeba.
    glm::vec3 evaluateTangent(float t) const;

    // Probkowanie do rysowania jako GL_LINE_STRIP (podglad debugowy).
    // samplesPerSegment punktow na segment; ostatni punkt domyka petle.
    std::vector<glm::vec3> sampleLine(int samplesPerSegment = 24) const;

    size_t pointCount() const { return controlPoints.size(); }
    const std::vector<glm::vec3>& points() const { return controlPoints; }

private:
    std::vector<glm::vec3> controlPoints;

    // Punkt kontrolny z zawinieciem indeksu (petla, dziala tez dla ujemnych).
    const glm::vec3& wrap(int i) const;

    // Rozklada globalne t w [0,1) na numer segmentu i lokalne s w [0,1).
    void locate(float t, int& seg, float& s) const;

    // Catmull-Rom dla jednego segmentu (p1->p2), s w [0,1]; p0/p3 to sasiedzi.
    static glm::vec3 catmullRom(const glm::vec3& p0, const glm::vec3& p1,
                                const glm::vec3& p2, const glm::vec3& p3, float s);
    static glm::vec3 catmullRomTangent(const glm::vec3& p0, const glm::vec3& p1,
                                       const glm::vec3& p2, const glm::vec3& p3, float s);
};
