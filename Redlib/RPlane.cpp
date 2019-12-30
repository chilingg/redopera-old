#include "RPlane.h"

#include "RDebug.h"

RShaderProgram *RPlane::lineBoxsProgram;
GLuint RPlane::lineBoxVAO = 0;
std::map<std::thread::id, RPlane::PlaneData> RPlane::threadData;

RPlane::RPlane(const RPlane &plane):
    rotateMat_(plane.rotateMat_),
    modelMat_(plane.modelMat_),
    shaders_(plane.shaders_),
    texture_(plane.texture_),
    modelLoc_(plane.modelLoc_),
    name_(plane.name_),
    width_(plane.width_),
    height_(plane.height_),
    pos_(plane.pos_),
    dirty_(plane.dirty_),
    flipH_(plane.flipH_),
    flipV_(plane.flipV_),
    marginTop_(plane.marginTop_),
    marginBottom_(plane.marginBottom_),
    marginLeft_(plane.marginLeft_),
    marginRight_(plane.marginRight_),
    paddingTop_(plane.paddingTop_),
    paddingBottom_(plane.paddingBottom_),
    paddingLeft_(plane.paddingLeft_),
    paddingRight_(plane.paddingRight_),
    sizeMode_(plane.sizeMode_),
    vAlign_(plane.vAlign_),
    hAlign_(plane.hAlign_),
    minWidth_(plane.minWidth_),
    minHeight_(plane.minHeight_),
    maxWidth_(plane.maxWidth_),
    maxHeight_(plane.maxHeight_)
{
    registration();
}

RPlane::RPlane():
    RPlane(32, 32, "Plane", RPoint(0, 0))
{

}

RPlane::RPlane(int width, int height, const std::string &name, const RPoint &pos):
    rotateMat_(1.0f),
    modelMat_(1.0f),
    shaders_(),
    texture_(),
    name_(name),
    width_(width),
    height_(height),
    pos_(pos)
{
    registration();

    setColorTexture(0xffffffff);
    texture_.rename(this->name() + "-ColorTexture");
}

RPlane::RPlane(int width, int height, const RTexture &tex, const RShaderProgram &prog, const std::string &name, const RPoint &pos):
    rotateMat_(1.0f),
    modelMat_(1.0f),
    shaders_(prog),
    texture_(tex),
    modelLoc_(prog.getUniformLocation("model")),
    name_(name),
    width_(width),
    height_(height),
    pos_(pos)
{
    registration();
}

RPlane::~RPlane()
{
    if(unregistration() < 1)
    {
        GLuint VAO = getThreadVAO(), VBO = getThreadVBO();
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
        threadData.erase(std::this_thread::get_id());
    }
    //if(--count == 0)
    //{
        //delete planeSProgram;
        //glDeleteBuffers(1, &planeVBO);
        //glDeleteVertexArrays(1, &planeVAO);
#ifdef R_DEBUG
        //if(lineBoxVAO)
        //{
            //delete lineBoxsProgram;
            //glDeleteVertexArrays(1, &lineBoxVAO);
        //}
#endif
    //}
}

void RPlane::setSize(int width, int height)
{
    width = RMath::max(width, minWidth_);
    width = RMath::min(width, maxWidth_);
    height = RMath::max(height, minHeight_);
    height = RMath::min(height, maxHeight_);

    width_ = width; height_ = height;
    updateModelMat();
}

void RPlane::setSize(const RSize &size)
{
    int width = size.width();
    int height = size.height();
    width = RMath::max(width, minWidth_);
    width = RMath::min(width, maxWidth_);
    height = RMath::max(height, minHeight_);
    height = RMath::min(height, maxHeight_);

    width_ = width;
    height_ = height;
    updateModelMat();
}

void RPlane::setWidth(int width)
{
    width = RMath::max(width, minWidth_);
    width = RMath::min(width, maxWidth_);
    width_ = width;
    updateModelMat();
}

void RPlane::setHeight(int height)
{
    height = RMath::max(height, minHeight_);
    height = RMath::min(height, maxHeight_);
    height_ = height;
    updateModelMat();
}

