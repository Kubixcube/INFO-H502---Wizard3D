//#include "shader.h"
//#include <fstream>
//#include <sstream>
//#include <iostream>
//
//Shader::Shader(const char* vertexPath, const char* fragmentPath) {
//    std::ifstream vFile(vertexPath), fFile(fragmentPath);
//    std::stringstream vStream, fStream;
//    vStream << vFile.rdbuf();
//    fStream << fFile.rdbuf();
//    std::string vCode = vStream.str();
//    std::string fCode = fStream.str();
//    const char* vShaderCode = vCode.c_str();
//    const char* fShaderCode = fCode.c_str();
//
//    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
//    glShaderSource(vertex, 1, &vShaderCode, NULL);
//    glCompileShader(vertex);
//
//    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
//    glShaderSource(fragment, 1, &fShaderCode, NULL);
//    glCompileShader(fragment);
//
//    ID = glCreateProgram();
//    glAttachShader(ID, vertex);
//    glAttachShader(ID, fragment);
//    glLinkProgram(ID);
//
//    glDeleteShader(vertex);
//    glDeleteShader(fragment);
//}
//
//void Shader::use() const {
//    glUseProgram(ID);
//}
//
//void Shader::setMat4(const std::string &name, const float* value) const {
//    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, value);
//}
//
//void Shader::setVec3(const std::string &name, float x, float y, float z) const {
//    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
//}
//
//void Shader::setInt(const std::string &name, int value) const {
//    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
//}
