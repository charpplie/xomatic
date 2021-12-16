#pragma once

#include "SWFwd.h"



SW_NAMESPACE_BEGIN();

class CMiniMapGenerator : public IScreenshotCallback
{
	typedef CMiniMapGenerator TSelf;
	typedef class CSegmentedWorldDoc TWorld;
	typedef class CSegmentedWorldDoc TUIStatus;

	TWorldCoords m_wcMiniMap;
	TWorldCoordsArray m_wcArrMiniMap;
	TLocalCoords m_lcTopLeft;
	int m_nWidth;
	int m_nHeight;
	TWorldCoords m_wcTopLeft;
	TWorldCoords m_wcBottomRight;
	int m_nScreenshotMode;
	std::map<string,float> m_ConstClearList;

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
	bool IsValid();
	bool Step();
	bool GenerateSegmentedMap(TWorldCoordsArray* pArrWorldCoords = NULL);

protected:
	bool DoGenerateSegmentedMap(int lx, int ly, int w, int h);
	bool DoGenerateSegmentedMap(const TLocalCoords &lc);
	void OnScreenshotTaken( void* data, uint32 width, uint32 height );
	void CopyToCache( const TWorldCoords& wc, CImage& scrShotImage );
	void SaveSegmentedMap();

private:
	virtual void SendParameters(void* data, uint32 width, uint32 height, f32 minx, f32 miny, f32 maxx, f32 maxy)
	{
		this->OnScreenshotTaken(data, width, height);
	}
	bool ForceSaveDataBlocks(const std::vector<sw::ESDBType> &arrDataBlockTypes, const TWorldCoords &wc, int64 nVersion, sw::EVersionType vt);
};


SW_NAMESPACE_END();