void RPlane::setMinimumSize(int minW, int minH)
{
    minW = RMath::min(minW, maxWidth_);
    minH = RMath::min(minH, maxHeight_);
    minWidth_ = minW;
    minHeight_ = minH;

    if(width_ < minW) width_ = minW;
    if(height_ < minH) height_ = minH;
}

void RPlane::setMaximumSize(int maxW, int maxH)
{
    maxW = RMath::max(maxW, minWidth_);
    maxH = RMath::max(maxH, minHeight_);
    maxWidth_ = maxW;
    maxHeight_ = maxH;

    if(width_ > maxW) width_ = maxW;
    if(height_ > maxH) height_ = maxH;
}

void RPlane::setPosition(const RPoint &pos)
{
    pos_ = pos;
}

void RPlane::setPosition(int x, int y, int z)
{
    pos_ = RPoint(x, y, z);
}

void RPlane::setPositionX(int x)
{
    pos_.setX(x);
}

void RPlane::setPositionY(int y)
{
    pos_.setY(y);
}

void RPlane::setPositionZ(int z)
{
    pos_.setZ(z);
}

void RPlane::setOuterPosition(int x, int y, int z)
{
    pos_ = RPoint(x+marginLeft_, y+marginBottom_, z);
}

void RPlane::setOuterPositionX(int value)
{
    pos_.setX(value+marginLeft_);
}

void RPlane::setOuterPositionY(int value)
{
    pos_.setY(value+marginBottom_);
}

void RPlane::setMargin(int top, int bottom, int left, int right)
{
    marginTop_ = top; marginBottom_ = bottom; marginLeft_ = left; marginRight_ = right;
}

void RPlane::setMargin(int value)
{
    marginTop_ = value; marginBottom_ = value; marginLeft_ = value; marginRight_ = value;
}

void RPlane::setPadding(int top, int bottom, int left, int right)
{
    paddingTop_ = top; paddingBottom_ = bottom; paddingLeft_ = left; paddingRight_ = right;
    updateModelMat();
}

void RPlane::setPadding(int value)
{
    paddingTop_ = value; paddingBottom_ = value; paddingLeft_ = value; paddingRight_ = value;
    updateModelMat();
}

void RPlane::setColorTexture(const RColor &color)
{
    R_RGBA rgba = color.rgba();
    setColorTexture(rgba);
}

void RPlane::setColorTexture(R_RGBA rgba)
{
    const unsigned char *colorData = reinterpret_cast<unsigned char*>(&rgba);
    texture_.setTexFilter(RTexture::Nearest);
    texture_.generate(1, 1, 4, colorData, 4);
}

void RPlane::setColorTexture(unsigned r, unsigned g, unsigned b, unsigned a)
{
    RColor color(r, g, b, a);
    setColorTexture(color.rgba());
}

void RPlane::setTexture(const RImage &image)
{
    texture_.generate(image);
    if(sizeMode_ != Auto) updateModelMat();
}

void RPlane::setTexture(const RTexture &texture)
{
    texture_ = texture;
    if(sizeMode_ != Auto) updateModelMat();
}

void RPlane::setSizeMode(RPlane::SizeMode mode)
{
    sizeMode_ = mode;
    updateModelMat();
}

void RPlane::setAlignment(RPlane::Alignment hAlign, RPlane::Alignment vAlign)
{
#ifdef R_DEBUG
    if(printError(hAlign == Align_Top || hAlign == Align_Bottom, "Wrong alignment! in " + name_))
        return;
    if(printError(vAlign == Align_Left || vAlign == Align_Right, "Wrong alignment! in " + name_))
        return;
#endif
    hAlign_ = hAlign; vAlign_ = vAlign;
    updateModelMat();
}

void RPlane::setShaderProgram(const RShaderProgram &program, const RUniformLocation &modelLoc)
{
    shaders_ = program;
    modelLoc_ = modelLoc;
}

void RPlane::setShaderProgram(const RShaderProgram &program, const std::string modelName)
{
    shaders_ = program;
    modelLoc_ = program.getUniformLocation(modelName);
}

