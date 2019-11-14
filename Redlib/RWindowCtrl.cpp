#include "RWindowCtrl.h"
#include "RResource/RResource.h"
#include "RResource/RImage.h"

#include "RDebug.h"

int RWindowCtrl::count = 0;
bool RWindowCtrl::vSync_ = true;

RWindowCtrl::RWindowCtrl(const std::string &name, RController *parent):
    RController(name, parent),
    window_(nullptr)
{
    if(++count == 1)
    {
        //glfw错误回调
        glfwSetErrorCallback(glfwErrorCallback);
        //初始化GLFW
        if(!glfwInit())
        {
            printError("Failed to initialize GLFW");
            parentToNull();
            exit(EXIT_FAILURE);
        }
        DefaultWindow();
        //加载手柄映射
        updataGamepadMappings(":/data/gamecontrollerdb.txt");
        //手柄连接回调
        glfwSetJoystickCallback(joystickPresentCallback);

        //需手动检测一次手柄连接，检测之前已连接的手柄
        for(int i = RInputEvent::joystick1; i <= RInputEvent::joystickMaxNum; ++i)
        {
            if(glfwJoystickIsGamepad(i))
                gamepads.insert(RInputEvent::toJoystickID(i));
        }
    }

    //同线程窗口统一共享上下文
    GLFWwindow *share = glfwGetCurrentContext();
    window_ = glfwCreateWindow(width_, height_, "Redopera", nullptr, share);
    if(!window_)
    {
        printError("Fainled to create GLFW window!");
        parentToNull();
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    //绑定上下文与this指针
    glfwSetWindowUserPointer(window_, this);
    glfwGetWindowSize(window_, &width_, &height_);

    //如果当前线程之前没有窗口创建，则将该context设置为当前线程主context
    if(!share)
    {
        glfwMakeContextCurrent(window_);
        if(count == 1)
        {
            //初始化glad
            if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
            {
                printError("Failed to initialize GLAD");
                parentToNull();
                glfwTerminate();
                exit(EXIT_FAILURE);
            }
#ifndef R_NO_DEBUG
            RDebug() << glGetString(GL_VERSION);
            //若启用OpenGL Debug
            GLint flags;
            glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
            if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
            {
                RDebug() << "Enable OpenGL debug output";
                glEnable(GL_DEBUG_OUTPUT);
                glDebugMessageCallback(openglDebugMessageCallback, nullptr);
            }
#endif
        }
        //glViewport(0, 0, width_, height_);
        //设置混合函数 Ps:混合需另外开启
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //默认开启垂直同步
        glfwSwapInterval(1);
    }

    glfwSetFramebufferSizeCallback(window_, resizeCallback);
    glfwSetKeyCallback(window_, keyboardCollback);
    glfwSetMouseButtonCallback(window_, mouseButtonCallback);
    glfwSetScrollCallback(window_, mouseScrollCallback);
    glfwSetCursorPosCallback(window_, mouseMoveCallback);
    glfwSetWindowFocusCallback(window_, windowFocusCallback);

    RImage img = RImage::getRedoperaIcon();
    GLFWimage icon{ img.width(), img.height(), img.data() };
    glfwSetWindowIcon(window_, 1, &icon);
    //GLFW事件触发
    poolEvent = &glfwPollEvents;
}

RWindowCtrl::~RWindowCtrl()
{
    assert(window_);
    glfwDestroyWindow(window_);
}

void RWindowCtrl::control()
{
    GLFWwindow *before = glfwGetCurrentContext();
    glfwMakeContextCurrent(window_);

    if(focused_)
    {
        //更新手柄输入
        for(auto jid = gamepads.begin(); jid != gamepads.end();)
            inputs.updateGamepadButtonInput(*jid++);
        //发布输入事件
        dispatchEvent(&inputs);
    }

    //清除颜色缓存
    glClearBufferfv(GL_COLOR, 0, backgroundColor);
    //调动子结点控制
    allChildrenActive();
    glfwSwapBuffers(window_);

    //切换为之前的上下文
    glfwMakeContextCurrent(before);

    if(glfwWindowShouldClose(window_))
        inactive();
}

void RWindowCtrl::setWindowSize(int width, int height)
{
    glfwSetWindowSize(window_, width, height);
}

void RWindowCtrl::setWindowTitle(const std::string &title)
{
    glfwSetWindowTitle(window_, title.c_str());
}

void RWindowCtrl::setBackground(int r, int g, int b)
{
    backgroundColor[0] = r / 255.0f;
    backgroundColor[1] = g / 255.0f;
    backgroundColor[2] = b / 255.0f;
}

void RWindowCtrl::setViewportRatio(double ratio)
{
    viewportRatio_ = ratio;
}

void RWindowCtrl::setViewportPattern(RWindowCtrl::ViewportPattern pattern)
{
    viewportPattern = pattern;
}

void RWindowCtrl::setVSync(bool enable)
{
    vSync_ = enable ? 1 : 0;//参数为1锁60帧
    glfwSwapInterval(vSync_);
}

void RWindowCtrl::setFullScreenWindow(bool b)
{
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *vidmode = glfwGetVideoMode(monitor);
    if(b)
    {
        glfwSetWindowMonitor(window_, monitor, 0, 0, vidmode->width, vidmode->height, vidmode->refreshRate);
    } else
    {
        glfwSetWindowMonitor(window_, nullptr, (vidmode->width-960)/2, (vidmode->height-540)/2,
                             960, 540, vidmode->refreshRate);
    }
}

void RWindowCtrl::setWindowSizeLimits(int minW, int minH, int maxW, int maxH)
{
    if(minW < 1) minW = GLFW_DONT_CARE;
    if(minH < 1) minH = GLFW_DONT_CARE;
    if(maxW < 1) maxW = GLFW_DONT_CARE;
    if(maxH < 1) maxH = GLFW_DONT_CARE;

    glfwSetWindowSizeLimits(window_, minW, minH, maxW, maxH);
}

void RWindowCtrl::setWindowSizeFixed(bool b)
{
    glfwSetWindowAttrib(window_, GLFW_RESIZABLE, b ? GLFW_FALSE : GLFW_TRUE);
}

void RWindowCtrl::setWindowDecrate(bool b)
{
    glfwSetWindowAttrib(window_, GLFW_DECORATED, b ? GLFW_TRUE: GLFW_FALSE);
}

void RWindowCtrl::setWindowFloatOnTop(bool b)
{
    glfwSetWindowAttrib(window_, GLFW_FLOATING, b ? GLFW_TRUE: GLFW_FALSE);
}

double RWindowCtrl::getViewportRatio() const
{
    return viewportRatio_;
}

bool RWindowCtrl::isShouldCloused() const
{
    glfwWindowShouldClose(window_);
}

void RWindowCtrl::DefaultWindow()
{
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);//set主版本号
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);//set副版本号
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);//删除当前版本不推荐使用的功能
#ifndef R_NO_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);//OpenGL的Debug输出
#endif
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);//创建窗口时初始不可见
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);//use核心模式
    //glfwWindowHint(GLFW_DECORATED, enable ? GLFW_TRUE : GLFW_FALSE);//边框与标题栏
}

