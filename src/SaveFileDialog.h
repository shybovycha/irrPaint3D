#pragma once

#include <codecvt> // for converting wstring to string
#include <xlocbuf> // for converting wstring to string

#include <memory>
#include <string>

#include <irrlicht.h>

const irr::s32 SAVE_FILE_DIALOG_WIDTH = 350;
const irr::s32 SAVE_FILE_DIALOG_HEIGHT = 250;

class SaveFileDialog : public irr::gui::IGUIFileOpenDialog
{
public:
    //! constructor
    SaveFileDialog(const wchar_t* title,
        irr::gui::IGUIEnvironment* environment,
        irr::gui::IGUIElement* parent,
        irr::s32 id,
        bool restoreCWD = false,
        irr::io::path::char_type* startDir = 0);

    //! destructor
    virtual ~SaveFileDialog();

    //! returns the filename of the selected file. Returns NULL, if no file was selected.
    virtual const wchar_t* getFileName() const override;

    //! Returns the filename of the selected file. Is empty if no file was selected.
    virtual const irr::io::path& getFileNameP() const;

    //! Returns the directory of the selected file. Returns NULL, if no directory was selected.
    virtual const irr::io::path& getDirectoryName();

    //! Returns the directory of the selected file converted to wide characters. Returns NULL if no directory was selected.
    virtual const wchar_t* getDirectoryNameW() const;

    //! called if an event happened.
    virtual bool OnEvent(const irr::SEvent& event) override;

    //! draws the element and its children
    virtual void draw() override;

protected:

    void setFileName(const irr::io::path& name);

    void setDirectoryName(const irr::io::path& name);

    //! Ensure filenames are converted correct depending on wide-char settings
    void pathToStringW(irr::core::stringw& result, const irr::io::path& p);

    //! fills the listbox with files.
    void fillListBox();

    //! sends the event that the file has been selected.
    void sendSelectedEvent(irr::gui::EGUI_EVENT_TYPE type);

    //! sends the event that the file choose process has been canceled
    void sendCancelEvent();

    irr::core::position2d<irr::s32> DragStart;
    irr::io::path FileName;
    irr::core::stringw FileNameW;
    irr::io::path FileDirectory;
    irr::io::path FileDirectoryFlat;
    irr::core::stringw FileDirectoryFlatW;
    irr::io::path RestoreDirectory;
    irr::io::path StartDirectory;

    irr::gui::IGUIButton* CloseButton;
    irr::gui::IGUIButton* OKButton;
    irr::gui::IGUIButton* CancelButton;
    irr::gui::IGUIListBox* FileBox;
    irr::gui::IGUIEditBox* FileNameText;
    irr::gui::IGUIElement* EventParent;

    irr::io::IFileSystem* FileSystem;
    irr::io::IFileList* FileList;

    bool Dragging;
};
