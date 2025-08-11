#pragma once
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define POSID
#define NORMID
#define TEXID
class Object {
public:
	GLuint VAO, VBO;
	Object() = default;
	Object(std::string path);
	Object(float* obj);
};

