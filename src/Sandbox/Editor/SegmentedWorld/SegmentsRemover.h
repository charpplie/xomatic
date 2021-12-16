
#ifndef __SegmentsRemover_h__
#define __SegmentsRemover_h__

class CSegmentsRemover
{
public:
	CSegmentsRemover(const CString& strLevelPath, const std::vector<CPoint>& wcs)
		:m_strLevelPath(strLevelPath), m_wcs(wcs)
	{
	}

	~CSegmentsRemover() {}

	bool ValidateInput(const CString& strLevelPath, const std::vector<CPoint>& wcs);
	void Process();

private:
	void RemoveFolders();
	void UpdateSegmentInfoFile(std::set<GUID, guid_less_predicate>& remainingLayers);
	void UpdateLayerInfoFile(const std::set<GUID, guid_less_predicate>& remainingLayers );

	CString m_strLevelPath;
	const std::vector<CPoint>& m_wcs;
};

#endif // __SegmentsRemover_h__
