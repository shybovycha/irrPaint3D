#include <irrlicht.h>
#include "driverChoice.h"

using namespace irr;
using namespace gui;

#ifdef _MSC_VER
#pragma comment(lib, "Irrlicht.lib")
#endif

IrrlichtDevice *Device = 0;
core::stringc StartUpModelFile;
core::stringw MessageText;
core::stringw Caption;
scene::ISceneNode* Model = 0;
scene::ISceneNode* SkyBox = 0;
bool Octree=false;
bool UseLight=false;

scene::ITriangleSelector* selector = 0;
scene::ISceneCollisionManager* collMan = 0;
scene::IAnimatedMesh* Mesh = 0;
video::ITexture* RTT = 0;
core::vector2di CursorOnModel;

scene::ICameraSceneNode* Camera[2] = {0, 0};

video::IImage* TextureImage(video::ITexture* texture) 
{
	video::IImage* image = Device->getVideoDriver()->createImageFromData(
		texture->getColorFormat(),
		texture->getSize(),
		texture->lock(),
		false  //copy mem
	);

	texture->unlock();

	return image;
}

video::ITexture* ImageTexture(video::IImage* image, core::stringc name) 
{
	video::ITexture* texture = Device->getVideoDriver()->addTexture(name.c_str(), image);

	if (texture)
		texture->grab();

	return texture;
}

core::vector2df getPointUV(core::triangle3df tri, core::vector3df p)
{
    core::vector3df 
    v0 = tri.pointC - tri.pointA,
    v1 = tri.pointB - tri.pointA,
    v2 = p - tri.pointA;

    float dot00 = v0.dotProduct(v0),
    dot01 = v0.dotProduct(v1),
    dot02 = v0.dotProduct(v2),
    dot11 = v1.dotProduct(v1),
    dot12 = v1.dotProduct(v2);

    float invDenom = 1.f / ((dot00 * dot11) - (dot01 * dot01)),
    u = (dot11 * dot02 - dot01 * dot12) * invDenom,
    v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    scene::IMesh* m = Mesh->getMesh(((scene::IAnimatedMeshSceneNode*)Model)->getFrameNr());
    
    core::array<video::S3DVertex> VA, VB, VC;
    video::SMaterial Material;
    
    for (unsigned int i = 0; i < m->getMeshBufferCount(); i++)
    {
    scene::IMeshBuffer* mb = m->getMeshBuffer(i);
    video::S3DVertex* vertices = (video::S3DVertex*) mb->getVertices();
    
    for (unsigned long long v = 0; v < mb->getVertexCount(); v++)
    {
        if (vertices[v].Pos == tri.pointA)
        VA.push_back(vertices[v]); else
        if (vertices[v].Pos == tri.pointB)
        VB.push_back(vertices[v]); else
        if (vertices[v].Pos == tri.pointC)
        VC.push_back(vertices[v]);
        
        if (vertices[v].Pos == tri.pointA || vertices[v].Pos == tri.pointB || vertices[v].Pos == tri.pointC)
        Material = mb->getMaterial();
        
        if (VA.size() > 0 && VB.size() > 0 && VC.size() > 0)
        break;
    }
    
    if (VA.size() > 0 && VB.size() > 0 && VC.size() > 0)
        break;
    }
    
    core::vector2df 
    A = VA[0].TCoords,
    B = VB[0].TCoords,
    C = VC[0].TCoords;

    //return (u >= 0.f) && (v >= 0.f) && (u + v <= 1.f);
    
    core::vector2df P(A + (u * (C - A)) + (v * (B - A)));
    core::dimension2du Size = Material.getTexture(0)->getSize();
    CursorOnModel = core::vector2di(Size.Width * P.X, Size.Height * P.Y);
    int X = Size.Width * P.X, Y = Size.Height * P.Y;
    
    Material.getTexture(0)->lock(true);
    Device->getVideoDriver()->setRenderTarget(Material.getTexture(0), true, true, 0);
        //Device->getVideoDriver()->draw2DRectangle(video::SColor(255, 0, 100, 75), core::rect<s32>(Size.Width - (X - 10), Size.Height - (Y - 10), 
        //    Size.Width - (X + 10), Size.Height - (Y + 10)));

        Device->getVideoDriver()->draw2DRectangle(video::SColor(255, 0, 0, 250), core::rect<s32>((X - 10), (Y - 10), 
            (X + 10), (Y + 10)));
    Device->getVideoDriver()->setRenderTarget(0, true, true, 0);
    Material.getTexture(0)->unlock();
    
    return core::vector2df(X, Y);
}

// Values used to identify individual GUI elements
enum
{
    GUI_ID_DIALOG_ROOT_WINDOW  = 0x10000,

    GUI_ID_X_SCALE,
    GUI_ID_Y_SCALE,
    GUI_ID_Z_SCALE,

    GUI_ID_OPEN_MODEL,
    GUI_ID_SET_MODEL_ARCHIVE,
    GUI_ID_LOAD_AS_OCTREE,

    GUI_ID_SKY_BOX_VISIBLE,
    GUI_ID_TOGGLE_DEBUG_INFO,

    GUI_ID_DEBUG_OFF,
    GUI_ID_DEBUG_BOUNDING_BOX,
    GUI_ID_DEBUG_NORMALS,
    GUI_ID_DEBUG_SKELETON,
    GUI_ID_DEBUG_WIRE_OVERLAY,
    GUI_ID_DEBUG_HALF_TRANSPARENT,
    GUI_ID_DEBUG_BUFFERS_BOUNDING_BOXES,
    GUI_ID_DEBUG_ALL,

    GUI_ID_MODEL_MATERIAL_SOLID,
    GUI_ID_MODEL_MATERIAL_TRANSPARENT,
    GUI_ID_MODEL_MATERIAL_REFLECTION,

