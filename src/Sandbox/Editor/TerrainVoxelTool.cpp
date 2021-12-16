////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001.
// -------------------------------------------------------------------------
//  File name:   TerrainVoxelTool.cpp
//  Version:     v1.00
//  Created:     17/11/2004 by Sergiy Shaykin.
//  Compilers:   Visual C++ 6.0
//  Description: Terrain Voxel Painter Tool implementation.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "INITGUID.H"

#include <I3DEngine.h>
#include <IPhysics.h>
#include <IRenderAuxGeom.h>


#include "TerrainPanel.h"
#include "Viewport.h"
#include "TerrainVoxelPanel.h"
#include ".\Terrain\Heightmap.h"
#include "Objects\DisplayContext.h"
#include "Objects\ObjectManager.h"
#include "Objects\VoxelObject.h"
#include "TerrainVoxelTool.h"
#include "ITransformManipulator.h"

#include "CryEditDoc.h"
#include ".\Terrain\SurfaceType.h"
#include ".\Terrain\Layer.h"
#include "EditMode/ObjectMode.h"

#include "Terrain/TerrainManager.h"

class CUndoTerrainVoxelTool : public IUndoObject
{
public:
	CUndoTerrainVoxelTool( CTerrainVoxelTool *pTool )
	{
		m_tool = pTool;
		m_Undo = *(m_tool->GetAlignPlaneTM());
	}
protected:
	virtual int GetSize() { return sizeof(*this); }
	virtual const char* GetDescription() { return ""; };

	virtual void Undo( bool bUndo )
	{
		if (bUndo)
		{
			m_Redo = *(m_tool->GetAlignPlaneTM());
		}
		*(m_tool->GetAlignPlaneTM()) = m_Undo;
		m_tool->UpdateTransformManipulator();
	}
	virtual void Redo()
	{
		*(m_tool->GetAlignPlaneTM()) = m_Redo;
		m_tool->UpdateTransformManipulator();
	}
private:
	CTerrainVoxelTool * m_tool;
	Matrix34 m_Redo;
	Matrix34 m_Undo;
};


//////////////////////////////////////////////////////////////////////////
IMPLEMENT_DYNCREATE(CTerrainVoxelTool,CEditTool)

CTerrainVoxelBrush CTerrainVoxelTool::m_brush[eveoLast];
EVoxelEditOperation			CTerrainVoxelTool::m_currentBrushType = eveoPaintHeightPos;
EVoxelEditOperation			CTerrainVoxelTool::m_prevBrushType = eveoLast;
VoxelBrushShape			CTerrainVoxelTool::m_currentBrushShape = eVoxelBrushShapeSphere;

VoxelPaintParams		CTerrainVoxelTool::m_vpParams;


static IClassDesc *s_pClassTool = 0;

bool CTerrainVoxelTool::m_bIsPositionSetuped;
Matrix34 CTerrainVoxelTool::m_AlignPlaneTM;
float CTerrainVoxelTool::m_PlaneSize=100;



//////////////////////////////////////////////////////////////////////////
CTerrainVoxelTool::CTerrainVoxelTool()
{
  m_pObjectMode = NULL;

	m_voxObjs.empty();

	m_pClassDesc = s_pClassTool;

	SetStatusText( _T("Voxel Painter") );
	m_panelId = 0;
	m_panel = 0;

	m_bSmoothOverride = false;
	m_bQueryHeightMode = false;
	m_bPaintingMode = false;
	m_bLowerTerrain = false;

  for(int i=0; i<eveoLast; i++)
  {
    m_brush[i].type = (EVoxelEditOperation)i;
    m_brush[i].shape = eVoxelBrushShapeSphere;
  }

  m_currentBrushType = gEnv->p3DEngine->GetIVoxTerrain() ? eveoIntegrateMeshPos : eveoPaintHeightPos;

	m_pBrush = &m_brush[m_currentBrushType];
	
	m_pointerPos(0,0,0);
	
	//m_bIsPositionSetuped = false;
	m_bIsSetupPosition = false;
	m_IsPlaneAlign = false;
	//m_PlaneSize = 100.0f;
	
	m_AlignPlaneTM.SetIdentity();

	m_hPaintCursor = AfxGetApp()->LoadCursor( IDC_HAND_INTERNAL );
}

