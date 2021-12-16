#include "StdAfx.h"
#include "CoordinationManager.h"
#include "CoordinationSetup.h"
#include "CoordinationPlan.h"
#include "CoordinationElement.h"

#include <StringUtils.h>

#include "DebugDrawContext.h"


//--------------------------------------------------------------------------------

CoordinationDebugDrawContext::CoordinationDebugDrawContext(float left, float top, float _fontSize, float _lineHeight, 
																													 float _identWidth, CDebugDrawContext& _dc)
: x(left)
, y(top)
, ident(0)
, fontSize(_fontSize)
, lineHeight(_lineHeight)
, identWidth(_identWidth)
, dc(_dc)
{
}

IAIDebugRenderer* CoordinationDebugDrawContext::operator->() const
{
	return dc.operator ->();
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

CCoordinationManager::CCoordinationManager()
: m_coordinationGenId(0)
{
}

CCoordinationManager::~CCoordinationManager()
{
}

void CCoordinationManager::ScanFolder(const char* folderName, bool recursing)
{
	string folder(PathUtil::MakeGamePath(folderName));
	folder += "/";

	string searchString(folder + "*.xml");
	
	_finddata_t fd;
	intptr_t handle = 0;

	ICryPak *pPak = gEnv->pCryPak;
	handle = pPak->FindFirst(searchString.c_str(), &fd);

	if (handle > -1)
	{
		do
		{
			if (!strcmp(fd.name, ".") || !strcmp(fd.name, ".."))
				continue;

			if (fd.attrib & _A_SUBDIR)
				ScanFolder(folder + fd.name, true);
			else
				LoadCoordinationSetupXML(folder + fd.name);

		} while (pPak->FindNext(handle, &fd) >= 0);

		pPak->FindClose(handle);
	}

	if (!recursing)
		stl::push_back_unique(m_folderNames, folderName);
}

bool CCoordinationManager::LoadCoordinationSetupXML(const char* fileName)
{
	XmlNodeRef rootNode = GetISystem()->LoadXmlFile(fileName);
	if (!rootNode)
	{
		AIWarning("Failed to open XML file '%s'...", fileName);

		return false;
	}

	const char* tagName = rootNode->getTag();

	if (!stricmp(tagName, "Coordination"))
	{
		LoadCoordinationSetup(rootNode, fileName);

		return true;
	}
	else if (!stricmp(tagName, "Coordinations"))
	{
		int childCount = rootNode->getChildCount();
		
		for (int i = 0; i < childCount; ++i)
		{
			XmlNodeRef childNode = rootNode->getChild(i);
			if (!stricmp(childNode->getTag(), "Coordination"))
			{
				LoadCoordinationSetup(childNode, fileName);
			}
			else
			{
				AIWarning(
					"Unexpected tag '%s' found at line %d'while parsing Coordination XML '%s'...",
					childNode->getTag(), childNode->getLine(), fileName);
			}
		}
	}
	else
	{
		AIWarning(
			"Unexpected tag '%s' found at line %d while parsing Coordination XML '%s'...",
			rootNode->getTag(), rootNode->getLine(), fileName);
	}

	return false;
}

void CCoordinationManager::LoadCoordinationSetup(const XmlNodeRef& rootNode, const char* fileName)
{
	if (!rootNode->haveAttr("name"))
	{
		AIWarning("Missing 'name' attribute for 'Coordination' tag at line %d...", 
			rootNode->getLine());

		return;
	}

	const char* name;
	rootNode->getAttr("name", &name);

	std::pair<CoordinationSetups::iterator, bool> iresult = 
		m_coordinationSetups.insert(CoordinationSetups::value_type(GetCoordinationSetupID(name), CoordinationSetup()));

	if (!iresult.second)
	{
		if (stricmp(iresult.first->second.GetName(), name))
		{
			AIWarning("Hash collision for Coordination '%s' and '%s' at line %d...",
				iresult.first->second.GetName(), name, rootNode->getLine());
			
			return;
		}
		else
		{
			AIWarning("Duplicate Coordination '%s' at line %d...", name, rootNode->getLine());
			
			return;
		}
	}

	CoordinationSetup& coordinationSetup = iresult.first->second;

	if (!coordinationSetup.LoadFromXML(rootNode, fileName))
		m_coordinationSetups.erase(iresult.first);
}

void CCoordinationManager::Reset()
{
	m_actorCoodinations.clear();

	while (!m_runningCoordinations.empty())
		StopCoordination(m_runningCoordinations.begin()->first);
}

void CCoordinationManager::Reload()
{
	Reset();

	m_coordinationSetups.clear();

	for (int i = 0; i < m_folderNames.size(); ++i)
		ScanFolder(m_folderNames[i].c_str());
}

void CCoordinationManager::Update(float updateTime)
{
	FUNCTION_PROFILER( GetISystem(), PROFILE_AI );

	CoordinationSetups::iterator it = m_coordinationSetups.begin();
	CoordinationSetups::iterator end = m_coordinationSetups.end();
	
	for ( ; it != end; ++it)
		it->second.Update(updateTime);

	RunningCoordinations::iterator rit = m_runningCoordinations.begin();
	RunningCoordinations::iterator rend = m_runningCoordinations.end();

	for ( ; rit != rend; )
	{
		const CoordinationID& coordinationID = rit->first;
		RunningCoordination& running = rit->second;

		ECoordinationState state = running.setup.GetState(coordinationID);
		if ((state != CoordinationRunning) && (state != CoordinationWaiting)) // maybe later we will have waiting coordination
		{
			running.setup.Stop(coordinationID);
			
			RunningCoordinations::iterator erased = rit++;
			m_runningCoordinations.erase(erased);

			continue;
		}
		++rit;
	}
}

void CCoordinationManager::DebugDraw()
{
}

void CCoordinationManager::DebugDrawStatus()
{
	CDebugDrawContext dc;
	
	CoordinationDebugDrawContext context(10.0f, 30.0f, 1.25f, 11.0f * 1.25f, 15.0f, dc);

	RunningCoordinations::iterator rit = m_runningCoordinations.begin();
	RunningCoordinations::iterator rend = m_runningCoordinations.end();

	for ( ; rit != rend; ++rit)
	{
		const CoordinationID& coordinationID = rit->first;
		RunningCoordination& running = rit->second;

		running.setup.DebugDrawStatus(coordinationID, context);
	}
}

CoordinationSetupID CCoordinationManager::GetCoordinationSetupID(const char* setupName) const
{
	return CryStringUtils::CalculateHashLowerCase(setupName);
}

const char* CCoordinationManager::GetCoordinationSetupName(const CoordinationSetupID& setupID) const
{
	CoordinationSetups::const_iterator it = m_coordinationSetups.find(setupID);
	if (it != m_coordinationSetups.end())
	{
		const CoordinationSetup& setup = it->second;

		return setup.GetName();
	}

	return "<null>";
}


ECoordinationState CCoordinationManager::GetCoordinationState(const CoordinationID& coordinationID) const
{
	RunningCoordinations::const_iterator it = m_runningCoordinations.find(coordinationID);
	if (it == m_runningCoordinations.end())
		return CoordinationNotRunning;

	const RunningCoordination& running = it->second;
	return running.setup.GetState(coordinationID);
}

bool CCoordinationManager::CanStart(const CoordinationSetupID& coordinationSetupID,
																		const CoordinationInternalStartParams& params) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CoordinationSetups::const_iterator it = m_coordinationSetups.find(coordinationSetupID);
	if (it != m_coordinationSetups.end())
	{
		const CoordinationSetup& setup = it->second;

		return setup.CanStart(0, params);
	}

	return false;
}

