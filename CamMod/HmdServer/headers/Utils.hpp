#pragma once
#include "openvr.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "gmath/Quaternion.hpp"

namespace
{
    void GetLocalOrthsNoRoll(const Vector3& aForward, Vector3& aRightOut, Vector3& aUpOut);
    void RotateVector(const vr::HmdVector4_t& aVector, const vr::HmdVector4_t& aAxis, float aTheta, vr::HmdVector4_t& aRotatedVector);
    void GetRotationMatrix(const vr::HmdVector4_t& aAxis, float aTheta, vr::HmdMatrix44_t& aMatrixOut);
    void RotationMatrixToYPR(vr::HmdMatrix34_t m, vr::HmdVector4_t& v);
    void YPRFromQuaternion(Quaternion q, vr::HmdVector4_t& v);
    void QuaternionFromMatrix(vr::HmdMatrix34_t m, Quaternion& q);
    Vector3 GetPosition(vr::HmdMatrix34_t matrix);
    vr::HmdQuaternionf_t QuatMult(vr::HmdQuaternionf_t& aA, vr::HmdQuaternionf_t& aB);


    inline float to_degrees(float radians) {
        return radians * (180.0f / M_PI_F);
    }

    inline float to_radians(float degrees) {
        return degrees * (M_PI_F / 180.f);
    }

    Vector3 GetPosition(vr::HmdMatrix34_t matrix)
    {
        Vector3 vector;

        vector.X = matrix.m[0][3];
        vector.Y = matrix.m[1][3];
        vector.Z = matrix.m[2][3];

        return vector;
    }

    void GetLocalOrthsNoRoll(const Vector3& aForward, Vector3& aRightOut, Vector3& aUpOut)
    {
        aRightOut = Vector3::Normalized(Vector3::Cross(Vector3::Up(), aForward));
        aUpOut = Vector3::Cross(aRightOut, aForward);
    }

    // Adapted from https://steamcommunity.com/app/250820/discussions/0/1728711392744037419/
    void QuaternionFromMatrix(vr::HmdMatrix34_t m, Quaternion& q)
    {
        q.W = sqrtf(1 + m.m[0][0] + m.m[1][1] + m.m[2][2]) / 2.0f; // Scalar
        q.X = (m.m[2][1] - m.m[1][2]) / (4 * q.W);
        q.Y = (m.m[0][2] - m.m[2][0]) / (4 * q.W);
        q.Z = (m.m[1][0] - m.m[0][1]) / (4 * q.W);
    }

    void YPRFromQuaternion(Quaternion q, vr::HmdVector4_t& v)
    {
        // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/index.htm
        // Conversion of the code under Code, Java code to do conversion:

        double test = q.X * q.Y + q.Z * q.W;
        if (test > 0.499)
        { // singularity at north pole
            v.v[0] = (float)(2 * atan2(q.X, q.W)); // heading
            v.v[1] = (float)(M_PI / 2); // attitude
            v.v[2] = 0.f; // bank
            return;
        }
        if (test < -0.499)
        { // singularity at south pole
            v.v[0] = (float)(-2 * atan2(q.X, q.W)); // headingq
            v.v[1] = (float)(-M_PI / 2); // attitude
            v.v[2] = 0.f; // bank
            return;
        }
        double sqx = q.X * q.X;
        double sqy = q.Y * q.Y;
        double sqz = q.Z * q.Z;
        v.v[0] = (float)(atan2(2 * q.Y * q.W - 2 * q.X * q.Z, 1 - 2 * sqy - 2 * sqz)); // heading
        v.v[1] = (float)(asin(2 * test)); // attitude
        v.v[2] = (float)(atan2(2 * q.X * q.W - 2 * q.Y * q.Z, 1 - 2 * sqx - 2 * sqz)); // bank

                                                                                       // Move range to [0,2pi]
        v.v[0] += (float)M_PI;
        v.v[1] += (float)M_PI;
        v.v[2] += (float)M_PI;
        v.v[3] = 0.f;
    }

    void RotationMatrixToYPR(vr::HmdMatrix34_t m, vr::HmdVector4_t& v)
    {
        Quaternion q;
        QuaternionFromMatrix(m, q);
        YPRFromQuaternion(q, v);
    }


