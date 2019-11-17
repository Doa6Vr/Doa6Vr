#include "CamVector.hpp"


namespace CamMod
{

    CamVector::CamVector(const Vector3& aStart, const Vector3& aEnd, float aRoll, bool aNormalize)
        : mStart( aStart )
        , mEnd( aEnd )
        , mRoll( aRoll )
        , mTransMatrix()
        , mTransMatrixReady( false )
    {
        if (aNormalize)
        {
            mEnd = Vector3::Normalized(aEnd - aStart);
            mStart = Vector3::Zero();
        }

    }
    

    void
    CamVector::SetStart(const Vector3& aPos)
    {
        mStart = aPos;
        mTransMatrixReady = false;
    }

    void
    CamVector::SetEnd(const Vector3& aPos)
    {
        mEnd = aPos;
        mTransMatrixReady = false;
    }

    void
    CamVector::SetRoll(float aRoll)
    {
        mRoll = aRoll;
        mTransMatrixReady = false;
    }

    const Vector3&
    CamVector::GetStart() const
    {
        return mStart;
    }

    const Vector3&
    CamVector::GetEnd() const
    {
        return mEnd;
    }

    float
    CamVector::GetRoll() const
    {
        return mRoll;
    }

    const Matrix3x3& 
    CamVector::GetTransMatrix() const
    {
        if (!mTransMatrixReady )
        {
            Vector3 right;
            Vector3 up;
            Vector3 forward = Vector3::Normalized(mStart - mEnd);
            GetLocalOrthsNoRoll(forward, right, up);

            //// Apply roll to camera local axis
            //Quaternion rollQuat = Quaternion::FromAngleAxis(mRoll, forward);
            //right = rollQuat * right;
            //up = rollQuat * up;

            mTransMatrix = Matrix3x3::Transpose(Matrix3x3(right, up, forward));
            mTransMatrixReady = true;
        }
        return mTransMatrix;
    }

    void
    CamVector::GetBasis(Vector3& aRight, Vector3& aUp, Vector3& aForward) const
    {
        auto transMatrix = GetTransMatrix();
        aRight =   Vector3(transMatrix.D00, transMatrix.D10, transMatrix.D20);
        aUp =      Vector3(transMatrix.D01, transMatrix.D11, transMatrix.D21);
        aForward = Vector3(transMatrix.D02, transMatrix.D12, transMatrix.D22);
    };

    bool
    CamVector::operator==(const CamVector& aOth) const
    {
        return(aOth.mStart == this->mStart && aOth.mEnd == this->mEnd);
    }

    bool
    CamVector::operator!=(const CamVector& aOth) const
    {
        return !(*this == aOth);
    }
}