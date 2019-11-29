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
        typedef uint8_t LockMode; enum
        {
            LOCK_MODE_NONE,
            LOCK_MODE_LOCK_SOURCE,
            LOCK_MODE_LOCK_TARGET,
            LOCK_MODE_LOCK_SOURCE_AND_TARGET,
            LOCK_MODE_LOCK_GAMEPAD_MOVE_Y,
            LOCK_MODE_LOCK_ALL,
            LOCK_MODE_LOCK_ALL_PLUS_HMD,

            LOCK_MODE_CNT
        };

        typedef uint8_t HmdLockMode; enum
        {
            HMD_LOCK_MODE_NONE,
            HMD_LOCK_MODE_Y,
            HMD_LOCK_MODE_XZ,
            HMD_LOCK_MODE_ALL,

            HMD_LOCK_MODE_CNT
        };

        typedef uint8_t FreeModes; enum
        {
            FREE_MODE_GAME_CAM,
            FREE_MODE_SRC_GAME_TARGET_NONE,
            FREE_MODE_SRC_GAME_TARGET_TABLE,
            FREE_MODE_NO_SOURCE_TARGET_GAME,
            FREE_MODE_NO_CHANGE,
            FREE_MODE_SOURCE_AND_TARGET,
            FREE_MODE_POV,
            FREE_MODE_NO_SOURCE,
            FREE_MODE_NO_SOURCE_NO_TARGET,
            FREE_MODE_CNT
        };

        typedef uint8_t BreakModes; enum
        {
            BREAK_MODE_NORMAL,
            BREAK_MODE_INFINITE,
            BREAK_MODE_EMPTY,
            BREAK_MODE_CNT
        };

        const uint8_t MAX_CPU_LEVEL = 9;

        struct UserCommands
        {
            Vector3 posDelta;
            Vector3 rotDelta;
            bool resetPos;
            bool altFunc;
            float zoom;
            uint8_t freeRoam;
            LockMode lockHeight;
            HmdLockMode hmdLockMode;
            uint8_t lockPitch;
            uint8_t fullBreak[2];
            uint8_t fullHealth[2];
            bool lockCam;
            bool hideUI;
            uint8_t hidePlayer;
            uint8_t cpu1Level;
            uint8_t cpu2Level;
        };



        UserInput();
        ~UserInput();

        bool GetInput(UserCommands& aCommands);

        bool Init();
        void Reset();

    private:
        void RotateOption(uint8_t& aOption, uint8_t aMax, LPCWSTR aOptionName = L"", bool aDecrement = false, const LPCWSTR* aStringArray = nullptr, bool aSpeakIdx = false );

        bool mSwapAxis;
        float mZoom;
        XINPUT_STATE mPrevState;
        uint8_t mFreeRoam;
        LockMode mLockHeight;
        HmdLockMode mHmdLockMode;
        uint8_t mLockPitch;
        SpeakTts mTts;
        uint8_t mFullBreaks[2];
        uint8_t mFullHealths[2];
        bool mLockCam;
        bool mHideUi;
        uint8_t mHidePlayer;
        uint8_t mCpu1Level;
        uint8_t mCpu2Level;
    };

}