//////////////////////////////////////////////////////////////////////////
CTerrainVoxelTool::~CTerrainVoxelTool()
{
	m_pointerPos(0,0,0);

	if (GetIEditor()->IsUndoRecording())
		GetIEditor()->CancelUndo();
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::BeginEditParams( IEditor *ie,int flags )
{
	m_ie = ie;
	if (!m_panelId)
	{
		m_panel = new CTerrainVoxelPanel(this,AfxGetMainWnd());
		m_panelId = m_ie->AddRollUpPage( ROLLUP_TERRAIN,"Voxel Painter",m_panel );
		AfxGetMainWnd()->SetFocus();

		m_panel->SetBrush(*m_pBrush, m_vpParams.m_voxelType);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::EndEditParams()
{
	if (m_panelId)
	{
		m_ie->RemoveRollUpPage(ROLLUP_TERRAIN,m_panelId);
		m_panel = 0;
		m_panelId = 0;
	}
	SetupPosition(false);
	PlaneAlign(false);
}


					/*
					HitContext hitInfo;
					if (view->HitTest( point,hitInfo ))
						if(hitInfo.object->GetClassDesc()->GetObjectType()==OBJTYPE_VOXEL)
							m_pCurObject = (CVoxelObject*) hitInfo.object;
				
					if(m_pCurObject)
						CUndo::Record( new CUndoVoxelObject(m_pCurObject, hit.pt,fRadius) );
						*/


//////////////////////////////////////////////////////////////////////////
bool CTerrainVoxelTool::MouseCallback( CViewport *view,EMouseEvent event,CPoint &point,int flags )
{
  // redirect to objects manipulation mode when needed
  if(m_brush[m_currentBrushType].type == eveoIntegrateMeshPos || m_brush[m_currentBrushType].type == eveoIntegrateMeshNeg)
  {
    if(!m_pObjectMode)
      m_pObjectMode = new CObjectMode;

    return m_pObjectMode->MouseCallback( view,event,point,flags );
  }

  SAFE_DELETE(m_pObjectMode);

	EVoxelEditTarget vt = evetVoxelObjects;

	switch(event)
	{
	case eMouseLDown:
		{
			if(!m_bIsSetupPosition)
			{
				m_voxObjs.resize(0);

				if(vt == evetVoxelObjects && m_vpParams.m_isUndoInUse)
				{
					GetIEditor()->BeginUndo();
				}
			}
		}
		break;
	case eMouseLUp:
		if(!m_bIsSetupPosition)
		{
			if(vt == evetVoxelObjects && m_vpParams.m_isUndoInUse)
        GetIEditor()->AcceptUndo(gEnv->p3DEngine->GetIVoxTerrain() ? "Voxel terrain undo" : "Voxel object undo");
			view->ReleaseMouse();
            
      if(gEnv->p3DEngine->GetIVoxTerrain())
        gEnv->p3DEngine->GetIVoxTerrain()->OnMouse(true);
		}
		if(m_bIsSetupPosition)
		{
			m_bIsPositionSetuped = true;
      if( m_vpParams.m_isUndoInUse )
			  GetIEditor()->AcceptUndo( "Move Plane" );
		}
		break;
	}

	Vec3 raySrc,rayDir;
	GetIEditor()->GetActiveView()->ViewToWorldRay(point,raySrc,rayDir); rayDir.Normalize();

	float fRadius = m_brush[m_currentBrushType].radius;

	Vec3 pos;
	bool bIsComputedPos = false;

  // skip hidden objects
  int objTypes = ent_static|ent_terrain;
  int nFlags = rwi_stop_at_pierceable|rwi_ignore_terrain_holes;
  ray_hit hit;
  float fRayLen = gEnv->pSystem->GetViewCamera().GetFarPlane();
  bool bGoodHitFound = false;
  while(gEnv->pPhysicalWorld->RayWorldIntersection( raySrc,rayDir*fRayLen,objTypes,nFlags,&hit,1 ))
  {
    raySrc = hit.pt + rayDir*.01f;
    if(!hit.pCollider)
      continue;
    if(IRenderNode *pRenderNode= (IRenderNode*)hit.pCollider->GetForeignData(PHYS_FOREIGN_ID_STATIC))
    {
      if(pRenderNode->GetRndFlags()&ERF_HIDDEN)
        continue;

      if(pRenderNode->GetRenderNodeType() == eERType_Vegetation)
        if(gEnv->pConsole->GetCVar("e_Vegetation")->GetIVal()==0)
          continue;
    }
    bGoodHitFound = true;
    break;
  }

  if(m_brush[m_currentBrushType].type == eveoPickHeight)
  {
    if(bGoodHitFound)
    {
      pos = hit.pt;

      if(event == eMouseLDown)
      {
        m_brush[eveoPaintHeightPos].elevation = m_brush[eveoPaintHeightNeg].elevation = m_brush[eveoPickHeight].elevation = pos.z;
        UpdateUI();
        return true;
      }

      m_pointerPos = pos;
    }
  }
  else if((m_brush[m_currentBrushType].type == eveoPaintHeightPos || m_brush[m_currentBrushType].type == eveoPaintHeightNeg) && m_brush[m_currentBrushType].elevation>0)
  {
    if(bGoodHitFound)
    {
      pos = hit.pt;
      pos.z = m_brush[m_currentBrushType].elevation;
      bIsComputedPos = true;
    }
  }
	else if(m_IsPlaneAlign)
	{
		Vec3 p1 = m_AlignPlaneTM * Vec3( 0,0,0 );
		Vec3 p2 = m_AlignPlaneTM * Vec3( 1,0,0 );
		Vec3 p3 = m_AlignPlaneTM * Vec3( 0,1,0 );

		Plane plane = Plane::CreatePlane(p1,p2,p3);

		Vec3 output;
		if(Intersect::Ray_Plane( Ray(raySrc, rayDir), plane, output))
		{
			pos = output;
			bIsComputedPos = true;

			if(m_brush[m_currentBrushType].colision_offset>0)
			{
				Vec3 norm = m_AlignPlaneTM * Vec3( 0, 0, 1) - m_AlignPlaneTM * Vec3( 0, 0, 0); 
				norm.Normalize();
				float dot = norm.Dot(-rayDir);
				Vec3 newpos = pos + (-rayDir)*(fRadius/dot*m_brush[m_currentBrushType].colision_offset/100.0f);
				if((newpos-pos).len2()<256*fRadius*fRadius)
					pos = newpos;
			}
		}
	}
	else  // World Align (terrain, voxels, objects)
	{
    if(bGoodHitFound)
		{
/*      if(m_brush[m_currentBrushType].type == eveoPaintHeightPos)
        pos = hit.pt + Vec3(0,0,m_brush[m_currentBrushType].radius/2);
      else if(m_brush[m_currentBrushType].type == eveoPaintHeightNeg)
        pos = hit.pt - Vec3(0,0,m_brush[m_currentBrushType].radius/2);
      else*/
        pos = hit.pt;

			if(m_bIsSetupPosition && !m_bIsPositionSetuped)
			{
				ITransformManipulator *pManipulator = GetIEditor()->ShowTransformManipulator(true);
				m_AlignPlaneTM.SetIdentity();
				m_AlignPlaneTM.SetTranslation( pos );
				pManipulator->SetTransformation( COORDS_LOCAL, m_AlignPlaneTM );
			}

			if(!m_bIsSetupPosition)
			{
				if(m_brush[m_currentBrushType].colision_offset>0)
				{
					Vec3 axis = Vec3(1, 0, 0);
					if(fabs(rayDir.Cross(axis).len2())<0.1f)
						axis = Vec3(0, 1, 0);

					Vec3 rayTan = rayDir.Cross(axis);   rayTan.Normalize();
					Vec3 rayBin = rayDir.Cross(rayTan); rayBin.Normalize();

					Vec3 raySrc1 = raySrc + 0.5f * rayTan;
					Vec3 raySrc2 = raySrc + 0.5f * rayBin;
					
					Vec3 pos1, pos2;
					if(gEnv->pPhysicalWorld->RayWorldIntersection( raySrc1,rayDir*1000.0f,objTypes,nFlags,&hit,1 ))
					{
						pos1 = hit.pt;
						if(gEnv->pPhysicalWorld->RayWorldIntersection( raySrc2,rayDir*1000.0f,objTypes,nFlags,&hit,1))
						{
							pos2 = hit.pt;
							Vec3 norm = (pos2-pos).Cross(pos1-pos); norm.Normalize();
							float dot = norm.Dot(-rayDir);
							Vec3 newpos = pos + (-rayDir)*(fRadius/dot*m_brush[m_currentBrushType].colision_offset/100.0f);
							if((newpos-pos).len2()<16*fRadius*fRadius + fRadius*0.2f)
								pos = newpos;
						}
					}
				}
				bIsComputedPos = true;
			}
		}
	}


	if(bIsComputedPos)
	{
		m_pointerPos = pos;
		if(flags&MK_LBUTTON)
		{
			int nSurfaceTypeID = -1;
      
      bool bVoxelTerrain = 0;
      if( ICVar * pHMECvar = gEnv->pConsole->GetCVar("e_VoxTer") )
        bVoxelTerrain = pHMECvar->GetIVal()!=0;

			if(strlen(m_vpParams.m_SurfType) || bVoxelTerrain)
			{
        if(gEnv->p3DEngine->GetIVoxTerrain())
        { // find selected terrain layer
          if(CTerrainManager::GetTerrainManager().GetLayerCount())
            nSurfaceTypeID = CTerrainManager::GetTerrainManager().GetLayer(0)->GetCurrentLayerId();
          else
            nSurfaceTypeID = 0;

          for(int l=0; l<CTerrainManager::GetTerrainManager().GetLayerCount(); l++)
          {
            if(CTerrainManager::GetTerrainManager().GetLayer(l)->IsSelected())
            {
              nSurfaceTypeID = CTerrainManager::GetTerrainManager().GetLayer(l)->GetCurrentLayerId();
              break;
            }
          }
        }
        else
          nSurfaceTypeID = CTerrainManager::GetTerrainManager().FindSurfaceType( m_vpParams.m_SurfType );

				EVoxelEditOperation eOperation = m_brush[m_currentBrushType].type;

				EVoxelBrushShape eShape;
				if(m_brush[m_currentBrushType].shape==eVoxelBrushShapeSphere)
					eShape = evbsSphere;
				else //if(m_brush[m_currentBrushType].shape==eVoxelBrushShapeBox)
					eShape = evbsBox;

				//if (CUndo::IsRecording())
					//CUndo::Record( new CUndoVoxelModify(pos,fRadius, vt == evetVoxelTerrain) );

        bool bSurfTypeNeeded = 
          ( eOperation==eveoCreate || 
            eOperation==eveoPaintHeightPos || 
            eOperation==eveoPaintHeightNeg || 
            eOperation==eveoSubstract || 
            eOperation==eveoMaterial || 
            eOperation==eveoBaseColor || 
            eOperation==eveoIntegrateMeshPos || 
            eOperation==eveoIntegrateMeshNeg || 
            eOperation==eveoCopyTerrainPos || eOperation==eveoCopyTerrainNeg);

				if(nSurfaceTypeID>=0 || !bSurfTypeNeeded)
				{
					if(vt == evetVoxelObjects && !gEnv->p3DEngine->GetIVoxTerrain())
					{
						_smart_ptr<IMemoryBlock> pMem = gEnv->p3DEngine->Voxel_GetObjects(pos, fRadius, 
						(eOperation==eveoCreate || eOperation==eveoSubstract || eOperation==eveoPaintHeightPos || eOperation==eveoPaintHeightNeg || eOperation==eveoMaterial || eOperation==eveoBaseColor) ? nSurfaceTypeID : -1, 
						eOperation, eShape, vt);

						IVoxelObject ** pData = (IVoxelObject **) pMem->GetData();
						int size = pMem->GetSize()/4;

						for(int i=0; i<size; i++)
						{
							bool find = false;
							for(std::vector<IVoxelObject*>::iterator it=m_voxObjs.begin(); it!=m_voxObjs.end(); it++)
							{
								if(pData[i] == *it)
								{
									find = true;
									break;
								}
							}
							if(!find)
							{
								m_voxObjs.push_back(pData[i]);
								if (CUndo::IsRecording())
								{
									CBaseObjectsArray objects;
									GetIEditor()->GetObjectManager()->GetObjects( objects );
									for (int j=0; j < objects.size(); j++)
									{  
										CBaseObject *obj = objects[j];
										if(obj->GetClassDesc()->GetObjectType()==OBJTYPE_VOXEL)
										{
											CVoxelObject * voxObj = (CVoxelObject *)obj;
											if(voxObj->GetRenderNode()==pData[i])
											{
												CUndo::Record( new CUndoVoxelObject( voxObj ));
												break;
											}
										}
									}
								}
							}
						}
					}
          else if( gEnv->p3DEngine->GetIVoxTerrain() && CUndo::IsRecording() && fRadius*4 > GetIEditor()->Get3DEngine()->GetTerrainSize() )
          {
            AABB sectorBox; sectorBox.Reset();
            CUndo::Record( new CUndoVoxTerrain( gEnv->p3DEngine->GetIVoxTerrain(), sectorBox ) );                  
          }
          else if( gEnv->p3DEngine->GetIVoxTerrain() && CUndo::IsRecording() )
          {
            // detect access to new sectors and save it content before it is modified
            // make sure that first click affects only one sector

            int nMapSize = GetIEditor()->Get3DEngine()->GetTerrainSize();

            int nUndoSectorSize = 1024*16;
            while( nUndoSectorSize > 4 && nUndoSectorSize > fRadius*8 )
              nUndoSectorSize /= 2;

            static Vec3 vUndoStartPos(0,0,0);
            static std::map<int,class CUndoVoxTerrain*> undoMap;

            if(eMouseLDown == event)
            {
              undoMap.clear();
              vUndoStartPos = pos;
              vUndoStartPos.z = 0;
            }

            AABB brushBox;
            float r = fRadius*1.5f;
            brushBox.min = pos - Vec3(r,r,r);
            brushBox.max = pos + Vec3(r,r,r);

            int x1 = floor((brushBox.min.x - vUndoStartPos.x + nUndoSectorSize/2) / nUndoSectorSize);
            int y1 = floor((brushBox.min.y - vUndoStartPos.y + nUndoSectorSize/2) / nUndoSectorSize);
            int x2 = ceil ((brushBox.max.x - vUndoStartPos.x + nUndoSectorSize/2) / nUndoSectorSize);
            int y2 = ceil ((brushBox.max.y - vUndoStartPos.y + nUndoSectorSize/2) / nUndoSectorSize);

            for(int x=x1; x<x2; x++)
            {
              for(int y=y1; y<y2; y++)
              {
                int key = (int)(x*nMapSize + y);

                if(undoMap.find(key) == undoMap.end())
                {
                  AABB sectorBox;
                  
                  sectorBox.min.Set(x*nUndoSectorSize,                  y*nUndoSectorSize, 0);
                  sectorBox.max.Set(x*nUndoSectorSize+nUndoSectorSize,  y*nUndoSectorSize+nUndoSectorSize, nMapSize);
                  
                  float fEpsilon = 0.01f;
                  sectorBox.min -= Vec3(fEpsilon,fEpsilon,fEpsilon);
                  sectorBox.max += Vec3(fEpsilon,fEpsilon,fEpsilon);
                  
                  sectorBox.min += vUndoStartPos - Vec3(nUndoSectorSize/2,nUndoSectorSize/2,0);
                  sectorBox.max += vUndoStartPos - Vec3(nUndoSectorSize/2,nUndoSectorSize/2,0);

                  CUndoVoxTerrain * pUndo = new CUndoVoxTerrain( gEnv->p3DEngine->GetIVoxTerrain(), sectorBox );                  
                  CUndo::Record( pUndo );                  

                  undoMap[key] = pUndo;
                  /*
                  sectorBox.min.z = pos.z-1.f;
                  sectorBox.max.z = pos.z+1.f;
                  gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(sectorBox,true,Col_White,eBBD_Faceted);
                  */
                }
              }
            }
          }

					ColorF cfColor = ColorF((unsigned int) m_vpParams.m_Color );

          // reset layer brightness
          /*if(eOperation==eveoCopyTerrainPos || eOperation==eveoCopyTerrainNeg)
          {
            for(int l=0; l<CTerrainManager::GetTerrainManager().GetLayerCount(); l++)
            {
              CLayer * pLayer = CTerrainManager::GetTerrainManager().GetLayer(l);
              pLayer->SetLayerBrightness(0.25f); // RGB values are multiplied by 4 during copying into octree
            }
          }*/

					gEnv->p3DEngine->Voxel_Paint(pos, fRadius, 
            bSurfTypeNeeded ? nSurfaceTypeID : -1, Vec3(cfColor.r,cfColor.g,cfColor.b), 
            eOperation, eShape, vt, NULL, m_brush[m_currentBrushType].minVoxelSize);
				}
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::Display( DisplayContext &dc )
{
  if(m_pObjectMode)
    return m_pObjectMode->Display( dc );

	if(m_bIsSetupPosition || m_IsPlaneAlign)
	{
		dc.SetColor( RGB(64,0,0),0.4f);
		Vec3 p1 = m_AlignPlaneTM * Vec3(-m_PlaneSize/2,-m_PlaneSize/2, 0);
		Vec3 p2 = m_AlignPlaneTM * Vec3( m_PlaneSize/2,-m_PlaneSize/2, 0);
		Vec3 p3 = m_AlignPlaneTM * Vec3( m_PlaneSize/2, m_PlaneSize/2, 0);
		Vec3 p4 = m_AlignPlaneTM * Vec3(-m_PlaneSize/2, m_PlaneSize/2, 0);
		dc.DrawQuad(p1, p2, p3, p4);

		for(int i=-10; i<=10; i++)
		{
			if(i==-10 || i==10)
			{
				if(m_bIsSetupPosition)
					dc.SetColor(dc.GetSelectedColor());
				else
					dc.SetColor( RGB(255,255,128), 1);
			}
			else
				dc.SetColor( RGB(0,64,0), 1);
			Vec3 pos1 = m_AlignPlaneTM * Vec3(m_PlaneSize*i/10/2,-m_PlaneSize/2, 0);
			Vec3 pos2 = m_AlignPlaneTM * Vec3(m_PlaneSize*i/10/2, m_PlaneSize/2, 0);
			dc.DrawLine(pos1, pos2);
			pos1 = m_AlignPlaneTM * Vec3(-m_PlaneSize/2, m_PlaneSize*i/10/2, 0);
			pos2 = m_AlignPlaneTM * Vec3( m_PlaneSize/2, m_PlaneSize*i/10/2, 0);
			dc.DrawLine(pos1, pos2);
		}
	}

	if(!m_bIsSetupPosition)
	{
		ColorB color(65,154,235,25);
		float radius = m_brush[m_currentBrushType].radius;

    /*if(((m_brush[m_currentBrushType].type==eveoPaintHeightPos || m_brush[m_currentBrushType].type==eveoPaintHeightNeg) && m_brush[m_currentBrushType].elevation>0) 
      || m_brush[m_currentBrushType].type==eveoPickHeight)
    {
      Vec3 vTopPos = m_pointerPos;
      vTopPos.z = m_brush[m_currentBrushType].elevation;
      dc.SetColor( 1,1,0,1 );
      float fTerrainZ = gEnv->p3DEngine->GetTerrainElevation( vTopPos.x, vTopPos.y, true );
      dc.DrawCircle( vTopPos, radius/2 );
      Vec3 pointerPosOnTerrain = vTopPos;
      pointerPosOnTerrain.z = fTerrainZ;
      dc.DrawLine( vTopPos, pointerPosOnTerrain );
    }*/
    
    if(m_brush[m_currentBrushType].shape==eVoxelBrushShapeSphere)
    {
      IMaterial * pHelperMat = gEnv->p3DEngine->GetMaterialManager()->LoadMaterial("Editor/Materials/voxel_editor");
      if(gEnv->p3DEngine->GetIVoxTerrain() && pHelperMat)
        gEnv->p3DEngine->GetIVoxTerrain()->DrawEditingHelper(Sphere(m_pointerPos,radius), m_brush[m_currentBrushType].type, pHelperMat);
      else
        gEnv->pRenderer->GetIRenderAuxGeom()->DrawSphere(m_pointerPos,radius,color);
    }
		else if(m_brush[m_currentBrushType].shape==eVoxelBrushShapeBox)
		{
			AABB aabbBox(m_pointerPos-Vec3(radius,radius,radius),m_pointerPos+Vec3(radius,radius,radius));
			gEnv->pRenderer->GetIRenderAuxGeom()->DrawAABB(aabbBox,true,color,eBBD_Faceted);
		}
	}


	/*
	if (dc.flags & DISPLAY_2D)
		return;

	if (m_pBrush->type != eVoxelBrushSmooth)
	{
		dc.SetColor( 0.5f,1,0.5f,1 );
		dc.DrawTerrainCircle( m_pointerPos,m_pBrush->radiusInside,0.2f );
	}
	dc.SetColor( 0,1,0,1 );
	dc.DrawTerrainCircle( m_pointerPos,m_pBrush->radius,0.2f );
	if (m_pointerPos.z < m_pBrush->height)
	{
		if (m_pBrush->type != eVoxelBrushSmooth)
		{
			Vec3 p = m_pointerPos;
			p.z = m_pBrush->height;
			dc.SetColor( 1,1,0,1 );
			if (m_pBrush->type == eVoxelBrushFlatten)
				dc.DrawTerrainCircle( p,m_pBrush->radius,m_pBrush->height-m_pointerPos.z );
			dc.DrawLine( m_pointerPos,p );
		}
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainVoxelTool::OnKeyDown( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
  if(m_pObjectMode)
    return m_pObjectMode->OnKeyDown(view,nChar,nRepCnt,nFlags);

	bool bProcessed = false;
	if (nChar == VK_ADD)
	{
		m_pBrush->radius += 1;
		bProcessed = true;
	}
	if (nChar == VK_SUBTRACT)
	{
		if (m_pBrush->radius > 1)
			m_pBrush->radius -= 1;
		bProcessed = true;
	}

  if (nChar == VK_SHIFT)
  {
    if(m_prevBrushType == eveoLast)
      m_prevBrushType	=	m_currentBrushType;
    SetActiveBrushType(eveoBlurNeg);
    bProcessed = true;
  }
  if (nChar == VK_CONTROL)
  {
    if(m_prevBrushType == eveoLast)
      m_prevBrushType	=	m_currentBrushType;
    SetActiveBrushType(eveoBlurPos);
    bProcessed = true;
  }

	if (bProcessed)
	{
		UpdateUI();
	}
	return bProcessed;
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainVoxelTool::OnKeyUp( CViewport *view,uint32 nChar,uint32 nRepCnt,uint32 nFlags )
{
  if(m_pObjectMode)
    return m_pObjectMode->OnKeyUp(view,nChar,nRepCnt,nFlags);

	bool bProcessed = false;
	if (nChar == VK_SHIFT)
	{
		if(m_prevBrushType != eveoLast)
			SetActiveBrushType(m_prevBrushType);
		m_prevBrushType	= eveoLast;
		bProcessed = true;
	}
	if (nChar == VK_CONTROL)
	{
		if(m_prevBrushType != eveoLast)
			SetActiveBrushType(m_prevBrushType);
		m_prevBrushType	= eveoLast;
		bProcessed = true;
	}
	if (bProcessed)
	{
		UpdateUI();
	}
	return bProcessed;
}


//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::UpdateUI()
{
	if (m_panel)
	{
		m_panel->SetBrush(*m_pBrush, m_vpParams.m_voxelType);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::Paint()
{
	/*
	CHeightmap *heightmap = GetIEditor()->GetHeightmap();
	int unitSize = heightmap->GetUnitSize();

	//dc.renderer->SetMaterialColor( 1,1,0,1 );
	int tx = RoundFloatToInt(m_pointerPos.y / unitSize);
	int ty = RoundFloatToInt(m_pointerPos.x / unitSize);

	float fInsideRadius = (m_pBrush->radiusInside / unitSize);
	int tsize = (m_pBrush->radius / unitSize);
	if (tsize == 0)
		tsize = 1;
	int tsize2 = tsize*2;
	int x1 = tx - tsize;
	int y1 = ty - tsize;

	if (m_pBrush->type == eVoxelBrushFlatten && !m_bSmoothOverride)
		heightmap->DrawSpot2( tx,ty,tsize,fInsideRadius,m_pBrush->height,m_pBrush->hardness,m_pBrush->bNoise,m_pBrush->noiseFreq/10.0f,m_pBrush->noiseScale/1000.0f );
	if (m_pBrush->type == eVoxelBrushRiseLower && !m_bSmoothOverride)
	{
		float h = m_pBrush->height;
		if (m_bLowerTerrain)
			h = -h;
		heightmap->RiseLowerSpot( tx,ty,tsize,fInsideRadius,h,m_pBrush->hardness,m_pBrush->bNoise,m_pBrush->noiseFreq/10.0f,m_pBrush->noiseScale/1000.0f );
	}
	else if (m_pBrush->type == eVoxelBrushSmooth || m_bSmoothOverride)
		heightmap->SmoothSpot( tx,ty,tsize,m_pBrush->height,m_pBrush->hardness );//,m_pBrush->noiseFreq/10.0f,m_pBrush->noiseScale/10.0f );

	heightmap->UpdateEngineTerrain( x1,y1,tsize2,tsize2,true,false );

	if (m_pBrush->bRepositionObjects)
	{
		BBox box;
		box.min = m_pointerPos - Vec3(m_pBrush->radius,m_pBrush->radius,0);
		box.max = m_pointerPos + Vec3(m_pBrush->radius,m_pBrush->radius,0);
		box.min.z -= 10000;
		box.max.z += 10000;
		// Make sure objects preserve height.
		GetIEditor()->GetObjectManager()->SendEvent( EVENT_KEEP_HEIGHT,box );
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
bool CTerrainVoxelTool::OnSetCursor( CViewport *vp )
{
  if(m_pObjectMode)
    return m_pObjectMode->OnSetCursor( vp );

	if (m_bQueryHeightMode)
	{
		//SetCursor( m_hPickCursor );
		//return true;
	}
	/*
	if (m_bPaintingMode)
	{
		SetCursor( m_hPaintCursor );
		return true;
	}
	*/
	//if ((m_pBrush->type == eVoxelBrushFlatten || m_pBrush->type == eVoxelBrushRiseLower) && !m_bSmoothOverride)
	{
		//SetCursor( m_hFlattenCursor );
		//return true;
	}
	//else if (m_pBrush->type == eVoxelBrushSmooth || m_bSmoothOverride)
	{
		//SetCursor( m_hSmoothCursor );
		//return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::SetActiveBrushType( EVoxelEditOperation type  )
{
	m_currentBrushType = type;
	m_pBrush = &m_brush[m_currentBrushType];
	if (m_panel)
	{
		CTerrainVoxelBrush brush;
		GetBrush( brush );
		m_panel->SetBrush( brush, m_vpParams.m_voxelType );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::SetActiveBrushShape( VoxelBrushShape shape  )
{
	m_currentBrushShape = shape;
	m_pBrush = &m_brush[m_currentBrushType];
	m_pBrush->shape = shape;
	if (m_panel)
	{
		CTerrainVoxelBrush brush;
		GetBrush( brush );
		brush.shape = shape;
		m_panel->SetBrush( brush, m_vpParams.m_voxelType );
	}
}


//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::SetActiveVoxelObjectType( int type )
{
	m_vpParams.m_voxelType = type;
}



//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::Command_Activate()
{
	/*
	CEditTool *pTool = GetIEditor()->GetEditTool();
	if (pTool && pTool->IsKindOf(RUNTIME_CLASS(CTerrainVoxelTool)))
	{
		// Already active.
		return;
	}
	pTool = new CTerrainVoxelTool;
	GetIEditor()->SetEditTool( pTool );
	GetIEditor()->SelectRollUpBar( ROLLUP_TERRAIN );
	AfxGetMainWnd()->RedrawWindow(NULL,NULL,RDW_INVALIDATE|RDW_ALLCHILDREN);
	*/
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::Command_Flatten()
{
	/*
	Command_Activate();
	CEditTool *pTool = GetIEditor()->GetEditTool();
	if (pTool && pTool->IsKindOf(RUNTIME_CLASS(CTerrainVoxelTool)))
	{
		CTerrainVoxelTool *pModTool = (CTerrainVoxelTool*)pTool;
		pModTool->SetActiveBrushType(eVoxelBrushFlatten);
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::Command_Smooth()
{
	/*
	Command_Activate();
	CEditTool *pTool = GetIEditor()->GetEditTool();
	if (pTool && pTool->IsKindOf(RUNTIME_CLASS(CTerrainVoxelTool)))
	{
		CTerrainVoxelTool *pModTool = (CTerrainVoxelTool*)pTool;
		pModTool->SetActiveBrushType(eVoxelBrushSmooth);
	}
	*/
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::Command_RiseLower()
{
	/*
	Command_Activate();
	CEditTool *pTool = GetIEditor()->GetEditTool();
	if (pTool && pTool->IsKindOf(RUNTIME_CLASS(CTerrainVoxelTool)))
	{
		CTerrainVoxelTool *pModTool = (CTerrainVoxelTool*)pTool;
		pModTool->SetActiveBrushType(eVoxelBrushRiseLower);
	}
	*/
}


//////////////////////////////////////////////////////////////////////////
// Class description.
//////////////////////////////////////////////////////////////////////////
class CTerrainVoxelTool_ClassDesc : public CRefCountClassDesc
{
	//! This method returns an Editor defined GUID describing the class this plugin class is associated with.
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_EDITTOOL; }

	//! Return the GUID of the class created by plugin.
	virtual REFGUID ClassID() 
	{
		return TERRAIN_VOXEL_TOOL_GUID;
	}

	//! This method returns the human readable name of the class.
	virtual const char* ClassName() { return "EditTool.VoxelPaint"; };

	//! This method returns Category of this class, Category is specifing where this plugin class fits best in
	//! create panel.
	virtual const char* Category() { return "Terrain"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTerrainVoxelTool); }
	//////////////////////////////////////////////////////////////////////////
};





//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::RegisterTool( CRegistrationContext &rc )
{
	rc.pClassFactory->RegisterClass( s_pClassTool = new CTerrainVoxelTool_ClassDesc );
	
	//rc.pCommandManager->RegisterCommand( "EditTool.TerrainVoxelTool.Activate",functor(CTerrainVoxelTool::Command_Activate) );
	//rc.pCommandManager->RegisterCommand( "EditTool.TerrainVoxelTool.Flatten",functor(CTerrainVoxelTool::Command_Flatten) );
	//rc.pCommandManager->RegisterCommand( "EditTool.TerrainVoxelTool.Smooth",functor(CTerrainVoxelTool::Command_Smooth) );
	//rc.pCommandManager->RegisterCommand( "EditTool.TerrainVoxelTool.RiseLower",functor(CTerrainVoxelTool::Command_RiseLower) );
}


void CTerrainVoxelTool::OnManipulatorDrag( CViewport *view,ITransformManipulator *pManipulator,CPoint &p0,CPoint &p1,const Vec3 &value )
{
  if(m_pObjectMode)
    return m_pObjectMode->OnManipulatorDrag( view,pManipulator,p0,p1,value );

	int editMode = GetIEditor()->GetEditMode();
	if (editMode == eEditModeMove)
	{
		GetIEditor()->RestoreUndo();

		if (CUndo::IsRecording())
			CUndo::Record( new CUndoTerrainVoxelTool(this) );

		Vec3 pos = m_AlignPlaneTM.GetTranslation();
		m_AlignPlaneTM.SetTranslation( pos + value);
		pManipulator->SetTransformation( COORDS_LOCAL, m_AlignPlaneTM );
	}
	else 
	if (editMode == eEditModeRotate)
	{
		GetIEditor()->RestoreUndo();
		if (CUndo::IsRecording())
			CUndo::Record( new CUndoTerrainVoxelTool(this) );

		Matrix34 rotateTM = Matrix34::CreateRotationXYZ( Ang3(value) );
		m_AlignPlaneTM = m_AlignPlaneTM * rotateTM;
		pManipulator->SetTransformation( COORDS_LOCAL, m_AlignPlaneTM );
	}
}


//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::SetupPosition(bool bIsSetup)
{
	if(bIsSetup)
	{
		ITransformManipulator *pManipulator = GetIEditor()->ShowTransformManipulator(true);
		if(!m_bIsPositionSetuped)
		{
			m_AlignPlaneTM.SetIdentity();
			m_AlignPlaneTM.SetTranslation( m_pointerPos );
		}
		pManipulator->SetTransformation( COORDS_LOCAL, m_AlignPlaneTM );
	}

	if(!bIsSetup && m_bIsSetupPosition)
	{
		m_bIsPositionSetuped = true;
		GetIEditor()->ShowTransformManipulator(false);
	}

	m_bIsSetupPosition = bIsSetup;
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::PlaneAlign(bool bIsAlign)
{
	m_IsPlaneAlign = bIsAlign;
	if(bIsAlign)
		GetIEditor()->ShowTransformManipulator(false);
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::UpdateTransformManipulator()
{
	if(m_bIsSetupPosition)
	{
		ITransformManipulator *pManipulator = GetIEditor()->ShowTransformManipulator(true);
		pManipulator->SetTransformation( COORDS_LOCAL, m_AlignPlaneTM );
	}
}

//////////////////////////////////////////////////////////////////////////
void CTerrainVoxelTool::InitPosition()
{
	m_bIsPositionSetuped = false;
	m_AlignPlaneTM.SetIdentity();
}