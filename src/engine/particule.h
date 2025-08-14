#ifndef WIZARD3D_PARTICULE_H
#define WIZARD3D_PARTICULE_H


#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "object.h"

struct Particle {
    bool alive=false;
    glm::vec3 pos{0}, vel{0};
    float life=0.0f, maxLife=0.0f;
    float size=0.0f, size0=0.0f;
    glm::vec4 color{1,0.5,0,1}; // orange
};

struct ExplosionLight {
    bool active=false;
    glm::vec3 pos{0};
    float blastTimer = 0.0f;
    const float blastDuration = 0.18f;
    float intensity=0.0f;
    float radius=0.0f;
    float decay=0.0f; // /sec
};

class Projectile : public Entity{
public:
    bool active = false;
    float size = 0.45f;
    glm::vec4 color{1.0f, 0.4f, 0.05f, 0.9f};
    float speed = 10.0f;
    // properties for game logic
    float lifetime = 5.0f;
    float smokeTimer = 0.0f;
    Projectile() = default;
    void setColor(glm::vec4 c);
    glm::vec3 getCurrentPos() const;
};
#endif //WIZARD3D_PARTICULE_H
