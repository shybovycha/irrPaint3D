#include "SaveFileDialog.h"

//! constructor
SaveFileDialog::SaveFileDialog(const wchar_t* title,
    irr::gui::IGUIEnvironment* environment,
    irr::gui::IGUIElement* parent,
    irr::s32 id,
    bool restoreCWD,
    irr::io::path::char_type* startDir)
    : IGUIFileOpenDialog(
        environment,
        parent,
        id,
        irr::core::rect<irr::s32>(
            ((parent ? parent : environment->getRootGUIElement())->getAbsolutePosition().getWidth() -
                SAVE_FILE_DIALOG_WIDTH) / 2,
            ((parent ? parent : environment->getRootGUIElement())->getAbsolutePosition().getHeight() -
                SAVE_FILE_DIALOG_HEIGHT) / 2,
            ((parent ? parent : environment->getRootGUIElement())->getAbsolutePosition().getWidth() -
                SAVE_FILE_DIALOG_WIDTH) / 2 + SAVE_FILE_DIALOG_WIDTH,
            ((parent ? parent : environment->getRootGUIElement())->getAbsolutePosition().getHeight() -
                SAVE_FILE_DIALOG_HEIGHT) / 2 + SAVE_FILE_DIALOG_HEIGHT)
    ),
    FileNameText(nullptr),
    FileList(nullptr),
    Dragging(false)
{
#ifdef _DEBUG
    IGUIElement::setDebugName("SaveFileDialog");
#endif

    Text = title;

    FileSystem = Environment ? Environment->getFileSystem() : nullptr;

    if (FileSystem)
    {
        FileSystem->grab();

        if (restoreCWD)
        {
            RestoreDirectory = FileSystem->getWorkingDirectory();
        }

        if (startDir)
        {
            StartDirectory = startDir;
            FileSystem->changeWorkingDirectoryTo(startDir);
        }
    }
    else
    {
        return;
    }

    irr::gui::IGUISpriteBank* sprites = nullptr;
    irr::video::SColor color(255, 255, 255, 255);
    irr::gui::IGUISkin* skin = Environment->getSkin();

    if (skin)
    {
        sprites = skin->getSpriteBank();
        color = skin->getColor(irr::gui::EGDC_WINDOW_SYMBOL);
    }

    const irr::s32 buttonw = skin ? skin->getSize(irr::gui::EGDS_WINDOW_BUTTON_WIDTH) : 2;
    const irr::s32 posx = RelativeRect.getWidth() - buttonw - 4;

    CloseButton = Environment->addButton(irr::core::rect<irr::s32>(posx, 3, posx + buttonw, 3 + buttonw),
        this,
        -1,
        L"",
        skin ? skin->getDefaultText(irr::gui::EGDT_WINDOW_CLOSE) : L"Close");

    CloseButton->setSubElement(true);
    CloseButton->setTabStop(false);

    if (sprites)
    {
        CloseButton->setSpriteBank(sprites);
        CloseButton->setSprite(irr::gui::EGBS_BUTTON_UP, skin->getIcon(irr::gui::EGDI_WINDOW_CLOSE), color);
        CloseButton->setSprite(irr::gui::EGBS_BUTTON_DOWN, skin->getIcon(irr::gui::EGDI_WINDOW_CLOSE), color);
    }

    CloseButton->setAlignment(irr::gui::EGUIA_LOWERRIGHT, irr::gui::EGUIA_LOWERRIGHT, irr::gui::EGUIA_UPPERLEFT,
        irr::gui::EGUIA_UPPERLEFT);
    CloseButton->grab();

    OKButton = Environment->addButton(
        irr::core::rect<irr::s32>(RelativeRect.getWidth() - 80, 30, RelativeRect.getWidth() - 10, 50),
        this, -1, skin ? skin->getDefaultText(irr::gui::EGDT_MSG_BOX_OK) : L"OK");
    OKButton->setSubElement(true);
    OKButton->setAlignment(irr::gui::EGUIA_LOWERRIGHT, irr::gui::EGUIA_LOWERRIGHT, irr::gui::EGUIA_UPPERLEFT,
        irr::gui::EGUIA_UPPERLEFT);
    OKButton->grab();

    CancelButton = Environment->addButton(
        irr::core::rect<irr::s32>(RelativeRect.getWidth() - 80, 55, RelativeRect.getWidth() - 10, 75),
        this, -1, skin ? skin->getDefaultText(irr::gui::EGDT_MSG_BOX_CANCEL) : L"Cancel");
    CancelButton->setSubElement(true);
    CancelButton->setAlignment(irr::gui::EGUIA_LOWERRIGHT, irr::gui::EGUIA_LOWERRIGHT, irr::gui::EGUIA_UPPERLEFT,
        irr::gui::EGUIA_UPPERLEFT);
    CancelButton->grab();

    FileBox = Environment->addListBox(irr::core::rect<irr::s32>(10, 55, RelativeRect.getWidth() - 90, 230), this, -1,
        true);
    FileBox->setSubElement(true);
    FileBox->setAlignment(irr::gui::EGUIA_UPPERLEFT, irr::gui::EGUIA_LOWERRIGHT, irr::gui::EGUIA_UPPERLEFT,
        irr::gui::EGUIA_LOWERRIGHT);
    FileBox->grab();

    FileNameText = Environment->addEditBox(nullptr, irr::core::rect<irr::s32>(10, 30, RelativeRect.getWidth() - 90, 50),
        true,
        this);
    FileNameText->setSubElement(true);
    FileNameText->setAlignment(irr::gui::EGUIA_UPPERLEFT, irr::gui::EGUIA_LOWERRIGHT, irr::gui::EGUIA_UPPERLEFT,
        irr::gui::EGUIA_UPPERLEFT);
    FileNameText->grab();

    setTabGroup(true);

    fillListBox();
}


