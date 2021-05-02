#include "IrrlichtEventReceiver.h"

IrrlichtEventReceiver::IrrlichtEventReceiver(std::shared_ptr<ApplicationDelegate> _applicationDelegate) : applicationDelegate(std::move(_applicationDelegate))
{
}

bool IrrlichtEventReceiver::OnEvent(const irr::SEvent& event)
{
    if (event.EventType == irr::EET_KEY_INPUT_EVENT)
    {
        // CTRL+S saves texture to a file
        if (event.KeyInput.Key == irr::KEY_KEY_S && event.KeyInput.Control)
        {
            applicationDelegate->saveTexture();
        }

        return false;
    }

    //if (event.EventType == irr::EET_MOUSE_INPUT_EVENT)
    //{
    //    if (event.MouseInput.isLeftPressed() && applicationDelegate->isMouseOverGUI())
    //    {
    //        return true;
    //    }

    //    if (event.MouseInput.Event == irr::EMIE_MOUSE_MOVED)
    //    {
    //        // TODO: draw on a texture
    //    }
    //}

    if (event.EventType == irr::EET_GUI_EVENT)
    {
        if (event.GUIEvent.EventType == irr::gui::EGET_FILE_SELECTED)
        {
            auto dialog = reinterpret_cast<irr::gui::IGUIFileOpenDialog*>(event.GUIEvent.Caller);
            
            std::string dialogName = dialog->getName();

            if (dialogName == "saveTextureDialog")
            {
                applicationDelegate->saveTexture(dialog->getFileName());
            }
            else if (dialogName == "loadModelDialog")
            {
                applicationDelegate->loadModel(dialog->getFileName());
            }

            return false;
        }

        if (event.GUIEvent.EventType == irr::gui::EGET_FILE_CHOOSE_DIALOG_CANCELLED)
        {
            auto dialog = reinterpret_cast<irr::gui::IGUIFileOpenDialog*>(event.GUIEvent.Caller);

            std::string dialogName = dialog->getName();

            if (dialogName == "saveTextureDialog")
            {
                applicationDelegate->closeSaveTextureDialog();
            }
            else if (dialogName == "loadModelDialog")
            {
                applicationDelegate->closeLoadModelDialog();
            }

            return false;
        }

        if (event.GUIEvent.EventType == irr::gui::EGET_BUTTON_CLICKED)
        {
            auto button = reinterpret_cast<irr::gui::IGUIButton*>(event.GUIEvent.Caller);

            std::string buttonName = button->getName();

            if (buttonName == "saveTextureButton")
            {
                applicationDelegate->saveTexture();
            }
            else if (buttonName == "openModelButton")
            {
                applicationDelegate->openLoadModelDialog();
            }

            return false;
        }
    }

    return false;
}
