#include "SpeakTts.hpp"


namespace CamMod
{
    SpeakTts::SpeakTts()
        : mTtsVoice(nullptr)
    {

    }


    SpeakTts::~SpeakTts()
    {
        Uninit();
    }

    bool
    SpeakTts::Init()
    {
        if (mTtsVoice)
        {
            return true;
        }

        if (FAILED(::CoInitialize(NULL)))
        {
            return false;
        }

        HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&mTtsVoice);
        if (FAILED(hr))
        {
            ::CoUninitialize();
            return false;
        }
        return true;
    }


    bool
    SpeakTts::Uninit()
    {
        if (nullptr == mTtsVoice)
        {
            return false;
        }

        mTtsVoice->Release();
        mTtsVoice = nullptr;
        ::CoUninitialize();
        return true;
    }


    void
    SpeakTts::Speak(LPCWSTR aText)
    {
        if (nullptr == mTtsVoice)
        {
            return;
        }
        mTtsVoice->Speak(aText, SVSFlagsAsync | SVSFPurgeBeforeSpeak, nullptr);
    }

}