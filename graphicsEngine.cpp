#include "graphicsEngine.hpp"
#include <vector>
#include <iostream>
#include "any.hpp"

GraphicsEngine::GraphicsEngine() {
    createWindow();
    createInstance();
    selectPhysicalDevice();
}

void GraphicsEngine::createWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "vk-game", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create window.");
    }
}

void GraphicsEngine::createInstance() {
    std::vector<const char*> layers;
    #ifdef DEBUG_MODE
        layers.push_back("VK_LAYER_KHRONOS_validation");
    #endif

    if (!vkGame::layersAreSupported(layers))
        throw std::runtime_error("Layers are not supported.");

    uint32_t glfwExtensionCount;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    #ifdef DEBUG_MODE
        extensions.push_back("VK_EXT_debug_utils");
    #endif

    if (!vkGame::extensionsAreSupported(extensions))
        throw std::runtime_error("Extensions are not supported.");

    auto appInfo = VkApplicationInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vk-game";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "vk-game";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    auto instInfo = VkInstanceCreateInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledLayerCount = layers.size();
    instInfo.ppEnabledLayerNames = layers.data();
    instInfo.enabledExtensionCount = extensions.size();
    instInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&instInfo, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance.");
}

void GraphicsEngine::selectPhysicalDevice() {
    uint32_t deviceCount;

    if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Failed to enumerate physical devices.");

    if (deviceCount == 0)
        throw std::runtime_error("Failed to find a GPU with Vulkan support.");

    std::vector<VkPhysicalDevice> devices(deviceCount);

    if (vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to enumerate physical devices.");

    for (auto device : devices) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            physicalDevice = device;
            break;
    }

    if (physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("Failed to select a suitable GPU.");
}

GraphicsEngine::~GraphicsEngine() {
    vkDestroyInstance(instance, nullptr);
    glfwTerminate();
}