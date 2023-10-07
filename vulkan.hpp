#pragma once

#include <vector>

// TODO: regroup functions under multiple files

namespace Vulkan {

    bool layersAreSupported(const std::vector<const char*> layerNames);
    bool extensionsAreSupported(const std::vector<const char*> extensionNames);

}