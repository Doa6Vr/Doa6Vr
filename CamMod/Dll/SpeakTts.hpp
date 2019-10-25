#pragma once
#include <sapi.h>

namespace CamMod
{
    class SpeakTts
    {
    public:
        SpeakTts();
        ~SpeakTts();

        bool Init();
        bool Uninit();

        void Speak(LPCWSTR aText);

    private:
        ISpVoice* mTtsVoice;
    };
}