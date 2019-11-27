#pragma once
#include "gmath/Vector3.hpp"
#include "gmath/Matrix3x3.hpp"
#include "Utils.hpp"

namespace CamMod
{
    class CamVector
    {
    public:
        CamVector() = default;
        CamVector( const Vector3& aStart, const Vector3& aEnd, float aRoll = 0.f, bool aNormalize = false );

        void SetStart(const Vector3& aPos);
        void SetEnd(const Vector3& aPos);
        void SetRoll(float aRoll);
        const Vector3& GetStart() const;
        const Vector3& GetEnd() const;
        float GetRoll() const;

        const Matrix3x3& GetBasisMatrix() const;
        const Matrix3x3& GetBasisInverseMatrix() const;
        void GetBasis(Vector3& aRight, Vector3& aUp, Vector3& aForward) const;
        bool operator==(const CamVector& aOth) const;
        bool operator!=(const CamVector& aOth) const;
    private:
        Vector3 mStart;
        Vector3 mEnd;
        float mRoll;
        
        mutable Matrix3x3 mBasisMatrix;
        mutable Matrix3x3 mBasisInvMatrix;
        mutable bool mBasisVectorReady;
        mutable bool mBasisInvMatrixReady;
    };
}