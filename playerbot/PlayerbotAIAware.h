#pragma once
#include <string>

class PlayerbotAI;

using namespace std;

namespace ai
{
    class PlayerbotAIAware 
    {
    public:
        PlayerbotAIAware(PlayerbotAI* const ai) : ai(ai) { }
        virtual string getName() { return string(); }
    protected:
        PlayerbotAI* ai;
    };
}