    GUI_ID_CAMERA_MAYA,
    GUI_ID_CAMERA_FIRST_PERSON,

    GUI_ID_POSITION_TEXT,

    GUI_ID_ABOUT,
    GUI_ID_QUIT,

    GUI_ID_TEXTUREFILTER,
    GUI_ID_SKIN_TRANSPARENCY,
    GUI_ID_SKIN_ANIMATION_FPS,

    GUI_ID_BUTTON_SET_SCALE,
    GUI_ID_BUTTON_SCALE_MUL10,
    GUI_ID_BUTTON_SCALE_DIV10,
    GUI_ID_BUTTON_OPEN_MODEL,
    GUI_ID_BUTTON_SHOW_ABOUT,
    GUI_ID_BUTTON_SHOW_TOOLBOX,
    GUI_ID_BUTTON_SELECT_ARCHIVE,

    GUI_ID_ANIMATION_INFO,

    // And some magic numbers
    MAX_FRAMERATE = 80,
    DEFAULT_FRAMERATE = 30
};


/*
Toggle between various cameras
*/
void setActiveCamera(scene::ICameraSceneNode* newActive)
{
    if (0 == Device)
        return;

    scene::ICameraSceneNode * active = Device->getSceneManager()->getActiveCamera();
    active->setInputReceiverEnabled(false);

    newActive->setInputReceiverEnabled(true);
    Device->getSceneManager()->setActiveCamera(newActive);
}

void setSkinTransparency(s32 alpha, gui::IGUISkin * skin)
{
    for (s32 i=0; i<gui::EGDC_COUNT ; ++i)
    {
        video::SColor col = skin->getColor((EGUI_DEFAULT_COLOR)i);
        col.setAlpha(alpha);
        skin->setColor((EGUI_DEFAULT_COLOR)i, col);
    }
}

void updateScaleInfo(scene::ISceneNode* model)
{
    IGUIElement* toolboxWnd = Device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(GUI_ID_DIALOG_ROOT_WINDOW, true);
    if (!toolboxWnd)
        return;
    if (!model)
    {
        toolboxWnd->getElementFromId(GUI_ID_X_SCALE, true)->setText( L"-" );
        toolboxWnd->getElementFromId(GUI_ID_Y_SCALE, true)->setText( L"-" );
        toolboxWnd->getElementFromId(GUI_ID_Z_SCALE, true)->setText( L"-" );
    }
    else
    {
        core::vector3df scale = model->getScale();
        toolboxWnd->getElementFromId(GUI_ID_X_SCALE, true)->setText( core::stringw(scale.X).c_str() );
        toolboxWnd->getElementFromId(GUI_ID_Y_SCALE, true)->setText( core::stringw(scale.Y).c_str() );
        toolboxWnd->getElementFromId(GUI_ID_Z_SCALE, true)->setText( core::stringw(scale.Z).c_str() );
    }
}

void showAboutText()
{
    Device->getGUIEnvironment()->addMessageBox(
        Caption.c_str(), MessageText.c_str());
}

void loadModel(const c8* fn)
{
    io::path filename(fn);

    io::path extension;
    core::getFileNameExtension(extension, filename);
    extension.make_lower();

    // if a texture is loaded apply it to the current model..
    if (extension == ".jpg" || extension == ".pcx" ||
        extension == ".png" || extension == ".ppm" ||
        extension == ".pgm" || extension == ".pbm" ||
        extension == ".psd" || extension == ".tga" ||
        extension == ".bmp" || extension == ".wal" ||
        extension == ".rgb" || extension == ".rgba")
    {
        video::ITexture * texture =
            Device->getVideoDriver()->getTexture( filename );
        if ( texture && Model )
        {
            // always reload texture
            Device->getVideoDriver()->removeTexture(texture);
            texture = Device->getVideoDriver()->getTexture( filename );

            Model->setMaterialTexture(0, texture);
        }
        return;
    }
    // if a archive is loaded add it to the FileArchive..
    else if (extension == ".pk3" || extension == ".zip" || extension == ".pak" || extension == ".npk")
    {
        Device->getFileSystem()->addFileArchive(filename.c_str());
        return;
    }

    // load a model into the engine

    if (Model)
        Model->remove();

    Model = 0;

    if (extension==".irr")
    {
        core::array<scene::ISceneNode*> outNodes;
        Device->getSceneManager()->loadScene(filename);
        Device->getSceneManager()->getSceneNodesFromType(scene::ESNT_ANIMATED_MESH, outNodes);
        if (outNodes.size())
            Model = outNodes[0];
        return;
    }

    scene::IAnimatedMesh* m = Device->getSceneManager()->getMesh( filename.c_str() );

    if (!m)
    {
        // model could not be loaded

        if (StartUpModelFile != filename)
            Device->getGUIEnvironment()->addMessageBox(
            Caption.c_str(), L"The model could not be loaded. " \
            L"Maybe it is not a supported file format.");
        return;
    }

    // set default material properties

    if (Octree)
    {
        Model = Device->getSceneManager()->addOctreeSceneNode(m->getMesh(0));
        selector = Device->getSceneManager()->createOctreeTriangleSelector(m->getMesh(0), Model);
    }
    else
    {
        scene::IAnimatedMeshSceneNode* animModel = Device->getSceneManager()->addAnimatedMeshSceneNode(m);
        animModel->setAnimationSpeed(0);
        Model = animModel;
        selector = Device->getSceneManager()->createTriangleSelector(animModel);
    }
    
    Mesh = m;
    
    Model->setMaterialFlag(video::EMF_LIGHTING, UseLight);
    Model->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, UseLight);
