namespace Tinker
{
    namespace Platform
    {
        enum
        {
            eKeyA = 0,
            eKeyB,
            eKeyC,
            eKeyD,
            eKeyE,
            eKeyF,
            eKeyG,
            eKeyH,
            eKeyI,
            eKeyJ,
            eKeyK,
            eKeyL,
            eKeyM,
            eKeyN,
            eKeyO,
            eKeyP,
            eKeyQ,
            eKeyR,
            eKeyS,
            eKeyT,
            eKeyU,
            eKeyV,
            eKeyW,
            eKeyX,
            eKeyY,
            eKeyZ,
            eKey0,
            eKey1,
            eKey2,
            eKey3,
            eKey4,
            eKey5,
            eKey6,
            eKey7,
            eKey8,
            eKey9,
            eKeyF1,
            eKeyF2,
            eKeyF3,
            eKeyF4,
            eKeyF5,
            eKeyF6,
            eKeyF7,
            eKeyF8,
            eKeyF9,
            eKeyF10,
            eKeyF11,
            eKeyF12,
            eMaxKeycodes
        };

        typedef struct keycode_state
        {
            uint8 isDown; // is the key currently down
            uint8 numStateChanges; // number of times the key went up/down
        } KeycodeState;

        typedef struct input_state_delta
        {
            // TODO: gamepad input

            // Keyboard input
            KeycodeState keyCodes[eMaxKeycodes];
        } InputStateDeltas;
    }
}
