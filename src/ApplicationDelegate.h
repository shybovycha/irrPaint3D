#pragma once

#include <fstream>
#include <iostream>
#include <map>
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

    void beginDrawing();

    void endDrawing();

    void updateBrushProperties();
    
    void updateModelProperties();

    void quit();

private:
    void initGUI();

    void loadGUI();

    void resetFont();

    void drawSelectedTriangle2D();
    void drawSelectedTriangle3D();

    irr::gui::IGUIElement* getElementByName(const std::string& name);
    irr::gui::IGUIElement* getElementByName(const std::string& name, irr::gui::IGUIElement* parent);

    void createBrush(float brushSize, float featherRadius = 0.f, irr::video::SColor color = irr::video::SColor(255, 255, 255, 255));

    void updatePropertiesWindow();

    irr::IrrlichtDevice* device;

    irr::video::IVideoDriver* driver;
    irr::scene::ISceneManager* smgr;
    irr::gui::IGUIEnvironment* guienv;
    
    irr::scene::ICameraSceneNode* camera;

    irr::scene::ISceneNode* modelSceneNode;
    irr::scene::IAnimatedMesh* modelMesh;

    irr::video::IImage* brushImage;
    irr::video::ITexture* brushTexture;

    irr::video::IImage* selectedTextureImage;
    irr::video::ITexture* selectedTexture;
    
    irr::video::IImage* tempImage;
    irr::video::ITexture* tempTexture;

    irr::scene::ITriangleSelector* triangleSelector;

    irr::core::vector2di previousMouseCursorPosition;

    bool loadModelDialogIsOpen;
    bool saveTextureDialogIsOpen;

    bool isDrawing;
    bool previousIsDrawing;

    unsigned int brushSize = 25;
    unsigned int brushFeatherRadius = 5;
    irr::video::SColor brushColor;

    std::wstring textureFilename;
};
