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
    const int maxFramesInFlight = 2;
    uint32_t currentFrame = 0;

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

    // TODO: rename or move once asset manager / build is created
    VkPipelineLayout swapPipelineLayout = VK_NULL_HANDLE;
    VkPipeline swapGraphicsPipeline = VK_NULL_HANDLE;

    void createGraphicsPipeline(VkPipelineLayout& layout, VkPipeline& pipeline);

    std::vector<VkFramebuffer> swapchainFramebuffers;
    void createFramebuffers();

    VkCommandPool commandPool;
    void createCommandPool();

    std::vector<VkCommandBuffer> commandBuffers;
    void createCommandBuffer();

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    void createSyncObjects();

    void drawFrame();

    std::unique_ptr<Utilities::FileWatcher> fileWatcher;
    void onChangedFile(const std::string& filename);
};