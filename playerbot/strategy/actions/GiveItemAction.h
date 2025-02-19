#pragma once
#include "../Action.h"

namespace ai
{
    class GiveItemAction : public Action
    {
    public:
        GiveItemAction(PlayerbotAI* ai, string name, string item) : Action(ai, name), item(item) {}
        virtual bool Execute(Event& event) override;
        virtual bool isUseful() { return GetTarget() && AI_VALUE2(uint8, "mana", "self target") > sPlayerbotAIConfig.lowMana; }
        virtual Unit* GetTarget();

    protected:
        string item;
    };

    class GiveFoodAction : public GiveItemAction
    {
    public:
        GiveFoodAction(PlayerbotAI* ai) : GiveItemAction(ai, "give food", "conjured food") {}
        virtual Unit* GetTarget();
        virtual bool isUseful()
        {
            if (!GetTarget())
                return false;

            bool isRandomBot = GetTarget()->IsPlayer() && sRandomPlayerbotMgr.IsRandomBot((Player*)GetTarget());

            return !isRandomBot || (isRandomBot && !sPlayerbotAIConfig.freeFood);
        }
    };

    class GiveWaterAction : public GiveItemAction
    {
    public:
        GiveWaterAction(PlayerbotAI* ai) : GiveItemAction(ai, "give water", "conjured water") {}
        virtual Unit* GetTarget();
        virtual bool isUseful()
        {
            if (!GetTarget())
                return false;

            bool isRandomBot = GetTarget()->IsPlayer() && sRandomPlayerbotMgr.IsRandomBot((Player*)GetTarget());

            return !isRandomBot || (isRandomBot && !sPlayerbotAIConfig.freeFood);
        }
    };
}