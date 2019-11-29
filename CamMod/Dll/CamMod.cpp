#include "CamMod.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "HmdServerIntf.hpp"
#include "HmdVorpx.hpp"
#include "UserInput.hpp"
#include "Utils.hpp"
#include "GameCamera.hpp"

#define DllExport   __declspec( dllexport )

#include <sstream>
#include <iomanip>

#include <cmath>

#define RETRY_FRAME_CNT 200
CamMod::IHmdServerIntf* gHmdServer = nullptr;
CamMod::UserInput gUserInput;
float gZoomAmount = 0.f;
uint8_t gPrevTargetIdx = 0;
uint8_t gPrevTargetTbl = 0;
uint8_t gPrevSrcIdx = 0;
uint8_t gPrevSrcTbl = 0;
CamMod::GameCamera gCamera;

Vector3 gCamSourceLock;
HmdServer::UpdateResponse gPrevStateUpdate;

const Quaternion sOrigCamRoll( Quaternion(0.f, 1.f, 0.f, 0.f) );

extern "C" DllExport void _cdecl Unload()
{
    delete gHmdServer;
    FreeLibraryAndExitThread(GetModuleHandle(TEXT("CamMod.dll")), 0);
}

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

void InitHmdServer();
bool GetStateUpdate(HmdServer::UpdateResponse& aState);
bool GetPosition(VariableBlock* aVarBlock, byte aTable, byte aIdx, PointDir& aPointDir);
void SetBreakAndHealth(VariableBlock* aVarBlock, uint8_t aP1Health, uint8_t aP1Break, uint8_t aP2Health, uint8_t aP2Break);

void targetBaseToTargetTables(void* aBase, CharTableEntry** aP1, CharTableEntry** aP2)
{
    CT_Base* base = (CT_Base*)aBase;
    if (aP1)
    {
        *aP1 = &base->lvl1->lvl2->finalLvl->entry;
    }
    if (aP2)
    {
        *aP2 = &base->lvl1->lvl2->lvl3->finalLvl->entry;
    }
}


