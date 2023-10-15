#pragma once

#include <vector>
#include <optional>
#include <memory>
#include "utilities.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class GraphicsEngine {
public:

    GraphicsEngine();
    ~GraphicsEngine();

    void mainLoop();

private:
    // TODO: move window to something else?
    // let the engine just be the interface to vulkan
    // and make the game loop outside, such that there is a draw function that can be called
    const uint32_t width = 640;
    const uint32_t height = 480;
    GLFWwindow* window;
    void createWindow();

    VkSurfaceKHR surface;
    void createSurface();

    VkInstance instance;
    void createInstance();

    VkDebugUtilsMessengerEXT debugMessenger;
    VkDebugUtilsMessengerCreateInfoEXT getDebugMessengerInfo();
    void createDebugMessenger();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData); 

    VkPhysicalDevice physicalDevice;
    void pickPhysicalDevice();

    std::optional<uint32_t> graphicsQueueIndex;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    void createDevice();

    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    void createSwapchain();

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    void createImageViews();

    VkRenderPass renderPass;
    void createRenderPass();

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkPipelineLayout swapPipelineLayout = VK_NULL_HANDLE;
    VkPipeline swapGraphicsPipeline = VK_NULL_HANDLE;

    void createGraphicsPipeline(VkPipelineLayout& layout, VkPipeline& pipeline);

    std::vector<VkFramebuffer> swapchainFramebuffers;
    void createFramebuffers();

    VkCommandPool commandPool;
    void createCommandPool();

    VkCommandBuffer commandBuffer;
    void createCommandBuffer();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
    void createSyncObjects();

    void drawFrame();

    std::unique_ptr<Utilities::FileWatcher> fileWatcher;
    void onChangedFile(const std::string& filename);
};