//  Model->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);
    Model->setDebugDataVisible(scene::EDS_OFF);


    // we need to uncheck the menu entries. would be cool to fake a menu event, but
    // that's not so simple. so we do it brute force
    gui::IGUIContextMenu* menu = (gui::IGUIContextMenu*)Device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(GUI_ID_TOGGLE_DEBUG_INFO, true);
    if (menu)
        for(int item = 1; item < 6; ++item)
            menu->setItemChecked(item, false);
    updateScaleInfo(Model);

	Device->getTimer()->setTime(0);
	Device->getTimer()->setSpeed();
	Device->getTimer()->start();

	printf("Let's start texture update. Now is: %d -> %d\n", Device->getTimer()->getTime(), Device->getTimer()->getRealTime());
    
    /*video::IImage* UberTexture = 0;

	u32 mbCnt = Mesh->getMeshBufferCount();
    
    for (int q = 0; q < mbCnt; q++)
    {
        scene::IMeshBuffer* mb = Mesh->getMeshBuffer(q);

        u16* indices = mb->getIndices();
        video::S3DVertex* vertices = (video::S3DVertex*) mb->getVertices();
		u32 indexCnt = mb->getIndexCount();

        for (u32 i = 0; i < indexCnt; i += 3)
        {
            core::vector3df vA = vertices[indices[i]].Pos,
                vB = vertices[indices[i + 1]].Pos,
                vC = vertices[indices[i + 2]].Pos;
            
            f32 a = (vB - vA).getLength(), b = (vC - vB).getLength(), c = (vC - vA).getLength();
            f32 x = ((b * b) + (a * a) - (c * c)) / (2.f * a * b), y = sqrt(1.f - (x * x));
            f32 R = c / y; // http://en.wikipedia.org/wiki/Circumscribed_circle

            core::vector2di A(0 + (R / 2), 0 + (R / 2)), B((b * x) + (R / 2), (b * y) + (R / 2)), C(a + (R / 2), 0 + (R / 2));

			video::ITexture* rt = Device->getVideoDriver()->addRenderTargetTexture(core::dimension2du(R, R), "rt001");
			Device->getVideoDriver()->setRenderTarget(rt, true, true, video::SColor(0, 0, 0, 255));
			Device->getVideoDriver()->draw2DLine(A, B, video::SColor(255, 0, 100, 255));
			Device->getVideoDriver()->draw2DLine(B, C, video::SColor(255, 0, 100, 255));
			Device->getVideoDriver()->draw2DLine(C, A, video::SColor(255, 0, 100, 255));
			Device->getVideoDriver()->setRenderTarget(0, true, true, 0);

			printf("Drawn <(%d , %d), (%d , %d), (%d , %d)> :: (%d / %d) frame\n", A.X, A.Y, B.X, B.Y, C.X, C.Y, i, indexCnt);

			if (UberTexture)
			{
				core::dimension2du biggerDimension = UberTexture->getDimension();

				biggerDimension.Width += R;

				if (R > biggerDimension.Height)
					biggerDimension.Height = R;

				video::ITexture* tmp001 = ImageTexture(UberTexture, "tmp001");

				video::IImage* uberTexture2 = Device->getVideoDriver()->createImage(video::ECF_A8R8G8B8, biggerDimension);
				Device->getVideoDriver()->draw2DImage(tmp001, core::vector2di(0, 0));
				Device->getVideoDriver()->draw2DImage(rt, core::vector2di(UberTexture->getDimension().Width, 0));
				UberTexture->grab();
				UberTexture = uberTexture2;
			} else
			{
				video::ITexture* tmp001 = ImageTexture(UberTexture, "tmp001");

				UberTexture = Device->getVideoDriver()->createImage(video::ECF_A8R8G8B8, core::dimension2du(R, R));
				Device->getVideoDriver()->draw2DImage(tmp001, core::vector2di(0, 0));
			}

			if (rt)
				rt->grab();

			io::IWriteFile* file = Device->getFileSystem()->createAndWriteFile("zzUberTexture.png");
			Device->getVideoDriver()->getImageWriter(0)->writeImage(file, UberTexture);
			file->drop();
            
			vertices[indices[i]].TCoords = core::vector2df((f32) A.X, (f32) A.Y);
			vertices[indices[i + 1]].TCoords = core::vector2df((f32) B.X, (f32) B.Y);
			vertices[indices[i + 2]].TCoords = core::vector2df((f32) C.X, (f32) C.Y);
        }
    }

	printf("Texture generation: %d -> %d\n", Device->getTimer()->getTime(), Device->getTimer()->getRealTime());

	Device->getTimer()->stop();
	Device->getTimer()->setTime(0);
	Device->getTimer()->start();

	u32 uberW = UberTexture->getDimension().Width, uberH = UberTexture->getDimension().Height;

	for (int q = 0; q < Mesh->getMeshBufferCount(); q++)
    {
        scene::IMeshBuffer* mb = Mesh->getMeshBuffer(q);

		char* s = new char[80];

		sprintf(s, "zzUberTexture%0.3d", q);

		mb->getMaterial().setTexture(0, ImageTexture(UberTexture, s));

        video::S3DVertex* vertices = (video::S3DVertex*) mb->getVertices();

		for (int i = 0; i < mb->getVertexCount(); i++)
        {
            vertices[i].TCoords.X /= uberW;
            vertices[i].TCoords.Y /= uberH;

            //driver->draw3DTriangle(core::triangle3df(vertices[indices[i]].Pos, vertices[indices[i + 1]].Pos, vertices[indices[i + 2]].Pos), video::SColor(0, 100, 200, 75));
        }
    }

	printf("Texture appliance: %d -> %d\n", Device->getTimer()->getTime(), Device->getTimer()->getRealTime());

	Device->getTimer()->stop();*/
}