extern "C" DllExport void _cdecl AdjustCamera(VariableBlock* varBlock)
{
    if (!varBlock || !varBlock->Cameras )
    {
        return;
    }
    Camera& mainCamera = varBlock->Cameras->cams[0];
    HmdServer::UpdateResponse stateUpdate = {};
    CamMod::UserInput::UserCommands commands = {};

    gCamera.BeginFrame(mainCamera);


    if (GetStateUpdate(stateUpdate))
    {
        if (varBlock->RequestPosReset)
        {
            gHmdServer->ResetPos();
            varBlock->RequestPosReset = 0;
        }
        if (varBlock->RequestVrReset)
        {
            gHmdServer->ResetVr();
            varBlock->RequestVrReset = 0;
        }

        const float HEADSET_MULT = 500 + varBlock->rotB;
        stateUpdate.headsetPos *= HEADSET_MULT;
        stateUpdate.headsetPosDelta *= HEADSET_MULT;

        stateUpdate.headsetRotEuler.roll += M_PI_F; // Oculus always flipped?

        //stateUpdate.headsetRotEulerDelta.pitch *= 12.f;


        //std::stringstream dbg;
        //dbg << "HeadsetRot:  ";
        //printVectDeg(dbg, stateUpdate.headsetRotEuler);
        //dbg << "-----" << std::endl;
        //OutputDebugString(dbg.str().c_str());
    }

    if (gUserInput.GetInput(commands))
    {
        gZoomAmount += commands.zoom;

        if (commands.resetPos)
        {
            gCamera.Reset();
            gZoomAmount = 0.f;
            varBlock->Cameras->camRoll = sOrigCamRoll;
            if (gHmdServer)
            {
                gHmdServer->ResetPos();
                gHmdServer->ResetVr();
            }
        }

    }

    SetBreakAndHealth( varBlock, commands.fullHealth[0], commands.fullBreak[0], commands.fullHealth[1], commands.fullBreak[1]);

    uint8_t sourceType = varBlock->sourceType;
    uint8_t targetType = varBlock->targetType;

    switch (commands.freeRoam)
    {
    case CamMod::UserInput::FREE_MODE_GAME_CAM:
        sourceType = SOURCE_ORIG;
        targetType = TARGET_ORIG;
        break;

    case CamMod::UserInput::FREE_MODE_SRC_GAME_TARGET_NONE:
        sourceType = SOURCE_ORIG;
        targetType = TARGET_LOCKED;
        break;

    case CamMod::UserInput::FREE_MODE_SRC_GAME_TARGET_TABLE:
        sourceType = SOURCE_ORIG;
        targetType = TARGET_TABLE;
        break;

    case CamMod::UserInput::FREE_MODE_NO_SOURCE_TARGET_GAME:
        sourceType = SOURCE_LOCKED;
        targetType = TARGET_ORIG;
        break;

    case CamMod::UserInput::FREE_MODE_NO_SOURCE:
        targetType = TARGET_TABLE;
        sourceType = SOURCE_OFF;
        break;

    case CamMod::UserInput::FREE_MODE_NO_SOURCE_NO_TARGET:
        targetType = TARGET_OFF;
        sourceType = SOURCE_OFF;
        break;

    case CamMod::UserInput::FREE_MODE_POV:
        sourceType = TARGET_TABLE;
        targetType = TARGET_POV;
        break;

    case CamMod::UserInput::FREE_MODE_SOURCE_AND_TARGET:
        sourceType = TARGET_TABLE;
        targetType = TARGET_TABLE;
        break;

    default:
    case CamMod::UserInput::FREE_MODE_NO_CHANGE: // nothing
        break;

    }
    
    if (commands.lockPitch > 0)
    {
        static HmdServer::UpdateResponse sLastUsedUpdate = stateUpdate;
        static int ROT_ALLOWED_FRAMES = 40;
        static int sRotAllowedCnt = 0;

        float MIN_POS_DELTA;
        float MIN_ROT_DELTA;

        switch (commands.lockPitch)
        {
            case 1:
                MIN_POS_DELTA = 0.5f;
                MIN_ROT_DELTA = .01f;
                ROT_ALLOWED_FRAMES = 20;
                break;

            case 2:
                MIN_POS_DELTA = 0.5f;
                MIN_ROT_DELTA = .01f;
                ROT_ALLOWED_FRAMES = 40;

            case 3:
                MIN_POS_DELTA = 2.0f;
                MIN_ROT_DELTA = .05f;
                ROT_ALLOWED_FRAMES = 40;
                break;

            case 4:
                MIN_POS_DELTA = 2.0f;
                MIN_ROT_DELTA = .05f;
                ROT_ALLOWED_FRAMES = 80;
                break;


            case 5:
                MIN_POS_DELTA = 8.0f;
                MIN_ROT_DELTA = .4f;
                ROT_ALLOWED_FRAMES = 80;
                break;

            case 6:
                MIN_POS_DELTA = 8.0f;
                MIN_ROT_DELTA = .4f;
                ROT_ALLOWED_FRAMES = 800;
                break;

            case 7:
                MIN_POS_DELTA = 99999.f;
                MIN_ROT_DELTA = 99999.f;
                ROT_ALLOWED_FRAMES = 100;
                break;

            default:
                break;
        }

        float sinceLastRotMag = Vector3::Magnitude( sLastUsedUpdate.headsetRotEuler - stateUpdate.headsetRotEuler );
        float sinceLastPosMag = Vector3::Magnitude(sLastUsedUpdate.headsetPos - stateUpdate.headsetPos);
        float rotMag = Vector3::Magnitude(stateUpdate.headsetRotEulerDelta);
        float posMag = Vector3::Magnitude(stateUpdate.headsetPosDelta);

        // If standing still
        if ( ( rotMag < MIN_ROT_DELTA && posMag < MIN_POS_DELTA )
          && ( sinceLastRotMag < 2 * MIN_ROT_DELTA && sinceLastPosMag < 2 * MIN_POS_DELTA) )
        {
            if (sRotAllowedCnt <= 0)
            {
                // Periodically reset the MIN_x_DELTA base position
                if (sRotAllowedCnt <= -ROT_ALLOWED_FRAMES)
                {
                    sLastUsedUpdate = stateUpdate;
                    sRotAllowedCnt = 0;
                }
                else
                {
                    sRotAllowedCnt--; // keep decrementing until -ROT_ALLOWED_FRAMES
                }

                stateUpdate.headsetPos = gPrevStateUpdate.headsetPos;
                stateUpdate.headsetRot = gPrevStateUpdate.headsetRot;
                stateUpdate.headsetRotEuler = gPrevStateUpdate.headsetRotEuler;
                stateUpdate.headsetPosDelta = Vector3::Zero();
                stateUpdate.headsetRotEulerDelta = Vector3::Zero();
            }
            else
            {
                sRotAllowedCnt--;
            }
        }
        else
        {
            sRotAllowedCnt = ROT_ALLOWED_FRAMES;
            sLastUsedUpdate = stateUpdate;
        }
    }

    PointDir sourcePos;
    if (sourceType == SOURCE_TABLE && GetPosition( varBlock, varBlock->SourceTableSelect, varBlock->SourceTableIdx, sourcePos ) )
    {
        switch (commands.lockHeight)
        {
        case CamMod::UserInput::LOCK_MODE_LOCK_SOURCE:
        case CamMod::UserInput::LOCK_MODE_LOCK_SOURCE_AND_TARGET:
        case CamMod::UserInput::LOCK_MODE_LOCK_ALL:
        case CamMod::UserInput::LOCK_MODE_LOCK_ALL_PLUS_HMD:
            sourcePos.pos.Y = gCamera.GetSrcTargetVector().GetStart().Y;
            break;
        }
        gCamera.SetSource(sourcePos.pos);
    }
    else if (sourceType == SOURCE_ORIG)
    {
        gCamera.SetSource(gCamera.GetFrameStartVector().GetStart());
    }
    else if (sourceType == SOURCE_LOCKED)
    {
        gCamera.SetSource(gCamera.GetSrcTargetVector().GetStart());
    }

    PointDir targetPos;
    // Handle targets
    if (targetType == TARGET_POV && sourceType == SOURCE_TABLE)
    {
        gCamera.LookDir(sourcePos.rot, 10.f);
    }
    else if (targetType == TARGET_TABLE && GetPosition( varBlock, varBlock->TargetTableSelect, varBlock->TargetTableIdx, targetPos ) )
    {
        switch (commands.lockHeight)
        {
            case CamMod::UserInput::LOCK_MODE_LOCK_TARGET:
            case CamMod::UserInput::LOCK_MODE_LOCK_SOURCE_AND_TARGET:
            case CamMod::UserInput::LOCK_MODE_LOCK_ALL:
            case CamMod::UserInput::LOCK_MODE_LOCK_ALL_PLUS_HMD:
                targetPos.pos.Y = gCamera.GetSrcTargetVector().GetEnd().Y;
                break;
        }
        gCamera.SetTarget(targetPos.pos);
    }
    else if (targetType == TARGET_ORIG)
    {
        gCamera.SetTarget(gCamera.GetFrameStartVector().GetEnd());
    }
    else if (targetType == TARGET_LOCKED)
    {
        gCamera.SetTarget(gCamera.GetSrcTargetVector().GetEnd());
    }


    if (varBlock->rotC > 0.f)
    {
        gCamera.Reverse();
    }

    // Apply position offsets
    if (Vector3::Magnitude(commands.posDelta) > 0.f)
    {
        // Convert the camera-based movement to world coordinates
        Vector3 camMovement;
        gCamera.ToWorldCoords(commands.posDelta, camMovement);

        // Only apply lock if we didn't use controller to change height
        if (commands.posDelta.Y == 0)
        {
            switch (commands.lockHeight)
            {
            case CamMod::UserInput::LOCK_MODE_LOCK_GAMEPAD_MOVE_Y:
            case CamMod::UserInput::LOCK_MODE_LOCK_ALL:
            case CamMod::UserInput::LOCK_MODE_LOCK_ALL_PLUS_HMD:
                camMovement.Y = 0.f;
                break;
            }
        }

        // Now translate the camera the calculated amount of world coordinates
        gCamera.Translate(camMovement, commands.altFunc /* target if true, source if false */);
    }

    if (Vector3::Magnitude(stateUpdate.headsetPosDelta) > 0.f)
    {
        switch (commands.hmdLockMode)
        {
            case CamMod::UserInput::HMD_LOCK_MODE_NONE:
                break;

            case CamMod::UserInput::HMD_LOCK_MODE_Y:
                stateUpdate.headsetPosDelta.Y = 0.f;
                break;

            case CamMod::UserInput::HMD_LOCK_MODE_XZ:
                stateUpdate.headsetPosDelta.X = 0.f;
                stateUpdate.headsetPosDelta.Z = 0.f;
                break;

            case CamMod::UserInput::HMD_LOCK_MODE_ALL:
                stateUpdate.headsetPosDelta = Vector3::Zero();
                break;
        }
        // Convert headset movement to Z-forward
        Vector3 headsetForwardDelta;
        GetForwardVector(stateUpdate.headsetPosDelta, stateUpdate.headsetRot, headsetForwardDelta);
        Vector3 camMovement;
        gCamera.ToWorldCoords(headsetForwardDelta, camMovement);
        gCamera.Translate(camMovement, false);
    }

    // Apply HMD + Controller rotations
    gCamera.SetRotation(stateUpdate.headsetRotEuler, commands.rotDelta );
    gCamera.SetSourceLock(commands.lockCam);

    // Set camera parameters
    gCamera.GetCamInfo(mainCamera.source, mainCamera.target, varBlock->Cameras->camRoll);

    // Apply zoom
    mainCamera.zoom = NORM_ZOOM + gZoomAmount;
    if (varBlock->clippingDist > 0.f)
    {
        mainCamera.clippingDist = varBlock->clippingDist;
    }
    else
    {
        mainCamera.clippingDist = 0.001f;
    }

    gCamSourceLock = mainCamera.source;
    gPrevStateUpdate = stateUpdate;

    varBlock->P1CpuLevel = commands.cpu1Level;
    varBlock->P2CpuLevel = commands.cpu2Level;
    varBlock->HideUI = commands.hideUI;
    switch (commands.hidePlayer)
    {
        case 1:
            varBlock->FpvHideHead = 1;
            varBlock->FpvHidePlayer = 0;
            break;

        case 2:
            varBlock->FpvHideHead = 1;
            varBlock->FpvHidePlayer = 1;
            break;

        case 3:
            varBlock->FpvHideHead = 2;
            varBlock->FpvHidePlayer = 0;
            break;

        case 4:
            varBlock->FpvHideHead = 2;
            varBlock->FpvHidePlayer = 2;
            break;

        case 5:
            varBlock->FpvHideHead = 0;
            varBlock->FpvHidePlayer = 0;
            break;

        case 0:
        default:
            break;
    }

    if( commands.altFunc &&
       ( gPrevSrcIdx != varBlock->SourceTableIdx
     || gPrevSrcTbl != varBlock->SourceTableSelect
     || gPrevTargetIdx != varBlock->TargetTableIdx
     || gPrevTargetTbl != varBlock->TargetTableSelect))
    {
        gCamera.Reset();
    }
    gPrevSrcIdx = varBlock->SourceTableIdx;
    gPrevSrcTbl = varBlock->SourceTableSelect;
    gPrevTargetIdx = varBlock->TargetTableIdx;
    gPrevTargetTbl = varBlock->TargetTableSelect;
}


