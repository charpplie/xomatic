////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2009.
// -------------------------------------------------------------------------
//  File name:   IAnimationNameDragDropListener.h
//  Version:     v1.00
//  Created:     15/07/2009 by Pau Novau
//  Description: Interface for listening to drag and drop events from the
//               Animation List control.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef __I_Animation_Name_Drag_Drop_Listener__h__
#define __I_Animation_Name_Drag_Drop_Listener__h__
#pragma once

struct IAnimationNameDragDropListener
{
	virtual void DragStarted( const CPoint& screenPoint, const CString& animationName ) {}
	virtual void DragEnded( const CPoint& screenPoint, const CString& animationName ) {}

	virtual bool IsValidDropPoint( const CPoint& screenPoint, const CString& animationName ) const = 0;

	virtual void DoDrop( const CPoint& screenPoint, const CString& animationName ) = 0;
};

#endif
