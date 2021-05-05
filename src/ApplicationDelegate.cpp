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
    brushImage(nullptr),
    brushTexture(nullptr),
    selectedTextureImage(nullptr),
    selectedTexture(nullptr),
    tempImage(nullptr),
    tempTexture(nullptr),
    brushSize(25),
    brushFeatherRadius(5),
    brushColor(irr::video::SColor(255, 0, 0, 0)),
    isDrawing(false),
    previousIsDrawing(false)
{
}

void ApplicationDelegate::initialize()
{
    camera = smgr->addCameraSceneNode();

    smgr->setActiveCamera(camera);

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

    paintTextureUnderCursor();

    guienv->drawAll();

    driver->endScene();
}

void ApplicationDelegate::paintTextureUnderCursor()
{
    if (triangleSelector == nullptr) {
        return;
    }

    const unsigned int MAX_TRIANGLES = 100;
    const unsigned int TEXTURE_CURSOR_SIZE = 25;

    const auto TRIANGLE_COLOR = irr::video::SColor(255, 0, 255, 0);

    auto triangles = new irr::core::triangle3df[MAX_TRIANGLES];

    int matchedTriangles = 0;

    auto cursorPosition = device->getCursorControl()->getPosition();

    if (cursorPosition == previousMouseCursorPosition && previousIsDrawing == isDrawing)
    {
        // mouse cursor position did not change - no need to perform all these operations
        return;
    }
    else
    {
        previousMouseCursorPosition = cursorPosition;
        previousIsDrawing = isDrawing;
    }

    irr::core::line3df ray = smgr->getSceneCollisionManager()->getRayFromScreenCoordinates(cursorPosition, camera);

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

    auto texturePreviewImage = reinterpret_cast<irr::gui::IGUIImage*>(getElementByName("image", currentMaterialTab));

    if (texturePreviewImage == nullptr) {
        return;
    }

    auto meshSceneNode = reinterpret_cast<irr::scene::IAnimatedMeshSceneNode*>(modelSceneNode);
    auto animatedMesh = meshSceneNode->getMesh();
    
    {
        irr::video::S3DVertex A, B, C;

        auto meshBufferCount = animatedMesh->getMeshBufferCount();
        auto meshBuffer = animatedMesh->getMeshBuffer(0);

        bool foundA = false;
        bool foundB = false;
        bool foundC = false;

        irr::video::SMaterial selectedMaterial;

        int materialTabCount = -1;
        int materialTabIndex = -1;

        for (auto t = 0; t < animatedMesh->getMeshBufferCount(); ++t)
        {
            auto meshBuffer = animatedMesh->getMeshBuffer(t);
            auto material = meshBuffer->getMaterial();

            if (material.getTexture(0) != nullptr)
            {
                ++materialTabCount;
            }

            auto vertices = static_cast<irr::video::S3DVertex*>(meshBuffer->getVertices());

            for (auto i = 0; i < meshBuffer->getVertexCount(); ++i)
            {
                if (vertices[i].Pos == selectedTriangle.pointA)
                {
                    A = vertices[i];
                    foundA = true;

                    materialTabIndex = materialTabCount;
                    selectedMaterial = material;
                }
                else if (vertices[i].Pos == selectedTriangle.pointB)
                {
                    B = vertices[i];
                    foundB = true;
                }
                else if (vertices[i].Pos == selectedTriangle.pointC)
                {
                    C = vertices[i];
                    foundC = true;
                }
            }
        }

        if (foundA && foundB && foundC) {
            irr::core::vector3df a = B.Pos - A.Pos;
            irr::core::vector3df b = C.Pos - A.Pos;
            irr::core::vector3df p = collisionPoint - A.Pos;

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

            // ...
            auto textureImage = selectedMaterial.getTexture(0);

            if (textureImage == nullptr) {
                return;
            }

            // TODO: rework this
            // this code is garbage, but it will open the corresponding material in the preview window, if a model has multiple materials, which is a superior feature
            auto materialsTabControl = reinterpret_cast<irr::gui::IGUITabControl*>(getElementByName("texturePreviewTabControl"));

            materialsTabControl->setActiveTab(materialTabIndex);

            auto textureSize = textureImage->getOriginalSize();

            auto point = irr::core::vector2di(
                (textureSize.Width * uvCoords.X) - (brushImage->getDimension().Width / 2),
                (textureSize.Height * uvCoords.Y) - (brushImage->getDimension().Height / 2)
            );

            selectedTextureImage->copyTo(tempImage);

            for (auto x = 0; x < brushImage->getDimension().Width; ++x) {
                for (auto y = 0; y < brushImage->getDimension().Height; ++y) {
                    auto brushColor = brushImage->getPixel(x, y);
                    auto originalColor = tempImage->getPixel(point.X + x, point.Y + y);

                    if (brushColor.getAlpha() == 0) {
                        continue;
                    }

                    if (brushColor.getAlpha() == 255) {
                        tempImage->setPixel(point.X + x, point.Y + y, brushColor, false);
                        continue;
                    }

                    /*
                        Alpha blending.
                        
                        Important: the "front" or "top" or "overlay" color must be (r0, g0, b0, a0)
                        whilst the "back" or "bottom" or "background" color must be (r1, g1, b1, a1)
                        or the results will be unpredictable
                    */
                    auto a1 = originalColor.getAlpha();
                    auto r1 = originalColor.getRed();
                    auto g1 = originalColor.getGreen();
                    auto b1 = originalColor.getBlue();

                    auto a0 = brushColor.getAlpha();
                    auto r0 = brushColor.getRed();
                    auto g0 = brushColor.getGreen();
                    auto b0 = brushColor.getBlue();

                    auto a00 = a0 / 255.f;
                    auto a01 = a1 / 255.f;

                    auto finalColor = irr::video::SColor(
                        255 * (a00 + (a01 * (1 - a00))),
                        ((r0 * a00) + (r1 * a01 * (1 - a00))),
                        ((g0* a00) + (g1 * a01 * (1 - a00))),
                        ((b0* a00) + (b1 * a01 * (1 - a00)))
                    );

                    tempImage->setPixel(point.X + x, point.Y + y, finalColor, false);
                }
            }

            driver->removeTexture(tempTexture);
            tempTexture = driver->addTexture("__tempTexture__", tempImage);

            selectedNode->setMaterialTexture(0, tempTexture);

            texturePreviewImage->setImage(tempTexture);

            if (isDrawing)
            {
                tempImage->copyTo(selectedTextureImage);
            }
        }
    }
}

