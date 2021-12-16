//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------

#include "StdAfx.h"
#include "MaxVertexAnimation.h"
#include <algorithm>

MaxVertexAnimation::MaxVertexAnimation(INode* pNode)
:	pNode(pNode),
	nNumVerts(0),
	nNumFaces(0)
{
	this->ReadNode();
}

MaxVertexAnimation::~MaxVertexAnimation()
{
}

int MaxVertexAnimation::GetNumVerts()
{
	return this->nNumVerts;
}

int MaxVertexAnimation::GetNumFaces()
{
	return this->nNumFaces;
}

IVertexAnimationFrame* MaxVertexAnimation::GetFrame(TimeValue time)
{
	// Get the mesh from the node.
	Object *der_obj = this->pNode->GetObjectRef();
	if (!der_obj)
		return 0;
	Object *obj = der_obj->Eval(time).obj;
	SClass_ID	sid = obj->SuperClassID();
	if (!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
		return 0;
	TriObject* tri = (TriObject*)obj->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
	bool needDel = (obj != tri);
	Mesh* mesh = &tri->mesh;
	if (!mesh)
		return 0;

	// Create the frame object.
	MaxVertexAnimationFrame* pFrame = new MaxVertexAnimationFrame(mesh);

	// Clean up the temporary object.
	if (needDel)
		tri->DeleteThis();

	return pFrame;
}

void MaxVertexAnimation::ReadNode()
{
	// Get the mesh from the node.
	Object *der_obj = this->pNode->GetObjectRef();
	if (!der_obj)
		return;
	TimeValue	time	= 0;
	Object		*obj	= der_obj->Eval(time).obj;
	SClass_ID	sid		= obj->SuperClassID();
	if (!obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
		return;
	TriObject	*tri	= (TriObject *) obj->ConvertToType(time, Class_ID(TRIOBJ_CLASS_ID, 0));
	BOOL needDel = (obj != tri);
	Mesh* mesh	= &tri->mesh;
	if (!mesh)
		return;

	// Read the number of vertices from the mesh.
	this->nNumVerts = mesh->getNumVerts();
	this->nNumFaces = mesh->getNumFaces();

	// Clean up the mesh.
	if (needDel)
		tri->DeleteThis();
}

MaxVertexAnimationFrame::MaxVertexAnimationFrame(Mesh* pMesh)
{
	this->vertices.resize(pMesh->getNumVerts());
	std::copy(pMesh->verts, pMesh->verts + pMesh->getNumVerts(), this->vertices.begin());
	this->faces.resize(pMesh->getNumFaces());
	std::copy(pMesh->faces, pMesh->faces + pMesh->getNumFaces(), this->faces.begin());
}

MaxVertexAnimationFrame::~MaxVertexAnimationFrame()
{
}

int MaxVertexAnimationFrame::GetNumVertices()
{
	return int(this->vertices.size());
}

int MaxVertexAnimationFrame::GetNumFaces()
{
	return int(this->faces.size());
}

Point3* MaxVertexAnimationFrame::GetVertices()
{
	return &this->vertices[0];
}

Face* MaxVertexAnimationFrame::GetFaces()
{
	return &this->faces[0];
}
