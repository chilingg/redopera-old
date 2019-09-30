#ifndef RTEXTLINE_H
#define RTEXTLINE_H

#include <RTexObject.h>

class RTextline : public RTexObject
{
public:
    RTextline(int width, int height);

protected:
    void renderControl(RShaderProgram *shader) override;

    GLuint texture;
};

#endif // RTEXTLINE_H
