#pragma once
#include <Windows.h>
#include "IHmd.hpp"
#include "HmdServerIpc.hpp"

namespace CamMod
{
    class HmdServerIntf : public IHmdServerIntf
    {
    public:
        HmdServerIntf();
        virtual ~HmdServerIntf();

        virtual bool Connect() override;
        virtual bool IsConnected() override;

        virtual bool UpdateState() override;
        virtual bool ResetVr() override;
        virtual bool ResetPos() override;

        virtual HmdServer::UpdateResponse_t GetLastUpdateState() override;

    private:
        HANDLE mPipeHndl;
        HmdServer::UpdateResponse_t mLastUpdate;

        bool SendRequest( HmdServer::IpcRequestType aRequest );

        bool ReceiveResponse( HmdServer::IpcResponseType aType, void* aBuffer, uint32_t aBufferSz );


    };
}