#pragma once

#include <vector>
#include <vulkan/vulkan.h>

// TODO: regroup functions under multiple files

namespace Vulkan {

    bool instanceSupportsLayers(const std::vector<const char*> layerNames);
    bool instanceSupportsExtensions(const std::vector<const char*> extensionNames);
    bool deviceSupportsExtensions(const VkPhysicalDevice device, std::vector<const char*> extensionNames);

    VkResult createDebugMessengerExtension(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* debugMessengerInfo, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debugMessenger);
    void destroyDebugMessengerExtension(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks*allocator);
}