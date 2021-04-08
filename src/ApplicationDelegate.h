#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>

#include <irrlicht.h>

class ApplicationDelegate
{
public:
    ApplicationDelegate(irr::IrrlichtDevice* _device);

    void initialize();

    void update();

    void saveTexture();

    void saveTexture(const std::wstring& filename);

    void loadModel(const std::wstring& filename);

    void openSaveTextureDialog();

    void closeSaveTextureDialog();

    void openLoadModelDialog();

    void closeLoadModelDialog();

    void quit();

private:
    void initGUI();

    void loadGUI();

    void createToolbar();

    void resetFont();

    irr::gui::IGUIElement* getElementByName(const std::string& name);
    irr::gui::IGUIElement* getElementByName(const std::string& name, irr::gui::IGUIElement* parent);

    irr::scene::ISceneNode* loadMesh(const std::wstring& meshFilename);

    irr::IrrlichtDevice* device;

    irr::video::IVideoDriver* driver;
    irr::scene::ISceneManager* smgr;
    irr::gui::IGUIEnvironment* guienv;
    irr::scene::ICameraSceneNode* camera;

    irr::scene::ITriangleSelector* triangleSelector;

    bool loadModelDialogIsOpen;
    bool saveTextureDialogIsOpen;
};