/*
Finally, the third function creates a toolbox window. In this simple mesh
viewer, this toolbox only contains a tab control with three edit boxes for
changing the scale of the displayed model.
*/
void createToolBox()
{
    // remove tool box if already there
    IGUIEnvironment* env = Device->getGUIEnvironment();
    IGUIElement* root = env->getRootGUIElement();
    IGUIElement* e = root->getElementFromId(GUI_ID_DIALOG_ROOT_WINDOW, true);
    if (e)
        e->remove();

    // create the toolbox window
    IGUIWindow* wnd = env->addWindow(core::rect<s32>(600,45,800,480),
        false, L"Toolset", 0, GUI_ID_DIALOG_ROOT_WINDOW);

    // create tab control and tabs
    IGUITabControl* tab = env->addTabControl(
        core::rect<s32>(2,20,800-602,480-7), wnd, true, true);

    IGUITab* t1 = tab->addTab(L"Config");

    // add some edit boxes and a button to tab one
    env->addStaticText(L"Scale:",
            core::rect<s32>(10,20,60,45), false, false, t1);
    env->addStaticText(L"X:", core::rect<s32>(22,48,40,66), false, false, t1);
    env->addEditBox(L"1.0", core::rect<s32>(40,46,130,66), true, t1, GUI_ID_X_SCALE);
    env->addStaticText(L"Y:", core::rect<s32>(22,82,40,96), false, false, t1);
    env->addEditBox(L"1.0", core::rect<s32>(40,76,130,96), true, t1, GUI_ID_Y_SCALE);
    env->addStaticText(L"Z:", core::rect<s32>(22,108,40,126), false, false, t1);
    env->addEditBox(L"1.0", core::rect<s32>(40,106,130,126), true, t1, GUI_ID_Z_SCALE);

    env->addButton(core::rect<s32>(10,134,85,165), t1, GUI_ID_BUTTON_SET_SCALE, L"Set");

    // quick scale buttons
    env->addButton(core::rect<s32>(65,20,95,40), t1, GUI_ID_BUTTON_SCALE_MUL10, L"* 10");
    env->addButton(core::rect<s32>(100,20,130,40), t1, GUI_ID_BUTTON_SCALE_DIV10, L"* 0.1");

    updateScaleInfo(Model);

    // add transparency control
    env->addStaticText(L"GUI Transparency Control:",
            core::rect<s32>(10,200,150,225), true, false, t1);
    IGUIScrollBar* scrollbar = env->addScrollBar(true,
            core::rect<s32>(10,225,150,240), t1, GUI_ID_SKIN_TRANSPARENCY);
    scrollbar->setMax(255);
    scrollbar->setPos(255);

    // add framerate control
    env->addStaticText(L":", core::rect<s32>(10,240,150,265), true, false, t1);
    env->addStaticText(L"Framerate:",
            core::rect<s32>(12,240,75,265), false, false, t1);
    env->addStaticText(L"", core::rect<s32>(75,240,200,265), false, false, t1,
            GUI_ID_ANIMATION_INFO);
    scrollbar = env->addScrollBar(true,
            core::rect<s32>(10,265,150,280), t1, GUI_ID_SKIN_ANIMATION_FPS);
    scrollbar->setMax(MAX_FRAMERATE);
    scrollbar->setMin(-MAX_FRAMERATE);
    scrollbar->setPos(DEFAULT_FRAMERATE);
    scrollbar->setSmallStep(1);
}

void updateToolBox()
{
    IGUIEnvironment* env = Device->getGUIEnvironment();
    IGUIElement* root = env->getRootGUIElement();
    IGUIElement* dlg = root->getElementFromId(GUI_ID_DIALOG_ROOT_WINDOW, true);
    if (!dlg )
        return;

    // update the info we have about the animation of the model
    IGUIStaticText *  aniInfo = (IGUIStaticText *)(dlg->getElementFromId(GUI_ID_ANIMATION_INFO, true));
    if (aniInfo)
    {
        if ( Model && scene::ESNT_ANIMATED_MESH == Model->getType() )
        {
            scene::IAnimatedMeshSceneNode* animatedModel = (scene::IAnimatedMeshSceneNode*)Model;

            core::stringw str( (s32)core::round_(animatedModel->getAnimationSpeed()) );
            str += L" Frame: ";
            str += core::stringw((s32)animatedModel->getFrameNr());
            aniInfo->setText(str.c_str());
        }
        else
            aniInfo->setText(L"");
    }
}

