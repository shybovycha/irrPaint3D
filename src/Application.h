#pragma once

#include <irrlicht.h>

#include <iostream>
#include <string>

class Application {
public:
  Application();

  void initialize();

  void run();

private:
  irr::IrrlichtDevice* device;
  irr::video::IVideoDriver* driver;
  irr::scene::ISceneManager* smgr;
  irr::gui::IGUIEnvironment* guienv;
};