void RWindowCtrl::updataGamepadMappings(std::string path)
{
    try{
        std::string mappingCode = RResource::getTextFileContent(path);
        glfwUpdateGamepadMappings(mappingCode.c_str());
    }
    catch(...)
    {
        printError("Failed to updata gamepad mapping! In path: " + path + '\n' +
                   "To https://github.com/gabomdq/SDL_GameControllerDB download gamecontrollerdb.txt file.");
        //加载内置的手柄映射
        std::string mappingCode = std::string() + RInputEvent::gamepadMappingCode0
                + RInputEvent::gamepadMappingCode1 + RInputEvent::gamepadMappingCode2;
        glfwUpdateGamepadMappings(mappingCode.c_str());
    }
}

void RWindowCtrl::closeWindow()
{
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void RWindowCtrl::trackCursor()
{
    glfwSetCursorPosCallback(window_, mouseMoveCallback);
}

void RWindowCtrl::untrackCursor()
{
    glfwSetCursorPosCallback(window_, [](GLFWwindow *, double , double) {});
}

void RWindowCtrl::showWindow()
{
    glfwShowWindow(window_);
}

void RWindowCtrl::hideWindow()
{
    glfwHideWindow(window_);
}

std::string RWindowCtrl::getDefaultName() const
{
    return "WindowCtrl";
}

void RWindowCtrl::initEvent(RInitEvent *event)
{
#ifndef R_NO_DEBUG
    if(!glfwGetWindowAttrib(window_, GLFW_VISIBLE))
        RDebug() << getName() << "Window is hide!";
#endif
}

void RWindowCtrl::closeEvent(RCloseEvent *event)
{
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

RWindowCtrl *RWindowCtrl::getWindowUserCtrl(GLFWwindow *window)
{
    return static_cast<RWindowCtrl*>(glfwGetWindowUserPointer(window));
}

void RWindowCtrl::glfwErrorCallback(int error, const char *description)
{
    printError("Error " + std::to_string(error) + ": " + description);
}

void RWindowCtrl::openglDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                             GLsizei length, const GLchar *message, const void *userParam)
{
    std::string sourceStr;
    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             sourceStr = "Source: API "; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   sourceStr = "Source: Window System "; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "Source: Shader Compiler "; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     sourceStr = "Source: Third Party "; break;
        case GL_DEBUG_SOURCE_APPLICATION:     sourceStr = "Source: Application "; break;
        case GL_DEBUG_SOURCE_OTHER:           sourceStr = "Source: Other "; break;
    }

    std::string typeStr;
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               typeStr = "Type: Error "; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "Type: Deprecated Behaviour "; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "Type: Undefined Behaviour "; break;
        case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "Type: Portability "; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "Type: Performance "; break;
        case GL_DEBUG_TYPE_MARKER:              typeStr = "Type: Marker "; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          typeStr = "Type: Push Group "; break;
        case GL_DEBUG_TYPE_POP_GROUP:           typeStr = "Type: Pop Group "; break;
        case GL_DEBUG_TYPE_OTHER:               typeStr = "Type: Other "; break;
    }

    std::string severityStr;
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         severityStr = "Severity: high "; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       severityStr = "Severity: medium "; break;
        case GL_DEBUG_SEVERITY_LOW:          severityStr = "Severity: low "; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: severityStr = "Severity: notification "; break;
    }
    RDebug() << sourceStr << typeStr << severityStr;
    RDebug() << "Debug message ID: " << id << ": " << message;
}

