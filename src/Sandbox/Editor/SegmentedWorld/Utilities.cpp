
#include "StdAfx.h"
#include "InternalCommon.h"
#include "CryEditDoc.h"
#include "Util\PakFile.h"
#include "SegmentedWorld\SegmentedWorldManager.h"
#include "SegmentedWorld\InternalCommon.h"
#include "Objects\Group.h"
#include "Terrain/Heightmap.h"

SW_NAMESPACE_BEGIN();
void SWLogV( bool bConsoleOnly,const char * pszLog, va_list args)
{

	char szLogBuf[MAX_PATH*2];
	strncpy(szLogBuf,"SW: ",4);
	char* pszPureString = szLogBuf + 4;

	char* pszEnd = pszPureString + vsprintf(pszPureString,pszLog,args);
	pszEnd[0] = '\n';
	pszEnd[1] = '\0';

	OutputDebugString(pszPureString);
	if (!bConsoleOnly)
	{
		CryLog(szLogBuf);
		//gEnv->pLog->LogToConsole(chLog);
	}
}

void SWLog( const char * pszLog, ...)
{
	va_list args;
	va_start(args,pszLog);
	SWLogV(false,pszLog,args);
	va_end(args);
}

void SWLogConsole( const char * pszLog, ...)
{
	va_list args;
	va_start(args,pszLog);
	SWLogV(true,pszLog,args);
	va_end(args);
}

void SWWarning( const char *format, ... )
{
	char szBuffer[MAX_WARNING_LENGTH] = {0};
	va_list args;
	va_start(args, format);
	vsnprintf(szBuffer, MAX_WARNING_LENGTH, format, args);
	va_end(args);

	gEnv->pSystem->Warning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, VALIDATOR_FLAG_FILE, 0, "SW: %s", szBuffer);
}

bool SaveMemBlockToFile(const char *pcFileName, CMemoryBlock &mem, UINT nOpenFlags/* = 0*/)
{
	//assert(mem.GetSize());

	CFile file;
	if (!CFileUtil::CreatePath(pcFileName) || !file.Open(pcFileName, CFile::modeCreate | CFile::modeWrite | nOpenFlags))
	{
		SWWarning("Failed to save (%s)", pcFileName);
		return false;
	}
	CArchive ar(&file, CArchive::store);
	mem.Serialize(ar);
	return true;
}

bool LoadMemBlockFromFile(const char *pcFileName, CMemoryBlock &mem, UINT nOpenFlags/* = 0*/)
{
	CFile file;
	if (!file.Open(pcFileName, CFile::modeRead | nOpenFlags))
	{
		SWWarning("Failed to load (%s)", pcFileName);
		return false;
	}

	CArchive ar(&file, CArchive::load);
	mem.Serialize(ar);
	return true;
}

bool SaveRawDataToFile(const char* pcFileName, CMemoryBlock& mem)
{
	return SaveRawDataToFile(pcFileName, mem.GetBuffer(), mem.GetSize());
}

// WARNING: This function is used in DataUpgrade, don't forget to "Copy-on-change" to DataUpgrade module.
bool SaveRawDataToFile(const char* pcFileName, void* pBuf, size_t cbSize)
{
	if(!pBuf || !cbSize)
	{
		assert(0);
		return false;
	}

	CFile file;
	if (!CFileUtil::CreatePath(pcFileName) || !file.Open(pcFileName, CFile::modeCreate | CFile::modeWrite))
	{
		SWWarning("Failed to save (%s)", pcFileName);
		return false;
	}
	file.Write(pBuf, cbSize);
	file.Close();
	return true;
}

bool LoadRawDataFromFile(const char* pcFileName, CMemoryBlock& mem)
{
	CFile file;
	if (!file.Open(pcFileName, CFile::modeRead))
	{
		SWWarning("Failed to load (%s)", pcFileName);
		return false;
	}

	mem.Allocate(file.GetLength());
	file.Read(mem.GetBuffer(), file.GetLength());
	file.Close();

	return true;
}

bool SaveRawDataToScry(const char* pcFileName, CMemoryBlock& mem)
{
	auto scryFileName = GetIEditor()->GetSegmentedWorldManager()->GetScryPath();

	CPakFile pakFile;
	if (!pakFile.Open(scryFileName, false))
	{
		return false;
	}

	if (!pakFile.UpdateFile(PathUtil::GetFile(pcFileName), mem.GetBuffer(), mem.GetSize()))
	{
		Warning("Failed to update pak file with %s", pcFileName);
	}
	else
	{
		remove(pcFileName);
	}

	return true;
}

