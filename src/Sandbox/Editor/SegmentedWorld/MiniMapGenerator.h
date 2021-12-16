#pragma once

#include "SWFwd.h"
#include "TerrainMiniMapTool.h"


SW_NAMESPACE_BEGIN();

class CMiniMapGenerator : public CTerrainMiniMapTool
{
	typedef class CSegmentedWorldDoc TWorld;
	typedef class CSegmentedWorldDoc TUIStatus;

	TWorldCoords m_wcMiniMap;
	TWorldCoordsArray m_wcArrMiniMap;
	TLocalCoords m_lcTopLeft;
	// map size in segments
	int m_nWidth;
	int m_nHeight;
	int m_nScreenshotMode;
	std::map<string,float> m_AdditionalConstClearList;

	TWorld* m_pWorld;
	TUIStatus* m_pUI;
	sw::ISWVersionControl* m_pVC;
public:
	explicit CMiniMapGenerator()
	{
	}

	void Init( TWorld* pWorld, TUIStatus* pUI, sw::ISWVersionControl* pVC )
	{
		Reset();
		m_pWorld = pWorld;
		m_pUI = pUI;
		m_pVC = pVC;
	}

	void Reset();
	bool Step();
	bool GenerateSegmentedMap(TWorldCoordsArray* pArrWorldCoords = NULL);

	virtual void OnEditorNotifyEvent( EEditorNotifyEvent event ) {}
	virtual bool MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags ) { return false; }
	virtual void ResetToDefault();

protected:
	bool DoGenerateSegmentedMap(int lx, int ly, int w, int h);
	void OnScreenshotTaken( void* data, uint32 width, uint32 height );
	void CopyToCache( const TWorldCoords& wc, CImageEx& scrShotImage );
	void SaveSegmentedMap();

private:
	virtual void SendParameters(void* data, uint32 width, uint32 height, f32 minx, f32 miny, f32 maxx, f32 maxy)
	{
		this->OnScreenshotTaken(data, width, height);
	}
	bool ForceSaveDataBlocks(const std::vector<sw::ESDBType> &arrDataBlockTypes, const TWorldCoords &wc, int64 nVersion, sw::EVersionType vt);
};


SW_NAMESPACE_END();