void ApplicationDelegate::createBrush(float brushSize, float featherRadius, irr::video::SColor color)
{
    const auto size = (brushSize + featherRadius) * 2;

    auto brush = driver->createImage(irr::video::ECF_A8R8G8B8, irr::core::dimension2du(size, size));

    brush->fill(irr::video::SColor(0, 0, 0, 0));

    auto maxDistance = featherRadius + brushSize;

    irr::core::vector2df centre(size / 2, size / 2);

    for (auto x = 0; x < size; ++x) {
        for (auto y = 0; y < size; ++y) {
            auto distanceFromCentre = irr::core::vector2df(x, y).getDistanceFrom(centre); // sqrt(pow(((size / 2) - x), 2) + pow(((size / 2) - y), 2));

            if (distanceFromCentre <= brushSize)
            {
                brush->setPixel(x, y, color);
            }
            else if (featherRadius > 0)
            {
                if (distanceFromCentre <= brushSize + featherRadius)
                {
                    // linear interpolation:
                    //
                    // x = distanceFromCentre
                    // y = y0 + ((x - x0) * ((y1 - y0) / (x1 - x0)))
                    // 
                    // x0 = brushSize
                    // y0 = 1
                    // x1 = brushSize + featherRadius
                    // y1 = 0
                    // y = 1 + ((distanceFromCentre - brushSize) * ((0 - 1) / (brushSize + featherRadius - brushSize)) = 1 + ((distanceFromCentre - brushSize) * (-1 / featherRadius)
                    //
                    // alternatively:
                    // 
                    // x0 = brushSize + featherRadius
                    // y0 = 0
                    // x1 = brushSize
                    // y1 = 1
                    // y = 0 + ((distanceFromCentre - brushSize - featherRadius) * ((1 - 0) / (brushSize - brushSize - featherRadius))) = ((distanceFromCentre - brushSize - featherRadius) * (1 / (-featherRadius)))

                    auto multiplier = 1 + ((distanceFromCentre - brushSize) * (-1 / featherRadius));

                    brush->setPixel(x, y, irr::video::SColor(multiplier * 255, color.getRed(), color.getGreen(), color.getBlue()));
                }
            }
        }
    }

    if (brushTexture != nullptr)
    {
        driver->removeTexture(brushTexture);
    }

    brushImage = brush;

    brushTexture = driver->addTexture("__brush__", brushImage);
}

void ApplicationDelegate::beginDrawing()
{
    isDrawing = true;
}

void ApplicationDelegate::endDrawing()
{
    isDrawing = false;
}

