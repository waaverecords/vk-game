#include "graphicsEngine.hpp"
#include <iostream>
#include "vulkan.hpp"
#include <set>
#include "utilities.hpp"
#include <functional>

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
    createRenderPass();
    createGraphicsPipeline(pipelineLayout, graphicsPipeline);
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();

    std::vector<std::string> a = {
        "shaders/shader.vert",
        "shaders/shader.frag",
    };
    fileWatcher = std::make_unique<Utilities::FileWatcher>(a, std::bind(&onChangedFile, this, std::placeholders::_1));
    fileWatcher->start();
}

GraphicsEngine::~GraphicsEngine() {
    fileWatcher.reset();

    for (auto i = 0; i < maxFramesInFlight; i++) {
        vkDestroyFence(device, inFlightFences[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    for (auto framebuffer : swapchainFramebuffers)
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

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

    glfwSetWindowPos(window, 1920 + 400, 350);
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

    std::optional<uint32_t> presentIndex;
    
    for (auto i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueIndex = i;
        }
        
        VkBool32 presentSupported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupported);
        if (presentSupported)
            presentIndex = i;

        if (graphicsQueueIndex.has_value() && presentIndex.has_value())
            break;
    }

    if (!graphicsQueueIndex.has_value() || !presentIndex.has_value())
        throw std::runtime_error("Failed to find a queue family supporting graphics and present operations.");

    std::set<uint32_t> queueIndices = {graphicsQueueIndex.value(), presentIndex.value()};
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

    vkGetDeviceQueue(device, graphicsQueueIndex.value(), 0, &graphicsQueue);
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
    swapchainExtent = swapchainInfo.imageExtent;
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

void GraphicsEngine::createRenderPass() {
    auto attachmentDescription = VkAttachmentDescription{};
    attachmentDescription.format = swapchainImageFormat;
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    auto colorAttachmentRef = VkAttachmentReference{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    auto subpass = VkSubpassDescription{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    auto dependency = VkSubpassDependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    auto renderPassInfo = VkRenderPassCreateInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &attachmentDescription;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount =1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=  VK_SUCCESS)
        throw std::runtime_error("Failed to create render pass.");
}

void GraphicsEngine::createGraphicsPipeline(VkPipelineLayout& layout, VkPipeline& pipeline) {
    // TODO: turn into a function for hot reloading shaders...
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

    auto pipelineShaderStageInfo = VkPipelineShaderStageCreateInfo{};
    pipelineShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineShaderStageInfo.module = shaderModule;
    pipelineShaderStageInfo.pName = "main";

    // TODO: turn into a function for hot reloading shaders...
    command = "C:/VulkanSDK/1.3.261.1/Bin/glslc.exe shaders/shader.frag -o build/shader.frag.spv";
    if (std::system(command) != 0)
        throw std::runtime_error("Failed to build the shader.");
        
    auto fragShaderCode = Utilities::readFile("build/shader.frag.spv");

    auto fShaderModuleInfo = VkShaderModuleCreateInfo{};
    fShaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fShaderModuleInfo.codeSize = fragShaderCode.size();
    fShaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(fragShaderCode.data());

    VkShaderModule fshaderModule;
    if (vkCreateShaderModule(device, &fShaderModuleInfo, nullptr, &fshaderModule) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module.");

    auto fpipelineShaderStageInfo = VkPipelineShaderStageCreateInfo{};
    fpipelineShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fpipelineShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fpipelineShaderStageInfo.module = fshaderModule;
    fpipelineShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { pipelineShaderStageInfo, fpipelineShaderStageInfo };

    auto vertexInputInfo = VkPipelineVertexInputStateCreateInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    auto inputAssemblyInfo = VkPipelineInputAssemblyStateCreateInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    auto viewport = VkViewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = swapchainExtent.width;
    viewport.height = swapchainExtent.height;
    viewport.minDepth = 0;
    viewport.maxDepth = 0;

    auto scissor = VkRect2D{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;

    auto viewportStateInfo = VkPipelineViewportStateCreateInfo{};
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissor;

    auto rasterizerInfo = VkPipelineRasterizationStateCreateInfo{};
    rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo.depthClampEnable = VK_FALSE;
    rasterizerInfo.rasterizerDiscardEnable = VK_FALSE; // disables output to framebuffer if set to true
    rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerInfo.depthBiasEnable = VK_FALSE;
    rasterizerInfo.lineWidth = 1;

    auto multisampleInfo = VkPipelineMultisampleStateCreateInfo{};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleInfo.sampleShadingEnable = VK_FALSE;

    auto colorBlendAttachmentState = VkPipelineColorBlendAttachmentState{};
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    auto colorBlendState = VkPipelineColorBlendStateCreateInfo{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = VK_FALSE;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachmentState;

    auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout.");

    auto pipelineInfo = VkGraphicsPipelineCreateInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizerInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline.");

    vkDestroyShaderModule(device, shaderModule, nullptr);
    vkDestroyShaderModule(device, fshaderModule, nullptr);
}

void GraphicsEngine::createFramebuffers() {
    swapchainFramebuffers.resize(swapchainImageViews.size());

    for (auto i = 0; i < swapchainImageViews.size(); i++) {
        VkImageView imageViews[] = { swapchainImageViews[i] };

        auto framebufferInfo = VkFramebufferCreateInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = imageViews;
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer.");
    }
}

void GraphicsEngine::createCommandPool() {
    auto commandPoolInfo = VkCommandPoolCreateInfo{};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.queueFamilyIndex = graphicsQueueIndex.value();

    if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool.");
}

void GraphicsEngine::createCommandBuffer() {
    commandBuffers.resize(maxFramesInFlight);

    auto allocateInfo = VkCommandBufferAllocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffer.");
}

void GraphicsEngine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    auto commandBufferBeginInfo = VkCommandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to begin command buffer.");

    auto clearValue = VkClearValue{{{0.0f, 0.0f, 0.0f}}};

    auto renderPassBeginInfo = VkRenderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = swapchainFramebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = swapchainExtent;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to end command buffer.");
}

void GraphicsEngine::createSyncObjects() {
    imageAvailableSemaphores.resize(maxFramesInFlight);
    renderFinishedSemaphores.resize(maxFramesInFlight);
    inFlightFences.resize(maxFramesInFlight);

    auto semaphoreInfo = VkSemaphoreCreateInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    auto fenceInfo = VkFenceCreateInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto i = 0; i < maxFramesInFlight; i++)
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create sync objects.");
}

void GraphicsEngine::mainLoop() {
    while (!glfwWindowShouldClose(window)) {

        // TODO: refactor shader hot reloading
        if (swapGraphicsPipeline != VK_NULL_HANDLE) {
            vkWaitForFences(device, inFlightFences.size(), inFlightFences.data(), VK_TRUE, UINT64_MAX);

            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

            pipelineLayout = swapPipelineLayout;
            graphicsPipeline = swapGraphicsPipeline;

            swapPipelineLayout = VK_NULL_HANDLE;
            swapGraphicsPipeline = VK_NULL_HANDLE;
        }

        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}

void GraphicsEngine::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    uint32_t imageIndex;
    if (vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex) != VK_SUCCESS)
        throw std::runtime_error("Failed to acquire next image.");

    if (vkResetCommandBuffer(commandBuffers[currentFrame], 0) != VK_SUCCESS)
        throw std::runtime_error("Failed to reset command buffer.");

    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    auto submitInfo = VkSubmitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit the command buffer to the queue.");

    VkSwapchainKHR swapchains[] = {swapchain};

    auto presentInfo = VkPresentInfoKHR{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;

    if (vkQueuePresentKHR(presentQueue, &presentInfo) != VK_SUCCESS)
        throw std::runtime_error("Failed to present the image.");

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void GraphicsEngine::onChangedFile(const std::string& filename) {
    std::cout << filename << std::endl;
    createGraphicsPipeline(swapPipelineLayout, swapGraphicsPipeline);
}