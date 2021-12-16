//===========================================================================*\
//	Part of Crytek Character Studio and Object Export Plug-in
//
//  Copyright: © Cuneyt Ozdas 2000, 2001
//				 cuneyt@cuneytozdas.com
//===========================================================================*/

#if !defined(AFX_VNORMAL_H__88C325A3_C0CE_4F3B_8DBE_72D510B3B073__INCLUDED_)
#define AFX_VNORMAL_H__88C325A3_C0CE_4F3B_8DBE_72D510B3B073__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Linked list of vertex normals
class VNormal 
{
	public:
		Point3 norm;
		DWORD smooth;
		VNormal *next;
		BOOL init;

		VNormal() {smooth=0;next=NULL;init=FALSE;norm=Point3(0,0,0);}
		VNormal(Point3 &n,DWORD s) {next=NULL;init=TRUE;norm=n;smooth=s;}
		~VNormal() {delete next;}
		void AddNormal(Point3 &n,DWORD s);
		Point3 &GetNormal(DWORD s);
		void Normalize();
};


#endif // !defined(AFX_VNORMAL_H__88C325A3_C0CE_4F3B_8DBE_72D510B3B073__INCLUDED_)
