#pragma once

#include <vector>
#include <vulkan/vulkan.h>

// TODO: regroup functions under multiple files

namespace Vulkan {

    bool layersAreSupported(const std::vector<const char*> layerNames);
    bool extensionsAreSupported(const std::vector<const char*> extensionNames);

    VkResult createDebugMessengerExtension(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* debugMessengerInfo, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debugMessenger);
    void destroyDebugMessengerExtension(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks*allocator);
}