#pragma once
#include "openvr.h"
#include <stdint.h>
#include "Utils.hpp"

namespace HmdServer
{

    // Request types
    typedef uint8_t IpcRequestType; enum
    {
        REQUEST_UPDATE,
        REQUEST_RESET_POS,
        REQUEST_RESET_VR
    };

    // Response types
    typedef uint8_t IpcResponseType; enum
    {
        RESPONSE_UPDATE,
        RESPONSE_ACK,
    };

    typedef struct UpdateResponse_t
    {
        Vector3 headsetPos;
        Vector3 headsetPosDelta;
        Quaternion headsetRot;
        Vector3 headsetRotEuler;
        Vector3 headsetRotEulerDelta;
    } UpdateResponse;

}