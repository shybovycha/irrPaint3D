#pragma once

#include "ApplicationDelegate.h"
#include "IrrlichtEventReceiver.h"

#include <irrlicht.h>

#include <iostream>
#include <string>

class Application {
public:
    Application();

    void run();

private:
    void initialize();

    irr::IrrlichtDevice* device;
    irr::video::IVideoDriver* driver;
    irr::scene::ISceneManager* smgr;
    irr::gui::IGUIEnvironment* guienv;

    std::shared_ptr<ApplicationDelegate> applicationDelegate;
    std::unique_ptr<IrrlichtEventReceiver> eventReceiver;
};
