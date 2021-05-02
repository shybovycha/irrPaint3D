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
    /*
      TODO: no need for model transformation here - just use the triangle vertices -> vectors and solve for AB.normalize*u + AC.normalize*v = AP (note how AP is NOT normalized)
      
      solving this system as follows:

      A(Ax, Ay, Az)
      B(Bx, By, Bz)
      C(Cx, Cy, Cz)

      P(Px, Py, Pz)

      a = vector3df(Cx - Ax, Cy - Ay, Cz - Az).normalize() <=> a(ax, ay, az)
      b = vector3df(Bx - Ax, By - Ay, Bz - Az).normalize() <=> b(bx, by, bz)
      p = vector3df(Px - Ax, Py - Ay, Pz - Az) <=> p(px, py, pz)

      a*u + b*v = p

      ax*u + bx*v = px
      ay*u + by*v = py
      az*u + bz*v = pz

      Al = sqrt(((Cx - Ax) * (Cx - Ax)) + ((Cy - Ay) * (Cy - Ay)) + ((Cz - Az) * (Cz - Az)))
      Bl = sqrt(((Bx - Ax) * (Bx - Ax)) + ((By - Ay) * (By - Ay)) + ((Bz - Az) * (Bz - Az)))

      ((Cx - Ax) / Al) * u + ((Bx - Ax) / Bl) * v = Px
      ((Cy - Ay) / Al) * u + ((By - Ay) / Bl) * v = Py
      ((Cz - Az) / Al) * u + ((Bz - Az) / Bl) * v = Pz

      A*X=B

      where

      A = matrix:

      {
        {(Cx - Ax) / Al, (Bx - Ax) / Bl, (Px)},
        {(Cy - Ay) / Al, (By - Ay) / Bl, (Py)},
        {(Cz - Az) / Al, (Bz - Az) / Bl, (Pz)}
      }

      X = matrix:

      {{u, v, -1}}

      B = matrix:

      {{0, 0, 0}}

      then

      X = A.inverse() * B

      in Irrlicht:

      a = (C - A).normalize()
      b = (B - A).normalize()
      p = (P - a);

      A = matrix4().setM({
        a.X, b.X, p.X, 0,
        a.Y, b.Y, p.Y, 0,
        a.Z, b.Z, p.Z, 0,
        0, 0, 0, 0
      }).getInverse()

      B = vector3df(0, 0, 0)

      X = vector3df(0, 0, -1)

      (A * B).transformVect(X)

      result will be in X
    */

    //irr::core::vector3df a = (triangle.pointC - triangle.pointA).normalize();
    //irr::core::vector3df b = (triangle.pointB - triangle.pointA).normalize();
    //irr::core::vector3df p = (point - triangle.pointA);

    //irr::core::matrix4 mA = irr::core::matrix4(irr::core::matrix4::EM4CONST_NOTHING);
    //
    //mA[0] = a.X;
    //mA[1] = b.X;
    //mA[2] = p.X; // mA[3] = 0,
    //mA[4] = a.Y;
    //mA[5] = b.Y;
    //mA[6] = p.Y; // mA[7] = 0,
    //mA[8] = a.Z;
    //mA[9] = b.Z;
    //mA[10] = p.Z; // mA[11] = 0,
    //  // 0, 0, 0, 0
    //    // });
    //
    //irr::core::matrix4 mAInv = irr::core::matrix4(irr::core::matrix4::EM4CONST_NOTHING);

    //irr::core::vector3df mB = irr::core::vector3df(0, 0, 0);
    //    
    //irr::core::vector3df mX = irr::core::vector3df(0, 0, -1);

    //(mA * mB).transformVect(mX);

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

    // auto mesh = sceneNode->getMesh();
    auto meshBufferCount = mesh->getMeshBufferCount();
    auto meshBuffer = mesh->getMeshBuffer(0);

    auto vertices = static_cast<irr::video::S3DVertex*>(mesh->getMeshBuffer(0)->getVertices());

    for (auto t = 0; t < mesh->getMeshBufferCount(); ++t)
    {
        auto meshBuffer = mesh->getMeshBuffer(t);

        for (auto i = 0; i < meshBuffer->getVertexCount(); ++i)
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

    // std::cout << "uv(" << uv.X << "," << uv.Y << ")" << std::endl;

    /*auto textureImage = image->getImage();

    if (textureImage == nullptr) {
        return;
    }*/

    auto textureImage = meshSceneNode->getMaterial(1).getTexture(0);

    auto textureSize = textureImage->getOriginalSize();

    auto imageRect = image->getAbsolutePosition();

    auto point = irr::core::vector2di(imageRect.UpperLeftCorner.X + (textureSize.Width * uv.X), imageRect.UpperLeftCorner.Y + (textureSize.Height * uv.Y));

    driver->setRenderTarget(renderTarget);

    smgr->setActiveCamera(fixedCamera);

    driver->draw2DImage(textureImage, irr::core::vector2di(0, 0));

    driver->draw2DLine(irr::core::position2di(point.X - 50, point.Y - 50), irr::core::position2di(point.X + 50, point.Y + 50), TRIANGLE_COLOR);
    driver->draw2DLine(irr::core::position2di(point.X + 50, point.Y - 50), irr::core::position2di(point.X - 50, point.Y + 50), TRIANGLE_COLOR);

    driver->draw2DLine(irr::core::position2di(100, 100), irr::core::position2di(220, 220), TRIANGLE_COLOR);
    driver->draw2DLine(irr::core::position2di(220, 100), irr::core::position2di(100, 220), TRIANGLE_COLOR);

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