CoordinationID CCoordinationManager::StartCoordination(const char* pCoordinationName, const CoordinationStartParams& params)
{
	CoordinationInternalStartParams internalParams;
	internalParams.inputs = params.inputs;
	internalParams.actors.resize(params.actorCount);
	std::copy(params.actors, params.actors + params.actorCount, internalParams.actors.begin());
	CoordinationSetupID coordinationSetupID = gAIEnv.pCoordinationManager->GetCoordinationSetupID(pCoordinationName);
	return StartCoordination(coordinationSetupID, internalParams);
}

CoordinationID CCoordinationManager::StartCoordination(const CoordinationSetupID& coordinationSetupID, const CoordinationInternalStartParams& params)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

	CoordinationSetups::iterator it = m_coordinationSetups.find(coordinationSetupID);
	if (it != m_coordinationSetups.end())
	{
		++m_coordinationGenId;
		if (!m_coordinationGenId)
			++m_coordinationGenId;

		CoordinationSetup& setup = it->second;
		if (setup.CanStart(m_coordinationGenId, params))
		{
			RunningCoordination& running = 
				stl::map_insert_or_get(m_runningCoordinations, m_coordinationGenId, RunningCoordination(setup));

			CoordinationActors::const_iterator ait = params.actors.begin();
			CoordinationActors::const_iterator aend = params.actors.end();

			for ( ; ait != aend; ++ait)
			{
				std::vector<CoordinationID>& coords = stl::map_insert_or_get(m_actorCoodinations, *ait, std::vector<CoordinationID>());
				coords.push_back(CoordinationID(m_coordinationGenId));
			}

			setup.Start(m_coordinationGenId, params);

			return m_coordinationGenId;
		}
	}

	return CoordinationID(0);
}

