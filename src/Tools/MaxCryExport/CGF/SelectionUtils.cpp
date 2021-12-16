//---------------------------------------------------------------------------
// Copyright 2005 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include <set>
#include "SelectionUtils.h"
#include "meshdelta.h"

namespace SelectionUtils
{
	Result GetMeshSelectionInterfaces(Object* pObject, IMeshSelect*& pMeshSelect, IMeshSelectData*& pMeshSelectData, std::string* psError);
	Result SelectFaces(IMeshSelect* pMeshSelect, IMeshSelectData* pMeshSelectData, const std::set<int>& faceIndices, std::string* psError);
	Result FindMeshSelectionInterfaceModifier(Object* pObject, IDerivedObject*& pReferencingObject, Object*& pMeshHolder, int& nModifierIndex, std::string* psError);
	Result CreateMeshSelectWrapper(INode* pNode, IDerivedObject* pReferencingObject, Object* pMeshHolder, int nModifierIndex, std::string* psError);
}

SelectionUtils::Result SelectionUtils::SelectFaces(Object* pObject, const std::set<int>& faceIndices, std::string* psError)
{
	// Set a default, catch-all error message.
	if (psError)
		*psError = "Internal Error.";

	IMeshSelect* pMeshSelect;
	IMeshSelectData* pMeshSelectData;
	SelectionUtils::Result result = GetMeshSelectionInterfaces(pObject, pMeshSelect, pMeshSelectData, psError);
	if (result == SelectionUtils::Result_Success)
	{
		// Using these interfaces, try to select the requested faces. If either of the interfaces was not found,
		// then this function will fail.
		result = SelectionUtils::SelectFaces(pMeshSelect, pMeshSelectData, faceIndices, psError);
	}
	return result;
}

SelectionUtils::Result SelectionUtils::CreateMeshSelectWrapper(INode* pNode, std::string* psError)
{
	// Set a default, catch-all error message.
	if (psError)
		*psError = "Internal Error.";

	// Find a modifier on top of which we can add an edit mesh.
	Object* pMeshHolder = 0;
	int nModifierIndex = -1;
	IDerivedObject* pReferencingObject;
	SelectionUtils::Result result = FindMeshSelectionInterfaceModifier(pNode->GetObjOrWSMRef(), pReferencingObject, pMeshHolder, nModifierIndex, psError);
	if (result == SelectionUtils::Result_Success)
	{
		// Create a mesh selection modifier on top of this modifier.
		result = CreateMeshSelectWrapper(pNode, pReferencingObject, pMeshHolder, nModifierIndex, psError);
	}

	return result;
}

// TODO: Refactor to use FindMeshSelectionInterfaceModifier to avoid duplication.
SelectionUtils::Result SelectionUtils::GetMeshSelectionInterfaces(Object* pObject, IMeshSelect*& pMeshSelect, IMeshSelectData*& pMeshSelectData, std::string* psError)
{
	// Loop through the list of derived objects, until we reach the base object or find a suitable modifier.
	for (; pObject->SuperClassID() == GEN_DERIVOB_CLASS_ID; pObject = static_cast<IDerivedObject*>(pObject)->GetObjRef())
	{
		// Get the derived object.
		IDerivedObject* pDerivedObject = static_cast<IDerivedObject*>(pObject);

		// Loop through the modifiers.
		for (int nModStackIndex = 0; nModStackIndex < pDerivedObject->NumModifiers(); ++nModStackIndex)
		{
			// Get current modifier.
			Modifier* pModifier = pDerivedObject->GetModifier(nModStackIndex);
			ModContext* pModContext = pDerivedObject->GetModContext(nModStackIndex);

			// Can we select faces with this modifier? Also, make sure that the object is an IMeshDeltaUser - this
			// includes Edit Mesh and Editable Mesh objects.
			if (pModContext->localData != 0)
			{
				pMeshSelect = GetMeshSelectInterface(pModifier);
				pMeshSelectData = GetMeshSelectDataInterface(pModContext->localData);
				MeshDeltaUser* pMeshDeltaUser = GetMeshDeltaUserInterface(pModifier);
				if (pMeshSelect != 0 && pMeshSelectData != 0 && (pMeshDeltaUser != 0 || pModifier->IsSubClassOf(Class_ID(MESHSELECT_CLASS_ID, 0))))
					return SelectionUtils::Result_Success;
			}
		}
	}

	// None of the modifiers was suitable - now pObject points to the base object. Check whether it
	// will do. Also, make sure that the object is an IMeshDeltaUser - this includes Edit Mesh and
	// Editable Mesh objects.
	pMeshSelect = GetMeshSelectInterface(pObject);
	pMeshSelectData = GetMeshSelectDataInterface(pObject);
	MeshDeltaUser* pMeshDeltaUser = GetMeshDeltaUserInterface(pObject);
	if (pMeshSelect != 0 && pMeshSelectData != 0 && pMeshDeltaUser != 0)
		return SelectionUtils::Result_Success;

	if (psError)
		*psError = "Cannot find modifier that allows selecting faces.";
	return SelectionUtils::Result_CannotFindMesh;
}

