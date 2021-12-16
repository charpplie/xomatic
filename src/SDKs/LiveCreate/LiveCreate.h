// LiveCreate
// Copyright (c) 2009 Crytek GmbH. All Rights Reserved.

#ifndef HEADER_LiveCreate
#define HEADER_LiveCreate

// Defines

#define LC_VIRTUAL virtual

// Types

typedef signed char LCi8;
typedef signed short LCi16;
typedef signed long LCi32;

typedef unsigned char LCu8;
typedef unsigned long LCu32;
typedef unsigned short LCu16;

typedef float LCf32;
typedef double LCf64;

typedef const char* LCString;

struct LCVec2f32 { LCf32 x; LCf32 y; };
struct LCVec2f64 { LCf64 x; LCf64 y; };

struct LCVec3f32 { LCf32 x; LCf32 y; LCf32 z; };
struct LCVec3f64 { LCf64 x; LCf64 y; LCf64 z; };

struct LCVec4f32 { LCf32 x; LCf32 y; LCf32 z; LCf32 w; };
struct LCVec4f64 { LCf64 x; LCf64 y; LCf64 z; LCf64 w; };

typedef LCVec4f32 LCQuat32;
typedef LCVec4f64 LCQuat64;

// Interfaces

class ILCActor;
class ILCScene;
class ILCSystem;

class LCSet;
class LCLiveCreate;

//

class ILCActor
{
public:
	LC_VIRTUAL void SetPosition(LCString name, LCu32 time, const LCVec3f32& position) = 0;
	LC_VIRTUAL void SetOrientation(LCString name, LCu32 time, const LCQuat32& orientation) = 0;
	LC_VIRTUAL void SetScale(LCString name, LCu32 time, const LCVec3f32& scale) = 0;
};

class ILCScene
{
public:
	LC_VIRTUAL ILCActor* CreateActor(LCString name, const LCSet* pSet) = 0;
};

class ILCSystem
{
public:
	LC_VIRTUAL void Log(LCString message, ...) = 0;
};

//

class LCSet
{
public:
	LC_VIRTUAL LCu32 GetNodeCount() const = 0;
	LC_VIRTUAL LCString GetNodeName(LCu32 index) const = 0;
	LC_VIRTUAL LCi32 GetNodeParent(LCu32 index) const = 0;
};

class LCLiveCreate
{
public:
	LC_VIRTUAL void Release() = 0;

	LC_VIRTUAL bool Connect(LCString address, ILCScene& scene) = 0;
	LC_VIRTUAL void Disconnect() = 0;

	LC_VIRTUAL bool Update() = 0;
};

//

typedef LCLiveCreate* (*FLiveCreate)(ILCSystem* pSystem);

#endif // HEADER_LiveCreate