    // Adapted from https://www.programming-techniques.com/2012/03/3d-rotation-algorithm-about-arbitrary-axis-with-c-c-code.html
    void GetRotationMatrix( const vr::HmdVector4_t& aAxis, float aTheta, vr::HmdMatrix44_t& aMatrixOut )
    {
        float u = aAxis.v[0];
        float v = aAxis.v[1];
        float w = aAxis.v[2];
        float u2 = pow( u, 2 );
        float v2 = pow( v, 2 );
        float w2 = pow( w, 2 );
        float L = u2 + v2 + w2;

        aMatrixOut.m[0][0] = (u2 + (v2 + w2) * cos(aTheta)) / L;
        aMatrixOut.m[0][1] = (u * v * (1 - cos(aTheta)) - w * sqrtf(L) * sin(aTheta)) / L;
        aMatrixOut.m[0][2] = (u * w * (1 - cos(aTheta)) + v * sqrtf(L) * sin(aTheta)) / L;
        aMatrixOut.m[0][3] = 0.0;

        aMatrixOut.m[1][0] = (u * v * (1 - cos(aTheta)) + w * sqrtf(L) * sin(aTheta)) / L;
        aMatrixOut.m[1][1] = (v2 + (u2 + w2) * cos(aTheta)) / L;
        aMatrixOut.m[1][2] = (v * w * (1 - cos(aTheta)) - u * sqrtf(L) * sin(aTheta)) / L;
        aMatrixOut.m[1][3] = 0.0;

        aMatrixOut.m[2][0] = (u * w * (1 - cos(aTheta)) - v * sqrtf(L) * sin(aTheta)) / L;
        aMatrixOut.m[2][1] = (v * w * (1 - cos(aTheta)) + u * sqrtf(L) * sin(aTheta)) / L;
        aMatrixOut.m[2][2] = (w2 + (u2 + v2) * cos(aTheta)) / L;
        aMatrixOut.m[2][3] = 0.0;

        aMatrixOut.m[3][0] = 0.0;
        aMatrixOut.m[3][1] = 0.0;
        aMatrixOut.m[3][2] = 0.0;
        aMatrixOut.m[3][3] = 1.0;
    }


    void RotateVector( const vr::HmdVector4_t& aVector, const vr::HmdVector4_t& aAxis, float aTheta, vr::HmdVector4_t& aRotatedVector )
    {
        // Set up rotation matrix
        vr::HmdMatrix44_t rotMatrix;
        GetRotationMatrix( aAxis, aTheta, rotMatrix );

        // Multiply the matrix
        for (int i = 0; i < 4; i++)
        {
            aRotatedVector.v[i] = 0;
            for (int k = 0; k < 4; k++)
            {
                aRotatedVector.v[i] += rotMatrix.m[i][k] * aVector.v[k];
            }
        }
    }

    
    void QuatFromEuler( const vr::HmdVector4_t& aEuler, vr::HmdQuaternionf_t& aQuatOut )
    {

    }

    vr::HmdQuaternionf_t QuatMult(vr::HmdQuaternionf_t& aA, vr::HmdQuaternionf_t& aB)
    {
        vr::HmdQuaternionf_t res;
        res.x =  ( aA.x * aB.w ) + ( aA.y * aB.z ) - ( aA.z * aB.y ) + ( aA.w * aB.x );
        res.y = -( aA.x * aB.z ) + ( aA.y * aB.w ) - ( aA.z * aB.x ) + ( aA.w * aB.y );
        res.z =  ( aA.x * aB.y ) - ( aA.y * aB.x ) + ( aA.z * aB.w ) + ( aA.w * aB.z );
        res.w = -( aA.x * aB.x ) - ( aA.y * aB.y ) - ( aA.z * aB.z ) + ( aA.w * aB.w );
        return res;
    }


    // Taken from https://gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion
    void rotate_vector_by_quaternion(const Vector3& v, const Quaternion& q, Vector3& vprime)
    {
        // Extract the vector part of the quaternion
        Vector3 u(q.X, q.Y, q.Z);

        // Extract the scalar part of the quaternion
        float s = (float)q.W;

        // Do the math
        vprime = 2.0f * Vector3::Dot(u, v) * u
            + (s*s - Vector3::Dot(u, u)) * v
            + 2.0f * s * Vector3::Cross(u, v);
    }
}
