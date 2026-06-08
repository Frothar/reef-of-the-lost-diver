//
// Spline.cpp - implementacja splajnu Catmull-Rom (NED-01).
//
#include "Spline.h"
#include <cmath>

void Spline::addControlPoint(const glm::vec3& p) { controlPoints.push_back(p); }
void Spline::clear() { controlPoints.clear(); }

const glm::vec3& Spline::wrap(int i) const
{
    int n = (int)controlPoints.size();
    int idx = ((i % n) + n) % n; // poprawne modulo rowniez dla ujemnych
    return controlPoints[idx];
}

void Spline::locate(float t, int& seg, float& s) const
{
    int n = (int)controlPoints.size();
    t = t - std::floor(t);          // zawin do [0,1)
    float scaled = t * n;           // n segmentow w petli
    seg = (int)std::floor(scaled);
    if (seg >= n) seg = n - 1;      // zabezpieczenie dla t bardzo blisko 1
    if (seg < 0)  seg = 0;
    s = scaled - (float)seg;
}

// Standardowa baza Catmull-Rom (tension 0.5).
glm::vec3 Spline::catmullRom(const glm::vec3& p0, const glm::vec3& p1,
                             const glm::vec3& p2, const glm::vec3& p3, float s)
{
    float s2 = s * s;
    float s3 = s2 * s;
    return 0.5f * ((2.0f * p1) +
                   (-p0 + p2) * s +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * s2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * s3);
}

// Pochodna powyzszego po s.
glm::vec3 Spline::catmullRomTangent(const glm::vec3& p0, const glm::vec3& p1,
                                    const glm::vec3& p2, const glm::vec3& p3, float s)
{
    float s2 = s * s;
    return 0.5f * ((-p0 + p2) +
                   2.0f * s  * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) +
                   3.0f * s2 * (-p0 + 3.0f * p1 - 3.0f * p2 + p3));
}

glm::vec3 Spline::evaluate(float t) const
{
    size_t n = controlPoints.size();
    if (n == 0) return glm::vec3(0.0f);
    if (n == 1) return controlPoints[0];

    int seg; float s;
    locate(t, seg, s);
    return catmullRom(wrap(seg - 1), wrap(seg), wrap(seg + 1), wrap(seg + 2), s);
}

glm::vec3 Spline::evaluateTangent(float t) const
{
    size_t n = controlPoints.size();
    if (n < 2) return glm::vec3(0.0f, 0.0f, 1.0f); // brak sensownego kierunku - domyslnie "do przodu"

    int seg; float s;
    locate(t, seg, s);
    return catmullRomTangent(wrap(seg - 1), wrap(seg), wrap(seg + 1), wrap(seg + 2), s);
}

std::vector<glm::vec3> Spline::sampleLine(int samplesPerSegment) const
{
    std::vector<glm::vec3> out;
    size_t n = controlPoints.size();
    if (n < 2) { out = controlPoints; return out; }
    if (samplesPerSegment < 1) samplesPerSegment = 1;

    int total = (int)n * samplesPerSegment;
    out.reserve((size_t)total + 1);
    for (int i = 0; i <= total; ++i)
    {
        float t = (float)i / (float)total; // 0..1 wlacznie; przy t=1 domyka petle
        out.push_back(evaluate(t));
    }
    return out;
}
