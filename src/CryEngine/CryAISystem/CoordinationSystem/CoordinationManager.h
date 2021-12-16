#ifndef __CoordinationManager_h__
#define __CoordinationManager_h__

#pragma once


#include "Coordination.h"
#include "CoordinationSetup.h"
#include <ICoordinationManager.h>


class CCoordinationManager
	: public ICoordinationManager
{
public:
	CCoordinationManager();
	~CCoordinationManager();

	void ScanFolder(const char* folderName, bool recursing = false);
	bool LoadCoordinationSetupXML(const char* fileName);
	void LoadCoordinationSetup(const XmlNodeRef& rootNode, const char* fileName);

	void Reset();
	void Reload();
	void Update(float updateTime);

	void DebugDraw();
	void DebugDrawStatus();

	CoordinationSetupID GetCoordinationSetupID(const char* setupName) const;
	const char* GetCoordinationSetupName(const CoordinationSetupID& setupID) const;

	ECoordinationState GetCoordinationState(const CoordinationID& coordinationID) const;
	bool CanStart(const CoordinationSetupID& coordinationSetupID, const CoordinationInternalStartParams& params) const;
	// ICommunicationManager
	virtual CoordinationID StartCoordination(const char* pCoordinationName, const CoordinationStartParams& params);
	// ~ICommunicationManager
	CoordinationID StartCoordination(const CoordinationSetupID& coordinationSetupID, const CoordinationInternalStartParams& params);
	void StopCoordination(const CoordinationID& coordinationID);

	CoordinationScriptEnvironment GetScriptEnvironment(const CoordinationID& coordinationID) const;
	CoordinationID GetActorCoordination(const CoordinationActor& actor) const;

	void SetInputValue(const CoordinationID& coordinationID, const char* name, const ScriptAnyValue& value);
	ScriptAnyValue GetInputValue(const CoordinationID& coordinationID, const char* name);

private:
	typedef std::map<CoordinationSetupID, CoordinationSetup> CoordinationSetups;
	CoordinationSetups m_coordinationSetups;

	typedef std::vector<string> FolderNames;
	FolderNames m_folderNames;

	struct RunningCoordination
	{
		RunningCoordination(CoordinationSetup& _setup)
			: setup(_setup)
		{
		}

		CoordinationSetup& setup;
	};

	// temporary workaround until role manager is implemented
	typedef std::map<CoordinationActor, std::vector<CoordinationID> > ActorCoordinations;
	ActorCoordinations m_actorCoodinations;

	typedef std::map<CoordinationID, RunningCoordination> RunningCoordinations;
	RunningCoordinations m_runningCoordinations;

	CoordinationID m_coordinationGenId;
};


#endif //__CoordinationManager_h__