/*
To get all the events sent by the GUI Elements, we need to create an event
receiver. This one is really simple. If an event occurs, it checks the id of
the caller and the event type, and starts an action based on these values. For
example, if a menu item with id GUI_ID_OPEN_MODEL was selected, if opens a file-open-dialog.
*/
class MyEventReceiver : public IEventReceiver
{
public:
    virtual bool OnEvent(const SEvent& event)
    {
        // Escape swaps Camera Input
        if (event.EventType == EET_KEY_INPUT_EVENT &&
            event.KeyInput.PressedDown == false)
        {
            if ( OnKeyUp(event.KeyInput.Key) )
                return true;
        }

        if (event.EventType == EET_MOUSE_INPUT_EVENT)
        {
            core::line3d<f32> ray = collMan->getRayFromScreenCoordinates(core::position2di(event.MouseInput.X, event.MouseInput.Y));
            
            scene::ICameraSceneNode* camera = Device->getSceneManager()->getActiveCamera();
            scene::ISceneNode *hitNode;
            core::vector3df hitPoint;
            core::triangle3df hitTriangle;

            collMan->getCollisionPoint(ray, selector, hitPoint, hitTriangle, (const scene::ISceneNode *&) hitNode);

            if (hitNode == Model)
            {
        core::vector2df hitUV = getPointUV(hitTriangle, hitPoint);
        
                printf("hit: (%0.3f , %0.3f , %0.3f) => (%0.3f , %0.3f)\n\n", 
                    hitPoint.X, hitPoint.Y, hitPoint.Z,
                    hitUV.X, hitUV.Y);
            }
        }

        if (event.EventType == EET_GUI_EVENT)
        {
            s32 id = event.GUIEvent.Caller->getID();
            IGUIEnvironment* env = Device->getGUIEnvironment();

            switch(event.GUIEvent.EventType)
            {
            case EGET_MENU_ITEM_SELECTED:
                    // a menu item was clicked
                    OnMenuItemSelected( (IGUIContextMenu*)event.GUIEvent.Caller );
                break;

            case EGET_FILE_SELECTED:
                {
                    // load the model file, selected in the file open dialog
                    IGUIFileOpenDialog* dialog =
                        (IGUIFileOpenDialog*)event.GUIEvent.Caller;
                    loadModel(core::stringc(dialog->getFileName()).c_str());
                }
                break;

            case EGET_SCROLL_BAR_CHANGED:

                // control skin transparency
                if (id == GUI_ID_SKIN_TRANSPARENCY)
                {
                    const s32 pos = ((IGUIScrollBar*)event.GUIEvent.Caller)->getPos();
                    setSkinTransparency(pos, env->getSkin());
                }
                // control animation speed
                else if (id == GUI_ID_SKIN_ANIMATION_FPS)
                {
                    const s32 pos = ((IGUIScrollBar*)event.GUIEvent.Caller)->getPos();
                    if (scene::ESNT_ANIMATED_MESH == Model->getType())
                        ((scene::IAnimatedMeshSceneNode*)Model)->setAnimationSpeed((f32)pos);
                }
                break;

            case EGET_COMBO_BOX_CHANGED:

                // control anti-aliasing/filtering
                if (id == GUI_ID_TEXTUREFILTER)
                {
                    OnTextureFilterSelected( (IGUIComboBox*)event.GUIEvent.Caller );
                }
                break;

            case EGET_BUTTON_CLICKED:

                switch(id)
                {
                case GUI_ID_BUTTON_SET_SCALE:
                    {
                        // set scale
                        gui::IGUIElement* root = env->getRootGUIElement();
                        core::vector3df scale;
                        core::stringc s;

                        s = root->getElementFromId(GUI_ID_X_SCALE, true)->getText();
                        scale.X = (f32)atof(s.c_str());
                        s = root->getElementFromId(GUI_ID_Y_SCALE, true)->getText();
                        scale.Y = (f32)atof(s.c_str());
                        s = root->getElementFromId(GUI_ID_Z_SCALE, true)->getText();
                        scale.Z = (f32)atof(s.c_str());

                        if (Model)
                            Model->setScale(scale);
                        updateScaleInfo(Model);
                    }
                    break;
                case GUI_ID_BUTTON_SCALE_MUL10:
                    if (Model)
                        Model->setScale(Model->getScale()*10.f);
                    updateScaleInfo(Model);
                    break;
                case GUI_ID_BUTTON_SCALE_DIV10:
                    if (Model)
                        Model->setScale(Model->getScale()*0.1f);
                    updateScaleInfo(Model);
                    break;
                case GUI_ID_BUTTON_OPEN_MODEL:
                    env->addFileOpenDialog(L"Please select a model file to open");
                    break;
                case GUI_ID_BUTTON_SHOW_ABOUT:
                    showAboutText();
                    break;
                case GUI_ID_BUTTON_SHOW_TOOLBOX:
                    createToolBox();
                    break;
                case GUI_ID_BUTTON_SELECT_ARCHIVE:
                    env->addFileOpenDialog(L"Please select your game archive/directory");
                    break;
                }

                break;
            default:
                break;
            }
        }

        return false;
    }