SelectionUtils::Result SelectionUtils::SelectFaces(IMeshSelect* pMeshSelect, IMeshSelectData* pMeshSelectData, const std::set<int>& faceIndices, std::string* psError)
{
	// We need the IMeshSelect and IMeshSelectData interfaces from the object. If they are not
	// available, then we cannot continue.
	if (pMeshSelect == 0 || pMeshSelectData == 0)
		return SelectionUtils::Result_CannotFindMesh;

	// Get the bit array indicating the current selection.
	BitArray selection = pMeshSelectData->GetFaceSel();

	// Update the bits to reflect the desired selection.
	selection.ClearAll();
	for (std::set<int>::const_iterator itFace = faceIndices.begin(); itFace != faceIndices.end(); ++itFace)
		selection.Set(*itFace);

	// Update the current selection with the changed bit array.
	pMeshSelectData->SetFaceSel(selection, pMeshSelect, 0);

	// Notify the object that we have updated it.
	pMeshSelect->LocalDataChanged();

	return SelectionUtils::Result_Success;
}

SelectionUtils::Result SelectionUtils::FindMeshSelectionInterfaceModifier(Object* pObject, IDerivedObject*& pReferencingObject, Object*& pMeshHolder, int& nModifierIndex, std::string* psError)
{
	pReferencingObject = 0;

	// Loop through the list of derived objects, until we reach the base object or find a suitable modifier.
	for (; pObject->SuperClassID() == GEN_DERIVOB_CLASS_ID; pObject = static_cast<IDerivedObject*>(pObject)->GetObjRef())
	{
		// Get the derived object.
		IDerivedObject* pDerivedObject = static_cast<IDerivedObject*>(pObject);

		// Loop through the modifiers.
		for (int nModStackIndex = 0; nModStackIndex < pDerivedObject->NumModifiers(); ++nModStackIndex)
		{
			// Get current modifier.
			Modifier* pModifier = pDerivedObject->GetModifier(nModStackIndex);
			ModContext* pModContext = pDerivedObject->GetModContext(nModStackIndex);

			// Can we select faces with this modifier?
			IMeshSelect* pMeshSelect = GetMeshSelectInterface(pModifier);
			if (pModContext->localData != 0)
			{
				IMeshSelectData* pMeshSelectData = GetMeshSelectDataInterface(pModContext->localData);
				if (pMeshSelect != 0 && pMeshSelectData != 0)
				{
					pMeshHolder = pObject;
					nModifierIndex = nModStackIndex;
					return SelectionUtils::Result_Success;
				}
			}
		}

		pReferencingObject = pDerivedObject;
	}

	// None of the modifiers was suitable - now pObject points to the base object. Check whether it
	// will do.
	IMeshSelect* pMeshSelect = GetMeshSelectInterface(pObject);
	IMeshSelectData* pMeshSelectData = GetMeshSelectDataInterface(pObject);
	if (pMeshSelect != 0 && pMeshSelectData != 0)
	{
		pMeshHolder = pObject;
		nModifierIndex = -1;
		return SelectionUtils::Result_Success;
	}

	if (psError)
		*psError = "Cannot find mesh or poly modifier.";
	return SelectionUtils::Result_CannotFindMesh;
}

SelectionUtils::Result SelectionUtils::CreateMeshSelectWrapper(INode* pNode, IDerivedObject* pReferencingObject, Object* pMeshHolder, int nModifierIndex, std::string* psError)
{
	// Make sure the input data is valid.
	if (nModifierIndex != -1 && pMeshHolder->SuperClassID() != GEN_DERIVOB_CLASS_ID)
		return SelectionUtils::Result_CannotAddMeshWrapper;

	// If the modifier index is -1, then the mesh we are to wrap is the base object. Therefore we need to create a derived object.
	IDerivedObject* pDerivedObject;
	if (nModifierIndex == -1)
	{
		pDerivedObject = static_cast<IDerivedObject*>(MakeObjectDerivedObject(pMeshHolder));
		if (pReferencingObject == 0)
			pNode->SetObjectRef(pDerivedObject);
		else
			pReferencingObject->ReferenceObject(pDerivedObject);
		nModifierIndex = 0;
	}
	else
	{
		pDerivedObject = static_cast<IDerivedObject*>(pMeshHolder);
	}

	//// Create the modifier context and initialise the local data. Note that we would normally not have to do this -
	//// the meshsel modifier would normally take care of this. However, it does not do so until the pipeline is invoked,
	//// so we have to do it ourselves here.
	//if (!pDerivedObject->IsSubClassOf(triObjectClassID))
	//{
	//	if (psError)
	//		*psError = "Mesh object does not support TriObject interface.h";
	//	return SelectionUtils::Result_CannotAddMeshWrapper;
	//}
	//TriObject *tobj = (TriObject*)pDerivedObject;
	//ModContext* pModContext = new ModContext(new Matrix3(1), 0, 0); 
	//if (!pModContext.localData)
	//	pModContext.localData = new MeshSelData(tobj->GetMesh());

	if (!pMeshHolder->CanConvertToType(triObjectClassID))
	{
		if (psError)
			*psError = "Mesh object does not support TriObject interface.h";
		return SelectionUtils::Result_CannotAddMeshWrapper;
	}

	// Add a modifier to the modifier stack of this derived object.
	Modifier* pModifier = static_cast<Modifier*>(CreateInstance(OSM_CLASS_ID, Class_ID(MESHSELECT_CLASS_ID, 0)));
	pDerivedObject->AddModifier(pModifier, 0, nModifierIndex);

	// Trick the modifier into initialising the local data - this only happens in ModifyObject();
	Object* pTriObject = pMeshHolder->ConvertToType(0, triObjectClassID);
	ObjectState state(pTriObject);
	pModifier->ModifyObject(0, *pDerivedObject->GetModContext(nModifierIndex), &state, pNode);
	if (pTriObject != pMeshHolder)
		pTriObject->DeleteMe();

	return SelectionUtils::Result_Success;
}
