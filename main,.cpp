#include "graphicsEngine.hpp"
#include "Utilities.hpp"
#include <iostream>

void callback(const std::string& filename) {
    std::cout << filename << std::endl;
}

int main() {
    GraphicsEngine* graphicsEngine = new GraphicsEngine();
    graphicsEngine->mainLoop();
    delete graphicsEngine;

    // Utilities::FileWatcher f = Utilities::FileWatcher(
    //     {
    //         "shaders/shader.frag",
    //         "shaders/shader.vert"
    //     },
    //     callback
    // );
    // f.start();
    // std::cout << "test" << std::endl;
    // std::this_thread::sleep_for(std::chrono::seconds(30));

    return 0;
}