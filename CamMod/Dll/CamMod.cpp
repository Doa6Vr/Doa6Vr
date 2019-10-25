#include "CamMod.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "HmdServerIntf.hpp"
#include "HmdVorpx.hpp"
#include "UserInput.hpp"
#include "Utils.hpp"

#define DllExport   __declspec( dllexport )

#include <sstream>
#include <iomanip>

#include <cmath>

#define RETRY_FRAME_CNT 200
CamMod::IHmdServerIntf* gHmdServer = nullptr;
CamMod::UserInput gUserInput;
Vector3 gSourceRotAmount;
Vector3 gSourcePosAmount;
Vector3 gTargetPosAmount;
float gZoomAmount;
float gRollAmount;

Vector3 gCamSourceLock;

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

void GetCameraPitchYaw(const Vector3& aVect, float& aPitch, float& aYaw)
{
    // Determine the pitch/yaw of the vector from Right
    Vector3 xyProject = Vector3::ProjectOnPlane(aVect, Vector3::Up());
    aPitch = Vector3::Angle(aVect, xyProject);
    aYaw = Vector3::Angle(Vector3::Right(), xyProject);

    // If Z is below the Right axis then flip
    if (xyProject.Z < 0.f)
    {
        aYaw = (2.f * M_PI_F - aYaw);
    }
    aPitch += M_PI_F / 2; // Default is looking straight up, so fix it
    if (aVect.Y > 0.f)
    {
        aPitch = (M_PI_F - aPitch);
    }
}

