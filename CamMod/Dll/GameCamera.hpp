#pragma once
#include "CamMod.h"
#include "CamVector.hpp"
#include "gmath/Matrix3x3.hpp"

namespace CamMod
{

class GameCamera
{
public:
    enum
    {
        POINT_SOURCE = 0,
        POINT_TARGET = 1
    };

    GameCamera();
    ~GameCamera();

    void BeginFrame(const Camera& aCamera);
    void ToWorldCoords(const Vector3& aLocalIn, Vector3& aWorldOut);
    void FromWorldCoords(const Vector3& aWorldIn, Vector3& aCamOut);

    void SetInvertY(bool aInvertY);
    void Reset();
    void Reverse();
    void LookDir(const Quaternion& aDir, float aDist = 10.f);
    void SetTarget(const Vector3& aPos);
    void SetSource(const Vector3& aPos);
    void SetSourceLock(bool aLock);
    void Translate(const Vector3& aWorldMove, bool mMoveTarget = false );
    void MoveCamera(const Vector3& aOffset, bool mMoveTarget = false);
    void SetRotation(const Vector3& aHeadsetRot, const Vector3& aRotDelta );
    void GetCamInfo( Vector3& aStart, Vector3& aEnd, Quaternion& aRoll );

    const CamVector& GetFrameStartVector() const;
    const CamVector& GetFrameEndVector() const;
    const CamVector& GetSrcTargetVector() const;

private:
    CamVector mCameraFrameStartVector;
    CamVector mCameraFrameEndVector;
    CamVector mSrcTargetVector;
    bool mTargetSet;
    bool mSourceSet;

    Vector3 mSrcOffset;
    Vector3 mTargetOffset;


    Vector3 mCumulativeRot;
    Vector3 mHeadsetRot;

    Vector3 mRotTgtAdjVec;

    bool mInvertY;
    bool mLockSource;
};

}