//! destructor
SaveFileDialog::~SaveFileDialog()
{
    if (CloseButton)
        CloseButton->drop();

    if (OKButton)
        OKButton->drop();

    if (CancelButton)
        CancelButton->drop();

    if (FileBox)
        FileBox->drop();

    if (FileNameText)
        FileNameText->drop();

    if (FileSystem)
    {
        // revert to original CWD if path was set in constructor
        if (!RestoreDirectory.empty())
            FileSystem->changeWorkingDirectoryTo(RestoreDirectory);

        FileSystem->drop();
    }

    if (FileList)
        FileList->drop();
}

//! returns the filename of the selected file. Returns NULL, if no file was selected.
const wchar_t* SaveFileDialog::getFileName() const
{
    return FileNameW.c_str();
}

const irr::io::path& SaveFileDialog::getFileNameP() const
{
    return FileName;
}

//! Returns the directory of the selected file. Returns NULL, if no directory was selected.
const irr::io::path& SaveFileDialog::getDirectoryName()
{
    return FileDirectoryFlat;
}

const wchar_t* SaveFileDialog::getDirectoryNameW() const
{
    return FileDirectoryFlatW.c_str();
}

void SaveFileDialog::setFileName(const irr::io::path& name)
{
    FileName = name;
    pathToStringW(FileNameW, FileName);
}

void SaveFileDialog::setDirectoryName(const irr::io::path& name)
{
    FileDirectory = name;
    FileDirectoryFlat = name;
    FileSystem->flattenFilename(FileDirectoryFlat);
    pathToStringW(FileDirectoryFlatW, FileDirectoryFlat);
}

//! called if an event happened.
bool SaveFileDialog::OnEvent(const irr::SEvent& event)
{
    if (isEnabled())
    {
        switch (event.EventType)
        {
        case irr::EET_GUI_EVENT:
            switch (event.GUIEvent.EventType)
            {
            case irr::gui::EGET_ELEMENT_FOCUS_LOST:
                Dragging = false;
                break;

            case irr::gui::EGET_BUTTON_CLICKED:
                if (event.GUIEvent.Caller == CloseButton || event.GUIEvent.Caller == CancelButton)
                {
                    sendCancelEvent();
                    remove();

                    return true;
                }
                else if (event.GUIEvent.Caller == OKButton)
                {
                    if (!FileName.empty())
                    {
                        sendSelectedEvent(irr::gui::EGET_FILE_SELECTED);
                        remove();

                        return true;
                    }

                    if (!FileDirectory.empty())
                    {
                        sendSelectedEvent(irr::gui::EGET_DIRECTORY_SELECTED);
                    }
                }
                break;

            case irr::gui::EGET_LISTBOX_CHANGED:
            {
                irr::s32 selected = FileBox->getSelected();

                if (FileList && FileSystem)
                {
                    if (FileList->isDirectory(selected))
                    {
                        setFileName("");
                        setDirectoryName(FileList->getFullFileName(selected));
                    }
                    else
                    {
                        setDirectoryName("");
                        setFileName(FileList->getFullFileName(selected));
                    }

                    return true;
                }
            }
            break;

            case irr::gui::EGET_LISTBOX_SELECTED_AGAIN:
            {
                const irr::s32 selected = FileBox->getSelected();

                if (FileList && FileSystem)
                {
                    if (FileList->isDirectory(selected))
                    {
                        setDirectoryName(FileList->getFullFileName(selected));
                        FileSystem->changeWorkingDirectoryTo(FileDirectory);
                        fillListBox();
                        setFileName("");
                    }
                    else
                    {
                        setFileName(FileList->getFullFileName(selected));
                    }

                    return true;
                }
            }
            break;

            case irr::gui::EGET_EDITBOX_CHANGED:
                if (event.GUIEvent.Caller == FileNameText)
                {
                    setFileName(FileNameText->getText());

                    return true;
                }
                break;

            case irr::gui::EGET_EDITBOX_ENTER:
                if (event.GUIEvent.Caller == FileNameText)
                {
                    irr::io::path dir(FileNameText->getText());

                    if (FileSystem->changeWorkingDirectoryTo(dir))
                    {
                        fillListBox();
                        setFileName("");
                    }
                    else
                    {
                        setFileName(FileNameText->getText());
                    }

                    return true;
                }
                break;

            default:
                break;
            }
            break;

        case irr::EET_MOUSE_INPUT_EVENT:
            switch (event.MouseInput.Event)
            {
            case irr::EMIE_MOUSE_WHEEL:
                return FileBox->OnEvent(event);

            case irr::EMIE_LMOUSE_PRESSED_DOWN:
                DragStart.X = event.MouseInput.X;
                DragStart.Y = event.MouseInput.Y;
                Dragging = true;
                return true;

            case irr::EMIE_LMOUSE_LEFT_UP:
                Dragging = false;
                return true;

            case irr::EMIE_MOUSE_MOVED:
                if (!event.MouseInput.isLeftPressed())
                    Dragging = false;

                if (Dragging)
                {
                    // gui window should not be dragged outside its parent
                    if (Parent)
                        if (event.MouseInput.X < Parent->getAbsolutePosition().UpperLeftCorner.X + 1 ||
                            event.MouseInput.Y < Parent->getAbsolutePosition().UpperLeftCorner.Y + 1 ||
                            event.MouseInput.X > Parent->getAbsolutePosition().LowerRightCorner.X - 1 ||
                            event.MouseInput.Y > Parent->getAbsolutePosition().LowerRightCorner.Y - 1)

                            return true;

                    move(irr::core::position2d<irr::s32>(event.MouseInput.X - DragStart.X,
                        event.MouseInput.Y - DragStart.Y));
                    DragStart.X = event.MouseInput.X;
                    DragStart.Y = event.MouseInput.Y;
                    return true;
                }
                break;

            default:
                break;
            }
        }
    }

    return IGUIElement::OnEvent(event);
}