void RPlane::rename(std::string name)
{
    name_.swap(name);
}

void RPlane::rotateX(float value)
{
    rotateMat_ = RMath::rotate(RMatrix4(1), value, {1.0f, 0.0f, 0.0f});
    updateModelMat();
}

void RPlane::rotateY(float value)
{
    rotateMat_ = RMath::rotate(RMatrix4(1), value, {0.0f, 1.0f, 0.0f});
    updateModelMat();
}

void RPlane::rotateZ(float value)
{
    rotateMat_ = RMath::rotate(RMatrix4(1), value, {0.0f, 0.0f, 1.0f});
    updateModelMat();
}

void RPlane::render()
{
    glBindVertexArray(getThreadVAO());
    shaders_.use();

    if(dirty_) updateModelMatNow();
    RMatrix4 modelMat = RMath::translate(RMatrix4(1), {pos_.x(), pos_.y(), pos_.z()}) * modelMat_;
    shaders_.setUniformMatrix(modelLoc_, 4, RMath::value_ptr(modelMat));

    texture_.bind();

    renderControl();

    shaders_.nonuse();
    glBindVertexArray(0);
}

void RPlane::render(RMatrix4 &modelMat)
{
    glBindVertexArray(getThreadVAO());
    shaders_.use();

    if(dirty_) updateModelMatNow();
    shaders_.setUniformMatrix(modelLoc_, 4, RMath::value_ptr(modelMat));

    texture_.bind();

    renderControl();

    shaders_.nonuse();
    glBindVertexArray(0);
}

void RPlane::renderUseSizeModel(RMatrix4 modelMat)
{
    glBindVertexArray(getThreadVAO());
    shaders_.use();

    if(dirty_) updateModelMatNow();
    modelMat = modelMat * modelMat_;
    shaders_.setUniformMatrix(modelLoc_, 4, RMath::value_ptr(modelMat));

    texture_.bind();

    renderControl();

    shaders_.nonuse();
    glBindVertexArray(0);
}

void RPlane::renderUsePositionAndSizeModel(RMatrix4 modelMat)
{
    glBindVertexArray(getThreadVAO());
    shaders_.use();

    if(dirty_) updateModelMatNow();
    modelMat = modelMat * RMath::translate(RMatrix4(1), {pos_.x(), pos_.y(), pos_.z()}) * modelMat_;
    shaders_.setUniformMatrix(modelLoc_, 4, RMath::value_ptr(modelMat));

    texture_.bind();

    renderControl();

    shaders_.nonuse();
    glBindVertexArray(0);
}

void RPlane::flipH()
{
    flipH_ = true;
    //简化的右乘一个翻转矩阵
    modelMat_[0][0] *= -1;
    modelMat_[0][1] *= -1;
    modelMat_[0][2] *= -1;
    modelMat_[0][3] *= -1;
}

void RPlane::flipV()
{
    flipV_ = true;
    //简化的右乘一个翻转矩阵
    modelMat_[1][0] *= -1;
    modelMat_[1][1] *= -1;
    modelMat_[1][2] *= -1;
    modelMat_[1][3] *= -1;
}

