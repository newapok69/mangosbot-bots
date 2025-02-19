#pragma once
#include "../Value.h"
#include "../../PlayerbotAIConfig.h"

namespace ai
{
    class AnyGameObjectInObjectRangeCheck
    {
    public:
        AnyGameObjectInObjectRangeCheck(WorldObject const* obj, float range) : i_obj(obj), i_range(range) {}
        WorldObject const& GetFocusObject() const { return *i_obj; }
        bool operator()(GameObject* u);
    private:
        WorldObject const* i_obj;
        float i_range;
    };

    class GameObjectsInObjectRangeCheck
    {
    public:
        GameObjectsInObjectRangeCheck(WorldObject const* obj, float range, uint32 gameObjectID) : i_obj(obj), i_range(range), i_gameObjectID(gameObjectID) {}
        WorldObject const& GetFocusObject() const { return *i_obj; }
        bool operator()(GameObject* u);
    private:
        WorldObject const* i_obj;
        float i_range;
        uint32 i_gameObjectID;
    };

    class NearestGameObjects : public ObjectGuidListCalculatedValue, public Qualified
	{
	public:
        NearestGameObjects(PlayerbotAI* ai, float range = sPlayerbotAIConfig.sightDistance, bool ignoreLos = false, string name = "nearest game objects") :
            ObjectGuidListCalculatedValue(ai, name), range(range) , ignoreLos(ignoreLos) {}

    protected:
        virtual list<ObjectGuid> Calculate();

    private:
        float range;
        bool ignoreLos;
	};

    class NearestDynamicObjects : public ObjectGuidListCalculatedValue
    {
    public:
        NearestDynamicObjects(PlayerbotAI* ai, float range = sPlayerbotAIConfig.farDistance, bool ignoreLos = false, string name = "nearest dynamic objects") :
            ObjectGuidListCalculatedValue(ai, name), range(range), ignoreLos(ignoreLos) {}

    protected:
        virtual list<ObjectGuid> Calculate();

    private:
        float range;
        bool ignoreLos;
    };
}
