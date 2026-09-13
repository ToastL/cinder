#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_internal.h>

#include <cstdio>
#include <cstdlib>

void cinderProbe(GLFWwindow* window, int frame) {
    static const char* env = std::getenv("CINDER_PROBE");
    static const int mode = env != nullptr ? std::atoi(env) : 0;
    if (mode == 0) return;

    ImGuiIO& io = ImGui::GetIO();
    if (frame == 2) {
        glfwSetCursorPosCallback(window, nullptr);
        glfwSetCursorEnterCallback(window, nullptr);
        glfwSetMouseButtonCallback(window, nullptr);
        glfwSetScrollCallback(window, nullptr);
        glfwSetKeyCallback(window, nullptr);
        glfwSetCharCallback(window, nullptr);
        glfwSetWindowFocusCallback(window, nullptr);
        ImGui_ImplGlfw_CursorEnterCallback(window, GLFW_TRUE);
        io.AddMousePosEvent(640.0f, 250.0f);
    }

    const int button = mode == 2 ? 1 : 0;
    if (mode == 4 && frame == 4) io.AddKeyEvent(ImGuiMod_Alt, true);
    if (frame == 5) io.AddMouseButtonEvent(button, true);
    if (mode == 3) {
        if (frame == 7) io.AddMouseButtonEvent(button, false);
        if (frame == 9) io.AddKeyEvent(ImGuiKey_S, true);
        if (frame == 69) io.AddKeyEvent(ImGuiKey_S, false);
    } else {
        if (frame >= 7 && frame < 27) io.AddMousePosEvent(640.0f + (frame - 6) * 10.0f, 250.0f);
        if (frame == 30) io.AddMouseButtonEvent(button, false);
        if (mode == 4 && frame == 31) io.AddKeyEvent(ImGuiMod_Alt, false);
    }
    if (frame == 3 || frame == 6 || frame == 8 || frame == 10 || frame == 12 || frame == 40 || frame == 68) {
        ImGuiWindow* nav = GImGui->NavWindow;
        std::printf("[probe] f%d ctrl=%d super=%d shift=%d alt=%d S=%d nav=%s glfwSuper=%d glfwCtrl=%d\n",
                    frame, io.KeyCtrl, io.KeySuper, io.KeyShift, io.KeyAlt,
                    ImGui::IsKeyDown(ImGuiKey_S), nav != nullptr ? nav->Name : "null",
                    glfwGetKey(window, GLFW_KEY_LEFT_SUPER), glfwGetKey(window, GLFW_KEY_LEFT_CONTROL));
    }
    if (frame == 80) std::printf("[probe] mode %d done, alt=%d\n", mode, io.KeyAlt ? 1 : 0);
}