void CCoordinationManager::StopCoordination(const CoordinationID& coordinationID)
{
	RunningCoordinations::iterator it = m_runningCoordinations.find(coordinationID);
	if (it == m_runningCoordinations.end())
		return;

	RunningCoordination& running = it->second;
	running.setup.Stop(coordinationID);

	// bad
	ActorCoordinations::iterator ait = m_actorCoodinations.begin();
	ActorCoordinations::iterator aend = m_actorCoodinations.end();

	for ( ; ait != aend; )
	{
		std::vector<CoordinationID>& coords = ait->second;

		if (coords.back() == coordinationID)
		{
			coords.pop_back();

			if (coords.empty())
			{
				ActorCoordinations::iterator erased = ait++;
				m_actorCoodinations.erase(erased);

				continue;
			}
		}

		assert(std::find(coords.begin(), coords.end(), coordinationID) == coords.end());

		++ait;
	}
	
	m_runningCoordinations.erase(it);
}

CoordinationScriptEnvironment CCoordinationManager::GetScriptEnvironment(const CoordinationID& coordinationID) const
{
	RunningCoordinations::const_iterator it = m_runningCoordinations.find(coordinationID);
	if (it == m_runningCoordinations.end())
		return CoordinationScriptEnvironment();

	const RunningCoordination& running = it->second;
	return running.setup.GetScriptEnvironment(coordinationID);
}

CoordinationID CCoordinationManager::GetActorCoordination(const CoordinationActor& actor) const
{
	ActorCoordinations::const_iterator it = m_actorCoodinations.find(actor);
	if (it == m_actorCoodinations.end())
		return CoordinationID(0);

	return it->second.back();
}

void CCoordinationManager::SetInputValue(const CoordinationID& coordinationID, const char* name, const ScriptAnyValue& value)
{
	RunningCoordinations::iterator it = m_runningCoordinations.find(coordinationID);
	if (it == m_runningCoordinations.end())
		return;

	RunningCoordination& running = it->second;

	running.setup.SetInputValue(coordinationID, name, value);
}

ScriptAnyValue CCoordinationManager::GetInputValue(const CoordinationID& coordinationID, const char* name)
{
	RunningCoordinations::iterator it = m_runningCoordinations.find(coordinationID);
	if (it == m_runningCoordinations.end())
		return ScriptAnyValue();

	RunningCoordination& running = it->second;

	ScriptAnyValue value;
	running.setup.GetInputValue(coordinationID, name, value);

	return value;
}
