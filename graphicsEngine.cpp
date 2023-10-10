#include "graphicsEngine.hpp"
#include <iostream>
#include "vulkan.hpp"
#include <optional>
#include <set>
#include "utilities.hpp"

GraphicsEngine::GraphicsEngine() {
    createWindow();
    createInstance();
    createSurface();
    #ifdef DEBUG_MODE
        createDebugMessenger();
    #endif
    pickPhysicalDevice();
    createDevice();
    createSwapchain();
    createImageViews();
    createGraphicsPipeline();
}

GraphicsEngine::~GraphicsEngine() {
    for (auto imageView : swapchainImageViews)
        vkDestroyImageView(device, imageView, nullptr);

    vkDestroySwapchainKHR(device, swapchain, nullptr);
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
    
    if (!Vulkan::instanceSupportsLayers(layers))
        throw std::runtime_error("Layers are not supported.");

    uint32_t glfwExtensionCount;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    #ifdef DEBUG_MODE
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

    if (!Vulkan::instanceSupportsExtensions(extensions))
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

    // https://ansi.gabebanks.net/
    const std::string defaultColor = "\033[0m";
    std:: string color = defaultColor;

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        severity = "VERBOSE";
        color = defaultColor;
    }
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        severity = "INFO";
        color = "\033[36;49m";
    }
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        severity = "WARNING";
        color = "\033[33;49m";
    }
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        severity = "ERROR";
        color = "\033[36;41m";
    }

    std::cout << color << severity << " " << callbackData->pMessage << defaultColor << std::endl;

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
        throw std::runtime_error("Failed to find a queue family supporting graphics and present operations.");

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

    const std::vector<const char*> extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    if (!Vulkan::deviceSupportsExtensions(physicalDevice, extensions))
        std::runtime_error("The extensions are not supported by the physical device.");

    auto deviceInfo = VkDeviceCreateInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = queueInfos.size();
    deviceInfo.pQueueCreateInfos = queueInfos.data();
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    deviceInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device.");

    vkGetDeviceQueue(device, graphicsIndex.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentIndex.value(), 0, &presentQueue);
}

void GraphicsEngine::createSwapchain() {
    auto capabilities = VkSurfaceCapabilitiesKHR{};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities) != VK_SUCCESS)
        throw std::runtime_error("Failed to get surface capabilites.");

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0)
        imageCount = std::min(imageCount, capabilities.maxImageCount);

    uint32_t formatCount;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Failed to get surface formats.");

    std::vector<VkSurfaceFormatKHR> availableFormats(formatCount);
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, availableFormats.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to get surface formats.");

    auto surfaceFormat = VkSurfaceFormatKHR{};
    for (const auto& availableFormat : availableFormats)
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }

    if (surfaceFormat.format == VK_FORMAT_UNDEFINED)
        throw std::runtime_error("Failed to find a suitable surface format.");

    uint32_t presentModeCount;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Failed to get surface present modes.");

    std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, availablePresentModes.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to get surface present modes.");

    auto presentMode = VkPresentModeKHR::VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : availablePresentModes)
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
            break;
        }

    if (surfaceFormat.format == VK_FORMAT_UNDEFINED)
        throw std::runtime_error("Failed to find a suitable surface format.");

    auto swapchainInfo = VkSwapchainCreateInfoKHR{};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = surface;
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = capabilities.currentExtent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = capabilities.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain.");

    uint32_t swapchainImageCount;
    if (vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Failed to get swapchain images.");

    swapchainImages.resize(swapchainImageCount);
    if (vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to get swapchain images.");

    swapchainImageFormat = surfaceFormat.format;
}

void GraphicsEngine::createImageViews() {
    swapchainImageViews.resize(swapchainImages.size());

    for (auto i = 0; i < swapchainImages.size(); i++) {
        auto imageViewinfo = VkImageViewCreateInfo{};
        imageViewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewinfo.image = swapchainImages[i];
        imageViewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewinfo.format = swapchainImageFormat;
        imageViewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewinfo.subresourceRange.baseMipLevel = 0;
        imageViewinfo.subresourceRange.levelCount = 1;
        imageViewinfo.subresourceRange.baseArrayLayer = 0;
        imageViewinfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &imageViewinfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image view.");
    }
}

void GraphicsEngine::createGraphicsPipeline() {
    const char* command = "C:/VulkanSDK/1.3.261.1/Bin/glslc.exe shaders/shader.vert -o build/shader.vert.spv";
    if (std::system(command) != 0)
        throw std::runtime_error("Failed to build the shader.");
        
    auto vertShaderCode = Utilities::readFile("build/shader.vert.spv");

    auto shaderModuleInfo = VkShaderModuleCreateInfo{};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.codeSize = vertShaderCode.size();
    shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(vertShaderCode.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &shaderModuleInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module.");

    vkDestroyShaderModule(device, shaderModule, nullptr);
}