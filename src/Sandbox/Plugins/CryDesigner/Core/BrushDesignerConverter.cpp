#include "StdAfx.h"
#include "BrushDesignerConverter.h"
#include "Objects/DesignerBrushObject.h"
#include "BaseBrush.h"
#include "BrushDesigner.h"
#include "BrushCommonInterface.h"
#include "Objects/BrushObject.h"
#include "Geometry/EdGeometry.h"
#include "Objects/EntityObject.h"
#include "CGFContent.h"
#include "Material/MaterialManager.h"
#include "BrushBSPTree2D.h"

bool CBrushDesignerConverter::CreateNewDesignerObject()
{
	std::vector<SSelectedMesh> pSelectedMeshes;
	GetSelectedObjects(pSelectedMeshes);

	if( pSelectedMeshes.empty() )
		return false;

	CUndo undo("Create Designer Objects");
	GetIEditor()->ClearSelection();

	std::vector<CDesignerBrushObject*> pDesignerObjects;
	CreateDesignerObjects(pSelectedMeshes,pDesignerObjects);

	int iSizeOfDesignerObjects(pDesignerObjects.size());

	if( iSizeOfDesignerObjects <= 0 )
		return false;

	const float kMarginBetweenOldAndNew = 1.2f;

	for( int i = 0; i < iSizeOfDesignerObjects; ++i )
	{
		AABB aabb;
		pDesignerObjects[i]->GetBoundBox(aabb);
		float gapX = (aabb.max.x-aabb.min.x)*kMarginBetweenOldAndNew;
		Matrix34 newTM(pDesignerObjects[i]->GetWorldTM());
		newTM.SetTranslation(newTM.GetTranslation()+Vec3(gapX,0,0));
		pDesignerObjects[i]->SetWorldTM(newTM);
		GetIEditor()->SelectObject(pDesignerObjects[i]);
	}

	return true;
}

bool CBrushDesignerConverter::ConvertToDesignerObject()
{
	std::vector<SSelectedMesh> pSelectedMeshes;
	GetSelectedObjects(pSelectedMeshes);

	if( pSelectedMeshes.empty() )
		return false;

	CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
	if( pSelection == NULL )
		return false;

	CUndo undo("Convert to Designer Objects");

	GetIEditor()->GetObjectManager()->ClearSelection();

	std::vector<CDesignerBrushObject*> pDesignerObjects;
	CreateDesignerObjects(pSelectedMeshes,pDesignerObjects);

	int iSizeOfDesignerObjects(pDesignerObjects.size());
	if( iSizeOfDesignerObjects <= 0 )
		return false;

	for( int i = 0; i < iSizeOfDesignerObjects; ++i )
		GetIEditor()->SelectObject(pDesignerObjects[i]);

	for( int i = 0, iSizeOfSelectedObjects(pSelectedMeshes.size()); i < iSizeOfSelectedObjects; ++i )
		GetIEditor()->DeleteObject(pSelectedMeshes[i].m_pOriginalObject);	

	return true;
}

void CBrushDesignerConverter::CreateDesignerObjects( std::vector<SSelectedMesh>& pSelectedMeshes, std::vector<CDesignerBrushObject*>& pOutDesignerObjects )
{
	for( int i = 0, iSizeOfSelectedObjs(pSelectedMeshes.size()); i < iSizeOfSelectedObjs; ++i )
	{
		SSelectedMesh& obj = pSelectedMeshes[i];

		if( obj.m_pIndexedMesh == NULL )
			continue;

		CDesignerBrushObject* pDesignerObj = CreateDesignerObject(obj.m_pIndexedMesh);

		if( obj.m_bLoadedIndexedMeshFromFile )
			delete obj.m_pIndexedMesh;

		if( pDesignerObj == NULL )
			continue;

		if( obj.m_pMaterial )
			pDesignerObj->SetMaterial(obj.m_pMaterial);

		pDesignerObj->SetWorldTM(obj.m_worldTM);
		pOutDesignerObjects.push_back(pDesignerObj);
	}
}

