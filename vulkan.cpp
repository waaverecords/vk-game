#include "vulkan.hpp"
#include <iostream>
#include <cstring>
#include <set>

bool Vulkan::instanceSupportsLayers(const std::vector<const char*> layerNames) {
    uint32_t propertyCount;
    if (vkEnumerateInstanceLayerProperties(&propertyCount, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Failed to enumerate instance layer properties.");

    std::vector<VkLayerProperties> properties(propertyCount);
    if (vkEnumerateInstanceLayerProperties(&propertyCount, properties.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to enumerate instance layer properties.");

    bool found;
    for (auto name : layerNames) {
        found = false;
        for (auto prop : properties)
            if (strcmp(name, prop.layerName) == 0) {
                found = true;
                break;
            }
        if (!found)
            return false;
    }

    return true;
}

bool Vulkan::instanceSupportsExtensions(const std::vector<const char*> extensionNames) {
    uint32_t propertyCount;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Failed to enumerate instance extension properties.");

    std::vector<VkExtensionProperties> extProperties(propertyCount);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, extProperties.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to enumerate instance extension properties.");

    bool found;
    for (auto name : extensionNames) {
        found = false;
        for (auto prop : extProperties)
            if (strcmp(name, prop.extensionName) == 0) {
                found = true;
                break;
            }
        if (!found)
            return false;
    }

    return true;
}

bool Vulkan::deviceSupportsExtensions(const VkPhysicalDevice physicalDevice, std::vector<const char*> extensionNames) {
    uint32_t extensionCount;
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr) != VK_SUCCESS)
        throw std::runtime_error("Failed to enumerate device extension properties.");

    std::vector<VkExtensionProperties> extProperties(extensionCount);
    if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extProperties.data()) != VK_SUCCESS)
        throw std::runtime_error("Failed to enumerate device extension properties.");

    std::set<std::string> requiredExtensions(extensionNames.begin(), extensionNames.end());
    for (const auto& extension : extProperties)
        requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
}

VkResult Vulkan::createDebugMessengerExtension(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* debugMessengerInfo,
    const VkAllocationCallbacks* allocator,
    VkDebugUtilsMessengerEXT* debugMessenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, debugMessengerInfo, allocator, debugMessenger);
        
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void Vulkan::destroyDebugMessengerExtension(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* allocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debugMessenger, allocator);
}