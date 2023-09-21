#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class GraphicsEngine {
public:

    GraphicsEngine();
    ~GraphicsEngine();

private:
    // TODO: move window to something else?
    // let the engine just be the interface to vulkan
    // and make the game loop outside, such that there is a draw function that can be called
    const uint32_t width = 640;
    const uint32_t height = 480;
    GLFWwindow* window;

    VkInstance instance;

    void createWindow();
    void createInstance();
};