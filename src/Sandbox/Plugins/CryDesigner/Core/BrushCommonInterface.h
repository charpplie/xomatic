#pragma once
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012
// -------------------------------------------------------------------------
//  File name:   BrushCommonInterface.h
//  Created:     March/19/2012 by Jaesik
//  Description: Brush common interface functions
//
////////////////////////////////////////////////////////////////////////////
class CBaseBrushCreator;
class CBaseBrush;
class CBrushDesigner;

class CBrushCommonInterface
{
public:
	static bool UpdateStatObjWithoutBackFaces( CBaseObject* pObj );
	static bool UpdateStatObj( CBaseObject* pObj );
	static bool UpdateGameResource( CBaseObject* pObj );
	static bool GetIStatObj( CBaseObject* pObj, _smart_ptr<IStatObj>* pOutStatObj );
	static bool GenerateGameFilename( CBaseObject* pObj, CString& outFileName );
	static bool GetBrushCreator( CBaseObject* pObj, CBaseBrushCreator*& pOutBrushCreator );
	static bool GetRenderFlag( CBaseObject* pObj, int& outRenderFlag );
	static bool GetBrush( CBaseObject* pObj, CBaseBrush*& pBrush );
	static bool GetDesigner( CBaseObject* pObj, CBrushDesigner*& pDesigner );
};