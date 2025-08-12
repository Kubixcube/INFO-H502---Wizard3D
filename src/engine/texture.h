#ifndef WIZARD3D_TEXTURE_H
#define WIZARD3D_TEXTURE_H

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
    Texture(const char* path);
    void map() const;
private:
    int width, height, channels;
};

#endif //WIZARD3D_TEXTURE_H
