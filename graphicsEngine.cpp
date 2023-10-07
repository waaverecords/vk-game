#include "graphicsEngine.hpp"
#include <vector>
#include <iostream>
#include "vulkan.hpp"
#include <optional>
#include <set>

GraphicsEngine::GraphicsEngine() {
    createWindow();
    createInstance();
    createSurface();
    #ifdef DEBUG_MODE
        createDebugMessenger();
    #endif
    pickPhysicalDevice();
    createDevice();
}

GraphicsEngine::~GraphicsEngine() {
    vkDestroyDevice(device, nullptr);
    #ifdef DEBUG_MODE
        Vulkan::destroyDebugMessengerExtension(instance, debugMessenger, nullptr);
    #endif
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    glfwTerminate();
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

void GraphicsEngine::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface.");
}

void GraphicsEngine::createInstance() {
    std::vector<const char*> layers;
    #ifdef DEBUG_MODE
        layers.push_back("VK_LAYER_KHRONOS_validation");
    #endif
    
    if (!Vulkan::layersAreSupported(layers))
        throw std::runtime_error("Layers are not supported.");

    uint32_t glfwExtensionCount;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    #ifdef DEBUG_MODE
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

    if (!Vulkan::extensionsAreSupported(extensions))
        throw std::runtime_error("Extensions are not supported.");

    auto appInfo = VkApplicationInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "vk-game";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "vk-game";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    auto instanceInfo = VkInstanceCreateInfo{};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledLayerCount = layers.size();
    instanceInfo.ppEnabledLayerNames = layers.data();
    instanceInfo.enabledExtensionCount = extensions.size();
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    #ifdef DEBUG_MODE
        auto debugMessengerInfo = getDebugMessengerInfo();
        instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugMessengerInfo;
    #endif

    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance.");
}

VkDebugUtilsMessengerCreateInfoEXT GraphicsEngine::getDebugMessengerInfo() {
    auto debugMessengerInfo = VkDebugUtilsMessengerCreateInfoEXT{};
    debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugMessengerInfo.pfnUserCallback = debugCallback;

    return debugMessengerInfo;
}

void GraphicsEngine::createDebugMessenger() {
    auto debugMessengerInfo = getDebugMessengerInfo();
    if (Vulkan::createDebugMessengerExtension(instance, &debugMessengerInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        throw std::runtime_error("Failed to create debug messenger.");
}

VKAPI_ATTR VkBool32 VKAPI_CALL GraphicsEngine::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData
) {
    std::string severity = "UNKNOWN";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        severity = "VERBOSE";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        severity = "INFO";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        severity = "WARNING";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        severity = "ERROR";

    std::cerr << severity << " " << callbackData->pMessage << std::endl;
    return VK_FALSE;
}

void GraphicsEngine::pickPhysicalDevice() {
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

void GraphicsEngine::createDevice() {
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    std::optional<uint32_t> graphicsIndex;
    std::optional<uint32_t> presentIndex;
    
    for (auto i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsIndex = i;
        }
        
        VkBool32 presentSupported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupported);
        if (presentSupported)
            presentIndex = i;

        if (graphicsIndex.has_value() && presentIndex.has_value())
            break;
    }

    if (!graphicsIndex.has_value() || !presentIndex.has_value())
        throw std::runtime_error("Failed to find a queue family supporting graphics and present operation.");

    std::set<uint32_t> queueIndices = {graphicsIndex.value(), presentIndex.value()};
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    auto queuePriority = 1.0f;

    for (auto queueIndex : queueIndices) {
        auto queueInfo = VkDeviceQueueCreateInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = queueIndex;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;

        queueInfos.push_back(queueInfo);
    }

    auto deviceInfo = VkDeviceCreateInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = queueInfos.size();
    deviceInfo.pQueueCreateInfos = queueInfos.data();
    deviceInfo.enabledExtensionCount = 0;

    if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device.");

    vkGetDeviceQueue(device, graphicsIndex.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentIndex.value(), 0, &presentQueue);
}