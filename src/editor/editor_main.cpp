#include "editor.h"
#include "controller/controller.h"
#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
    std::cout << "mupsi v0.1 — editor" << std::endl;

    const char* configPath = (argc >= 2) ? argv[1] : "scene.json";

    mupsi::Controller ctrl;
    if (!ctrl.load(configPath)) {
        std::cerr << "Failed to load " << configPath << std::endl;
        return -1;
    }

    return mupsi::runEditor(ctrl, "mupsi editor");
}
