#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdint.h>
#include "HmdServerIpc.hpp"
#include "Utils.hpp"
#include "OpenVrHmd.hpp"

#define PIPE_BUF_SZ 1024
#define ROT_STEPS 300
DWORD WINAPI PipeThreadFunc(LPVOID);

#define M_PI_F (float(M_PI))
#define PITCH_MARGIN ( M_PI_F/8 )

HmdServer::OpenVrHmd gVrHmd;
Vector3 gHmdPosOffset;
Vector3 gPosTargetOffset;
Vector3 gHmdRotEuler;
Quaternion gHmdRotOffset;

HmdServer::UpdateResponse gLastUpdate = {};

// The client shoudl really be a class
bool ReceiveRequest(HANDLE aPipeHndl, HmdServer::IpcRequestType* aRequest);
void HandleRequest( HANDLE aPipeHndl, HmdServer::IpcRequestType aRequest );
bool SendResponse(HANDLE aPipeHndl, HmdServer::IpcResponseType aType, void* aBuffer, uint32_t aBufferSz);
void ResetPos();
void PerformUpdate();
void CheckKeyboard();
void CheckHeadset();


int main(int* argc, int* argv)
{
    LPTSTR pipeName = TEXT("\\\\.\\pipe\\HmdServer");
    HANDLE pipeHndl = INVALID_HANDLE_VALUE;
    bool pipeConnected = false;

    _tprintf(TEXT("\nStarting HMD Server\n"));

    if(!gVrHmd.Init() )
    {
        _tprintf(TEXT("Failed to initialize OpenVR!\n") );
    }

    for (;;)
    {
        _tprintf(TEXT("Waiting for client connection on %s\n"), pipeName);

        pipeHndl = CreateNamedPipe(
            pipeName,                 // pipe name 
            PIPE_ACCESS_DUPLEX,       // read/write access 
            PIPE_TYPE_MESSAGE |       // message type pipe 
            PIPE_READMODE_MESSAGE |   // message-read mode 
            PIPE_WAIT,                // blocking mode 
            PIPE_UNLIMITED_INSTANCES, // max. instances  
            PIPE_BUF_SZ,              // output buffer size 
            PIPE_BUF_SZ,              // input buffer size 
            0,                        // client time-out 
            NULL);

        if (pipeHndl == INVALID_HANDLE_VALUE)
        {
            _tprintf(TEXT("CreateNamedPipe failed: %d\n"), GetLastError());
            return -1;
        }

        pipeConnected = ConnectNamedPipe(pipeHndl, NULL) ? true : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (pipeConnected)
        {
            _tprintf(TEXT("Client connected!\n"));
            DWORD threadId;
            // Create a thread for this client. 
            HANDLE threadHndl = CreateThread(
                NULL,              // no security attribute 
                0,                 // default stack size 
                PipeThreadFunc,    // thread proc
                (LPVOID)pipeHndl,  // thread parameter 
                0,                 // not suspended 
                &threadId);        // returns thread ID 

            if (threadHndl == NULL)
            {
                _tprintf(TEXT("CreateThread failed: %d\n"), GetLastError());
                return -1;
            }
            else
            {
                CloseHandle(threadHndl);
            }
            
        }
    }
    return 0;
}


DWORD WINAPI PipeThreadFunc(LPVOID lpvParam)
{
    _tprintf(TEXT("Pipe handler thread!\n"));

    HANDLE pipeHndl = (HANDLE)lpvParam;

    for (;;)
    {
        // Wait for command
        HmdServer::IpcRequestType request;
        bool success = ReceiveRequest( pipeHndl, &request );
        if( !success )
        {
            break;
        }
        HandleRequest( pipeHndl, request );

    }

    _tprintf(TEXT("Shutting down pipe...\n"));
    FlushFileBuffers(pipeHndl);
    DisconnectNamedPipe(pipeHndl);
    CloseHandle(pipeHndl);
    _tprintf(TEXT("Pipe handler thread is exiting\n"));
    return 0;
}



bool
SendResponse(HANDLE aPipeHndl, HmdServer::IpcResponseType aType, void* aBuffer, uint32_t aBufferSz)
{
    uint8_t sendBuffer[PIPE_BUF_SZ];
    if( sizeof(sendBuffer) <= aBufferSz + sizeof(aType) )
    {
        return false;
    }

    memcpy( sendBuffer, &aType, sizeof(aType) );
    memcpy( sendBuffer + sizeof(aType), aBuffer, aBufferSz );
    DWORD bytesToSend = sizeof(aType) + aBufferSz;

    DWORD bytesWritten;
    BOOL success = WriteFile(
        aPipeHndl,
        &sendBuffer,
        bytesToSend,
        &bytesWritten,
        NULL);

    return (success == TRUE && bytesWritten == bytesToSend );
}


