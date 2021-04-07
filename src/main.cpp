#include <irrlicht.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int main() {
    irr::IrrlichtDevice *device =
            irr::createDevice(irr::video::EDT_BURNINGSVIDEO, irr::core::dimension2d<irr::u32>(1024, 768), 32,
                    false, false, true, 0);

    if (!device) {
        std::cerr << "Could not initialize video device\n";

        return 1;
    }

    device->setWindowCaption(L"irrPaint3D");

    irr::video::IVideoDriver* driver = device->getVideoDriver();
    irr::scene::ISceneManager* smgr = device->getSceneManager();
    irr::gui::IGUIEnvironment* guienv = device->getGUIEnvironment();

    irr::scene::IAnimatedMesh* modelMesh = smgr->getMesh(argv[1]);

    if (!modelMesh) {
        std::cerr << "Could not read model file " << argv[1] << "\n";

        device->drop();

        return 1;
    }

    irr::scene::IAnimatedMeshSceneNode* node = smgr->addAnimatedMeshSceneNode(modelMesh);

    // if (node)
    // {
    //     node->setMaterialFlag(EMF_LIGHTING, false);
    //     node->setAnimationSpeed(0);
    //     node->setMaterialTexture(0, driver->getTexture("dwarf.jpg"));
    //     //node->setMaterialTexture( 0, driver->getTexture("../../media/Sovereign_1.jpg") );
    //     //node->setScale(vector3df(0.f, 0.f, 0.f));
    // }

    // node->setVisible(false);

    // while (device->run())
    // {
    //     if (!device->isWindowActive() || !device->isWindowFocused() || device->isWindowMinimized())
    //         continue;

    //     driver->beginScene(true, true, SColor(255,100,101,140));

    //     smgr->drawAll();
    //     guienv->drawAll();

    //     driver->setTransform(video::ETS_WORLD, matrix4::EM4CONST_IDENTITY);

    //     /*for (u16 i = 0; i < seams.size(); i++)
    //     {
    //         driver->draw3DLine(seams[i]->pe1 * 1.005, seams[i]->pe2 * 1.005, SColor(55, 100, 255, 140));
    //     }*/

    //     for (u16 i = 0; i < features.size(); i++)
    //     {
    //         for (u16 t = 0; t < features[i].size(); t++)
    //         {
    //             Triangle tri = features[i][t];

    //             driver->draw3DLine(tri.positions[0] * 1.005, tri.positions[1] * 1.005, SColor(55, 100, 255, 140));
    //             driver->draw3DLine(tri.positions[0] * 1.005, tri.positions[2] * 1.005, SColor(55, 100, 255, 140));
    //             driver->draw3DLine(tri.positions[1] * 1.005, tri.positions[2] * 1.005, SColor(55, 100, 255, 140));
    //         }
    //     }

    //     driver->endScene();
    // }

    device->drop();

    return 0;
}
