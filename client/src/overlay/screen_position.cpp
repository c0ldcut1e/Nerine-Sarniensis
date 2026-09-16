#include "screen_position.hpp"

ImVec2 ScreenPosition::Resolve(int position, ImVec2 boxSize, float screenWidth, float screenHeight, float margin)
{
    float left    = margin;
    float right   = screenWidth - boxSize.x - margin;
    float top     = margin;
    float bottom  = screenHeight - boxSize.y - margin;
    float centerX = (screenWidth - boxSize.x) / 2.0f;
    float centerY = (screenHeight - boxSize.y) / 2.0f;

    switch (position)
    {
        case 1:
            return ImVec2(centerX, top);
        case 2:
            return ImVec2(right, top);
        case 3:
            return ImVec2(left, centerY);
        case 4:
            return ImVec2(right, centerY);
        case 5:
            return ImVec2(left, bottom);
        case 6:
            return ImVec2(centerX, bottom);
        case 7:
            return ImVec2(right, bottom);
        default:
            return ImVec2(left, top);
    }
}
