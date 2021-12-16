////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2001-2004.
// -------------------------------------------------------------------------
//  File name:   PrefabItem.cpp
//  Version:     v1.00
//  Created:     10/11/2003 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "PrefabItem.h"

#include "PrefabLibrary.h"
#include "PrefabManager.h"
#include "BaseLibraryManager.h"

#include "Settings.h"
#include "Grid.h"

#include "Objects\ObjectManager.h"
#include "Objects\PrefabObject.h"
#include "Objects\PrefabBuildingObject.h"
#include "Objects\SelectionGroup.h"

#include "ProceduralSystem/ProceduralCreation.h"

//////////////////////////////////////////////////////////////////////////
CPrefabItem::CPrefabItem()
{
	m_PrefabClassName = PREFAB_OBJECT_CLASS_NAME;
}

void CPrefabItem::SetPrefabClassName(CString prefabClassNameString) 
{
	if (GetFloorCount()==0) 
		SetFloorHeight(0,0);

	m_PrefabClassName = prefabClassNameString;
}

//////////////////////////////////////////////////////////////////////////
CPrefabItem::~CPrefabItem()
{

}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::Serialize( SerializeContext &ctx )
{
	CBaseLibraryItem::Serialize( ctx );
	XmlNodeRef node = ctx.node;
	if (ctx.bLoading)
	{
		//node->getAttr( "Origin",m_origin );
		XmlNodeRef objects = node->findChild( "Objects" );
		if (objects)
		{
			m_objectsNode = objects;
			LoadFloorHeights(ctx);
		}
	}
	else
	{
		//node->setAttr( "Origin",m_origin );
		if (m_objectsNode)
		{
			node->addChild( m_objectsNode );

			if (GetPrefabObjectClassName()==PREFABBUILDING_OBJECT_CLASS_NAME)
				SaveFloorHeights(ctx);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::Update()
{
	// Mark library as modified.
	SetModified();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::MakeFromSelection( CSelectionGroup &fromSelection )
{
	IObjectManager *pObjMan = GetIEditor()->GetObjectManager();
	CSelectionGroup selection;
	
	//////////////////////////////////////////////////////////////////////////
	// Clone selected objects, without changes thier names.
	//bool bPrevGenUniqNames = pObjMan->EnableUniqObjectNames( false );
	//fromSelection.Clone( selection );
	//pObjMan->EnableUniqObjectNames( bPrevGenUniqNames );
	selection.Copy( fromSelection );


	if (selection.GetCount() == 1)
	{
		CBaseObject *pObject = selection.GetObject(0);
		if (pObject->IsKindOf(RUNTIME_CLASS(CPrefabObject)))
		{
			CPrefabObject *pPrefabObject = (CPrefabObject*)pObject;
			pObjMan->SelectObject(pObject);
			return;
		}
	}

	// Snap center to grid.
	Vec3 vCenter = gSettings.pGrid->Snap( selection.GetBounds().min );

	//////////////////////////////////////////////////////////////////////////
	// Transform all objects in selection into local space of prefab.
	Matrix34 invParentTM;
	invParentTM.SetIdentity();
	invParentTM.SetTranslation( vCenter );
	invParentTM.Invert();

	CUndo undo( "Make Prefab" );
	int i;
	for (i = 0; i < selection.GetCount(); i++)
	{
		CBaseObject *pObj = selection.GetObject(i);
		Matrix34 localTM = invParentTM * pObj->GetWorldTM();
		pObj->SetLocalTM( localTM );
	}

	//GetIEditor()->SuspendUndo();

	//////////////////////////////////////////////////////////////////////////
	// Save all objects in flat selection to XML.
	CSelectionGroup flatSelection;
	selection.FlattenHierarchy( flatSelection );

	m_objectsNode = CreateXmlNode("Objects");
	CObjectArchive ar( pObjMan,m_objectsNode,false );
	for (i = 0; i < flatSelection.GetCount(); i++)
	{
		CBaseObject *pObj = flatSelection.GetObject(i);
		if (!pObj->CheckFlags(OBJFLAG_PREFAB))
		//if (!pObj ->IsKindOf(RUNTIME_CLASS(CPrefabObject)))		
			ar.SaveObject( pObj );
	}

	//////////////////////////////////////////////////////////////////////////
	// Delete all objects in cloned flat selection.
	//for (i = 0; i < flatSelection.GetCount(); i++)
	//{
	//	CBaseObject *pObj = flatSelection.GetObject(i);
	//	//if (!pObj->CheckFlags(OBJFLAG_PREFAB))
	//		//pObjMan->DeleteObject( pObj );
	//}

	//GetIEditor()->ResumeUndo();
	//////////////////////////////////////////////////////////////////////////
	// Create prefab object.
	CBaseObject *pObj = pObjMan->NewObject( PREFAB_OBJECT_CLASS_NAME );
	if (pObj && pObj->IsKindOf(RUNTIME_CLASS(CPrefabObject)))
	{
		CPrefabObject *pPrefabObj = (CPrefabObject*)pObj;

		pPrefabObj->SetUniqName( GetName() );
		pPrefabObj->SetPos( vCenter );
		pPrefabObj->SetPrefab( this,false );

		if(selection.GetCount())
			pObj->SetLayer(selection.GetObject(0)->GetLayer());
	}
	else if (pObj)
	{
		pObjMan->DeleteObject( pObj );
		pObj = 0;
	}

	//////////////////////////////////////////////////////////////////////////
	// Delete objects in original selection.
	//////////////////////////////////////////////////////////////////////////
	//selection.Copy( fromSelection );
	for (i = 0; i < selection.GetCount(); i++)
	{
		pObjMan->DeleteObject( selection.GetObject(i) );
	}

	if (pObj)
		pObjMan->SelectObject(pObj);

	SetModified();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabItem::UpdateFromPrefabObject( CPrefabObject *pPrefabObject  )
{
	IObjectManager *pObjMan = GetIEditor()->GetObjectManager();
	CSelectionGroup selection;
	CSelectionGroup flatSelection;
	int i;

	//////////////////////////////////////////////////////////////////////////
	// Save all objects in flat selection to XML.
	for (i = 0; i < pPrefabObject->GetChildCount(); i++)
	{
		CBaseObject *pChild = pPrefabObject->GetChild(i);
		if (!pChild->CheckFlags(OBJFLAG_PREFAB))
			continue;
		selection.AddObject( pChild );
	}

	selection.FlattenHierarchy( flatSelection );

	m_objectsNode = CreateXmlNode("Objects");
	CObjectArchive ar( pObjMan,m_objectsNode,false );
	for (i = 0; i < flatSelection.GetCount(); i++)
	{
		CBaseObject *pObj = flatSelection.GetObject(i);
		// Save only prefab objects.
		if (pObj->CheckFlags(OBJFLAG_PREFAB))
		{
      if (pObj->GetParent() == pPrefabObject)
				pObj->DetachThis(false);
			ar.SaveObject( pObj,false,true );
			if (!pObj->GetParent())
				pPrefabObject->AttachChild( pObj,false );
		}
	}

	SetModified();

	GetIEditor()->GetPrefabManager()->SetSelectedItem(this);
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_PREFAB_REMAKE );
}

void CPrefabItem::UpdateFromPrefabBuildingObject( CPrefabBuildingObject *pPrefabObject  )
{
	IObjectManager *pObjMan = GetIEditor()->GetObjectManager();
	CSelectionGroup selection;
	CSelectionGroup flatSelection;
	int i;

	//////////////////////////////////////////////////////////////////////////
	// Save all objects in flat selection to XML.
	for (i = 0; i < pPrefabObject->GetChildCount(); i++)
	{
		CBaseObject *pChild = pPrefabObject->GetChild(i);
		if (!pChild->CheckFlags(OBJFLAG_PREFAB))
			continue;
		selection.AddObject( pChild );
	}

	selection.FlattenHierarchy( flatSelection );

	m_objectsNode = CreateXmlNode("Objects");
	CObjectArchive ar( pObjMan,m_objectsNode,false );
	for (i = 0; i < flatSelection.GetCount(); i++)
	{
		CBaseObject *pObj = flatSelection.GetObject(i);
		// Save only prefab objects.
		if (pObj->CheckFlags(OBJFLAG_PREFAB))
		{
			if (pObj->GetParent() == pPrefabObject)
				pObj->DetachThis(false);
			ar.SaveObject( pObj,false,true );
			if (!pObj->GetParent())
				pPrefabObject->AttachChild( pObj,false );
		}
	}

	SetModified();
	GetIEditor()->GetPrefabManager()->SetSelectedItem(this);
	GetIEditor()->GetObjectManager()->SendEvent( EVENT_PREFAB_REMAKE );
	pPrefabObject->SetBuildingModified(false);
}

void CPrefabItem::LoadFloorHeights( SerializeContext &ctx)
{
	XmlNodeRef procSystemNode = ctx.node->findChild("ProceduralCreationSystem"); // Procedural Creation System main node <ProceduralCreationSystem> 
	if (!procSystemNode) return;

	XmlNodeRef buildingNode = procSystemNode->findChild("BuildingInfo");
	if (!buildingNode) return;

	XmlNodeRef floorHeightNode = buildingNode->findChild("floorHeights");
	if (!floorHeightNode) return;

	CString heightAttrNameStr = "height";
	CString heighDefinedByUserAttrNameStr = "heightDefinedByUser";

	for (int a=0;a<floorHeightNode->getChildCount();a++) //
	{
		XmlNodeRef newFloorHeightNode = floorHeightNode->getChild(a);

		float floorHeight=0;

		if (newFloorHeightNode->haveAttr(heightAttrNameStr))
			newFloorHeightNode->getAttr(heightAttrNameStr,floorHeight);

		SetFloorHeight(floorHeight,a);

		bool heightDefinedByUser=false;

		if (newFloorHeightNode->haveAttr(heighDefinedByUserAttrNameStr))
			newFloorHeightNode->getAttr(heighDefinedByUserAttrNameStr,heightDefinedByUser);
		else
			heightDefinedByUser=false;

		SetFloorHeightUserDefined(a,heightDefinedByUser);
	}

	// We need now to set procedural building prefab class name
	SetPrefabClassName(PREFABBUILDING_OBJECT_CLASS_NAME);
}

void CPrefabItem::SaveFloorHeights( SerializeContext &ctx)
{
	XmlNodeRef procNode = ctx.node->findChild("ProceduralCreationSystem");
	if (procNode)
		ctx.node->removeChild(procNode);

	if (GetPrefabObjectClassName()==PREFABBUILDING_OBJECT_CLASS_NAME)
	{
		ctx.node->addChild(CreateXmlNode("ProceduralCreationSystem"));
		XmlNodeRef newProcNode = ctx.node->findChild("ProceduralCreationSystem");
		newProcNode->addChild(CreateXmlNode("BuildingInfo"));
		XmlNodeRef buildingNode = newProcNode->findChild("BuildingInfo");
		buildingNode->addChild(CreateXmlNode("FloorHeights"));
		XmlNodeRef floorHeightNode = buildingNode->findChild("floorHeights");

		int floorsCount = GetFloorCount();
		for (int d=0;d<floorsCount;d++)
		{					
			CString floorNoStr;
			floorNoStr.Format("%d",d);				
			floorHeightNode->addChild(CreateXmlNode("floor"+floorNoStr));

			CString floorHeightStr;
			float floorHeight = GetFloorHeight(d);
			floorHeightStr.Format(_T("%f"), floorHeight );

			XmlNodeRef newFloorNode = floorHeightNode->findChild("floor"+floorNoStr);
			newFloorNode->setAttr("height",floorHeightStr);
			newFloorNode->setAttr("heightDefinedByUser",IsFloorHeightDefinedByUser(d));
		}
	}
}