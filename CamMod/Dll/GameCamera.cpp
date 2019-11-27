#include "GameCamera.hpp"

#include "Windows.h"
#include <sstream>
#include <iomanip>
#include "gmath/Matrix3x3.hpp"


namespace
{
    const Quaternion ORIG_CAM_ROLL_QUAT(Quaternion(0.f, 1.f, 0.f, 0.f));


    void printQuat(std::stringstream& stream, Quaternion& quat)
    {
        stream << "w: " << std::setw(9) << quat.W << ", "
            << "x: " << std::setw(9) << quat.X << ", "
            << "y: " << std::setw(9) << quat.Y << ", "
            << "z: " << std::setw(9) << quat.Z << std::endl;
    }

    void printVect(std::stringstream& stream, Vector3& vect)
    {
        stream << std::setw(9) << vect.X << ", "
            << std::setw(9) << vect.Y << ", "
            << std::setw(9) << vect.Z << ", " << std::endl;
    }

    void printVectDeg(std::stringstream& stream, Vector3& vect)
    {
        stream << std::setw(9) << to_degrees(vect.X) << ", "
            << std::setw(9) << to_degrees(vect.Y) << ", "
            << std::setw(9) << to_degrees(vect.Z) << ", " << std::endl;
    }
}

namespace CamMod
{


    GameCamera::GameCamera( )
        : mCameraFrameStartVector()
        , mCameraFrameEndVector()
        , mSrcTargetVector()
        , mTargetSet( false )
        , mSourceSet( false )
        , mSrcOffset()
        , mTargetOffset()
        , mCumulativeRot( Vector3::Zero() )
        , mHeadsetRot( Vector3::Zero() )
        , mInvertY( true )
        , mLockSource( false )
    {
    }


    GameCamera::~GameCamera()
    {

    }


    void
    GameCamera::BeginFrame(const Camera& aCamera)
    {
        mCameraFrameStartVector = CamVector(aCamera.source, aCamera.target, mCameraFrameStartVector.GetRoll());
        mTargetSet = false;
        mSourceSet = false;

        if (mCameraFrameStartVector != mCameraFrameEndVector)
        {
            //OutputDebugString("Camera was moved external to CamMod");
        }
        if( VectorHasNan( mSrcOffset ) || VectorHasNan( mCameraFrameStartVector.GetStart() )
            || VectorHasNan(mCameraFrameStartVector.GetEnd()))
        {
            Reset();
        }
    }

    void
    GameCamera::ToWorldCoords(const Vector3& aLocalIn, Vector3& aWorldOut)
    {
        const auto& forward = mCameraFrameEndVector;
        //auto forwardUnit = Vector3::Normalized(forward.GetEnd() - forward.GetStart());
        aWorldOut = forward.GetBasisMatrix() * aLocalIn;
    }


    void
    GameCamera::FromWorldCoords(const Vector3& aWorldIn, Vector3& aLocalOut)
    {
        const auto& forward = mCameraFrameEndVector;
        aLocalOut = forward.GetBasisInverseMatrix() * aWorldIn;
    }

    void
    GameCamera::SetInvertY( bool aSetInvertY )
    {
        mInvertY = aSetInvertY;
    }

    void
    GameCamera::Reset()
    {
        mSrcOffset = Vector3::Zero();
        mTargetOffset = Vector3::Zero();
        mCumulativeRot = Vector3::Zero();
        mHeadsetRot = Vector3::Zero();
    }

    void
    GameCamera::Reverse()
    {
        Vector3 tmp = mSrcTargetVector.GetEnd();
        mSrcTargetVector.SetEnd(mSrcTargetVector.GetStart());
        mSrcTargetVector.SetStart(tmp);

        tmp = mSrcOffset;
        mSrcOffset = mTargetOffset;
        mTargetOffset = tmp;
    }

    void
    GameCamera::LookDir(const Quaternion& aDir, float aDist)
    {
        Vector3 adjVector;
        rotate_vector_by_quaternion(Vector3::Right(), aDir, adjVector);
        SetTarget(mSrcTargetVector.GetStart() + adjVector * aDist);
    }


    void
    GameCamera::SetTarget(const Vector3& aPos)
    {
        mSrcTargetVector.SetEnd(aPos);
        mTargetSet = true;
    }

    void
    GameCamera::SetSource(const Vector3& aPos)
    {
        mSrcTargetVector.SetStart(aPos);
        mSourceSet = true;
    }

    void GameCamera::SetSourceLock(bool aLock)
    {
        mLockSource = aLock;
    }


