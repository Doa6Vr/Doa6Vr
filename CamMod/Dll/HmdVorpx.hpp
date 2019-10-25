#pragma once
#include <Windows.h>
#include "IHmd.hpp"
#include "HmdServerIpc.hpp"

namespace CamMod
{
    class HmdVorpx : public IHmdServerIntf
    {
    public:
        HmdVorpx();
        virtual ~HmdVorpx();

        virtual bool Connect() override;
        virtual bool IsConnected() override;

        virtual bool UpdateState() override;
        virtual bool ResetVr() override;
        virtual bool ResetPos() override;

        virtual HmdServer::UpdateResponse_t GetLastUpdateState() override;

    private:
        HmdServer::UpdateResponse_t mLastUpdate;
        bool mConnected;
    };
}