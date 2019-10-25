#include "HmdServerIntf.hpp"

//TODO: Most of this should be a shared IPC class with HmdServer

#define PIPE_BUF_SZ 1024

namespace
{
    LPTSTR PIPE_NAME = TEXT("\\\\.\\pipe\\HmdServer");
}

namespace CamMod
{

    HmdServerIntf::HmdServerIntf()
        : mPipeHndl( INVALID_HANDLE_VALUE )
        , mLastUpdate( {} )
    {

    }


    HmdServerIntf::~HmdServerIntf()
    {
        if (IsConnected())
        {
            CloseHandle( mPipeHndl );
        }
    }


    bool
    HmdServerIntf::Connect()
    {
        // Blocks until connected
        mPipeHndl = CreateFile(
            PIPE_NAME,
            GENERIC_READ |  // read and write access 
            GENERIC_WRITE,
            0,              // no sharing 
            NULL,           // default security attributes
            OPEN_EXISTING,  // opens existing pipe 
            0,              // default attributes 
            NULL);          // no template file


        return ( INVALID_HANDLE_VALUE != mPipeHndl );
    }

    bool
    HmdServerIntf::IsConnected()
    {
        return (INVALID_HANDLE_VALUE != mPipeHndl);
    }


    bool
    HmdServerIntf::UpdateState()
    {
        bool ok = SendRequest( HmdServer::REQUEST_UPDATE );

        HmdServer::UpdateResponse_t response;
        ok = ok && ReceiveResponse( HmdServer::RESPONSE_UPDATE, &response, sizeof(response) );
        if( ok )
        {
            mLastUpdate = response;
        }

        return ok;
    }

    bool 
    HmdServerIntf::ResetVr()
    {
        bool ok = SendRequest(HmdServer::REQUEST_RESET_VR);

        ok = ok && ReceiveResponse(HmdServer::RESPONSE_ACK, nullptr, 0);
        return ok;
    }

    bool 
    HmdServerIntf::ResetPos()
    {
        bool ok = SendRequest(HmdServer::REQUEST_RESET_POS);

        ok = ok && ReceiveResponse(HmdServer::RESPONSE_ACK, nullptr, 0);
        return ok;
    }

    HmdServer::UpdateResponse_t
    HmdServerIntf::GetLastUpdateState()
    {
        return mLastUpdate;
    }

    bool
    HmdServerIntf::SendRequest(HmdServer::IpcRequestType aRequest)
    {
        DWORD bytesWritten;
        BOOL success = WriteFile(
            mPipeHndl,
            &aRequest,
            sizeof(aRequest),
            &bytesWritten,
            NULL);

        return (success == TRUE && bytesWritten == sizeof(aRequest) );
    }


    bool
    HmdServerIntf::ReceiveResponse(HmdServer::IpcResponseType aType, void* aBuffer, uint32_t aBufferSz)
    {
        uint8_t readBuf[PIPE_BUF_SZ];
        DWORD bytesRead;
        BOOL success = ReadFile(mPipeHndl, readBuf, sizeof(readBuf), &bytesRead, NULL);

        // Check the response is expected type
        if (success && bytesRead == aBufferSz + sizeof(aType) )
        {
            HmdServer::IpcResponseType responseType;
            memcpy( &responseType, readBuf, sizeof(responseType) );

            if( responseType != aType )
            {
                OutputDebugString(TEXT("Unexpected response type!"));
                return false;
            }
            else
            {
                memcpy( aBuffer, readBuf + sizeof(aType), aBufferSz );
            }
        }
        else
        {
            if (GetLastError() == ERROR_BROKEN_PIPE)
            {
                OutputDebugString(TEXT("Pipe is broken!\n"));
            }
            else if (!success)
            {
                OutputDebugString(TEXT("ReadFile failed"));
            }
            else
            {
                // Currently we expect every request to wait for a response, so this should be valid
                OutputDebugString(TEXT("Unexpected response size"));
            }
            return false;
        }
        return true;
    }
}