    void
    GameCamera::Translate( const Vector3& aWorldMove, bool aMoveTarget )
    {
        // Given aWorldMove, how must we change the current offset to accomplish that world move?
        Vector3& offset = (aMoveTarget ? mTargetOffset : mSrcOffset);
        Vector3 adjOffset;
        adjOffset = mSrcTargetVector.GetBasisInverseMatrix()*aWorldMove;
        //FromWorldCoords(aWorldMove, adjOffset);
        offset += adjOffset;

        // If no target was set and we just moved the source, then also move the target
        if (!aMoveTarget && !mTargetSet)
        {
            Translate(aWorldMove, true);
        }
    }

    void
    GameCamera::MoveCamera(const Vector3& aOffset, bool aMoveTarget)
    {
        Vector3& targetOffset = (aMoveTarget ? mTargetOffset : mSrcOffset);
        targetOffset += aOffset;

        // If no target was set and we just moved the source, then also move the target
        if (!aMoveTarget && !mTargetSet)
        {
            MoveCamera(aOffset, true);
        }
    }


    void
    GameCamera::SetRotation(const Vector3& aHeadsetRot, const Vector3& aRotDelta)
    {
        // Adjust the rotations based off any current screen roll
        float rollTheta = mCumulativeRot.roll + mHeadsetRot.roll;
        Quaternion curRollQuat = Quaternion::FromAngleAxis(rollTheta, Vector3::Forward());
        Quaternion rotDeltaQuat = Quaternion::FromEuler(aRotDelta);
        rotDeltaQuat = curRollQuat*rotDeltaQuat*Quaternion::Inverse(curRollQuat);
        Vector3 adjRotDelta = Quaternion::ToEuler(rotDeltaQuat);

        mCumulativeRot += adjRotDelta;
        mHeadsetRot = aHeadsetRot;
        mSrcTargetVector.SetRoll(mCumulativeRot.roll);
    }


    void
    GameCamera::GetCamInfo(Vector3& aStart, Vector3& aEnd, Quaternion& aRoll)
    {
        if (mLockSource)
        {
            mSrcTargetVector.SetStart(mCameraFrameEndVector.GetStart());
        }



        // Ensure we don't have zeroed values
        if (!mSourceSet && mSrcTargetVector.GetStart() == Vector3::Zero())
        {
            mSrcTargetVector.SetStart(mCameraFrameStartVector.GetStart());
        }
        if (!mTargetSet && mSrcTargetVector.GetEnd() == Vector3::Zero() )
        {
            mSrcTargetVector.SetEnd(mCameraFrameStartVector.GetEnd());
        }


        // Apply position offsets
        aStart = mSrcTargetVector.GetStart() + mSrcTargetVector.GetBasisMatrix()*mSrcOffset;
        aEnd = mSrcTargetVector.GetEnd() + mSrcTargetVector.GetBasisMatrix()*mTargetOffset;


        // Now rotate the end point
        // First normalize aStart->aEnd to be a unit vector
        CamVector camVect = CamVector(aEnd, aStart, mSrcTargetVector.GetRoll(), true);

        float yawAdj = mCumulativeRot.yaw + mHeadsetRot.yaw;
        float pitchAdj = mCumulativeRot.pitch + mHeadsetRot.pitch;

        // Now figure out the angle to adjust by and create rotation quaternions
        Vector3 camUp;
        Vector3 camRight;
        Vector3 camForward;
        camVect.GetBasis(camRight, camUp, camForward);

        if (mInvertY)
        {
            pitchAdj = -pitchAdj;
        }

        Quaternion pitchQuat = Quaternion::FromAngleAxis(pitchAdj, camRight);
        Quaternion yawQuat = Quaternion::FromAngleAxis(yawAdj, camUp);
        Quaternion rotQuat = yawQuat* pitchQuat;

        // If looks valid then apply rotation. This rotates camForward.
        Vector3 rotAdjVector;
        if (!isnan(rotQuat.W))
        {
            rotate_vector_by_quaternion(camForward, rotQuat, rotAdjVector);
        }
        else
        {
            Vector3 unused;
            mSrcTargetVector.GetBasis(unused, unused, rotAdjVector);
        }

        // Update aEnd to be the rotated vector
        aEnd = aStart + 1000.f*rotAdjVector;

        mCameraFrameEndVector = CamVector(aStart, aEnd, mSrcTargetVector.GetRoll());

        // Calculate total screen roll
        float rollTheta = mCumulativeRot.roll + mHeadsetRot.roll;
        Quaternion fromEuler = Quaternion::FromAngleAxis(rollTheta, rotAdjVector);
        aRoll = fromEuler * ORIG_CAM_ROLL_QUAT * Quaternion::Inverse(fromEuler);
    }


    const CamVector&
    GameCamera::GetFrameStartVector() const
    {
        return mCameraFrameStartVector;
    }

    const CamVector&
        GameCamera::GetFrameEndVector() const
    {
        return mCameraFrameEndVector;
    }

    const CamVector&
    GameCamera::GetSrcTargetVector() const
    {
        return mSrcTargetVector;
    }
}