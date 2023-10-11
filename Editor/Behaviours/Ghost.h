#pragma once
#include "Behaviour/Behaviour.h"

class Ghost : public Behaviour
{
public:
    DEFINE_BEHAVIOUR(Ghost);

    void Update(ModuleManager& Modules, Scene* Scene, float DeltaTime) override;

    void SetTarget(Model* Target) { this->Target = Target; }
    float GhostSpeed = 4.0f;
private:

    bool Started = false;

    Model* Target = nullptr;

};
