#include "HmdVorpx.hpp"
#include "Vorpx/vorpAPI.h"
#include "Utils.hpp"

namespace
{
    void Vect3FromVorpx(const vpxfloat3& aVorpxVect, Vector3& aFloatOut )
    {
        aFloatOut.X = aVorpxVect.x;
        aFloatOut.Y = aVorpxVect.y;
        aFloatOut.Z = aVorpxVect.z;
    }

    void QuatFromVorpx(const vpxfloat4& aVorpxQuat, Quaternion& aQuatOut)
    {
        aQuatOut.W = aVorpxQuat.w;
        aQuatOut.X = aVorpxQuat.x;
        aQuatOut.Y = aVorpxQuat.y;
        aQuatOut.Z = aVorpxQuat.z;
    }

    void Vect3ToRadians(Vector3& aVect3)
    {
        aVect3.X = to_radians(aVect3.X);
        aVect3.Y = to_radians(aVect3.Y);
        aVect3.Z = to_radians(aVect3.Z);
    }
}


namespace CamMod
{

    HmdVorpx::HmdVorpx()
        : mLastUpdate()
        , mConnected(false)
    {

    }


    HmdVorpx::~HmdVorpx()
    {
        if (IsConnected())
        {
            //vpxFree();
        }
    }


    bool
    HmdVorpx::Connect()
    {
        if (mConnected)
        {
            return false;
        }
        mConnected = VPX_RES_OK == vpxInit();
        return mConnected;
    }

    bool
    HmdVorpx::IsConnected()
    {
        return mConnected;
    }


    bool
    HmdVorpx::UpdateState()
    {
        HmdServer::UpdateResponse_t response = {};
        Vect3FromVorpx(vpxGetHeadsetPosition(), response.headsetPos);
        QuatFromVorpx(vpxGetHeadsetRotationQuaternion(), response.headsetRot);
        Vect3FromVorpx(vpxGetHeadsetRotationEuler(), response.headsetRotEuler);
        Vect3ToRadians(response.headsetRotEuler);

        // Todo: Unify with HmdServer
        response.headsetRotEuler.pitch = -response.headsetRotEuler.pitch;
        response.headsetRotEuler.yaw = -response.headsetRotEuler.yaw;

        response.headsetPosDelta = mLastUpdate.headsetPos - response.headsetPos;
        response.headsetRotEulerDelta = mLastUpdate.headsetRotEuler - response.headsetRotEuler;


        mLastUpdate = response;
        return true;
    }

    bool 
    HmdVorpx::ResetVr()
    {
        return false;
    }

    bool 
    HmdVorpx::ResetPos()
    {
        return false;
    }

    HmdServer::UpdateResponse_t
    HmdVorpx::GetLastUpdateState()
    {
        return mLastUpdate;
    }
}