void InitHmdServer()
{
    static uint32_t sFramesTillRetry = 0;
    if (!gHmdServer)
    {
        if (sFramesTillRetry == 0)
        {
            gUserInput.Init();
            //gHmdServer = new CamMod::HmdVorpx();
            gHmdServer = new CamMod::HmdServerIntf();
            if (!gHmdServer->Connect())
            {
                delete gHmdServer;
                gHmdServer = nullptr;
                sFramesTillRetry = RETRY_FRAME_CNT;
            }
        }
        else
        {
            sFramesTillRetry--;
        }
    }
}

bool GetStateUpdate(HmdServer::UpdateResponse& aState)
{
    if (nullptr == gHmdServer)
    {
        InitHmdServer();
    }

    if (gHmdServer)
    {
        if (gHmdServer->UpdateState())
        {
            aState = gHmdServer->GetLastUpdateState();
        }
        else
        {
            delete gHmdServer;
            gHmdServer = nullptr;
        }
    }
    return (gHmdServer != nullptr);
}

BOOL APIENTRY DllMain(HINSTANCE hInst     /* Library instance handle. */,
    DWORD reason        /* Reason this function is being called. */,
    LPVOID reserved     /* Not used. */)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    /* Returns TRUE on success, FALSE on failure */
    return TRUE;
}



