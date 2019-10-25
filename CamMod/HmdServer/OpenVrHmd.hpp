#pragma once
#include "openvr.h"

struct Quaternion;
struct Vector3;

namespace HmdServer
{
    class OpenVrHmd
    {
        public:
            OpenVrHmd();
            ~OpenVrHmd();

            bool Init();
            bool Uninit();

            void SetLag(float aFrames);
            float GetLag();

            void ResetVrPosition();

            bool GetHeadsetPosition(Vector3* aPos, Vector3* aRotEuler, Quaternion* aRot );

        private:
            vr::IVRSystem* mVrSystem;
            vr::TrackedDeviceIndex_t mHmdIdx;
            float mExpectedFrameLag;

            bool UpdateHmdIndex();
    };
}
