#include "OpenVrHmd.hpp"
#include "Utils.hpp"

#pragma comment(lib, "..\\OpenVR\\lib\\win64\\openvr_api.lib")

namespace HmdServer
{

    OpenVrHmd::OpenVrHmd()
        : mVrSystem( nullptr )
        , mHmdIdx( 0 )
        , mExpectedFrameLag( 0.f )
    {

    }


    OpenVrHmd::~OpenVrHmd()
    {
        Uninit();
    }


    bool
    OpenVrHmd::Init()
    {
        vr::EVRInitError errCode;
        if (!mVrSystem && vr::VR_IsHmdPresent())
        {
            mVrSystem = vr::VR_Init(&errCode, vr::VRApplication_Scene);
            ResetVrPosition();
        }
        return( nullptr != mVrSystem);
    }


    bool
    OpenVrHmd::Uninit()
    {
        if (mVrSystem)
        {
            vr::VR_Shutdown();
            return true;
        }
        return false;
    }


    void
    OpenVrHmd::ResetVrPosition()
    {
        if (mVrSystem)
        {
            mVrSystem->ResetSeatedZeroPose();
        }
    }

    bool
    OpenVrHmd::UpdateHmdIndex()
    {
        if (!mVrSystem)
        {
            return false;
        }

        if (mVrSystem->GetTrackedDeviceClass(mHmdIdx) == vr::TrackedDeviceClass_HMD && mVrSystem->IsTrackedDeviceConnected(mHmdIdx) )
        {
            return true;
        }

        bool success = false;
        for (uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; i++)
        {
            if (mVrSystem->GetTrackedDeviceClass(i) == vr::TrackedDeviceClass_HMD && mVrSystem->IsTrackedDeviceConnected(i))
            {
                mHmdIdx = i;
                break;
            }
        }

        return success;
    }

    void
    OpenVrHmd::SetLag(float aMs)
    {
        mExpectedFrameLag = aMs;
    }
    
    
    float
    OpenVrHmd::GetLag()
    {
        return mExpectedFrameLag;
    }


    bool
    OpenVrHmd::GetHeadsetPosition(Vector3* aPos, Vector3* aRotEuler, Quaternion* aRot)
    {
        if (!mVrSystem || !UpdateHmdIndex())
        {
            return false;
        }

        vr::TrackedDevicePose_t hmdPose;
        vr::TrackedDevicePose_t poseArray[vr::k_unMaxTrackedDeviceCount];

        mVrSystem->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseSeated, mExpectedFrameLag, poseArray, vr::k_unMaxTrackedDeviceCount);
        hmdPose = poseArray[mHmdIdx];

        bool success = hmdPose.bPoseIsValid;
        if (success)
        {
            if (aPos)
            {
                *aPos = GetPosition(hmdPose.mDeviceToAbsoluteTracking);
            }
            if (aRotEuler || aRot)
            {
                Quaternion quat;
                QuaternionFromMatrix(hmdPose.mDeviceToAbsoluteTracking, quat);
                if (aRot)
                {
                    *aRot = quat;
                    //QuaternionFromMatrix(hmdPose.mDeviceToAbsoluteTracking, *aRot);
                }
                if (aRotEuler)
                {
                    *aRotEuler = -Quaternion::ToEuler(quat);
                    // Shift range from [-pi, pi] to [0, 2pi]
                    *aRotEuler += (float)M_PI;
                    //vr::HmdVector4_t vrVect;
                    //RotationMatrixToYPR(hmdPose.mDeviceToAbsoluteTracking, vrVect);
                    //aRotEuler->yaw = -vrVect.v[0];
                    //aRotEuler->roll = -vrVect.v[1];
                    //aRotEuler->pitch = -vrVect.v[2];

                    //// Shift range from [-pi, pi] to [0, 2pi]
                    //*aRotEuler += (float)M_PI;
                }
            }
        }
        return success;
    }
}
