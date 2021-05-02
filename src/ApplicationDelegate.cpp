#include "ApplicationDelegate.h"

ApplicationDelegate::ApplicationDelegate(irr::IrrlichtDevice* _device) :
    device(_device),
    smgr(device->getSceneManager()),
    guienv(device->getGUIEnvironment()),
    driver(device->getVideoDriver()),
    camera(nullptr),
    loadModelDialogIsOpen(false),
    saveTextureDialogIsOpen(false),
    triangleSelector(nullptr),
    modelMesh(nullptr),
    modelSceneNode(nullptr)
{
}

void ApplicationDelegate::initialize()
{
    camera = smgr->addCameraSceneNodeMaya(); // addCameraSceneNode();

    /*auto animator = new CameraSceneNodeAnimator(device->getCursorControl());
    camera->addAnimator(animator);*/

    initGUI();
}

void ApplicationDelegate::initGUI()
{
    loadGUI();

    resetFont();
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

irr::core::vector2df ApplicationDelegate::getPointUV(irr::core::triangle3df triangle, irr::core::vector3df point, irr::scene::IMeshSceneNode* sceneNode)
{
    irr::core::matrix4 inverseTransform(
        sceneNode->getAbsoluteTransformation(),
        irr::core::matrix4::EM4CONST_INVERSE
    );

    inverseTransform.transformVect(triangle.pointA);
    inverseTransform.transformVect(triangle.pointB);
    inverseTransform.transformVect(triangle.pointC);

    auto v0 = triangle.pointC - triangle.pointA;
    auto v1 = triangle.pointB - triangle.pointA;
    auto v2 = point - triangle.pointA;

    float dot00 = v0.dotProduct(v0);
    float dot01 = v0.dotProduct(v1);
    float dot02 = v0.dotProduct(v2);
    float dot11 = v1.dotProduct(v1);
    float dot12 = v1.dotProduct(v2);

    float invDenom = 1.f / ((dot00 * dot11) - (dot01 * dot01));
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    irr::video::S3DVertex A, B, C;

    auto vertices = static_cast<irr::video::S3DVertex*>(sceneNode->getMesh()->getMeshBuffer(0)->getVertices());

    for (auto i = 0; i < sceneNode->getMesh()->getMeshBuffer(0)->getVertexCount(); ++i)
    {
        if (vertices[i].Pos == triangle.pointA)
        {
            A = vertices[i];
        }
        else if (vertices[i].Pos == triangle.pointB)
        {
            B = vertices[i];
        }
        else if (vertices[i].Pos == triangle.pointC)
        {
            C = vertices[i];
        }
    }

    auto t2 = B.TCoords - A.TCoords;
    auto t1 = C.TCoords - A.TCoords;

    auto uvCoords = A.TCoords + t1 * u + t2 * v;

    return uvCoords;
}
void ApplicationDelegate::resetFont()
{
    irr::gui::IGUIFont* font = guienv->getFont("media/calibri.xml");
    guienv->getSkin()->setFont(font);
}

void ApplicationDelegate::quit()
{
    if (modelMesh != nullptr) {
        modelMesh->drop();
    }

    device->closeDevice();
}

void ApplicationDelegate::update()
{
    driver->beginScene(true, true, irr::video::SColor(0, 200, 200, 200));

    smgr->drawAll();

    guienv->drawAll();

    drawSelectedTriangle();

    driver->endScene();
}

void ApplicationDelegate::drawSelectedTriangle()
{
    if (triangleSelector == nullptr) {
        return;
    }

    const unsigned int MAX_TRIANGLES = 100;
    const auto TRIANGLE_COLOR = irr::video::SColor(255, 0, 255, 0);

    auto triangles = new irr::core::triangle3df[MAX_TRIANGLES];

    int matchedTriangles = 0;

    irr::core::line3df ray = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(device->getCursorControl()->getPosition(), camera);

    irr::core::vector3df collisionPoint;
    irr::core::triangle3df selectedTriangle;
    irr::scene::ISceneNode* selectedNode;

    smgr->getSceneCollisionManager()->getCollisionPoint(ray, triangleSelector, collisionPoint, selectedTriangle, selectedNode);

    driver->draw3DTriangle(selectedTriangle, TRIANGLE_COLOR);
}

void ApplicationDelegate::saveTexture()
{
    if (textureFilename.empty()) {
        auto saveTextureDialog = reinterpret_cast<SaveFileDialog*>(getElementByName("saveTextureDialog"));

        if (saveTextureDialog == nullptr) {
            std::cerr << "Could not save texture to a non-existent (empty name) file" << std::endl;
            return;
        }

        textureFilename = saveTextureDialog->getFileName();
    }

    saveTexture(textureFilename);
}

void ApplicationDelegate::saveTexture(const std::wstring& filename)
{
    // TODO: implement
}

void ApplicationDelegate::loadModel(const std::wstring& filename)
{
    if (modelSceneNode != nullptr) {
        modelSceneNode->drop();
    }

    if (filename.empty()) {
        std::cerr << "Could not load non-existent (empty filename) model" << std::endl;
        return;
    }

    modelMesh = smgr->getMesh(filename.c_str());

    if (triangleSelector != nullptr) {
        triangleSelector->drop();
    }

    modelSceneNode = smgr->addAnimatedMeshSceneNode(modelMesh);

    modelMesh->drop();

    reinterpret_cast<irr::scene::IAnimatedMeshSceneNode*>(modelSceneNode)->setAnimationSpeed(0);

    modelSceneNode->setMaterialFlag(irr::video::EMF_LIGHTING, false);

    auto materialsTabControl = reinterpret_cast<irr::gui::IGUITabControl*>(getElementByName("texturePreviewTabControl"));

    materialsTabControl->clear();

    for (auto i = 0; i < modelSceneNode->getMaterialCount(); ++i) {
        auto material = modelSceneNode->getMaterial(i);
        std::wostringstream tabCaption;
        
        tabCaption << "Material " << i + 1;

        auto tab = materialsTabControl->addTab(tabCaption.str().c_str());

        auto textureImage = guienv->addImage(material.getTexture(0), irr::core::vector2di(10, 10));

        tab->addChild(textureImage);

        // textureImage->setMaxSize(irr::core::dimension2du(330, 490));
        textureImage->setScaleImage(true);
    }

    triangleSelector = smgr->createTriangleSelector(reinterpret_cast<irr::scene::IAnimatedMeshSceneNode*>(modelSceneNode));
}

void ApplicationDelegate::openSaveTextureDialog()
{
    if (saveTextureDialogIsOpen) {
        return;
    }

    auto saveTextureDialog = new SaveFileDialog(
        L"Save levels file",
        guienv,
        nullptr,
        static_cast<irr::s32>(-1),
        true
    );

    saveTextureDialog->setName("saveTextureDialog");

    guienv->getRootGUIElement()->addChild(saveTextureDialog);
    
    saveTextureDialogIsOpen = true;
}

void ApplicationDelegate::closeSaveTextureDialog()
{
    auto saveTextureDialog = reinterpret_cast<SaveFileDialog*>(getElementByName("saveTextureDialog"));

    saveTextureDialog->drop();

    saveTextureDialogIsOpen = false;
}

void ApplicationDelegate::openLoadModelDialog()
{
    auto loadModelDialog = guienv->addFileOpenDialog(L"Select model");

    loadModelDialog->setName(L"loadModelDialog");

    guienv->getRootGUIElement()->addChild(loadModelDialog);

    loadModelDialogIsOpen = true;
}

void ApplicationDelegate::closeLoadModelDialog()
{
    auto loadModelDialog = reinterpret_cast<irr::gui::IGUIFileOpenDialog*>(getElementByName("loadModelDialog"));

    loadModelDialog->drop();

    loadModelDialogIsOpen = false;
}
