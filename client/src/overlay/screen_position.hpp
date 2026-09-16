#ifndef SCREEN_POSITION_HPP
#define SCREEN_POSITION_HPP

#include <imgui.h>

class ScreenPosition
{
public:
    static ImVec2 Resolve(int position, ImVec2 boxSize, float screenWidth, float screenHeight, float margin);
};

#endif