bool LoadRawDataFromScry(const char* pcFileName, CMemoryBlock& mem)
{
	auto pIPak = GetIEditor()->GetSystem()->GetIPak();

	auto file = pIPak->FOpen(pcFileName, "rb");
	if (!file)
	{
		return false;
	}

	// get file size
	pIPak->FSeek(file, 0, SEEK_END);
	auto size = pIPak->FTell(file);
	pIPak->FSeek(file, 0, SEEK_SET);

	mem.Allocate(size);
	pIPak->FReadRaw(mem.GetBuffer(), 1, size, file);
	pIPak->FClose(file);

	return true;
}

bool UpdateFileToScryAndDelete(const CString& sPath)
{
	const auto& path = Path::GetRelativePath(GetIEditor()->GetSegmentedWorldManager()->GetScryPath());

	auto IPak = GetIEditor()->GetSystem()->GetIPak();
	IPak->ClosePack(path); // close possible read-only open entry
	CPakFile pakFile;
	if (!pakFile.Open(path))
	{
		return false;
	}

	CFile file;
	if (file.Open(sPath, CFile::modeRead))
	{
		CMemoryBlock mem;
		mem.Allocate(file.GetLength());
		file.Read(mem.GetBuffer(), file.GetLength());
		file.Close();

		if (!pakFile.UpdateFile(sPath, mem))
		{
			return false;
		}
		else
		{
			DeleteFile(sPath);
		}
	}

	return true;
}

bool CanAccessChilds(CBaseObject* pObject)
{
	assert(pObject);
	if (pObject->IsKindOf(RUNTIME_CLASS(CGroup)))
		return false;
	return true;
}

void GetBoundBoxWithChilds(CBaseObject* pObject, AABB &box)
{
	assert(pObject);

	pObject->GetBoundBox(box);

	if (!CanAccessChilds(pObject))
		return;

	int num = pObject->GetChildCount();
	for (int i = 0; i < num; i++)
	{
		CBaseObject *obj = pObject->GetChild(i);
		AABB box2;
		GetBoundBoxWithChilds(obj, box2);
		box.Add(box2);
		box.Add(obj->GetWorldPos());
	}
}

TLocalCoords WorldToSegmentNoClamp(const Vec3 &v)
{
	CPoint ptHM = GetIEditor()->GetHeightmap()->WorldToHmap(v);
	if (ptHM.x < 0)
	{
		ptHM.x += -SEGMENT_SIZE_UNITS;
	}
	if (ptHM.y < 0)
	{
		ptHM.y += -SEGMENT_SIZE_UNITS;
	}

	int lx = (int)(ptHM.x / SEGMENT_SIZE_UNITS);
	int ly = (int)(ptHM.y / SEGMENT_SIZE_UNITS);

	return TLocalCoords(lx, ly);
}

bool IsObjectBig(CBaseObject* pObject)
{
	//handle big objects
	TWorldCoords wcPos, wcBoxMin, wcBoxMax;
	wcPos = WorldToSegmentNoClamp(pObject->GetWorldPos());//m_lc;
	AABB box;
	GetBoundBoxWithChilds(pObject, box);
	//pObject->GetBoundBox(box);
	wcBoxMin = WorldToSegmentNoClamp(box.min);
	wcBoxMax = WorldToSegmentNoClamp(box.max);

	return (wcPos != wcBoxMin || wcPos != wcBoxMax || wcBoxMin != wcBoxMax);
}

void SegIdToWorldRect(SegmentID nSegmentID, CRect &destWorldRect)
{
	int wx=0;
	int wy=0;
	GetWorldCoords(nSegmentID,wx,wy);
	destWorldRect.left = wx*SEGMENT_SIZE_METERS;
	destWorldRect.right = (wx+1)*SEGMENT_SIZE_METERS;
	destWorldRect.top = wy*SEGMENT_SIZE_METERS;
	destWorldRect.bottom = (wy+1)*SEGMENT_SIZE_METERS;
}

void SegIdToWorldRect(SegmentID nSegmentID, Recti &destWorldRect, int meterSize)
{
	int wx=0;
	int wy=0;
	GetWorldCoords(nSegmentID,wx,wy);
	destWorldRect.Min.x = wx*meterSize;
	destWorldRect.Max.x = (wx+1)*meterSize;
	destWorldRect.Min.y = wy*meterSize;
	destWorldRect.Max.y = (wy+1)*meterSize;
}

SW_NAMESPACE_END();