#ifdef R_DEBUG
void RPlane::renderLineBox(const RMatrix4 &projection, const RMatrix4 &view)
{
    static RUniformLocation posLoc;
    static RUniformLocation sizeLoc;

    if(!lineBoxVAO)
    {
        const GLchar *vertexCode = {
            "#version 430 core\n"
            "uniform vec4 aPos[3];\n"
            "uniform vec2 aSize[3];\n"
            "out vec2 size;\n"
            "out vec3 color;\n"
            "void main(void)\n"
            "{\n"
                "color = vec3(0, 0, 0);\n"
                "switch(gl_InstanceID)\n"
                "{\n"
                    "case 0: color.r = 1; break;\n"
                    "case 1: color.g = 1; break;\n"
                    "case 2: color.b = 1; break;\n"
                "}\n"
                "size = aSize[gl_InstanceID];\n"
                "gl_Position = aPos[gl_InstanceID];\n"
            "};\n"
        };
        const GLchar *geomCode = {
            "#version 430 core\n"
            "layout(points) in;\n"
            "layout(line_strip, max_vertices = 5) out;\n"
            "in vec2 size[1];\n"
            "in vec3 color[1];\n"
            "out vec3 lineColor;\n"
            "void main(void)\n"
            "{\n"
                "lineColor = color[0];\n"
                "vec4 position = gl_in[0].gl_Position;\n"
            "\n"
                "gl_Position = position;\n"
                "EmitVertex();\n"
                "gl_Position = position + vec4(0.0, size[0].y, 0.0, 0.0);\n"
                "EmitVertex();\n"
                "gl_Position = position + vec4(size[0], 0.0, 0.0);\n"
                "EmitVertex();\n"
                "gl_Position = position + vec4(size[0].x, 0.0, 0.0, 0.0);\n"
                "EmitVertex();\n"
                "gl_Position = position;\n"
                "EmitVertex();\n"
            "\n"
                "EndPrimitive();\n"
            "}\n"
        };
        const GLchar *fragCode = {
            "#version 430 core\n"
            "in vec3 lineColor;\n"
            "out vec4 outColor;\n"
            "void main(void)\n"
            "{\n"
                "outColor = vec4(lineColor, 1.0);\n"
            "}\n"
        };
        RShader lineBoxV;
        lineBoxV.compileShaderCode(vertexCode, ShaderType::VertexShader);
        RShader lineBoxG;
        lineBoxG.compileShaderCode(geomCode, ShaderType::GeometryShader);
        RShader lineBoxF;
        lineBoxF.compileShaderCode(fragCode, ShaderType::FragmentShader);
        lineBoxsProgram = new RShaderProgram;
        lineBoxsProgram->rename("LineBoxProgram");
        lineBoxsProgram->attachShader(lineBoxV);
        lineBoxsProgram->attachShader(lineBoxF);
        lineBoxsProgram->attachShader(lineBoxG);
        lineBoxsProgram->linkProgram();
        glGenVertexArrays(1, &lineBoxVAO);
        lineBoxsProgram->use();
        posLoc = lineBoxsProgram->getUniformLocation("aPos");
        sizeLoc = lineBoxsProgram->getUniformLocation("aSize");
    }

    glBindVertexArray(lineBoxVAO);
    lineBoxsProgram->use();

    RVector4 vec[3];

    float lineBoxs[3][2];
    //外边距框
    vec[0].x = outerWidth();
    vec[0].y = outerHeight();
    vec[0].z = pos_.z();
    vec[0].w = 1.0;
    vec[0] = projection * vec[0];
    lineBoxs[0][0] = vec[0].x - -1.0f;
    lineBoxs[0][1] = vec[0].y - -1.0f;
    //实际边框
    vec[0].x = width_;
    vec[0].y = height_;
    vec[0].z = pos_.z();
    vec[0].w = 1.0;
    vec[0] = projection * vec[0];
    lineBoxs[1][0] = vec[0].x - -1.0f;
    lineBoxs[1][1] = vec[0].y - -1.0f;
    //内边距框
    vec[0].x = innerWidth();
    vec[0].y = innerHeight();
    vec[0].z = pos_.z();
    vec[0].w = 1.0;
    vec[0] = projection * vec[0];
    lineBoxs[2][0] = vec[0].x - -1.0f;
    lineBoxs[2][1] = vec[0].y - -1.0f;

    vec[0] = {pos_.x()-marginLeft_, pos_.y()-marginBottom_, pos_.z(), 1.0f};
    vec[1] = {pos_.x(), pos_.y(), pos_.z(), 1.0f};
    vec[2] = {pos_.x()+paddingLeft_, pos_.y()+paddingBottom_, pos_.z(), 1.0f};

    vec[0] = projection * view * vec[0];
    vec[1] = projection * view * vec[1];
    vec[2] = projection * view * vec[2];
    lineBoxsProgram->setUniform(posLoc, 4, &vec[0].x, 3);
    lineBoxsProgram->setUniform(sizeLoc, 2, &lineBoxs[0][0], 3);
    //平面的三个线框
    glDrawArraysInstanced(GL_POINTS, 0, 1, 3);

    lineBoxsProgram->nonuse();
}