    /*
        Handle key-up events
    */
    bool OnKeyUp(EKEY_CODE keyCode)
    {
        if (keyCode == KEY_ESCAPE)
        {
            if (Device)
            {
                scene::ICameraSceneNode * camera =
                    Device->getSceneManager()->getActiveCamera();
                if (camera)
                {
                    camera->setInputReceiverEnabled( !camera->isInputReceiverEnabled() );
                }
                return true;
            }
        }
        else if (keyCode == KEY_F1)
        {
            if (Device)
            {
                IGUIElement* elem = Device->getGUIEnvironment()->getRootGUIElement()->getElementFromId(GUI_ID_POSITION_TEXT);
                if (elem)
                    elem->setVisible(!elem->isVisible());
            }
        }
        else if (keyCode == KEY_KEY_M)
        {
            if (Device)
                Device->minimizeWindow();
        }
        else if (keyCode == KEY_KEY_L)
        {
            UseLight=!UseLight;
            if (Model)
            {
                Model->setMaterialFlag(video::EMF_LIGHTING, UseLight);
                Model->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, UseLight);
            }
        }
        return false;
    }


    /*
        Handle "menu item clicked" events.
    */
    void OnMenuItemSelected( IGUIContextMenu* menu )
    {
        s32 id = menu->getItemCommandId(menu->getSelectedItem());
        IGUIEnvironment* env = Device->getGUIEnvironment();

        switch(id)
        {
        case GUI_ID_OPEN_MODEL: // FilOnButtonSetScalinge -> Open Model
            env->addFileOpenDialog(L"Please select a model file to open");
            break;
        case GUI_ID_SET_MODEL_ARCHIVE: // File -> Set Model Archive
            env->addFileOpenDialog(L"Please select your game archive/directory");
            break;
        case GUI_ID_LOAD_AS_OCTREE: // File -> LoadAsOctree
            Octree = !Octree;
            menu->setItemChecked(menu->getSelectedItem(), Octree);
            break;
        case GUI_ID_QUIT: // File -> Quit
            Device->closeDevice();
            break;
        case GUI_ID_SKY_BOX_VISIBLE: // View -> Skybox
            menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
            SkyBox->setVisible(!SkyBox->isVisible());
            break;
        case GUI_ID_DEBUG_OFF: // View -> Debug Information
            menu->setItemChecked(menu->getSelectedItem()+1, false);
            menu->setItemChecked(menu->getSelectedItem()+2, false);
            menu->setItemChecked(menu->getSelectedItem()+3, false);
            menu->setItemChecked(menu->getSelectedItem()+4, false);
            menu->setItemChecked(menu->getSelectedItem()+5, false);
            menu->setItemChecked(menu->getSelectedItem()+6, false);
            if (Model)
                Model->setDebugDataVisible(scene::EDS_OFF);
            break;
        case GUI_ID_DEBUG_BOUNDING_BOX: // View -> Debug Information
            menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
            if (Model)
                Model->setDebugDataVisible((scene::E_DEBUG_SCENE_TYPE)(Model->isDebugDataVisible()^scene::EDS_BBOX));
            break;
        case GUI_ID_DEBUG_NORMALS: // View -> Debug Information
            menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
            if (Model)
                Model->setDebugDataVisible((scene::E_DEBUG_SCENE_TYPE)(Model->isDebugDataVisible()^scene::EDS_NORMALS));
            break;
        case GUI_ID_DEBUG_SKELETON: // View -> Debug Information
            menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
            if (Model)
                Model->setDebugDataVisible((scene::E_DEBUG_SCENE_TYPE)(Model->isDebugDataVisible()^scene::EDS_SKELETON));
            break;
        case GUI_ID_DEBUG_WIRE_OVERLAY: // View -> Debug Information
            menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
            if (Model)
                Model->setDebugDataVisible((scene::E_DEBUG_SCENE_TYPE)(Model->isDebugDataVisible()^scene::EDS_MESH_WIRE_OVERLAY));
            break;
        case GUI_ID_DEBUG_HALF_TRANSPARENT: // View -> Debug Information
            menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
            if (Model)
                Model->setDebugDataVisible((scene::E_DEBUG_SCENE_TYPE)(Model->isDebugDataVisible()^scene::EDS_HALF_TRANSPARENCY));
            break;
        case GUI_ID_DEBUG_BUFFERS_BOUNDING_BOXES: // View -> Debug Information
            menu->setItemChecked(menu->getSelectedItem(), !menu->isItemChecked(menu->getSelectedItem()));
            if (Model)
                Model->setDebugDataVisible((scene::E_DEBUG_SCENE_TYPE)(Model->isDebugDataVisible()^scene::EDS_BBOX_BUFFERS));
            break;
        case GUI_ID_DEBUG_ALL: // View -> Debug Information
            menu->setItemChecked(menu->getSelectedItem()-1, true);
            menu->setItemChecked(menu->getSelectedItem()-2, true);
            menu->setItemChecked(menu->getSelectedItem()-3, true);
            menu->setItemChecked(menu->getSelectedItem()-4, true);
            menu->setItemChecked(menu->getSelectedItem()-5, true);
            menu->setItemChecked(menu->getSelectedItem()-6, true);
            if (Model)
                Model->setDebugDataVisible(scene::EDS_FULL);
            break;
        case GUI_ID_ABOUT: // Help->About
            showAboutText();
            break;
        case GUI_ID_MODEL_MATERIAL_SOLID: // View -> Material -> Solid
            if (Model)
                Model->setMaterialType(video::EMT_SOLID);
            break;
        case GUI_ID_MODEL_MATERIAL_TRANSPARENT: // View -> Material -> Transparent
            if (Model)
                Model->setMaterialType(video::EMT_TRANSPARENT_ADD_COLOR);
            break;
        case GUI_ID_MODEL_MATERIAL_REFLECTION: // View -> Material -> Reflection
            if (Model)
                Model->setMaterialType(video::EMT_SPHERE_MAP);
            break;

        case GUI_ID_CAMERA_MAYA:
            setActiveCamera(Camera[0]);
            break;
        case GUI_ID_CAMERA_FIRST_PERSON:
            setActiveCamera(Camera[1]);
            break;
        }
    }

    /*
        Handle the event that one of the texture-filters was selected in the corresponding combobox.
    */
    void OnTextureFilterSelected( IGUIComboBox* combo )
    {
        s32 pos = combo->getSelected();
        switch (pos)
        {
            case 0:
            if (Model)
            {
                Model->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
                Model->setMaterialFlag(video::EMF_TRILINEAR_FILTER, false);
                Model->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, false);
            }
            break;
            case 1:
            if (Model)
            {
                Model->setMaterialFlag(video::EMF_BILINEAR_FILTER, true);
                Model->setMaterialFlag(video::EMF_TRILINEAR_FILTER, false);
            }
            break;
            case 2:
            if (Model)
            {
                Model->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
                Model->setMaterialFlag(video::EMF_TRILINEAR_FILTER, true);
            }
            break;
            case 3:
            if (Model)
            {
                Model->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, true);
            }
            break;
            case 4:
            if (Model)
            {
                Model->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, false);
            }
            break;
        }
    }
};


