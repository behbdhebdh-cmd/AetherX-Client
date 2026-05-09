#pragma once
#include "modules/module.h"

class ESP : public Module {
public:
    ESP();
    void onRender() override;
};
