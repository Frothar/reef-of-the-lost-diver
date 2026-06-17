#pragma once
//
// ParticleSystem.h - MRZ-07: banki powietrza unoszace sie do gory.
//
// Prosty CPU-owy system czastek renderowany jako point sprites (okragle,
// miekkie alpha). Czastki wznosza sie z emiterow przy dnie, lekko dryfuja na
// boki i znikaja po czasie zycia, po czym sa recyklingowane (respawn).
//
#include "glew.h"
#include "glm.hpp"
#include <vector>
#include <cstdlib>

class BubbleSystem
{
public:
    struct Particle
    {
        glm::vec3 pos;
        glm::vec3 vel;
        float life;      // pozostaly czas zycia (s)
        float maxLife;
        float size;      // rozmiar bazowy (px, skalowany perspektywicznie w shaderze)
    };

    // count - liczba czastek; emitters - punkty wybicia (przy dnie).
    void init(int count, const std::vector<glm::vec3>& emitters)
    {
        emitters_ = emitters;
        particles_.resize(count);
        for (auto& p : particles_)
            respawn(p, frand());          // porozkladane w roznych fazach zycia

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, particles_.size() * 5 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0); // posSize (xyz + size)
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1); // alpha
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(4 * sizeof(float)));
        glBindVertexArray(0);
    }

    void update(float dt)
    {
        if (dt <= 0.0f) return;
        gpu_.clear();
        gpu_.reserve(particles_.size() * 5);

        for (auto& p : particles_)
        {
            p.life -= dt;
            if (p.life <= 0.0f)
                respawn(p, 0.0f);

            // Lekki, zmienny dryf na boki + tlumienie (chwianie banki).
            p.vel.x += (frand() - 0.5f) * 0.8f * dt;
            p.vel.z += (frand() - 0.5f) * 0.8f * dt;
            p.vel.x *= 0.95f;
            p.vel.z *= 0.95f;
            p.pos += p.vel * dt;

            float lifeFrac = p.life / p.maxLife;                 // 1 -> 0
            float alpha = glm::clamp(lifeFrac * 1.3f, 0.0f, 0.65f); // zanik pod koniec zycia

            gpu_.push_back(p.pos.x);
            gpu_.push_back(p.pos.y);
            gpu_.push_back(p.pos.z);
            gpu_.push_back(p.size);
            gpu_.push_back(alpha);
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, gpu_.size() * sizeof(float), gpu_.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void draw() const
    {
        glBindVertexArray(vao_);
        glDrawArrays(GL_POINTS, 0, (GLsizei)particles_.size());
        glBindVertexArray(0);
    }

    int count() const { return (int)particles_.size(); }

private:
    std::vector<Particle>  particles_;
    std::vector<glm::vec3> emitters_;
    std::vector<float>     gpu_;
    GLuint vao_ = 0, vbo_ = 0;

    static float frand() { return (float)rand() / (float)RAND_MAX; }

    void respawn(Particle& p, float initialAgeFrac)
    {
        glm::vec3 e = emitters_.empty() ? glm::vec3(0.0f)
                                        : emitters_[rand() % emitters_.size()];
        p.pos = e + glm::vec3((frand() - 0.5f) * 1.5f, frand() * 0.3f, (frand() - 0.5f) * 1.5f);
        p.vel = glm::vec3((frand() - 0.5f) * 0.2f, 0.6f + frand() * 0.9f, (frand() - 0.5f) * 0.2f);
        p.maxLife = 4.0f + frand() * 4.0f;
        p.life = p.maxLife * (1.0f - initialAgeFrac);
        p.size = 7.0f + frand() * 11.0f;
    }
};