/*
Most of the hard work is done. We only need to create the Irrlicht Engine
device and all the buttons, menus and toolbars. We start up the engine as
usual, using createDevice(). To make our application catch events, we set our
eventreceiver as parameter. As you can see, there is also a call to
IrrlichtDevice::setResizeable(). This makes the render window resizeable, which
is quite useful for a mesh viewer.
*/
int main(int argc, char* argv[])
{
    // ask user for driver
    video::E_DRIVER_TYPE driverType=driverChoiceConsole();
    if (driverType==video::EDT_COUNT)
        return 1;

    // create device and exit if creation failed
    MyEventReceiver receiver;
    Device = createDevice(driverType, core::dimension2d<u32>(800, 600),
        16, false, false, false, &receiver);

    if (Device == 0)
        return 1; // could not create selected driver.

    Device->setResizable(true);

    Device->setWindowCaption(L"Irrlicht Engine - Loading...");

    video::IVideoDriver* driver = Device->getVideoDriver();
    IGUIEnvironment* env = Device->getGUIEnvironment();
    scene::ISceneManager* smgr = Device->getSceneManager();
    smgr->getParameters()->setAttribute(scene::COLLADA_CREATE_SCENE_INSTANCES, true);

    driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, true);

    smgr->addLightSceneNode(0, core::vector3df(200,200,200),
        video::SColorf(1.0f,1.0f,1.0f),2000);
    smgr->setAmbientLight(video::SColorf(0.3f,0.3f,0.3f));
    collMan = smgr->getSceneCollisionManager();
    // add our media directory as "search path"
    Device->getFileSystem()->addFolderFileArchive("../../media/");

    /*
    The next step is to read the configuration file. It is stored in the xml
    format and looks a little bit like this:

    @verbatim
    <?xml version="1.0"?>
    <config>
        <startUpModel file="some filename" />
        <messageText caption="Irrlicht Engine Mesh Viewer">
            Hello!
        </messageText>
    </config>
    @endverbatim

    We need the data stored in there to be written into the global variables
    StartUpModelFile, MessageText and Caption. This is now done using the
    Irrlicht Engine integrated XML parser:
    */

    // read configuration from xml file

    io::IXMLReader* xml = Device->getFileSystem()->createXMLReader( L"config.xml");

    while(xml && xml->read())
    {
        switch(xml->getNodeType())
        {
        case io::EXN_TEXT:
            // in this xml file, the only text which occurs is the
            // messageText
            MessageText = xml->getNodeData();
            break;
        case io::EXN_ELEMENT:
            {
                if (core::stringw("startUpModel") == xml->getNodeName())
                    StartUpModelFile = xml->getAttributeValue(L"file");
                else
                if (core::stringw("messageText") == xml->getNodeName())
                    Caption = xml->getAttributeValue(L"caption");
            }
            break;
        default:
            break;
        }
    }

    if (xml)
        xml->drop(); // don't forget to delete the xml reader

    if (argc > 1)
        StartUpModelFile = argv[1];

    /*
    That wasn't difficult. Now we'll set a nicer font and create the Menu.
    It is possible to create submenus for every menu item. The call
    menu->addItem(L"File", -1, true, true); for example adds a new menu
    Item with the name "File" and the id -1. The following parameter says
    that the menu item should be enabled, and the last one says, that there
    should be a submenu. The submenu can now be accessed with
    menu->getSubMenu(0), because the "File" entry is the menu item with
    index 0.
    */

    // set a nicer font

    IGUISkin* skin = env->getSkin();
    IGUIFont* font = env->getFont("fonthaettenschweiler.bmp");
    if (font)
        skin->setFont(font);

    // create menu
    gui::IGUIContextMenu* menu = env->addMenu();
    menu->addItem(L"File", -1, true, true);
    menu->addItem(L"View", -1, true, true);
    menu->addItem(L"Camera", -1, true, true);
    menu->addItem(L"Help", -1, true, true);

    gui::IGUIContextMenu* submenu;
    submenu = menu->getSubMenu(0);
    submenu->addItem(L"Open Model File & Texture...", GUI_ID_OPEN_MODEL);
    submenu->addItem(L"Set Model Archive...", GUI_ID_SET_MODEL_ARCHIVE);
    submenu->addItem(L"Load as Octree", GUI_ID_LOAD_AS_OCTREE);
    submenu->addSeparator();
    submenu->addItem(L"Quit", GUI_ID_QUIT);

    submenu = menu->getSubMenu(1);
    submenu->addItem(L"sky box visible", GUI_ID_SKY_BOX_VISIBLE, true, false, true);
    submenu->addItem(L"toggle model debug information", GUI_ID_TOGGLE_DEBUG_INFO, true, true);
    submenu->addItem(L"model material", -1, true, true );

    submenu = submenu->getSubMenu(1);
    submenu->addItem(L"Off", GUI_ID_DEBUG_OFF);
    submenu->addItem(L"Bounding Box", GUI_ID_DEBUG_BOUNDING_BOX);
    submenu->addItem(L"Normals", GUI_ID_DEBUG_NORMALS);
    submenu->addItem(L"Skeleton", GUI_ID_DEBUG_SKELETON);
    submenu->addItem(L"Wire overlay", GUI_ID_DEBUG_WIRE_OVERLAY);
    submenu->addItem(L"Half-Transparent", GUI_ID_DEBUG_HALF_TRANSPARENT);
    submenu->addItem(L"Buffers bounding boxes", GUI_ID_DEBUG_BUFFERS_BOUNDING_BOXES);
    submenu->addItem(L"All", GUI_ID_DEBUG_ALL);

    submenu = menu->getSubMenu(1)->getSubMenu(2);
    submenu->addItem(L"Solid", GUI_ID_MODEL_MATERIAL_SOLID);
    submenu->addItem(L"Transparent", GUI_ID_MODEL_MATERIAL_TRANSPARENT);
    submenu->addItem(L"Reflection", GUI_ID_MODEL_MATERIAL_REFLECTION);

    submenu = menu->getSubMenu(2);
    submenu->addItem(L"Maya Style", GUI_ID_CAMERA_MAYA);
    submenu->addItem(L"First Person", GUI_ID_CAMERA_FIRST_PERSON);

    submenu = menu->getSubMenu(3);
    submenu->addItem(L"About", GUI_ID_ABOUT);

    /*
    Below the menu we want a toolbar, onto which we can place colored
    buttons and important looking stuff like a senseless combobox.
    */

    // create toolbar

    gui::IGUIToolBar* bar = env->addToolBar();

    video::ITexture* image = driver->getTexture("open.png");
    bar->addButton(GUI_ID_BUTTON_OPEN_MODEL, 0, L"Open a model",image, 0, false, true);

    image = driver->getTexture("tools.png");
    bar->addButton(GUI_ID_BUTTON_SHOW_TOOLBOX, 0, L"Open Toolset",image, 0, false, true);

    image = driver->getTexture("zip.png");
    bar->addButton(GUI_ID_BUTTON_SELECT_ARCHIVE, 0, L"Set Model Archive",image, 0, false, true);

    image = driver->getTexture("help.png");
    bar->addButton(GUI_ID_BUTTON_SHOW_ABOUT, 0, L"Open Help", image, 0, false, true);

    // create a combobox for texture filters

    gui::IGUIComboBox* box = env->addComboBox(core::rect<s32>(250,4,350,23), bar, GUI_ID_TEXTUREFILTER);
    box->addItem(L"No filtering");
    box->addItem(L"Bilinear");
    box->addItem(L"Trilinear");
    box->addItem(L"Anisotropic");
    box->addItem(L"Isotropic");

    /*
    To make the editor look a little bit better, we disable transparent gui
    elements, and add an Irrlicht Engine logo. In addition, a text showing
    the current frames per second value is created and the window caption is
    changed.
    */

    // disable alpha

    for (s32 i=0; i<gui::EGDC_COUNT ; ++i)
    {
        video::SColor col = env->getSkin()->getColor((gui::EGUI_DEFAULT_COLOR)i);
        col.setAlpha(255);
        env->getSkin()->setColor((gui::EGUI_DEFAULT_COLOR)i, col);
    }

    // add a tabcontrol

    createToolBox();

    // create fps text

    IGUIStaticText* fpstext = env->addStaticText(L"",
            core::rect<s32>(400,4,570,23), true, false, bar);

    IGUIStaticText* postext = env->addStaticText(L"",
            core::rect<s32>(10,50,470,80),false, false, 0, GUI_ID_POSITION_TEXT);
    postext->setVisible(false);

    // set window caption

    Caption += " - [";
    Caption += driver->getName();
    Caption += "]";
    Device->setWindowCaption(Caption.c_str());

    /*
    That's nearly the whole application. We simply show the about message
    box at start up, and load the first model. To make everything look
    better, a skybox is created and a user controled camera, to make the
    application a little bit more interactive. Finally, everything is drawn
    in a standard drawing loop.
    */

    // show about message box and load default model
    if (argc==1)
        showAboutText();
    loadModel(StartUpModelFile.c_str());

    // add skybox

    SkyBox = smgr->addSkyBoxSceneNode(
        driver->getTexture("irrlicht2_up.jpg"),
        driver->getTexture("irrlicht2_dn.jpg"),
        driver->getTexture("irrlicht2_lf.jpg"),
        driver->getTexture("irrlicht2_rt.jpg"),
        driver->getTexture("irrlicht2_ft.jpg"),
        driver->getTexture("irrlicht2_bk.jpg"));

    // add a camera scene node
    Camera[0] = smgr->addCameraSceneNodeMaya();
    Camera[0]->setFarValue(20000.f);
    // Maya cameras reposition themselves relative to their target, so target the location
    // where the mesh scene node is placed.
    Camera[0]->setTarget(core::vector3df(0,30,0));

    Camera[1] = smgr->addCameraSceneNodeFPS();
    Camera[1]->setFarValue(20000.f);
    Camera[1]->setPosition(core::vector3df(0,0,-70));
    Camera[1]->setTarget(core::vector3df(0,30,0));

    setActiveCamera(Camera[0]);

    // load the irrlicht engine logo
    IGUIImage *img =
        env->addImage(driver->getTexture("irrlichtlogo2.png"),
            core::position2d<s32>(10, driver->getScreenSize().Height - 128));

    // lock the logo's edges to the bottom left corner of the screen
    img->setAlignment(EGUIA_UPPERLEFT, EGUIA_UPPERLEFT,
            EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT);

    // draw everything

    while(Device->run() && driver)
    {
        if (Device->isWindowActive())
        {
            driver->beginScene(true, true, video::SColor(150,50,50,50));

            smgr->drawAll();
            env->drawAll();

            //Device->getVideoDriver()->draw3DTriangle(some_triangle, video::SColor(0,255,0,0));

            driver->endScene();

            // update information about current frame-rate
            core::stringw str(L"FPS: ");
            str.append(core::stringw(driver->getFPS()));
            str += L" Tris: ";
            str.append(core::stringw(driver->getPrimitiveCountDrawn()));
            fpstext->setText(str.c_str());

            // update information about the active camera
            scene::ICameraSceneNode* cam = Device->getSceneManager()->getActiveCamera();
            str = L"Pos: ";
            str.append(core::stringw(cam->getPosition().X));
            str += L" ";
            str.append(core::stringw(cam->getPosition().Y));
            str += L" ";
            str.append(core::stringw(cam->getPosition().Z));
            str += L" Tgt: ";
            str.append(core::stringw(cam->getTarget().X));
            str += L" ";
            str.append(core::stringw(cam->getTarget().Y));
            str += L" ";
            str.append(core::stringw(cam->getTarget().Z));
            postext->setText(str.c_str());

            // update the tool dialog
            updateToolBox();
        }
        else
            Device->yield();
    }

    Device->drop();
    return 0;
}