void CBrushDesignerConverter::GetSelectedObjects( std::vector<SSelectedMesh>& pObjects ) const
{
	CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();
	if( pSelection == NULL )
		return;

	for( int i = 0, iCount(pSelection->GetCount()); i < iCount; ++i )
	{
		SSelectedMesh s;
		CBaseObject* pObj = pSelection->GetObject(i);

		if( pObj->GetMaterial() )
			s.m_pMaterial = pObj->GetMaterial();

		if( pObj->IsKindOf( RUNTIME_CLASS(CBrushObject) ) )
		{
			CBrushObject* pBrushObj = (CBrushObject*)pObj;
			if( pBrushObj->GetGeometry() )
			{
				s.m_pIndexedMesh = pBrushObj->GetGeometry()->GetIndexedMesh();
				IStatObj* pStatObj = pBrushObj->GetGeometry()->GetIStatObj();
				if( pStatObj )
					s.m_pMaterial = GetIEditor()->GetMaterialManager()->FromIMaterial(pStatObj->GetMaterial());
			}
		}
 		else if( pObj->IsKindOf( RUNTIME_CLASS(CEntityObject) ) )
 		{
			CEntityObject* pEntityObject = (CEntityObject*)pObj;
			IEntity* pEntity = pEntityObject->GetIEntity();
			if( pEntity )
			{
				int nSlotCount = pEntity->GetSlotCount();
				int nCurSlot = 0;
				while( nCurSlot < nSlotCount )
				{
					SEntitySlotInfo slotInfo;
					if( pEntity->GetSlotInfo( nCurSlot++, slotInfo ) == false )
						continue;
					if( !slotInfo.pStatObj )
						continue;
					s.m_pIndexedMesh = slotInfo.pStatObj->GetIndexedMesh();
					s.m_pMaterial = GetIEditor()->GetMaterialManager()->FromIMaterial(slotInfo.pStatObj->GetMaterial());
					if( s.m_pIndexedMesh == NULL )
					{
						CString sFilename(slotInfo.pStatObj->GetFilePath());
 						CContentCGF cgf(sFilename);
 						if( !GetIEditor()->Get3DEngine()->LoadChunkFileContent(&cgf, sFilename) )
 							continue;
						for (int i = 0; i < cgf.GetNodeCount(); i++)
						{
 							if(cgf.GetNode(i)->type != CNodeCGF::NODE_MESH)
								continue;
 							CMesh* pMesh = cgf.GetNode(i)->pMesh;
 							if(!pMesh)
 								continue;
							s.m_pIndexedMesh = GetIEditor()->Get3DEngine()->CreateIndexedMesh();
							s.m_pIndexedMesh->SetMesh(*pMesh);
							s.m_bLoadedIndexedMeshFromFile = true;
							break;
						}
					}
				}
			}
 		}
		if( s.m_pIndexedMesh == NULL )
			continue;
		s.m_worldTM = pObj->GetWorldTM();
		s.m_pOriginalObject = pObj;
		pObjects.push_back(s);
 	}
}

CDesignerBrushObject* CBrushDesignerConverter::CreateDesignerObject( IIndexedMesh* pMesh )
{
	if( pMesh == NULL )
		return NULL;

	CDesignerBrushObject* pDesignerObj = (CDesignerBrushObject*)GetIEditor()->NewObject("Designer", "");		

	if( !ConvertMeshToDesignerObject( pDesignerObj, pMesh ) )
	{
		GetIEditor()->DeleteObject(pDesignerObj);
		return NULL;
	}

	return pDesignerObj;
}

bool CBrushDesignerConverter::ConvertMeshToDesignerObject( CDesignerBrushObject* pDesignerObject, IIndexedMesh* pMesh  )
{
	if(ConvertMeshToBrushDesigner(pMesh, pDesignerObject->GetDesigner()))
	{
		pDesignerObject->UpdateBrush();
		return true;
	}

	return false;
}

