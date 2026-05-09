#pragma once
#include "pch.h"
#include "modules/module.h"

class Jesus : public Module {
public:
    Jesus();
    void onUpdate(JNIEnv* env) override;
};
