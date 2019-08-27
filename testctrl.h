#ifndef TESTCTRL_H
#define TESTCTRL_H

#include <RShaderProgram.h>
#include <RController.h>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <RObject.h>
#include <RTimer.h>

class TestCtrl : public RController
{
public:
    TestCtrl(RController *parent = nullptr);
    ~TestCtrl() override;

protected:
    void control() override;
    void paintEvent() override;
    void keyPressEvent(RKeyEvent *event) override;
    void keyReleaseEvent(RKeyEvent *event) override;
    void mousePressEvent(RMouseEvent *event) override;
    void mouseReleaseEvent(RMouseEvent *event) override;

private:
    void FPS();

    const float VIEW_PROT_WIDTH;
    const float VIEW_PROT_HEIGHT;
    glm::vec2 move;
    float step;
    RObject ob;
    RObject ob2;
    int jumpPoint;

    RTimer timer;
    RShaderProgram program;
    glm::mat4 model;
    glm::mat4 projection;
};

#endif // TESTCTRL_H
