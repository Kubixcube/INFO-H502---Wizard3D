#ifndef DYNAMIC_OBJECT_H
#define DYNAMIC_OBJECT_H
#define GRAVITY 9.80665f

#include "src/object.h"
class DynamicObject : public Object {
public:
	DynamicObject() = default;
	DynamicObject(const char* path, float m);
	glm::vec3 posistion{0.0f,0.0f,0.0f};
	glm::vec3 mouvement{0.0f};
	glm::vec3 accForces{0.0f};
	float mass;
	void setPos(glm::vec3 pos);
	void updatePos(float deltaTime);
};
#endif

