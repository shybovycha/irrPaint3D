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
    modelSceneNode(nullptr),
    renderTarget(nullptr)
{
}

void ApplicationDelegate::initialize()
{
    camera = smgr->addCameraSceneNodeMaya();
    fixedCamera = smgr->addCameraSceneNode();

    smgr->setActiveCamera(camera);

    renderTarget = driver->addRenderTargetTexture(irr::core::dimension2du(1024, 1024), "RTT1");

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

irr::core::vector2df ApplicationDelegate::getPointUV(irr::core::triangle3df triangle, irr::core::vector3df point, irr::scene::ISceneNode* sceneNode, irr::scene::IMesh* mesh)
{
    irr::video::S3DVertex A, B, C;

    auto meshBufferCount = mesh->getMeshBufferCount();
    auto meshBuffer = mesh->getMeshBuffer(0);

    bool foundA = false;
    bool foundB = false;
    bool foundC = false;

    for (auto t = 0; t < mesh->getMeshBufferCount(); ++t)
    {
        auto meshBuffer = mesh->getMeshBuffer(t);

        auto vertices = static_cast<irr::video::S3DVertex*>(meshBuffer->getVertices());

        for (auto i = 0; i < meshBuffer->getVertexCount(); ++i)
        {
            if (vertices[i].Pos == triangle.pointA)
            {
                A = vertices[i];
                foundA = true;
            }
            else if (vertices[i].Pos == triangle.pointB)
            {
                B = vertices[i];
                foundB = true;
            }
            else if (vertices[i].Pos == triangle.pointC)
            {
                C = vertices[i];
                foundC = true;
            }
        }
    }

    if (!foundA || !foundB || !foundC) {
        return irr::core::vector2df();
    }

    irr::core::vector3df a = B.Pos - A.Pos;
    irr::core::vector3df b = C.Pos - A.Pos;
    irr::core::vector3df p = point - A.Pos;

    float u, v;

    // au + bv = p
    if (a.X != 0) {
        v = ((a.X * p.Y) - (a.Y * p.X)) / ((a.X * b.Y) - (a.Y * b.X));
        u = (p.X - (b.X * v)) / a.X;
    }
    else if (a.Y != 0) {
        v = ((a.Y * p.Z) - (a.Z * p.Y)) / ((a.Y * b.Z) - (a.Z * b.Y));
        u = (p.Y - (b.Y * v)) / a.Y;
    }
    else if (a.Z != 0) {
        v = ((a.Z * p.X) - (a.X * p.Z)) / ((a.Z * b.X) - (a.X * b.Z));
        u = (p.Z - (b.Z * v)) / a.Z;
    }
    else {
        throw "Invalid input - zero basis vector";
    }

    // assert((a * u + b * v) == p);

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

    drawSelectedTriangle2D();

    smgr->drawAll();

    guienv->drawAll();

    drawSelectedTriangle3D();

    driver->endScene();
}

void ApplicationDelegate::drawSelectedTriangle2D()
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

    bool collisionDetected = smgr->getSceneCollisionManager()->getCollisionPoint(ray, triangleSelector, collisionPoint, selectedTriangle, selectedNode);

    if (!collisionDetected) {
        return;
    }

    auto materialsTabControl = reinterpret_cast<irr::gui::IGUITabControl*>(getElementByName("texturePreviewTabControl"));

    auto currentMaterialTabIndex = materialsTabControl->getActiveTab();

    if (currentMaterialTabIndex < 0) {
        return;
    }

    auto currentMaterialTab = materialsTabControl->getTab(currentMaterialTabIndex);

    auto image = reinterpret_cast<irr::gui::IGUIImage*>(getElementByName("image", currentMaterialTab));

    if (image == nullptr) {
        return;
    }

    auto meshSceneNode = reinterpret_cast<irr::scene::IAnimatedMeshSceneNode*>(modelSceneNode);
    auto animatedMesh = meshSceneNode->getMesh();
    auto mesh = animatedMesh->getMesh(0);

    auto uv = getPointUV(selectedTriangle, collisionPoint, meshSceneNode, mesh);

    auto textureImage = meshSceneNode->getMaterial(1).getTexture(0);

    auto textureSize = textureImage->getOriginalSize();

    auto imageRect = image->getAbsolutePosition();

    auto point = irr::core::vector2di((textureSize.Width * uv.X), (textureSize.Height * uv.Y));

    driver->setRenderTarget(renderTarget);

    smgr->setActiveCamera(fixedCamera);

    driver->draw2DImage(textureImage, irr::core::vector2di(0, 0));

    driver->draw2DLine(irr::core::position2di(point.X - 50, point.Y - 50), irr::core::position2di(point.X + 50, point.Y + 50), TRIANGLE_COLOR);
    driver->draw2DLine(irr::core::position2di(point.X + 50, point.Y - 50), irr::core::position2di(point.X - 50, point.Y + 50), TRIANGLE_COLOR);

    driver->setRenderTarget(0);

    smgr->setActiveCamera(camera);

    image->setImage(renderTarget);
}

void ApplicationDelegate::drawSelectedTriangle3D()
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

    bool collisionDetected = smgr->getSceneCollisionManager()->getCollisionPoint(ray, triangleSelector, collisionPoint, selectedTriangle, selectedNode);

    if (!collisionDetected) {
        return;
    }

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
        auto texture = material.getTexture(0);

        if (texture == nullptr) {
            continue;
        }

        std::wostringstream tabCaption;
        
        tabCaption << "Material " << i + 1;

        auto tab = materialsTabControl->addTab(tabCaption.str().c_str());

        auto textureImage = guienv->addImage(texture, irr::core::vector2di(10, 10));

        textureImage->setName("image");

        tab->addChild(textureImage);

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
