/********************************************************************
StalTech Source File.
Copyright (C), StalTech Studios, 2006-2007.
---------------------------------------------------------------------
File name:   TacticInfo.h
$Id$
$DateTime$
Description: Read-only configuration storage attached to each subtactic.
It just contains all the data need by a subtatic to run, like min and max
resources allowed.
---------------------------------------------------------------------
History:
- 12:07:2007 : Created by Ricardo Pillosu

*********************************************************************/

#ifndef _TACTIC_INFO_H_
#define _TACTIC_INFO_H_

enum EBehaviorWhenNoEnoughResources
{
	EBWNER_INVALID = -1,
	EBWNER_DESTROY_TACTIC,
	EBWNER_COUNT
};

class CTacticInfo;

struct SSubTacticRequirement
{
	uint32 uMinResourcesToStart;
	uint32 uMinResources;
	uint32 uMaxResources;
	string sQSortCallback;

	SSubTacticRequirement();
};
typedef std::map<uint32, SSubTacticRequirement> TSubTacticRequirements;

struct SSubTacticInfo
{
	CTacticInfo const* pInfo;
	string sName;
	TSubTacticRequirements Requirements;

	SSubTacticInfo (const string&sName, CTacticInfo const* pInfo);
};

class CTacticInfo
{
	typedef std::vector<uint32> TBTProfileReferences;
	typedef std::vector<SSubTacticInfo*> TSubTacticVector;

public:

	/*$1- Basics -------------------------------------------------------------*/
	CTacticInfo( const char* sName );
	~CTacticInfo();

	/*$1- Util ---------------------------------------------------------------*/
	const string& GetName() const;
	const SSubTacticInfo* GetSubTacticInfo( const string& sName ) const;
	const TBTProfileReferences& GetBTProfileReferences() const;
	const bool UsesGroup() const;
	const bool ExceedsMaxGroupInstances( uint32 uNumInstances ) const;

private:
	bool LoadCell_Tactic( uint32 uCell, const string& sData );
	bool LoadCell_Requirement( uint32 uCell, const string& sData );

	void FindOrCreateSubInfo( const string& sName, SSubTacticInfo *& pOutSubTacticInfo );

protected:
	friend class CTacticsManager;
	friend class CTactic;

	/*$1- Members ------------------------------------------------------------*/
	bool LoadCell( uint32 uCell, const string& sData, bool bIsRequirements );

	/*$1- Members ------------------------------------------------------------*/
	bool m_bUsesGroup;
	uint32 m_uMaxInstances;
	uint32 m_uMaxGroupInstances;
	uint32 m_uLastBTProfileId;
	EBehaviorWhenNoEnoughResources m_eDestructor;
	SSubTacticInfo* m_pLastSubInfo;
	string m_sName;
	TSubTacticVector m_vecSubTacticsInfo;
	TBTProfileReferences m_vecBTProfileReferences;
};

#endif // _TACTIC_INFO_H_
