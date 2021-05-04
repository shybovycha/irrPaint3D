#include "Application.h"

Application::Application() {}

void Application::initialize() {
    device = irr::createDevice(
        irr::video::EDT_OPENGL,
        irr::core::dimension2d<irr::u32>(1024, 768),
        32,
        false,
        false,
        true,
        0
    );

    if (!device) {
        std::cerr << "Could not initialize video device\n";

        return;
    }

    device->setWindowCaption(L"irrPaint3D");

    driver = device->getVideoDriver();
    smgr = device->getSceneManager();
    guienv = device->getGUIEnvironment();

    applicationDelegate = std::make_shared<ApplicationDelegate>(device);

    applicationDelegate->initialize();

    eventReceiver = std::make_unique<IrrlichtEventReceiver>(applicationDelegate);

    device->setEventReceiver(eventReceiver.get());
}

void Application::run() {
    initialize();

    while (device->run()) {
        if (!device->isWindowActive() || !device->isWindowFocused() || device->isWindowMinimized()) {
            continue;
        }

        applicationDelegate->update();
    }

    device->drop();
}
