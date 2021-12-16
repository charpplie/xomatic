#ifndef __HUDDEFINES_H__
#define __HUDDEFINES_H__

// Enable/disable extra HUD debug.
#if !defined(PROFILE) && !defined(_RELEASE)
#define ENABLE_HUD_EXTRA_DEBUG 1
#else
#define ENABLE_HUD_EXTRA_DEBUG 0
#endif

// Nightly HUD TRC tests.
#if !defined(_RELEASE)
#define ENABLE_HUD_TESTS 1
#else
#define ENABLE_HUD_TESTS 0
#endif

// Register here for extra HUD debug messages.
#if (ENABLE_HUD_EXTRA_DEBUG || ENABLE_HUD_TESTS) && ( defined(USER_frankh) || defined(USER_jann) )
#define IS_HUD_DEV 1
#else
#define IS_HUD_DEV 0
#endif

// HUD Developer get warnings as asserts and warnings.
#if IS_HUD_DEV
#define CryHUDWarning( a, b, msg ) CRY_ASSERT_MESSAGE(0,msg); CryWarning( a, b, msg );
#else
#define CryHUDWarning( a, b, msg ) CryWarning( a, b, msg );
#endif

//////////////////////////////////////////////////////////////////////////

// Utility define for unregistering a FlashVariableObject
//                        IHUDAsset*, char*, IFlashVariableObject*
#define HUD_FLASHOBJ_SAFERELEASE( a )       \
	if(a)                                     \
	{                                         \
		a->Release();                           \
		a = NULL;                               \
	}              

// Utility for registering a FlashVariableObject
//                          IHUDAsset*, char*, IFlashVariableObject*
#define HUD_FLASVAROBJ_REG(  pHudAsset,  path, flashObj )                \
	if( pHudAsset == CHUD::s_pHUDAssetNULL )                               \
	{                                                                      \
		CryHUDWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: Attempting to access data from CHUD::s_pHUDAssetNULL. Path '%s' for '" #flashObj "'", path).c_str() ); \
	}                                                                      \
	else                                                                   \
	{                                                                      \
		HUD_FLASHOBJ_SAFERELEASE( flashObj );                                \
		pHudAsset->GetVariable( path, flashObj );                            \
		if( flashObj == CHUD::s_pHUDFlashvariableObjectNULL )                \
		{                                                                    \
			CryHUDWarning( VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, string().Format("HUD: Failed to alloc on '%s' for '" #flashObj "'", path).c_str() ); \
		}                                                                    \
	}


// Util for registering a FlashVariableObject and associated display info.
//                             IHUDAsset*, char*, IFlashVariableObject*, SFlashDisplayInfo )
#define HUD_FLASVAROBJ_REG_DI(  pHudAsset,  path,              flashObj,        displayInfo) \
	HUD_FLASVAROBJ_REG(  pHudAsset,  path, flashObj )                                          \
	flashObj->GetDisplayInfo( displayInfo );


//-----------------------------------------

ILINE const string HUDGetVictimName( EntityId id  )
{
	if( IEntity *v = gEnv->pEntitySystem->GetEntity( id ) )
	{
		return v->GetName();
	}

	CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_WARNING, "Failed to get Victim name for Entity #%d", (int)id );
	return "Unknown";
}
#define OBJECTIVES_MIN_PIXEL_FROM_EDGE 60.0f

#endif // __HUDDEFINES_H__
