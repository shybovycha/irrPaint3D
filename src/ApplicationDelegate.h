#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>

#include <irrlicht.h>

#include "SaveFileDialog.h"

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

    void resetFont();

    void drawSelectedTriangle2D();
    void drawSelectedTriangle3D();

    irr::gui::IGUIElement* getElementByName(const std::string& name);
    irr::gui::IGUIElement* getElementByName(const std::string& name, irr::gui::IGUIElement* parent);

    irr::core::vector2df getPointUV(irr::core::triangle3df triangle, irr::core::vector3df point, irr::scene::ISceneNode* sceneNode, irr::scene::IMesh* mesh);

    irr::IrrlichtDevice* device;

    irr::video::IVideoDriver* driver;
    irr::scene::ISceneManager* smgr;
    irr::gui::IGUIEnvironment* guienv;
    
    irr::scene::ICameraSceneNode* camera;
    irr::scene::ICameraSceneNode* fixedCamera;

    irr::scene::ISceneNode* modelSceneNode;
    irr::scene::IAnimatedMesh* modelMesh;

    irr::video::ITexture* renderTarget;

    irr::scene::ITriangleSelector* triangleSelector;

    bool loadModelDialogIsOpen;
    bool saveTextureDialogIsOpen;

    std::wstring textureFilename;
};