void RPlane::renderLineBox(int left, int right, int buttom, int top, RPoint pos)
{
    RMatrix4 projection = RMath::ortho(left*1.0f, right*1.0f, buttom*1.0f, top*1.0f);
    RMatrix4 view = RMath::translate(RMatrix4(1), { pos.x(), pos.y(), 0 });
    renderLineBox(projection, view);
}

GLuint RPlane::getThreadVAO()
{
    auto it = threadData.find(std::this_thread::get_id());
    assert(it != threadData.end());
    return it->second.VAO;
}

GLuint RPlane::getThreadVBO()
{
    auto it = threadData.find(std::this_thread::get_id());
    assert(it != threadData.end());
    return it->second.VBO;
}

int RPlane::getThreadUser()
{
    auto it = threadData.find(std::this_thread::get_id());
    assert(it != threadData.end());
    return it->second.VBO;
}

void RPlane::renderControl()
{
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void RPlane::updateModelMat()
{
    dirty_ = true;
}

void RPlane::updateModelMatOver()
{
    dirty_ = false;
}

void RPlane::registration()
{
    auto it = threadData.find(std::this_thread::get_id());
    if(it == threadData.end())
    {
        GLuint planeVAO, planeVBO;

        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);

        float plane[24]{
                0.5f,-0.5f, 0.0f, 1.0f, 0.0f,//右下
                0.5f, 0.5f, 0.0f, 1.0f, 1.0f,//右上
               -0.5f,-0.5f, 0.0f, 0.0f, 0.0f,//左下
               -0.5f, 0.5f, 0.0f, 0.0f, 1.0f,//左上
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), offsetBuffer(3*sizeof(float)));
        glBindVertexArray(0);

        it = threadData.emplace(std::this_thread::get_id(), PlaneData{planeVAO, planeVBO, 0}).first;
    }
    ++it->second.user;
}

int RPlane::unregistration()
{
    auto it = threadData.find(std::this_thread::get_id());
    assert(it != threadData.end());
    return --it->second.user;
}

void RPlane::updateModelMatNow()
{
    float w, h, tw = texture_.width(), th = texture_.height();
    float min = innerWidth() / tw*1.f;
    float max = innerHeight() / th*1.f;
    if(min > max) std::swap(min, max);

    switch(sizeMode_)
    {
    case Fixed:
        w = tw;
        h = th;
        break;
    case Auto:
        w = innerWidth();
        h = innerHeight();
        break;
    case Cover:
        w = tw * max;
        h = th * max;
        break;
    case Contain:
        w = tw * min;
        h = th * min;
        break;
    }

    float x, y;
    switch(hAlign_)
    {
    case Align_Left:
        x = w/2.0f + paddingLeft_;
        break;
    case Align_Mind:
        x = width_/2.0f;
        break;
    case Align_Right:
        x = width_ - w/2.0f - paddingRight_;
        break;
    default:
        x = width_/2.0f;
        break;
    }
    switch(vAlign_)
    {
    case Align_Bottom:
        y = h/2.0f + paddingBottom_;
        break;
    case Align_Mind:
        y = height_/2.0f;
        break;
    case Align_Top:
        y = height_ - h/2.0f - paddingTop_;
        break;
    default:
        y = height_/2.0f;
        break;
    }
    modelMat_ = RMatrix4(1);
    modelMat_ = RMath::translate(modelMat_, {x, y, 0});
    modelMat_ *= rotateMat_;
    modelMat_ = RMath::scale(modelMat_, {w, h, 1.0f});

    //简化的右乘一个翻转矩阵
    if(flipH_)
    {
        modelMat_[0][0] *= -1;
        modelMat_[0][1] *= -1;
        modelMat_[0][2] *= -1;
        modelMat_[0][3] *= -1;
    }
    if(flipV_)
    {
        modelMat_[1][0] *= -1;
        modelMat_[1][1] *= -1;
        modelMat_[1][2] *= -1;
        modelMat_[1][3] *= -1;
    }
    dirty_ = false;
}
#endif
