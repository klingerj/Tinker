namespace Tk
{
namespace Platform
{

namespace Keycode
{
    enum : uint32
    {
        eA = 0,
        eB,
        eC,
        eD,
        eE,
        eF,
        eG,
        eH,
        eI,
        eJ,
        eK,
        eL,
        eM,
        eN,
        eO,
        eP,
        eQ,
        eR,
        eS,
        eT,
        eU,
        eV,
        eW,
        eX,
        eY,
        eZ,
        e0,
        e1,
        e2,
        e3,
        e4,
        e5,
        e6,
        e7,
        e8,
        e9,
        eF1,
        eF2,
        eF3,
        eF4,
        eF5,
        eF6,
        eF7,
        eF8,
        eF9,
        eF10,
        eF11,
        eF12,
        eMax
    };
}

typedef struct keycode_state
{
    uint8 isDown; // is the key currently down
    uint8 numStateChanges; // number of times the key went up/down
} KeycodeState;

typedef struct input_state_delta
{
    // TODO: gamepad input

    // Keyboard input
    KeycodeState keyCodes[Keycode::eMax];
} InputStateDeltas;

}
}
