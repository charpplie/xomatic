#include "StdAfx.h"
#include "HUDTester.h"

#include "HUD/HUD.h"

#if ENABLE_HUD_TESTS

#include "HUD/HUDState.h"
#include "HUD/HUDStateManager.h"
#include "HUD/HUDObjectManager.h"
#include "HUD/HUDCVars.h"

#include "Testing/AutoTester.h"

#include "Game.h"
#include "GameCVars.h"
#include "IRenderer.h"

int HUDTester::s_running = 0;

HUDTester::HUDTester(CHUDStateManager* pStateManager, CHUDObjectManager* pObjectManager)
: m_outputPath(""),
m_numTestsuites(0),
m_numFailedTestsuites(0),
m_numTestcases(0),
m_numFailedTestcases(0),
m_stateManager(pStateManager),
m_objectManager(pObjectManager),
m_curStateId(0),
m_settleDownTimer(static_cast<float>(g_pGame->GetHUD()->GetCVars()->hud_test_timeBetweenTestScreenShots))
{		
	m_resolutions.reserve(5);
}

HUDTester::~HUDTester()
{
	// Quit after test for auto testing
	if(g_pGame->GetHUD()->GetCVars()->hud_quitAfterTest)
	{
		gEnv->pConsole->ExecuteString("quit");
	}
}

void HUDTester::Start(const char* outputPath)
{
	// set the global multi-player to true so we 
	// get all mp HUD (sp may need to modify this!)
	m_prevMultiplayerState = gEnv->bMultiplayer;
	gEnv->bMultiplayer = (0 != g_pGame->GetCVars()->g_multiplayerDefault);

	g_pGame->GetHUD()->GetCVars()->hud_debugDrawAssets = 1;
	g_pGame->GetHUD()->GetCVars()->hud_drawSafeAreas = 1;
	g_pGame->GetHUD()->GetCVars()->hud_debugActiveStates = 1;
	g_pGame->GetHUD()->GetCVars()->hud_hideMenus = 1;

	HUDTester::s_running = 1;

	m_safeAreas["eHSAID_360_tcr"] = eHSAID_360_tcr;
	if( g_pGame->GetHUD()->GetCVars()->hud_test_iterateSafeAreas )
	{
		m_safeAreas["eHSAID_fullscreen"] = eHSAID_fullscreen;
		m_safeAreas["eHSAID_PS3_trc"] = eHSAID_PS3_trc;
	}
	m_curSafeArea = m_safeAreas.begin();

	if( g_pGame->GetHUD()->GetCVars()->hud_test_iterateResolutions )
	{ // use the list of target resolutions plus the current resolution.
		m_resolutions.push_back(Vec2i(640,480));   // std def (PAL)
		m_resolutions.push_back(Vec2i(800,600));   // old skool PC
		m_resolutions.push_back(Vec2i(1280,720));  // 720p
		m_resolutions.push_back(Vec2i(1920,1080)); // 1080i
	}

	m_outputPath = outputPath;

	CHUD::AddHUDEventListener(this, "OnHUDReload");

	m_xmlOutput = GetISystem()->CreateXmlNode(); // Create a new XML file
	m_xmlOutput->setTag("testsuites");
	m_xmlOutput->setAttr("name", "HUDTests");

	Vec2i cur_res( gEnv->pRenderer->GetWidth(), gEnv->pRenderer->GetHeight() );
	if( !stl::find(m_resolutions, cur_res ) )
		m_resolutions.push_back( cur_res );     // current size, default.

	m_curResolution = m_resolutions.begin();
	SetResolution( m_curResolution->x, m_curResolution->y );
	m_settleDownTimer = static_cast<float>(g_pGame->GetHUD()->GetCVars()->hud_test_timeBetweenTestScreenShots);

	m_curStateId=0;																 

	g_pGame->GetHUD()->Reload();
}

