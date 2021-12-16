
#ifndef __SegmentLevelMerger_h__
#define __SegmentLevelMerger_h__

class CSegmentLevelMerger
{
public:
	CSegmentLevelMerger(const CString& strDestPath, 
		const CString& strSrcPath, 
		const Vec2i& vDestTopLeft, 
		const Vec2i& vSrcTopLeft, 
		const Vec2i& vSize)
		: m_strDestPath(strDestPath)
		, m_strSrcPath(strSrcPath)
		, m_vDestTopLeft(vDestTopLeft)
		, m_vSrcTopLeft(vSrcTopLeft)
		, m_vSize(vSize)
		, m_nProgStageEstimate(vSize.x * vSize.y)
		, m_progressBar(nullptr)
		, m_nCurStep(0)
		, m_nTotoalSteps(0)
	{}

	~CSegmentLevelMerger() {}

	bool ValidateInput();
	void Process(bool bWithGuiReport);

private:
	typedef std::vector<std::pair<CPoint, XmlNodeRef>> XmlArray;
	typedef std::map<GUID, XmlNodeRef, guid_less_predicate> TGuidXmlNodeMap;

	const Vec2i GetDiffSrcToDest() const {return m_vDestTopLeft - m_vSrcTopLeft;}
	const Recti GetRectDest() const
	{
		Recti rectDest;
		rectDest.Min = m_vDestTopLeft;
		rectDest.Max = rectDest.Min + m_vSize - Vec2i(1,1);
		assert(Recti(0,0,0,0).InRect(Vec2i(0,0)));			// as long as this assert is true, we need to minus 1 in Max

		return rectDest;
	}

	const Recti GetRectSrc() const
	{
		Recti rectSrc;
		rectSrc.Min = m_vSrcTopLeft;
		rectSrc.Max = rectSrc.Min + m_vSize - Vec2i(1,1);		// the TRectI uses close range(not half open range)
		assert(Recti(0,0,0,0).InRect(Vec2i(0,0)));			// as long as this assert is true, we need to minus 1 in Max

		return rectSrc;
	}

	void CacheXmlArray(XmlArray& arrMainLayerFragments, XmlArray& arrSegInfoFragments);
	void CopyExtobjFolder();
	void CopySegeditorFolders();
	void CopySegFolders();

	void MergeLayerInfoFile(XmlNodeRef& defaultLayer, std::vector<GUID>& otherMainLayerGUIDs, TGuidXmlNodeMap& layermapping);
	void UpdateGUIDForMainLayerFiles(const XmlArray& arrMainLayerFragments, const GUID& guidDefMainLayer);
	void UpdateGUIDForSegInfoFiles(const XmlArray& arrSegInfoFragments, const std::vector<GUID>& otherMainLayerGUIDs, const GUID& guidDefMainLayer);

	typedef std::deque<GUID> TGuidList;
	void MergeSegmentInfoFile(const std::vector<GUID>& otherMainLayerGUIDs, const TGuidXmlNodeMap& layermapping, const GUID& guidDefMainLayer);
	void MergeObjectinfoXml (const XmlNodeRef& defaultLayer, TGuidXmlNodeMap& layermapping, TGuidList& lstGuidNeedToCopy);
	void CopyExternalObjectFiles(const TGuidList& lstGuidNeedToCopy);
	void MergeGlobalTerrainLayersData();
	void MergeGlobalVegetationData();
	void MergeLevelPak();

	CString m_strDestPath; 
	CString m_strSrcPath;
	CString m_strMainLayerPattern;
	CString m_strSegInfoPattern;
	Vec2i m_vDestTopLeft;
	Vec2i m_vSrcTopLeft;
	Vec2i m_vSize;

	enum
	{ 
		SourceData_Dest, 
		SourceData_Src, 
		SourceData_count 
	};

	const int m_nProgStageEstimate;
	boost::shared_ptr<CWaitProgress> m_progressBar;
	int m_nCurStep;
	int m_nTotoalSteps;
};

#endif // __SegmentLevelMerger_h__