bool CBrushDesignerConverter::ConvertMeshToBrushDesigner( IIndexedMesh* pMesh, CBrushDesigner* pDesigner )
{
	if( pMesh == NULL || pDesigner == NULL )
		return false;

	int numVerts = pMesh->GetVertexCount();
	int numFaces = pMesh->GetFaceCount();

	IIndexedMesh::SMeshDescription md;
	pMesh->GetMeshDescription(md);

	Vec3* const positions = pMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::POSITIONS);
	Vec3f16* const positionsf16 = pMesh->GetMesh()->GetStreamPtr<Vec3f16>(CMesh::POSITIONSF16);
	SMeshFace* const faces = pMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);
	vtx_idx* indices = pMesh->GetMesh()->GetStreamPtr<vtx_idx>(CMesh::INDICES);	

	if( positions == NULL && positionsf16 == NULL )
		return false;

	if( faces == NULL && indices == NULL )
		return false;

	if( numFaces == 0 )
	{
		numFaces = pMesh->GetIndexCount()/3;
		if( numFaces == 0 )
			return false;
	}

	for( int k = 0; k < numFaces; ++k )
	{
		int i0 = faces ? faces[k].v[0] : indices[k*3+0];
		int i1 = faces ? faces[k].v[1] : indices[k*3+1];
		int i2 = faces ? faces[k].v[2] : indices[k*3+2];

		if( i0 >= numVerts || i0 < 0 )
			continue;
		if( i1 >= numVerts || i1 < 0 )
			continue;
		if( i2 >= numVerts || i2 < 0 )
			continue;

		Vec3 v[3];

		if( positions )
		{
			v[0] = positions[i0];
			v[1] = positions[i1];
			v[2] = positions[i2];
		}
		else if( positionsf16 )
		{
			v[0] = positionsf16[i0].ToVec3();
			v[1] = positionsf16[i1].ToVec3();
			v[2] = positionsf16[i2].ToVec3();
		}

		// To avoid registering degenerated triangles 
		Vec3 d0 = (v[2]-v[1]).GetNormalized();
		Vec3 d1 = (v[0]-v[1]).GetNormalized();
		if( std::abs(d0.Dot(d1)) >= 1-kDesignerEpsilon )
			continue;

		std::vector<BrushVec3> facePoints;
		facePoints.reserve(3);

		facePoints.push_back(v[0]);
		facePoints.push_back(v[1]);
		facePoints.push_back(v[2]);
		BrushPlane plane(facePoints[0],facePoints[1],facePoints[2],kDesignerEpsilon);

		int nMatID = faces ? faces[k].nSubset : 0;
		CBrushRegion::RegionPtr pRegion = new CBrushRegion(facePoints,plane,nMatID,NULL,true);
		pDesigner->AddRegion(pRegion,CBrushDesigner::eOpType_Union);
	}

	return true;
}

bool CBrushDesignerConverter::ConvertSolidXMLToDesignerObject( XmlNodeRef pSolidNode, CDesignerBrushObject* pDesignerObject )
{
	if( pSolidNode->getChildCount() == 0 )
	{
		DESIGNER_ASSERT(0);
		return false;
	}

	const char* tag = pSolidNode->getChild(0)->getTag();

	if( !stricmp(tag,"Face") )
	{
		int numFaces = pSolidNode->getChildCount();
		std::vector<SSolidPolygon> polygonlist;
		std::vector<BrushVec3> vertexlist;
		for (int i = 0; i < numFaces; ++i)
		{
			XmlNodeRef faceNode = pSolidNode->getChild(i);
			if( faceNode == NULL )
				continue;
			if( faceNode->haveAttr("NumberOfPoints") )
			{
				SSolidPolygon polygon;
				LoadPolygon( &polygon, faceNode);
				polygonlist.push_back(polygon);
				if( i == 0 )
					LoadVertexList( vertexlist, faceNode );
			}
			else
			{
				DESIGNER_ASSERT(!"Can't convert an old type of solid");
				return true;
			}
		}
		AddRegionsToDesigner(polygonlist,vertexlist,pDesignerObject);
	}
	else if( !stricmp(tag,"Polygon") )
	{
		std::vector<SSolidPolygon> polylist;
		int numberOfChildren = pSolidNode->getChildCount();
		for( int i = 0; i < numberOfChildren; ++i )
		{
			XmlNodeRef polygonNode = pSolidNode->getChild(i);
			if( polygonNode && !strcmp( polygonNode->getTag(), "Polygon" ) )
			{
				SSolidPolygon polygon;
				LoadPolygon( &polygon, polygonNode );
				polylist.push_back(polygon);
			}
		}
		std::vector<BrushVec3> vertexlist;
		LoadVertexList( vertexlist, pSolidNode );

		AddRegionsToDesigner(polylist,vertexlist,pDesignerObject);
	}

	return true;
}

