#include "DynamicObject.h"

DynamicObject::DynamicObject(const char* path, float m) : Object(path), mass(m){
}

void DynamicObject::setPos(glm::vec3 pos) {
	posistion = pos;
}

void DynamicObject::updatePos(float deltaTime) {
	const glm::vec3 gravity{ 0.0f, -GRAVITY, 0.0f };
	// a = f/m + g 
	glm::vec3 acceleration = (accForces / mass) + gravity;
	mouvement += acceleration * deltaTime;
	posistion += mouvement * deltaTime;
	accForces = {};
}
