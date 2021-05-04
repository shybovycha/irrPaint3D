#pragma once

#include "ApplicationDelegate.h"

#include <memory>

#include <irrlicht.h>

class IrrlichtEventReceiver : public irr::IEventReceiver
{
public:
    IrrlichtEventReceiver(std::shared_ptr<ApplicationDelegate> applicationDelegate);

    bool OnEvent(const irr::SEvent& event) override;

private:
    std::shared_ptr<ApplicationDelegate> applicationDelegate;
};