void CBrushDesignerConverter::AddRegionsToDesigner( const std::vector<SSolidPolygon>& polygonList, const std::vector<BrushVec3>& vList, CDesignerBrushObject* pDesignerObject )
{
	for( int i = 0, iPolySize(polygonList.size()); i < iPolySize; ++i )
	{
		const SSolidPolygon& solidPolygon = polygonList[i];
		std::vector<BrushVec3> reorderedVertexList;
		for( int k = 0, iVIndexCount(solidPolygon.vIndexList.size()); k < iVIndexCount; ++k )
			reorderedVertexList.push_back(vList[solidPolygon.vIndexList[k]]);
		if( reorderedVertexList.size() < 3 )
			continue;
		BrushPlane plane(reorderedVertexList[0],reorderedVertexList[1],reorderedVertexList[2],kDesignerEpsilon);
		CBrushRegion::RegionPtr pRegion = new CBrushRegion( reorderedVertexList, plane, solidPolygon.matID, &solidPolygon.texinfo, true );

		if( !pRegion->IsOpen() )
		{
			if( pRegion->GetBSPTree() && !pRegion->GetBSPTree()->HasNegativeNode() )
				pRegion->SetPlane(pRegion->GetPlane().GetInverted());
			pDesignerObject->GetDesigner()->AddRegion(pRegion,CBrushDesigner::eOpType_Add);
		}
	}
}

void CBrushDesignerConverter::LoadTexInfo( BUtil::STexInfo*	texinfo, const XmlNodeRef& node )
{
	Vec3 texScale( 1,1,1 );
	Vec3 texShift( 0,0,0 );
	node->getAttr( "TexScale",texScale );
	node->getAttr( "TexShift",texShift );
	node->getAttr( "TexRotate",texinfo->rotate );
	texinfo->scale[0] = texScale.x;
	texinfo->scale[1] = texScale.y;
	texinfo->shift[0] = texShift.x;
	texinfo->shift[1] = texShift.y;
}

void CBrushDesignerConverter::LoadPolygon( SSolidPolygon* polygon, const XmlNodeRef& polygonNode )
{
	int numberOfPoints = 0;
	polygonNode->getAttr("NumberOfPoints",numberOfPoints);

	int nCount = 0;
	while(1)
	{
		CString attribute;
		attribute.Format( "v%d", nCount++ );
		BUtil::VertexFaceIndexType vertexindex;
		bool ok = polygonNode->getAttr(attribute, vertexindex);
		if(ok == false)
			break;
		polygon->vIndexList.push_back(vertexindex);
	}

	int nMatId;
	polygonNode->getAttr( "MatId", nMatId );
	polygon->matID = nMatId;

	LoadTexInfo( &polygon->texinfo, polygonNode );
}

void CBrushDesignerConverter::LoadVertexList( std::vector<BrushVec3>& vertexlist, const XmlNodeRef& node )
{
	XmlNodeRef vertexNode = node->findChild( "Vertex" );
	int numberOfVertices;
	vertexNode->getAttr("NumberOfVertices",numberOfVertices);
	int nCount = 0;
	while(1)
	{
		Vec3 position;
		CString attribute;
		attribute.Format("p%d", nCount++);
		bool ok = vertexNode->getAttr(attribute, position);
		if(ok == false)
			break;
		vertexlist.push_back(ToBrushVec3(position));
	}
}