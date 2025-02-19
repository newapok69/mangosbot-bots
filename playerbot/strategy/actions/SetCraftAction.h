#pragma once
#include "../values/CraftValue.h"
#include "GenericActions.h"

namespace ai
{
    class SetCraftAction : public ChatCommandAction
    {
    public:
        SetCraftAction(PlayerbotAI* ai) : ChatCommandAction(ai, "craft") {}
        virtual bool Execute(Event& event) override;
        static uint32 GetCraftFee(CraftData&);

    private:
        void TellCraft();

    private:
        static map<uint32, SkillLineAbilityEntry const*> skillSpells;
    };
}
