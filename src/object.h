#ifndef OBJECT_H
#define OBJECT_H

#include<iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>


#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include "../texture.h"
#include "shader.h"

struct Vertex {
	glm::vec3 Position;
	glm::vec2 Texture;
	glm::vec3 Normal;
};


struct Properties {
	bool isStatic = true;
	bool hasCollision = true;
	bool hasGravity = true;
	bool hasVelocity = true;
	bool hasTexture = false;
};
// Axis-aligned bounding boxes
// https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection
struct AABB
{
	glm::vec3 min, max;
	inline static bool collisionCheck(const AABB& a, const AABB& b) {
		return (a.max.x >= b.min.x && a.min.x <= b.max.x) &&
			   (a.max.y >= b.min.y && a.min.y <= b.max.y) &&
			   (a.max.z >= b.min.z && a.min.z <= b.max.z);
	}
};
class Object
{
public:
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> textures;
	std::vector<glm::vec3> normals;
	std::vector<Vertex> vertices;
	Properties properties;
	AABB collisionBox;
	glm::vec3 localCenter{ 0.0f };
	glm::vec3 localExtents{ 0.0f };
	int numVertices;
	Texture texture;
	GLuint VBO, VAO;
	//Camera* camera = nullptr;
	glm::mat4 model = glm::mat4(1.0);

	Object() = default;
	Object(const char* path);
	Object(const char* path, Properties p);
	void computeCollisionBox();
	AABB worldAABB() const;
	void bindTexture(std::string path);
	void makeObject(Shader shader, bool texture = true);
	virtual void draw();
};
#endif