void HUDTester::Stop(void)
{
	// set multi-player state to previous
	gEnv->bMultiplayer = m_prevMultiplayerState;

	m_resolutions.pop_back(); // remove cur res.
	g_pGame->GetIGameFramework()->UnregisterListener(this);
	CHUD::RemoveHUDEventListener(this);

	m_xmlOutput->setAttr("tests", m_numTestsuites);
	m_xmlOutput->setAttr("failures", m_numFailedTestsuites);
	m_xmlOutput->setAttr("totaltestcases", m_numTestcases); // Non standard JUnit
	m_xmlOutput->setAttr("errors", m_numFailedTestcases);
	CAutoTester::SaveToValidXmlFile( m_xmlOutput, m_outputPath.c_str() );

	CHUD::CmdTestHudDone( NULL );

	g_pGame->GetHUD()->GetCVars()->hud_hideMenus = 0;
	HUDTester::s_running = 0;
}


void HUDTester::OnPostUpdate(float fDeltaTime)
{
	m_settleDownTimer -= fDeltaTime;
	if(m_settleDownTimer>0)
	{
		return;
	}

	ScreenShotCurState();

	if( AllPerResolutionTestsDone() )
	{
		// Change resolution, will be active next frame.
		m_curResolution++;

		if( m_curResolution >= m_resolutions.end() )
		{
			Stop();
			return;
		}

		SetResolution( m_curResolution->x, m_curResolution->y );
		m_curStateId=0;
		m_curSafeArea=m_safeAreas.begin();

		if( g_pGame->GetHUD()->GetCVars()->hud_test_takeScreenShots )
		{
			m_settleDownTimer = static_cast<float>(g_pGame->GetHUD()->GetCVars()->hud_test_timeBetweenTestScreenShots); // sleep for screen to settle.
		}
	}
	else
	{
		// SetUpTest
		if( SetupNextTest() )
		{
			// Test the current setup.
			TestStatesOcclusion(&m_xmlOutput);
		}
	}
}

void HUDTester::TestStateObjectsOcclusion( const IHUDState* pState, const char* stateName, TAssetInfos* pAssetInfos ) const 
{
	if( !pState )
	{
		return;
	}

	assert(pAssetInfos);
	TAssetInfos& assetInfos = *pAssetInfos;

	// Scan this states objects
	const THUDObjects* pObjects = pState->GetObjects();
	if(pObjects)
	{
		THUDObjects::const_iterator it_object = pObjects->begin();
		THUDObjects::const_iterator end_object = pObjects->end();
		for(; it_object!=end_object; ++it_object)
		{
			const CHUDObject* pObject = *it_object;

			// Skip overlay objects
			if( pState->IsObjectAnOverlay(pObject) )
				continue;

			const char* objectName = m_objectManager->FindObjectName( pObject );

			const CHUDObject::TAssets* pAssets = pObject->GetAssets();
			CHUDObject::TAssets::const_iterator it_asset = pAssets->begin();
			CHUDObject::TAssets::const_iterator end_asset = pAssets->end();
			for(; it_asset!=end_asset; ++it_asset)
			{
				const IHUDAsset* pAsset = &*it_asset->second;
				SAssetInfo ainfo;
				ainfo.pParentObject = pObject;
				ainfo.pSizeAndPosInfo = &(pAsset->m_info);
				ainfo.stateName  = stateName;
				ainfo.objectName = objectName;
				ainfo.assetName  = pAsset->GetName();
				assetInfos.push_back( ainfo );
			}//end for _asset
		}//end for _object
	}// if pObjects
}

