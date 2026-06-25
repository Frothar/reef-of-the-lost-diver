//
// Spline.cpp - implementacja splajnu Catmull-Rom (NED-01) + PTF (NED-02).
//
#include "Spline.h"
#include "gtc/matrix_transform.hpp"
#include <cmath>
#include <algorithm>

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
        float t = (float)i / (float)total;
        out.push_back(evaluate(t));
    }
    return out;
}

// ---------------------------------------------------------------------------
// SplineFrame
// ---------------------------------------------------------------------------

glm::mat4 SplineFrame::toMatrix() const
{
    // Ryba skierowana nosem w kierunku T (jej lokalna os -Z to T w swiecie).
    // Kolumny macierzy 4x4: right(B), up(N), forward(-T), pozycja.
    return glm::mat4(
        glm::vec4( B,        0.0f),
        glm::vec4( N,        0.0f),
        glm::vec4(-T,        0.0f),
        glm::vec4( position, 1.0f)
    );
}

// ---------------------------------------------------------------------------
// PTF (NED-02) - Parallel Transport Frames
//
// Algorytm: double-reflection transport (Bishop 1975 / Bloomenthal 1990).
// Dla kazdej kolejnej pary punktow (p_i, p_{i+1}) z tangentami (T_i, T_{i+1}):
//   v1 = p_{i+1} - p_i
//   odbij N_i i T_i przez plaszczyzne o normalnej v1 -> riL, tiL
//   v2 = T_{i+1} - tiL
//   odbij riL przez plaszczyzne o normalnej v2 -> N_{i+1}
// To eliminuje skrecanie Frenet-Serret na odcinkach prostych i przy przegieciach.
// ---------------------------------------------------------------------------

static glm::vec3 reflect(const glm::vec3& v, const glm::vec3& n)
{
    // odbicie v przez plaszczyzne o normalnej n (n znormalizowane)
    return v - 2.0f * glm::dot(n, v) * n;
}

void Spline::buildFrames(int samples)
{
    ptfFrames.clear();
    if (controlPoints.size() < 2 || samples < 2) return;

    ptfFrames.resize((size_t)samples);

    // --- ramka startowa (i=0) ---
    {
        SplineFrame& f0 = ptfFrames[0];
        f0.position = evaluate(0.0f);
        f0.T = glm::normalize(evaluateTangent(0.0f));

        // Wybierz dowolna normalna prostopadla do T0.
        // Unikamy koliniowosci z T wybierajac os mniej rownolega do T.
        glm::vec3 helper = (std::abs(f0.T.y) < 0.9f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        f0.N = glm::normalize(glm::cross(glm::cross(f0.T, helper), f0.T));
        f0.B = glm::normalize(glm::cross(f0.T, f0.N));
    }

    // --- transport kolejnych ramek ---
    for (int i = 1; i < samples; ++i)
    {
        float t_prev = (float)(i - 1) / (float)samples;
        float t_curr = (float)i       / (float)samples;

        SplineFrame& prev = ptfFrames[(size_t)i - 1];
        SplineFrame& curr = ptfFrames[(size_t)i];

        curr.position = evaluate(t_curr);
        curr.T        = glm::normalize(evaluateTangent(t_curr));

        // Double reflection transport:
        glm::vec3 v1 = curr.position - prev.position;
        float c1 = glm::dot(v1, v1);

        glm::vec3 riL, tiL;
        if (c1 < 1e-10f)
        {
            // punkty zbyt blisko - skopiuj poprzednia ramke
            riL = prev.N;
            tiL = prev.T;
        }
        else
        {
            glm::vec3 nv1 = v1 / std::sqrt(c1);
            riL = reflect(prev.N, nv1);
            tiL = reflect(prev.T, nv1);
        }

        glm::vec3 v2 = curr.T - tiL;
        float c2 = glm::dot(v2, v2);

        if (c2 < 1e-10f)
        {
            curr.N = riL; // tangenty rownolegle - normalna bez zmian
        }
        else
        {
            curr.N = reflect(riL, glm::normalize(v2));
        }

        curr.N = glm::normalize(curr.N);
        curr.B = glm::normalize(glm::cross(curr.T, curr.N));
    }
}

SplineFrame Spline::getFrame(float t) const
{
    if (ptfFrames.empty()) return SplineFrame{ evaluate(t), glm::vec3(0,0,-1), glm::vec3(0,1,0), glm::vec3(1,0,0) };

    int n = (int)ptfFrames.size();

    // Zawin t do [0,1)
    t = t - std::floor(t);

    float scaled = t * (float)n;
    int   i0     = (int)std::floor(scaled);
    float alpha  = scaled - (float)i0;
    int   i1     = (i0 + 1) % n;

    const SplineFrame& fa = ptfFrames[(size_t)i0];
    const SplineFrame& fb = ptfFrames[(size_t)i1];

    SplineFrame out;
    out.position = glm::mix(fa.position, fb.position, alpha);
    out.T        = glm::normalize(glm::mix(fa.T, fb.T, alpha));
    out.N        = glm::normalize(glm::mix(fa.N, fb.N, alpha));
    out.B        = glm::normalize(glm::cross(out.T, out.N));
    return out;
}
