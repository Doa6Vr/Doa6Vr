#pragma once
#include "HmdServerIpc.hpp"

namespace CamMod
{
    class IHmdServerIntf
    {
    public:
        virtual ~IHmdServerIntf() = default;

        virtual bool Connect() = 0;
        virtual bool IsConnected() = 0;

        virtual bool UpdateState() = 0;
        virtual bool ResetVr() = 0;
        virtual bool ResetPos() = 0;

        virtual HmdServer::UpdateResponse_t GetLastUpdateState() = 0;
    };
}