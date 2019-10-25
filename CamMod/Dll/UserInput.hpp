#pragma once
#include "Utils.hpp"
#include <Windows.h>
#include <Xinput.h>
#include "SpeakTts.hpp"

namespace CamMod
{
    class UserInput
    {
    public:
        struct UserCommands
        {
            Vector3 posDelta;
            Vector3 rotDelta;
            bool resetPos;
            bool chaseCam;
            bool altFunc;
            float zoom;
            uint8_t freeRoam;
            bool lockHeight;
            bool lockPitch;
            uint8_t fullBoost[2];
            uint8_t fullHealth[2];
            bool lockCam;
            bool hideUI;
            uint8_t hidePlayer;
        };



        UserInput();
        ~UserInput();

        bool GetInput(UserCommands& aCommands);

        bool Init();

    private:
        bool mSwapAxis;
        bool mChaseCam;
        float mZoom;
        XINPUT_STATE mPrevState;
        uint8_t mFreeRoam;
        bool mLockHeight;
        bool mLockPitch;
        SpeakTts mTts;
        uint8_t mFullBoosts[2];
        uint8_t mFullHealths[2];
        bool mLockCam;
        bool mHideUi;
        uint8_t mHidePlayer;
    };

}