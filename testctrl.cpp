#include "testctrl.h"
#include "constant.h"
#include <RDebug.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

TestCtrl::TestCtrl(RController *parent):
    RController(parent),
    VIEW_PROT_WIDTH(1600.0f),
    VIEW_PROT_HEIGHT(900.0f),
    move(0.0f, 0.0f),
    step(0.1f),
    ob(32, 32),
    ob2(255, 16),
    jumpPoint(0)
{
    RShader vertex(RE_PATH + "shaders/vertex.vert", RShader::VERTEX_SHADER);
    RShader fragment((RE_PATH + "shaders/fragment.frag"), RShader::FRAGMENT_SHADER);
    program.attachShader(vertex);
    program.attachShader(fragment);
    program.linkProgram();

    projection = glm::ortho(0.0f, VIEW_PROT_WIDTH, 0.0f, VIEW_PROT_HEIGHT, -1.0f, 1.0f);
    //projection = glm::mat4(1);

    //model = glm::translate(model, {16.0f/2, 9.0f/2, 0.0f});
    ob.setPosition(800, 0);
    ob2.setPosition(100, 200);

    //timer.start();
}

TestCtrl::~TestCtrl()
{

}

void TestCtrl::control()
{
    timer.elapsed(1.0/60.0);
    update();
    timer.start();
}

void TestCtrl::paintEvent()
{
    //FPS();

    glDisable(GL_CULL_FACE);
    program.use();
    program.setUniformMatrix4fv("projection", glm::value_ptr(projection));

    ob2.render(&program);

    ob.setColor(0, 0, 0);
    if(jumpPoint != 0)
    {
        if(jumpPoint == 1)
            ob.stop();
        --jumpPoint;
    }
    else if(ob.y() > 0.0f)
    {
        ob.move({0.0f, -1.0f}, 10);
        ob.setColor(255, 0, 0);
    }

    ob.move(move, 10);
    if(ob.checkCollision(ob2))
    {
        ob.stop();
        ob.move(-move, 20);
    }

    ob.render(&program);
}

void TestCtrl::keyPressEvent(RKeyEvent *event)
{
    //移动
    if(event->key() == RKeyEvent::KEY_RIGHT)
        move.x += 1.0f;
    if(event->key() == RKeyEvent::KEY_LEFT)
        move.x -= 1.0f;
    if(event->key() == RKeyEvent::KEY_UP)
    {
        jumpPoint = 10;
        ob.giveVelocity(0, 25);
    }
}

void TestCtrl::keyReleaseEvent(RKeyEvent *event)
{
    //移动
    if(event->key() == RKeyEvent::KEY_RIGHT)
        move.x -= 1.0f;
    if(event->key() == RKeyEvent::KEY_LEFT)
        move.x += 1.0f;
    if(event->key() == RKeyEvent::KEY_UP)
        ob.stop();
}

void TestCtrl::mousePressEvent(RMouseEvent *event)
{
    //RDebug() << event->x() << event->y();
}

void TestCtrl::mouseReleaseEvent(RMouseEvent *event)
{
    //RDebug() << event->x() << event->y();
}

void TestCtrl::FPS()
{
    static RTimer t;
    static int fps = 0;
    ++fps;
    if(t.elapsed() >= 1.0)
    {
        RDebug() << "fps:" << fps;//5000 1700
        fps = 0;
        t.start();
    }

}