void HUDTester::TestStatesOcclusion( XmlNodeRef* pOutXmlNode )
{
	const CHUDStateManager::TStates* pStates = m_stateManager->GetStates();
	CHUDStateManager::TStates::const_iterator it_state = pStates->begin();
	CHUDStateManager::TStates::const_iterator end_state = pStates->end();
	for(; it_state!=end_state; ++it_state)
	{
		// Output start
		XmlNodeRef testsuite = GetISystem()->CreateXmlNode("HUDTest");
		testsuite->setTag("testsuite");

		int testcount=0;
		int failedtestcount=0;

		TAssetInfos assetInfos;
		const string& stateName_str = it_state->first;
		const char* stateName = stateName_str.c_str();
		IHUDState* pState = it_state->second;

		TestStateObjectsOcclusion(pState, stateName, &assetInfos);

		// Scan the required states objects.
		const TRequiredStates requiredStates = pState->GetDepAncestralStates();
		TRequiredStates::const_iterator it_parentStates = requiredStates.begin();
		TRequiredStates::const_iterator end_parentStates = requiredStates.end();
		for(; it_parentStates!=end_parentStates; ++it_parentStates)
		{
			const IHUDState* pParentState = *it_parentStates;
			TestStateObjectsOcclusion(pParentState, m_stateManager->FindStateName(pParentState), &assetInfos);
		}

		TAssetInfos::const_iterator it_assetInfo = assetInfos.begin();
		TAssetInfos::const_iterator end_assetInfo = assetInfos.end();
		for(;it_assetInfo!=end_assetInfo;++it_assetInfo)
		{
			TAssetInfos::const_iterator it_assetInfo2 = it_assetInfo;

			for(;it_assetInfo2!=end_assetInfo;++it_assetInfo2)
			{
				if( it_assetInfo2 == it_assetInfo ) // don't check against self
				{
					continue;
				}

				const SAssetInfo& ainfo1 = *(it_assetInfo);
				const SAssetInfo& ainfo2 = *(it_assetInfo2);

				// skip if allowed overlap
				if( pState->IsObjectObjectOverlapAllowed( ainfo1.pParentObject, ainfo2.pParentObject ) )
				{
					continue;
				}

				const SHUDSizeAndPositionInfo& spi1 = *(ainfo1.pSizeAndPosInfo);
				const SHUDSizeAndPositionInfo& spi2 = *(ainfo2.pSizeAndPosInfo);

				const int minx1 = min(spi1.x, spi1.x + spi1.w);
				const int maxx1 = max(spi1.x, spi1.x + spi1.w);

				const int miny1 = min(spi1.y, spi1.y + spi1.h);
				const int maxy1 = max(spi1.y, spi1.y + spi1.h);

				const int minx2 = min(spi2.x, spi2.x + spi2.w);
				const int maxx2 = max(spi2.x, spi2.x + spi2.w);

				const int miny2 = min(spi2.y, spi2.y + spi2.h);
				const int maxy2 = max(spi2.y, spi2.y + spi2.h);

				bool pass = false;
				if( maxx1 <= minx2 )
				{
					pass = true;
				}

				if( minx1 >= maxx2 )
				{
					pass = true;
				}

				if( maxy1 <= miny2 )
				{
					pass = true;
				}

				if( miny1 >= maxy2 )
				{
					pass = true;
				}

				XmlNodeRef testcase = GetISystem()->CreateXmlNode();
				testcase->setTag("testcase");
				testcase->setAttr("statename", stateName );
				string nameStr;
				nameStr.Format("%s.%s.%s === %s.%s.%s", ainfo1.stateName, ainfo1.objectName, ainfo1.assetName, ainfo2.stateName, ainfo2.objectName, ainfo2.assetName );
				testcase->setAttr("name", nameStr.c_str());
				testcase->setAttr("HUDStateName1", ainfo1.stateName);
				testcase->setAttr("HUDObject1",   ainfo1.objectName);
				testcase->setAttr("HUDAsset1",    ainfo1.assetName);
				testcase->setAttr("HUDStateName2", ainfo2.stateName);
				testcase->setAttr("HUDObject2",   ainfo2.objectName);
				testcase->setAttr("HUDAsset2",    ainfo2.assetName);

				testcount++;
				if(pass)
				{
					testcase->setAttr("status", "run");
				}
				else
				{
					failedtestcount++;
					//testcase->setAttr("status", "failed");
					XmlNodeRef failedCase = GetISystem()->CreateXmlNode();
					failedCase->setTag("failure");
					failedCase->setAttr("type", "OverLap");
					string msg;
					msg.Format("At resolution %dx%d, HUD Element %s.%s.%s (with bounding box tl[%d,%d], br[%d,%d]) overlaps element %s.%s.%s (with bounding box tl[%d,%d] br[%d,%d])", gEnv->pRenderer->GetWidth(), gEnv->pRenderer->GetHeight(), ainfo1.stateName, ainfo1.objectName, ainfo1.assetName, minx1, miny1, maxx1, maxy1, ainfo2.stateName, ainfo2.objectName, ainfo2.assetName, minx2, miny2, maxx2, maxy2 );
					failedCase->setAttr("message", msg.c_str() );
					testcase->addChild(failedCase);
				}

				// record results.
				testsuite->addChild( testcase );

			} // end for it_assetInfo2
		}	// end for it_assetInfo

		testsuite->setAttr("name", string().Format("%s_-_at_%dx%d_-_%s", stateName, gEnv->pRenderer->GetWidth(),gEnv->pRenderer->GetHeight(), m_curSafeArea->first.c_str()).c_str());
		testsuite->setAttr("tests", testcount);
		testsuite->setAttr("failed", failedtestcount);
		testsuite->setAttr("errors", 0);
		testsuite->setAttr("time", 0);
		testsuite->setAttr("r_Width", gEnv->pRenderer->GetWidth());
		testsuite->setAttr("r_Height", gEnv->pRenderer->GetHeight());
		testsuite->setAttr("safearea", m_curSafeArea->first.c_str());

		m_numTestcases += testcount;
		m_numFailedTestcases += failedtestcount;
		m_numTestsuites++;
		m_numFailedTestsuites += failedtestcount ? failedtestcount : 0;
		(*pOutXmlNode)->addChild(testsuite);

	}//end for _states
}