void InitHmdServer();
bool GetStateUpdate(HmdServer::UpdateResponse& aState);

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
    PointDir source;
    PointDir target;
    Camera& mainCamera = varBlock->Cameras->cams[0];
    HmdServer::UpdateResponse stateUpdate = {};
    CamMod::UserInput::UserCommands commands = {};
    bool targetUseDeltas = false;
    bool sourceUseDeltas = false;

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


        //std::stringstream dbg;
        //dbg << "HeadsetRot:  ";
        //printVectDeg(dbg, stateUpdate.headsetRotEuler);
        //dbg << "-----" << std::endl;
        //OutputDebugString(dbg.str().c_str());
    }

    if (gUserInput.GetInput(commands))
    {
        if (commands.altFunc)
        {
            gTargetPosAmount += commands.posDelta;
        }
        else
        {
            gSourcePosAmount += commands.posDelta;
            gSourceRotAmount += commands.rotDelta;
        }

        gZoomAmount += commands.zoom;
        gRollAmount += commands.rotDelta.roll;

        if (commands.resetPos)
        {
            gSourceRotAmount = Vector3::Zero();
            gSourcePosAmount = Vector3::Zero();
            gTargetPosAmount = Vector3::Zero();
            gZoomAmount = 0.f;
            gRollAmount = 0.f;
            varBlock->Cameras->camRoll = sOrigCamRoll;
            if (gHmdServer)
            {
                gHmdServer->ResetPos();
                gHmdServer->ResetVr();
            }
        }

    }


    if(varBlock->playerTableFp )
    {
        auto table1 = varBlock->playerTableFp(0);
        auto table2 = varBlock->playerTableFp(1);

        if (table1)
        {
            switch (commands.fullBoost[0])
            {
            case 0:
                break;

            case 1:
                table1->boost = 200;
                table1->boost_display = 200;
                break;

            case 2:
                table1->boost = 0;
                break;
            }

            switch (commands.fullHealth[0])
            {
            case 1:
                table1->health = 500;
                table1->health_display = 500;
            }
        }

        if (table2)
        {
            switch (commands.fullBoost[1])
            {
            case 0:
                break;

            case 1:
                table2->boost = 200;
                table2->boost_display = 200;
                break;

            case 2:
                table2->boost = 0;
                break;
            }

            switch (commands.fullHealth[1])
            {
            case 1:
                table2->health = 500;
                table2->health_display = 500;
            }
        }
    }

    uint8_t enableSource = varBlock->enableSource;
    uint8_t targetType = varBlock->targetType;

    switch (commands.freeRoam)
    {
    case 0: // nothing
        break;

    case 1: // no source
        enableSource = false;
        targetType = 1;
        break;

    case 2: // no source no target
        enableSource = false;
        targetType = 0;
        break;

    case 3: // source, no target
        stateUpdate.headsetPosDelta = Vector3::Zero();
        stateUpdate.headsetRotEulerDelta = Vector3::Zero();
        break;
    }
    
    if (commands.lockPitch)
    {
        stateUpdate.headsetRotEuler.pitch = 0.f;
        stateUpdate.headsetRotEuler.roll = 0.f;
    }

    if (enableSource)
    {
        byte sourceIdx = varBlock->SourceTableIdx;
        switch (varBlock->SourceTableSelect)
        {
            case TARGET_TABLE_1:
            {
                auto targetTable = varBlock->TargetsTablePtr[0];
                if (targetTable)
                {
                    source.pos = targetTable[sourceIdx].pos;
                    source.rot = targetTable[sourceIdx].rot;
                }
                else
                {
                    source.pos = mainCamera.source;
                }

                break;
            }

            case TARGET_TABLE_2:
            {
                auto targetTable = varBlock->TargetsTablePtr[1];
                if (targetTable)
                {
                    source.pos = targetTable[sourceIdx].pos;
                    source.rot = targetTable[sourceIdx].rot;
                }
                else
                {
                    source.pos = mainCamera.source;
                }
                break;
            }

            case CHAR_TARGET_TABLE_1:
            {
                CharTableEntry* targetTable;
                targetBaseToTargetTables(varBlock->TargetTableBasePtr, &targetTable, nullptr);
                if (targetTable)
                {
                    source.pos = targetTable[sourceIdx].pos;
                    source.rot = targetTable[sourceIdx].rot;
                }
                else
                {
                    source.pos = mainCamera.source;
                }
                break;
            }

            case CHAR_TARGET_TABLE_2:
            {
                CharTableEntry* targetTable;
                targetBaseToTargetTables(varBlock->TargetTableBasePtr, nullptr, &targetTable );

                if (targetTable)
                {
                    source.pos = targetTable[sourceIdx].pos;
                    source.rot = targetTable[sourceIdx].rot;
                }
                else
                {
                    source.pos = mainCamera.source;
                }
                break;
            }

            default:
                source.pos = mainCamera.source;
                break;
        }

        mainCamera.source = source.pos;
    }
    else
    {
        sourceUseDeltas = true;
    }


    // Handle targets
    if (targetType == TARGET_POV && enableSource)
    {
        Vector3 adjVector;
        rotate_vector_by_quaternion(Vector3::Right(), source.rot, adjVector);
        mainCamera.target = source.pos + ( 10 * adjVector );
    }
    else if (targetType == TARGET_TABLE)
    {
        byte targetIdx = varBlock->TargetTableIdx;
        switch (varBlock->TargetTableSelect)
        {
        case TARGET_TABLE_1:
        {
            auto tableEntry = &(varBlock->TargetsTablePtr[0])[targetIdx];
            if (tableEntry)
            {
                target.pos = tableEntry->pos;
                target.rot = tableEntry->rot;
            }
            break;
        }

        case TARGET_TABLE_2:
        {
            auto tableEntry = &(varBlock->TargetsTablePtr[1])[targetIdx];
            if (tableEntry)
            {
                target.pos = tableEntry->pos;
                target.rot = tableEntry->rot;
            }
            break;
        }

        case CHAR_TARGET_TABLE_1:
        {
            CharTableEntry* targetTable;
            targetBaseToTargetTables(varBlock->TargetTableBasePtr, &targetTable, nullptr);
            auto tableEntry = &targetTable[targetIdx];
            if (tableEntry)
            {
                target.pos = tableEntry->pos;
                target.rot = tableEntry->rot;
            }
            break;
        }

        case CHAR_TARGET_TABLE_2:
        {
            CharTableEntry* targetTable;
            targetBaseToTargetTables(varBlock->TargetTableBasePtr, nullptr, &targetTable);
            auto tableEntry = &targetTable[targetIdx];
            if (tableEntry)
            {
                target.pos = tableEntry->pos;
                target.rot = tableEntry->rot;
            }
            break;
        }

        default:
            target.pos = mainCamera.target;
            break;

        }

        mainCamera.target = target.pos;
    }
    else
    {
        targetUseDeltas = true; // If we didn't set the target then only apply deltas to it.
    }



    if (commands.lockCam)
    {
        mainCamera.source = gCamSourceLock;
    }

    // Apply position offsets
    if( true )
    {
        // Convert headset movement to 'forward' being Z
        Vector3 headsetMovement = -stateUpdate.headsetPosDelta;
        Quaternion rotQuat = stateUpdate.headsetRot;
        rotQuat.W = -rotQuat.W;
        Vector3 headsetForward;
        rotate_vector_by_quaternion(headsetMovement, rotQuat, headsetForward);


        Vector3 camForward = Vector3::Normalized(mainCamera.target - mainCamera.source);
        Vector3 camUp;
        Vector3 camRight;
        GetLocalOrthsNoRoll(camForward, camRight, camUp);


        Vector3 moveOffset = Vector3::Zero();
        if (sourceUseDeltas)
        {
            Vector3 adjVect = commands.posDelta + headsetForward;
            moveOffset = (adjVect.X * camRight) + (adjVect.Y * camUp) + (adjVect.Z * camForward);
        }
        else
        {
            gSourcePosAmount += headsetForward;
            Vector3 adjVect = gSourcePosAmount;
            moveOffset = (adjVect.X * camRight) + (adjVect.Y * camUp) + (adjVect.Z * camForward);
        }

        if (commands.lockHeight)
        {
            moveOffset.Y = 0;
        }
        mainCamera.source += moveOffset;
        mainCamera.target += moveOffset;
    }


    // Apply HMD + Controller rotations
    {
        Vector3 camVect = mainCamera.target - mainCamera.source;
        Vector3 camRight;
        Vector3 camUp;
        GetLocalOrthsNoRoll(camVect, camRight, camUp);

        // Figure out total pitch/yaw adjustments
        float yawAdj;
        float pitchAdj;
        if(targetUseDeltas)
        {
            yawAdj = stateUpdate.headsetRotEulerDelta.yaw + commands.rotDelta.yaw;
            pitchAdj = stateUpdate.headsetRotEulerDelta.pitch + commands.rotDelta.pitch;
        }
        else
        {
            yawAdj = stateUpdate.headsetRotEuler.yaw + gSourceRotAmount.yaw;
            pitchAdj = -(stateUpdate.headsetRotEuler.pitch + gSourceRotAmount.pitch);
        }

        // Create an adjustment vector
        Quaternion pitchQuat = Quaternion::FromAngleAxis(pitchAdj, camRight);
        Quaternion yawQuat = Quaternion::FromAngleAxis(yawAdj, camUp);
        Quaternion rotQuat = yawQuat*pitchQuat;
        Vector3 adjVector;
        if (!isnan(rotQuat.W))
        {
            rotate_vector_by_quaternion(camVect, rotQuat, adjVector);
        }
        else
        {
            adjVector = Vector3::Zero();
        }

        if ( false && commands.lockPitch)
        {
            adjVector.Y = 0;
        }

        if(targetUseDeltas) 
        {
            mainCamera.target = mainCamera.source + adjVector;
        }
        else
        {
            mainCamera.target += 100 * adjVector;
        }
    }

    // Apply total screen roll
    {
        Vector3 camForward = Vector3::Normalized(mainCamera.target - mainCamera.source);
        float rollTheta = gRollAmount + stateUpdate.headsetRotEuler.roll;
        rollTheta *= 2;
        Quaternion fromEuler = Quaternion::FromAngleAxis(rollTheta, camForward);
        varBlock->Cameras->camRoll = fromEuler * sOrigCamRoll;

    }

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
