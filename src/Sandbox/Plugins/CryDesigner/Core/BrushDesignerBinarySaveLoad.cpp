#include "StdAfx.h"
#include "BrushDesignerBinarySaveLoad.h"
#include "BrushDesigner.h"
#include "Objects/DesignerBrushObject.h"
#include "ISourceControl.h"

namespace
{
	const char* kDesignerBinaryFile = "DesignerObjects.dat";
	const char* kDesignerMeshFile = "DesignerMeshes.dat";
};

std::vector< _smart_ptr<CDesignerBrushObject> > GetDesignerObjects( bool bIncludeOpenDesigner )
{
	DynArray<CBaseObject*> objects;
	GetIEditor()->GetObjectManager()->GetObjects(objects);
	int nObjectCount = objects.size();
	std::vector< _smart_ptr<CDesignerBrushObject> > designerObjects;
	for( int i = 0; i < nObjectCount; ++i )
	{
		if( !objects[i]->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
			continue;

		CBrushDesigner* pDesigner = ((CDesignerBrushObject*)objects[i])->GetDesigner();
		if( pDesigner->GetRegionSize() <= 0 )
			continue;

		CBaseBrush* pBrush = ((CDesignerBrushObject*)objects[i])->GetBrush();

		bool bHaveClosedRegions = false;
		for( int k = 0, iRegionCount(pDesigner->GetRegionSize()); k < iRegionCount; ++k )
		{
			if( !pDesigner->GetRegion(k)->IsOpen() )
			{
				bHaveClosedRegions = true;
				break;
			}
		}

		bool bValidMesh = false;
		_smart_ptr<IStatObj> pStatObj;
		if( pBrush->GetIStatObj(&pStatObj) )
		{
			IIndexedMesh* pMesh = pStatObj->GetIndexedMesh();
			if( pMesh )
			{
				int nSubObjectCount = pStatObj->GetSubObjectCount();
				if( nSubObjectCount == 0 )
				{
					bValidMesh = ( pMesh->GetFaceCount() > 0 || pMesh->GetIndexCount() > 0 ) && pMesh->GetVertexCount() > 0 && pMesh->GetTexCoordCount() > 0;
				}
				else
				{
					bValidMesh = nSubObjectCount == 2;
				}
			}
		}

		DESIGNER_ASSERT(bValidMesh);

		if( bIncludeOpenDesigner || bValidMesh && bHaveClosedRegions )
			designerObjects.push_back((CDesignerBrushObject*)objects[i]);
	}
	return designerObjects;
}

void CBrushDesignerBinarySaveLoad::Save()
{
	std::vector< _smart_ptr<CDesignerBrushObject> > designerObjects = GetDesignerObjects(true);
	if( designerObjects.empty() )
		return;

	CString fileName = GetIEditor()->GetLevelDataFolder() + kDesignerBinaryFile;

	if (!CFileUtil::OverwriteFile( fileName ))
		return;

	CFile cFile;
	if (!cFile.Open(fileName, CFile::modeCreate | CFile::modeWrite))
		return;
	CArchive ar(&cFile, CArchive::store);
	int nDesignerObjectCount = designerObjects.size();
	ar.Write(&nDesignerObjectCount,sizeof(int));
	for( int i = 0; i < nDesignerObjectCount; ++i )
	{
		CDesignerBrushObject* pObject = designerObjects[i];
		ar.Write( &pObject->GetId(), sizeof(GUID) );
		((CDesignerBrushObject*)pObject)->GetDesigner()->Save(ar);
	}
}

void SkipLoadingOneDesigner( CArchive& ar )
{
	_smart_ptr<CBrushDesigner> pDesigner = new CBrushDesigner;
	pDesigner->Load(ar);
}

void CBrushDesignerBinarySaveLoad::Load()
{
	CString fileName = GetIEditor()->GetLevelDataFolder() + kDesignerBinaryFile;
	CFile cFile;
	if (!cFile.Open(fileName, CFile::modeRead))
		return;

	CArchive ar(&cFile, CArchive::load);
	int nDesignerObjectCount = 0;
	ar.Read(&nDesignerObjectCount,sizeof(int));

	std::vector< _smart_ptr<CDesignerBrushObject> > designerObjects;
	for( int i = 0; i < nDesignerObjectCount; ++i )
	{
		GUID guid;
		ar.Read( &guid, sizeof(guid) );
		CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(guid);
		if( !pObject )
		{
			SkipLoadingOneDesigner(ar);
			continue;
		}

		CBrushDesigner* pDesigner = ((CDesignerBrushObject*)pObject)->GetDesigner();
		if( pDesigner->IsEmpty() )
			pDesigner->Load(ar);
		else
			SkipLoadingOneDesigner(ar);
	}
}

void CBrushDesignerBinarySaveLoad::SaveMeshes()
{
	std::vector< _smart_ptr<CDesignerBrushObject> > designerObjects = GetDesignerObjects(false);
	if( designerObjects.empty() )
		return;

	CString fileName = GetIEditor()->GetLevelDataFolder() + kDesignerMeshFile;
	if (!CFileUtil::OverwriteFile( fileName ))
		return;
	CFile cFile;
	if (!cFile.Open(fileName, CFile::modeCreate | CFile::modeWrite))
		return;
	CArchive ar(&cFile, CArchive::store);
	int nDesignerObjectCount = designerObjects.size();
	ar.Write(&nDesignerObjectCount,sizeof(int));
	for( int i = 0; i < nDesignerObjectCount; ++i )
	{
		CDesignerBrushObject* pObject = designerObjects[i];
		ar.Write( &pObject->GetId(), sizeof(GUID) );
		((CDesignerBrushObject*)pObject)->GetBrush()->SaveMesh(ar,pObject,pObject->GetDesigner());
	}
}

bool CBrushDesignerBinarySaveLoad::LoadMeshes()
{
	CString fileName = GetIEditor()->GetLevelDataFolder() + kDesignerMeshFile;
	CFile cFile;
	if (!cFile.Open(fileName, CFile::modeRead))
		return false;

	CArchive ar(&cFile, CArchive::load);
	int nDesignerObjectCount = 0;
	ar.Read(&nDesignerObjectCount,sizeof(int));

	for( int i = 0; i < nDesignerObjectCount; ++i )
	{
		GUID guid;
		ar.Read( &guid, sizeof(guid) );
		CBaseObject* pObject = GetIEditor()->GetObjectManager()->FindObject(guid);
		if( !pObject || !pObject->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
			return false;
		
		CBaseBrush* pBrush = ((CDesignerBrushObject*)pObject)->GetBrush();
		if( !pBrush->LoadMesh(ar,pObject,((CDesignerBrushObject*)pObject)->GetDesigner()) )
			return false;
	}

	return true;
}

void CBrushDesignerBinarySaveLoad::UpdateAllDesignerObjects( bool bForce )
{
	DynArray<CBaseObject*> objects;
	GetIEditor()->GetObjectManager()->GetObjects(objects);
	int nObjectCount = objects.size();
	for( int i = 0; i < nObjectCount; ++i )
	{
		if( objects[i]->IsKindOf(RUNTIME_CLASS(CDesignerBrushObject)) )
		{
			CDesignerBrushObject* pDesignerObj = ((CDesignerBrushObject*)objects[i]);
			if( !pDesignerObj->IsEmpty() && pDesignerObj->GetDesigner()->HasClosedRegion() && ( bForce || pDesignerObj->GetBrush() && !pDesignerObj->GetBrush()->IsValid() ) )
				pDesignerObj->UpdateBrush();
		}
	}
}

void CBrushDesignerBinarySaveLoad::DeleteDesignerBinaryFiles()
{
	CString binaryFiles[2] = { GetIEditor()->GetLevelDataFolder() + kDesignerBinaryFile, GetIEditor()->GetLevelDataFolder() + kDesignerMeshFile };
	for( int i = 0; i < 2; ++i )
	{
		if(!CFileUtil::OverwriteFile(binaryFiles[i]))
			continue;
		if( GetIEditor()->IsSourceControlAvailable() && (GetIEditor()->GetSourceControl()->GetFileAttributes(binaryFiles[i])&SCC_FILE_ATTRIBUTE_CHECKEDOUT) )
			GetIEditor()->GetSourceControl()->Delete(binaryFiles[i]);
		else
			CFileUtil::DeleteFile(binaryFiles[i]);
	}
}