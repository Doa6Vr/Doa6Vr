#include "UserInput.hpp"

#include <Windows.h>
#include <Xinput.h>
#include <sstream>

#pragma comment(lib, "XInput.lib")


namespace
{
    const LPCWSTR sLockModeStrings[] =
    {
        L"NONE",
        L"SOURCE",
        L"TARGET",
        L"SOURCE AND TARGET",
        L"GAMEPAD MOVE Y",
        L"ALL",
        L"ALL PLUS HMD",
    };

    const LPCWSTR sHmdLockModeStrings[] =
    {
        L"NONE",
        L"Y",
        L"X Z",
        L"ALL",
    };

    const LPCWSTR sFreeRoamStrings[] =
    {
        L"Game Cam",                 // FREE_MODE_GAME_CAM,
        L"Game no target",           // FREE_MODE_SRC_GAME_TARGET_NONE,
        L"Game with target",         // FREE_MODE_SRC_GAME_TARGET_TABLE,
        L"No source, game target",   // FREE_MODE_NO_SOURCE_TARGET_GAME,
        L"Cheat Engine Values",      // FREE_MODE_NO_CHANGE,
        L"Source and Target On",     // FREE_MODE_SOURCE_AND_TARGET,
        L"POV",                      // FREE_MODE_POV,
        L"Source Off Target On",     // FREE_MODE_NO_SOURCE,
        L"Source Locked Target On",  // FREE_MODE_SOURCE_LOCKED,
        L"Source and Target Off",    // FREE_MODE_NO_SOURCE_NO_TARGET,
                                     // FREE_MODE_CNT
        };

    const LPCWSTR sBreakModeStrings[] =
    {
        L"Normal",
        L"Infinite",
        L"Empty",
    };
}

namespace CamMod
{
    UserInput::UserInput()
        : mSwapAxis( false )
        , mZoom( 0.f )
        , mFreeRoam( 0 )
        , mLockHeight(LOCK_MODE_NONE )
        , mHmdLockMode(HMD_LOCK_MODE_NONE )
        , mLockPitch( 0 )
        , mFullBreaks()
        , mFullHealths()
        , mHideUi()
        , mHidePlayer(0)
    {

    }


    UserInput::~UserInput()
    {
        mTts.Uninit();
    }

    bool
    UserInput::Init()
    {
        bool ok = mTts.Init();
        return ok;
    }

    void
    UserInput::Reset()
    {
        mSwapAxis = false;
        mZoom = 0.f;
        mFreeRoam = 0;
        mLockHeight = 0;
        mLockPitch = 0;
        mHidePlayer = 0;
    }

