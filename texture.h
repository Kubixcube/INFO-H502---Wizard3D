#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <iostream>
#include "stb_image.h"
class Texture {
public:
	GLuint ID;
	Texture() = default;
	Texture(std::string path);
	void map() const;
private:
	int width, height, channels;
};
#endif