#include "particule.h"
#include "PhysicsUtils.h"

void Projectile::setColor(glm::vec4 c) {
    color = c;
}

glm::vec3 Projectile::getCurrentPos() const {
    if (body == nullptr) return glm::vec3(0.0f);
    return toGLM(body->getTransform().getPosition());
}