enum TargetTableItems
{
    TARGET_CHEST = 0,
    TARGET_RIGHT_HIP = 1,
    TARGET_LEFT_HIP = 2,
    TARGET_RIGHT_FOOT = 3,
    TARGET_LEFT_FOOT = 4,
    TARGET_UNK1 = 5,  // Hip?
    TARGET_RIGHT_TOES = 6,
    TARGET_LEFT_TOES = 7,
    TARGET_RIGHT_ELBOW = 8,
    TARGET_LEFT_ELBOW = 9,
    TARGET_FACE = 10,
    TARGET_RIGHT_HAND = 11,
    TARGET_LEFT_HAND = 12,   // Between feet one match?
    TARGET_RIGHT_FINGERS = 13,
    TARGET_LEFT_FINGERS = 14,
    TARGET_UNK2 = 15, // Between feet
};

enum CharTableItems
{
    CHAR_BETWEEN_FEET = 0,
    CHAR_HIP1 = 1,
    CHAR_HIP2 = 2,
    CHAR_HIP3 = 3,
    CHAR_HIP4 = 4,
    CHAR_LEFT_KNEE = 5,
    CHAR_RIGHT_KNEE = 6,
    CHAR_LEFT_FOOT = 7,
    CHAR_RIGHT_FOOT = 8,
    CHAR_TORSO = 9,
    CHAR_TORSO_2 = 10,
    CHAR_CHEST = 11,
    CHAR_NECK = 12,
    CHAR_CHEST2 = 13,
    CHAR_CHEST_RIGHT = 14,
    CHAR_LEFT_SHOULDER = 15,
    CHAR_RIGHT_SHOULDER = 16,
    CHAR_LEFT_ELBOW = 17,
    CHAR_RIGHT_ELBOW = 18,
    CHAR_LEFT_HAND = 19,
    CHAR_RIGHT_HAND = 20,
    CHAR_LEFT_FINGERS = 21,
    CHAR_RIGHT_FINGERS = 22,
    CHAR_LEFT_FOOT2 = 23,
    CHAR_RIGHT_FOOT2 = 24,
    CHAR_HIP5 = 25,
    CHAR_BETWEEN_FEET_2 = 26,
};