bool
ReceiveRequest(HANDLE aPipeHndl, HmdServer::IpcRequestType* aRequest)
{
    uint8_t readBuf[PIPE_BUF_SZ];
    DWORD bytesRead;
    BOOL success = ReadFile(aPipeHndl, readBuf, sizeof(readBuf), &bytesRead, NULL);

    // Currently requests can only be a byte
    if( success && sizeof(*aRequest) == bytesRead )
    {
        memcpy( aRequest, readBuf, sizeof(*aRequest) );
    }
    else
    {
        if (GetLastError() == ERROR_BROKEN_PIPE)
        {
            _tprintf(TEXT("Pipe is broken!\n"));
        }
        else if(!success)
        {
            _tprintf(TEXT("ReadFile failed: %d\n"), GetLastError());
        }
        else
        {
            // Currently we expect every request to wait for a response, so this should be valid
            _tprintf(TEXT("Expected %d bytes, received %d bytes"), (int)sizeof(*aRequest), bytesRead );
        }
        return false;
    }
    return true;
}


vr::HmdVector4_t SubVects( vr::HmdVector4_t& a, vr::HmdVector4_t& b )
{
    vr::HmdVector4_t ret;
    for( uint32_t i = 0; i < sizeof(ret.v)/sizeof(ret.v[0]); ++i )
    {
        ret.v[i] = a.v[i] - b.v[i];
    }
    return ret;
}


void
HandleRequest(HANDLE aPipeHndl, HmdServer::IpcRequestType aRequest)
{
    switch( aRequest )
    {
    case HmdServer::REQUEST_UPDATE:
        {
        //_tprintf(TEXT("Received REQUEST_UPDATE\n"));
        PerformUpdate();
        HmdServer::UpdateResponse updateResponse = {};
        updateResponse.headsetPos = gHmdPosOffset;
        updateResponse.headsetRot = gHmdRotOffset;
        updateResponse.headsetPosDelta = updateResponse.headsetPos - gLastUpdate.headsetPos;
        updateResponse.headsetRotEuler = gHmdRotEuler;
        updateResponse.headsetRotEulerDelta = updateResponse.headsetRotEuler - gLastUpdate.headsetRotEuler;

        //_tprintf(TEXT("Headset Pos: %f, %f, %f    Delta: %f, %f, %f\n"), updateResponse.headsetPos.X, updateResponse.headsetPos.Y, updateResponse.headsetPos.Z,
        //        updateResponse.headsetPosDelta.X, updateResponse.headsetPosDelta.Y updateResponse.headsetPosDelta.Z);

        //_tprintf(TEXT("Headset Rot: %f, %f, %f    Quat: %lf, %lf, %lf, %lf\n"), updateResponse.headsetRot.v[0], updateResponse.headsetRot.v[1], updateResponse.headsetRot.v[2],
        //    gQuat.w, gQuat.x, gQuat.y, gQuat.z);
            //updateResponse.headsetRotDelta.v[0], updateResponse.headsetRotDelta.v[1], updateResponse.headsetRotDelta.v[2]);
        //_tprintf(TEXT("-----\n"));
        SendResponse( aPipeHndl, HmdServer::RESPONSE_UPDATE, &updateResponse, sizeof(updateResponse) );

        gLastUpdate = updateResponse;
        break;
        }

    case HmdServer::REQUEST_RESET_POS:
        {
        _tprintf(TEXT("Received REQUEST_RESET_POS\n"));
        ResetPos();
        SendResponse( aPipeHndl, HmdServer::RESPONSE_ACK, nullptr, 0 );
        break;
        }

    case HmdServer::REQUEST_RESET_VR:
    {
        _tprintf(TEXT("Received REQUEST_RESET_VR\n"));
        gVrHmd.ResetVrPosition();
        SendResponse(aPipeHndl, HmdServer::RESPONSE_ACK, nullptr, 0);
        break;
    }
    }
}


void ResetPos()
{
    gVrHmd.ResetVrPosition();
    gHmdPosOffset = Vector3::Zero();
    gPosTargetOffset = Vector3::Zero();
    gHmdRotOffset = Quaternion::Identity();
    gHmdRotEuler = Vector3::Zero();
}

#define KEY_PRESSED( _vkey ) ( GetKeyState( _vkey ) & 0x8000 )
void
PerformUpdate()
{
    CheckKeyboard();
    CheckHeadset();
}

void CheckHeadset()
{
    if (gVrHmd.GetHeadsetPosition(&gHmdPosOffset, &gHmdRotEuler, &gHmdRotOffset ))
    {

    }
}


void CheckKeyboard()
{
    if (KEY_PRESSED(VK_SPACE))
    {
        _tprintf("Reset VR Headset position\n");
        gVrHmd.ResetVrPosition();
        ResetPos();
    }
}
