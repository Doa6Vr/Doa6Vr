#pragma once
#include "openvr.h"
#include "Utils.hpp"
typedef unsigned char byte;

#define NORM_ZOOM 0.6195918918f

enum VectTables
{
    TARGET_TABLE_1,
    TARGET_TABLE_2,
    CHAR_TARGET_TABLE_1,
    CHAR_TARGET_TABLE_2
};

typedef byte TargetType; enum
{
    TARGET_OFF,
    TARGET_TABLE,
    TARGET_POV,
    TARGET_ORIG
};

typedef byte SourceTargetType; enum
{
    SOURCE_OFF,
    SOURCE_TABLE,
    SOURCE_ORIG
};


typedef struct PointDir_s
{
    Vector3 pos;
    Quaternion rot;
} PointDir;

typedef struct Camera_s
{
    Vector3 source;
    float unused_src;
    Vector3 target;
    float unused_tgt;
    vr::HmdVector4_t unk;
    float zoom;
    float roll;
    float clippingDist;
    float unkf2;
} Camera;


struct camQuat
{
    float w;
    float x;
    float y;
    float z;
};

typedef struct CameraArray_s
{
    Camera cams[3];
    Quaternion camRoll;
    vr::HmdVector4_t unk[4];
} CameraArray;

typedef struct CharTableEntry_s
{
    // 0x40 bytes
    Quaternion rot;
    vr::HmdVector4_t unk[2];
    Vector3 pos;
    float unused;
} CharTableEntry;


typedef struct TargetsTableEntry_s
{
    // 0xe0 bytes
    Quaternion rot;
    vr::HmdVector4_t unk[3];
    Vector3 pos;
    float unused;
    char pad_1[0x90];
} TargetTableEntry;


typedef struct PlayerBoostTableEntry_s
{
    char unk1[0x550];
    uint16_t health_display;
    uint16_t health;
    char unk2[0x1160];
    uint32_t boost_display;
    uint32_t boost;
} PlayerBoostTableEntry;


typedef PlayerBoostTableEntry*(*GetPlayerTableFp)(uint32_t aPlayerIdx);

// This block must match the CheatEngine script's variable block!
#pragma pack(push, 1)
typedef struct VariableBlock_s
{
    void* TargetTableBasePtr;
    TargetTableEntry* TargetsTablePtr[2];
    CameraArray* Cameras;
    GetPlayerTableFp playerTableFp;
    float zoom;
    float clippingDist;
    float rotation;
    float gameSpeed;
    float rotA;
    float rotB;
    float rotC;
    TargetType targetType;
    byte sourceType;
    byte SourceTableSelect;
    byte TargetTableSelect;
    byte SourceTableIdx;
    byte TargetTableIdx;
    byte FpvHideHead;
    byte FpvHidePlayer;
    byte RequestVrReset;
    byte RequestPosReset;
    byte HideUI;
    byte P1CpuLevel;
    byte P2CpuLevel;
} VariableBlock;
#pragma pack(pop)



// The structure to get to CharTablePtr
struct CT_Final
{
    char pad[0x11f0];
    CharTableEntry entry;
};

struct CT_Lvl3
{
    char pad[0x28];
    CT_Final* finalLvl;
};

struct CT_Lvl2
{
    char pad[0x10];
    CT_Lvl3* lvl3;
    char pad2[0x10];
    CT_Final* finalLvl;
};

struct CT_Lvl1
{
    CT_Lvl2* lvl2;
};


struct CT_Base
{
    char pad[0x10];
    CT_Lvl1* lvl1;
};
