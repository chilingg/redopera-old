#include "rshader.h"
#include "rdebug.h"

RShader::RShader()
{

}

RShader::RShader(const GLchar *shaderPath, GLenum type)
{
    compileShader(shaderPath, type);
}

bool RShader::compileShader(const GLchar *path, GLenum type)
{
    int success;
    char infoLog[512];

    std::string code = openTextFile(path);
    const char* shaderCode = code.c_str();
    //创建编译顶点着色器
    shaderID = glCreateShader(type);
    glShaderSource(shaderID, 1, &shaderCode, nullptr);
    glCompileShader(shaderID);
    valid = true;
    //若有错误，则打印
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        valid = false;
        glGetShaderInfoLog(shaderID, 512, nullptr, infoLog);
        std::string shaderType;
        if(type == GL_VERTEX_SHADER)
            shaderType = "Vertex";
        else if(type == GL_FRAGMENT_SHADER)
            shaderType = "Fragment";
        else if(type == GL_GEOMETRY_SHADER)
            shaderType = "Geometry";
        else
            shaderType = "Uknow";

        RDebug() << "Eroor: " << shaderType << "shader compilation failed!";
        exit(EXIT_FAILURE);
    }

    return valid;
}
