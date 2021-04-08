#include "ApplicationDelegate.h"

ApplicationDelegate::ApplicationDelegate(irr::IrrlichtDevice* _device) :
    device(_device),
    smgr(device->getSceneManager()),
    guienv(device->getGUIEnvironment()),
    driver(device->getVideoDriver()),
    camera(nullptr),
    loadModelDialogIsOpen(false),
    saveTextureDialogIsOpen(false),
    triangleSelector(nullptr)
{
}

void ApplicationDelegate::initialize()
{
    camera = smgr->addCameraSceneNode();

    /*auto animator = new CameraSceneNodeAnimator(device->getCursorControl());
    camera->addAnimator(animator);*/

    initGUI();
}

void ApplicationDelegate::initGUI()
{
    loadGUI();

    resetFont();

    createToolbar();
}

void ApplicationDelegate::loadGUI()
{
    guienv->loadGUI("media/gui.xml", nullptr);
}

irr::gui::IGUIElement* ApplicationDelegate::getElementByName(const std::string& name)
{
    return getElementByName(name, guienv->getRootGUIElement());
}

irr::gui::IGUIElement* ApplicationDelegate::getElementByName(const std::string& name, irr::gui::IGUIElement* parent)
{
    std::queue<irr::gui::IGUIElement*> queue;

    queue.push(parent);

    while (!queue.empty())
    {
        auto currentElement = queue.front();
        
        queue.pop();

        auto currentElementName = std::string(currentElement->getName());

        if (name == currentElementName) {
            return currentElement;
        }

        for (auto child : currentElement->getChildren())
        {
            queue.push(child);
        }
    }

    return nullptr;
}

irr::scene::ISceneNode* ApplicationDelegate::loadMesh(const std::wstring& meshFilename)
{
    return nullptr;
}

void ApplicationDelegate::createToolbar()
{
    /*auto toolbar = reinterpret_cast<irr::gui::IGUIToolBar*>(getElementByName("mainToolBar"));

    auto openModelButton = reinterpret_cast<irr::gui::IGUIButton*>(getElementByName("openModelButton", toolbar));
    auto saveTextureButton = reinterpret_cast<irr::gui::IGUIButton*>(getElementByName("saveTextureButton", toolbar));*/
}

void ApplicationDelegate::resetFont()
{
    irr::gui::IGUIFont* font = guienv->getFont("media/calibri.xml");
    guienv->getSkin()->setFont(font);
}

void ApplicationDelegate::quit()
{
    device->closeDevice();
}

void ApplicationDelegate::update()
{
    driver->beginScene(true, true, irr::video::SColor(0, 200, 200, 200));

    smgr->drawAll();

    guienv->drawAll();

    driver->endScene();
}

void ApplicationDelegate::saveTexture()
{
}

void ApplicationDelegate::saveTexture(const std::wstring& filename)
{
}

void ApplicationDelegate::loadModel(const std::wstring& filename)
{
}

void ApplicationDelegate::openSaveTextureDialog()
{
}

void ApplicationDelegate::closeSaveTextureDialog()
{
}

void ApplicationDelegate::openLoadModelDialog()
{
}

void ApplicationDelegate::closeLoadModelDialog()
{
    /*auto dialog = getElementByName("loadModelDialog");

    dialog->hide();*/
}