bool ApplicationDelegate::isMouseOverGUI()
{
    auto element = device->getGUIEnvironment()->getFocus();

    if (element == nullptr)
    {
        return false;
    }

    std::string elementName = element->getName();

    return elementName != "modelViewer";
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
        modelSceneNode->remove();
    }

    if (filename.empty()) {
        std::cerr << "Could not load non-existent (empty filename) model" << std::endl;
        return;
    }

    modelMesh = smgr->getMesh(filename.c_str());

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

        auto textureImage = guienv->addImage(irr::core::recti(10, 10, tab->getAbsoluteClippingRect().getWidth() - 10, tab->getAbsoluteClippingRect().getHeight() - 10));
        
        textureImage->setImage(texture);

        textureImage->setName("image");
        textureImage->setScaleImage(true);

        tab->addChild(textureImage);

        textureImage->setScaleImage(true);

        // TODO: refactor this to be done on every __tab change__ and potentially use some caching
        selectedTextureImage = driver->createImage(texture, irr::core::vector2di(0, 0), texture->getOriginalSize());
        
        tempImage = driver->createImage(irr::video::ECF_A8R8G8B8, selectedTextureImage);
        tempTexture = driver->addTexture("__tempTexture__", tempImage);
    }

    createBrush(brushSize, brushFeatherRadius, brushColor);

    updatePropertiesWindow();

    triangleSelector = smgr->createTriangleSelector(reinterpret_cast<irr::scene::IAnimatedMeshSceneNode*>(modelSceneNode));

    auto toolWindow = reinterpret_cast<irr::gui::IGUIWindow*>(getElementByName("toolWindow"));
    toolWindow->setVisible(true);
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

    saveTextureDialog->remove();

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

    loadModelDialog->remove();

    loadModelDialogIsOpen = false;
}

void ApplicationDelegate::updatePropertiesWindow()
{
    auto brushSizeSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("brushSizeSlider"));
    brushSizeSlider->setPos(brushSize);

    auto brushFeatherSizeSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("brushFeatherSizeScroll"));
    brushFeatherSizeSlider->setPos(brushFeatherRadius);

    auto brushRedColorSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("brushColorRedSlider"));
    brushRedColorSlider->setPos(brushColor.getRed());

    auto brushGreenColorSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("brushColorGreenSlider"));
    brushGreenColorSlider->setPos(brushColor.getGreen());

    auto brushBlueColorSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("brushColorBlueSlider"));
    brushBlueColorSlider->setPos(brushColor.getRed());

    auto brushPreviewImage = reinterpret_cast<irr::gui::IGUIImage*>(getElementByName("brushPreviewImage"));
    brushPreviewImage->setImage(brushTexture);

    brushPreviewImage->setScaleImage(true);

    /*brushPreviewImage->getAbsoluteClippingRect().getWidth() < brushTexture->getSize().Width ||
    brushPreviewImage->getAbsoluteClippingRect().getHeight() < brushTexture->getSize().Height*/
}

void ApplicationDelegate::updateBrushProperties()
{
    auto brushSizeSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("brushSizeSlider"));
    brushSize = brushSizeSlider->getPos();

    auto brushFeatherSizeSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("brushFeatherSizeScroll"));
    brushFeatherRadius = brushFeatherSizeSlider->getPos();

    auto brushRedColorSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("brushColorRedSlider"));
    auto brushRed = brushRedColorSlider->getPos();

    auto brushGreenColorSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("brushColorGreenSlider"));
    auto brushGreen = brushGreenColorSlider->getPos();

    auto brushBlueColorSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("brushColorBlueSlider"));
    auto brushBlue = brushBlueColorSlider->getPos();

    brushColor = irr::video::SColor(255, brushRed, brushGreen, brushBlue);

    createBrush(brushSize, brushFeatherRadius, brushColor);

    auto brushPreviewImage = reinterpret_cast<irr::gui::IGUIImage*>(getElementByName("brushPreviewImage"));
    brushPreviewImage->setImage(brushTexture);

    brushPreviewImage->setScaleImage(
        brushPreviewImage->getAbsoluteClippingRect().getWidth() < brushTexture->getSize().Width ||
        brushPreviewImage->getAbsoluteClippingRect().getHeight() < brushTexture->getSize().Height
    );
}

void ApplicationDelegate::updateModelProperties()
{
    // auto modelScaleSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("modelScaleSlider"));
    // auto scale = modelScaleSlider->getPos();

    // auto modelRotationYSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("modelRotationYSlider"));
    // auto rotationY = modelRotationYSlider->getPos();

    auto modelOffsetXSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("modelOffsetXSlider"));
    auto x = modelOffsetXSlider->getPos();

    auto modelOffsetYSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("modelOffsetYSlider"));
    auto y = modelOffsetYSlider->getPos();

    auto modelOffsetZSlider = reinterpret_cast<irr::gui::IGUIScrollBar*>(getElementByName("modelOffsetZSlider"));
    auto z = modelOffsetZSlider->getPos();

    // TODO: changing the model transform has a negative impact on triangleSelector working - it just stops working
    // modelSceneNode->setScale(irr::core::vector3df(scale));
    // modelSceneNode->setRotation(irr::core::vector3df(0, rotationY, 0));
    camera->setPosition(irr::core::vector3df(x, y, z));

    // triangleSelector->drop();
    // triangleSelector = smgr->createTriangleSelector(reinterpret_cast<irr::scene::IAnimatedMeshSceneNode*>(modelSceneNode));
    // camera->setTriangleSelector(triangleSelector);
}
