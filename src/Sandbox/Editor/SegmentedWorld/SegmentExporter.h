////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2013.
// -------------------------------------------------------------------------
//  File name:   SegmentExporter.h
//  Version:     v1.00
//  Created:     18/10/2013 by Allen Chen
//  Compilers:   Visual Studio.NET
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////
#ifndef __SEGMENT_EXPORTER_H__
#define __SEGMENT_EXPORTER_H__

#include "GameExporter.h"

struct SSegmentPakHelper : public SLevelPakHelper
{
	CString m_sBlockPath;
	CString m_sCtcFilename;
};

class CSegmentExporter
{
public:
	void ExportSegments(CGameExporter *pge, bool bSurfaceTexture);
	void UpdateLevelInfo(XmlNodeRef levelFile);

private:
	bool OpenSegmentPacksForWrite();
	bool ExportSurfaceTexture();
	bool ExportMap();
	bool ExportHeightMap(bool bForce = true);
	void ExportNavigation();
	void UpdateAndCloseSegmentPacksForWrite();

	std::vector<SSegmentPakHelper> m_segPak;
	CGameExporter *m_pGameExporter;
	int m_nWidth, m_nHeight;
	bool m_bMultiPak;
	bool m_bExportMap;
};

#endif // __SEGMENT_EXPORTER_H__