////////////////////////////////////////////////////////////////////
//
// Crysis2.spa.h
//
// Auto-generated on Monday, 22 February 2010 at 15:41:13
// XLAST project version 1.0.40.0
// SPA Compiler version 2.0.9328.0
//
////////////////////////////////////////////////////////////////////

#ifndef __CRYSIS_2_SPA_H__
#define __CRYSIS_2_SPA_H__

#ifdef __cplusplus
extern "C" {
#endif

	//
	// Title info
	//

#define TITLEID_CRYSIS_2                            0x454108E3

	//
	// Context ids
	//
	// These values are passed as the dwContextId to XUserSetContext.
	//


	//
	// Context values
	//
	// These values are passed as the dwContextValue to XUserSetContext.
	//

	// Values for X_CONTEXT_PRESENCE

#define CONTEXT_PRESENCE_TEST                       0

	// Values for X_CONTEXT_GAME_MODE

#define CONTEXT_GAME_MODE_CTF                       0

	//
	// Property ids
	//
	// These values are passed as the dwPropertyId value to XUserSetProperty
	// and as the dwPropertyId value in the XUSER_PROPERTY structure.
	//

#define PROPERTY_MAP                                0x10000002
#define PROPERTY_SESSION_DATA_I32_0                 0x10000003
#define PROPERTY_SESSION_DATA_I32_1                 0x10000004
#define PROPERTY_SESSION_DATA_I32_2                 0x10000005
#define PROPERTY_SESSION_DATA_I32_4                 0x10000006
#define PROPERTY_SESSION_DATA_I32_5                 0x10000007
#define PROPERTY_SESSION_DATA_I32_6                 0x10000008
#define PROPERTY_SESSION_DATA_I32_7                 0x10000009
#define PROPERTY_SESSION_DATA_I32_3                 0x1000001A
#define PROPERTY_SCORE                              0x20000001
#define PROPERTY_SESSION_DATA_I64_0                 0x2000000A
#define PROPERTY_SESSION_DATA_I64_1                 0x2000000B
#define PROPERTY_SESSION_DATA_I64_2                 0x2000000C
#define PROPERTY_SESSION_DATA_I64_3                 0x2000000D
#define PROPERTY_SESSION_DATA_I64_4                 0x2000000E
#define PROPERTY_SESSION_DATA_I64_5                 0x2000000F
#define PROPERTY_SESSION_DATA_I64_6                 0x20000010
#define PROPERTY_SESSION_DATA_I64_7                 0x20000011
#define PROPERTY_SESSION_DATA_I64_8                 0x20000012
#define PROPERTY_SESSION_DATA_I64_9                 0x20000013
#define PROPERTY_SESSION_DATA_I64_10                0x20000014
#define PROPERTY_SESSION_DATA_I64_11                0x20000015
#define PROPERTY_SESSION_DATA_I64_12                0x20000016
#define PROPERTY_SESSION_DATA_I64_13                0x20000017
#define PROPERTY_SESSION_DATA_I64_14                0x20000018
#define PROPERTY_SESSION_DATA_I64_15                0x20000019
#define PROPERTY_XP_ARMOUR                          0x2000001C
#define PROPERTY_XP_TACTICAL                        0x2000001D
#define PROPERTY_XP_POWER                           0x2000001E
#define PROPERTY_XP_STEALTH                         0x2000001F

	//
	// Achievement ids
	//
	// These values are used in the dwAchievementId member of the
	// XUSER_ACHIEVEMENT structure that is used with
	// XUserWriteAchievements and XUserCreateAchievementEnumerator.
	//

#define ACHIEVEMENT_ACHIEVEMENT1                    2
#define ACHIEVEMENT_ACHIEVEMENT2                    3
#define ACHIEVEMENT_ACHIEVEMENT3                    4
#define ACHIEVEMENT_ACHIEVEMENT4                    5
#define ACHIEVEMENT_ACHIEVEMENT5                    6

	//
	// AvatarAssetAward ids
	//


	//
	// Stats view ids
	//
	// These are used in the dwViewId member of the XUSER_STATS_SPEC structure
	// passed to the XUserReadStats* and XUserCreateStatsEnumerator* functions.
	//

	// Skill leaderboards for ranked game modes

#define STATS_VIEW_SKILL_RANKED_CTF                 0xFFFF0000

	// Skill leaderboards for unranked (standard) game modes

#define STATS_VIEW_SKILL_STANDARD_CTF               0xFFFE0000

	// Title defined leaderboards

#define STATS_VIEW_TEST                             1
#define STATS_VIEW_XP                               2
#define STATS_VIEW_KILLS                            3
#define STATS_VIEW_KILLDEATHRATIO                   4
#define STATS_VIEW_ACCURACY                         5
#define STATS_VIEW_KILLSTREAK                       6

	//
	// Stats view column ids
	//
	// These ids are used to read columns of stats views.  They are specified in
	// the rgwColumnIds array of the XUSER_STATS_SPEC structure.  Rank, rating
	// and gamertag are not retrieved as custom columns and so are not included
	// in the following definitions.  They can be retrieved from each row's
	// header (e.g., pStatsResults->pViews[x].pRows[y].dwRank, etc.).
	//

	// Column ids for TEST


	// Column ids for XP

#define STATS_COLUMN_XP_XP_POWER                    2
#define STATS_COLUMN_XP_XP_ARMOUR                   1
#define STATS_COLUMN_XP_XP_STEALTH                  3
#define STATS_COLUMN_XP_XP_TACTICAL                 4

	// Column ids for KILLS


	// Column ids for KILLDEATHRATIO


	// Column ids for ACCURACY


	// Column ids for KILLSTREAK


	//
	// Matchmaking queries
	//
	// These values are passed as the dwProcedureIndex parameter to
	// XSessionSearch to indicate which matchmaking query to run.
	//

#define SESSION_MATCH_QUERY_TEST                    0

	//
	// Gamer pictures
	//
	// These ids are passed as the dwPictureId parameter to XUserAwardGamerTile.
	//



#ifdef __cplusplus
}
#endif

#endif // __CRYSIS_2_SPA_H__