    bool
    UserInput::GetInput(UserCommands& aCommands)
    {
        //TODO: Make this all configurable via a file rather than this mess

        XINPUT_STATE state = { 0 };
        #define CLICKED( _BUTTON ) ( 0 != ( state.Gamepad.wButtons & _BUTTON ) && 0 == ( mPrevState.Gamepad.wButtons & _BUTTON ) )

        // Check P2 controller. If Left Trigger held then use P2
        DWORD success = XInputGetState(1, &state);
        if (ERROR_SUCCESS == success )
        {
            float leftTriggerP2 = fmaxf(-1, (float)state.Gamepad.bLeftTrigger / 255.f);
            if (leftTriggerP2 == 0.f)
            {
                success = -1;
            }
        }
        // Else use P1
        if( ERROR_SUCCESS != success)
        {
            success = XInputGetState(0, &state);
        }
        const bool lbHeld = (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
        const bool rbHeld = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
        const bool rStickClicked = (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
        const float rightTrigger = fmaxf(-1, (float)state.Gamepad.bRightTrigger / 255.f);
        const float leftTrigger = fmaxf(-1, (float)state.Gamepad.bLeftTrigger / 255.f);
        const float lX = fmaxf(-1, (float)state.Gamepad.sThumbLX / 32767.f);
        const float lY = fmaxf(-1, (float)state.Gamepad.sThumbLY / 32767.f);
        const float rX = fmaxf(-1, (float)state.Gamepad.sThumbRX / 32767.f);
        const float rY = fmaxf(-1, (float)state.Gamepad.sThumbRY / 32767.f);

        if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0)
        {
            aCommands.altFunc = true;
        }
        else
        {
            aCommands.altFunc = false;
        }


        if (leftTrigger > 0.f)
        {
            const int8_t offset = (aCommands.altFunc ? -1 : 1);
            if (rbHeld == false)
            {
                if CLICKED(XINPUT_GAMEPAD_A)
                {
                    RotateOption(mFullBreaks[0], BREAK_MODE_CNT, L"P 1 Break", aCommands.altFunc, sBreakModeStrings);
                }
                else if CLICKED(XINPUT_GAMEPAD_B)
                {
                    RotateOption(mFullBreaks[1], BREAK_MODE_CNT, L"P 2 Break", aCommands.altFunc, sBreakModeStrings);
                }
                else if CLICKED(XINPUT_GAMEPAD_X)
                {
                    RotateOption(mFullHealths[0], 2, L"P 1 Health", aCommands.altFunc);
                }
                else if CLICKED(XINPUT_GAMEPAD_Y)
                {
                    RotateOption(mFullHealths[1], 2, L"P 2 Health", aCommands.altFunc);
                }
            }
            else
            {
                // used by Lua script for speed control on controller 2
            }
        }

        if (rightTrigger > 0.f)
        {
            if (CLICKED(XINPUT_GAMEPAD_X))
            {
                RotateOption(mHmdLockMode, HMD_LOCK_MODE_CNT, L"Headset Lock", aCommands.altFunc, sHmdLockModeStrings);
            }
            else if (CLICKED(XINPUT_GAMEPAD_B))
            {
                if (aCommands.altFunc)
                {
                    RotateOption(mCpu2Level, MAX_CPU_LEVEL, L"P 2 Level");
                }
                else
                {
                    RotateOption(mCpu1Level, MAX_CPU_LEVEL, L"P 1 Level");
                }
            }
        }

        if( lbHeld && rbHeld && CLICKED(XINPUT_GAMEPAD_BACK) )
        {
            mLockCam = !mLockCam;
            std::wstringstream stream;
            stream << "Cam lock " << (mLockCam ? "On" : "Off");
            mTts.Speak(stream.str().c_str());
        }
        else if (lbHeld && !rbHeld && CLICKED(XINPUT_GAMEPAD_BACK))
        {
            mHideUi = !mHideUi;
        }
        else if (rbHeld && CLICKED(XINPUT_GAMEPAD_BACK))
        {
            RotateOption(mHidePlayer, 6, L"", aCommands.altFunc);
        }

        if (CLICKED(XINPUT_GAMEPAD_LEFT_THUMB))
        {
            if (lbHeld && rbHeld)
            {
                RotateOption(mLockPitch, 8, L"Lock Pitch", aCommands.altFunc);
            }
            else if (rbHeld)
            {
                RotateOption(mLockHeight, LOCK_MODE_CNT, L"Height Lock", aCommands.altFunc, sLockModeStrings, true);
            }
        }
        if (CLICKED(XINPUT_GAMEPAD_RIGHT_THUMB))
        {
            if (lbHeld && rbHeld)
            {
                Reset();
            }
            else if (lbHeld)
            {
                // used by lua speed control
            }
            else if (rbHeld)
            {
                RotateOption(mFreeRoam, FREE_MODE_CNT, L"Free Roam", aCommands.altFunc, sFreeRoamStrings, true);
            }
            else
            {
                mSwapAxis = !mSwapAxis;
            }
        }
        aCommands.freeRoam = mFreeRoam;
        aCommands.lockHeight = mLockHeight;
        aCommands.lockPitch = mLockPitch;
        aCommands.hmdLockMode = mHmdLockMode;
        memcpy(&aCommands.fullBreak, mFullBreaks, sizeof(mFullBreaks));
        memcpy(&aCommands.fullHealth, mFullHealths, sizeof(mFullHealths));
        aCommands.lockCam = mLockCam;
        aCommands.hideUI = mHideUi;
        aCommands.hidePlayer = mHidePlayer;
        aCommands.cpu1Level = mCpu1Level;
        aCommands.cpu2Level = mCpu2Level;

        if (0 != (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
            && 0 != (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB))
        {
            aCommands.resetPos = true;
        }


        float yawMult = (2.f * M_PI_F)*.001f;
        float pitchMult = yawMult;
        float rollMult = yawMult;
        float posMult = 1.0f;
        float zoomMult = -0.01f;
        float rDeadzoneMult = 0.7f;
        float lDeadzoneMult = 0.7f;

        if ( abs( state.Gamepad.bRightTrigger ) > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
        {
            float trigMult = 1.f + 10.f*(rightTrigger);
            yawMult *= trigMult;
            pitchMult *= trigMult;
            rollMult *= trigMult;
            posMult *= trigMult;
            zoomMult *= trigMult;
        }

        if ( !rStickClicked && ( abs( state.Gamepad.sThumbRX ) >= rDeadzoneMult*XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE  || abs( state.Gamepad.sThumbRY ) >= rDeadzoneMult*XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ) )
        {
            // Right stick is Up/Yaw,Up/Roll, unless rbHeld then it is zoom
            if (rbHeld)
            {
                if(abs(rY) > 2 * abs(rX))
                {
                    aCommands.zoom = zoomMult * rY;
                }
                else if (abs(rX) > 2 * abs(rY) )
                {
                    aCommands.rotDelta.roll = rollMult*rX;
                }
            }
            else 
            {
                bool swapAxis = mSwapAxis;
                if (rbHeld)
                {
                    swapAxis = !swapAxis;
                }
                aCommands.rotDelta.yaw = yawMult*rX;
                if (swapAxis)
                {
                    aCommands.posDelta.Y = -posMult*rY;
                }
                else
                {
                    aCommands.rotDelta.pitch = pitchMult*rY;
                }
            }
        }


        if (abs(state.Gamepad.sThumbLX) >= lDeadzoneMult*XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE || abs(state.Gamepad.sThumbLY) >= lDeadzoneMult*XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
        {
            aCommands.posDelta.X = posMult*lX;

            if (lbHeld && mSwapAxis )
            {
                aCommands.posDelta.Y = -posMult*lY;
            }
            else
            {
                aCommands.posDelta.Z = -posMult*lY;
            }

        }

        memcpy(&mPrevState, &state, sizeof(state));
        return true;
    }


    /* private */ void
    UserInput::RotateOption(uint8_t& aOption, uint8_t aMax, LPCWSTR aOptionName, bool aDecrement, const LPCWSTR* aStringArray, bool aSpeakIdx )
    {
        if (aDecrement)
        {
            if (aOption == 0)
            {
                aOption = aMax - 1;
            }
            else
            {
                aOption--;
            }

        }
        else
        {
            aOption = (aOption + 1) % aMax;
        }

        if (aOptionName[0] != L'\0')
        {
            std::wstringstream stream;
            stream << aOptionName << " is ";
            if (aStringArray)
            {
                if (aSpeakIdx)
                {
                    stream << aOption << ", ";
                }
                stream << aStringArray[aOption];
            }
            else
            {
                stream << aOption;
            }
            mTts.Speak(stream.str().c_str());
        }
    }
}