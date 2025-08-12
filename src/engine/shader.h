
#ifndef SHADER_H
#define SHADER_H
#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
class Shader{
public:
    GLuint ID;
    Shader(const char* vp,const char* fp){
        std::ifstream v(vp), f(fp);
        std::stringstream vs,fs; vs<<v.rdbuf(); fs<<f.rdbuf();
        std::string vsrc=vs.str(), fsrc=fs.str();
        const char* V=vsrc.c_str(); const char* F=fsrc.c_str();
        GLuint vsId=glCreateShader(GL_VERTEX_SHADER); glShaderSource(vsId,1,&V,NULL); glCompileShader(vsId);
        GLuint fsId=glCreateShader(GL_FRAGMENT_SHADER); glShaderSource(fsId,1,&F,NULL); glCompileShader(fsId);
        ID=glCreateProgram(); glAttachShader(ID,vsId); glAttachShader(ID,fsId); glLinkProgram(ID);
        glDeleteShader(vsId); glDeleteShader(fsId);
    }
    void use() const { glUseProgram(ID); }
    GLuint id() const { return ID; }
    void setInt(const std::string& n,int v)const{ glUniform1i(glGetUniformLocation(ID,n.c_str()),v); }
    void setFloat(const std::string& n,float v)const{ glUniform1f(glGetUniformLocation(ID,n.c_str()),v); }
    void setVec3(const std::string& n,const glm::vec3& v)const{ glUniform3fv(glGetUniformLocation(ID,n.c_str()),1,glm::value_ptr(v)); }
    void setMat4(const std::string& n,const glm::mat4& m)const{ glUniformMatrix4fv(glGetUniformLocation(ID,n.c_str()),1,GL_FALSE,glm::value_ptr(m)); }
};
#endif