void RWindowCtrl::joystickPresentCallback(int jid, int event)
{
    RInputEvent::JoystickID J = RInputEvent::toJoystickID(jid);
    bool isConnected = event == RInputEvent::joystickConnected ? true : false;
    //必需转换为RWindowCtrl才能使用protected发布函数
    RWindowCtrl *wctrl = static_cast<RWindowCtrl*>(getFreeTree());

    if(glfwJoystickIsGamepad(jid))//断开的JID无法通过
    {
        gamepads.insert(RInputEvent::toJoystickID(jid));
        RjoystickPresentEvent e(RInputEvent::toJoystickID(jid), isConnected);
        wctrl->dispatchEvent(&e);
    }
    else if(gamepads.find(J) != gamepads.end())
    {
        gamepads.erase(RInputEvent::toJoystickID(jid));
        RjoystickPresentEvent e(RInputEvent::toJoystickID(jid), isConnected);
        wctrl->dispatchEvent(&e);
    }
    //不是手柄的JID忽视
}

void RWindowCtrl::resizeCallback(GLFWwindow *window, int width, int height)
{
    RWindowCtrl *wctrl = getWindowUserCtrl(window);

    if(wctrl->viewportPattern == FullWindow)
    {
        wctrl->width_ = width;
        wctrl->height_ = height;
        glViewport(0, 0, width, height);
        RResizeEvent e(width, height);
        wctrl->dispatchEvent(&e);
    }
    else if(wctrl->viewportPattern == KeepScale)
    {
        double ratio = static_cast<double>(width) / height;
        int newW = width;
        int newH = height;
        if(ratio > wctrl->viewportRatio_)
        {
            newW = height * wctrl->viewportRatio_;
            //glViewport((width - newW) / 2.0, 0, newW, newH);
        }
        else
        {
            newH = width / wctrl->viewportRatio_;
            glViewport(0, (height - newH) / 2.0, newW, newH);
        }
        wctrl->width_ = newW;
        wctrl->height_ = newH;
        RResizeEvent e(newW, newH);
        wctrl->dispatchEvent(&e);
    }
}

void RWindowCtrl::mouseMoveCallback(GLFWwindow *window, double xpos, double ypos)
{
    RWindowCtrl *wctrl = getWindowUserCtrl(window);
    wctrl->inputs.updateMouseInput(RInputEvent::Mouse_None, RPoint(xpos, ypos));
}

void RWindowCtrl::keyboardCollback(GLFWwindow *window, int key, int , int action, int )
{
    RWindowCtrl *wctrl = getWindowUserCtrl(window);
    wctrl->inputs.updateKeyboardInput(RInputEvent::toKeyboards(key), RInputEvent::toButtonAction(action));
}

void RWindowCtrl::mouseButtonCallback(GLFWwindow *window, int button, int action, int )
{
    RWindowCtrl *wctrl = getWindowUserCtrl(window);
    RPoint p;
    if(action != RInputEvent::RELEASE)
    {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        p.setPoint(x, y);
    }
    wctrl->inputs.updateMouseInput(RInputEvent::toMouseButtons(button), p);
}

void RWindowCtrl::mouseScrollCallback(GLFWwindow *window, double x, double y)
{
    RWindowCtrl *wctrl = getWindowUserCtrl(window);
    wctrl->scrolled.emit(y);
}

void RWindowCtrl::windowFocusCallback(GLFWwindow *window, int focused)
{
    RWindowCtrl *wctrl = getWindowUserCtrl(window);
    wctrl->focused_ = focused;
}
