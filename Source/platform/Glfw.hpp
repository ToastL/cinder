#pragma once

namespace cinder::platform {

class Glfw {
public:
    static void acquire();
    static void release();
    static void pollEvents();
    static void waitEvents();
    static double time();
};

class GlfwSession {
public:
    GlfwSession() { Glfw::acquire(); }
    ~GlfwSession() { Glfw::release(); }

    GlfwSession(const GlfwSession&) = delete;
    GlfwSession& operator=(const GlfwSession&) = delete;
};

}
