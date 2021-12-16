#ifndef __HUDASSETOVERLAPCHECKER_H__
#define __HUDASSETOVERLAPCHECKER_H__

//////////////////////////////////////////////////////////////////////////

#include "HUD/HUD.h"

#if ENABLE_HUD_TESTS

#include <IGameFramework.h>
#include "HUD/HUDEventDispatcher.h"
#include "HUD/ScreenLayoutManager.h"

//////////////////////////////////////////////////////////////////////////

class CHUDObject;
class CHUDStateManager;
class CHUDObjectManager;

struct SHUDSizeAndPositionInfo;
struct IHUDState;

//////////////////////////////////////////////////////////////////////////

class HUDTester : public IGameFrameworkListener
	, public IHUDEventListener
{
	struct SAssetInfo{
		const CHUDObject* pParentObject; // used for allowed occlusions.
		const SHUDSizeAndPositionInfo* pSizeAndPosInfo;
		const char* stateName;
		const char* objectName;
		const char* assetName; 

		SAssetInfo()
		{
			pSizeAndPosInfo = NULL;
			stateName = NULL;
			objectName = NULL;
			assetName = NULL; 
		}
	};
	typedef std::vector<SAssetInfo> TAssetInfos; // Only used but tests.

public :
	explicit HUDTester(CHUDStateManager* pStateManager, CHUDObjectManager* pObjectManager);

	~HUDTester();

	void Start(const char* outputPath);
	void Stop(void);

	// IGameFrameworkListener
	virtual void OnPostUpdate(float fDeltaTime);
	virtual void OnSaveGame(ISaveGame* pSaveGame) {}
	virtual void OnLoadGame(ILoadGame* pLoadGame) {}
	virtual void OnLevelEnd(const char* nextLevel) {}
	virtual void OnActionEvent(const SActionEvent& event) {}
	//~IGameFrameworkListener

	// IHUDEventListener
	virtual void OnHUDEvent(const SHUDEvent& event);
	// ~IHUDEventListener

	// Tests
	void TestStateObjectsOcclusion( const IHUDState* pState, const char* stateName, TAssetInfos* pAssetInfos ) const ;
	void TestStatesOcclusion( XmlNodeRef* pOutXmlNode );

	void ScreenShotCurState( void );

	void SetResolution( const int width, const int height ) const;

	bool AllPerResolutionTestsDone( void );
	void ActivateNextHUDState( void );
	bool SetupNextTest( void );
private :

	// JUnit Stats for XML Output.
	int m_numTestsuites;
	int m_numFailedTestsuites;
	int m_numTestcases;
	int m_numFailedTestcases;
	XmlNodeRef           m_xmlOutput;
	CryFixedStringT<255> m_outputPath;

	// HUD Managers
	CHUDStateManager*    m_stateManager;
	CHUDObjectManager*   m_objectManager;

	// Resolution test specifics
	std::vector<Vec2i>   m_resolutions;
	std::vector<Vec2i>::const_iterator   m_curResolution;
	int                  m_curStateId;
	std::map<CryFixedStringT<16>,EHUDSafeAreaID> m_safeAreas;
	std::map<CryFixedStringT<16>,EHUDSafeAreaID>::const_iterator m_curSafeArea;

	float                m_settleDownTimer;

	bool                 m_prevMultiplayerState;

public:
	static int s_running;
};

#endif // ENABLE_HUD_TESTS

#endif //__HUDASSETOVERLAPCHECKER_H__