bool GetPosition(VariableBlock* aVarBlock, byte aTable, byte aIdx, PointDir& aPointDir)
{
    bool ok = false;
    switch (aTable)
    {
        case TARGET_TABLE_1:
        {
            auto targetTable = aVarBlock->TargetsTablePtr[0];
            if (targetTable)
            {
                targetTable -= 10;
                aPointDir.pos = targetTable[aIdx].pos;
                aPointDir.rot = targetTable[aIdx].rot;
                ok = true;
            }
            break;
        }

        case TARGET_TABLE_2:
        {
            auto targetTable = aVarBlock->TargetsTablePtr[1];
            if (targetTable)
            {
                targetTable -= 10;
                aPointDir.pos = targetTable[aIdx].pos;
                aPointDir.rot = targetTable[aIdx].rot;
                ok = true;
            }
            break;
        }

        case CHAR_TARGET_TABLE_1:
        {
            CharTableEntry* targetTable;
            targetBaseToTargetTables(aVarBlock->TargetTableBasePtr, &targetTable, nullptr);
            if (targetTable)
            {
                aPointDir.pos = targetTable[aIdx].pos;
                aPointDir.rot = targetTable[aIdx].rot;
                ok = true;
            }
            break;
        }

        case CHAR_TARGET_TABLE_2:
        {
            CharTableEntry* targetTable;
            targetBaseToTargetTables(aVarBlock->TargetTableBasePtr, nullptr, &targetTable);
            if (targetTable)
            {
                aPointDir.pos = targetTable[aIdx].pos;
                aPointDir.rot = targetTable[aIdx].rot;
                ok = true;
            }
            break;
        }
    }
    return ok;
}


void SetBreakAndHealth(VariableBlock* aVarBlock, uint8_t aP1Health, uint8_t aP1Break, uint8_t aP2Health, uint8_t aP2Break)
{
    if (aVarBlock->playerTableFp)
    {
        auto table1 = aVarBlock->playerTableFp(0);
        auto table2 = aVarBlock->playerTableFp(1);

        if (table1)
        {
            switch (aP1Break)
            {
            case CamMod::UserInput::BREAK_MODE_NORMAL:
                break;

            case CamMod::UserInput::BREAK_MODE_INFINITE:
                table1->boost = 200;
                table1->boost_display = 200;
                break;

            case CamMod::UserInput::BREAK_MODE_EMPTY:
                table1->boost = 0;
                table1->boost_display = 0;
                break;
            }

            switch (aP1Health)
            {
            case 1:
                table1->health = 500;
                table1->health_display = 500;
                break;
            }
        }

        if (table2)
        {
            switch (aP2Break)
            {
            case CamMod::UserInput::BREAK_MODE_NORMAL:
                break;

            case CamMod::UserInput::BREAK_MODE_INFINITE:
                table2->boost = 200;
                table2->boost_display = 200;
                break;

            case CamMod::UserInput::BREAK_MODE_EMPTY:
                table2->boost = 0;
                table2->boost_display = 0;
                break;
            }

            switch (aP2Health)
            {
            case 1:
                table2->health = 500;
                table2->health_display = 500;
            }
        }
    }
}