void HUDTester::ScreenShotCurState( void )
{
	if( g_pGame->GetHUD()->GetCVars()->hud_test_takeScreenShots )
	{
		const Vec2i& testRes = *m_curResolution;
		const char* curStateName = m_stateManager->FindStateName( m_stateManager->GetActiveState() );
		gEnv->pConsole->ExecuteString( string().Format("Screenshot %d_%d_%d_%s", m_curStateId, testRes.x, testRes.y, curStateName ).c_str() );
		m_settleDownTimer = static_cast<float>(g_pGame->GetHUD()->GetCVars()->hud_test_timeBetweenTestScreenShots);
	}
}

bool HUDTester::AllPerResolutionTestsDone( void )
{
	// do all states at one safe area first
	// then change safe area and do them again.
	const CHUDStateManager::TStates* pStates = m_stateManager->GetStates();
	//return (m_curStateId >= pStates->size()) && ( m_curSafeArea == m_safeAreas.end() );
	return ( m_curSafeArea == m_safeAreas.end() );
}

void HUDTester::ActivateNextHUDState( void )
{
	const CHUDStateManager::TStates* pStates = m_stateManager->GetStates();

	// Set up next state for next frame.
	CHUDStateManager::TStates::const_iterator it = pStates->begin();
	for(int i=0;i<m_curStateId;i++, it++){}

	const char* stateName = it->first;
	g_pGame->GetHUD()->ActivateState( stateName );
	m_curStateId++;
}

bool HUDTester::SetupNextTest( void )
{
	// do all states at one safe area first
	// then change safe area and do them again.
	const CHUDStateManager::TStates* pStates = m_stateManager->GetStates();
	if(m_curStateId >= pStates->size())
	{
		m_curSafeArea++;

		// done with states, get next safe area and reset states
		if( m_curSafeArea != m_safeAreas.end() )
		{
			m_curStateId = 0;
			g_pGame->GetHUD()->GetLayoutManager()->SetSafeArea( m_curSafeArea->second );
		}
		else
		{
			// all state across all safe-areas have been done
			return false;
		}
	}
	
	ActivateNextHUDState();

	return true;
}

void HUDTester::SetResolution( const int width, const int height ) const
{
	gEnv->pConsole->ExecuteString(string().Format("r_Width %d", width).c_str());
	gEnv->pConsole->ExecuteString(string().Format("r_Height %d", height).c_str());
}

void HUDTester::OnHUDEvent(const SHUDEvent& event)
{
	assert( event.eventType == eHUDEvent_OnHUDReload );
	assert( g_pGame );
	g_pGame->GetIGameFramework()->RegisterListener(this,"HUD", FRAMEWORKLISTENERPRIORITY_GAME); // set up to render once each frame (after resolutions have been updated)
}

#endif // ENABLE_HUD_TESTS