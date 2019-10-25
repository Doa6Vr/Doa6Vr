#include "UserInput.hpp"

#include <Windows.h>
#include <Xinput.h>
#include <sstream>

#pragma comment(lib, "XInput.lib")

namespace CamMod
{
    UserInput::UserInput()
        : mSwapAxis( false )
        , mChaseCam( false )
        , mZoom( 0.f )
        , mFreeRoam( false )
        , mLockHeight( false )
        , mLockPitch( false )
        , mFullBoosts()
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

    bool
    UserInput::GetInput(UserCommands& aCommands)
    {
        //TODO: Make this all configurable via a file rather than this mess

        #define CLICKED( _BUTTON ) ( 0 != ( state.Gamepad.wButtons & _BUTTON ) && 0 == ( mPrevState.Gamepad.wButtons & _BUTTON ) )
        XINPUT_STATE state = { 0 };
        DWORD success = XInputGetState(0, &state);
        if (ERROR_SUCCESS != success )
        {
            return false;
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


        if (leftTrigger > 0.f)
        {
            if CLICKED(XINPUT_GAMEPAD_A)
            {
                mFullBoosts[0] = (mFullBoosts[0] + 1) % 3;

                std::wstringstream stream;
                stream << "P 1 Boost " << (mFullBoosts[0]);
                mTts.Speak(stream.str().c_str());
            }
            else if CLICKED(XINPUT_GAMEPAD_B)
            {
                mFullBoosts[1] = (mFullBoosts[1] + 1) % 3;

                std::wstringstream stream;
                stream << "P 2 Boost " << (mFullBoosts[1]);
                mTts.Speak(stream.str().c_str());
            }
            else if CLICKED(XINPUT_GAMEPAD_X)
            {
                mFullHealths[0] = (mFullHealths[0] + 1) % 2;

                std::wstringstream stream;
                stream << "P 1 Health " << (mFullHealths[0]);
                mTts.Speak(stream.str().c_str());
            }
            else if CLICKED(XINPUT_GAMEPAD_Y)
            {
                mFullHealths[1] = (mFullHealths[1] + 1) % 2;

                std::wstringstream stream;
                stream << "P 2 Health " << (mFullHealths[1]);
                mTts.Speak(stream.str().c_str());
            }
        }

        aCommands.hidePlayer = 0;
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
            mHidePlayer = (mHidePlayer + 1) % 5;
            aCommands.hidePlayer = mHidePlayer + 1;
        }

        if (CLICKED(XINPUT_GAMEPAD_LEFT_THUMB))
        {
            if (lbHeld && rbHeld)
            {
                mLockPitch = !mLockPitch;
                std::wstringstream stream;
                stream << "Pitch Lock " << (mLockPitch ? "On" : "Off");
                mTts.Speak(stream.str().c_str());
            }
            else if (rbHeld)
            {
                mLockHeight = !mLockHeight;
                std::wstringstream stream;
                stream << "Height Lock " << (mLockHeight ? "On" : "Off");
                mTts.Speak(stream.str().c_str());
            }
        }
        if (CLICKED(XINPUT_GAMEPAD_RIGHT_THUMB))
        {
            if (lbHeld && rbHeld)
            {
                mChaseCam = !mChaseCam;
            }
            else if (lbHeld)
            {
                mSwapAxis = !mSwapAxis;
            }
            else if (rbHeld)
            {
                mFreeRoam = (mFreeRoam + 1) % 4;


                std::wstringstream stream;
                stream << "Free roam is " << mFreeRoam;
                mTts.Speak(stream.str().c_str());
            }
            else
            {
                mSwapAxis = !mSwapAxis;
            }
        }
        aCommands.chaseCam = mChaseCam;
        aCommands.freeRoam = mFreeRoam;
        aCommands.lockHeight = mLockHeight;
        aCommands.lockPitch = mLockPitch;
        memcpy(&aCommands.fullBoost, mFullBoosts, sizeof(mFullBoosts));
        memcpy(&aCommands.fullHealth, mFullHealths, sizeof(mFullHealths));
        aCommands.lockCam = mLockCam;
        aCommands.hideUI = mHideUi;

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
            aCommands.posDelta.X = -posMult*lX;

            if (lbHeld && mSwapAxis )
            {
                aCommands.posDelta.Y = -posMult*lY;
            }
            else
            {
                aCommands.posDelta.Z = posMult*lY;
            }

        }

        memcpy(&mPrevState, &state, sizeof(state));
        return true;
    }

}