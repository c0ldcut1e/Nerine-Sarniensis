#ifndef INPUT_BINDINGS_HPP
#define INPUT_BINDINGS_HPP

#include <string>
#include <vector>

struct GamepadBinding
{
    std::string Action;
    std::string Key;
    int AxisSign;
};

class InputBindings
{
public:
    static std::vector<GamepadBinding> GetGamepadBindings();
    static void Debug();
};

#endif
