/******************************************************************************
 *
 *    vorpX C API for modders and developers
 *    Copyright 2019 Ralf Ostertag, Animation Labs. All rights reserved.
 *
 *****************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "vorpAPI.h"
#include <stdio.h>

typedef void* (__cdecl* VorpApiGetFuncAddress_t)(const DWORD procId, const char* funcName); VorpApiGetFuncAddress_t _vpxGetFuncAddress = NULL;
typedef VPX_RESULT (__cdecl* VorpApiInit_t)(); VorpApiInit_t _vpxInit = NULL;
typedef VPX_RESULT (__cdecl* VorpApiFree_t)(); VorpApiFree_t _vpxFree = NULL;
typedef VPX_BOOL (__cdecl* VorpApiIsActive_t)(); VorpApiIsActive_t _vpxIsActive = NULL;
typedef void (__cdecl* VorpApiSetInt_t)(const VPX_PROP_ID id, const int value); VorpApiSetInt_t _vpxSetInt = NULL;
typedef void(__cdecl* VorpApiSetFloat_t)(const VPX_PROP_ID id, const float value); VorpApiSetFloat_t _vpxSetFloat = NULL;
typedef int (__cdecl* VorpApiGetInt_t)(const VPX_PROP_ID id); VorpApiGetInt_t _vpxGetInt = NULL;
typedef vpxint2 (__cdecl* VorpApiGetInt2_t)(const VPX_PROP_ID id); VorpApiGetInt2_t _vpxGetInt2 = NULL;
typedef vpxint3 (__cdecl* VorpApiGetInt3_t)(const VPX_PROP_ID id); VorpApiGetInt3_t _vpxGetInt3 = NULL;
typedef vpxint4 (__cdecl* VorpApiGetInt4_t)(const VPX_PROP_ID id); VorpApiGetInt4_t _vpxGetInt4 = NULL;
typedef float (__cdecl* VorpApiGetFloat_t)(const VPX_PROP_ID id); VorpApiGetFloat_t _vpxGetFloat = NULL;
typedef vpxfloat2 (__cdecl* VorpApiGetFloat2_t)(const VPX_PROP_ID id); VorpApiGetFloat2_t _vpxGetFloat2 = NULL;
typedef vpxfloat3 (__cdecl* VorpApiGetFloat3_t)(const VPX_PROP_ID id); VorpApiGetFloat3_t _vpxGetFloat3 = NULL;
typedef vpxfloat4 (__cdecl* VorpApiGetFloat4_t)(const VPX_PROP_ID id); VorpApiGetFloat4_t _vpxGetFloat4 = NULL;
typedef vpxmtx4x4 (__cdecl* VorpApiGetFloat4x4_t)(const VPX_PROP_ID id); VorpApiGetFloat4x4_t _vpxGetFloat4x4 = NULL;
typedef VPX_CONTROLLER_STATE (__cdecl* VorpApiGetControllerState_t)(const unsigned int controllerNum); VorpApiGetControllerState_t _vpxGetControllerState = NULL;
typedef vpxfloat3 (__cdecl* VorpApiYawCorrection_t)(const vpxfloat3 position, const float yawInDegrees); VorpApiYawCorrection_t _vpxYawCorrection = NULL;

typedef float (__cdecl* VorpApiVertToHorFOV_t)(const float fov_deg, const float aspect); VorpApiVertToHorFOV_t _vpxVertToHorFOV = NULL;

VPX_RESULT vpxIsActive() { if (!_vpxIsActive) return VPX_FALSE; return _vpxIsActive(); }
float vpxGetHeadsetFOV() { if (!_vpxGetFloat) return 0.0f; return _vpxGetFloat(VPX_PROP_HEADSET_FOV); }
vpxfloat3 vpxGetHeadsetRotationEuler() { if (!_vpxGetFloat3) { vpxfloat3 r = { 0,0,0 }; return r; } return _vpxGetFloat3(VPX_PROP_HEADSET_ROT_EULER); }
vpxfloat4 vpxGetHeadsetRotationQuaternion() { if (!_vpxGetFloat4) { vpxfloat4 r = { 0,0,0,0 }; return r; } return _vpxGetFloat4(VPX_PROP_HEADSET_ROT_QUATERNION); }
vpxfloat3 vpxGetHeadsetPosition() { if (!_vpxGetFloat3) { vpxfloat3 r = { 0,0,0 }; return r; } return _vpxGetFloat3(VPX_PROP_HEADSET_POS); }
VPX_CONTROLLER_STATE vpxGetControllerState(const unsigned int num) { if (!_vpxGetControllerState) { VPX_CONTROLLER_STATE r; memset(&r, 0, sizeof(r)); return r; } return _vpxGetControllerState(num); }
vpxfloat3 vpxGetControllerRotationEuler(const unsigned int num) { if (!_vpxGetFloat3) { vpxfloat3 r = { 0,0,0 }; return r; } return num == VPX_LEFT ? _vpxGetFloat3(VPX_PROP_CONTROLLER_L_ROT_EULER) : _vpxGetFloat3(VPX_PROP_CONTROLLER_R_ROT_EULER); }
vpxfloat4 vpxGetControllerRotationQuaternion(const unsigned int num) { if (!_vpxGetFloat4) { vpxfloat4 r = { 0,0,0,0 }; return r; } return num == VPX_LEFT ? _vpxGetFloat4(VPX_PROP_CONTROLLER_L_ROT_QUATERNION) : _vpxGetFloat4(VPX_PROP_CONTROLLER_R_ROT_QUATERNION); }
vpxfloat3 vpxGetControllerPosition(const unsigned int num) { if (!_vpxGetFloat3) { vpxfloat3 r = { 0,0,0 }; return r; } return num == VPX_LEFT ? _vpxGetFloat3(VPX_PROP_CONTROLLER_L_POS) : _vpxGetFloat3(VPX_PROP_CONTROLLER_R_POS); }
void vpxRequestEdgePeek(const VPX_BOOL val) { if (!_vpxSetInt) return; _vpxSetInt(VPX_PROP_EDGE_PEEK_REQUESTED, val); }
VPX_BOOL vpxGetEdgePeekRequested() { if (!_vpxGetInt) return VPX_FALSE; return _vpxGetInt(VPX_PROP_EDGE_PEEK_REQUESTED); }
VPX_BOOL vpxGetEdgePeekActual() { if (!_vpxGetInt) return VPX_FALSE; return _vpxGetInt(VPX_PROP_EDGE_PEEK_ACTUAL); }
vpxfloat3 vpxYawCorrection(const vpxfloat3 position, const float yawInDegrees) { if (!_vpxYawCorrection) { return position; } return _vpxYawCorrection(position, yawInDegrees); }
float vpxVertToHorFOV(const float fov_deg, const float aspect) { if (!_vpxVertToHorFOV) { return 0.0f; } return _vpxVertToHorFOV(fov_deg, aspect); }
void vpxSetStereoReduction(const float reduction) { if (!_vpxSetFloat) return; _vpxSetFloat(VPX_PROP_STEREO_REDUCTION, reduction); }
void vpxForceControllerRendering(const VPX_BOOL val) { if (!_vpxSetInt) return; _vpxSetInt(VPX_PROP_CONTROLLER_FORCE_RENDER, val); }
void _nullifyFunctions();

VPX_RESULT vpxFree()
{
	VPX_RESULT res = VPX_RES_OK;
	if (_vpxFree) { res = _vpxFree(); }
	_nullifyFunctions();
	return res;
}

VPX_RESULT vpxInit()
{
	const VPX_RESULT RFU = VPX_RES_FUNCTION_UNAVAILABLE;
	VPX_RESULT res = VPX_RES_FAIL;
	DWORD pid = GetCurrentProcessId();
	char fn[64] = { 0 };
	sprintf_s(fn, 64, "Local\\vpxapi_gpa%u", pid);
	HANDLE hmf = OpenFileMappingA(FILE_MAP_READ, FALSE, fn);
    if (hmf == NULL)
    {
        DWORD err = GetLastError();
        err = 0;
    }
	if (hmf) {
		void* pmf = MapViewOfFile(hmf, FILE_MAP_READ, 0, 0, sizeof(__int64));
		if (pmf) {
			if (!(_vpxGetFuncAddress = (void*)*(__int64*)pmf)) { res = RFU; goto end; }
			if (!(_vpxInit = _vpxGetFuncAddress(pid, "VorpApiInit"))) { res = RFU; goto end; }
			if (VPX_RES_OK != _vpxInit()) { res = VPX_RES_NOT_INITIALIZED; goto end; }
			if (!(_vpxFree = _vpxGetFuncAddress(pid, "VorpApiFree"))) { res = RFU; goto end; }
			if (!(_vpxIsActive = _vpxGetFuncAddress(pid, "VorpApiIsActive"))) { res = RFU; goto end; }
			if (!(_vpxSetInt = _vpxGetFuncAddress(pid, "VorpApiSetInt"))) { res = RFU; goto end; }
			if (!(_vpxSetFloat = _vpxGetFuncAddress(pid, "VorpApiSetFloat"))) { res = RFU; goto end; }
			if (!(_vpxGetInt = _vpxGetFuncAddress(pid, "VorpApiGetInt"))) { res = RFU; goto end; }
			if (!(_vpxGetInt2 = _vpxGetFuncAddress(pid, "VorpApiGetInt2"))) { res = RFU; goto end; }
			if (!(_vpxGetInt3 = _vpxGetFuncAddress(pid, "VorpApiGetInt3"))) { res = RFU; goto end; }
			if (!(_vpxGetInt4 = _vpxGetFuncAddress(pid, "VorpApiGetInt4"))) { res = RFU; goto end; }
			if (!(_vpxGetFloat = _vpxGetFuncAddress(pid, "VorpApiGetFloat"))) { res = RFU; goto end; }
			if (!(_vpxGetFloat2 = _vpxGetFuncAddress(pid, "VorpApiGetFloat2"))) { res = RFU; goto end; }
			if (!(_vpxGetFloat3 = _vpxGetFuncAddress(pid, "VorpApiGetFloat3"))) { res = RFU; goto end; }
			if (!(_vpxGetFloat4 = _vpxGetFuncAddress(pid, "VorpApiGetFloat4"))) { res = RFU; goto end; }
			if (!(_vpxGetFloat4x4 = _vpxGetFuncAddress(pid, "VorpApiGetFloat4x4"))) { res = RFU; goto end; }
			if (!(_vpxGetControllerState = _vpxGetFuncAddress(pid, "VorpApiGetControllerState"))) { res = RFU; goto end; }
			if (!(_vpxYawCorrection = _vpxGetFuncAddress(pid, "VorpApiYawCorrection"))) { res = RFU; goto end; }
			if (!(_vpxVertToHorFOV = _vpxGetFuncAddress(pid, "VorpApiVertToHorFOV"))) { res = RFU; goto end; }
			res = VPX_RES_OK;
		end:
			UnmapViewOfFile(pmf); pmf = NULL;
		}
		CloseHandle(hmf); hmf = NULL;
	}
	if (res != VPX_RES_OK) { _nullifyFunctions(); }
	return res;
}

void _nullifyFunctions()
{
	_vpxGetFuncAddress = NULL;
	_vpxInit = NULL;
	_vpxFree = NULL;
	_vpxIsActive = NULL;
	_vpxSetInt = NULL;
	_vpxSetFloat = NULL;
	_vpxGetInt = NULL;
	_vpxGetInt2 = NULL;
	_vpxGetInt3 = NULL;
	_vpxGetInt4 = NULL;
	_vpxGetFloat = NULL;
	_vpxGetFloat2 = NULL;
	_vpxGetFloat3 = NULL;
	_vpxGetFloat4 = NULL;
	_vpxGetFloat4x4 = NULL;
	_vpxGetControllerState = NULL;
	_vpxYawCorrection = NULL;
	_vpxVertToHorFOV = NULL;
}