//! draws the element and its children
void SaveFileDialog::draw()
{
    if (!IsVisible)
        return;

    irr::gui::IGUISkin* skin = Environment->getSkin();

    irr::core::rect<irr::s32> rect = AbsoluteRect;

    rect = skin->draw3DWindowBackground(this, true, skin->getColor(irr::gui::EGDC_ACTIVE_BORDER),
        rect, &AbsoluteClippingRect);

    if (!Text.empty())
    {
        rect.UpperLeftCorner.X += 2;
        rect.LowerRightCorner.X -= skin->getSize(irr::gui::EGDS_WINDOW_BUTTON_WIDTH) + 5;

        irr::gui::IGUIFont* font = skin->getFont(irr::gui::EGDF_WINDOW);

        if (font)
            font->draw(Text.c_str(), rect,
                skin->getColor(irr::gui::EGDC_ACTIVE_CAPTION),
                false, true, &AbsoluteClippingRect);
    }

    IGUIElement::draw();
}

void SaveFileDialog::pathToStringW(irr::core::stringw& result, const irr::io::path& p)
{
#ifndef _IRR_WCHAR_FILESYSTEM
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wstringConverter;
    result = wstringConverter.from_bytes(p.c_str()).c_str();
#else
    result = p.c_str();
#endif
}

//! fills the listbox with files.
void SaveFileDialog::fillListBox()
{
    irr::gui::IGUISkin* skin = Environment->getSkin();

    if (!FileSystem || !FileBox || !skin)
        return;

    if (FileList)
        FileList->drop();

    FileBox->clear();

    FileList = FileSystem->createFileList();
    irr::core::stringw s;

    if (FileList)
    {
        for (irr::u32 i = 0; i < FileList->getFileCount(); ++i)
        {
            pathToStringW(s, FileList->getFileName(i));
            FileBox->addItem(s.c_str(),
                skin->getIcon(FileList->isDirectory(i) ? irr::gui::EGDI_DIRECTORY : irr::gui::EGDI_FILE));
        }
    }

    if (FileNameText)
    {
        setDirectoryName(FileSystem->getWorkingDirectory());
        pathToStringW(s, FileDirectory);
        FileNameText->setText(s.c_str());
    }
}

//! sends the event that the file has been selected.
void SaveFileDialog::sendSelectedEvent(irr::gui::EGUI_EVENT_TYPE type)
{
    irr::SEvent event;
    event.EventType = irr::EET_GUI_EVENT;
    event.GUIEvent.Caller = this;
    event.GUIEvent.Element = nullptr;
    event.GUIEvent.EventType = type;
    Parent->OnEvent(event);
}


//! sends the event that the file choose process has been cancelled
void SaveFileDialog::sendCancelEvent()
{
    irr::SEvent event;
    event.EventType = irr::EET_GUI_EVENT;
    event.GUIEvent.Caller = this;
    event.GUIEvent.Element = nullptr;
    event.GUIEvent.EventType = irr::gui::EGET_FILE_CHOOSE_DIALOG_CANCELLED;
    Parent->OnEvent(event);
}
