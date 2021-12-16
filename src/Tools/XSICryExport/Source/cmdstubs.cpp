//***************************************************************************************
//
// File supervisor: Softimage Rendering & Pipeline team
//
// (c) Copyright 2001-2005 Avid Technology, Inc. . All rights reserved.
//
//***************************************************************************************

/****************************************************************************************
THIS CODE IS PUBLISHED AS A SAMPLE ONLY AND IS PROVIDED "AS IS".
IN NO EVENT SHALL SOFTIMAGE, AVID TECHNOLOGY, INC. AND/OR THEIR RESPECTIVE
SUPPLIERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS CODE .

COPYRIGHT NOTICE. Copyright © 1999-2005 Avid Technology Inc. . All rights reserved. 

SOFTIMAGE is a registered trademark of Avid Technology Inc. or its subsidiaries 
or divisions. Windows NT is a registered trademark of Microsoft Corp. All other
trademarks contained herein are the property of their respective owners. 
****************************************************************************************/

/*! \file cmdstubs.cpp   
    command function stubs for XSI v4.2.2004.1216-QFE2
*/

#include "stdafx.h"
#include "cmdstubs.h"

namespace XSI {
namespace COMMANDS {

Application gApp;

// AbortFramePreview
void AbortFramePreview( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"AbortFramePreview", args, retval );

	return;
}

// AcclaimImport
void AcclaimImport( const CString& in_skeletonfilename, const CString& in_motionfilename, bool in_cropmotionvaluestolimit, bool in_overridebonelengthmultiplier, const CValue& /*number*/ in_bonelengthmultiplier, bool in_insermotion, bool in_nullshierarchy, bool in_gaplessskeleton, bool in_si3dchains, bool in_groupanimobjs )
{
	CValueArray args(10);
	CValue retval;
	LONG i(0);

	args[i++]= in_skeletonfilename;
	args[i++]= in_motionfilename;
	args[i++]= in_cropmotionvaluestolimit;
	args[i++]= in_overridebonelengthmultiplier;
	args[i++]= in_bonelengthmultiplier;
	args[i++]= in_insermotion;
	args[i++]= in_nullshierarchy;
	args[i++]= in_gaplessskeleton;
	args[i++]= in_si3dchains;
	args[i++]= in_groupanimobjs;

	CStatus st = gApp.ExecuteCommand( L"AcclaimImport", args, retval );

	return;
}

// ActivateEdgeSelTool
void ActivateEdgeSelTool( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivateEdgeSelTool", args, retval );

	return;
}

// ActivateEdgeSelToolWithNoObjStateChange
void ActivateEdgeSelToolWithNoObjStateChange( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivateEdgeSelToolWithNoObjStateChange", args, retval );

	return;
}

// ActivateElements
void ActivateElements( const CValue& /*undefined*/ in_selectionlist )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;

	CStatus st = gApp.ExecuteCommand( L"ActivateElements", args, retval );

	return;
}

// ActivateExternalConstraints
CValue ActivateExternalConstraints( const CValue& /*undefined*/ in_inputobj, const CString& in_operation )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_operation;

	CStatus st = gApp.ExecuteCommand( L"ActivateExternalConstraints", args, retval );

	return retval;
}

// ActivateFCurve
void ActivateFCurve( const CValue& /*undefined*/ in_inputobjs, bool in_activate, const CValue& /*undefined*/ in_startframe, const CValue& /*undefined*/ in_endframe )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_activate;
	args[i++]= in_startframe;
	args[i++]= in_endframe;

	CStatus st = gApp.ExecuteCommand( L"ActivateFCurve", args, retval );

	return;
}

// ActivateMarkingSet
void ActivateMarkingSet( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ActivateMarkingSet", args, retval );

	return;
}

// ActivateObjectSelTool
void ActivateObjectSelTool( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivateObjectSelTool", args, retval );

	return;
}

// ActivateObjectSelToolWithNoObjStateChange
void ActivateObjectSelToolWithNoObjStateChange( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ActivateObjectSelToolWithNoObjStateChange", args, retval );

	return;
}

// ActivatePolygonSelTool
void ActivatePolygonSelTool( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivatePolygonSelTool", args, retval );

	return;
}

// ActivatePolygonSelToolWithNoObjStateChange
void ActivatePolygonSelToolWithNoObjStateChange( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivatePolygonSelToolWithNoObjStateChange", args, retval );

	return;
}

// ActivateRaycastEdgeSelTool
void ActivateRaycastEdgeSelTool( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivateRaycastEdgeSelTool", args, retval );

	return;
}

// ActivateRaycastEdgeSelToolWithNoObjStateChange
void ActivateRaycastEdgeSelToolWithNoObjStateChange( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivateRaycastEdgeSelToolWithNoObjStateChange", args, retval );

	return;
}

// ActivateRaycastPolySelTool
void ActivateRaycastPolySelTool( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivateRaycastPolySelTool", args, retval );

	return;
}

// ActivateRaycastPolySelToolWithNoObjStateChange
void ActivateRaycastPolySelToolWithNoObjStateChange( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivateRaycastPolySelToolWithNoObjStateChange", args, retval );

	return;
}

// ActivateSampledPointSelTool
void ActivateSampledPointSelTool( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivateSampledPointSelTool", args, retval );

	return;
}

// ActivateSampledPointSelToolWithNoObjStateChange
void ActivateSampledPointSelToolWithNoObjStateChange( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivateSampledPointSelToolWithNoObjStateChange", args, retval );

	return;
}

// ActivateTipSelTool
void ActivateTipSelTool( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivateTipSelTool", args, retval );

	return;
}

// ActivateVertexSelTool
void ActivateVertexSelTool( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivateVertexSelTool", args, retval );

	return;
}

// ActivateVertexSelToolWithNoObjStateChange
void ActivateVertexSelToolWithNoObjStateChange( const CString& in_selregionmode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selregionmode;

	CStatus st = gApp.ExecuteCommand( L"ActivateVertexSelToolWithNoObjStateChange", args, retval );

	return;
}

// Add2DShape
CValue Add2DShape( const CValue& /*undefined*/ in_shapelist, const CString& in_shapetype, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_shapelist;
	args[i++]= in_shapetype;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"Add2DShape", args, retval );

	return retval;
}

// Add2DShapePoint
CValue Add2DShapePoint( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"Add2DShapePoint", args, retval );

	return retval;
}

// Add2DShapePointArray
void Add2DShapePointArray( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_points )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_points;

	CStatus st = gApp.ExecuteCommand( L"Add2DShapePointArray", args, retval );

	return;
}

// Add3DObjectToScene
void Add3DObjectToScene( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_parent )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_parent;

	CStatus st = gApp.ExecuteCommand( L"Add3DObjectToScene", args, retval );

	return;
}

// AddAlphaFloodFillPaintShape
CValue AddAlphaFloodFillPaintShape( const CValue& /*undefined*/ in_shapelist, const CValue& /*number*/ in_seedx, const CValue& /*number*/ in_seedy, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_shapelist;
	args[i++]= in_seedx;
	args[i++]= in_seedy;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddAlphaFloodFillPaintShape", args, retval );

	return retval;
}

// AddAudioClip
CValue AddAudioClip( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_source, const CValue& /*undefined*/ in_compound, const CValue& /*undefined*/ in_track, const CValue& /*undefined*/ in_time, const CString& in_name )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_source;
	args[i++]= in_compound;
	args[i++]= in_track;
	args[i++]= in_time;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"AddAudioClip", args, retval );

	return retval;
}

// AddBoneToEnd
void AddBoneToEnd( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"AddBoneToEnd", args, retval );

	return;
}

// AddBoundaryFillPaintShape
CValue AddBoundaryFillPaintShape( const CValue& /*undefined*/ in_shapelist, const CValue& /*number*/ in_seedx, const CValue& /*number*/ in_seedy, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_shapelist;
	args[i++]= in_seedx;
	args[i++]= in_seedy;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddBoundaryFillPaintShape", args, retval );

	return retval;
}

// AddBridgeTransition
CValue AddBridgeTransition( const CValue& /*undefined*/ in_from, const CValue& /*undefined*/ in_to, const CString& in_name )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_from;
	args[i++]= in_to;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"AddBridgeTransition", args, retval );

	return retval;
}

// AddBrushStroke
CValue AddBrushStroke( const CValue& /*undefined*/ in_shapelist, const CValue& /*undefined*/ in_points, const CValue& /*undefined*/ in_pressure, const CValue& /*number*/ in_index, const CRef& /*object*/ in_value )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_shapelist;
	args[i++]= in_points;
	args[i++]= in_pressure;
	args[i++]= in_index;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddBrushStroke", args, retval );

	return retval;
}

// AddBVHAction
CValue AddBVHAction( const CValue& /*undefined*/ in_arg0, const CValue& /*undefined*/ in_arg1, const CValue& /*undefined*/ in_arg2, const CValue& /*undefined*/ in_arg3, const CValue& /*undefined*/ in_arg4, const CValue& /*undefined*/ in_arg5, const CValue& /*undefined*/ in_arg6, const CValue& /*undefined*/ in_arg7 )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_arg0;
	args[i++]= in_arg1;
	args[i++]= in_arg2;
	args[i++]= in_arg3;
	args[i++]= in_arg4;
	args[i++]= in_arg5;
	args[i++]= in_arg6;
	args[i++]= in_arg7;

	CStatus st = gApp.ExecuteCommand( L"AddBVHAction", args, retval );

	return retval;
}

// AddClearImageShape
CValue AddClearImageShape( const CValue& /*undefined*/ in_shapelist, const CRef& /*object*/ in_value )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shapelist;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddClearImageShape", args, retval );

	return retval;
}

// AddClip
CValue AddClip( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_source, const CValue& /*undefined*/ in_compound, const CValue& /*undefined*/ in_track, const CValue& /*undefined*/ in_time, const CString& in_name, const CValue& /*undefined*/ in_connectionmappingtemplate, const CValue& /*undefined*/ in_valuemappingtemplate, bool in_shareparent )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_source;
	args[i++]= in_compound;
	args[i++]= in_track;
	args[i++]= in_time;
	args[i++]= in_name;
	args[i++]= in_connectionmappingtemplate;
	args[i++]= in_valuemappingtemplate;
	args[i++]= in_shareparent;

	CStatus st = gApp.ExecuteCommand( L"AddClip", args, retval );

	return retval;
}

// AddClipRelation
CValue AddClipRelation( const CValue& /*undefined*/ in_from, const CValue& /*undefined*/ in_to, const CString& in_name )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_from;
	args[i++]= in_to;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"AddClipRelation", args, retval );

	return retval;
}

// AddClipTextureLayer
CValue AddClipTextureLayer( const CValue& /*undefined*/ in_clip, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, bool in_after, const CValue& /*undefined*/ in_reference )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_after;
	args[i++]= in_reference;

	CStatus st = gApp.ExecuteCommand( L"AddClipTextureLayer", args, retval );

	return retval;
}

// AddClothClusters
void AddClothClusters( const CValue& /*undefined*/ in_inputobj, bool in_interactive )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_interactive;

	CStatus st = gApp.ExecuteCommand( L"AddClothClusters", args, retval );

	return;
}

// AddColorPaintShape
CValue AddColorPaintShape( const CValue& /*undefined*/ in_shapelist, const CRef& /*object*/ in_value )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shapelist;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddColorPaintShape", args, retval );

	return retval;
}

// AddColSphere
CValue AddColSphere( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"AddColSphere", args, retval );

	return retval;
}

// AddComponentTool
void AddComponentTool( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"AddComponentTool", args, retval );

	return;
}

// AddContainerToEnvironment
CValue AddContainerToEnvironment( const CValue& /*undefined*/ in_environment, const CString& in_type, bool in_support3dobject, const CString& in_name, const CRef& /*object*/ in_container )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_environment;
	args[i++]= in_type;
	args[i++]= in_support3dobject;
	args[i++]= in_name;
	args[i++]= in_container;

	CStatus st = gApp.ExecuteCommand( L"AddContainerToEnvironment", args, retval );

	return retval;
}

// AddEdge
CValue AddEdge( const CValue& /*undefined*/ in_connectionset, const CValue& /*number*/ in_ratio, siOperationMode in_immediatemode, const CValue& /*number*/ in_newpointid, const CValue& /*number*/ in_newedgeid )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= in_ratio;
	args[i++]= (LONG)in_immediatemode;
	args[i++]= in_newpointid;
	args[i++]= in_newedgeid;

	CStatus st = gApp.ExecuteCommand( L"AddEdge", args, retval );

	return retval;
}

// AddEllipsePaintShape
CValue AddEllipsePaintShape( const CValue& /*undefined*/ in_shapelist, const CValue& /*number*/ in_x1, const CValue& /*number*/ in_y1, const CValue& /*number*/ in_x2, const CValue& /*number*/ in_y2, bool in_fromcenter, const CRef& /*object*/ in_value )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_shapelist;
	args[i++]= in_x1;
	args[i++]= in_y1;
	args[i++]= in_x2;
	args[i++]= in_y2;
	args[i++]= in_fromcenter;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddEllipsePaintShape", args, retval );

	return retval;
}

// AddExpr
void AddExpr( const CValue& /*undefined*/ in_inputobj, const CString& in_exprstr, bool in_disableautoinspect )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_exprstr;
	args[i++]= in_disableautoinspect;

	CStatus st = gApp.ExecuteCommand( L"AddExpr", args, retval );

	return;
}

// AddFCurve
void AddFCurve( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"AddFCurve", args, retval );

	return;
}

// AddFloodFillPaintShape
CValue AddFloodFillPaintShape( const CValue& /*undefined*/ in_shapelist, const CValue& /*number*/ in_seedx, const CValue& /*number*/ in_seedy, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_shapelist;
	args[i++]= in_seedx;
	args[i++]= in_seedy;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddFloodFillPaintShape", args, retval );

	return retval;
}

// AddFloor
CValue AddFloor( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"AddFloor", args, retval );

	return retval;
}

// AddFxMaskShape
CValue AddFxMaskShape( const CValue& /*undefined*/ in_shapelist, const CRef& /*object*/ in_value )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shapelist;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddFxMaskShape", args, retval );

	return retval;
}

// AddFxOp
CValue AddFxOp( const CValue& /*undefined*/ in_fxtree, const CString& in_presetname, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_fxtree;
	args[i++]= in_presetname;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddFxOp", args, retval );

	return retval;
}

// AddGroup
void AddGroup( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"AddGroup", args, retval );

	return;
}

// AddHelperBone
void AddHelperBone( const CValue& /*undefined*/ in_rig, const CValue& /*undefined*/ in_root, const CValue& /*undefined*/ in_parent, const CValue& /*undefined*/ in_child )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_rig;
	args[i++]= in_root;
	args[i++]= in_parent;
	args[i++]= in_child;

	CStatus st = gApp.ExecuteCommand( L"AddHelperBone", args, retval );

	return;
}

// AddHelperTrigger
void AddHelperTrigger( const CValue& /*undefined*/ in_helper, const CValue& /*undefined*/ in_bone1, const CValue& /*undefined*/ in_bone2 )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_helper;
	args[i++]= in_bone1;
	args[i++]= in_bone2;

	CStatus st = gApp.ExecuteCommand( L"AddHelperTrigger", args, retval );

	return;
}

// AddImageClip
CValue AddImageClip( const CValue& /*undefined*/ in_source, const CString& in_name, const CString& in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_name;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddImageClip", args, retval );

	return retval;
}

// AddImageClipFxOp
CValue AddImageClipFxOp( const CValue& /*undefined*/ in_fxtree, const CValue& /*undefined*/ in_imageclip, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_fxtree;
	args[i++]= in_imageclip;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddImageClipFxOp", args, retval );

	return retval;
}

// AddImageSource
CValue AddImageSource( const CString& in_filename, const CString& in_name, const CString& in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddImageSource", args, retval );

	return retval;
}

// AddImageTextureLayer
CValue AddImageTextureLayer( const CString& in_filename, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, bool in_after, const CValue& /*undefined*/ in_reference )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_after;
	args[i++]= in_reference;

	CStatus st = gApp.ExecuteCommand( L"AddImageTextureLayer", args, retval );

	return retval;
}

// AddInputToOperator
void AddInputToOperator( const CValue& /*undefined*/ in_reference, const CValue& /*undefined*/ in_new )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_reference;
	args[i++]= in_new;

	CStatus st = gApp.ExecuteCommand( L"AddInputToOperator", args, retval );

	return;
}

// AddJointLimit
CValue AddJointLimit( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"AddJointLimit", args, retval );

	return retval;
}

// AddMappingNode
void AddMappingNode( const CValue& /*undefined*/ in_operator, const CValue& /*undefined*/ in_propobj, const CRef& /*object*/ in_mappingnodeprop, const CRef& /*object*/ in_mapmixerop )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_propobj;
	args[i++]= in_mappingnodeprop;
	args[i++]= in_mapmixerop;

	CStatus st = gApp.ExecuteCommand( L"AddMappingNode", args, retval );

	return;
}

// AddMappingRule
void AddMappingRule( const CValue& /*undefined*/ in_mappingtemplate, const CString& in_from, const CString& in_to, const CValue& /*number*/ in_index )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_mappingtemplate;
	args[i++]= in_from;
	args[i++]= in_to;
	args[i++]= in_index;

	CStatus st = gApp.ExecuteCommand( L"AddMappingRule", args, retval );

	return;
}

// AddMarker
CValue AddMarker( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_container, const CValue& /*undefined*/ in_time, const CValue& /*number*/ in_duration, const CString& in_name )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_container;
	args[i++]= in_time;
	args[i++]= in_duration;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"AddMarker", args, retval );

	return retval;
}

// AddModelClip
CValue AddModelClip( const CString& in_source, const CString& in_name, const CString& in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_name;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddModelClip", args, retval );

	return retval;
}

// AddModelSource
CValue AddModelSource( const CString& in_filename, const CString& in_name, const CValue& /*number*/ in_resolution, const CString& in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_resolution;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddModelSource", args, retval );

	return retval;
}

// AddObjectsToShader
void AddObjectsToShader( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_target )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"AddObjectsToShader", args, retval );

	return;
}

// AddPaintMaskShape
CValue AddPaintMaskShape( const CValue& /*number*/ in_selectionaction, bool in_antialiased, const CValue& /*number*/ in_feather, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionaction;
	args[i++]= in_antialiased;
	args[i++]= in_feather;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddPaintMaskShape", args, retval );

	return retval;
}

// AddPaintShapePoint
CValue AddPaintShapePoint( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_insertposition, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_lhx, const CValue& /*number*/ in_lhy, const CValue& /*number*/ in_rhx, const CValue& /*number*/ in_rhy, const CValue& /*number*/ in_value )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_insertposition;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_lhx;
	args[i++]= in_lhy;
	args[i++]= in_rhx;
	args[i++]= in_rhy;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddPaintShapePoint", args, retval );

	return retval;
}

// AddParticleEmitter
void AddParticleEmitter( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobj, bool in_interactive )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobj;
	args[i++]= in_interactive;

	CStatus st = gApp.ExecuteCommand( L"AddParticleEmitter", args, retval );

	return;
}

// AddParticleEvent
void AddParticleEvent( const CString& in_target, const CString& in_source, const CRef& /*object*/ in_output )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_source;
	args[i++]= in_output;

	CStatus st = gApp.ExecuteCommand( L"AddParticleEvent", args, retval );

	return;
}

// AddParticleForce
void AddParticleForce( const CString& in_target, const CString& in_inputobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"AddParticleForce", args, retval );

	return;
}

// AddParticleGoal
void AddParticleGoal( const CString& in_target, const CString& in_inputobj, const CRef& /*object*/ in_output )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobj;
	args[i++]= in_output;

	CStatus st = gApp.ExecuteCommand( L"AddParticleGoal", args, retval );

	return;
}

// AddParticleObstacle
void AddParticleObstacle( const CString& in_target, const CString& in_inputobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"AddParticleObstacle", args, retval );

	return;
}

// AddParticles
CValue AddParticles( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_partype, const CValue& /*undefined*/ in_positions, const CValue& /*undefined*/ in_velocities, bool in_world_referential )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_partype;
	args[i++]= in_positions;
	args[i++]= in_velocities;
	args[i++]= in_world_referential;

	CStatus st = gApp.ExecuteCommand( L"AddParticles", args, retval );

	return retval;
}

// AddParticleTool
void AddParticleTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"AddParticleTool", args, retval );

	return;
}

// AddPassInputFxOp
CValue AddPassInputFxOp( const CValue& /*undefined*/ in_fxtree, const CValue& /*undefined*/ in_renderpass, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_fxtree;
	args[i++]= in_renderpass;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddPassInputFxOp", args, retval );

	return retval;
}

// AddPointInteriorToPolygon
void AddPointInteriorToPolygon( const CValue& /*undefined*/ in_operator, const CValue& /*number*/ in_polygonid, const CValue& /*number*/ in_firstoffset, const CValue& /*number*/ in_secondoffset, const CValue& /*number*/ in_thirdoffset, const CValue& /*number*/ in_firstweight, const CValue& /*number*/ in_secondweight, const CValue& /*number*/ in_thirdweight, const CValue& /*number*/ in_height )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_polygonid;
	args[i++]= in_firstoffset;
	args[i++]= in_secondoffset;
	args[i++]= in_thirdoffset;
	args[i++]= in_firstweight;
	args[i++]= in_secondweight;
	args[i++]= in_thirdweight;
	args[i++]= in_height;

	CStatus st = gApp.ExecuteCommand( L"AddPointInteriorToPolygon", args, retval );

	return;
}

// AddPointOnCurveToolByCVs
void AddPointOnCurveToolByCVs( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"AddPointOnCurveToolByCVs", args, retval );

	return;
}

// AddPointOnCurveToolByKnotPoints
void AddPointOnCurveToolByKnotPoints( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"AddPointOnCurveToolByKnotPoints", args, retval );

	return;
}

// AddPointToNewPolygon
CValue AddPointToNewPolygon( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z, const CValue& /*number*/ in_insertposition, const CValue& /*number*/ in_polygonid )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= in_insertposition;
	args[i++]= in_polygonid;

	CStatus st = gApp.ExecuteCommand( L"AddPointToNewPolygon", args, retval );

	return retval;
}

// AddPoseActionItems
void AddPoseActionItems( const CValue& /*undefined*/ in_inputobjs, const CString& in_name )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"AddPoseActionItems", args, retval );

	return;
}

// AddPresetTextureLayer
CValue AddPresetTextureLayer( const CString& in_presetobj, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, bool in_after, const CValue& /*undefined*/ in_reference )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_after;
	args[i++]= in_reference;

	CStatus st = gApp.ExecuteCommand( L"AddPresetTextureLayer", args, retval );

	return retval;
}

// AddProp
void AddProp( const CString& in_presetobj, const CString& in_inputobjs, const CString& in_propagation, const CString& in_propertyname, const CRef& /*object*/ in_value )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobjs;
	args[i++]= in_propagation;
	args[i++]= in_propertyname;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddProp", args, retval );

	return;
}

// AddProxyParam
void AddProxyParam( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_proxied, const CString& in_prefix )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_proxied;
	args[i++]= in_prefix;

	CStatus st = gApp.ExecuteCommand( L"AddProxyParam", args, retval );

	return;
}

// AddRectanglePaintShape
CValue AddRectanglePaintShape( const CValue& /*undefined*/ in_shapelist, const CValue& /*number*/ in_x1, const CValue& /*number*/ in_y1, const CValue& /*number*/ in_x2, const CValue& /*number*/ in_y2, bool in_fromcenter, const CRef& /*object*/ in_value )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_shapelist;
	args[i++]= in_x1;
	args[i++]= in_y1;
	args[i++]= in_x2;
	args[i++]= in_y2;
	args[i++]= in_fromcenter;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"AddRectanglePaintShape", args, retval );

	return retval;
}

// AddScriptedOp
CValue AddScriptedOp( const CValue& /*undefined*/ in_outputobjs, const CString& in_code, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, const CString& in_language, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_outputobjs;
	args[i++]= in_code;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_language;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"AddScriptedOp", args, retval );

	return retval;
}

// AddScriptedOpFromFile
CValue AddScriptedOpFromFile( const CValue& /*undefined*/ in_outputobjs, const CString& in_filename, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, const CString& in_language, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_outputobjs;
	args[i++]= in_filename;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_language;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"AddScriptedOpFromFile", args, retval );

	return retval;
}

// AddTextureLayer
CValue AddTextureLayer( const CValue& /*undefined*/ in_object, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, bool in_after, const CValue& /*undefined*/ in_reference )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_object;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_after;
	args[i++]= in_reference;

	CStatus st = gApp.ExecuteCommand( L"AddTextureLayer", args, retval );

	return retval;
}

// AddTextureLayerPorts
CValue AddTextureLayerPorts( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"AddTextureLayerPorts", args, retval );

	return retval;
}

// AddToCluster
void AddToCluster( const CString& in_inputobjs, siActionWhenLocalMaterialsOverlap in_actionwhenlocalmaterialsoverlap )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_actionwhenlocalmaterialsoverlap;

	CStatus st = gApp.ExecuteCommand( L"AddToCluster", args, retval );

	return;
}

// AddToContainer
void AddToContainer( const CString& in_target, const CString& in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"AddToContainer", args, retval );

	return;
}

// AddToGroup
void AddToGroup( const CString& in_target, const CString& in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"AddToGroup", args, retval );

	return;
}

// AddToMarking
void AddToMarking( const CValue& /*undefined*/ in_pathlist )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_pathlist;

	CStatus st = gApp.ExecuteCommand( L"AddToMarking", args, retval );

	return;
}

// AddToPaintShapePointSelection
void AddToPaintShapePointSelection( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_indices )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_indices;

	CStatus st = gApp.ExecuteCommand( L"AddToPaintShapePointSelection", args, retval );

	return;
}

// AddToPartition
void AddToPartition( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"AddToPartition", args, retval );

	return;
}

// AddToSelection
void AddToSelection( const CValue& /*undefined*/ in_selectionlist, const CString& in_hierarchylevel, bool in_checkobjectselectability )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;
	args[i++]= in_hierarchylevel;
	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"AddToSelection", args, retval );

	return;
}

// AddToTOC
void AddToTOC( const CValue& /*undefined*/ in_source )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;

	CStatus st = gApp.ExecuteCommand( L"AddToTOC", args, retval );

	return;
}

// AddTrack
CValue AddTrack( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_compound, const CValue& /*number*/ in_type, const CString& in_name, const CValue& /*number*/ in_insertposition )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_compound;
	args[i++]= in_type;
	args[i++]= in_name;
	args[i++]= in_insertposition;

	CStatus st = gApp.ExecuteCommand( L"AddTrack", args, retval );

	return retval;
}

// AddTransition
CValue AddTransition( const CValue& /*undefined*/ in_from, const CValue& /*undefined*/ in_to, const CValue& /*number*/ in_type, const CString& in_name )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_from;
	args[i++]= in_to;
	args[i++]= in_type;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"AddTransition", args, retval );

	return retval;
}

// AddTWTransition
CValue AddTWTransition( const CValue& /*undefined*/ in_from, const CValue& /*undefined*/ in_to, const CValue& /*number*/ in_from_period, const CValue& /*number*/ in_to_period, const CString& in_name )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_from;
	args[i++]= in_to;
	args[i++]= in_from_period;
	args[i++]= in_to_period;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"AddTWTransition", args, retval );

	return retval;
}

// AddUserParameter
void AddUserParameter( const CValue& /*undefined*/ in_target, const CString& in_name, const CValue& /*number*/ in_type )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_name;
	args[i++]= in_type;

	CStatus st = gApp.ExecuteCommand( L"AddUserParameter", args, retval );

	return;
}

// AddVertexTool
void AddVertexTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"AddVertexTool", args, retval );

	return;
}

// Align
void Align( const CValue& /*undefined*/ in_inputobjs, siAxesFilter in_alignaxes, siAlignModes in_posx, siAlignModes in_posy, siAlignModes in_posz, siAlignModes in_refx, siAlignModes in_refy, siAlignModes in_refz, bool in_allobjects )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_alignaxes;
	args[i++]= (LONG)in_posx;
	args[i++]= (LONG)in_posy;
	args[i++]= (LONG)in_posz;
	args[i++]= (LONG)in_refx;
	args[i++]= (LONG)in_refy;
	args[i++]= (LONG)in_refz;
	args[i++]= in_allobjects;

	CStatus st = gApp.ExecuteCommand( L"Align", args, retval );

	return;
}

// AlignRootToFirstBone
void AlignRootToFirstBone( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"AlignRootToFirstBone", args, retval );

	return;
}

// AppendBone
CValue AppendBone( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_ex, const CValue& /*number*/ in_ey, const CValue& /*number*/ in_ez, bool in_pin )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_ex;
	args[i++]= in_ey;
	args[i++]= in_ez;
	args[i++]= in_pin;

	CStatus st = gApp.ExecuteCommand( L"AppendBone", args, retval );

	return retval;
}

// ApplyAction
void ApplyAction( const CValue& /*undefined*/ in_source, const CValue& /*undefined*/ in_target, bool in_consider_time, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, bool in_ripple )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_target;
	args[i++]= in_consider_time;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_ripple;

	CStatus st = gApp.ExecuteCommand( L"ApplyAction", args, retval );

	return;
}

// ApplyCloth
CValue ApplyCloth( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyCloth", args, retval );

	return retval;
}

// ApplyCns
CValue ApplyCns( const CString& in_presetobj, const CValue& /*undefined*/ in_constrainedobj, const CValue& /*undefined*/ in_constrainingobj, bool in_compensation )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_constrainedobj;
	args[i++]= in_constrainingobj;
	args[i++]= in_compensation;

	CStatus st = gApp.ExecuteCommand( L"ApplyCns", args, retval );

	return retval;
}

// ApplyDeformByCage
CValue ApplyDeformByCage( const CValue& /*undefined*/ in_connectionset, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"ApplyDeformByCage", args, retval );

	return retval;
}

// ApplyDeformBySpine
void ApplyDeformBySpine( const CString& in_connectionset, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"ApplyDeformBySpine", args, retval );

	return;
}

// ApplyDeformKey
CValue ApplyDeformKey( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"ApplyDeformKey", args, retval );

	return retval;
}

// ApplyFlexEnv
CValue ApplyFlexEnv( const CValue& /*undefined*/ in_connectionset, bool in_assignnewdeformers, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= in_assignnewdeformers;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"ApplyFlexEnv", args, retval );

	return retval;
}

// ApplyGenOp
void ApplyGenOp( const CString& in_presetobj, const CString& in_target, const CString& in_connectionset, const CValue& /*number*/ in_connecttype, siOperationMode in_immediatemode, siGeneratorInputsDisposalMode in_genopinputsdisposal, const CRef& /*object*/ in_outputobjs )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_target;
	args[i++]= in_connectionset;
	args[i++]= in_connecttype;
	args[i++]= (LONG)in_immediatemode;
	args[i++]= (LONG)in_genopinputsdisposal;
	args[i++]= in_outputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyGenOp", args, retval );

	return;
}

// ApplyHairAttenuateOp
CValue ApplyHairAttenuateOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairAttenuateOp", args, retval );

	return retval;
}

// ApplyHairClumpOp
CValue ApplyHairClumpOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairClumpOp", args, retval );

	return retval;
}

// ApplyHairCombNormalOp
CValue ApplyHairCombNormalOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairCombNormalOp", args, retval );

	return retval;
}

// ApplyHairCombOp
CValue ApplyHairCombOp( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairCombOp", args, retval );

	return retval;
}

// ApplyHairCutOp
CValue ApplyHairCutOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairCutOp", args, retval );

	return retval;
}

// ApplyHairDynamicsOp
CValue ApplyHairDynamicsOp( const CValue& /*undefined*/ in_in_objs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_in_objs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairDynamicsOp", args, retval );

	return retval;
}

// ApplyHairLockOp
CValue ApplyHairLockOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairLockOp", args, retval );

	return retval;
}

// ApplyHairMergeOp
CValue ApplyHairMergeOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairMergeOp", args, retval );

	return retval;
}

// ApplyHairOp
CValue ApplyHairOp( const CValue& /*undefined*/ in_in_objs, const CString& in_presetobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_in_objs;
	args[i++]= in_presetobj;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairOp", args, retval );

	return retval;
}

// ApplyHairPopOp
CValue ApplyHairPopOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairPopOp", args, retval );

	return retval;
}

// ApplyHairPopSelectedOp
CValue ApplyHairPopSelectedOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairPopSelectedOp", args, retval );

	return retval;
}

// ApplyHairPuffOp
CValue ApplyHairPuffOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairPuffOp", args, retval );

	return retval;
}

// ApplyHairRecombOp
CValue ApplyHairRecombOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairRecombOp", args, retval );

	return retval;
}

// ApplyHairResampleOp
CValue ApplyHairResampleOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairResampleOp", args, retval );

	return retval;
}

// ApplyHairRotateOp
CValue ApplyHairRotateOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairRotateOp", args, retval );

	return retval;
}

// ApplyHairScaleOp
CValue ApplyHairScaleOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairScaleOp", args, retval );

	return retval;
}

// ApplyHairShatterOp
CValue ApplyHairShatterOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairShatterOp", args, retval );

	return retval;
}

// ApplyHairSplineGenOp
CValue ApplyHairSplineGenOp( const CValue& /*undefined*/ in_in_objs, const CString& in_presetobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_in_objs;
	args[i++]= in_presetobj;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairSplineGenOp", args, retval );

	return retval;
}

// ApplyHairSplitOp
CValue ApplyHairSplitOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairSplitOp", args, retval );

	return retval;
}

// ApplyHairUnlockOp
CValue ApplyHairUnlockOp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyHairUnlockOp", args, retval );

	return retval;
}

// ApplyHelperBoneChanges
void ApplyHelperBoneChanges( const CValue& /*undefined*/ in_prop )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_prop;

	CStatus st = gApp.ExecuteCommand( L"ApplyHelperBoneChanges", args, retval );

	return;
}

// ApplyKinematicOp
CValue ApplyKinematicOp( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_connectionset, siBranchFlag in_connecttype )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_connectionset;
	args[i++]= (LONG)in_connecttype;

	CStatus st = gApp.ExecuteCommand( L"ApplyKinematicOp", args, retval );

	return retval;
}

// ApplyOp
void ApplyOp( const CString& in_presetobj, const CString& in_connectionset, const CValue& /*number*/ in_connecttype, siOperationMode in_immediatemode, const CRef& /*object*/ in_outputobjs, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_connectionset;
	args[i++]= in_connecttype;
	args[i++]= (LONG)in_immediatemode;
	args[i++]= in_outputobjs;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"ApplyOp", args, retval );

	return;
}

// ApplyOperator
CValue ApplyOperator( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_connectionset, siBranchFlag in_connecttype )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_connectionset;
	args[i++]= (LONG)in_connecttype;

	CStatus st = gApp.ExecuteCommand( L"ApplyOperator", args, retval );

	return retval;
}

// ApplyParticleShader
void ApplyParticleShader( const CString& in_inputobjs, const CString& in_material, const CString& in_volume )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_material;
	args[i++]= in_volume;

	CStatus st = gApp.ExecuteCommand( L"ApplyParticleShader", args, retval );

	return;
}

// ApplyPath
void ApplyPath( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_path, const CValue& /*undefined*/ in_startframe, const CValue& /*undefined*/ in_endframe, const CValue& /*number*/ in_nbkeys, bool in_linear, bool in_tangent )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_path;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_nbkeys;
	args[i++]= in_linear;
	args[i++]= in_tangent;

	CStatus st = gApp.ExecuteCommand( L"ApplyPath", args, retval );

	return;
}

// Applypolymatricks
CValue Applypolymatricks( const CString& in_target, const CValueArray&  in_stack )
{
	CValueArray args(2);
	CValue retval;

	args[0]= in_target;
	args[1]= in_stack;

	CStatus st = gApp.ExecuteCommand( L"Applypolymatricks", args, retval );

	return retval;
}

// ApplyPropVolumeOp
void ApplyPropVolumeOp( const CString& in_presetobj, const CString& in_connectionset, const CValue& /*number*/ in_connecttype, bool in_immediatemode, siOperationMode in_outputobjs, const CRef& /*object*/ in_constructionmode )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_connectionset;
	args[i++]= in_connecttype;
	args[i++]= in_immediatemode;
	args[i++]= (LONG)in_outputobjs;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"ApplyPropVolumeOp", args, retval );

	return;
}

// ApplyShader
void ApplyShader( const CString& in_presetobj, const CString& in_inputobjs, const CString& in_name, const CString& in_propagationtype, siActionWhenLocalMaterialsOverlap in_actionwhenlocalmaterialsoverlap )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_propagationtype;
	args[i++]= (LONG)in_actionwhenlocalmaterialsoverlap;

	CStatus st = gApp.ExecuteCommand( L"ApplyShader", args, retval );

	return;
}

// ApplyShapeKey
CValue ApplyShapeKey( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_compound, const CValue& /*undefined*/ in_track, const CValue& /*undefined*/ in_time, const CValue& /*undefined*/ in_duration, const CValue& /*undefined*/ in_mode, const CValue& /*undefined*/ in_name, const CValue& /*undefined*/ in_overwrite )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_compound;
	args[i++]= in_track;
	args[i++]= in_time;
	args[i++]= in_duration;
	args[i++]= in_mode;
	args[i++]= in_name;
	args[i++]= in_overwrite;

	CStatus st = gApp.ExecuteCommand( L"ApplyShapeKey", args, retval );

	return retval;
}

// ApplySnapFaceOp
void ApplySnapFaceOp( const CValue& /*undefined*/ in_connectionset, siBranchFlag in_connecttype, siOperationMode in_immediatemode, const CRef& /*object*/ in_outputobjs )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= (LONG)in_connecttype;
	args[i++]= (LONG)in_immediatemode;
	args[i++]= in_outputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplySnapFaceOp", args, retval );

	return;
}

// ApplySoftBody
CValue ApplySoftBody( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplySoftBody", args, retval );

	return retval;
}

// ApplySpringOp
void ApplySpringOp( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_mass, const CValue& /*number*/ in_linearstiffness, const CValue& /*number*/ in_lineardamping, const CString& in_springcache, const CRef& /*object*/ in_outputobjs )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_mass;
	args[i++]= in_linearstiffness;
	args[i++]= in_lineardamping;
	args[i++]= in_springcache;
	args[i++]= in_outputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplySpringOp", args, retval );

	return;
}

// ApplyTopoOp
void ApplyTopoOp( const CString& in_presetobj, const CString& in_connectionset, const CValue& /*number*/ in_connecttype, siOperationMode in_immediatemode, const CRef& /*object*/ in_outputobjs )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_connectionset;
	args[i++]= in_connecttype;
	args[i++]= (LONG)in_immediatemode;
	args[i++]= in_outputobjs;

	CStatus st = gApp.ExecuteCommand( L"ApplyTopoOp", args, retval );

	return;
}

// AssembleNurbsMesh
void AssembleNurbsMesh( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_tolerance, bool in_copyusercluster, bool in_copymaterial, bool in_centergeomcenter )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_tolerance;
	args[i++]= in_copyusercluster;
	args[i++]= in_copymaterial;
	args[i++]= in_centergeomcenter;

	CStatus st = gApp.ExecuteCommand( L"AssembleNurbsMesh", args, retval );

	return;
}

// AssignMaterial
void AssignMaterial( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"AssignMaterial", args, retval );

	return;
}

// AssociateEnvelopeBoundingVolume
void AssociateEnvelopeBoundingVolume( const CValue& /*undefined*/ in_inputobjs, const CString& in_bndvoltype )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_bndvoltype;

	CStatus st = gApp.ExecuteCommand( L"AssociateEnvelopeBoundingVolume", args, retval );

	return;
}

// AttachRigidBodies
void AttachRigidBodies( const CString& in_name, const CValue& /*undefined*/ in_bodies, const CValue& /*number*/ in_px, const CValue& /*number*/ in_py, const CValue& /*number*/ in_pz )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_name;
	args[i++]= in_bodies;
	args[i++]= in_px;
	args[i++]= in_py;
	args[i++]= in_pz;

	CStatus st = gApp.ExecuteCommand( L"AttachRigidBodies", args, retval );

	return;
}

// AutoInspect
void AutoInspect( const CValue& /*undefined*/ in_inputobjs, const CString& in_title, siInspectMode in_mode, const CString& in_keywords )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_title;
	args[i++]= (LONG)in_mode;
	args[i++]= in_keywords;

	CStatus st = gApp.ExecuteCommand( L"AutoInspect", args, retval );

	return;
}

// AutoInspectEnabled
CValue AutoInspectEnabled( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"AutoInspectEnabled", args, retval );

	return retval;
}

// BeginUndo
void BeginUndo( const CString& in_name )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"BeginUndo", args, retval );

	return;
}

// BiovisionImport
void BiovisionImport( const CString& in_filename, bool in_insermotion, bool in_nullshierarchy, bool in_gaplessskeleton, bool in_si3dchains, bool in_groupanimobjs )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_insermotion;
	args[i++]= in_nullshierarchy;
	args[i++]= in_gaplessskeleton;
	args[i++]= in_si3dchains;
	args[i++]= in_groupanimobjs;

	CStatus st = gApp.ExecuteCommand( L"BiovisionImport", args, retval );

	return;
}

// BlendClipsInTextureLayers
CValue BlendClipsInTextureLayers( const CValue& /*undefined*/ in_cliplist, const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_sharedmataction, bool in_notifynoresult, siBlendInTextureLayersModes in_mode, bool in_ambient, bool in_diffuse, bool in_transparency, bool in_reflection )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_cliplist;
	args[i++]= in_inputobjs;
	args[i++]= in_sharedmataction;
	args[i++]= in_notifynoresult;
	args[i++]= (LONG)in_mode;
	args[i++]= in_ambient;
	args[i++]= in_diffuse;
	args[i++]= in_transparency;
	args[i++]= in_reflection;

	CStatus st = gApp.ExecuteCommand( L"BlendClipsInTextureLayers", args, retval );

	return retval;
}

// BlendClipsInTextureLayersInsp
CValue BlendClipsInTextureLayersInsp( const CValue& /*undefined*/ in_cliplist, const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_sharedmataction, bool in_notifynoresult, siBlendInTextureLayersModes in_mode, bool in_ambient, bool in_diffuse, bool in_transparency, bool in_reflection )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_cliplist;
	args[i++]= in_inputobjs;
	args[i++]= in_sharedmataction;
	args[i++]= in_notifynoresult;
	args[i++]= (LONG)in_mode;
	args[i++]= in_ambient;
	args[i++]= in_diffuse;
	args[i++]= in_transparency;
	args[i++]= in_reflection;

	CStatus st = gApp.ExecuteCommand( L"BlendClipsInTextureLayersInsp", args, retval );

	return retval;
}

// BlendClipsInTextureLayersWithPorts
CValue BlendClipsInTextureLayersWithPorts( const CValue& /*undefined*/ in_cliplist, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_cliplist;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"BlendClipsInTextureLayersWithPorts", args, retval );

	return retval;
}

// BlendClipsInTextureLayersWithPortsInsp
CValue BlendClipsInTextureLayersWithPortsInsp( const CValue& /*undefined*/ in_cliplist, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_cliplist;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"BlendClipsInTextureLayersWithPortsInsp", args, retval );

	return retval;
}

// BlendInPresets
CValue BlendInPresets( const CString& in_presetnames, const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_sharedmataction, bool in_notifynoresult, siBlendInPresetModes in_mode, bool in_ambient, bool in_diffuse, bool in_transparency, bool in_reflection )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetnames;
	args[i++]= in_inputobjs;
	args[i++]= in_sharedmataction;
	args[i++]= in_notifynoresult;
	args[i++]= (LONG)in_mode;
	args[i++]= in_ambient;
	args[i++]= in_diffuse;
	args[i++]= in_transparency;
	args[i++]= in_reflection;

	CStatus st = gApp.ExecuteCommand( L"BlendInPresets", args, retval );

	return retval;
}

// BlendInPresetsInsp
CValue BlendInPresetsInsp( const CString& in_presetnames, const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_sharedmataction, bool in_notifynoresult, siBlendInPresetModes in_mode, bool in_ambient, bool in_diffuse, bool in_transparency, bool in_reflection )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetnames;
	args[i++]= in_inputobjs;
	args[i++]= in_sharedmataction;
	args[i++]= in_notifynoresult;
	args[i++]= (LONG)in_mode;
	args[i++]= in_ambient;
	args[i++]= in_diffuse;
	args[i++]= in_transparency;
	args[i++]= in_reflection;

	CStatus st = gApp.ExecuteCommand( L"BlendInPresetsInsp", args, retval );

	return retval;
}

// BlendInPresetsWithPorts
CValue BlendInPresetsWithPorts( const CString& in_presetnames, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetnames;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"BlendInPresetsWithPorts", args, retval );

	return retval;
}

// BlendInPresetsWithPortsInsp
CValue BlendInPresetsWithPortsInsp( const CString& in_presetnames, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetnames;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"BlendInPresetsWithPortsInsp", args, retval );

	return retval;
}

// BlendInTextureLayers
CValue BlendInTextureLayers( const CString& in_presetnames, const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_sharedmataction, bool in_notifynoresult, siBlendInTextureLayersModes in_mode, bool in_ambient, bool in_diffuse, bool in_transparency, bool in_reflection )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetnames;
	args[i++]= in_inputobjs;
	args[i++]= in_sharedmataction;
	args[i++]= in_notifynoresult;
	args[i++]= (LONG)in_mode;
	args[i++]= in_ambient;
	args[i++]= in_diffuse;
	args[i++]= in_transparency;
	args[i++]= in_reflection;

	CStatus st = gApp.ExecuteCommand( L"BlendInTextureLayers", args, retval );

	return retval;
}

// BlendInTextureLayersInsp
CValue BlendInTextureLayersInsp( const CString& in_presetnames, const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_sharedmataction, bool in_notifynoresult, siBlendInTextureLayersModes in_mode, bool in_ambient, bool in_diffuse, bool in_transparency, bool in_reflection )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetnames;
	args[i++]= in_inputobjs;
	args[i++]= in_sharedmataction;
	args[i++]= in_notifynoresult;
	args[i++]= (LONG)in_mode;
	args[i++]= in_ambient;
	args[i++]= in_diffuse;
	args[i++]= in_transparency;
	args[i++]= in_reflection;

	CStatus st = gApp.ExecuteCommand( L"BlendInTextureLayersInsp", args, retval );

	return retval;
}

// BlendInTextureLayersWithPorts
CValue BlendInTextureLayersWithPorts( const CString& in_presetnames, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetnames;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"BlendInTextureLayersWithPorts", args, retval );

	return retval;
}

// BlendInTextureLayersWithPortsInsp
CValue BlendInTextureLayersWithPortsInsp( const CString& in_presetnames, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetnames;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"BlendInTextureLayersWithPortsInsp", args, retval );

	return retval;
}

// BreakChainAtBone
void BreakChainAtBone( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"BreakChainAtBone", args, retval );

	return;
}

// CamDollyTool
void CamDollyTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"CamDollyTool", args, retval );

	return;
}

// CamDriveTool
void CamDriveTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"CamDriveTool", args, retval );

	return;
}

// CameraRedo
void CameraRedo( const CValue& /*number*/ in_view )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_view;

	CStatus st = gApp.ExecuteCommand( L"CameraRedo", args, retval );

	return;
}

// CameraReset
void CameraReset( const CValue& /*number*/ in_view, bool in_allviews )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_view;
	args[i++]= in_allviews;

	CStatus st = gApp.ExecuteCommand( L"CameraReset", args, retval );

	return;
}

// CameraTextureProj
void CameraTextureProj( const CValue& /*undefined*/ in_inputobjs, const CString& in_name, const CString& in_camera )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_camera;

	CStatus st = gApp.ExecuteCommand( L"CameraTextureProj", args, retval );

	return;
}

// CameraUndo
void CameraUndo( const CValue& /*number*/ in_view )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_view;

	CStatus st = gApp.ExecuteCommand( L"CameraUndo", args, retval );

	return;
}

// CamFlyTool
void CamFlyTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"CamFlyTool", args, retval );

	return;
}

// CamNavigTool
void CamNavigTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"CamNavigTool", args, retval );

	return;
}

// CamOrbitTool
void CamOrbitTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"CamOrbitTool", args, retval );

	return;
}

// CamPanZoomTool
void CamPanZoomTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"CamPanZoomTool", args, retval );

	return;
}

// CamPivotTool
void CamPivotTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"CamPivotTool", args, retval );

	return;
}

// CamRectZoomTool
void CamRectZoomTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"CamRectZoomTool", args, retval );

	return;
}

// CamRollTool
void CamRollTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"CamRollTool", args, retval );

	return;
}

// CamTrackTool
void CamTrackTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"CamTrackTool", args, retval );

	return;
}

// CamWalkTool
void CamWalkTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"CamWalkTool", args, retval );

	return;
}

// CamZoomTool
void CamZoomTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"CamZoomTool", args, retval );

	return;
}

// CaptureViewport
void CaptureViewport( const CValue& /*number*/ in_target, bool in_interactive )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_interactive;

	CStatus st = gApp.ExecuteCommand( L"CaptureViewport", args, retval );

	return;
}

// ChangeSubdivision
CValue ChangeSubdivision( const CString& in_inputobj, bool in_updirection )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_updirection;

	CStatus st = gApp.ExecuteCommand( L"ChangeSubdivision", args, retval );

	return retval;
}

// ClearImage
void ClearImage( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ClearImage", args, retval );

	return;
}

// ClearMarking
void ClearMarking( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ClearMarking", args, retval );

	return;
}

// ClearPaintClipFrameSelection
void ClearPaintClipFrameSelection( const CValue& /*undefined*/ in_paintclip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_paintclip;

	CStatus st = gApp.ExecuteCommand( L"ClearPaintClipFrameSelection", args, retval );

	return;
}

// ClearPaintSelection
void ClearPaintSelection( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ClearPaintSelection", args, retval );

	return;
}

// ClearParameterColor
void ClearParameterColor( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ClearParameterColor", args, retval );

	return;
}

// ClearThumbnail
void ClearThumbnail( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ClearThumbnail", args, retval );

	return;
}

// ClearUserKeyword
void ClearUserKeyword( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ClearUserKeyword", args, retval );

	return;
}

// ClipMatch
void ClipMatch( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"ClipMatch", args, retval );

	return;
}

// Clone
CValue Clone( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_nbitems, siDupParent in_hierarchy, siDupGroup in_grouping, siDupProperty in_properties, siDupAnimation in_animation, siDupConstraint in_constraints, siDupSelection in_selection, siDupTransform in_xfrom, const CString& in_sx, const CString& in_sy, const CString& in_sz, const CString& in_rx, const CString& in_ry, const CString& in_rz, const CString& in_tx, const CString& in_ty, const CString& in_tz, const CString& in_trackxform )
{
	CValueArray args(19);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_nbitems;
	args[i++]= (LONG)in_hierarchy;
	args[i++]= (LONG)in_grouping;
	args[i++]= (LONG)in_properties;
	args[i++]= (LONG)in_animation;
	args[i++]= (LONG)in_constraints;
	args[i++]= (LONG)in_selection;
	args[i++]= (LONG)in_xfrom;
	args[i++]= in_sx;
	args[i++]= in_sy;
	args[i++]= in_sz;
	args[i++]= in_rx;
	args[i++]= in_ry;
	args[i++]= in_rz;
	args[i++]= in_tx;
	args[i++]= in_ty;
	args[i++]= in_tz;
	args[i++]= in_trackxform;

	CStatus st = gApp.ExecuteCommand( L"Clone", args, retval );

	return retval;
}

// ClosestPaintShapeKey
void ClosestPaintShapeKey( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"ClosestPaintShapeKey", args, retval );

	return;
}

// ClusterNonOverlapEnforcement
CValue ClusterNonOverlapEnforcement( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_property, siActionWhenLocalMaterialsOverlap in_actionwhenlocalmaterialsoverlap, const CRef& /*object*/ in_outputobj )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_property;
	args[i++]= (LONG)in_actionwhenlocalmaterialsoverlap;
	args[i++]= in_outputobj;

	CStatus st = gApp.ExecuteCommand( L"ClusterNonOverlapEnforcement", args, retval );

	return retval;
}

// ClusterPropSetValues
void ClusterPropSetValues( const CRef& /*object*/ in_clusterprop )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clusterprop;

	CStatus st = gApp.ExecuteCommand( L"ClusterPropSetValues", args, retval );

	return;
}

// CollapseOp
void CollapseOp( const CValue& /*undefined*/ in_inputobjs, bool in_relative, const CRef& /*object*/ in_collapseoperators )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_relative;
	args[i++]= in_collapseoperators;

	CStatus st = gApp.ExecuteCommand( L"CollapseOp", args, retval );

	return;
}

// ColorizeObject
void ColorizeObject( const CValue& /*number*/ in_colorid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_colorid;

	CStatus st = gApp.ExecuteCommand( L"ColorizeObject", args, retval );

	return;
}

// ConnectBevelProfileCurve
void ConnectBevelProfileCurve( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_source )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_source;

	CStatus st = gApp.ExecuteCommand( L"ConnectBevelProfileCurve", args, retval );

	return;
}

// ConnectFxOp
void ConnectFxOp( const CValue& /*undefined*/ in_op1, const CValue& /*undefined*/ in_op2, const CString& in_connection )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_op1;
	args[i++]= in_op2;
	args[i++]= in_connection;

	CStatus st = gApp.ExecuteCommand( L"ConnectFxOp", args, retval );

	return;
}

// ConnectImageTo3DO
CValue ConnectImageTo3DO( const CValue& /*undefined*/ in_inputobjs, const CString& in_filename )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"ConnectImageTo3DO", args, retval );

	return retval;
}

// ConnectMap
void ConnectMap( const CValue& /*undefined*/ in_propobj, const CValue& /*undefined*/ in_target )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_propobj;
	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"ConnectMap", args, retval );

	return;
}

// ConnectNewPolygonToEdge
CValue ConnectNewPolygonToEdge( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_edgeid, const CValue& /*number*/ in_insertposition, bool in_invertpolygon )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_edgeid;
	args[i++]= in_insertposition;
	args[i++]= in_invertpolygon;

	CStatus st = gApp.ExecuteCommand( L"ConnectNewPolygonToEdge", args, retval );

	return retval;
}

// ConnectNewPolygonToPoint
CValue ConnectNewPolygonToPoint( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_pointid, const CValue& /*number*/ in_insertposition, bool in_invertpolygon )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_pointid;
	args[i++]= in_insertposition;
	args[i++]= in_invertpolygon;

	CStatus st = gApp.ExecuteCommand( L"ConnectNewPolygonToPoint", args, retval );

	return retval;
}

// ConnectOperatorParameter
void ConnectOperatorParameter( const CValue& /*undefined*/ in_parameter, const CValue& /*undefined*/ in_projobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_parameter;
	args[i++]= in_projobj;

	CStatus st = gApp.ExecuteCommand( L"ConnectOperatorParameter", args, retval );

	return;
}

// ConnectParticleInitState
void ConnectParticleInitState( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_initcloud )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_initcloud;

	CStatus st = gApp.ExecuteCommand( L"ConnectParticleInitState", args, retval );

	return;
}

// ConnectPlaneReferenceToSlicePolygonsOp
void ConnectPlaneReferenceToSlicePolygonsOp( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_source )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_source;

	CStatus st = gApp.ExecuteCommand( L"ConnectPlaneReferenceToSlicePolygonsOp", args, retval );

	return;
}

// ConnectSource
void ConnectSource( const CValue& /*undefined*/ in_target, const CRef& /*object*/ in_inputobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"ConnectSource", args, retval );

	return;
}

// ConnectToProjection
void ConnectToProjection( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_propertyname, const CValue& /*undefined*/ in_textureprojection )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_propertyname;
	args[i++]= in_textureprojection;

	CStatus st = gApp.ExecuteCommand( L"ConnectToProjection", args, retval );

	return;
}

// ConnectToSupport
void ConnectToSupport( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_propertyname, const CValue& /*undefined*/ in_texturesupport )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_propertyname;
	args[i++]= in_texturesupport;

	CStatus st = gApp.ExecuteCommand( L"ConnectToSupport", args, retval );

	return;
}

// ConnectToTextureProj
void ConnectToTextureProj( const CValue& /*undefined*/ in_inputobjs, const CString& in_name, const CString& in_texturesupport )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_texturesupport;

	CStatus st = gApp.ExecuteCommand( L"ConnectToTextureProj", args, retval );

	return;
}

// ConnectWeightMap
void ConnectWeightMap( const CValue& /*undefined*/ in_operator, const CValue& /*undefined*/ in_propobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_propobj;

	CStatus st = gApp.ExecuteCommand( L"ConnectWeightMap", args, retval );

	return;
}

// ConnectWeightMapToComponent
void ConnectWeightMapToComponent( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_propobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_propobj;

	CStatus st = gApp.ExecuteCommand( L"ConnectWeightMapToComponent", args, retval );

	return;
}

// Convert
void Convert( const CString& in_target, const CString& in_source )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_source;

	CStatus st = gApp.ExecuteCommand( L"Convert", args, retval );

	return;
}

// ConvertEulerToQuat
void ConvertEulerToQuat( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ConvertEulerToQuat", args, retval );

	return;
}

// ConvertPosAnimToPath
void ConvertPosAnimToPath( const CValue& /*undefined*/ in_inputobjs, const CString& in_name, const CValue& /*undefined*/ in_starttime, const CValue& /*undefined*/ in_endtime, const CValue& /*undefined*/ in_stepframe )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_starttime;
	args[i++]= in_endtime;
	args[i++]= in_stepframe;

	CStatus st = gApp.ExecuteCommand( L"ConvertPosAnimToPath", args, retval );

	return;
}

// ConvertQuatToEuler
void ConvertQuatToEuler( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ConvertQuatToEuler", args, retval );

	return;
}

// ConvertScriptedOp
CValue ConvertScriptedOp( const CValue& /*undefined*/ in_source )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;

	CStatus st = gApp.ExecuteCommand( L"ConvertScriptedOp", args, retval );

	return retval;
}

// ConvertShapeReferenceMode
void ConvertShapeReferenceMode( const CValue& /*undefined*/ in_inputobj, siShapeReferenceMode in_refmode )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= (LONG)in_refmode;

	CStatus st = gApp.ExecuteCommand( L"ConvertShapeReferenceMode", args, retval );

	return;
}

// Copy
void Copy( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"Copy", args, retval );

	return;
}

// CopyAllAnimation2
void CopyAllAnimation2( const CValue& /*undefined*/ in_inputobjs, siSourceType in_sourcemask, siParameterFilter in_parametermask, bool in_removeanimation )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_sourcemask;
	args[i++]= (LONG)in_parametermask;
	args[i++]= in_removeanimation;

	CStatus st = gApp.ExecuteCommand( L"CopyAllAnimation2", args, retval );

	return;
}

// CopyAnimation
void CopyAnimation( const CValue& /*undefined*/ in_inputobjs, bool in_usefcurves, bool in_useexpressions, bool in_removeanimation )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_usefcurves;
	args[i++]= in_useexpressions;
	args[i++]= in_removeanimation;

	CStatus st = gApp.ExecuteCommand( L"CopyAnimation", args, retval );

	return;
}

// CopyAnimContainer
void CopyAnimContainer( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_target )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"CopyAnimContainer", args, retval );

	return;
}

// CopyClipToTrack
CValue CopyClipToTrack( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_clip, const CValue& /*undefined*/ in_compound, const CValue& /*undefined*/ in_track, const CValue& /*undefined*/ in_time )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_clip;
	args[i++]= in_compound;
	args[i++]= in_track;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"CopyClipToTrack", args, retval );

	return retval;
}

// CopyCluster
void CopyCluster( const CValue& /*undefined*/ in_cluster, const CValue& /*undefined*/ in_target )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_cluster;
	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"CopyCluster", args, retval );

	return;
}

// CopyExtFilesIntoProject
void CopyExtFilesIntoProject( const CString& in_scenefilename, bool in_overwrite )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_scenefilename;
	args[i++]= in_overwrite;

	CStatus st = gApp.ExecuteCommand( L"CopyExtFilesIntoProject", args, retval );

	return;
}

// CopyFxOp
void CopyFxOp( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"CopyFxOp", args, retval );

	return;
}

// CopyKeys
void CopyKeys( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, bool in_reset )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_reset;

	CStatus st = gApp.ExecuteCommand( L"CopyKeys", args, retval );

	return;
}

// CopyPartition
void CopyPartition( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CopyPartition", args, retval );

	return;
}

// CopyPaste
void CopyPaste( const CValue& /*undefined*/ in_source, const CString& in_filename, const CValue& /*undefined*/ in_target, const CValue& /*number*/ in_mode )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_filename;
	args[i++]= in_target;
	args[i++]= in_mode;

	CStatus st = gApp.ExecuteCommand( L"CopyPaste", args, retval );

	return;
}

// CopyRegionOptionsFromRender
void CopyRegionOptionsFromRender( const CString& in_from, const CString& in_to, const CString& in_area )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_from;
	args[i++]= in_to;
	args[i++]= in_area;

	CStatus st = gApp.ExecuteCommand( L"CopyRegionOptionsFromRender", args, retval );

	return;
}

// CopyRegionWindowFromRender
void CopyRegionWindowFromRender( const CString& in_from, const CString& in_to )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_from;
	args[i++]= in_to;

	CStatus st = gApp.ExecuteCommand( L"CopyRegionWindowFromRender", args, retval );

	return;
}

// CopyRenderOptions
void CopyRenderOptions( const CString& in_from, const CString& in_to, const CString& in_area )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_from;
	args[i++]= in_to;
	args[i++]= in_area;

	CStatus st = gApp.ExecuteCommand( L"CopyRenderOptions", args, retval );

	return;
}

// CopyRenderOptionsFromRegion
void CopyRenderOptionsFromRegion( const CString& in_from, const CString& in_to, const CString& in_area )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_from;
	args[i++]= in_to;
	args[i++]= in_area;

	CStatus st = gApp.ExecuteCommand( L"CopyRenderOptionsFromRegion", args, retval );

	return;
}

// CopyRenderWindow
void CopyRenderWindow( const CString& in_from, const CString& in_to )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_from;
	args[i++]= in_to;

	CStatus st = gApp.ExecuteCommand( L"CopyRenderWindow", args, retval );

	return;
}

// CopyRenderWindowFromRegion
void CopyRenderWindowFromRegion( const CString& in_from, const CString& in_to )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_from;
	args[i++]= in_to;

	CStatus st = gApp.ExecuteCommand( L"CopyRenderWindowFromRegion", args, retval );

	return;
}

// CopyShape
void CopyShape( const CValue& /*undefined*/ in_source, const CValue& /*undefined*/ in_target )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"CopyShape", args, retval );

	return;
}

// CopySourceActionToFolder
void CopySourceActionToFolder( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_target )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"CopySourceActionToFolder", args, retval );

	return;
}

// CopyStyle
void CopyStyle( const CValue& /*undefined*/ in_targethairobject, const CValue& /*undefined*/ in_modelhairstyleobject )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_targethairobject;
	args[i++]= in_modelhairstyleobject;

	CStatus st = gApp.ExecuteCommand( L"CopyStyle", args, retval );

	return;
}

// CopyUVW
void CopyUVW( const CValue& /*undefined*/ in_textureprojection, const CValue& /*undefined*/ in_subcomp )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_textureprojection;
	args[i++]= in_subcomp;

	CStatus st = gApp.ExecuteCommand( L"CopyUVW", args, retval );

	return;
}

// Create2DMap
CValue Create2DMap( const CValue& /*undefined*/ in_inputobjs, const CString& in_propertyname, siPropagationType in_propagationtype )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_propertyname;
	args[i++]= (LONG)in_propagationtype;

	CStatus st = gApp.ExecuteCommand( L"Create2DMap", args, retval );

	return retval;
}

// Create2DMapWithProjection
CValue Create2DMapWithProjection( const CValue& /*undefined*/ in_inputobjs, const CString& in_propertyname, siPropagationType in_propagationtype, siTxtCreationType in_type, const CString& in_camera, bool in_inspect )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_propertyname;
	args[i++]= (LONG)in_propagationtype;
	args[i++]= (LONG)in_type;
	args[i++]= in_camera;
	args[i++]= in_inspect;

	CStatus st = gApp.ExecuteCommand( L"Create2DMapWithProjection", args, retval );

	return retval;
}

// Create2DSkeleton
CValue Create2DSkeleton( const CValue& /*number*/ in_rx, const CValue& /*number*/ in_ry, const CValue& /*number*/ in_rz, const CValue& /*number*/ in_ex, const CValue& /*number*/ in_ey, const CValue& /*number*/ in_ez, const CValue& /*number*/ in_nx, const CValue& /*number*/ in_ny, const CValue& /*number*/ in_nz, const CValue& /*number*/ in_viewtype, const CRef& /*object*/ in_bone, const CRef& /*object*/ in_effector )
{
	CValueArray args(12);
	CValue retval;
	LONG i(0);

	args[i++]= in_rx;
	args[i++]= in_ry;
	args[i++]= in_rz;
	args[i++]= in_ex;
	args[i++]= in_ey;
	args[i++]= in_ez;
	args[i++]= in_nx;
	args[i++]= in_ny;
	args[i++]= in_nz;
	args[i++]= in_viewtype;
	args[i++]= in_bone;
	args[i++]= in_effector;

	CStatus st = gApp.ExecuteCommand( L"Create2DSkeleton", args, retval );

	return retval;
}

// Create3DSkeleton
CValue Create3DSkeleton( const CValue& /*number*/ in_rx, const CValue& /*number*/ in_ry, const CValue& /*number*/ in_rz, const CValue& /*number*/ in_ex, const CValue& /*number*/ in_ey, const CValue& /*number*/ in_ez, const CValue& /*number*/ in_nx, const CValue& /*number*/ in_ny, const CValue& /*number*/ in_nz, const CValue& /*number*/ in_bone, const CRef& /*object*/ in_effector )
{
	CValueArray args(11);
	CValue retval;
	LONG i(0);

	args[i++]= in_rx;
	args[i++]= in_ry;
	args[i++]= in_rz;
	args[i++]= in_ex;
	args[i++]= in_ey;
	args[i++]= in_ez;
	args[i++]= in_nx;
	args[i++]= in_ny;
	args[i++]= in_nz;
	args[i++]= in_bone;
	args[i++]= in_effector;

	CStatus st = gApp.ExecuteCommand( L"Create3DSkeleton", args, retval );

	return retval;
}

// CreateActiveRigidBody
void CreateActiveRigidBody( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreateActiveRigidBody", args, retval );

	return;
}

// CreateAndEditCommand
CValue CreateAndEditCommand( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"CreateAndEditCommand", args, retval );

	return retval;
}

// CreateChainfromCurve
void CreateChainfromCurve( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_nbjoints, const CValue& /*undefined*/ in_usecns )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_nbjoints;
	args[i++]= in_usecns;

	CStatus st = gApp.ExecuteCommand( L"CreateChainfromCurve", args, retval );

	return;
}

// CreateCluster
CValue CreateCluster( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreateCluster", args, retval );

	return retval;
}

// CreateClusterCenter
CValue CreateClusterCenter( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_s3d_cmdarg_nulllist, bool in_supplymode, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_s3d_cmdarg_nulllist;
	args[i++]= in_supplymode;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"CreateClusterCenter", args, retval );

	return retval;
}

// CreateClusterCenterWithNull
CValue CreateClusterCenterWithNull( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"CreateClusterCenterWithNull", args, retval );

	return retval;
}

// CreateClusterFromSubComponent
CValue CreateClusterFromSubComponent( const CValue& /*undefined*/ in_inputobjs, const CString& in_name, bool in_canconnecttotag )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_canconnecttotag;

	CStatus st = gApp.ExecuteCommand( L"CreateClusterFromSubComponent", args, retval );

	return retval;
}

// CreateClustersAlongDepthFromCurvesToMeshOperator
CValue CreateClustersAlongDepthFromCurvesToMeshOperator( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreateClustersAlongDepthFromCurvesToMeshOperator", args, retval );

	return retval;
}

// CreateClustersFromSlicePolygonsOperator
CValue CreateClustersFromSlicePolygonsOperator( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreateClustersFromSlicePolygonsOperator", args, retval );

	return retval;
}

// CreateCompound
CValue CreateCompound( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_compound, const CValue& /*undefined*/ in_track, const CValue& /*undefined*/ in_time, const CString& in_name )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_inputobj;
	args[i++]= in_compound;
	args[i++]= in_track;
	args[i++]= in_time;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"CreateCompound", args, retval );

	return retval;
}

// CreateConnectionMap
void CreateConnectionMap( const CValue& /*undefined*/ in_obj1, const CValue& /*undefined*/ in_obj2, const CRef& /*object*/ in_mappingtemplate )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_obj1;
	args[i++]= in_obj2;
	args[i++]= in_mappingtemplate;

	CStatus st = gApp.ExecuteCommand( L"CreateConnectionMap", args, retval );

	return;
}

// CreateCPSPreset
void CreateCPSPreset( const CValue& /*undefined*/ in_filename, const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"CreateCPSPreset", args, retval );

	return;
}

// CreateCurve
void CreateCurve( const CValue& /*number*/ in_degree, const CValue& /*number*/ in_curvetype, const CString& in_points, bool in_local, const CRef& /*object*/ in_value )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_degree;
	args[i++]= in_curvetype;
	args[i++]= in_points;
	args[i++]= in_local;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"CreateCurve", args, retval );

	return;
}

// CreateCurvePartitionClusters
void CreateCurvePartitionClusters( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreateCurvePartitionClusters", args, retval );

	return;
}

// CreateDisplayInfo
CValue CreateDisplayInfo( const CValue& /*undefined*/ in_inputobj, const CString& in_psetname, bool in_showdisplayinfo )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_psetname;
	args[i++]= in_showdisplayinfo;

	CStatus st = gApp.ExecuteCommand( L"CreateDisplayInfo", args, retval );

	return retval;
}

// CreateEmptyConnectionMap
void CreateEmptyConnectionMap( const CValue& /*undefined*/ in_obj1, const CRef& /*object*/ in_mappingtemplate )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_obj1;
	args[i++]= in_mappingtemplate;

	CStatus st = gApp.ExecuteCommand( L"CreateEmptyConnectionMap", args, retval );

	return;
}

// CreateEmptyPartition
void CreateEmptyPartition( const CValue& /*undefined*/ in_target, const CString& in_name, siPartitionType in_type, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_name;
	args[i++]= (LONG)in_type;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"CreateEmptyPartition", args, retval );

	return;
}

// CreateEmptyValueMap
void CreateEmptyValueMap( const CValue& /*undefined*/ in_obj1, const CRef& /*object*/ in_mappingtemplate )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_obj1;
	args[i++]= in_mappingtemplate;

	CStatus st = gApp.ExecuteCommand( L"CreateEmptyValueMap", args, retval );

	return;
}

// CreateEnvironment
CValue CreateEnvironment( const CString& in_name, const CRef& /*object*/ in_environment )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_name;
	args[i++]= in_environment;

	CStatus st = gApp.ExecuteCommand( L"CreateEnvironment", args, retval );

	return retval;
}

// CreateEnvironmentCache
CValue CreateEnvironmentCache( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_environment )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_environment;

	CStatus st = gApp.ExecuteCommand( L"CreateEnvironmentCache", args, retval );

	return retval;
}

// CreateEventSource
void CreateEventSource( const CString& in_target, const CRef& /*object*/ in_output )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_output;

	CStatus st = gApp.ExecuteCommand( L"CreateEventSource", args, retval );

	return;
}

// CreateExplosion
CValue CreateExplosion( const CString& in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreateExplosion", args, retval );

	return retval;
}

// CreateFluid
CValue CreateFluid( const CString& in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreateFluid", args, retval );

	return retval;
}

// CreateFolder
void CreateFolder( const CValue& /*undefined*/ in_target, const CString& in_name, const CRef& /*object*/ in_folder )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_name;
	args[i++]= in_folder;

	CStatus st = gApp.ExecuteCommand( L"CreateFolder", args, retval );

	return;
}

// CreateForce
CValue CreateForce( const CString& in_name, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_name;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreateForce", args, retval );

	return retval;
}

// CreateFxTree
CValue CreateFxTree( const CRef& /*object*/ in_value )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"CreateFxTree", args, retval );

	return retval;
}

// CreateGridAndConnectToSlicePolygonsOp
CValue CreateGridAndConnectToSlicePolygonsOp( const CValue& /*undefined*/ in_target )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"CreateGridAndConnectToSlicePolygonsOp", args, retval );

	return retval;
}

// CreateGroup
CValue CreateGroup( const CString& in_name, const CValue& /*undefined*/ in_inputobjs, const CString& in_parent )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_name;
	args[i++]= in_inputobjs;
	args[i++]= in_parent;

	CStatus st = gApp.ExecuteCommand( L"CreateGroup", args, retval );

	return retval;
}

// CreateHierarchyFromBiped
CValue CreateHierarchyFromBiped( const CString& in_guidename, const CValue& /*number*/ in_hierarchystyle )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_guidename;
	args[i++]= in_hierarchystyle;

	CStatus st = gApp.ExecuteCommand( L"CreateHierarchyFromBiped", args, retval );

	return retval;
}

// CreateImageBasedLightingPass
void CreateImageBasedLightingPass( const CString& in_filename, const CValue& /*undefined*/ in_clip )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"CreateImageBasedLightingPass", args, retval );

	return;
}

// CreateImageClip
CValue CreateImageClip( const CString& in_filename, const CString& in_name, const CString& in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"CreateImageClip", args, retval );

	return retval;
}

// CreateImageClip2
CValue CreateImageClip2( const CString& in_filename, const CString& in_name, const CString& in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"CreateImageClip2", args, retval );

	return retval;
}

// CreateInspectParticleTypeBillboard
void CreateInspectParticleTypeBillboard( const CString& in_rendertype )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_rendertype;

	CStatus st = gApp.ExecuteCommand( L"CreateInspectParticleTypeBillboard", args, retval );

	return;
}

// CreateInspectParticleTypeBlob
void CreateInspectParticleTypeBlob( const CString& in_rendertype )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_rendertype;

	CStatus st = gApp.ExecuteCommand( L"CreateInspectParticleTypeBlob", args, retval );

	return;
}

// CreateInspectParticleTypeSphere
void CreateInspectParticleTypeSphere( const CString& in_rendertype )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_rendertype;

	CStatus st = gApp.ExecuteCommand( L"CreateInspectParticleTypeSphere", args, retval );

	return;
}

// CreateLayer
void CreateLayer( const CString& in_presetobj, const CString& in_name, const CValue& /*undefined*/ in_inputobjs, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_inputobjs;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"CreateLayer", args, retval );

	return;
}

// CreateLibrary
void CreateLibrary( const CString& in_name, const CValue& /*number*/ in_type, CRef& /*object*/ io_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_name;
	args[i++]= in_type;
	args[i++]= CValue();

	CStatus st = gApp.ExecuteCommand( L"CreateLibrary", args, retval );

	io_value = args[2];

	return;
}

// CreateLightRigFromImage
void CreateLightRigFromImage( const CString& in_filename, const CValue& /*number*/ in_number_layers, const CValue& /*number*/ in_areahigh, const CValue& /*number*/ in_lightdisthigh )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_number_layers;
	args[i++]= in_areahigh;
	args[i++]= in_lightdisthigh;

	CStatus st = gApp.ExecuteCommand( L"CreateLightRigFromImage", args, retval );

	return;
}

// CreateMarkingSet
void CreateMarkingSet( const CValue& /*undefined*/ in_inputobjs, bool in_reset )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_reset;

	CStatus st = gApp.ExecuteCommand( L"CreateMarkingSet", args, retval );

	return;
}

// CreateMeshPartitionClusters
void CreateMeshPartitionClusters( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreateMeshPartitionClusters", args, retval );

	return;
}

// CreateMeshText
CValue CreateMeshText( const CValue& /*undefined*/ in_presetobj, siOperationMode in_immediatemode )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= (LONG)in_immediatemode;

	CStatus st = gApp.ExecuteCommand( L"CreateMeshText", args, retval );

	return retval;
}

// CreateModel
void CreateModel( const CString& in_inputobj, const CString& in_name, const CString& in_parent, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"CreateModel", args, retval );

	return;
}

// CreateModelClip
CValue CreateModelClip( const CString& in_filename, const CString& in_name, const CString& in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"CreateModelClip", args, retval );

	return retval;
}

// CreateNonOverlappingCluster
CValue CreateNonOverlappingCluster( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreateNonOverlappingCluster", args, retval );

	return retval;
}

// CreateObjectFromPreset
CValue CreateObjectFromPreset( const CValue& /*undefined*/ in_presetobj, const CString& in_name )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"CreateObjectFromPreset", args, retval );

	return retval;
}

// CreateOffsetEffect
void CreateOffsetEffect( const CValue& /*undefined*/ in_clip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"CreateOffsetEffect", args, retval );

	return;
}

// CreateParticleCloud
CValue CreateParticleCloud( const CValue& /*undefined*/ in_inputobjs, const CString& in_prim )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_prim;

	CStatus st = gApp.ExecuteCommand( L"CreateParticleCloud", args, retval );

	return retval;
}

// CreateParticleSimulationFromFile
CValue CreateParticleSimulationFromFile( const CString& in_filename )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"CreateParticleSimulationFromFile", args, retval );

	return retval;
}

// CreateParticleType
void CreateParticleType( siParticleRenderType in_rendertype, const CRef& /*object*/ in_output )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= (LONG)in_rendertype;
	args[i++]= in_output;

	CStatus st = gApp.ExecuteCommand( L"CreateParticleType", args, retval );

	return;
}

// CreatePartition
void CreatePartition( const CString& in_target, const CString& in_name, const CString& in_inputobjs, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_name;
	args[i++]= in_inputobjs;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"CreatePartition", args, retval );

	return;
}

// CreatePass
void CreatePass( const CString& in_presetobj, const CString& in_name, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"CreatePass", args, retval );

	return;
}

// CreatePassFromLayers
void CreatePassFromLayers( const CString& in_name, const CRef& /*object*/ in_value )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_name;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"CreatePassFromLayers", args, retval );

	return;
}

// CreatePassiveRigidBody
void CreatePassiveRigidBody( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreatePassiveRigidBody", args, retval );

	return;
}

// CreatePassWithPartition
void CreatePassWithPartition( const CString& in_presetobj, const CString& in_name, const CString& in_paramname, const CValue& /*undefined*/ in_inputobjs, const CRef& /*object*/ in_value )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_paramname;
	args[i++]= in_inputobjs;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"CreatePassWithPartition", args, retval );

	return;
}

// CreatePreset
CValue CreatePreset( const CString& in_name, const CString& in_family, const CString& in_path )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_name;
	args[i++]= in_family;
	args[i++]= in_path;

	CStatus st = gApp.ExecuteCommand( L"CreatePreset", args, retval );

	return retval;
}

// CreatePrim
void CreatePrim( const CString& in_presetobj, const CString& in_geometrytype, const CString& in_name, const CString& in_parent )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_geometrytype;
	args[i++]= in_name;
	args[i++]= in_parent;

	CStatus st = gApp.ExecuteCommand( L"CreatePrim", args, retval );

	return;
}

// CreateProjection
void CreateProjection( const CValue& /*undefined*/ in_inputobjs, siTxtCreationType in_type, siTxtDefaultCreationType in_uvdefaulttype, const CValue& /*undefined*/ in_supportname, const CValue& /*undefined*/ in_propertyname, bool in_parenting, siTxtProjFitMethod in_fitting, const CString& in_camera )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_type;
	args[i++]= (LONG)in_uvdefaulttype;
	args[i++]= in_supportname;
	args[i++]= in_propertyname;
	args[i++]= in_parenting;
	args[i++]= (LONG)in_fitting;
	args[i++]= in_camera;

	CStatus st = gApp.ExecuteCommand( L"CreateProjection", args, retval );

	return;
}

// CreateReferencePlane
CValue CreateReferencePlane( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreateReferencePlane", args, retval );

	return retval;
}

// CreateRigFromGuide
void CreateRigFromGuide( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"CreateRigFromGuide", args, retval );

	return;
}

// CreateRigFromQuadruped
void CreateRigFromQuadruped( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"CreateRigFromQuadruped", args, retval );

	return;
}

// CreateScaleEffect
void CreateScaleEffect( const CValue& /*undefined*/ in_clip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"CreateScaleEffect", args, retval );

	return;
}

// CreateSourceFileFromDoc
CValue CreateSourceFileFromDoc( const CValue& /*undefined*/ in_tag, const CValue& /*undefined*/ in_method, const CValue& /*undefined*/ in_lang )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_tag;
	args[i++]= in_method;
	args[i++]= in_lang;

	CStatus st = gApp.ExecuteCommand( L"CreateSourceFileFromDoc", args, retval );

	return retval;
}

// CreateSpine
CValue CreateSpine( const CValue& /*undefined*/ in_hip, const CValue& /*undefined*/ in_chest, const CValue& /*number*/ in_numvertebrae, bool in_implicitvertebrae, bool in_interactive )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_hip;
	args[i++]= in_chest;
	args[i++]= in_numvertebrae;
	args[i++]= in_implicitvertebrae;
	args[i++]= in_interactive;

	CStatus st = gApp.ExecuteCommand( L"CreateSpine", args, retval );

	return retval;
}

// CreateSubprojection
void CreateSubprojection( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_textureprojection, siTxtProjType in_type, siTxtProjPlane in_plane, const CString& in_supportname, bool in_parenting, siTxtProjFitMethod in_fitting, bool in_fittoimage, bool in_maintainaspect, const CRef& /*object*/ in_3dobjtexture )
{
	CValueArray args(10);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_textureprojection;
	args[i++]= (LONG)in_type;
	args[i++]= (LONG)in_plane;
	args[i++]= in_supportname;
	args[i++]= in_parenting;
	args[i++]= (LONG)in_fitting;
	args[i++]= in_fittoimage;
	args[i++]= in_maintainaspect;
	args[i++]= in_3dobjtexture;

	CStatus st = gApp.ExecuteCommand( L"CreateSubprojection", args, retval );

	return;
}

// CreateSymmetryMap
CValue CreateSymmetryMap( const CString& in_presetobj, const CValue& /*undefined*/ in_inputobj, const CString& in_name, const CString& in_proppresetobj )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobj;
	args[i++]= in_name;
	args[i++]= in_proppresetobj;

	CStatus st = gApp.ExecuteCommand( L"CreateSymmetryMap", args, retval );

	return retval;
}

// CreateSymmetryMappingTemplate
CValue CreateSymmetryMappingTemplate( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_filltemplate, const CValue& /*number*/ in_symmetry_plane, bool in_createsymmetrymap )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_filltemplate;
	args[i++]= in_symmetry_plane;
	args[i++]= in_createsymmetrymap;

	CStatus st = gApp.ExecuteCommand( L"CreateSymmetryMappingTemplate", args, retval );

	return retval;
}

// CreateTextureProj
void CreateTextureProj( const CValue& /*undefined*/ in_inputobjs, const CString& in_projectionname, bool in_parenting, siTxtProjType in_projectiontype, siTxtProjPlane in_projectionplane )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_projectionname;
	args[i++]= in_parenting;
	args[i++]= (LONG)in_projectiontype;
	args[i++]= (LONG)in_projectionplane;

	CStatus st = gApp.ExecuteCommand( L"CreateTextureProj", args, retval );

	return;
}

// CreateTextureSpatial
void CreateTextureSpatial( const CValue& /*undefined*/ in_inputobjs, const CString& in_name )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"CreateTextureSpatial", args, retval );

	return;
}

// CreateTextureSupport
void CreateTextureSupport( const CValue& /*undefined*/ in_inputobjs, siTxtCreationType in_type, siTxtDefaultCreationType in_uvdefaulttype, const CString& in_name, bool in_parenting )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_type;
	args[i++]= (LONG)in_uvdefaulttype;
	args[i++]= in_name;
	args[i++]= in_parenting;

	CStatus st = gApp.ExecuteCommand( L"CreateTextureSupport", args, retval );

	return;
}

// CreateTextureUVDomain
void CreateTextureUVDomain( const CValue& /*undefined*/ in_inputobjs, const CString& in_supportname, const CString& in_propertyname )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_supportname;
	args[i++]= in_propertyname;

	CStatus st = gApp.ExecuteCommand( L"CreateTextureUVDomain", args, retval );

	return;
}

// CreateUserDataMap
CValue CreateUserDataMap( const CValue& /*undefined*/ in_inputobj, const CString& in_propertyname, const CValue& /*undefined*/ in_template )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_propertyname;
	args[i++]= in_template;

	CStatus st = gApp.ExecuteCommand( L"CreateUserDataMap", args, retval );

	return retval;
}

// CreateUserDataMapAndPickTemplate
CValue CreateUserDataMapAndPickTemplate( const CValue& /*undefined*/ in_inputobj, const CString& in_propertyname )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_propertyname;

	CStatus st = gApp.ExecuteCommand( L"CreateUserDataMapAndPickTemplate", args, retval );

	return retval;
}

// CreateValueMap
void CreateValueMap( const CValue& /*undefined*/ in_obj1, const CValue& /*undefined*/ in_obj2, const CValue& /*number*/ in_mode, const CValue& /*number*/ in_scope, bool in_usenames, const CRef& /*object*/ in_mappingtemplate )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_obj1;
	args[i++]= in_obj2;
	args[i++]= in_mode;
	args[i++]= in_scope;
	args[i++]= in_usenames;
	args[i++]= in_mappingtemplate;

	CStatus st = gApp.ExecuteCommand( L"CreateValueMap", args, retval );

	return;
}

// CreateVertexColorSupport
CValue CreateVertexColorSupport( const CString& in_presetobj, const CString& in_propertyname, const CValue& /*undefined*/ in_inputobjs)
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_propertyname;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"CreateVertexColorSupport", args, retval );

	return retval;
}

// CreateWeightMap
CValue CreateWeightMap( const CString& in_presetobj, const CValue& /*undefined*/ in_inputobj, const CString& in_propertyname, const CString& in_proppresetobj, bool in_select )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobj;
	args[i++]= in_propertyname;
	args[i++]= in_proppresetobj;
	args[i++]= in_select;

	CStatus st = gApp.ExecuteCommand( L"CreateWeightMap", args, retval );

	return retval;
}

// CrossfadeClipWeights
void CrossfadeClipWeights( const CValue& /*undefined*/ in_firstclip, const CValue& /*undefined*/ in_secondclip )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_firstclip;
	args[i++]= in_secondclip;

	CStatus st = gApp.ExecuteCommand( L"CrossfadeClipWeights", args, retval );

	return;
}

// CurrentFxTracking
void CurrentFxTracking( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"CurrentFxTracking", args, retval );

	return;
}

// Cut
void Cut( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"Cut", args, retval );

	return;
}

// CutClip
CValue CutClip( const CValue& /*undefined*/ in_clip, siCutClipOptions in_mode, const CValue& /*undefined*/ in_time )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;
	args[i++]= (LONG)in_mode;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"CutClip", args, retval );

	return retval;
}

// CutClipInSpace
CValue CutClipInSpace( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_clip )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_inputobjs;
	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"CutClipInSpace", args, retval );

	return retval;
}

// CutFxOp
void CutFxOp( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"CutFxOp", args, retval );

	return;
}

// CutKeys
void CutKeys( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, bool in_reset, bool in_ripple )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_reset;
	args[i++]= in_ripple;

	CStatus st = gApp.ExecuteCommand( L"CutKeys", args, retval );

	return;
}

// CutObj
void CutObj( const CValue& /*undefined*/ in_children )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_children;

	CStatus st = gApp.ExecuteCommand( L"CutObj", args, retval );

	return;
}

// CycleChecking
CValue CycleChecking( const CValue& /*undefined*/ in_startinggraphpt, bool in_check_current_frame, bool in_iscycle )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_startinggraphpt;
	args[i++]= in_check_current_frame;
	args[i++]= in_iscycle;

	CStatus st = gApp.ExecuteCommand( L"CycleChecking", args, retval );

	return retval;
}

// CycleUVW
CValue CycleUVW( const CValue& /*undefined*/ in_textureprojection, const CValue& /*undefined*/ in_subcomp, const CValue& /*number*/ in_offset )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_textureprojection;
	args[i++]= in_subcomp;
	args[i++]= in_offset;

	CStatus st = gApp.ExecuteCommand( L"CycleUVW", args, retval );

	return retval;
}

// DeactivateAbove
void DeactivateAbove( const CValue& /*undefined*/ in_operator, bool in_mute )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_mute;

	CStatus st = gApp.ExecuteCommand( L"DeactivateAbove", args, retval );

	return;
}

// DeactivateElements
void DeactivateElements( const CValue& /*undefined*/ in_selectionlist )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;

	CStatus st = gApp.ExecuteCommand( L"DeactivateElements", args, retval );

	return;
}

// DeactivateExternalConstraints
CValue DeactivateExternalConstraints( const CValue& /*undefined*/ in_inputobj, const CString& in_operation )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_operation;

	CStatus st = gApp.ExecuteCommand( L"DeactivateExternalConstraints", args, retval );

	return retval;
}

// DecPaintShapePointWeight
void DecPaintShapePointWeight( const CValue& /*undefined*/ in_shape )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;

	CStatus st = gApp.ExecuteCommand( L"DecPaintShapePointWeight", args, retval );

	return;
}

// DeformMotion
void DeformMotion( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"DeformMotion", args, retval );

	return;
}

// Delete2DShapePoint
void Delete2DShapePoint( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_index )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_index;

	CStatus st = gApp.ExecuteCommand( L"Delete2DShapePoint", args, retval );

	return;
}

// DeleteAll
void DeleteAll( bool in_confirm )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_confirm;

	CStatus st = gApp.ExecuteCommand( L"DeleteAll", args, retval );

	return;
}

// DeleteAllPaintShapes
void DeleteAllPaintShapes( const CValue& /*undefined*/ in_shapelist )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_shapelist;

	CStatus st = gApp.ExecuteCommand( L"DeleteAllPaintShapes", args, retval );

	return;
}

// DeleteAllUnusedMaterials
void DeleteAllUnusedMaterials( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"DeleteAllUnusedMaterials", args, retval );

	return;
}

// DeleteCurrentLayer
void DeleteCurrentLayer( const CString& in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"DeleteCurrentLayer", args, retval );

	return;
}

// DeleteCurrentPass
void DeleteCurrentPass( const CString& in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"DeleteCurrentPass", args, retval );

	return;
}

// DeleteGeneratorInputs
void DeleteGeneratorInputs( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"DeleteGeneratorInputs", args, retval );

	return;
}

// DeleteHelperBone
CValue DeleteHelperBone( const CValue& /*undefined*/ in_helper, bool in_confirm )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_helper;
	args[i++]= in_confirm;

	CStatus st = gApp.ExecuteCommand( L"DeleteHelperBone", args, retval );

	return retval;
}

// DeleteHelperTrigger
CValue DeleteHelperTrigger( const CValue& /*undefined*/ in_helper, const CValue& /*number*/ in_index, bool in_confirm )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_helper;
	args[i++]= in_index;
	args[i++]= in_confirm;

	CStatus st = gApp.ExecuteCommand( L"DeleteHelperTrigger", args, retval );

	return retval;
}

// DeleteMappingRule
void DeleteMappingRule( const CValue& /*undefined*/ in_mappingtemplate, const CValue& /*number*/ in_index )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_mappingtemplate;
	args[i++]= in_index;

	CStatus st = gApp.ExecuteCommand( L"DeleteMappingRule", args, retval );

	return;
}

// DeleteMarkingSet
void DeleteMarkingSet( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"DeleteMarkingSet", args, retval );

	return;
}

// DeleteModelFromObj
void DeleteModelFromObj( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"DeleteModelFromObj", args, retval );

	return;
}

// DeleteMotionDerivator
void DeleteMotionDerivator( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"DeleteMotionDerivator", args, retval );

	return;
}

// DeleteObj
void DeleteObj( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"DeleteObj", args, retval );

	return;
}

// DeleteOverride
void DeleteOverride( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"DeleteOverride", args, retval );

	return;
}

// DeletePaintShapeKey
void DeletePaintShapeKey( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"DeletePaintShapeKey", args, retval );

	return;
}

// DeletePaintShapePoints
void DeletePaintShapePoints( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_index )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_index;

	CStatus st = gApp.ExecuteCommand( L"DeletePaintShapePoints", args, retval );

	return;
}

// DeleteUnusedImageClips
void DeleteUnusedImageClips( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"DeleteUnusedImageClips", args, retval );

	return;
}

// DeleteUnusedImageSources
void DeleteUnusedImageSources( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"DeleteUnusedImageSources", args, retval );

	return;
}

// DeleteUnusedMaterials
void DeleteUnusedMaterials( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"DeleteUnusedMaterials", args, retval );

	return;
}

// DeleteUnusedMixerTracks
void DeleteUnusedMixerTracks( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"DeleteUnusedMixerTracks", args, retval );

	return;
}

// DelPointOnCurveTool
void DelPointOnCurveTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"DelPointOnCurveTool", args, retval );

	return;
}

// DeselectAll
void DeselectAll( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"DeselectAll", args, retval );

	return;
}

// DeselectAllUsingFilter
void DeselectAllUsingFilter( const CValue& /*undefined*/ in_selfilter )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selfilter;

	CStatus st = gApp.ExecuteCommand( L"DeselectAllUsingFilter", args, retval );

	return;
}

// DisableAllDevices
void DisableAllDevices( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"DisableAllDevices", args, retval );

	return;
}

// DisconnectAndDeleteShaders
void DisconnectAndDeleteShaders( const CValue& /*undefined*/ in_shaders )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_shaders;

	CStatus st = gApp.ExecuteCommand( L"DisconnectAndDeleteShaders", args, retval );

	return;
}

// DisconnectBevelProfileCurve
void DisconnectBevelProfileCurve( const CValue& /*undefined*/ in_target )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"DisconnectBevelProfileCurve", args, retval );

	return;
}

// DisconnectFromSimulationOp
void DisconnectFromSimulationOp( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_objectstodisconnect )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_objectstodisconnect;

	CStatus st = gApp.ExecuteCommand( L"DisconnectFromSimulationOp", args, retval );

	return;
}

// DisconnectFxOp
void DisconnectFxOp( const CValue& /*undefined*/ in_op, const CString& in_connection )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_op;
	args[i++]= in_connection;

	CStatus st = gApp.ExecuteCommand( L"DisconnectFxOp", args, retval );

	return;
}

// DisconnectOperatorParameter
void DisconnectOperatorParameter( const CValue& /*undefined*/ in_parameter )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_parameter;

	CStatus st = gApp.ExecuteCommand( L"DisconnectOperatorParameter", args, retval );

	return;
}

// DisconnectParticleInitState
void DisconnectParticleInitState( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"DisconnectParticleInitState", args, retval );

	return;
}

// DisconnectPlaneReferenceFromSlicePolygonsOp
void DisconnectPlaneReferenceFromSlicePolygonsOp( const CValue& /*undefined*/ in_target )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"DisconnectPlaneReferenceFromSlicePolygonsOp", args, retval );

	return;
}

// DisposeRenderingCache
void DisposeRenderingCache( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"DisposeRenderingCache", args, retval );

	return;
}

// DrawImplicitPrimitiveTool
void DrawImplicitPrimitiveTool( const CString& in_presetobj, const CString& in_name, const CString& in_parent )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_parent;

	CStatus st = gApp.ExecuteCommand( L"DrawImplicitPrimitiveTool", args, retval );

	return;
}

// DrawNullTool
void DrawNullTool( const CString& in_presetobj, const CString& in_name, const CString& in_parent )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_parent;

	CStatus st = gApp.ExecuteCommand( L"DrawNullTool", args, retval );

	return;
}

// DrawPrimitiveTool
void DrawPrimitiveTool( const CString& in_presetobj, const CString& in_geometrytype, const CString& in_name, const CString& in_parent )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_geometrytype;
	args[i++]= in_name;
	args[i++]= in_parent;

	CStatus st = gApp.ExecuteCommand( L"DrawPrimitiveTool", args, retval );

	return;
}

// Duplicate
CValue Duplicate( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_nbitems, siDupHistory in_history, siDupParent in_hierarchy, siDupGroup in_grouping, siDupProperty in_properties, siDupAnimation in_animation, siDupConstraint in_constraints, siDupSelection in_selection, siDupTransform in_xfrom, const CString& in_sx, const CString& in_sy, const CString& in_sz, const CString& in_rx, const CString& in_ry, const CString& in_rz, const CString& in_tx, const CString& in_ty, const CString& in_tz, const CString& in_trackxform, siDupTextureSupport in_texturesupports )
{
	CValueArray args(21);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_nbitems;
	args[i++]= (LONG)in_history;
	args[i++]= (LONG)in_hierarchy;
	args[i++]= (LONG)in_grouping;
	args[i++]= (LONG)in_properties;
	args[i++]= (LONG)in_animation;
	args[i++]= (LONG)in_constraints;
	args[i++]= (LONG)in_selection;
	args[i++]= (LONG)in_xfrom;
	args[i++]= in_sx;
	args[i++]= in_sy;
	args[i++]= in_sz;
	args[i++]= in_rx;
	args[i++]= in_ry;
	args[i++]= in_rz;
	args[i++]= in_tx;
	args[i++]= in_ty;
	args[i++]= in_tz;
	args[i++]= in_trackxform;
	args[i++]= (LONG)in_texturesupports;

	CStatus st = gApp.ExecuteCommand( L"Duplicate", args, retval );

	return retval;
}

// DuplicateFromAnim
CValue DuplicateFromAnim( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, const CValue& /*number*/ in_stepframe, bool in_doinstance )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_stepframe;
	args[i++]= in_doinstance;

	CStatus st = gApp.ExecuteCommand( L"DuplicateFromAnim", args, retval );

	return retval;
}

// DuplicateMeshComponent
void DuplicateMeshComponent( const CString& in_connectionset, siOperationMode in_immediatemode, const CValue& /*number*/ in_length, const CValue& /*number*/ in_nbsubdivs, bool in_merge, const CValue& /*number*/ in_tx, const CValue& /*number*/ in_ty, const CValue& /*number*/ in_tz, const CValue& /*number*/ in_rx, const CValue& /*number*/ in_ry, const CValue& /*number*/ in_rz, const CValue& /*number*/ in_sx, const CValue& /*number*/ in_sy, const CValue& /*number*/ in_sz, bool in_duplicatepolygons )
{
	CValueArray args(15);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= (LONG)in_immediatemode;
	args[i++]= in_length;
	args[i++]= in_nbsubdivs;
	args[i++]= in_merge;
	args[i++]= in_tx;
	args[i++]= in_ty;
	args[i++]= in_tz;
	args[i++]= in_rx;
	args[i++]= in_ry;
	args[i++]= in_rz;
	args[i++]= in_sx;
	args[i++]= in_sy;
	args[i++]= in_sz;
	args[i++]= in_duplicatepolygons;

	CStatus st = gApp.ExecuteCommand( L"DuplicateMeshComponent", args, retval );

	return;
}

// DuplicateModelFromObj
void DuplicateModelFromObj( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"DuplicateModelFromObj", args, retval );

	return;
}

// DuplicateSymmetry
CValue DuplicateSymmetry( const CValue& /*undefined*/ in_inputobjs, bool in_duplicateconstraints, bool in_shareparent, const CValue& /*number*/ in_symmetryplanea, const CValue& /*number*/ in_symmetryplaneb, const CValue& /*number*/ in_symmetryplanec, const CValue& /*number*/ in_symmetryplaned, bool in_s3d_cmdarg_freezescl )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_duplicateconstraints;
	args[i++]= in_shareparent;
	args[i++]= in_symmetryplanea;
	args[i++]= in_symmetryplaneb;
	args[i++]= in_symmetryplanec;
	args[i++]= in_symmetryplaned;
	args[i++]= in_s3d_cmdarg_freezescl;

	CStatus st = gApp.ExecuteCommand( L"DuplicateSymmetry", args, retval );

	return retval;
}

// DuplicateTool
void DuplicateTool( const CValue& /*undefined*/ in_selectionlist )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;

	CStatus st = gApp.ExecuteCommand( L"DuplicateTool", args, retval );

	return;
}

// DuplicateUsingGeomApprox
CValue DuplicateUsingGeomApprox( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"DuplicateUsingGeomApprox", args, retval );

	return retval;
}

// EditClip
void EditClip( const CValue& /*undefined*/ in_inputobj, siEditClipOptions in_mode, const CValue& /*undefined*/ in_time )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= (LONG)in_mode;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"EditClip", args, retval );

	return;
}

// EditCommand
CValue EditCommand( const CString& in_commandname )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_commandname;

	CStatus st = gApp.ExecuteCommand( L"EditCommand", args, retval );

	return retval;
}

// EditExpr
void EditExpr( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"EditExpr", args, retval );

	return;
}

// EditHelperBones
void EditHelperBones( const CValue& /*undefined*/ in_rig )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_rig;

	CStatus st = gApp.ExecuteCommand( L"EditHelperBones", args, retval );

	return;
}

// EditParameterDefinition
void EditParameterDefinition( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_paramname, const CValue& /*number*/ in_vartype, const CValue& /*undefined*/ in_minvalue, const CValue& /*undefined*/ in_maxvalue, const CValue& /*undefined*/ in_suggmin, const CValue& /*undefined*/ in_suggmax, const CValue& /*undefined*/ in_scriptname, const CValue& /*undefined*/ in_description )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_paramname;
	args[i++]= in_vartype;
	args[i++]= in_minvalue;
	args[i++]= in_maxvalue;
	args[i++]= in_suggmin;
	args[i++]= in_suggmax;
	args[i++]= in_scriptname;
	args[i++]= in_description;

	CStatus st = gApp.ExecuteCommand( L"EditParameterDefinition", args, retval );

	return;
}

// EllipsePaintRaster
void EllipsePaintRaster( const CValue& /*number*/ in_x1, const CValue& /*number*/ in_y1, const CValue& /*number*/ in_x2, const CValue& /*number*/ in_y2, bool in_fromcenter )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_x1;
	args[i++]= in_y1;
	args[i++]= in_x2;
	args[i++]= in_y2;
	args[i++]= in_fromcenter;

	CStatus st = gApp.ExecuteCommand( L"EllipsePaintRaster", args, retval );

	return;
}

// EllipticalPaintMask
CValue EllipticalPaintMask( const CValue& /*number*/ in_x1, const CValue& /*number*/ in_y1, const CValue& /*number*/ in_x2, const CValue& /*number*/ in_y2, bool in_fromcenter, const CValue& /*number*/ in_selectionaction, bool in_antialiased, const CValue& /*number*/ in_feather, const CRef& /*object*/ in_value )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_x1;
	args[i++]= in_y1;
	args[i++]= in_x2;
	args[i++]= in_y2;
	args[i++]= in_fromcenter;
	args[i++]= in_selectionaction;
	args[i++]= in_antialiased;
	args[i++]= in_feather;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"EllipticalPaintMask", args, retval );

	return retval;
}

// EnableAllDevices
void EnableAllDevices( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"EnableAllDevices", args, retval );

	return;
}

// EndUndo
void EndUndo( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"EndUndo", args, retval );

	return;
}

// EnumElements
CValue EnumElements( const CValue& /*undefined*/ in_inputobj, bool in_enumchildren, const CValue& /*number*/ in_capabilities )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_enumchildren;
	args[i++]= in_capabilities;

	CStatus st = gApp.ExecuteCommand( L"EnumElements", args, retval );

	return retval;
}

// EqualizeAction
CValue EqualizeAction( const CValue& /*undefined*/ in_source, const CValue& /*number*/ in_time, const CValue& /*number*/ in_duration, const CString& in_name )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_time;
	args[i++]= in_duration;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"EqualizeAction", args, retval );

	return retval;
}

// EqualizeClips
void EqualizeClips( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"EqualizeClips", args, retval );

	return;
}

// EvalFCurve
CValue EvalFCurve( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"EvalFCurve", args, retval );

	return retval;
}

// EvaluateCurveAt
void EvaluateCurveAt( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_value, bool in_local, const CValue& /*number*/ in_posx, const CValue& /*number*/ in_posy, const CValue& /*number*/ in_posz, const CValue& /*number*/ in_tanx, const CValue& /*number*/ in_tany, const CValue& /*number*/ in_tanz, const CValue& /*number*/ in_norx, const CValue& /*number*/ in_nory, const CValue& /*number*/ in_norz )
{
	CValueArray args(12);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_value;
	args[i++]= in_local;
	args[i++]= in_posx;
	args[i++]= in_posy;
	args[i++]= in_posz;
	args[i++]= in_tanx;
	args[i++]= in_tany;
	args[i++]= in_tanz;
	args[i++]= in_norx;
	args[i++]= in_nory;
	args[i++]= in_norz;

	CStatus st = gApp.ExecuteCommand( L"EvaluateCurveAt", args, retval );

	return;
}

// ExplodeCompound
void ExplodeCompound( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_compound )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_compound;

	CStatus st = gApp.ExecuteCommand( L"ExplodeCompound", args, retval );

	return;
}

// ExportAction
void ExportAction( const CValue& /*undefined*/ in_action, const CValue& /*undefined*/ in_filename )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_action;
	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"ExportAction", args, retval );

	return;
}

// ExportAllPassesToMI2
void ExportAllPassesToMI2( bool in_passcentric )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_passcentric;

	CStatus st = gApp.ExecuteCommand( L"ExportAllPassesToMI2", args, retval );

	return;
}

// ExportDotXSI
void ExportDotXSI( const CValue& /*undefined*/ in_model, const CString& in_filename )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"ExportDotXSI", args, retval );

	return;
}

// ExportHelperBones
void ExportHelperBones( const CValue& /*undefined*/ in_rig, const CString& in_filename, bool in_prompt )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_rig;
	args[i++]= in_filename;
	args[i++]= in_prompt;

	CStatus st = gApp.ExecuteCommand( L"ExportHelperBones", args, retval );

	return;
}

// ExportImageClip
void ExportImageClip( const CValue& /*undefined*/ in_clip, const CString& in_filename, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, const CValue& /*number*/ in_stepframe )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;
	args[i++]= in_filename;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_stepframe;

	CStatus st = gApp.ExecuteCommand( L"ExportImageClip", args, retval );

	return;
}

// ExportMaterialLibrary
void ExportMaterialLibrary( const CValue& /*undefined*/ in_filename, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ExportMaterialLibrary", args, retval );

	return;
}

// ExportMI2File
void ExportMI2File( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_startframe, const CValue& /*undefined*/ in_endframe, const CValue& /*undefined*/ in_stepframe, const CString& in_filename, const CValue& /*undefined*/ in_approximatepolygons, const CValue& /*undefined*/ in_approximatefaces, const CValue& /*undefined*/ in_asciioutput, const CValue& /*undefined*/ in_verbatimtextures, const CValue& /*undefined*/ in_multiplefiles, const CValue& /*undefined*/ in_incremental )
{
	CValueArray args(12);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_presetobj;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_stepframe;
	args[i++]= in_filename;
	args[i++]= in_approximatepolygons;
	args[i++]= in_approximatefaces;
	args[i++]= in_asciioutput;
	args[i++]= in_verbatimtextures;
	args[i++]= in_multiplefiles;
	args[i++]= in_incremental;

	CStatus st = gApp.ExecuteCommand( L"ExportMI2File", args, retval );

	return;
}

// ExportMixer
void ExportMixer( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_filename, const CValue& /*number*/ in_tracktypes )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_filename;
	args[i++]= in_tracktypes;

	CStatus st = gApp.ExecuteCommand( L"ExportMixer", args, retval );

	return;
}

// ExportModel
void ExportModel( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_filename, bool in_includesubmodels )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_filename;
	args[i++]= in_includesubmodels;

	CStatus st = gApp.ExecuteCommand( L"ExportModel", args, retval );

	return;
}

// ExportPreferences
void ExportPreferences( const CString& in_filename, const CString& in_categories )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_categories;

	CStatus st = gApp.ExecuteCommand( L"ExportPreferences", args, retval );

	return;
}

// ExtractFromComponents
void ExtractFromComponents( const CString& in_presetobj, const CString& in_connectionset, const CString& in_name, bool in_deleteinputcomponents, siOperationMode in_immediatemode, siGeneratorInputsDisposalMode in_genopinputsdisposal, const CRef& /*object*/ in_outputobjs )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_connectionset;
	args[i++]= in_name;
	args[i++]= in_deleteinputcomponents;
	args[i++]= (LONG)in_immediatemode;
	args[i++]= (LONG)in_genopinputsdisposal;
	args[i++]= in_outputobjs;

	CStatus st = gApp.ExecuteCommand( L"ExtractFromComponents", args, retval );

	return;
}

// FilterPSet
CValue FilterPSet( const CValue& /*undefined*/ in_inputobjs, siPSetRelationship in_relationship, bool in_include )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_relationship;
	args[i++]= in_include;

	CStatus st = gApp.ExecuteCommand( L"FilterPSet", args, retval );

	return retval;
}

// FindObjects
CValue FindObjects( const CString& in_path, const CString& in_type )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_path;
	args[i++]= in_type;

	CStatus st = gApp.ExecuteCommand( L"FindObjects", args, retval );

	return retval;
}

// FindPeriods
void FindPeriods( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_algorithm, const CValue& /*undefined*/ in_startframe, const CValue& /*undefined*/ in_endframe, const CValue& /*undefined*/ in_minimum_period_length, const CValue& /*undefined*/ in_mode, const CValue& /*undefined*/ in_name, const CString& in_title )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_algorithm;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_minimum_period_length;
	args[i++]= in_mode;
	args[i++]= in_name;
	args[i++]= in_title;

	CStatus st = gApp.ExecuteCommand( L"FindPeriods", args, retval );

	return;
}

// FirstFrame
void FirstFrame( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"FirstFrame", args, retval );

	return;
}

// FirstKey
CValue FirstKey( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"FirstKey", args, retval );

	return retval;
}

// FirstPaintShapeKey
void FirstPaintShapeKey( const CValue& /*undefined*/ in_shape )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;

	CStatus st = gApp.ExecuteCommand( L"FirstPaintShapeKey", args, retval );

	return;
}

// FitSubcomponentUVsToImage
void FitSubcomponentUVsToImage( const CValue& /*undefined*/ in_properties, const CValue& /*undefined*/ in_subcomp, bool in_explicit, bool in_maintainaspect )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_properties;
	args[i++]= in_subcomp;
	args[i++]= in_explicit;
	args[i++]= in_maintainaspect;

	CStatus st = gApp.ExecuteCommand( L"FitSubcomponentUVsToImage", args, retval );

	return;
}

// Flipbook
void Flipbook( const CString& in_cmdline )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_cmdline;

	CStatus st = gApp.ExecuteCommand( L"Flipbook", args, retval );

	return;
}

// FlipUVW
CValue FlipUVW( const CValue& /*undefined*/ in_textureprojection, const CValue& /*undefined*/ in_subcomp, const CValue& /*number*/ in_pivot )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_textureprojection;
	args[i++]= in_subcomp;
	args[i++]= in_pivot;

	CStatus st = gApp.ExecuteCommand( L"FlipUVW", args, retval );

	return retval;
}

// FloodFillRaster
void FloodFillRaster( const CValue& /*number*/ in_xseed, const CValue& /*number*/ in_yseed )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_xseed;
	args[i++]= in_yseed;

	CStatus st = gApp.ExecuteCommand( L"FloodFillRaster", args, retval );

	return;
}

// FrameAll
void FrameAll( bool in_allobjects, bool in_allviews, const CValue& /*number*/ in_view )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_allobjects;
	args[i++]= in_allviews;
	args[i++]= in_view;

	CStatus st = gApp.ExecuteCommand( L"FrameAll", args, retval );

	return;
}

// FramePreviewAllLayers
void FramePreviewAllLayers( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"FramePreviewAllLayers", args, retval );

	return;
}

// FramePreviewCurrentLayer
void FramePreviewCurrentLayer( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"FramePreviewCurrentLayer", args, retval );

	return;
}

// FramePreviewSelection
void FramePreviewSelection( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"FramePreviewSelection", args, retval );

	return;
}

// FramePreviewSetup
void FramePreviewSetup( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"FramePreviewSetup", args, retval );

	return;
}

// FramePreviewVisibleLayers
void FramePreviewVisibleLayers( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"FramePreviewVisibleLayers", args, retval );

	return;
}

// FrameSelection
void FrameSelection( bool in_allobjects, bool in_allviews, const CValue& /*number*/ in_view )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_allobjects;
	args[i++]= in_allviews;
	args[i++]= in_view;

	CStatus st = gApp.ExecuteCommand( L"FrameSelection", args, retval );

	return;
}

// FreezeAndReplaceClips
CValue FreezeAndReplaceClips( const CValue& /*undefined*/ in_inputobj, const CString& in_name, bool in_ignoreinactivechannels, const CValue& /*number*/ in_stepframe, const CValue& /*number*/ in_fcurve_kind, const CValue& /*number*/ in_defaultsegkind, bool in_fit_fcurve, const CValue& /*number*/ in_fit_tolerance, bool in_processcontrots )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_name;
	args[i++]= in_ignoreinactivechannels;
	args[i++]= in_stepframe;
	args[i++]= in_fcurve_kind;
	args[i++]= in_defaultsegkind;
	args[i++]= in_fit_fcurve;
	args[i++]= in_fit_tolerance;
	args[i++]= in_processcontrots;

	CStatus st = gApp.ExecuteCommand( L"FreezeAndReplaceClips", args, retval );

	return retval;
}

// FreezeClips
CValue FreezeClips( const CValue& /*undefined*/ in_inputobj, const CString& in_name, bool in_ignoreinactivechannels, const CValue& /*number*/ in_stepframe, const CValue& /*number*/ in_fcurve_kind, const CValue& /*number*/ in_defaultsegkind, bool in_fit_fcurve, const CValue& /*number*/ in_fit_tolerance, bool in_processcontrots )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_name;
	args[i++]= in_ignoreinactivechannels;
	args[i++]= in_stepframe;
	args[i++]= in_fcurve_kind;
	args[i++]= in_defaultsegkind;
	args[i++]= in_fit_fcurve;
	args[i++]= in_fit_tolerance;
	args[i++]= in_processcontrots;

	CStatus st = gApp.ExecuteCommand( L"FreezeClips", args, retval );

	return retval;
}

// FreezeModeling
void FreezeModeling( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_time, siBranchFlag in_propagationtype )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;
	args[i++]= (LONG)in_propagationtype;

	CStatus st = gApp.ExecuteCommand( L"FreezeModeling", args, retval );

	return;
}

// FreezeObj
void FreezeObj( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_time, siBranchFlag in_propagationtype )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;
	args[i++]= (LONG)in_propagationtype;

	CStatus st = gApp.ExecuteCommand( L"FreezeObj", args, retval );

	return;
}

// FreezeUVWTransformation
void FreezeUVWTransformation( const CRef& /*object*/ in_textureprojectiondefinition )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_textureprojectiondefinition;

	CStatus st = gApp.ExecuteCommand( L"FreezeUVWTransformation", args, retval );

	return;
}

// GenDiffuseMap
CValue GenDiffuseMap( const CValue& /*undefined*/ in_pset, const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_pset;
	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"GenDiffuseMap", args, retval );

	return retval;
}

// GenerateActor
CValue GenerateActor( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"GenerateActor", args, retval );

	return retval;
}

// GenerateAOMap
CValue GenerateAOMap( const CValue& /*undefined*/ in_pset )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_pset;

	CStatus st = gApp.ExecuteCommand( L"GenerateAOMap", args, retval );

	return retval;
}

// GenerateDepthMap
CValue GenerateDepthMap( const CValue& /*undefined*/ in_pset )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_pset;

	CStatus st = gApp.ExecuteCommand( L"GenerateDepthMap", args, retval );

	return retval;
}

// GenerateInfoReport
void GenerateInfoReport( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_filename )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"GenerateInfoReport", args, retval );

	return;
}

// GenerateMatColorMap
CValue GenerateMatColorMap( const CValue& /*undefined*/ in_pset )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_pset;

	CStatus st = gApp.ExecuteCommand( L"GenerateMatColorMap", args, retval );

	return retval;
}

// GenerateNormalMap
CValue GenerateNormalMap( const CValue& /*undefined*/ in_pset )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_pset;

	CStatus st = gApp.ExecuteCommand( L"GenerateNormalMap", args, retval );

	return retval;
}

// GenerateSurfaceMap
CValue GenerateSurfaceMap( const CValue& /*undefined*/ in_pset )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_pset;

	CStatus st = gApp.ExecuteCommand( L"GenerateSurfaceMap", args, retval );

	return retval;
}

// GenerateUniqueUVs
void GenerateUniqueUVs( const CValue& /*undefined*/ in_inputobjs, const CString& in_propertyname, const CValue& /*number*/ in_lowu, const CValue& /*number*/ in_lowv, const CValue& /*number*/ in_width, const CValue& /*number*/ in_height, const CValue& /*number*/ in_spaceu, const CValue& /*number*/ in_spacev, const CRef& /*object*/ in_properties )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_propertyname;
	args[i++]= in_lowu;
	args[i++]= in_lowv;
	args[i++]= in_width;
	args[i++]= in_height;
	args[i++]= in_spaceu;
	args[i++]= in_spacev;
	args[i++]= in_properties;

	CStatus st = gApp.ExecuteCommand( L"GenerateUniqueUVs", args, retval );

	return;
}

// GenerateUVSetByContourStretching
CValue GenerateUVSetByContourStretching( const CValue& /*undefined*/ in_inputobjs, const CString& in_propertyname, const CValue& /*number*/ in_contourstretchingmethod, bool in_maintainaspect, const CValue& /*undefined*/ in_cornerpoints )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_propertyname;
	args[i++]= in_contourstretchingmethod;
	args[i++]= in_maintainaspect;
	args[i++]= in_cornerpoints;

	CStatus st = gApp.ExecuteCommand( L"GenerateUVSetByContourStretching", args, retval );

	return retval;
}

// GenNormalMap
CValue GenNormalMap( const CValue& /*undefined*/ in_pset, const CValue& /*undefined*/ in_inputobj, bool in_gennormalmapm, bool in_gennormalmapt, bool in_gentangents )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_pset;
	args[i++]= in_inputobj;
	args[i++]= in_gennormalmapm;
	args[i++]= in_gennormalmapt;
	args[i++]= in_gentangents;

	CStatus st = gApp.ExecuteCommand( L"GenNormalMap", args, retval );

	return retval;
}

// GetBBox
void GetBBox( const CValue& /*undefined*/ in_inputobj, bool in_finecomputation, const CValue& /*number*/ in_lowerboundx, const CValue& /*number*/ in_lowerboundy, const CValue& /*number*/ in_lowerboundz, const CValue& /*number*/ in_upperboundx, const CValue& /*number*/ in_upperboundy, const CValue& /*number*/ in_upperboundz )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_finecomputation;
	args[i++]= in_lowerboundx;
	args[i++]= in_lowerboundy;
	args[i++]= in_lowerboundz;
	args[i++]= in_upperboundx;
	args[i++]= in_upperboundy;
	args[i++]= in_upperboundz;

	CStatus st = gApp.ExecuteCommand( L"GetBBox", args, retval );

	return;
}

// GetClosestPaintShapeKey
CValue GetClosestPaintShapeKey( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_time, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_time;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"GetClosestPaintShapeKey", args, retval );

	return retval;
}

// GetConnectedPropVolume
CValue GetConnectedPropVolume( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"GetConnectedPropVolume", args, retval );

	return retval;
}

// GetConnectionSet
CValue GetConnectionSet( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_connectionset, siCnxSetType in_cnxgrouptype, siBranchFlag in_connecttype, bool in_interactive, bool in_createoutputobjs, const CRef& /*object*/ in_outputobjs )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_connectionset;
	args[i++]= (LONG)in_cnxgrouptype;
	args[i++]= (LONG)in_connecttype;
	args[i++]= in_interactive;
	args[i++]= in_createoutputobjs;
	args[i++]= in_outputobjs;

	CStatus st = gApp.ExecuteCommand( L"GetConnectionSet", args, retval );

	return retval;
}

// GetConstrainedObjects
void GetConstrainedObjects( const CValue& /*undefined*/ in_inputobj, const CRef& /*object*/ in_outputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_outputobjs;

	CStatus st = gApp.ExecuteCommand( L"GetConstrainedObjects", args, retval );

	return;
}

// GetCurrentLayer
void GetCurrentLayer( const CRef& /*object*/ in_value )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"GetCurrentLayer", args, retval );

	return;
}

// GetCurrentPass
CValue GetCurrentPass( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"GetCurrentPass", args, retval );

	return retval;
}

// GetCurveLength
void GetCurveLength( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_length )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_length;

	CStatus st = gApp.ExecuteCommand( L"GetCurveLength", args, retval );

	return;
}

// GetCurvePercentageAtKnotIndex
void GetCurvePercentageAtKnotIndex( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_index, const CValue& /*number*/ in_percentage )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_index;
	args[i++]= in_percentage;

	CStatus st = gApp.ExecuteCommand( L"GetCurvePercentageAtKnotIndex", args, retval );

	return;
}

// GetDrivingActions
CValue GetDrivingActions( const CValue& /*undefined*/ in_inputobjs, bool in_consider_time, const CValue& /*undefined*/ in_time, bool in_keep_compound_actions, bool in_keep_inactive_actions )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_consider_time;
	args[i++]= in_time;
	args[i++]= in_keep_compound_actions;
	args[i++]= in_keep_inactive_actions;

	CStatus st = gApp.ExecuteCommand( L"GetDrivingActions", args, retval );

	return retval;
}

// GetFCurveInfo
void GetFCurveInfo( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_curvekind, const CValue& /*number*/ in_valuewhennokey, const CValue& /*number*/ in_nbkeys, const CValue& /*number*/ in_extrapolationkind, const CValue& /*number*/ in_defaultsegkind, const CValue& /*number*/ in_lowclamp, const CValue& /*number*/ in_highclamp )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_curvekind;
	args[i++]= in_valuewhennokey;
	args[i++]= in_nbkeys;
	args[i++]= in_extrapolationkind;
	args[i++]= in_defaultsegkind;
	args[i++]= in_lowclamp;
	args[i++]= in_highclamp;

	CStatus st = gApp.ExecuteCommand( L"GetFCurveInfo", args, retval );

	return;
}

// GetFirstPaintShapeKey
CValue GetFirstPaintShapeKey( const CValue& /*undefined*/ in_shape, const CRef& /*object*/ in_value )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"GetFirstPaintShapeKey", args, retval );

	return retval;
}

// GetGeneratorInputsForDisposal
void GetGeneratorInputsForDisposal( const CValue& /*undefined*/ in_inputobjs, const CString& in_genopinputsdisposal, const CRef& /*object*/ in_outputobjs )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_genopinputsdisposal;
	args[i++]= in_outputobjs;

	CStatus st = gApp.ExecuteCommand( L"GetGeneratorInputsForDisposal", args, retval );

	return;
}

// GetInputAndStoreAction
CValue GetInputAndStoreAction( const CString& in_model, const CString& in_inputobj, const CValue& /*number*/ in_mode, const CString& in_name, bool in_removeanim, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, bool in_consider_time, bool in_ripple, bool in_addclip, const CValue& /*number*/ in_clipoffset )
{
	CValueArray args(11);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_inputobj;
	args[i++]= in_mode;
	args[i++]= in_name;
	args[i++]= in_removeanim;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_consider_time;
	args[i++]= in_ripple;
	args[i++]= in_addclip;
	args[i++]= in_clipoffset;

	CStatus st = gApp.ExecuteCommand( L"GetInputAndStoreAction", args, retval );

	return retval;
}

// GetInstances
CValue GetInstances( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"GetInstances", args, retval );

	return retval;
}

// GetKeyboardState
void GetKeyboardState( const CValue& /*number*/ in_keycode, const CValue& /*number*/ in_shift )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_keycode;
	args[i++]= in_shift;

	CStatus st = gApp.ExecuteCommand( L"GetKeyboardState", args, retval );

	return;
}

// GetKeyInfo
void GetKeyInfo( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_index, const CValue& /*number*/ in_parameter, const CValue& /*number*/ in_leftvalue, const CValue& /*number*/ in_rightvalue, const CValue& /*number*/ in_lefttanparam, const CValue& /*number*/ in_lefttanvalue, const CValue& /*number*/ in_righttanparam, const CValue& /*number*/ in_righttanvalue, const CValue& /*number*/ in_rightsegkind, const CValue& /*number*/ in_constraints )
{
	CValueArray args(11);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_index;
	args[i++]= in_parameter;
	args[i++]= in_leftvalue;
	args[i++]= in_rightvalue;
	args[i++]= in_lefttanparam;
	args[i++]= in_lefttanvalue;
	args[i++]= in_righttanparam;
	args[i++]= in_righttanvalue;
	args[i++]= in_rightsegkind;
	args[i++]= in_constraints;

	CStatus st = gApp.ExecuteCommand( L"GetKeyInfo", args, retval );

	return;
}

// GetLastPaintShapeKey
CValue GetLastPaintShapeKey( const CValue& /*undefined*/ in_shape, const CRef& /*object*/ in_value )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"GetLastPaintShapeKey", args, retval );

	return retval;
}

// GetMappingRule
void GetMappingRule( const CValue& /*undefined*/ in_mappingtemplate, const CValue& /*number*/ in_index, const CString& in_from, const CString& in_to, bool in_active, bool in_poseoffset )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_mappingtemplate;
	args[i++]= in_index;
	args[i++]= in_from;
	args[i++]= in_to;
	args[i++]= in_active;
	args[i++]= in_poseoffset;

	CStatus st = gApp.ExecuteCommand( L"GetMappingRule", args, retval );

	return;
}

// GetMarking
CValue GetMarking( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"GetMarking", args, retval );

	return retval;
}

// GetMaster
CValue GetMaster( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"GetMaster", args, retval );

	return retval;
}

// GetNbTriangles
CValue GetNbTriangles( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"GetNbTriangles", args, retval );

	return retval;
}

// GetNeighborSubcomponents
CValue GetNeighborSubcomponents( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_neighbortype, const CString& in_selfilter )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_neighbortype;
	args[i++]= in_selfilter;

	CStatus st = gApp.ExecuteCommand( L"GetNeighborSubcomponents", args, retval );

	return retval;
}

// GetNextPaintShapeKey
CValue GetNextPaintShapeKey( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_time, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_time;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"GetNextPaintShapeKey", args, retval );

	return retval;
}

// GetNumberOfKnots
void GetNumberOfKnots( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_nbitems )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_nbitems;

	CStatus st = gApp.ExecuteCommand( L"GetNumberOfKnots", args, retval );

	return;
}

// GetNumMappingRules
CValue GetNumMappingRules( const CValue& /*undefined*/ in_mappingtemplate, const CRef& /*object*/ in_value )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_mappingtemplate;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"GetNumMappingRules", args, retval );

	return retval;
}

// GetNumRelativeValues
CValue GetNumRelativeValues( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"GetNumRelativeValues", args, retval );

	return retval;
}

// GetPresetModel
CValue GetPresetModel( const CString& in_modelname, const CString& in_name, const CString& in_parent, const CString& in_psetname )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_modelname;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_psetname;

	CStatus st = gApp.ExecuteCommand( L"GetPresetModel", args, retval );

	return retval;
}

// GetPrevPaintShapeKey
CValue GetPrevPaintShapeKey( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_time, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_time;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"GetPrevPaintShapeKey", args, retval );

	return retval;
}

// GetPrim
CValue GetPrim( const CValue& /*undefined*/ in_presetobj, const CString& in_name, const CString& in_parent, const CRef& /*object*/ in_primitive )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_primitive;

	CStatus st = gApp.ExecuteCommand( L"GetPrim", args, retval );

	return retval;
}

// GetPrimCamera
CValue GetPrimCamera( const CString& in_presetobj, const CString& in_name, const CString& in_parent, const CRef& /*object*/ in_primitive, const CRef& /*object*/ in_3dobjcamera, const CRef& /*object*/ in_3dobjcamerainterest )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_primitive;
	args[i++]= in_3dobjcamera;
	args[i++]= in_3dobjcamerainterest;

	CStatus st = gApp.ExecuteCommand( L"GetPrimCamera", args, retval );

	return retval;
}

// GetPrimLattice
CValue GetPrimLattice( const CString& in_presetobj, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, const CString& in_parent, const CRef& /*object*/ in_primobj )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_primobj;

	CStatus st = gApp.ExecuteCommand( L"GetPrimLattice", args, retval );

	return retval;
}

// GetPrimLight
CValue GetPrimLight( const CString& in_presetobj, const CString& in_name, const CString& in_parent, const CRef& /*object*/ in_primitive, const CRef& /*object*/ in_primobjspot, const CRef& /*object*/ in_primobjspotinterest )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_primitive;
	args[i++]= in_primobjspot;
	args[i++]= in_primobjspotinterest;

	CStatus st = gApp.ExecuteCommand( L"GetPrimLight", args, retval );

	return retval;
}

// GetPrimVolumeDeform
CValue GetPrimVolumeDeform( const CString& in_presetobj, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, const CString& in_parent, const CRef& /*object*/ in_primobj )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_primobj;

	CStatus st = gApp.ExecuteCommand( L"GetPrimVolumeDeform", args, retval );

	return retval;
}

// GetPrimWave
void GetPrimWave( const CString& in_presetobj, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, const CString& in_parent, const CRef& /*object*/ in_primitive )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_primitive;

	CStatus st = gApp.ExecuteCommand( L"GetPrimWave", args, retval );

	return;
}

// GetProportionalGuide
CValue GetProportionalGuide( const CString& in_modelname, const CValue& /*number*/ in_guidestyle, const CValue& /*number*/ in_guidesymmetry )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_modelname;
	args[i++]= in_guidestyle;
	args[i++]= in_guidesymmetry;

	CStatus st = gApp.ExecuteCommand( L"GetProportionalGuide", args, retval );

	return retval;
}

// GetRelativeValue
CValue GetRelativeValue( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_index )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_index;

	CStatus st = gApp.ExecuteCommand( L"GetRelativeValue", args, retval );

	return retval;
}

// GetRigidBodyInitStateFromAnim
void GetRigidBodyInitStateFromAnim( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"GetRigidBodyInitStateFromAnim", args, retval );

	return;
}

// GetSize
CValue GetSize( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_staticsize, const CValue& /*number*/ in_dynamicsize )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_staticsize;
	args[i++]= in_dynamicsize;

	CStatus st = gApp.ExecuteCommand( L"GetSize", args, retval );

	return retval;
}

// GetSkeleton
CValue GetSkeleton( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_index, const CRef& /*object*/ in_chainelements )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_index;
	args[i++]= in_chainelements;

	CStatus st = gApp.ExecuteCommand( L"GetSkeleton", args, retval );

	return retval;
}

// GetSource
CValue GetSource( const CValue& /*undefined*/ in_inputobjs, siSourceType in_sourcemask )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_sourcemask;

	CStatus st = gApp.ExecuteCommand( L"GetSource", args, retval );

	return retval;
}

// GetStoreActionDefaults
void GetStoreActionDefaults( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_mode, const CValue& /*number*/ in_from, const CValue& /*number*/ in_to, const CValue& /*number*/ in_size )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_inputobj;
	args[i++]= in_mode;
	args[i++]= in_from;
	args[i++]= in_to;
	args[i++]= in_size;

	CStatus st = gApp.ExecuteCommand( L"GetStoreActionDefaults", args, retval );

	return;
}

// GetSurfaceFromCurvesConnectionSet
CValue GetSurfaceFromCurvesConnectionSet( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_connectionset, siCnxSetType in_cnxgrouptype, siBranchFlag in_connecttype, bool in_interactive, const CRef& /*object*/ in_outputobjs, bool in_createoutputobjs )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_target;
	args[i++]= in_connectionset;
	args[i++]= (LONG)in_cnxgrouptype;
	args[i++]= (LONG)in_connecttype;
	args[i++]= in_interactive;
	args[i++]= in_outputobjs;
	args[i++]= in_createoutputobjs;

	CStatus st = gApp.ExecuteCommand( L"GetSurfaceFromCurvesConnectionSet", args, retval );

	return retval;
}

// GetUserKeyword
CValue GetUserKeyword( const CValue& /*undefined*/ in_inputobjs, const CRef& /*object*/ in_keywords )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_keywords;

	CStatus st = gApp.ExecuteCommand( L"GetUserKeyword", args, retval );

	return retval;
}

// GetUserPref
CValue GetUserPref( const CString& in_target )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"GetUserPref", args, retval );

	return retval;
}

// GetValue
CValue GetValue( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"GetValue", args, retval );

	return retval;
}

// GotoHome
CValue GotoHome( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"GotoHome", args, retval );

	return retval;
}

// GotoSelectedPaintClipFrame
void GotoSelectedPaintClipFrame( const CValue& /*undefined*/ in_paintclip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_paintclip;

	CStatus st = gApp.ExecuteCommand( L"GotoSelectedPaintClipFrame", args, retval );

	return;
}

// GrowSelection
void GrowSelection( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"GrowSelection", args, retval );

	return;
}

// HairClumpTool
void HairClumpTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"HairClumpTool", args, retval );

	return;
}

// HairPuffRootsTool
void HairPuffRootsTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"HairPuffRootsTool", args, retval );

	return;
}

// HairRotateCursorTool
void HairRotateCursorTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"HairRotateCursorTool", args, retval );

	return;
}

// HairRotateRootTool
void HairRotateRootTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"HairRotateRootTool", args, retval );

	return;
}

// HairRotateTipTool
void HairRotateTipTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"HairRotateTipTool", args, retval );

	return;
}

// HairScalingTool
void HairScalingTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"HairScalingTool", args, retval );

	return;
}

// HairStandOnEndTool
void HairStandOnEndTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"HairStandOnEndTool", args, retval );

	return;
}

// HealUVW
CValue HealUVW( const CValue& /*undefined*/ in_textureprojection, const CValue& /*undefined*/ in_subcomp )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_textureprojection;
	args[i++]= in_subcomp;

	CStatus st = gApp.ExecuteCommand( L"HealUVW", args, retval );

	return retval;
}

// HelpOnHelperBoneTool
void HelpOnHelperBoneTool( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"HelpOnHelperBoneTool", args, retval );

	return;
}

// HideAndUnselect
void HideAndUnselect( const CValue& /*undefined*/ in_target, const CString& in_propertyname )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_propertyname;

	CStatus st = gApp.ExecuteCommand( L"HideAndUnselect", args, retval );

	return;
}

// HideGeneratorInputs
void HideGeneratorInputs( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"HideGeneratorInputs", args, retval );

	return;
}

// HideTextureUVDomains
void HideTextureUVDomains( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_type )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_type;

	CStatus st = gApp.ExecuteCommand( L"HideTextureUVDomains", args, retval );

	return;
}

// HideUnselected
void HideUnselected( const CValue& /*undefined*/ in_target, const CString& in_propertyname )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_propertyname;

	CStatus st = gApp.ExecuteCommand( L"HideUnselected", args, retval );

	return;
}

// hierachy_from_Biped
CValue hierachy_from_Biped( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"hierachy_from_Biped", args, retval );

	return retval;
}

// ImportAction
CValue ImportAction( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_filename, const CString& in_name, const CValue& /*number*/ in_actionstorage )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_actionstorage;

	CStatus st = gApp.ExecuteCommand( L"ImportAction", args, retval );

	return retval;
}

// ImportActionAndAddClip
CValue ImportActionAndAddClip( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_filename )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"ImportActionAndAddClip", args, retval );

	return retval;
}

// ImportActionAndApply
CValue ImportActionAndApply( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_filename )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"ImportActionAndApply", args, retval );

	return retval;
}

// ImportAudio
CValue ImportAudio( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_filename, const CString& in_name )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_filename;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"ImportAudio", args, retval );

	return retval;
}

// ImportAudioAndAddClip
CValue ImportAudioAndAddClip( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_filename )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"ImportAudioAndAddClip", args, retval );

	return retval;
}

// ImportBVHAction
CValue ImportBVHAction( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ImportBVHAction", args, retval );

	return retval;
}

// ImportBVHActor
CValue ImportBVHActor( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ImportBVHActor", args, retval );

	return retval;
}

// ImportDotXSI
void ImportDotXSI( const CString& in_filename, const CString& in_parent )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_parent;

	CStatus st = gApp.ExecuteCommand( L"ImportDotXSI", args, retval );

	return;
}

// ImportEPS
void ImportEPS( const CValue& /*undefined*/ in_filename, const CValue& /*undefined*/ in_parent, bool in_updateselection, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_parent;
	args[i++]= in_updateselection;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"ImportEPS", args, retval );

	return;
}

// ImportFromSI3D
CValue ImportFromSI3D( const CValue& /*undefined*/ in_name, bool in_createpasslayer, const CRef& /*object*/ in_createdobj )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_name;
	args[i++]= in_createpasslayer;
	args[i++]= in_createdobj;

	CStatus st = gApp.ExecuteCommand( L"ImportFromSI3D", args, retval );

	return retval;
}

// ImportGuide
CValue ImportGuide( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ImportGuide", args, retval );

	return retval;
}

// ImportHelperBones
void ImportHelperBones( const CValue& /*undefined*/ in_rig, const CString& in_filename, bool in_prompt )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_rig;
	args[i++]= in_filename;
	args[i++]= in_prompt;

	CStatus st = gApp.ExecuteCommand( L"ImportHelperBones", args, retval );

	return;
}

// ImportIges
void ImportIges( const CString& in_filename, const CString& in_parent, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_parent;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"ImportIges", args, retval );

	return;
}

// ImportMatadorShapeList
void ImportMatadorShapeList( const CValue& /*undefined*/ in_shapelist, const CValue& /*undefined*/ in_filename, const CValue& /*number*/ in_destwidth, const CValue& /*number*/ in_destheight )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_shapelist;
	args[i++]= in_filename;
	args[i++]= in_destwidth;
	args[i++]= in_destheight;

	CStatus st = gApp.ExecuteCommand( L"ImportMatadorShapeList", args, retval );

	return;
}

// ImportMixer
void ImportMixer( const CValue& /*undefined*/ in_filename, const CValue& /*undefined*/ in_parent, bool in_merge, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_parent;
	args[i++]= in_merge;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"ImportMixer", args, retval );

	return;
}

// ImportModel
void ImportModel( const CString& in_filename, const CString& in_parent, bool in_reference, const CRef& /*object*/ in_value, const CString& in_name, const CValue& /*number*/ in_shareoptions, const CString& in_externalcnxmappingtemplate )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_parent;
	args[i++]= in_reference;
	args[i++]= in_value;
	args[i++]= in_name;
	args[i++]= in_shareoptions;
	args[i++]= in_externalcnxmappingtemplate;

	CStatus st = gApp.ExecuteCommand( L"ImportModel", args, retval );

	return;
}

// ImportPreferences
void ImportPreferences( const CString& in_filename )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"ImportPreferences", args, retval );

	return;
}

// ImportPrefs
void ImportPrefs( const CValue& /*undefined*/ in_filename )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"ImportPrefs", args, retval );

	return;
}

// ImportRefAction
CValue ImportRefAction( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_filename, const CString& in_name, const CValue& /*number*/ in_actionstorage )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_actionstorage;

	CStatus st = gApp.ExecuteCommand( L"ImportRefAction", args, retval );

	return retval;
}

// ImportRefModels
CValue ImportRefModels( const CString& in_filename, const CString& in_parent, const CString& in_externalcnxmappingtemplate )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_parent;
	args[i++]= in_externalcnxmappingtemplate;

	CStatus st = gApp.ExecuteCommand( L"ImportRefModels", args, retval );

	return retval;
}

// ImportWeightMap
CValue ImportWeightMap( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ImportWeightMap", args, retval );

	return retval;
}

// IncPaintShapePointWeight
void IncPaintShapePointWeight( const CValue& /*undefined*/ in_shape )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;

	CStatus st = gApp.ExecuteCommand( L"IncPaintShapePointWeight", args, retval );

	return;
}

// InfoGeometry
void InfoGeometry( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"InfoGeometry", args, retval );

	return;
}

// InfoScene
void InfoScene( bool in_dogeneral, bool in_doanimation, bool in_dorendering )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_dogeneral;
	args[i++]= in_doanimation;
	args[i++]= in_dorendering;

	CStatus st = gApp.ExecuteCommand( L"InfoScene", args, retval );

	return;
}

// InfoSelection
void InfoSelection( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"InfoSelection", args, retval );

	return;
}

// InitActor
CValue InitActor( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"InitActor", args, retval );

	return retval;
}

// InitPropVolume
void InitPropVolume( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"InitPropVolume", args, retval );

	return;
}

// InitShapeKey
CValue InitShapeKey( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"InitShapeKey", args, retval );

	return retval;
}

// Insert2DShapePoint
CValue Insert2DShapePoint( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_position, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_value )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_position;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"Insert2DShapePoint", args, retval );

	return retval;
}

// InsertPaintShapePoint
CValue InsertPaintShapePoint( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_insertposition, const CValue& /*number*/ in_contour, const CValue& /*number*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_insertposition;
	args[i++]= in_contour;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"InsertPaintShapePoint", args, retval );

	return retval;
}

// InsertShader
CValue InsertShader( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_inputobjs, const CString& in_name )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobjs;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"InsertShader", args, retval );

	return retval;
}

// InspectBrowse
void InspectBrowse( siInspectBrowseMode in_mode, const CValue& /*number*/ in_length )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= (LONG)in_mode;
	args[i++]= in_length;

	CStatus st = gApp.ExecuteCommand( L"InspectBrowse", args, retval );

	return;
}

// InspectChainProperties
void InspectChainProperties( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"InspectChainProperties", args, retval );

	return;
}

// InspectDevices
void InspectDevices( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"InspectDevices", args, retval );

	return;
}

// InspectEnvelopeWeights
void InspectEnvelopeWeights( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"InspectEnvelopeWeights", args, retval );

	return;
}

// InspectInitStateProp
CValue InspectInitStateProp( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"InspectInitStateProp", args, retval );

	return retval;
}

// InspectLastOperator
void InspectLastOperator( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"InspectLastOperator", args, retval );

	return;
}

// InspectLocks
void InspectLocks( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"InspectLocks", args, retval );

	return;
}

// InspectMaterial
void InspectMaterial( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"InspectMaterial", args, retval );

	return;
}

// InspectObj
CValue InspectObj( const CString& in_inputobjs, const CString& in_keywords, const CString& in_title, const CValue& /*number*/ in_mode, bool in_throw )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_keywords;
	args[i++]= in_title;
	args[i++]= in_mode;
	args[i++]= in_throw;

	CStatus st = gApp.ExecuteCommand( L"InspectObj", args, retval );

	return retval;
}

// InspectPropVolume
void InspectPropVolume( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"InspectPropVolume", args, retval );

	return;
}

// InspectSnapProperties
void InspectSnapProperties( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"InspectSnapProperties", args, retval );

	return;
}

// InspectSpringPropertiesProc
void InspectSpringPropertiesProc( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"InspectSpringPropertiesProc", args, retval );

	return;
}

// InspectTransformProperties
void InspectTransformProperties( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"InspectTransformProperties", args, retval );

	return;
}

// InspectUserData
void InspectUserData( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"InspectUserData", args, retval );

	return;
}

// InstallAddon
void InstallAddon( const CString& in_filename, const CValue& /*number*/ in_installpath )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_installpath;

	CStatus st = gApp.ExecuteCommand( L"InstallAddon", args, retval );

	return;
}

// InstallCustomPreferences
void InstallCustomPreferences( const CValue& /*undefined*/ in_inputobjs, const CString& in_name )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"InstallCustomPreferences", args, retval );

	return;
}

// Instance
CValue Instance( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_nbitems, const CValue& /*number*/ in_hierarchy, const CValue& /*number*/ in_grouping, const CValue& /*number*/ in_properties, const CValue& /*number*/ in_animation, const CValue& /*number*/ in_constraints, const CValue& /*number*/ in_selection, const CValue& /*number*/ in_xfrom, const CString& in_sx, const CString& in_sy, const CString& in_sz, const CString& in_rx, const CString& in_ry, const CString& in_rz, const CString& in_tx, const CString& in_ty, const CString& in_tz, const CString& in_trackxform )
{
	CValueArray args(19);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_nbitems;
	args[i++]= in_hierarchy;
	args[i++]= in_grouping;
	args[i++]= in_properties;
	args[i++]= in_animation;
	args[i++]= in_constraints;
	args[i++]= in_selection;
	args[i++]= in_xfrom;
	args[i++]= in_sx;
	args[i++]= in_sy;
	args[i++]= in_sz;
	args[i++]= in_rx;
	args[i++]= in_ry;
	args[i++]= in_rz;
	args[i++]= in_tx;
	args[i++]= in_ty;
	args[i++]= in_tz;
	args[i++]= in_trackxform;

	CStatus st = gApp.ExecuteCommand( L"Instance", args, retval );

	return retval;
}

// Instantiate
CValue Instantiate( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_nbitems, siDupParent in_hierarchy, siDupGroup in_grouping, siDupSelection in_selection, siDupTransform in_xfrom, const CValue& in_sx, const CValue& in_sy, const CValue& in_sz, const CValue& in_rx, const CValue& in_ry, const CValue& in_rz, const CValue& in_tx, const CValue& in_ty, const CValue& in_tz, const CValue& in_trackxform)
{
	CValueArray args(16);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_nbitems;
	args[i++]= (LONG)in_hierarchy;
	args[i++]= (LONG)in_grouping;
	args[i++]= (LONG)in_selection;
	args[i++]= (LONG)in_xfrom;
	args[i++]= in_sx;
	args[i++]= in_sy;
	args[i++]= in_sz;
	args[i++]= in_rx;
	args[i++]= in_ry;
	args[i++]= in_rz;
	args[i++]= in_tx;
	args[i++]= in_ty;
	args[i++]= in_tz;
	args[i++]= in_trackxform;

	CStatus st = gApp.ExecuteCommand( L"Instantiate", args, retval );

	return retval;
}

// InvertPaintStencil
void InvertPaintStencil( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"InvertPaintStencil", args, retval );

	return;
}

// InvertSelection
void InvertSelection( const CValue& /*undefined*/ in_selfilter, siComponentVisibilityMode in_checkcomponentvisibility, bool in_checkobjectselectability )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_selfilter;
	args[i++]= (LONG)in_checkcomponentvisibility;
	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"InvertSelection", args, retval );

	return;
}

// IsAnimated
CValue IsAnimated( const CValue& /*undefined*/ in_inputobj, siSourceType in_sourcemask, siBranchFlag in_branchflag, const CRef& /*object*/ in_animcomponents, bool in_local )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= (LONG)in_sourcemask;
	args[i++]= (LONG)in_branchflag;
	args[i++]= in_animcomponents;
	args[i++]= in_local;

	CStatus st = gApp.ExecuteCommand( L"IsAnimated", args, retval );

	return retval;
}

// IslandHealUVW
CValue IslandHealUVW( const CValue& /*undefined*/ in_textureprojection, const CValue& /*undefined*/ in_targetboundary, const CValue& /*undefined*/ in_sourceislands, bool in_allowscaling, const CValue& /*number*/ in_interpolation )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_textureprojection;
	args[i++]= in_targetboundary;
	args[i++]= in_sourceislands;
	args[i++]= in_allowscaling;
	args[i++]= in_interpolation;

	CStatus st = gApp.ExecuteCommand( L"IslandHealUVW", args, retval );

	return retval;
}

// IsolateAll
void IsolateAll( bool in_all, const CValue& /*number*/ in_view )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_all;
	args[i++]= in_view;

	CStatus st = gApp.ExecuteCommand( L"IsolateAll", args, retval );

	return;
}

// IsolateSelected
void IsolateSelected( bool in_all, const CValue& /*number*/ in_view )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_all;
	args[i++]= in_view;

	CStatus st = gApp.ExecuteCommand( L"IsolateSelected", args, retval );

	return;
}

// Key2DShapePoints
void Key2DShapePoints( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_index )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_index;

	CStatus st = gApp.ExecuteCommand( L"Key2DShapePoints", args, retval );

	return;
}

// KeyAllBoneRotations
void KeyAllBoneRotations( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"KeyAllBoneRotations", args, retval );

	return;
}

// LastFrame
void LastFrame( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"LastFrame", args, retval );

	return;
}

// LastKey
CValue LastKey( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"LastKey", args, retval );

	return retval;
}

// LastPaintShapeKey
void LastPaintShapeKey( const CValue& /*undefined*/ in_shape )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;

	CStatus st = gApp.ExecuteCommand( L"LastPaintShapeKey", args, retval );

	return;
}

// LaunchFlipbookForPass
void LaunchFlipbookForPass( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"LaunchFlipbookForPass", args, retval );

	return;
}

// LightAssociate
void LightAssociate( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"LightAssociate", args, retval );

	return;
}

// LinkDeform
void LinkDeform( const CValue& /*undefined*/ in_cluster, const CValue& /*undefined*/ in_parameter_list, bool in_linkwithori )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_cluster;
	args[i++]= in_parameter_list;
	args[i++]= in_linkwithori;

	CStatus st = gApp.ExecuteCommand( L"LinkDeform", args, retval );

	return;
}

// LinkDeformWithOri
void LinkDeformWithOri( const CValue& /*undefined*/ in_cluster, const CValue& /*undefined*/ in_parameter_list, bool in_linkwithori )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_cluster;
	args[i++]= in_parameter_list;
	args[i++]= in_linkwithori;

	CStatus st = gApp.ExecuteCommand( L"LinkDeformWithOri", args, retval );

	return;
}

// LoadActionPreset
CValue LoadActionPreset( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_filename, const CString& in_name, const CValue& /*number*/ in_actionstorage )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_actionstorage;

	CStatus st = gApp.ExecuteCommand( L"LoadActionPreset", args, retval );

	return retval;
}

// LoadActor
CValue LoadActor( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"LoadActor", args, retval );

	return retval;
}

// LoadDefaultRig
void LoadDefaultRig( const CString& in_whichrig )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_whichrig;

	CStatus st = gApp.ExecuteCommand( L"LoadDefaultRig", args, retval );

	return;
}

// LoadPreset
void LoadPreset( const CString& in_presetobj, const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"LoadPreset", args, retval );

	return;
}

// LocalAssignFlexEnv
void LocalAssignFlexEnv( const CValue& /*undefined*/ in_subcomp, const CString& in_connectionset, const CValue& /*number*/ in_nbdeformersperpoint )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_subcomp;
	args[i++]= in_connectionset;
	args[i++]= in_nbdeformersperpoint;

	CStatus st = gApp.ExecuteCommand( L"LocalAssignFlexEnv", args, retval );

	return;
}

// Lock
void Lock( const CValue& /*undefined*/ in_inputobjs, siLockLevel in_level )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_level;

	CStatus st = gApp.ExecuteCommand( L"Lock", args, retval );

	return;
}

// MagicWandPaintMask
CValue MagicWandPaintMask( const CValue& /*number*/ in_seedx, const CValue& /*number*/ in_seedy, const CValue& /*number*/ in_selectionaction, bool in_antialiased, const CValue& /*number*/ in_feather, const CRef& /*object*/ in_value )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_seedx;
	args[i++]= in_seedy;
	args[i++]= in_selectionaction;
	args[i++]= in_antialiased;
	args[i++]= in_feather;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"MagicWandPaintMask", args, retval );

	return retval;
}

// Make2PointSlide
CValue Make2PointSlide( const CString& in_prefix, const CString& in_guidepoint1, const CString& in_guidepoint2, const CString& in_parent1, const CString& in_parent2, const CString& in_upvector, const CString& in_boundingvolume )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_prefix;
	args[i++]= in_guidepoint1;
	args[i++]= in_guidepoint2;
	args[i++]= in_parent1;
	args[i++]= in_parent2;
	args[i++]= in_upvector;
	args[i++]= in_boundingvolume;

	CStatus st = gApp.ExecuteCommand( L"Make2PointSlide", args, retval );

	return retval;
}

// MakeAllUnselectable
void MakeAllUnselectable( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"MakeAllUnselectable", args, retval );

	return;
}

// MakeArm
CValue MakeArm( const CString& in_armparent, const CString& in_guideobjectcollection, const CString& in_prefix, const CString& in_armrootconstrainingobj, const CValue& /*number*/ in_nbforearmdivisions, const CValue& /*number*/ in_nbbicepdivisions, const CValue& /*number*/ in_shadowtype, const CString& in_shadowparent, const CString& in_handroot, bool in_negativescale )
{
	CValueArray args(10);
	CValue retval;
	LONG i(0);

	args[i++]= in_armparent;
	args[i++]= in_guideobjectcollection;
	args[i++]= in_prefix;
	args[i++]= in_armrootconstrainingobj;
	args[i++]= in_nbforearmdivisions;
	args[i++]= in_nbbicepdivisions;
	args[i++]= in_shadowtype;
	args[i++]= in_shadowparent;
	args[i++]= in_handroot;
	args[i++]= in_negativescale;

	CStatus st = gApp.ExecuteCommand( L"MakeArm", args, retval );

	return retval;
}

// MakeBelly
CValue MakeBelly( const CString& in_hipobject, const CString& in_vertebraobject, const CString& in_guideobjectcollection, const CValue& /*number*/ in_forwardrotationperc )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_hipobject;
	args[i++]= in_vertebraobject;
	args[i++]= in_guideobjectcollection;
	args[i++]= in_forwardrotationperc;

	CStatus st = gApp.ExecuteCommand( L"MakeBelly", args, retval );

	return retval;
}

// MakeBicepRoll
CValue MakeBicepRoll( const CString& in_bicep, const CString& in_prefix, const CValue& /*number*/ in_nbdivisions )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_bicep;
	args[i++]= in_prefix;
	args[i++]= in_nbdivisions;

	CStatus st = gApp.ExecuteCommand( L"MakeBicepRoll", args, retval );

	return retval;
}

// MakeBiped
CValue MakeBiped( const CValue& /*number*/ in_torsotype, const CValue& /*number*/ in_torsostretch, const CValue& /*number*/ in_torsodivisions, const CValue& /*number*/ in_icontype, bool in_makebelly, const CValue& /*number*/ in_bellyslide, const CValue& /*number*/ in_bellycenterpercentage, const CValue& /*number*/ in_headtype, const CValue& /*number*/ in_headstretch, const CValue& /*number*/ in_headdivisions, bool in_ears, const CValue& /*number*/ in_rotationorder, const CValue& /*number*/ in_armsymmetry, const CValue& /*number*/ in_armattachment, const CValue& /*number*/ in_fingertype, bool in_forearmroll, const CValue& /*number*/ in_forearmdivisions, bool in_biceproll, const CValue& /*number*/ in_bicepdivisions, bool in_thighroll, const CValue& /*number*/ in_thighdivisions, bool in_armpitslide, bool in_hipslide, bool in_thighslide, bool in_elbowsjoint, bool in_kneejoint, const CValue& /*number*/ in_shadowtype, const CValue& /*number*/ in_shadowhands, bool in_selectusingui )
{
	CValueArray args(29);
	CValue retval;
	LONG i(0);

	args[i++]= in_torsotype;
	args[i++]= in_torsostretch;
	args[i++]= in_torsodivisions;
	args[i++]= in_icontype;
	args[i++]= in_makebelly;
	args[i++]= in_bellyslide;
	args[i++]= in_bellycenterpercentage;
	args[i++]= in_headtype;
	args[i++]= in_headstretch;
	args[i++]= in_headdivisions;
	args[i++]= in_ears;
	args[i++]= in_rotationorder;
	args[i++]= in_armsymmetry;
	args[i++]= in_armattachment;
	args[i++]= in_fingertype;
	args[i++]= in_forearmroll;
	args[i++]= in_forearmdivisions;
	args[i++]= in_biceproll;
	args[i++]= in_bicepdivisions;
	args[i++]= in_thighroll;
	args[i++]= in_thighdivisions;
	args[i++]= in_armpitslide;
	args[i++]= in_hipslide;
	args[i++]= in_thighslide;
	args[i++]= in_elbowsjoint;
	args[i++]= in_kneejoint;
	args[i++]= in_shadowtype;
	args[i++]= in_shadowhands;
	args[i++]= in_selectusingui;

	CStatus st = gApp.ExecuteCommand( L"MakeBiped", args, retval );

	return retval;
}

// MakeBipedDogLeg
CValue MakeBipedDogLeg( const CValue& /*number*/ in_torsotype, const CValue& /*number*/ in_torsostretch, const CValue& /*number*/ in_torsodivisions, const CValue& /*number*/ in_icontype, bool in_makebelly, const CValue& /*number*/ in_bellyslide, const CValue& /*number*/ in_bellycenterpercentage, const CValue& /*number*/ in_headtype, const CValue& /*number*/ in_headstretch, const CValue& /*number*/ in_headdivisions, bool in_ears, const CValue& /*number*/ in_rotationorder, const CValue& /*number*/ in_armsymmetry, const CValue& /*number*/ in_armattachment, const CValue& /*number*/ in_fingertype, bool in_forearmroll, const CValue& /*number*/ in_forearmdivisions, bool in_biceproll, const CValue& /*number*/ in_bicepdivisions, bool in_thighroll, const CValue& /*number*/ in_thighdivisions, bool in_armpitslide, bool in_hipslide, bool in_thighslide, bool in_elbowsjoint, const CValue& /*number*/ in_shadowtype, const CValue& /*number*/ in_shadowhands, bool in_selectusingui )
{
	CValueArray args(28);
	CValue retval;
	LONG i(0);

	args[i++]= in_torsotype;
	args[i++]= in_torsostretch;
	args[i++]= in_torsodivisions;
	args[i++]= in_icontype;
	args[i++]= in_makebelly;
	args[i++]= in_bellyslide;
	args[i++]= in_bellycenterpercentage;
	args[i++]= in_headtype;
	args[i++]= in_headstretch;
	args[i++]= in_headdivisions;
	args[i++]= in_ears;
	args[i++]= in_rotationorder;
	args[i++]= in_armsymmetry;
	args[i++]= in_armattachment;
	args[i++]= in_fingertype;
	args[i++]= in_forearmroll;
	args[i++]= in_forearmdivisions;
	args[i++]= in_biceproll;
	args[i++]= in_bicepdivisions;
	args[i++]= in_thighroll;
	args[i++]= in_thighdivisions;
	args[i++]= in_armpitslide;
	args[i++]= in_hipslide;
	args[i++]= in_thighslide;
	args[i++]= in_elbowsjoint;
	args[i++]= in_shadowtype;
	args[i++]= in_shadowhands;
	args[i++]= in_selectusingui;

	CStatus st = gApp.ExecuteCommand( L"MakeBipedDogLeg", args, retval );

	return retval;
}

// MakeControlSplines
CValue MakeControlSplines( const CValue& /*undefined*/ in_curvecollection, const CValue& /*number*/ in_nbdivisions, const CValue& /*number*/ in_scale, const CString& in_parent, const CValue& /*number*/ in_pointmarkertype, const CValue& /*number*/ in_pathmarkertype, const CValue& /*number*/ in_offsetmarkertype, const CValue& /*number*/ in_pointcolor, const CValue& /*number*/ in_pathcolor, const CValue& /*number*/ in_offsetcolor, bool in_inputfromui )
{
	CValueArray args(11);
	CValue retval;
	LONG i(0);

	args[i++]= in_curvecollection;
	args[i++]= in_nbdivisions;
	args[i++]= in_scale;
	args[i++]= in_parent;
	args[i++]= in_pointmarkertype;
	args[i++]= in_pathmarkertype;
	args[i++]= in_offsetmarkertype;
	args[i++]= in_pointcolor;
	args[i++]= in_pathcolor;
	args[i++]= in_offsetcolor;
	args[i++]= in_inputfromui;

	CStatus st = gApp.ExecuteCommand( L"MakeControlSplines", args, retval );

	return retval;
}

// MakeCorridor
CValue MakeCorridor( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"MakeCorridor", args, retval );

	return retval;
}

// MakeDogLeg
CValue MakeDogLeg( const CString& in_model, const CString& in_parent, const CString& in_footparent, const CString& in_guideobjectcollection, const CString& in_shortprefix, const CString& in_fullprefix, const CString& in_sliders, const CValue& /*number*/ in_nbthighdivisions, const CValue& /*number*/ in_shadowtype, const CString& in_shadowparent )
{
	CValueArray args(10);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_parent;
	args[i++]= in_footparent;
	args[i++]= in_guideobjectcollection;
	args[i++]= in_shortprefix;
	args[i++]= in_fullprefix;
	args[i++]= in_sliders;
	args[i++]= in_nbthighdivisions;
	args[i++]= in_shadowtype;
	args[i++]= in_shadowparent;

	CStatus st = gApp.ExecuteCommand( L"MakeDogLeg", args, retval );

	return retval;
}

// MakeDoor
CValue MakeDoor( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"MakeDoor", args, retval );

	return retval;
}

// MakeFoot
CValue MakeFoot( const CString& in_model, const CString& in_parent, const CString& in_shortprefix, const CString& in_fullprefix, const CString& in_guideobjectcollection, const CString& in_sliders, bool in_extension, const CValue& /*number*/ in_shadowtype, const CString& in_shadowparent )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_parent;
	args[i++]= in_shortprefix;
	args[i++]= in_fullprefix;
	args[i++]= in_guideobjectcollection;
	args[i++]= in_sliders;
	args[i++]= in_extension;
	args[i++]= in_shadowtype;
	args[i++]= in_shadowparent;

	CStatus st = gApp.ExecuteCommand( L"MakeFoot", args, retval );

	return retval;
}

// MakeForearmRoll
CValue MakeForearmRoll( const CString& in_bicep, const CString& in_forearm, const CString& in_armeff, const CString& in_handeff, const CString& in_handbone, const CString& in_prefix, const CValue& /*number*/ in_nbdivisions, const CValue& /*number*/ in_rolloffset )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_bicep;
	args[i++]= in_forearm;
	args[i++]= in_armeff;
	args[i++]= in_handeff;
	args[i++]= in_handbone;
	args[i++]= in_prefix;
	args[i++]= in_nbdivisions;
	args[i++]= in_rolloffset;

	CStatus st = gApp.ExecuteCommand( L"MakeForearmRoll", args, retval );

	return retval;
}

// MakeHand
CValue MakeHand( const CString& in_handparent, const CString& in_fingerguidecollection, const CString& in_bladeguidecollection, const CString& in_handguidecollection, const CString& in_prefix, const CValue& /*number*/ in_fingerbonetype, const CValue& /*number*/ in_shadowtype, const CString& in_shadowparent, bool in_negativescale )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_handparent;
	args[i++]= in_fingerguidecollection;
	args[i++]= in_bladeguidecollection;
	args[i++]= in_handguidecollection;
	args[i++]= in_prefix;
	args[i++]= in_fingerbonetype;
	args[i++]= in_shadowtype;
	args[i++]= in_shadowparent;
	args[i++]= in_negativescale;

	CStatus st = gApp.ExecuteCommand( L"MakeHand", args, retval );

	return retval;
}

// MakeHead
CValue MakeHead( const CString& in_parent, const CString& in_guideobjectcollection, const CString& in_prefix, const CValue& /*number*/ in_assemblytype, const CValue& /*number*/ in_headtype, const CValue& /*number*/ in_nbdivisions, const CValue& /*number*/ in_stretchtype, const CString& in_sliders, const CString& in_earguidecollection, const CValue& /*number*/ in_shadowtype, const CString& in_shadowparent )
{
	CValueArray args(11);
	CValue retval;
	LONG i(0);

	args[i++]= in_parent;
	args[i++]= in_guideobjectcollection;
	args[i++]= in_prefix;
	args[i++]= in_assemblytype;
	args[i++]= in_headtype;
	args[i++]= in_nbdivisions;
	args[i++]= in_stretchtype;
	args[i++]= in_sliders;
	args[i++]= in_earguidecollection;
	args[i++]= in_shadowtype;
	args[i++]= in_shadowparent;

	CStatus st = gApp.ExecuteCommand( L"MakeHead", args, retval );

	return retval;
}

// MakeHip
CValue MakeHip( const CString& in_guideobj, const CValue& /*number*/ in_topx, const CValue& /*number*/ in_topy, const CValue& /*number*/ in_topz, const CValue& /*number*/ in_sidex, const CValue& /*number*/ in_sidey, const CValue& /*number*/ in_sidez )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_guideobj;
	args[i++]= in_topx;
	args[i++]= in_topy;
	args[i++]= in_topz;
	args[i++]= in_sidex;
	args[i++]= in_sidey;
	args[i++]= in_sidez;

	CStatus st = gApp.ExecuteCommand( L"MakeHip", args, retval );

	return retval;
}

// MakeJointCompression
CValue MakeJointCompression( const CString& in_prefix, const CString& in_upperbone, const CString& in_lowerbone, const CString& in_guideobjectcollection, const CString& in_rolldivisioncollection )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_prefix;
	args[i++]= in_upperbone;
	args[i++]= in_lowerbone;
	args[i++]= in_guideobjectcollection;
	args[i++]= in_rolldivisioncollection;

	CStatus st = gApp.ExecuteCommand( L"MakeJointCompression", args, retval );

	return retval;
}

// MakeLeg
CValue MakeLeg( const CString& in_model, const CString& in_parent, const CString& in_prefix, const CString& in_guideobjectcollection, bool in_upvectorbehindleg, const CValue& /*number*/ in_nbthighdivisions, const CValue& /*number*/ in_shadowtype, const CString& in_shadowparent )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_parent;
	args[i++]= in_prefix;
	args[i++]= in_guideobjectcollection;
	args[i++]= in_upvectorbehindleg;
	args[i++]= in_nbthighdivisions;
	args[i++]= in_shadowtype;
	args[i++]= in_shadowparent;

	CStatus st = gApp.ExecuteCommand( L"MakeLeg", args, retval );

	return retval;
}

// MakeLocal
void MakeLocal( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_propagationtype )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_propagationtype;

	CStatus st = gApp.ExecuteCommand( L"MakeLocal", args, retval );

	return;
}

// MakeLocalMaterialsForHierarchy
void MakeLocalMaterialsForHierarchy( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"MakeLocalMaterialsForHierarchy", args, retval );

	return;
}

// MakeLoop
CValue MakeLoop( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"MakeLoop", args, retval );

	return retval;
}

// MakeModelLocal
void MakeModelLocal( const CValue& /*undefined*/ in_clip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"MakeModelLocal", args, retval );

	return;
}

// MakeQuadruped
CValue MakeQuadruped( const CValue& /*number*/ in_torsotype, const CValue& /*number*/ in_torsostretch, const CValue& /*number*/ in_torsodivisions, const CValue& /*number*/ in_icontype, bool in_makebelly, const CValue& /*number*/ in_bellyslide, const CValue& /*number*/ in_bellycenterpercentage, const CValue& /*number*/ in_headtype, const CValue& /*number*/ in_headstretch, const CValue& /*number*/ in_headdivisions, bool in_ears, const CValue& /*number*/ in_maketail, const CValue& /*number*/ in_taildivisions, bool in_frontroll, const CValue& /*number*/ in_frontrolldivisions, bool in_backroll, const CValue& /*number*/ in_backrolldivisions, const CValue& /*number*/ in_shadowtype, bool in_selectusingui )
{
	CValueArray args(19);
	CValue retval;
	LONG i(0);

	args[i++]= in_torsotype;
	args[i++]= in_torsostretch;
	args[i++]= in_torsodivisions;
	args[i++]= in_icontype;
	args[i++]= in_makebelly;
	args[i++]= in_bellyslide;
	args[i++]= in_bellycenterpercentage;
	args[i++]= in_headtype;
	args[i++]= in_headstretch;
	args[i++]= in_headdivisions;
	args[i++]= in_ears;
	args[i++]= in_maketail;
	args[i++]= in_taildivisions;
	args[i++]= in_frontroll;
	args[i++]= in_frontrolldivisions;
	args[i++]= in_backroll;
	args[i++]= in_backrolldivisions;
	args[i++]= in_shadowtype;
	args[i++]= in_selectusingui;

	CStatus st = gApp.ExecuteCommand( L"MakeQuadruped", args, retval );

	return retval;
}

// MakeRigIcon
CValue MakeRigIcon( const CString& in_parent, const CValue& /*number*/ in_type, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z, const CValue& /*number*/ in_xlength, const CValue& /*number*/ in_ylength, const CValue& /*number*/ in_zlength, const CValue& /*number*/ in_color, const CString& in_name, const CValue& /*number*/ in_xoffset, const CValue& /*number*/ in_yoffset, const CValue& /*number*/ in_zoffset )
{
	CValueArray args(13);
	CValue retval;
	LONG i(0);

	args[i++]= in_parent;
	args[i++]= in_type;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= in_xlength;
	args[i++]= in_ylength;
	args[i++]= in_zlength;
	args[i++]= in_color;
	args[i++]= in_name;
	args[i++]= in_xoffset;
	args[i++]= in_yoffset;
	args[i++]= in_zoffset;

	CStatus st = gApp.ExecuteCommand( L"MakeRigIcon", args, retval );

	return retval;
}

// MakeRoom
CValue MakeRoom( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"MakeRoom", args, retval );

	return retval;
}

// MakeRotationsContinuous
void MakeRotationsContinuous( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"MakeRotationsContinuous", args, retval );

	return;
}

// MakeShadowChain
CValue MakeShadowChain( const CString& in_chain, const CString& in_shadowparent, bool in_si3dstylechain, const CValue& /*number*/ in_iconscale, const CValue& /*number*/ in_shadowtype )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_chain;
	args[i++]= in_shadowparent;
	args[i++]= in_si3dstylechain;
	args[i++]= in_iconscale;
	args[i++]= in_shadowtype;

	CStatus st = gApp.ExecuteCommand( L"MakeShadowChain", args, retval );

	return retval;
}

// MakeShadowSpine
CValue MakeShadowSpine( const CString& in_spline, const CString& in_base, const CString& in_vertebra, const CString& in_topvertebra, const CString& in_shadowparent, const CString& in_chesttop, const CValue& /*number*/ in_iconscale, const CValue& /*number*/ in_shadowtype )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_spline;
	args[i++]= in_base;
	args[i++]= in_vertebra;
	args[i++]= in_topvertebra;
	args[i++]= in_shadowparent;
	args[i++]= in_chesttop;
	args[i++]= in_iconscale;
	args[i++]= in_shadowtype;

	CStatus st = gApp.ExecuteCommand( L"MakeShadowSpine", args, retval );

	return retval;
}

// MakeSkill
CValue MakeSkill( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"MakeSkill", args, retval );

	return retval;
}

// MakeSpine
CValue MakeSpine( const CString& in_model, const CString& in_parent, const CValue& /*number*/ in_nbvertebrae, const CValue& /*number*/ in_spinetype, bool in_stretchy, const CString& in_spinebaseobject, const CString& in_spinetopobject, const CString& in_bottomdepthobject, const CString& in_topdepthobject, const CString& in_chesttop, const CString& in_sliders, const CValue& /*number*/ in_shadowtype, const CString& in_shadowparent )
{
	CValueArray args(13);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_parent;
	args[i++]= in_nbvertebrae;
	args[i++]= in_spinetype;
	args[i++]= in_stretchy;
	args[i++]= in_spinebaseobject;
	args[i++]= in_spinetopobject;
	args[i++]= in_bottomdepthobject;
	args[i++]= in_topdepthobject;
	args[i++]= in_chesttop;
	args[i++]= in_sliders;
	args[i++]= in_shadowtype;
	args[i++]= in_shadowparent;

	CStatus st = gApp.ExecuteCommand( L"MakeSpine", args, retval );

	return retval;
}

// MakeTail
CValue MakeTail( const CString& in_parent, const CValue& /*undefined*/ in_guideobjectcollection, const CValue& /*number*/ in_nbtailbones, const CValue& /*number*/ in_shadowtype, const CString& in_shadowparent )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_parent;
	args[i++]= in_guideobjectcollection;
	args[i++]= in_nbtailbones;
	args[i++]= in_shadowtype;
	args[i++]= in_shadowparent;

	CStatus st = gApp.ExecuteCommand( L"MakeTail", args, retval );

	return retval;
}

// MakeThighSlide
CValue MakeThighSlide( const CString& in_prefix, const CString& in_guideobjectcollection, const CString& in_upperparent, const CString& in_lowerparent )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_prefix;
	args[i++]= in_guideobjectcollection;
	args[i++]= in_upperparent;
	args[i++]= in_lowerparent;

	CStatus st = gApp.ExecuteCommand( L"MakeThighSlide", args, retval );

	return retval;
}

// MakeTorso
CValue MakeTorso( const CString& in_parent, const CValue& /*number*/ in_nbdivisions, const CValue& /*number*/ in_stretchtype, const CString& in_guideobjectcollection, const CValue& /*number*/ in_controltype, const CValue& /*number*/ in_spinetype, const CString& in_sliders, const CValue& /*number*/ in_shadowtype, const CString& in_shadowparent, const CValue& /*number*/ in_negativescale )
{
	CValueArray args(10);
	CValue retval;
	LONG i(0);

	args[i++]= in_parent;
	args[i++]= in_nbdivisions;
	args[i++]= in_stretchtype;
	args[i++]= in_guideobjectcollection;
	args[i++]= in_controltype;
	args[i++]= in_spinetype;
	args[i++]= in_sliders;
	args[i++]= in_shadowtype;
	args[i++]= in_shadowparent;
	args[i++]= in_negativescale;

	CStatus st = gApp.ExecuteCommand( L"MakeTorso", args, retval );

	return retval;
}

// ManipulateTool
void ManipulateTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"ManipulateTool", args, retval );

	return;
}

// MatchTransform
void MatchTransform( const CValue& /*undefined*/ in_inputobjs, const CString& in_targetobj, const CString& in_type, bool in_localreference )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_targetobj;
	args[i++]= in_type;
	args[i++]= in_localreference;

	CStatus st = gApp.ExecuteCommand( L"MatchTransform", args, retval );

	return;
}

// MatchUVW
CValue MatchUVW( const CValue& /*undefined*/ in_textureprojection, const CValue& /*undefined*/ in_subcomp, const CValue& /*number*/ in_tolerance )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_textureprojection;
	args[i++]= in_subcomp;
	args[i++]= in_tolerance;

	CStatus st = gApp.ExecuteCommand( L"MatchUVW", args, retval );

	return retval;
}

// MergeClips
CValue MergeClips( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"MergeClips", args, retval );

	return retval;
}

// MergeMixer
void MergeMixer( const CValue& /*undefined*/ in_filename, const CValue& /*undefined*/ in_parent, bool in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_parent;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"MergeMixer", args, retval );

	return;
}

// MergeOrFreezeCharacter
void MergeOrFreezeCharacter( const CValue& /*undefined*/ in_ob1, const CValue& /*undefined*/ in_ob2, const CValue& /*undefined*/ in_bcreatemodel )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_ob1;
	args[i++]= in_ob2;
	args[i++]= in_bcreatemodel;

	CStatus st = gApp.ExecuteCommand( L"MergeOrFreezeCharacter", args, retval );

	return;
}

// MergeScene
CValue MergeScene( const CString& in_filename, const CString& in_name, const CString& in_parent, const CValue& /*number*/ in_shareoptions, const CString& in_createdobj )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_shareoptions;
	args[i++]= in_createdobj;

	CStatus st = gApp.ExecuteCommand( L"MergeScene", args, retval );

	return retval;
}

// MergeSimulationEnvironments
void MergeSimulationEnvironments( const CValue& /*undefined*/ in_simulationenvironments )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_simulationenvironments;

	CStatus st = gApp.ExecuteCommand( L"MergeSimulationEnvironments", args, retval );

	return;
}

// MeshSubdivideWithCenter
void MeshSubdivideWithCenter( const CString& in_presetobj, const CString& in_connectionset, const CValue& /*number*/ in_connecttype, siOperationMode in_immediatemode, siGeneratorInputsDisposalMode in_genopinputsdisposal )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_connectionset;
	args[i++]= in_connecttype;
	args[i++]= (LONG)in_immediatemode;
	args[i++]= (LONG)in_genopinputsdisposal;

	CStatus st = gApp.ExecuteCommand( L"MeshSubdivideWithCenter", args, retval );

	return;
}

// ModifyUVSetByContourStretching
void ModifyUVSetByContourStretching( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_textureprojection, const CValue& /*number*/ in_contourstretchingmethod, bool in_maintainaspect, const CValue& /*undefined*/ in_cornerpoints )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_textureprojection;
	args[i++]= in_contourstretchingmethod;
	args[i++]= in_maintainaspect;
	args[i++]= in_cornerpoints;

	CStatus st = gApp.ExecuteCommand( L"ModifyUVSetByContourStretching", args, retval );

	return;
}

// Move2DShape
void Move2DShape( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_x;
	args[i++]= in_y;

	CStatus st = gApp.ExecuteCommand( L"Move2DShape", args, retval );

	return;
}

// Move2DShapeLHandle
void Move2DShapeLHandle( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_index, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_index;
	args[i++]= in_x;
	args[i++]= in_y;

	CStatus st = gApp.ExecuteCommand( L"Move2DShapeLHandle", args, retval );

	return;
}

// Move2DShapePin
void Move2DShapePin( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_x;
	args[i++]= in_y;

	CStatus st = gApp.ExecuteCommand( L"Move2DShapePin", args, retval );

	return;
}

// Move2DShapePoint
void Move2DShapePoint( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_index, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_index;
	args[i++]= in_x;
	args[i++]= in_y;

	CStatus st = gApp.ExecuteCommand( L"Move2DShapePoint", args, retval );

	return;
}

// Move2DShapeRHandle
void Move2DShapeRHandle( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_index, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_index;
	args[i++]= in_x;
	args[i++]= in_y;

	CStatus st = gApp.ExecuteCommand( L"Move2DShapeRHandle", args, retval );

	return;
}

// MoveClipToTrack
void MoveClipToTrack( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_clip, const CValue& /*undefined*/ in_compound, const CValue& /*undefined*/ in_track, const CValue& /*undefined*/ in_time )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_clip;
	args[i++]= in_compound;
	args[i++]= in_track;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"MoveClipToTrack", args, retval );

	return;
}

// MoveComponent
void MoveComponent( const CRef& /*object*/ in_inputobjs, const CRef& /*object*/ in_deletedobjs, const CRef& /*object*/ in_createdobjs, bool in_proptagonly, const CRef& /*object*/ in_propinputobjs )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_deletedobjs;
	args[i++]= in_createdobjs;
	args[i++]= in_proptagonly;
	args[i++]= in_propinputobjs;

	CStatus st = gApp.ExecuteCommand( L"MoveComponent", args, retval );

	return;
}

// MoveCtr2Vertices
void MoveCtr2Vertices( const CValue& /*undefined*/ in_inputobjs, bool in_boundingbox )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_boundingbox;

	CStatus st = gApp.ExecuteCommand( L"MoveCtr2Vertices", args, retval );

	return;
}

// MoveJoint
void MoveJoint( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z, bool in_movejointbranch, bool in_movejointbone )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= in_movejointbranch;
	args[i++]= in_movejointbone;

	CStatus st = gApp.ExecuteCommand( L"MoveJoint", args, retval );

	return;
}

// MoveOperatorAfter
void MoveOperatorAfter( const CValue& /*undefined*/ in_object, const CValue& /*undefined*/ in_operatortomove, const CValue& /*undefined*/ in_operatorreference )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_object;
	args[i++]= in_operatortomove;
	args[i++]= in_operatorreference;

	CStatus st = gApp.ExecuteCommand( L"MoveOperatorAfter", args, retval );

	return;
}

// MoveOperatorBefore
void MoveOperatorBefore( const CValue& /*undefined*/ in_object, const CValue& /*undefined*/ in_operatortomove, const CValue& /*undefined*/ in_operatorreference )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_object;
	args[i++]= in_operatortomove;
	args[i++]= in_operatorreference;

	CStatus st = gApp.ExecuteCommand( L"MoveOperatorBefore", args, retval );

	return;
}

// MovePaintShapeHandle
void MovePaintShapeHandle( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_point, const CValue& /*number*/ in_handle, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_mode )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_point;
	args[i++]= in_handle;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_mode;

	CStatus st = gApp.ExecuteCommand( L"MovePaintShapeHandle", args, retval );

	return;
}

// MovePointTool
void MovePointTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"MovePointTool", args, retval );

	return;
}

// MoveTextureLayers
void MoveTextureLayers( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_offset )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_offset;

	CStatus st = gApp.ExecuteCommand( L"MoveTextureLayers", args, retval );

	return;
}

// MoveToFolder
void MoveToFolder( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"MoveToFolder", args, retval );

	return;
}

// MoveToLayer
void MoveToLayer( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"MoveToLayer", args, retval );

	return;
}

// MoveToLibrary
void MoveToLibrary( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_target )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"MoveToLibrary", args, retval );

	return;
}

// MoveToPartition
void MoveToPartition( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_source )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;
	args[i++]= in_source;

	CStatus st = gApp.ExecuteCommand( L"MoveToPartition", args, retval );

	return;
}

// MultiAttachRigidBodiesTool
CValue MultiAttachRigidBodiesTool( const CString& in_name, const CValue& /*undefined*/ in_bodies )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_name;
	args[i++]= in_bodies;

	CStatus st = gApp.ExecuteCommand( L"MultiAttachRigidBodiesTool", args, retval );

	return retval;
}

// NavigateNode
CValue NavigateNode( const CValue& /*undefined*/ in_source, const CValue& /*number*/ in_navdirection, bool in_navloop, const CValue& /*number*/ in_navfilter, const CValue& /*number*/ in_navsubfilter, const CRef& /*object*/ in_outputobj )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_navdirection;
	args[i++]= in_navloop;
	args[i++]= in_navfilter;
	args[i++]= in_navsubfilter;
	args[i++]= in_outputobj;

	CStatus st = gApp.ExecuteCommand( L"NavigateNode", args, retval );

	return retval;
}

// NewHelperBoneRig
void NewHelperBoneRig( const CValue& /*undefined*/ in_rig )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_rig;

	CStatus st = gApp.ExecuteCommand( L"NewHelperBoneRig", args, retval );

	return;
}

// NewScene
void NewScene( const CString& in_projectpathname, bool in_confirm )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_projectpathname;
	args[i++]= in_confirm;

	CStatus st = gApp.ExecuteCommand( L"NewScene", args, retval );

	return;
}

// NextFrame
void NextFrame( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"NextFrame", args, retval );

	return;
}

// NextKey
CValue NextKey( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_time, const CValue& /*number*/ in_tolerance )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;
	args[i++]= in_tolerance;

	CStatus st = gApp.ExecuteCommand( L"NextKey", args, retval );

	return retval;
}

// NextModifiedPaintClipFrame
void NextModifiedPaintClipFrame( const CValue& /*undefined*/ in_paintclip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_paintclip;

	CStatus st = gApp.ExecuteCommand( L"NextModifiedPaintClipFrame", args, retval );

	return;
}

// NextPaintShapeKey
void NextPaintShapeKey( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"NextPaintShapeKey", args, retval );

	return;
}

// ObjExport
void ObjExport( const CString& in_filename, const CValue& /*number*/ in_fileperobject, bool in_fileperframe, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, const CValue& /*number*/ in_stepframe, bool in_polymsh, bool in_surfmsh, bool in_crvlist, bool in_cloud, bool in_coordinatesystem, const CValue& /*number*/ in_tesselation, bool in_material, bool in_uv, bool in_usernormal )
{
	CValueArray args(15);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_fileperobject;
	args[i++]= in_fileperframe;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_stepframe;
	args[i++]= in_polymsh;
	args[i++]= in_surfmsh;
	args[i++]= in_crvlist;
	args[i++]= in_cloud;
	args[i++]= in_coordinatesystem;
	args[i++]= in_tesselation;
	args[i++]= in_material;
	args[i++]= in_uv;
	args[i++]= in_usernormal;

	CStatus st = gApp.ExecuteCommand( L"ObjExport", args, retval );

	return;
}

// ObjImport
void ObjImport( const CString& in_filename, const CValue& /*number*/ in_group, const CValue& /*number*/ in_hrc, bool in_material, bool in_uv, bool in_usernormal )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_group;
	args[i++]= in_hrc;
	args[i++]= in_material;
	args[i++]= in_uv;
	args[i++]= in_usernormal;

	CStatus st = gApp.ExecuteCommand( L"ObjImport", args, retval );

	return;
}

// OffloadAction
void OffloadAction( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"OffloadAction", args, retval );

	return;
}

// OffsetValueMap
void OffsetValueMap( const CValue& /*undefined*/ in_inputobjs, siEffectInputType in_type, const CValue& /*undefined*/ in_time, const CValue& /*undefined*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_type;
	args[i++]= in_time;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"OffsetValueMap", args, retval );

	return;
}

// OpenAnimationEditor
void OpenAnimationEditor( const CString& in_animationview, const CString& in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_animationview;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"OpenAnimationEditor", args, retval );

	return;
}

// OpenAttachments
void OpenAttachments( const CValue& /*undefined*/ in_target )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"OpenAttachments", args, retval );

	return;
}

// OpenNetView
void OpenNetView( const CValue& /*undefined*/ in_url, bool in_reuse, const CValue& /*number*/ in_flags )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_url;
	args[i++]= in_reuse;
	args[i++]= in_flags;

	CStatus st = gApp.ExecuteCommand( L"OpenNetView", args, retval );

	return;
}

// OpenScene
void OpenScene( const CValue& /*undefined*/ in_scenefilename, bool in_confirm, bool in_applyauxiliarydata )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_scenefilename;
	args[i++]= in_confirm;
	args[i++]= in_applyauxiliarydata;

	CStatus st = gApp.ExecuteCommand( L"OpenScene", args, retval );

	return;
}

// OpenTransientExplorer
CValue OpenTransientExplorer( const CValue& /*undefined*/ in_initobjects, siTransientExplorerFilter in_filter, const CValue& /*number*/ in_expandlevel, bool in_sort, bool in_multiselect, const CRef& /*object*/ in_value )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_initobjects;
	args[i++]= (LONG)in_filter;
	args[i++]= in_expandlevel;
	args[i++]= in_sort;
	args[i++]= in_multiselect;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"OpenTransientExplorer", args, retval );

	return retval;
}

// OpenXSIExplorer
void OpenXSIExplorer( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"OpenXSIExplorer", args, retval );

	return;
}

// OverrideLightShader
void OverrideLightShader( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"OverrideLightShader", args, retval );

	return;
}

// PaintPushTool
void PaintPushTool( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"PaintPushTool", args, retval );

	return;
}

// PaintRasterLine
void PaintRasterLine( const CValue& /*number*/ in_x1, const CValue& /*number*/ in_y1, const CValue& /*number*/ in_p1, const CValue& /*number*/ in_x2, const CValue& /*number*/ in_y2, const CValue& /*number*/ in_p2 )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_x1;
	args[i++]= in_y1;
	args[i++]= in_p1;
	args[i++]= in_x2;
	args[i++]= in_y2;
	args[i++]= in_p2;

	CStatus st = gApp.ExecuteCommand( L"PaintRasterLine", args, retval );

	return;
}

// PaintSelectTool
void PaintSelectTool( const CString& in_clsid, const CString& in_cmdid, bool in_keyboardprocess )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;
	args[i++]= in_cmdid;
	args[i++]= in_keyboardprocess;

	CStatus st = gApp.ExecuteCommand( L"PaintSelectTool", args, retval );

	return;
}

// PaintShapeManipTool
void PaintShapeManipTool( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"PaintShapeManipTool", args, retval );

	return;
}

// PaintTool
void PaintTool( const CString& in_clsid, const CString& in_cmdid, bool in_keyboardprocess )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;
	args[i++]= in_cmdid;
	args[i++]= in_keyboardprocess;

	CStatus st = gApp.ExecuteCommand( L"PaintTool", args, retval );

	return;
}

// PaintVertexColors
CValue PaintVertexColors( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_polynodes, const CValue& /*undefined*/ in_colors, const CValue& /*number*/ in_mode, const CRef& /*object*/ in_outputobj )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_polynodes;
	args[i++]= in_colors;
	args[i++]= in_mode;
	args[i++]= in_outputobj;

	CStatus st = gApp.ExecuteCommand( L"PaintVertexColors", args, retval );

	return retval;
}

// PaintVertexColorTool
void PaintVertexColorTool( const CString& in_clsid, const CString& in_cmdid, bool in_keyboardprocess )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;
	args[i++]= in_cmdid;
	args[i++]= in_keyboardprocess;

	CStatus st = gApp.ExecuteCommand( L"PaintVertexColorTool", args, retval );

	return;
}

// PaintWeights
CValue PaintWeights( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_indices, const CValue& /*undefined*/ in_weights, const CValue& /*number*/ in_flags, const CValue& /*number*/ in_deformer, const CRef& /*object*/ in_outputobj )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_indices;
	args[i++]= in_weights;
	args[i++]= in_flags;
	args[i++]= in_deformer;
	args[i++]= in_outputobj;

	CStatus st = gApp.ExecuteCommand( L"PaintWeights", args, retval );

	return retval;
}

// ParamBracketing
void ParamBracketing( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ParamBracketing", args, retval );

	return;
}

// ParentObj
void ParentObj( const CValue& /*undefined*/ in_parent, const CValue& /*undefined*/ in_children )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_parent;
	args[i++]= in_children;

	CStatus st = gApp.ExecuteCommand( L"ParentObj", args, retval );

	return;
}

// PartialOffloadAction
void PartialOffloadAction( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, bool in_remember )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_remember;

	CStatus st = gApp.ExecuteCommand( L"PartialOffloadAction", args, retval );

	return;
}

// Paste
void Paste( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"Paste", args, retval );

	return;
}

// PasteAction
CValue PasteAction( const CValue& /*undefined*/ in_source, const CValue& /*undefined*/ in_target, const CValue& /*number*/ in_copystart, const CValue& /*number*/ in_copyend, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, bool in_ripple )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_target;
	args[i++]= in_copystart;
	args[i++]= in_copyend;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_ripple;

	CStatus st = gApp.ExecuteCommand( L"PasteAction", args, retval );

	return retval;
}

// PasteAllAnimation
void PasteAllAnimation( const CValue& /*undefined*/ in_inputobjs, bool in_useposition )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_useposition;

	CStatus st = gApp.ExecuteCommand( L"PasteAllAnimation", args, retval );

	return;
}

// PasteAnimation
void PasteAnimation( const CValue& /*undefined*/ in_inputobjs, bool in_useposition )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_useposition;

	CStatus st = gApp.ExecuteCommand( L"PasteAnimation", args, retval );

	return;
}

// PasteFxOp
void PasteFxOp( const CValue& /*undefined*/ in_fxtree )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_fxtree;

	CStatus st = gApp.ExecuteCommand( L"PasteFxOp", args, retval );

	return;
}

// PasteKeys
void PasteKeys( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, bool in_ripple, const CValue& /*number*/ in_offset, bool in_merge )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_ripple;
	args[i++]= in_offset;
	args[i++]= in_merge;

	CStatus st = gApp.ExecuteCommand( L"PasteKeys", args, retval );

	return;
}

// PasteUVW
void PasteUVW( const CValue& /*undefined*/ in_textureprojection, const CValue& /*undefined*/ in_subcomp )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_textureprojection;
	args[i++]= in_subcomp;

	CStatus st = gApp.ExecuteCommand( L"PasteUVW", args, retval );

	return;
}

// PathRetime
void PathRetime( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_s3d_cmdarg_start1, const CValue& /*number*/ in_s3d_cmdarg_end1, const CValue& /*number*/ in_s3d_cmdarg_start2, const CValue& /*number*/ in_s3d_cmdarg_end2 )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_s3d_cmdarg_start1;
	args[i++]= in_s3d_cmdarg_end1;
	args[i++]= in_s3d_cmdarg_start2;
	args[i++]= in_s3d_cmdarg_end2;

	CStatus st = gApp.ExecuteCommand( L"PathRetime", args, retval );

	return;
}

// PickElement
void PickElement( const CValue& /*undefined*/ in_selfilter, const CString& in_leftmessage, const CString& in_middlemessage, const CRef& /*object*/ in_pickedelement, const CValue& /*number*/ in_buttonpressed, const CValue& /*number*/ in_selregionmode, const CValue& /*number*/ in_modifierpressed )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_selfilter;
	args[i++]= in_leftmessage;
	args[i++]= in_middlemessage;
	args[i++]= in_pickedelement;
	args[i++]= in_buttonpressed;
	args[i++]= in_selregionmode;
	args[i++]= in_modifierpressed;

	CStatus st = gApp.ExecuteCommand( L"PickElement", args, retval );

	return;
}

// PickObject
void PickObject( const CString& in_leftmessage, const CString& in_middlemessage, const CString& in_pickedelement, const CRef& /*object*/ in_buttonpressed, const CValue& /*number*/ in_modifierpressed )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_leftmessage;
	args[i++]= in_middlemessage;
	args[i++]= in_pickedelement;
	args[i++]= in_buttonpressed;
	args[i++]= in_modifierpressed;

	CStatus st = gApp.ExecuteCommand( L"PickObject", args, retval );

	return;
}

// PickPosition
void PickPosition( const CString& in_leftmessage, const CString& in_middlemessage, const CValue& /*number*/ in_posx, const CValue& /*number*/ in_posy, const CValue& /*number*/ in_posz, const CValue& /*number*/ in_buttonpressed )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_leftmessage;
	args[i++]= in_middlemessage;
	args[i++]= in_posx;
	args[i++]= in_posy;
	args[i++]= in_posz;
	args[i++]= in_buttonpressed;

	CStatus st = gApp.ExecuteCommand( L"PickPosition", args, retval );

	return;
}

// PlaybackStop
void PlaybackStop( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"PlaybackStop", args, retval );

	return;
}

// PlayBackwards
void PlayBackwards( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"PlayBackwards", args, retval );

	return;
}

// PlayBackwardsFromEnd
void PlayBackwardsFromEnd( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"PlayBackwardsFromEnd", args, retval );

	return;
}

// PlayForwards
void PlayForwards( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"PlayForwards", args, retval );

	return;
}

// PlayForwardsFromStart
void PlayForwardsFromStart( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"PlayForwardsFromStart", args, retval );

	return;
}

// PlayFrame
void PlayFrame( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"PlayFrame", args, retval );

	return;
}

// PlayRealTime
void PlayRealTime( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"PlayRealTime", args, retval );

	return;
}

// PlayRealTimeFromStart
void PlayRealTimeFromStart( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"PlayRealTimeFromStart", args, retval );

	return;
}

// PlotAndApplyAction
void PlotAndApplyAction( const CValue& /*undefined*/ in_model, const CString& in_inputobj, const CValue& /*undefined*/ in_name, const CValue& /*undefined*/ in_startframe, const CValue& /*undefined*/ in_endframe, const CValue& /*undefined*/ in_stepframe, const CValue& /*undefined*/ in_fcurve_kind, const CValue& /*undefined*/ in_defaultsegkind, const CValue& /*undefined*/ in_fit_fcurve, const CValue& /*undefined*/ in_fit_tolerance, const CValue& /*undefined*/ in_processcontrots, const CValue& /*undefined*/ in_apply, const CValue& /*undefined*/ in_paste, const CValue& /*undefined*/ in_delete, const CString& in_title )
{
	CValueArray args(14);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_inputobj;
	args[i++]= in_name;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_stepframe;
	args[i++]= in_fcurve_kind;
	args[i++]= in_defaultsegkind;
	args[i++]= in_fit_fcurve;
	args[i++]= in_fit_tolerance;
	args[i++]= in_processcontrots;
	args[i++]= in_apply;
	args[i++]= in_paste;
	args[i++]= in_delete;
//	args[i++]= in_title;

	CStatus st = gApp.ExecuteCommand( L"PlotAndApplyAction", args, retval );

	return;
}

// PlotConstrainedTransforms
void PlotConstrainedTransforms( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_name, const CValue& /*undefined*/ in_startframe, const CValue& /*undefined*/ in_endframe, const CValue& /*undefined*/ in_stepframe, const CValue& /*undefined*/ in_fcurve_kind, const CValue& /*undefined*/ in_defaultsegkind, const CValue& /*undefined*/ in_fit_fcurve, const CValue& /*undefined*/ in_fit_tolerance, const CValue& /*undefined*/ in_processcontrots, const CValue& /*undefined*/ in_apply, const CValue& /*undefined*/ in_paste, const CValue& /*undefined*/ in_delete, const CString& in_title )
{
	CValueArray args(14);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_stepframe;
	args[i++]= in_fcurve_kind;
	args[i++]= in_defaultsegkind;
	args[i++]= in_fit_fcurve;
	args[i++]= in_fit_tolerance;
	args[i++]= in_processcontrots;
	args[i++]= in_apply;
	args[i++]= in_paste;
	args[i++]= in_delete;
	args[i++]= in_title;

	CStatus st = gApp.ExecuteCommand( L"PlotConstrainedTransforms", args, retval );

	return;
}

// PlotCurve
void PlotCurve( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_name, const CValue& /*undefined*/ in_startframe, const CValue& /*undefined*/ in_endframe, const CValue& /*undefined*/ in_stepframe )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_stepframe;

	CStatus st = gApp.ExecuteCommand( L"PlotCurve", args, retval );

	return;
}

// PlotShape
void PlotShape( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_name, const CValue& /*undefined*/ in_startframe, const CValue& /*undefined*/ in_endframe, const CValue& /*undefined*/ in_stepframe, const CValue& /*undefined*/ in_applykeys, const CValue& /*undefined*/ in_mode, const CValue& /*undefined*/ in_content, const CValue& /*undefined*/ in_refmode, const CValue& /*undefined*/ in_singlesource, const CRef& /*object*/ in_keylist, const CRef& /*object*/ in_cliplist )
{
	CValueArray args(12);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_name;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_stepframe;
	args[i++]= in_applykeys;
	args[i++]= in_mode;
	args[i++]= in_content;
	args[i++]= in_refmode;
	args[i++]= in_singlesource;
	args[i++]= in_keylist;
	args[i++]= in_cliplist;

	CStatus st = gApp.ExecuteCommand( L"PlotShape", args, retval );

	return;
}

// PlotToAction
CValue PlotToAction( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_inputobj, const CString& in_name, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, const CValue& /*number*/ in_stepframe, const CValue& /*number*/ in_fcurve_kind, const CValue& /*number*/ in_defaultsegkind, bool in_fit_fcurve, const CValue& /*number*/ in_fit_tolerance, bool in_processcontrots )
{
	CValueArray args(11);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_inputobj;
	args[i++]= in_name;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_stepframe;
	args[i++]= in_fcurve_kind;
	args[i++]= in_defaultsegkind;
	args[i++]= in_fit_fcurve;
	args[i++]= in_fit_tolerance;
	args[i++]= in_processcontrots;

	CStatus st = gApp.ExecuteCommand( L"PlotToAction", args, retval );

	return retval;
}

// PrevFrame
void PrevFrame( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"PrevFrame", args, retval );

	return;
}

// PrevKey
CValue PrevKey( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_time, const CValue& /*number*/ in_tolerance )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;
	args[i++]= in_tolerance;

	CStatus st = gApp.ExecuteCommand( L"PrevKey", args, retval );

	return retval;
}

// PrevModifiedPaintClipFrame
void PrevModifiedPaintClipFrame( const CValue& /*undefined*/ in_paintclip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_paintclip;

	CStatus st = gApp.ExecuteCommand( L"PrevModifiedPaintClipFrame", args, retval );

	return;
}

// PrevPaintShapeKey
void PrevPaintShapeKey( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"PrevPaintShapeKey", args, retval );

	return;
}

// PropertyTransfer
void PropertyTransfer( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_propobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_propobj;

	CStatus st = gApp.ExecuteCommand( L"PropertyTransfer", args, retval );

	return;
}

// RandomizeParameters
void RandomizeParameters( const CValue& /*undefined*/ in_params, const CValue& /*number*/ in_percentage, bool in_relative, const CValue& /*number*/ in_seed )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_params;
	args[i++]= in_percentage;
	args[i++]= in_relative;
	args[i++]= in_seed;

	CStatus st = gApp.ExecuteCommand( L"RandomizeParameters", args, retval );

	return;
}

// ReconnectMixer
void ReconnectMixer( const CValue& /*undefined*/ in_model )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;

	CStatus st = gApp.ExecuteCommand( L"ReconnectMixer", args, retval );

	return;
}

// ReconnectModel
void ReconnectModel( const CValue& /*undefined*/ in_model, const CString& in_externalcnxmappingtemplate, bool in_deleteunconnectedobjects )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_externalcnxmappingtemplate;
	args[i++]= in_deleteunconnectedobjects;

	CStatus st = gApp.ExecuteCommand( L"ReconnectModel", args, retval );

	return;
}

// RectanglePaintRaster
void RectanglePaintRaster( const CValue& /*number*/ in_x1, const CValue& /*number*/ in_y1, const CValue& /*number*/ in_x2, const CValue& /*number*/ in_y2, bool in_fromcenter )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_x1;
	args[i++]= in_y1;
	args[i++]= in_x2;
	args[i++]= in_y2;
	args[i++]= in_fromcenter;

	CStatus st = gApp.ExecuteCommand( L"RectanglePaintRaster", args, retval );

	return;
}

// RectangularPaintMask
CValue RectangularPaintMask( const CValue& /*number*/ in_x1, const CValue& /*number*/ in_y1, const CValue& /*number*/ in_x2, const CValue& /*number*/ in_y2, bool in_fromcenter, const CValue& /*number*/ in_selectionaction, bool in_antialiased, const CValue& /*number*/ in_feather, const CRef& /*object*/ in_value )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_x1;
	args[i++]= in_y1;
	args[i++]= in_x2;
	args[i++]= in_y2;
	args[i++]= in_fromcenter;
	args[i++]= in_selectionaction;
	args[i++]= in_antialiased;
	args[i++]= in_feather;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"RectangularPaintMask", args, retval );

	return retval;
}

// Redo
void Redo( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"Redo", args, retval );

	return;
}

// Refresh
void Refresh( const CValue& /*undefined*/ in_time )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"Refresh", args, retval );

	return;
}

// RefreshCustomPreferences
void RefreshCustomPreferences( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"RefreshCustomPreferences", args, retval );

	return;
}

// RefreshRegion
void RefreshRegion( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"RefreshRegion", args, retval );

	return;
}

// RegenerateMaps
void RegenerateMaps( const CValue& /*undefined*/ in_properties )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_properties;

	CStatus st = gApp.ExecuteCommand( L"RegenerateMaps", args, retval );

	return;
}

// RelaxUVW
CValue RelaxUVW( const CValue& /*undefined*/ in_textureprojection, const CValue& /*undefined*/ in_subcomp )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_textureprojection;
	args[i++]= in_subcomp;

	CStatus st = gApp.ExecuteCommand( L"RelaxUVW", args, retval );

	return retval;
}

// ReloadAction
void ReloadAction( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ReloadAction", args, retval );

	return;
}

// ReloadAddonLib
void ReloadAddonLib( const CValue& /*undefined*/ in_filename )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"ReloadAddonLib", args, retval );

	return;
}

// ReloadAllAddonLibs
void ReloadAllAddonLibs( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ReloadAllAddonLibs", args, retval );

	return;
}

// RemoveAllAnimation
void RemoveAllAnimation( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_time, siBranchFlag in_branchflag, siSourceType in_sourcemask, siParameterFilter in_parametermask )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;
	args[i++]= (LONG)in_branchflag;
	args[i++]= (LONG)in_sourcemask;
	args[i++]= (LONG)in_parametermask;

	CStatus st = gApp.ExecuteCommand( L"RemoveAllAnimation", args, retval );

	return;
}

// RemoveAllCns
void RemoveAllCns( const CValue& /*undefined*/ in_constrainedobj, const CValue& /*number*/ in_propagation )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_constrainedobj;
	args[i++]= in_propagation;

	CStatus st = gApp.ExecuteCommand( L"RemoveAllCns", args, retval );

	return;
}

// RemoveAllProperties
void RemoveAllProperties( const CValue& /*undefined*/ in_inputobjs, siBranchFlag in_branchflag )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_branchflag;

	CStatus st = gApp.ExecuteCommand( L"RemoveAllProperties", args, retval );

	return;
}

// RemoveAllShadersFromCnxPoint
void RemoveAllShadersFromCnxPoint( const CValue& /*undefined*/ in_inputobjs, siShaderCnxPointType in_type )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_type;

	CStatus st = gApp.ExecuteCommand( L"RemoveAllShadersFromCnxPoint", args, retval );

	return;
}

// RemoveAnimation
void RemoveAnimation( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"RemoveAnimation", args, retval );

	return;
}

// RemoveCluster
void RemoveCluster( const CString& in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"RemoveCluster", args, retval );

	return;
}

// RemoveCns
void RemoveCns( const CValue& /*undefined*/ in_constrainedobj, const CValue& /*undefined*/ in_constrainingobj, bool in_forcedeletion )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_constrainedobj;
	args[i++]= in_constrainingobj;
	args[i++]= in_forcedeletion;

	CStatus st = gApp.ExecuteCommand( L"RemoveCns", args, retval );

	return;
}

// RemoveCnsType
void RemoveCnsType( const CString& in_type, const CValue& /*undefined*/ in_constrainedobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_type;
	args[i++]= in_constrainedobj;

	CStatus st = gApp.ExecuteCommand( L"RemoveCnsType", args, retval );

	return;
}

// RemoveCOG
CValue RemoveCOG( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"RemoveCOG", args, retval );

	return retval;
}

// RemoveCustomParam
void RemoveCustomParam( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"RemoveCustomParam", args, retval );

	return;
}

// RemoveEffect
void RemoveEffect( const CValue& /*undefined*/ in_clip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"RemoveEffect", args, retval );

	return;
}

// RemoveExternalConstraints
CValue RemoveExternalConstraints( const CValue& /*undefined*/ in_inputobj, const CString& in_operation )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_operation;

	CStatus st = gApp.ExecuteCommand( L"RemoveExternalConstraints", args, retval );

	return retval;
}

// RemoveFlexEnv
void RemoveFlexEnv( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"RemoveFlexEnv", args, retval );

	return;
}

// RemoveFlexEnvDeformer
void RemoveFlexEnvDeformer( const CValue& /*undefined*/ in_connectionset, bool in_interactive )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= in_interactive;

	CStatus st = gApp.ExecuteCommand( L"RemoveFlexEnvDeformer", args, retval );

	return;
}

// RemoveFloor
CValue RemoveFloor( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"RemoveFloor", args, retval );

	return retval;
}

// RemoveFolder
void RemoveFolder( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"RemoveFolder", args, retval );

	return;
}

// RemoveFoot
CValue RemoveFoot( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"RemoveFoot", args, retval );

	return retval;
}

// RemoveFromCluster
void RemoveFromCluster( const CString& in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"RemoveFromCluster", args, retval );

	return;
}

// RemoveFromContainer
void RemoveFromContainer( const CString& in_target, const CString& in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"RemoveFromContainer", args, retval );

	return;
}

// RemoveFromFolder
void RemoveFromFolder( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"RemoveFromFolder", args, retval );

	return;
}

// RemoveFromGroup
void RemoveFromGroup( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"RemoveFromGroup", args, retval );

	return;
}

// RemoveFromMarking
void RemoveFromMarking( const CValue& /*undefined*/ in_pathlist )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_pathlist;

	CStatus st = gApp.ExecuteCommand( L"RemoveFromMarking", args, retval );

	return;
}

// RemoveFromPartition
void RemoveFromPartition( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"RemoveFromPartition", args, retval );

	return;
}

// RemoveFromSelection
CValue RemoveFromSelection( const CValue& /*undefined*/ in_selectionlist, const CString& in_hierarchylevel )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;
	args[i++]= in_hierarchylevel;

	CStatus st = gApp.ExecuteCommand( L"RemoveFromSelection", args, retval );

	return retval;
}

// RemoveGroup
void RemoveGroup( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"RemoveGroup", args, retval );

	return;
}

// RemoveInputFromOperator
void RemoveInputFromOperator( const CValue& /*undefined*/ in_reference )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_reference;

	CStatus st = gApp.ExecuteCommand( L"RemoveInputFromOperator", args, retval );

	return;
}

// RemoveJointLimit
CValue RemoveJointLimit( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"RemoveJointLimit", args, retval );

	return retval;
}

// RemoveKey
void RemoveKey( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_time, const CValue& /*number*/ in_tolerance )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;
	args[i++]= in_tolerance;

	CStatus st = gApp.ExecuteCommand( L"RemoveKey", args, retval );

	return;
}

// RemoveLastPointInteriorToPolygon
void RemoveLastPointInteriorToPolygon( const CValue& /*undefined*/ in_operator, const CValue& /*number*/ in_polygonid )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_polygonid;

	CStatus st = gApp.ExecuteCommand( L"RemoveLastPointInteriorToPolygon", args, retval );

	return;
}

// RemoveObjectsFromShader
void RemoveObjectsFromShader( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_target )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"RemoveObjectsFromShader", args, retval );

	return;
}

// RemoveParTypeInstanceGroup
void RemoveParTypeInstanceGroup( const CValue& /*undefined*/ in_partype )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_partype;

	CStatus st = gApp.ExecuteCommand( L"RemoveParTypeInstanceGroup", args, retval );

	return;
}

// RemovePointFromPolygon
void RemovePointFromPolygon( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_pointid, const CValue& /*number*/ in_polygonid )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_pointid;
	args[i++]= in_polygonid;

	CStatus st = gApp.ExecuteCommand( L"RemovePointFromPolygon", args, retval );

	return;
}

// RemoveProperties
void RemoveProperties( const CValue& /*undefined*/ in_inputobjs, const CString& in_type, siBranchFlag in_branchflag )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_type;
	args[i++]= (LONG)in_branchflag;

	CStatus st = gApp.ExecuteCommand( L"RemoveProperties", args, retval );

	return;
}

// RemoveProxyParam
void RemoveProxyParam( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"RemoveProxyParam", args, retval );

	return;
}

// RemoveRelativeValue
void RemoveRelativeValue( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_index )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_index;

	CStatus st = gApp.ExecuteCommand( L"RemoveRelativeValue", args, retval );

	return;
}

// RemoveRigidBody
void RemoveRigidBody( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"RemoveRigidBody", args, retval );

	return;
}

// RemoveShaderFromCnxPoint
CValue RemoveShaderFromCnxPoint( const CValue& /*undefined*/ in_source, const CValue& /*undefined*/ in_inputobjs, bool in_deleteonlastdisconnection )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_inputobjs;
	args[i++]= in_deleteonlastdisconnection;

	CStatus st = gApp.ExecuteCommand( L"RemoveShaderFromCnxPoint", args, retval );

	return retval;
}

// RemoveTextureLayers
void RemoveTextureLayers( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_container )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_container;

	CStatus st = gApp.ExecuteCommand( L"RemoveTextureLayers", args, retval );

	return;
}

// RemoveUserParameter
void RemoveUserParameter( const CValue& /*undefined*/ in_target, const CString& in_name )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"RemoveUserParameter", args, retval );

	return;
}

// RenamePropAndRebind
void RenamePropAndRebind( const CValue& /*undefined*/ in_properties, const CString& in_newname, bool in_updaterefs, bool in_updatewildcards, bool in_showui )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_properties;
	args[i++]= in_newname;
	args[i++]= in_updaterefs;
	args[i++]= in_updatewildcards;
	args[i++]= in_showui;

	CStatus st = gApp.ExecuteCommand( L"RenamePropAndRebind", args, retval );

	return;
}

// RenamePropAndRebind_Internal
void RenamePropAndRebind_Internal( const CValue& /*undefined*/ in_properties, const CString& in_propertyname, bool in_updaterefs, bool in_updatewildcards )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_properties;
	args[i++]= in_propertyname;
	args[i++]= in_updaterefs;
	args[i++]= in_updatewildcards;

	CStatus st = gApp.ExecuteCommand( L"RenamePropAndRebind_Internal", args, retval );

	return;
}

// RenamePropAndRebindUI
void RenamePropAndRebindUI( const CValue& /*undefined*/ in_properties, const CString& in_newname, bool in_updaterefs, bool in_updatewildcards )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_properties;
	args[i++]= in_newname;
	args[i++]= in_updaterefs;
	args[i++]= in_updatewildcards;

	CStatus st = gApp.ExecuteCommand( L"RenamePropAndRebindUI", args, retval );

	return;
}

// RenderAllPasses
void RenderAllPasses( bool in_passcentric )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_passcentric;

	CStatus st = gApp.ExecuteCommand( L"RenderAllPasses", args, retval );

	return;
}

// RenderFxOp
void RenderFxOp( const CValue& /*undefined*/ in_inputobj, bool in_interactive )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_interactive;

	CStatus st = gApp.ExecuteCommand( L"RenderFxOp", args, retval );

	return;
}

// RenderPass
void RenderPass( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_startframe, const CValue& /*undefined*/ in_endframe, const CValue& /*undefined*/ in_stepframe, bool in_disposerendering )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_presetobj;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_stepframe;
	args[i++]= in_disposerendering;

	CStatus st = gApp.ExecuteCommand( L"RenderPass", args, retval );

	return;
}

// RenderRegionTool
void RenderRegionTool( const CString& in_clsid, bool in_keyboardprocess )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;
	args[i++]= in_keyboardprocess;

	CStatus st = gApp.ExecuteCommand( L"RenderRegionTool", args, retval );

	return;
}

// RenderSetup
CValue RenderSetup( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"RenderSetup", args, retval );

	return retval;
}

// ReplaceShader
void ReplaceShader( const CValue& /*undefined*/ in_source, const CValue& /*undefined*/ in_target )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"ReplaceShader", args, retval );

	return;
}

// ReplaceShaderWithPreset
void ReplaceShaderWithPreset( const CValue& /*undefined*/ in_source, const CValue& /*undefined*/ in_presetobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_presetobj;

	CStatus st = gApp.ExecuteCommand( L"ReplaceShaderWithPreset", args, retval );

	return;
}

// ReplaceShapeKey
void ReplaceShapeKey( const CValue& /*undefined*/ in_inputobj, const CRef& /*object*/ in_outputobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_outputobj;

	CStatus st = gApp.ExecuteCommand( L"ReplaceShapeKey", args, retval );

	return;
}

// ResetActor
CValue ResetActor( const CValue& /*undefined*/ in_propobj, bool in_parameter )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_propobj;
	args[i++]= in_parameter;

	CStatus st = gApp.ExecuteCommand( L"ResetActor", args, retval );

	return retval;
}

// ResetAllEffectCurves
void ResetAllEffectCurves( const CValue& /*undefined*/ in_clip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"ResetAllEffectCurves", args, retval );

	return;
}

// ResetAllHelperTriggers
void ResetAllHelperTriggers( const CValue& /*undefined*/ in_helper, bool in_confirm )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_helper;
	args[i++]= in_confirm;

	CStatus st = gApp.ExecuteCommand( L"ResetAllHelperTriggers", args, retval );

	return;
}

// ResetAllViews
void ResetAllViews( const CValue& /*number*/ in_view, bool in_allviews )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_view;
	args[i++]= in_allviews;

	CStatus st = gApp.ExecuteCommand( L"ResetAllViews", args, retval );

	return;
}

// ResetEffectAtFrame
void ResetEffectAtFrame( const CValue& /*undefined*/ in_clip, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"ResetEffectAtFrame", args, retval );

	return;
}

// ResetFlexEnvPtsAssignment
CValue ResetFlexEnvPtsAssignment( const CValue& /*undefined*/ in_subcomp, const CValue& /*undefined*/ in_connectionset )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_subcomp;
	args[i++]= in_connectionset;

	CStatus st = gApp.ExecuteCommand( L"ResetFlexEnvPtsAssignment", args, retval );

	return retval;
}

// ResetHelperBoneRig
void ResetHelperBoneRig( const CValue& /*undefined*/ in_rig, bool in_prompt )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_rig;
	args[i++]= in_prompt;

	CStatus st = gApp.ExecuteCommand( L"ResetHelperBoneRig", args, retval );

	return;
}

// ResetOffset
void ResetOffset( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"ResetOffset", args, retval );

	return;
}

// ResetPaintColor
void ResetPaintColor( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ResetPaintColor", args, retval );

	return;
}

// ResetRenderFilePaths
void ResetRenderFilePaths( const CValue& /*undefined*/ in_inputobjs, bool in_resettorelative, bool in_includepassname, bool in_includescenename, const CString& in_prefix, const CString& in_separatortoken )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_resettorelative;
	args[i++]= in_includepassname;
	args[i++]= in_includescenename;
	args[i++]= in_prefix;
	args[i++]= in_separatortoken;

	CStatus st = gApp.ExecuteCommand( L"ResetRenderFilePaths", args, retval );

	return;
}

// ResetSelectedEffectCurves
void ResetSelectedEffectCurves( const CValue& /*undefined*/ in_clip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"ResetSelectedEffectCurves", args, retval );

	return;
}

// ResetTransform
void ResetTransform( const CValue& /*undefined*/ in_inputobjs, siCenterMode in_center, siTransformFilter in_type, siAxesFilter in_axesfilter )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_center;
	args[i++]= (LONG)in_type;
	args[i++]= (LONG)in_axesfilter;

	CStatus st = gApp.ExecuteCommand( L"ResetTransform", args, retval );

	return;
}

// RestoreDefaultPreference
void RestoreDefaultPreference( const CString& in_preferencename )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_preferencename;

	CStatus st = gApp.ExecuteCommand( L"RestoreDefaultPreference", args, retval );

	return;
}

// RevertPaintClip
void RevertPaintClip( const CValue& /*undefined*/ in_paintclip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_paintclip;

	CStatus st = gApp.ExecuteCommand( L"RevertPaintClip", args, retval );

	return;
}

// RevertPaintClipFrame
void RevertPaintClipFrame( const CValue& /*undefined*/ in_paintclip, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_paintclip;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"RevertPaintClipFrame", args, retval );

	return;
}

// RevertSelectedPaintClipFrame
void RevertSelectedPaintClipFrame( const CValue& /*undefined*/ in_paintclip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_paintclip;

	CStatus st = gApp.ExecuteCommand( L"RevertSelectedPaintClipFrame", args, retval );

	return;
}

// Rotate
void Rotate( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z, siDeltaMode in_delta, siRefMode in_refmode, siCenterMode in_center, siAxesFilter in_axesfilter, const CValue& /*undefined*/ in_reference, bool in_splitlocalcomponents, bool in_proptagonly, bool in_pivot, const CValue& /*number*/ in_pivotx, const CValue& /*number*/ in_pivoty, const CValue& /*number*/ in_pivotz, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(16);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= (LONG)in_delta;
	args[i++]= (LONG)in_refmode;
	args[i++]= (LONG)in_center;
	args[i++]= (LONG)in_axesfilter;
	args[i++]= in_reference;
	args[i++]= in_splitlocalcomponents;
	args[i++]= in_proptagonly;
	args[i++]= in_pivot;
	args[i++]= in_pivotx;
	args[i++]= in_pivoty;
	args[i++]= in_pivotz;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"Rotate", args, retval );

	return;
}

// RotateUVW
void RotateUVW( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_textureprojection, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z, siDeltaMode in_delta, siUVWAxesFilter in_axesfilter, bool in_pivot, const CValue& /*number*/ in_pivotu, const CValue& /*number*/ in_pivotv, const CValue& /*number*/ in_pivotw, const CValue& /*number*/ in_aspectratio )
{
	CValueArray args(12);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_textureprojection;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= (LONG)in_delta;
	args[i++]= (LONG)in_axesfilter;
	args[i++]= in_pivot;
	args[i++]= in_pivotu;
	args[i++]= in_pivotv;
	args[i++]= in_pivotw;
	args[i++]= in_aspectratio;

	CStatus st = gApp.ExecuteCommand( L"RotateUVW", args, retval );

	return;
}

// RotationTool
void RotationTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"RotationTool", args, retval );

	return;
}

// RunPerformanceTest
void RunPerformanceTest( const CValue& /*number*/ in_type, const CValue& /*undefined*/ in_filename, const CValue& /*number*/ in_duration )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_type;
	args[i++]= in_filename;
	args[i++]= in_duration;

	CStatus st = gApp.ExecuteCommand( L"RunPerformanceTest", args, retval );

	return;
}

// SampleParameterValues
CValue SampleParameterValues( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_samplingmethod, const CValue& /*number*/ in_nbsamples, const CValue& /*number*/ in_percentage, bool in_relative, const CValue& /*number*/ in_seed, const CString& in_capturemethod, const CString& in_capturename, const CValue& /*number*/ in_capturesize, bool in_retrieveresult, const CValue& /*number*/ in_resultid )
{
	CValueArray args(11);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_samplingmethod;
	args[i++]= in_nbsamples;
	args[i++]= in_percentage;
	args[i++]= in_relative;
	args[i++]= in_seed;
	args[i++]= in_capturemethod;
	args[i++]= in_capturename;
	args[i++]= in_capturesize;
	args[i++]= in_retrieveresult;
	args[i++]= in_resultid;

	CStatus st = gApp.ExecuteCommand( L"SampleParameterValues", args, retval );

	return retval;
}

// SaveActionKey
void SaveActionKey( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_time, const CValue& /*undefined*/ in_value, const CValue& /*undefined*/ in_tolerance, const CString& in_source, const CValue& /*number*/ in_with_autokey_behavior )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;
	args[i++]= in_value;
	args[i++]= in_tolerance;
	args[i++]= in_source;
	args[i++]= in_with_autokey_behavior;

	CStatus st = gApp.ExecuteCommand( L"SaveActionKey", args, retval );

	return;
}

// SaveActor
CValue SaveActor( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SaveActor", args, retval );

	return retval;
}

// SaveDeformKey
CValue SaveDeformKey( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_clustershapekey )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_clustershapekey;

	CStatus st = gApp.ExecuteCommand( L"SaveDeformKey", args, retval );

	return retval;
}

// SaveKey
void SaveKey( const CString& in_inputobjs, const CValue& /*undefined*/ in_time, const CValue& /*undefined*/ in_value, const CValue& /*undefined*/ in_tolerance, const CString& in_source, const CValue& /*number*/ in_with_autokey_behavior )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;
	args[i++]= in_value;
	args[i++]= in_tolerance;
	args[i++]= in_source;
	args[i++]= in_with_autokey_behavior;

	CStatus st = gApp.ExecuteCommand( L"SaveKey", args, retval );

	return;
}

// SaveKeyAtMultipleFrames
void SaveKeyAtMultipleFrames( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SaveKeyAtMultipleFrames", args, retval );

	return;
}

// SaveKeyOnPath
void SaveKeyOnPath( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_time, const CValue& /*undefined*/ in_x, const CValue& /*undefined*/ in_y, const CValue& /*undefined*/ in_z, bool in_locked )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_time;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= in_locked;

	CStatus st = gApp.ExecuteCommand( L"SaveKeyOnPath", args, retval );

	return;
}

// SaveOffsetKeys
void SaveOffsetKeys( const CValue& /*undefined*/ in_inputobjs, siEffectInputType in_type, const CValue& /*undefined*/ in_time )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_type;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"SaveOffsetKeys", args, retval );

	return;
}

// SavePaintClip
void SavePaintClip( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"SavePaintClip", args, retval );

	return;
}

// SavePaintShapeKey
void SavePaintShapeKey( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_time )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"SavePaintShapeKey", args, retval );

	return;
}

// SavePivotKey
void SavePivotKey( const CValue& /*undefined*/ in_inputobjs, siTransformFilter in_type, const CValue& /*undefined*/ in_time )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_type;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"SavePivotKey", args, retval );

	return;
}

// SavePreset
CValue SavePreset( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_presetname, const CString& in_presetpath, siPresetLocation in_presetlocation, bool in_overwrite, bool in_createthumbnail, const CString& in_description )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_presetname;
	args[i++]= in_presetpath;
	args[i++]= (LONG)in_presetlocation;
	args[i++]= in_overwrite;
	args[i++]= in_createthumbnail;
	args[i++]= in_description;

	CStatus st = gApp.ExecuteCommand( L"SavePreset", args, retval );

	return retval;
}

// SaveRegionAs
void SaveRegionAs( const CString& in_filename )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"SaveRegionAs", args, retval );

	return;
}

// SaveScene
void SaveScene( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SaveScene", args, retval );

	return;
}

// SaveSceneAs
void SaveSceneAs( const CValue& /*undefined*/ in_scenefilename, bool in_copyextfiles )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_scenefilename;
	args[i++]= in_copyextfiles;

	CStatus st = gApp.ExecuteCommand( L"SaveSceneAs", args, retval );

	return;
}

// SaveSelectedPaintClipFrames
void SaveSelectedPaintClipFrames( const CValue& /*undefined*/ in_paintclip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_paintclip;

	CStatus st = gApp.ExecuteCommand( L"SaveSelectedPaintClipFrames", args, retval );

	return;
}

// SaveShapeKey
CValue SaveShapeKey( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_compound, const CValue& /*undefined*/ in_track, const CValue& /*undefined*/ in_time, const CValue& /*undefined*/ in_duration, const CValue& /*undefined*/ in_mode, const CValue& /*undefined*/ in_name, const CValue& /*undefined*/ in_overwrite, siShapeReferenceMode in_refmode )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_compound;
	args[i++]= in_track;
	args[i++]= in_time;
	args[i++]= in_duration;
	args[i++]= in_mode;
	args[i++]= in_name;
	args[i++]= in_overwrite;
	args[i++]= (LONG)in_refmode;

	CStatus st = gApp.ExecuteCommand( L"SaveShapeKey", args, retval );

	return retval;
}

// Scale
void Scale( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z, siDeltaMode in_delta, siRefMode in_refmode, siCenterMode in_center, siAxesFilter in_axesfilter, const CValue& /*undefined*/ in_reference, bool in_splitlocalcomponents, bool in_proptagonly, bool in_pivot, const CValue& /*number*/ in_pivotx, const CValue& /*number*/ in_pivoty, const CValue& /*number*/ in_pivotz, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(16);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= (LONG)in_delta;
	args[i++]= (LONG)in_refmode;
	args[i++]= (LONG)in_center;
	args[i++]= (LONG)in_axesfilter;
	args[i++]= in_reference;
	args[i++]= in_splitlocalcomponents;
	args[i++]= in_proptagonly;
	args[i++]= in_pivot;
	args[i++]= in_pivotx;
	args[i++]= in_pivoty;
	args[i++]= in_pivotz;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"Scale", args, retval );

	return;
}

// Scale2DShape
void Scale2DShape( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_x;
	args[i++]= in_y;

	CStatus st = gApp.ExecuteCommand( L"Scale2DShape", args, retval );

	return;
}

// ScaleAndOffset
void ScaleAndOffset( const CValue& /*undefined*/ in_inputobjs, siSequenceType in_type, const CValue& /*number*/ in_offset, const CValue& /*number*/ in_scale, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, const CValue& /*number*/ in_pivot, bool in_ripple, siSequenceMode in_mode, bool in_discrete_time )
{
	CValueArray args(10);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_type;
	args[i++]= in_offset;
	args[i++]= in_scale;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_pivot;
	args[i++]= in_ripple;
	args[i++]= (LONG)in_mode;
	args[i++]= in_discrete_time;

	CStatus st = gApp.ExecuteCommand( L"ScaleAndOffset", args, retval );

	return;
}

// ScaleUVW
void ScaleUVW( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_textureprojection, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z, siDeltaMode in_delta, siUVWAxesFilter in_axesfilter, bool in_pivot, const CValue& /*number*/ in_pivotu, const CValue& /*number*/ in_pivotv, const CValue& /*number*/ in_pivotw )
{
	CValueArray args(11);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_textureprojection;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= (LONG)in_delta;
	args[i++]= (LONG)in_axesfilter;
	args[i++]= in_pivot;
	args[i++]= in_pivotu;
	args[i++]= in_pivotv;
	args[i++]= in_pivotw;

	CStatus st = gApp.ExecuteCommand( L"ScaleUVW", args, retval );

	return;
}

// ScalingTool
void ScalingTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"ScalingTool", args, retval );

	return;
}

// SceneRefresh
void SceneRefresh( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SceneRefresh", args, retval );

	return;
}

// SearchAndReplacePath
void SearchAndReplacePath( const CString& in_category, const CString& in_oldstring, const CString& in_newstring, bool in_casesensitive )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_category;
	args[i++]= in_oldstring;
	args[i++]= in_newstring;
	args[i++]= in_casesensitive;

	CStatus st = gApp.ExecuteCommand( L"SearchAndReplacePath", args, retval );

	return;
}

// SelectActionsFromObjects
CValue SelectActionsFromObjects( const CValue& /*undefined*/ in_inputobjs, bool in_consider_time, const CValue& /*undefined*/ in_time, bool in_keep_compound_actions, bool in_keep_inactive_actions )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_consider_time;
	args[i++]= in_time;
	args[i++]= in_keep_compound_actions;
	args[i++]= in_keep_inactive_actions;

	CStatus st = gApp.ExecuteCommand( L"SelectActionsFromObjects", args, retval );

	return retval;
}

// SelectAdjacent
void SelectAdjacent( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_selfilter, bool in_extendselection )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_selfilter;
	args[i++]= in_extendselection;

	CStatus st = gApp.ExecuteCommand( L"SelectAdjacent", args, retval );

	return;
}

// SelectAll
CValue SelectAll( const CString& in_affectselectionlist, bool in_checkobjectselectability )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_affectselectionlist;
	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"SelectAll", args, retval );

	return retval;
}

// SelectAllHelperBones
void SelectAllHelperBones( const CValue& /*undefined*/ in_rig )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_rig;

	CStatus st = gApp.ExecuteCommand( L"SelectAllHelperBones", args, retval );

	return;
}

// SelectAllUsingFilter
CValue SelectAllUsingFilter( const CValue& /*undefined*/ in_selfilter, siComponentVisibilityMode in_checkcomponentvisibility, bool in_affectselectionlist, bool in_checkobjectselectability )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_selfilter;
	args[i++]= (LONG)in_checkcomponentvisibility;
	args[i++]= in_affectselectionlist;
	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"SelectAllUsingFilter", args, retval );

	return retval;
}

// SelectBoundaryEdgeLoop
void SelectBoundaryEdgeLoop( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SelectBoundaryEdgeLoop", args, retval );

	return;
}

// SelectBranch
void SelectBranch( const CValue& /*undefined*/ in_checkobjectselectability )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"SelectBranch", args, retval );

	return;
}

// SelectChain
void SelectChain( const CValue& /*undefined*/ in_checkobjectselectability )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"SelectChain", args, retval );

	return;
}

// SelectChildNodes
CValue SelectChildNodes( const CString& in_selectionlist, const CValue& /*undefined*/ in_affectselectionlist, bool in_checkobjectselectability )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;
	args[i++]= in_affectselectionlist;
	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"SelectChildNodes", args, retval );

	return retval;
}

// SelectConstrainedObjects
void SelectConstrainedObjects( const CValue& /*undefined*/ in_constrainingobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_constrainingobj;

	CStatus st = gApp.ExecuteCommand( L"SelectConstrainedObjects", args, retval );

	return;
}

// SelectConstrainingObjects
void SelectConstrainingObjects( const CValue& /*undefined*/ in_constrainedobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_constrainedobj;

	CStatus st = gApp.ExecuteCommand( L"SelectConstrainingObjects", args, retval );

	return;
}

// SelectDeformersFromEnvelope
void SelectDeformersFromEnvelope( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SelectDeformersFromEnvelope", args, retval );

	return;
}

// SelectEdgeFilter
void SelectEdgeFilter( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SelectEdgeFilter", args, retval );

	return;
}

// SelectEdgeLoop
void SelectEdgeLoop( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SelectEdgeLoop", args, retval );

	return;
}

// SelectExternalConstraints
CValue SelectExternalConstraints( const CValue& /*undefined*/ in_inputobj, const CString& in_operation )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_operation;

	CStatus st = gApp.ExecuteCommand( L"SelectExternalConstraints", args, retval );

	return retval;
}

// SelectFilter
void SelectFilter( const CValue& /*undefined*/ in_selfilter )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selfilter;

	CStatus st = gApp.ExecuteCommand( L"SelectFilter", args, retval );

	return;
}

// SelectGeneratorInputs
void SelectGeneratorInputs( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SelectGeneratorInputs", args, retval );

	return;
}

// SelectGeometryComponents
void SelectGeometryComponents( const CValue& /*undefined*/ in_selectionlist )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;

	CStatus st = gApp.ExecuteCommand( L"SelectGeometryComponents", args, retval );

	return;
}

// SelectMembers
CValue SelectMembers( const CValue& /*undefined*/ in_selectionlist, bool in_affectselectionlist, bool in_checkobjectselectability )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;
	args[i++]= in_affectselectionlist;
	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"SelectMembers", args, retval );

	return retval;
}

// SelectModel
void SelectModel( const CValue& /*undefined*/ in_checkobjectselectability )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"SelectModel", args, retval );

	return;
}

// SelectNeighborObj
void SelectNeighborObj( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_navigdirection, const CString& in_hierarchylevel, bool in_addtoselection )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_navigdirection;
	args[i++]= in_hierarchylevel;
	args[i++]= in_addtoselection;

	CStatus st = gApp.ExecuteCommand( L"SelectNeighborObj", args, retval );

	return;
}

// SelectNSidedPolygons
void SelectNSidedPolygons( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_value, bool in_exactedgecount )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_value;
	args[i++]= in_exactedgecount;

	CStatus st = gApp.ExecuteCommand( L"SelectNSidedPolygons", args, retval );

	return;
}

// SelectObj
void SelectObj( const CValue& /*undefined*/ in_selectionlist, const CString& in_hierarchylevel, bool in_checkobjectselectability )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;
	args[i++]= in_hierarchylevel;
	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"SelectObj", args, retval );

	return;
}

// SelectObjectFilter
void SelectObjectFilter( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SelectObjectFilter", args, retval );

	return;
}

// SelectObjectsFromAction
CValue SelectObjectsFromAction( const CValue& /*undefined*/ in_inputobj, bool in_onlyposeoffset )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_onlyposeoffset;

	CStatus st = gApp.ExecuteCommand( L"SelectObjectsFromAction", args, retval );

	return retval;
}

// SelectPaintClipFrames
void SelectPaintClipFrames( const CValue& /*undefined*/ in_paintclip, const CValue& /*undefined*/ in_frames )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_paintclip;
	args[i++]= in_frames;

	CStatus st = gApp.ExecuteCommand( L"SelectPaintClipFrames", args, retval );

	return;
}

// SelectPaintShapePoint
void SelectPaintShapePoint( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_indices )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_indices;

	CStatus st = gApp.ExecuteCommand( L"SelectPaintShapePoint", args, retval );

	return;
}

// SelectParallelEdgeLoop
void SelectParallelEdgeLoop( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SelectParallelEdgeLoop", args, retval );

	return;
}

// SelectPolygonFilter
void SelectPolygonFilter( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SelectPolygonFilter", args, retval );

	return;
}

// SelectSampledPointFilter
void SelectSampledPointFilter( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SelectSampledPointFilter", args, retval );

	return;
}

// SelectShapeKey
CValue SelectShapeKey( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_sourceobj, siShapeReferenceMode in_refmode, bool in_relational, bool in_applykeys )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_sourceobj;
	args[i++]= (LONG)in_refmode;
	args[i++]= in_relational;
	args[i++]= in_applykeys;

	CStatus st = gApp.ExecuteCommand( L"SelectShapeKey", args, retval );

	return retval;
}

// SelectTree
void SelectTree( const CValue& /*undefined*/ in_checkobjectselectability )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"SelectTree", args, retval );

	return;
}

// SelectUVRow
void SelectUVRow( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SelectUVRow", args, retval );

	return;
}

// SelectVertexFilter
void SelectVertexFilter( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SelectVertexFilter", args, retval );

	return;
}

// Sequence
void Sequence( const CValue& /*undefined*/ in_inputobjs, siSequenceType in_type, const CValue& /*number*/ in_offset, const CValue& /*number*/ in_scale, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, const CValue& /*number*/ in_pivot, siSequenceMode in_mode )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_type;
	args[i++]= in_offset;
	args[i++]= in_scale;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_pivot;
	args[i++]= (LONG)in_mode;

	CStatus st = gApp.ExecuteCommand( L"Sequence", args, retval );

	return;
}

// Set2DShapeRotation
void Set2DShapeRotation( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_angle )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_angle;

	CStatus st = gApp.ExecuteCommand( L"Set2DShapeRotation", args, retval );

	return;
}

// SetActiveElements
void SetActiveElements( const CValue& /*undefined*/ in_selectionlist )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;

	CStatus st = gApp.ExecuteCommand( L"SetActiveElements", args, retval );

	return;
}

// SetAndToggleActiveElements
void SetAndToggleActiveElements( const CValue& /*undefined*/ in_selectionlist )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;

	CStatus st = gApp.ExecuteCommand( L"SetAndToggleActiveElements", args, retval );

	return;
}

// SetAndToggleSelection
void SetAndToggleSelection( const CValue& /*undefined*/ in_selectionlist, const CString& in_hierarchylevel, bool in_checkobjectselectability )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;
	args[i++]= in_hierarchylevel;
	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"SetAndToggleSelection", args, retval );

	return;
}

// SetBoneMaxRotLimit
void SetBoneMaxRotLimit( const CValue& /*undefined*/ in_inputobjs, bool in_domin )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_domin;

	CStatus st = gApp.ExecuteCommand( L"SetBoneMaxRotLimit", args, retval );

	return;
}

// SetBoneMinRotLimit
void SetBoneMinRotLimit( const CValue& /*undefined*/ in_inputobjs, bool in_domin )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_domin;

	CStatus st = gApp.ExecuteCommand( L"SetBoneMinRotLimit", args, retval );

	return;
}

// SetBoneRotationToPrefRotation
void SetBoneRotationToPrefRotation( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SetBoneRotationToPrefRotation", args, retval );

	return;
}

// SetCOG
CValue SetCOG( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SetCOG", args, retval );

	return retval;
}

// SetCollisionOnly
CValue SetCollisionOnly( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SetCollisionOnly", args, retval );

	return retval;
}

// SetCopyShapeCluster
CValue SetCopyShapeCluster( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SetCopyShapeCluster", args, retval );

	return retval;
}

// SetCurrentEnvironment
void SetCurrentEnvironment( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"SetCurrentEnvironment", args, retval );

	return;
}

// SetCurrentLayer
void SetCurrentLayer( const CValue& /*undefined*/ in_value )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SetCurrentLayer", args, retval );

	return;
}

// SetCurrentMaterialLibrary
void SetCurrentMaterialLibrary( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"SetCurrentMaterialLibrary", args, retval );

	return;
}

// SetCurrentParType
void SetCurrentParType( const CValue& /*undefined*/ in_value )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SetCurrentParType", args, retval );

	return;
}

// SetCurrentPass
void SetCurrentPass( const CValue& /*undefined*/ in_value )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SetCurrentPass", args, retval );

	return;
}

// SetCurrentReferencePlane
void SetCurrentReferencePlane( const CValue& /*undefined*/ in_value )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SetCurrentReferencePlane", args, retval );

	return;
}

// SetCurveType
void SetCurveType( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_interpolation )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_interpolation;

	CStatus st = gApp.ExecuteCommand( L"SetCurveType", args, retval );

	return;
}

// SetDefault
CValue SetDefault( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SetDefault", args, retval );

	return retval;
}

// SetDefaultWeight
void SetDefaultWeight( const CValue& /*undefined*/ in_inputobj, siWeightPresets in_mode, const CValue& /*number*/ in_extra )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= (LONG)in_mode;
	args[i++]= in_extra;

	CStatus st = gApp.ExecuteCommand( L"SetDefaultWeight", args, retval );

	return;
}

// SetDisplayMode
void SetDisplayMode( const CValue& /*undefined*/ in_inputobjs, const CString& in_displaymode )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_displaymode;

	CStatus st = gApp.ExecuteCommand( L"SetDisplayMode", args, retval );

	return;
}

// SetDropMessage
void SetDropMessage( const CString& in_message )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_message;

	CStatus st = gApp.ExecuteCommand( L"SetDropMessage", args, retval );

	return;
}

// SetEnvelopeRefPoses
CValue SetEnvelopeRefPoses( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SetEnvelopeRefPoses", args, retval );

	return retval;
}

// SetEventObstacle
void SetEventObstacle( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_source )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_source;

	CStatus st = gApp.ExecuteCommand( L"SetEventObstacle", args, retval );

	return;
}

// SetExpr
void SetExpr( const CValue& /*undefined*/ in_inputobj, const CString& in_exprstr, const CRef& /*object*/ in_exprobj )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_exprstr;
	args[i++]= in_exprobj;

	CStatus st = gApp.ExecuteCommand( L"SetExpr", args, retval );

	return;
}

// SetFoot
CValue SetFoot( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SetFoot", args, retval );

	return retval;
}

// SetFxOpAttrib
void SetFxOpAttrib( const CValue& /*undefined*/ in_op, const CString& in_attrib, const CValue& /*undefined*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_op;
	args[i++]= in_attrib;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SetFxOpAttrib", args, retval );

	return;
}

// SetFxOpCnxFlag
void SetFxOpCnxFlag( const CValue& /*undefined*/ in_fxop, const CString& in_connection, const CString& in_flagname, bool in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_fxop;
	args[i++]= in_connection;
	args[i++]= in_flagname;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SetFxOpCnxFlag", args, retval );

	return;
}

// SetFxOpPos
void SetFxOpPos( const CValue& /*undefined*/ in_fxop, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_fxop;
	args[i++]= in_x;
	args[i++]= in_y;

	CStatus st = gApp.ExecuteCommand( L"SetFxOpPos", args, retval );

	return;
}

// SetFxTrackerSearchArea
void SetFxTrackerSearchArea( const CValue& /*undefined*/ in_fxop, const CValue& /*undefined*/ in_time, const CValue& /*number*/ in_tracker, const CValue& /*number*/ in_x1, const CValue& /*number*/ in_y1, const CValue& /*number*/ in_x2, const CValue& /*number*/ in_y2 )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_fxop;
	args[i++]= in_time;
	args[i++]= in_tracker;
	args[i++]= in_x1;
	args[i++]= in_y1;
	args[i++]= in_x2;
	args[i++]= in_y2;

	CStatus st = gApp.ExecuteCommand( L"SetFxTrackerSearchArea", args, retval );

	return;
}

// SetFxTrackerTemplateArea
void SetFxTrackerTemplateArea( const CValue& /*undefined*/ in_fxop, const CValue& /*undefined*/ in_time, const CValue& /*number*/ in_tracker, const CValue& /*number*/ in_x1, const CValue& /*number*/ in_y1, const CValue& /*number*/ in_x2, const CValue& /*number*/ in_y2 )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_fxop;
	args[i++]= in_time;
	args[i++]= in_tracker;
	args[i++]= in_x1;
	args[i++]= in_y1;
	args[i++]= in_x2;
	args[i++]= in_y2;

	CStatus st = gApp.ExecuteCommand( L"SetFxTrackerTemplateArea", args, retval );

	return;
}

// SetFxTrackerTrackedPoint
void SetFxTrackerTrackedPoint( const CValue& /*undefined*/ in_fxop, const CValue& /*undefined*/ in_time, const CValue& /*number*/ in_tracker, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_fxop;
	args[i++]= in_time;
	args[i++]= in_tracker;
	args[i++]= in_x;
	args[i++]= in_y;

	CStatus st = gApp.ExecuteCommand( L"SetFxTrackerTrackedPoint", args, retval );

	return;
}

// SetHelperTriggerOri
void SetHelperTriggerOri( const CValue& /*undefined*/ in_helper, const CValue& /*number*/ in_index )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_helper;
	args[i++]= in_index;

	CStatus st = gApp.ExecuteCommand( L"SetHelperTriggerOri", args, retval );

	return;
}

// SetHome
CValue SetHome( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SetHome", args, retval );

	return retval;
}

// SetImageSource
void SetImageSource( const CValue& /*undefined*/ in_source, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SetImageSource", args, retval );

	return;
}

// SetInstanceDataValue
void SetInstanceDataValue( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_inputobj;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SetInstanceDataValue", args, retval );

	return;
}

// SetKey
void SetKey( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_time, const CValue& /*undefined*/ in_value, const CValue& /*number*/ in_tolerance, const CValue& /*undefined*/ in_prevvalue, bool in_haskey )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_time;
	args[i++]= in_value;
	args[i++]= in_tolerance;
	args[i++]= in_prevvalue;
	args[i++]= in_haskey;

	CStatus st = gApp.ExecuteCommand( L"SetKey", args, retval );

	return;
}

// SetKeyUsingMarkingSet
void SetKeyUsingMarkingSet( const CValue& /*undefined*/ in_inputobjs, bool in_branch )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_branch;

	CStatus st = gApp.ExecuteCommand( L"SetKeyUsingMarkingSet", args, retval );

	return;
}

// SetMappingRule
void SetMappingRule( const CValue& /*undefined*/ in_mappingtemplate, const CString& in_from, const CString& in_to, const CValue& /*number*/ in_index, const CValue& /*number*/ in_active, const CValue& /*number*/ in_poseoffset )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_mappingtemplate;
	args[i++]= in_from;
	args[i++]= in_to;
	args[i++]= in_index;
	args[i++]= in_active;
	args[i++]= in_poseoffset;

	CStatus st = gApp.ExecuteCommand( L"SetMappingRule", args, retval );

	return;
}

// SetMarking
void SetMarking( const CValue& /*undefined*/ in_pathlist )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_pathlist;

	CStatus st = gApp.ExecuteCommand( L"SetMarking", args, retval );

	return;
}

// SetNeutralPose
void SetNeutralPose( const CValue& /*undefined*/ in_inputobjs, siTransformFilter in_type, bool in_reset )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_type;
	args[i++]= in_reset;

	CStatus st = gApp.ExecuteCommand( L"SetNeutralPose", args, retval );

	return;
}

// SetPaintBrush
void SetPaintBrush( const CString& in_brush )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_brush;

	CStatus st = gApp.ExecuteCommand( L"SetPaintBrush", args, retval );

	return;
}

// SetPaintColor
void SetPaintColor( const CValue& /*number*/ in_red, const CValue& /*number*/ in_green, const CValue& /*number*/ in_blue, const CValue& /*number*/ in_alpha )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_red;
	args[i++]= in_green;
	args[i++]= in_blue;
	args[i++]= in_alpha;

	CStatus st = gApp.ExecuteCommand( L"SetPaintColor", args, retval );

	return;
}

// SetPaintMaskStore
void SetPaintMaskStore( const CValue& /*undefined*/ in_fxop )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_fxop;

	CStatus st = gApp.ExecuteCommand( L"SetPaintMaskStore", args, retval );

	return;
}

// SetPaintMergeStore
void SetPaintMergeStore( const CValue& /*undefined*/ in_fxop )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_fxop;

	CStatus st = gApp.ExecuteCommand( L"SetPaintMergeStore", args, retval );

	return;
}

// SetPaintShapeHandle
void SetPaintShapeHandle( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_point, const CValue& /*number*/ in_lhx, const CValue& /*number*/ in_lhy, const CValue& /*number*/ in_lrx, const CValue& /*number*/ in_lry )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_point;
	args[i++]= in_lhx;
	args[i++]= in_lhy;
	args[i++]= in_lrx;
	args[i++]= in_lry;

	CStatus st = gApp.ExecuteCommand( L"SetPaintShapeHandle", args, retval );

	return;
}

// SetPaintStore
void SetPaintStore( const CValue& /*undefined*/ in_fxop )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_fxop;

	CStatus st = gApp.ExecuteCommand( L"SetPaintStore", args, retval );

	return;
}

// SetParameterColor
void SetParameterColor( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_value )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SetParameterColor", args, retval );

	return;
}

// SetParticleInitState
void SetParticleInitState( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_overwrite )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_overwrite;

	CStatus st = gApp.ExecuteCommand( L"SetParticleInitState", args, retval );

	return;
}

// SetParticleType
void SetParticleType( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_source )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_source;

	CStatus st = gApp.ExecuteCommand( L"SetParticleType", args, retval );

	return;
}

// SetParTypeInstanceGroup
void SetParTypeInstanceGroup( const CValue& /*undefined*/ in_partype, const CValue& /*undefined*/ in_group )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_partype;
	args[i++]= in_group;

	CStatus st = gApp.ExecuteCommand( L"SetParTypeInstanceGroup", args, retval );

	return;
}

// SetReference
void SetReference( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_value, const CValue& /*undefined*/ in_time )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_value;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"SetReference", args, retval );

	return;
}

// SetReferencePlane
void SetReferencePlane( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"SetReferencePlane", args, retval );

	return;
}

// SetRefPlaneFromSymmetrizePolygonOp
CValue SetRefPlaneFromSymmetrizePolygonOp( const CString& in_refplane, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_refplane;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SetRefPlaneFromSymmetrizePolygonOp", args, retval );

	return retval;
}

// SetRelativeValues
void SetRelativeValues( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"SetRelativeValues", args, retval );

	return;
}

// SetResolutionOfRefModels
void SetResolutionOfRefModels( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_resolution )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_resolution;

	CStatus st = gApp.ExecuteCommand( L"SetResolutionOfRefModels", args, retval );

	return;
}

// SetRGB
void SetRGB( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SetRGB", args, retval );

	return;
}

// SetRigidBodyInitState
void SetRigidBodyInitState( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SetRigidBodyInitState", args, retval );

	return;
}

// SetSelFilter
void SetSelFilter( const CValue& /*undefined*/ in_selfilter )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selfilter;

	CStatus st = gApp.ExecuteCommand( L"SetSelFilter", args, retval );

	return;
}

// SetSymmetrizePolygonOpRefPlane
CValue SetSymmetrizePolygonOpRefPlane( const CValue& /*undefined*/ in_inputobjs, const CString& in_refplane )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_refplane;

	CStatus st = gApp.ExecuteCommand( L"SetSymmetrizePolygonOpRefPlane", args, retval );

	return retval;
}

// SetThumbnail
void SetThumbnail( const CValue& /*undefined*/ in_inputobjs, const CString& in_pathname )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_pathname;

	CStatus st = gApp.ExecuteCommand( L"SetThumbnail", args, retval );

	return;
}

// SetThumbnailFromRegion
void SetThumbnailFromRegion( const CValue& /*undefined*/ in_inputobjs, const CString& in_filename, bool in_overwrite )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_filename;
	args[i++]= in_overwrite;

	CStatus st = gApp.ExecuteCommand( L"SetThumbnailFromRegion", args, retval );

	return;
}

// SetTransformRefMode
CValue SetTransformRefMode( const CValue& /*number*/ in_mode )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_mode;

	CStatus st = gApp.ExecuteCommand( L"SetTransformRefMode", args, retval );

	return retval;
}

// SetTransientReferencePlane
void SetTransientReferencePlane( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SetTransientReferencePlane", args, retval );

	return;
}

// SetupTestScene
void SetupTestScene( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SetupTestScene", args, retval );

	return;
}

// SetUserKeyword
void SetUserKeyword( const CValue& /*undefined*/ in_inputobjs, const CString& in_keywords )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_keywords;

	CStatus st = gApp.ExecuteCommand( L"SetUserKeyword", args, retval );

	return;
}

// SetUserPref
CValue SetUserPref( const CString& in_target, const CValue& /*number*/ in_value )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SetUserPref", args, retval );

	return retval;
}

// SetValue
CValue SetValue( const CString& in_target,const CValue& /*value*/ in_value, const CValue& /*undefined*/ in_time )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_value;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"SetValue", args, retval );

	return retval;
}

// SetVisualOnly
CValue SetVisualOnly( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SetVisualOnly", args, retval );

	return retval;
}

// ShadeObject
void ShadeObject( const CValue& /*number*/ in_shademodeid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_shademodeid;

	CStatus st = gApp.ExecuteCommand( L"ShadeObject", args, retval );

	return;
}

// Show3DContextMenu
void Show3DContextMenu( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"Show3DContextMenu", args, retval );

	return;
}

// ShowConcaveEdges
CValue ShowConcaveEdges( const CValue& /*number*/ in_arg0 )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_arg0;

	CStatus st = gApp.ExecuteCommand( L"ShowConcaveEdges", args, retval );

	return retval;
}

// ShowHelperBoneTool
void ShowHelperBoneTool( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ShowHelperBoneTool", args, retval );

	return;
}

// ShowNextOGLProjection
void ShowNextOGLProjection( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ShowNextOGLProjection", args, retval );

	return;
}

// ShowPreviousOGLProjection
void ShowPreviousOGLProjection( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ShowPreviousOGLProjection", args, retval );

	return;
}

// ShowSwiftMenu
void ShowSwiftMenu( const CValue& /*number*/ in_target )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"ShowSwiftMenu", args, retval );

	return;
}

// ShowValveSourceHelp
void ShowValveSourceHelp( const CValue& /*undefined*/ in_relativepath )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_relativepath;

	CStatus st = gApp.ExecuteCommand( L"ShowValveSourceHelp", args, retval );

	return;
}

// SIAddArrayElement
CValue SIAddArrayElement( const CValue& /*undefined*/ in_object )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_object;

	CStatus st = gApp.ExecuteCommand( L"SIAddArrayElement", args, retval );

	return retval;
}

// SIAddCustomParam
void SIAddCustomParam( const CValue& /*undefined*/ in_inputobj, const CString& in_scriptname, siVariantType in_vartype, const CValue& /*undefined*/ in_defaultvalue, const CValue& /*number*/ in_minvalue, const CValue& /*number*/ in_maxvalue, const CValue& /*number*/ in_classification, const CValue& /*number*/ in_capabilities, const CValue& /*number*/ in_suggmin, const CValue& /*number*/ in_suggmax, const CString& in_paramname, const CString& in_description )
{
	CValueArray args(12);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_scriptname;
	args[i++]= (LONG)in_vartype;
	args[i++]= in_defaultvalue;
	args[i++]= in_minvalue;
	args[i++]= in_maxvalue;
	args[i++]= in_classification;
	args[i++]= in_capabilities;
	args[i++]= in_suggmin;
	args[i++]= in_suggmax;
	args[i++]= in_paramname;
	args[i++]= in_description;

	CStatus st = gApp.ExecuteCommand( L"SIAddCustomParam", args, retval );

	return;
}

// SIAddCustomParameter
void SIAddCustomParameter( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_scriptname, siVariantType in_vartype, const CValue& /*undefined*/ in_defaultvalue, const CValue& /*undefined*/ in_minvalue, const CValue& /*undefined*/ in_maxvalue, const CValue& /*number*/ in_classification, const CValue& /*undefined*/ in_capabilities, const CValue& /*undefined*/ in_suggmin, const CValue& /*undefined*/ in_suggmax, const CValue& /*undefined*/ in_paramname, const CValue& /*undefined*/ in_description )
{
	CValueArray args(12);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_scriptname;
	args[i++]= (LONG)in_vartype;
	args[i++]= in_defaultvalue;
	args[i++]= in_minvalue;
	args[i++]= in_maxvalue;
	args[i++]= in_classification;
	args[i++]= in_capabilities;
	args[i++]= in_suggmin;
	args[i++]= in_suggmax;
	args[i++]= in_paramname;
	args[i++]= in_description;

	CStatus st = gApp.ExecuteCommand( L"SIAddCustomParameter", args, retval );

	return;
}

// SIAddEntryToOverride
void SIAddEntryToOverride( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_type )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_inputobjs;
	args[i++]= in_type;

	CStatus st = gApp.ExecuteCommand( L"SIAddEntryToOverride", args, retval );

	return;
}

// SIAddImageClip
void SIAddImageClip( const CValue& /*undefined*/ in_source, const CString& in_name, const CRef& /*object*/ in_clip )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_name;
	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"SIAddImageClip", args, retval );

	return;
}

// SIAddImageOp
CValue SIAddImageOp( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_clip )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"SIAddImageOp", args, retval );

	return retval;
}

// SIAddImageSource
void SIAddImageSource( const CValue& /*undefined*/ in_filename, const CString& in_name, const CRef& /*object*/ in_source )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_source;

	CStatus st = gApp.ExecuteCommand( L"SIAddImageSource", args, retval );

	return;
}

// SIAddImageSourceOp
CValue SIAddImageSourceOp( const CValue& /*undefined*/ in_clip, const CValue& /*undefined*/ in_source )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;
	args[i++]= in_source;

	CStatus st = gApp.ExecuteCommand( L"SIAddImageSourceOp", args, retval );

	return retval;
}

// SIAddModelClip
void SIAddModelClip( const CValue& /*undefined*/ in_source, const CString& in_name, const CRef& /*object*/ in_clip )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_name;
	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"SIAddModelClip", args, retval );

	return;
}

// SIAddModelSource
void SIAddModelSource( const CString& in_filename, const CString& in_name, const CRef& /*object*/ in_source, const CValue& /*number*/ in_resolution )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_source;
	args[i++]= in_resolution;

	CStatus st = gApp.ExecuteCommand( L"SIAddModelSource", args, retval );

	return;
}

// SIAddParameterEntryToOverride
void SIAddParameterEntryToOverride( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_value, const CString& in_path )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_inputobjs;
	args[i++]= in_value;
	args[i++]= in_path;

	CStatus st = gApp.ExecuteCommand( L"SIAddParameterEntryToOverride", args, retval );

	return;
}

// SIAddParticleEvent
void SIAddParticleEvent( const CString& in_target, const CString& in_source, const CRef& /*object*/ in_output )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_source;
	args[i++]= in_output;

	CStatus st = gApp.ExecuteCommand( L"SIAddParticleEvent", args, retval );

	return;
}

// SIAddParticleForce
void SIAddParticleForce( const CString& in_target, const CString& in_inputobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"SIAddParticleForce", args, retval );

	return;
}

// SIAddParticleGoal
void SIAddParticleGoal( const CString& in_target, const CString& in_inputobj, const CRef& /*object*/ in_output )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobj;
	args[i++]= in_output;

	CStatus st = gApp.ExecuteCommand( L"SIAddParticleGoal", args, retval );

	return;
}

// SIAddPointOnCurve
void SIAddPointOnCurve( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_insertposition, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z, bool in_local, const CValue& /*number*/ in_curvetype )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_insertposition;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= in_local;
	args[i++]= in_curvetype;

	CStatus st = gApp.ExecuteCommand( L"SIAddPointOnCurve", args, retval );

	return;
}

// SIAddPointOnCurveAtEnd
void SIAddPointOnCurveAtEnd( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z, bool in_local, const CValue& /*number*/ in_curvetype )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= in_local;
	args[i++]= in_curvetype;

	CStatus st = gApp.ExecuteCommand( L"SIAddPointOnCurveAtEnd", args, retval );

	return;
}

// SIAddPointOnCurveAtStart
void SIAddPointOnCurveAtStart( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z, bool in_local, const CValue& /*number*/ in_curvetype )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= in_local;
	args[i++]= in_curvetype;

	CStatus st = gApp.ExecuteCommand( L"SIAddPointOnCurveAtStart", args, retval );

	return;
}

// SIAddProp
void SIAddProp( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_inputobjs, siPropagationType in_propagationtype, const CString& in_propertyname, const CRef& /*object*/ in_value )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_propagationtype;
	args[i++]= in_propertyname;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SIAddProp", args, retval );

	return;
}

// SIAddPropEx
void SIAddPropEx( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_property, siPropagationType in_propagationtype )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_property;
	args[i++]= (LONG)in_propagationtype;

	CStatus st = gApp.ExecuteCommand( L"SIAddPropEx", args, retval );

	return;
}

// SIAddShaderEntryToOverride
void SIAddShaderEntryToOverride( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_inputobj, const CString& in_path, const CString& in_name )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobj;
	args[i++]= in_path;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"SIAddShaderEntryToOverride", args, retval );

	return;
}

// SIAddToCluster
void SIAddToCluster( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_subcomp, const CRef& /*object*/ in_outputobj )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_subcomp;
	args[i++]= in_outputobj;

	CStatus st = gApp.ExecuteCommand( L"SIAddToCluster", args, retval );

	return;
}

// SIAddToGroup
void SIAddToGroup( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobjs, const CRef& /*object*/ in_outputobj )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;
	args[i++]= in_outputobj;

	CStatus st = gApp.ExecuteCommand( L"SIAddToGroup", args, retval );

	return;
}

// SIAddToGroupLowLevel
void SIAddToGroupLowLevel( const CRef& /*object*/ in_target, const CRef& /*object*/ in_inputobjs, const CRef& /*object*/ in_outputobj )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;
	args[i++]= in_outputobj;

	CStatus st = gApp.ExecuteCommand( L"SIAddToGroupLowLevel", args, retval );

	return;
}

// SIAppendBone
CValue SIAppendBone( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_ex, const CValue& /*number*/ in_ey, const CValue& /*number*/ in_ez, bool in_pin, bool in_alignroottofirstbone )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_ex;
	args[i++]= in_ey;
	args[i++]= in_ez;
	args[i++]= in_pin;
	args[i++]= in_alignroottofirstbone;

	CStatus st = gApp.ExecuteCommand( L"SIAppendBone", args, retval );

	return retval;
}

// SIApplyCns
CValue SIApplyCns( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_constrainedobj, const CValue& /*undefined*/ in_constrainingobj, bool in_compensation )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_constrainedobj;
	args[i++]= in_constrainingobj;
	args[i++]= in_compensation;

	CStatus st = gApp.ExecuteCommand( L"SIApplyCns", args, retval );

	return retval;
}

// SIApplyConstraint
CValue SIApplyConstraint( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_constrainedobj, const CValue& /*undefined*/ in_constrainingobj, bool in_compensation, const CValue& /*undefined*/ in_offset1x, const CValue& /*undefined*/ in_offset1y, const CValue& /*undefined*/ in_offset1z, const CValue& /*undefined*/ in_offset2x, const CValue& /*undefined*/ in_offset2y, const CValue& /*undefined*/ in_offset2z )
{
	CValueArray args(10);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_constrainedobj;
	args[i++]= in_constrainingobj;
	args[i++]= in_compensation;
	args[i++]= in_offset1x;
	args[i++]= in_offset1y;
	args[i++]= in_offset1z;
	args[i++]= in_offset2x;
	args[i++]= in_offset2y;
	args[i++]= in_offset2z;

	CStatus st = gApp.ExecuteCommand( L"SIApplyConstraint", args, retval );

	return retval;
}

// SIApplyDeformByCage
CValue SIApplyDeformByCage( const CValue& /*undefined*/ in_connectionset, bool in_interactive, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= in_interactive;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"SIApplyDeformByCage", args, retval );

	return retval;
}

// SIApplyDeformBySpine
CValue SIApplyDeformBySpine( const CValue& /*undefined*/ in_connectionset, bool in_interactive, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= in_interactive;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"SIApplyDeformBySpine", args, retval );

	return retval;
}

// SIApplyFlexEnv
CValue SIApplyFlexEnv( const CValue& /*undefined*/ in_connectionset, bool in_interactive, bool in_assignnewdeformers, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= in_interactive;
	args[i++]= in_assignnewdeformers;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"SIApplyFlexEnv", args, retval );

	return retval;
}

// SIApplyFlexEnvWithWeight
void SIApplyFlexEnvWithWeight( const CValue& /*undefined*/ in_connectionset, bool in_interactive, const CValue& /*undefined*/ in_envelopeweights, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= in_interactive;
	args[i++]= in_envelopeweights;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"SIApplyFlexEnvWithWeight", args, retval );

	return;
}

// SIApplyShader
CValue SIApplyShader( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, const CValue& /*number*/ in_propagationtype )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_propagationtype;

	CStatus st = gApp.ExecuteCommand( L"SIApplyShader", args, retval );

	return retval;
}

// SIApplyShaderToCnxPoint
CValue SIApplyShaderToCnxPoint( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, bool in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SIApplyShaderToCnxPoint", args, retval );

	return retval;
}

// SIAssembleNurbsMesh
void SIAssembleNurbsMesh( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_tolerance, bool in_copyusercluster, bool in_copymaterial, bool in_centergeomcenter, const CRef& /*object*/ in_outputobj )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_tolerance;
	args[i++]= in_copyusercluster;
	args[i++]= in_copymaterial;
	args[i++]= in_centergeomcenter;
	args[i++]= in_outputobj;

	CStatus st = gApp.ExecuteCommand( L"SIAssembleNurbsMesh", args, retval );

	return;
}

// SIAssignMaterial
void SIAssignMaterial( const CValue& /*undefined*/ in_objects, const CValue& /*undefined*/ in_material )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_objects;
	args[i++]= in_material;

	CStatus st = gApp.ExecuteCommand( L"SIAssignMaterial", args, retval );

	return;
}

// SIBasicTransferClusterPropertiesAcrossGenOp
CValue SIBasicTransferClusterPropertiesAcrossGenOp( const CValue& /*undefined*/ in_operator, const CValue& /*undefined*/ in_cluster, const CValue& /*undefined*/ in_clusterpropstotransfer, const CString& in_clusterpropertyname )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_cluster;
	args[i++]= in_clusterpropstotransfer;
	args[i++]= in_clusterpropertyname;

	CStatus st = gApp.ExecuteCommand( L"SIBasicTransferClusterPropertiesAcrossGenOp", args, retval );

	return retval;
}

// SIBasicTransferClustersAcrossGenOp
CValue SIBasicTransferClustersAcrossGenOp( const CValue& /*undefined*/ in_operator, const CValue& /*undefined*/ in_object, const CValue& /*undefined*/ in_clusterstotransfer, const CString& in_clustername )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_object;
	args[i++]= in_clusterstotransfer;
	args[i++]= in_clustername;

	CStatus st = gApp.ExecuteCommand( L"SIBasicTransferClustersAcrossGenOp", args, retval );

	return retval;
}

// SICameraTextureProj
void SICameraTextureProj( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_propertyname, const CValue& /*undefined*/ in_primobjtexture, const CRef& /*object*/ in_3dobjcamera )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_propertyname;
	args[i++]= in_primobjtexture;
	args[i++]= in_3dobjcamera;

	CStatus st = gApp.ExecuteCommand( L"SICameraTextureProj", args, retval );

	return;
}

// SIConnectShaderToCnxPoint
CValue SIConnectShaderToCnxPoint( const CValue& /*undefined*/ in_source, const CValue& /*undefined*/ in_inputobjs, bool in_deleteonlastdisconnection )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_source;
	args[i++]= in_inputobjs;
	args[i++]= in_deleteonlastdisconnection;

	CStatus st = gApp.ExecuteCommand( L"SIConnectShaderToCnxPoint", args, retval );

	return retval;
}

// SIConnectShaderWithBlendsInsp
void SIConnectShaderWithBlendsInsp( const CString& in_presetobj, const CString& in_mixer, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_mixer;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SIConnectShaderWithBlendsInsp", args, retval );

	return;
}

// SIConnectToProjection
void SIConnectToProjection( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_propertyname, const CValue& /*undefined*/ in_textureprojection )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_propertyname;
	args[i++]= in_textureprojection;

	CStatus st = gApp.ExecuteCommand( L"SIConnectToProjection", args, retval );

	return;
}

// SIConnectToSupport
void SIConnectToSupport( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_propertyname, const CValue& /*undefined*/ in_primobjtexture, const CRef& /*object*/ in_3dobjtexture )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_propertyname;
	args[i++]= in_primobjtexture;
	args[i++]= in_3dobjtexture;

	CStatus st = gApp.ExecuteCommand( L"SIConnectToSupport", args, retval );

	return;
}

// SIConvert
void SIConvert( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_source, const CRef& /*object*/ in_createdobj, const CRef& /*object*/ in_outputobj )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_source;
	args[i++]= in_createdobj;
	args[i++]= in_outputobj;

	CStatus st = gApp.ExecuteCommand( L"SIConvert", args, retval );

	return;
}

// SICreateCluster
void SICreateCluster( const CString& in_type, const CString& in_name, const CValue& /*undefined*/ in_3dobject, const CValue& /*number*/ in_mode, const CRef& /*object*/ in_value )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_type;
	args[i++]= in_name;
	args[i++]= in_3dobject;
	args[i++]= in_mode;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SICreateCluster", args, retval );

	return;
}

// SICreateCurve
CValue SICreateCurve( const CString& in_name, const CValue& /*number*/ in_degree, const CValue& /*number*/ in_curvetype )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_name;
	args[i++]= in_degree;
	args[i++]= in_curvetype;

	CStatus st = gApp.ExecuteCommand( L"SICreateCurve", args, retval );

	return retval;
}

// SICreateEmptyPartition
void SICreateEmptyPartition( const CValue& /*undefined*/ in_target, const CString& in_name, siPartitionType in_type, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_name;
	args[i++]= (LONG)in_type;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SICreateEmptyPartition", args, retval );

	return;
}

// SICreateGroup
CValue SICreateGroup( const CString& in_name, const CValue& /*undefined*/ in_parent )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_name;
	args[i++]= in_parent;

	CStatus st = gApp.ExecuteCommand( L"SICreateGroup", args, retval );

	return retval;
}

// SICreateImageClip
void SICreateImageClip( const CValue& /*undefined*/ in_filename, const CString& in_name, const CRef& /*object*/ in_clip )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"SICreateImageClip", args, retval );

	return;
}

// SICreateImageClip2
CValue SICreateImageClip2( const CValue& /*undefined*/ in_filename, const CString& in_name, const CRef& /*object*/ in_clip )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"SICreateImageClip2", args, retval );

	return retval;
}

// SICreateLayer
void SICreateLayer( const CString& in_presetobj, const CString& in_name, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SICreateLayer", args, retval );

	return;
}

// SICreateMaterial
CValue SICreateMaterial( const CValue& /*undefined*/ in_presetobj, const CString& in_name, const CValue& /*undefined*/ in_library, const CValue& /*undefined*/ in_objects, bool in_assigntolist, bool in_initialize )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_library;
	args[i++]= in_objects;
	args[i++]= in_assigntolist;
	args[i++]= in_initialize;

	CStatus st = gApp.ExecuteCommand( L"SICreateMaterial", args, retval );

	return retval;
}

// SICreateMaterialDD
CValue SICreateMaterialDD( const CValue& /*undefined*/ in_presetobj, const CString& in_name, const CValue& /*undefined*/ in_library, const CValue& /*undefined*/ in_objects, bool in_assigntolist, bool in_initialize )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_library;
	args[i++]= in_objects;
	args[i++]= in_assigntolist;
	args[i++]= in_initialize;

	CStatus st = gApp.ExecuteCommand( L"SICreateMaterialDD", args, retval );

	return retval;
}

// SICreateModel
void SICreateModel( const CValue& /*undefined*/ in_inputobj, const CString& in_name, const CValue& /*undefined*/ in_parent, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SICreateModel", args, retval );

	return;
}

// SICreateModelClip
void SICreateModelClip( const CValue& /*undefined*/ in_filename, const CString& in_name, const CRef& /*object*/ in_clip )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"SICreateModelClip", args, retval );

	return;
}

// SICreatePartition
void SICreatePartition( const CString& in_target, const CString& in_name, const CString& in_inputobjs, const CRef& /*object*/ in_value )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_name;
	args[i++]= in_inputobjs;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SICreatePartition", args, retval );

	return;
}

// SICreatePass
void SICreatePass( const CString& in_presetobj, const CString& in_name, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SICreatePass", args, retval );

	return;
}

// SICreateRefPlane
CValue SICreateRefPlane( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SICreateRefPlane", args, retval );

	return retval;
}

// SICreateSkeleton
CValue SICreateSkeleton( const CValue& /*number*/ in_rx, const CValue& /*number*/ in_ry, const CValue& /*number*/ in_rz, const CValue& /*number*/ in_ex, const CValue& /*number*/ in_ey, const CValue& /*number*/ in_ez, const CValue& /*number*/ in_nx, const CValue& /*number*/ in_ny, const CValue& /*number*/ in_nz, const CValue& /*number*/ in_viewtype, const CRef& /*object*/ in_bone, const CRef& /*object*/ in_effector, const CValue& /*number*/ in_skeletontype, bool in_alignroottofirstbone )
{
	CValueArray args(14);
	CValue retval;
	LONG i(0);

	args[i++]= in_rx;
	args[i++]= in_ry;
	args[i++]= in_rz;
	args[i++]= in_ex;
	args[i++]= in_ey;
	args[i++]= in_ez;
	args[i++]= in_nx;
	args[i++]= in_ny;
	args[i++]= in_nz;
	args[i++]= in_viewtype;
	args[i++]= in_bone;
	args[i++]= in_effector;
	args[i++]= in_skeletontype;
	args[i++]= in_alignroottofirstbone;

	CStatus st = gApp.ExecuteCommand( L"SICreateSkeleton", args, retval );

	return retval;
}

// SICreateTextureBasicGeo
void SICreateTextureBasicGeo( const CString& in_presetobj, const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_supportname, const CValue& /*undefined*/ in_propertyname, const CValue& /*undefined*/ in_parent, siTxtProjType in_projectiontype, siTxtProjPlane in_projectionplane, bool in_visibility, const CValue& /*undefined*/ in_textureprojection, const CRef& /*object*/ in_3dobjtexture, const CRef& /*object*/ in_properties, siTxtProjFitMethod in_fitting )
{
	CValueArray args(12);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobj;
	args[i++]= in_supportname;
	args[i++]= in_propertyname;
	args[i++]= in_parent;
	args[i++]= (LONG)in_projectiontype;
	args[i++]= (LONG)in_projectionplane;
	args[i++]= in_visibility;
	args[i++]= in_textureprojection;
	args[i++]= in_3dobjtexture;
	args[i++]= in_properties;
	args[i++]= (LONG)in_fitting;

	CStatus st = gApp.ExecuteCommand( L"SICreateTextureBasicGeo", args, retval );

	return;
}

// SICreateTextureProj
void SICreateTextureProj( const CString& in_presetobj, const CValue& /*undefined*/ in_inputobj, const CString& in_name, const CValue& /*undefined*/ in_parent, const CRef& /*object*/ in_3dobjtexture, const CRef& /*object*/ in_primobjtexture, const CString& in_propertyname )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobj;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_3dobjtexture;
	args[i++]= in_primobjtexture;
	args[i++]= in_propertyname;

	CStatus st = gApp.ExecuteCommand( L"SICreateTextureProj", args, retval );

	return;
}

// SICreateTexturePureImplicit
void SICreateTexturePureImplicit( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_propertyname )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_propertyname;

	CStatus st = gApp.ExecuteCommand( L"SICreateTexturePureImplicit", args, retval );

	return;
}

// SICreateTextureSpatial
void SICreateTextureSpatial( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_propertyname, const CRef& /*object*/ in_createdobj )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_propertyname;
	args[i++]= in_createdobj;

	CStatus st = gApp.ExecuteCommand( L"SICreateTextureSpatial", args, retval );

	return;
}

// SICreateTextureUVDomain
void SICreateTextureUVDomain( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_supportname, const CValue& /*undefined*/ in_propertyname, bool in_visibility, const CRef& /*object*/ in_outputobjs, const CRef& /*object*/ in_properties )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobj;
	args[i++]= in_supportname;
	args[i++]= in_propertyname;
	args[i++]= in_visibility;
	args[i++]= in_outputobjs;
	args[i++]= in_properties;

	CStatus st = gApp.ExecuteCommand( L"SICreateTextureUVDomain", args, retval );

	return;
}

// SIDeletePointOnCurve
void SIDeletePointOnCurve( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_insertposition )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_insertposition;

	CStatus st = gApp.ExecuteCommand( L"SIDeletePointOnCurve", args, retval );

	return;
}

// SIDuplicate
CValue SIDuplicate( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_nbitems, siBranchFlag in_propagationtype, const CString& in_history, const CString& in_hierarchy, const CString& in_grouping, const CString& in_properties, const CString& in_animation, const CString& in_constraints, const CString& in_texturesupports )
{
	CValueArray args(10);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_nbitems;
	args[i++]= (LONG)in_propagationtype;
	args[i++]= in_history;
	args[i++]= in_hierarchy;
	args[i++]= in_grouping;
	args[i++]= in_properties;
	args[i++]= in_animation;
	args[i++]= in_constraints;
	args[i++]= in_texturesupports;

	CStatus st = gApp.ExecuteCommand( L"SIDuplicate", args, retval );

	return retval;
}

// SIExportDotXSIFile
void SIExportDotXSIFile( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_filename )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"SIExportDotXSIFile", args, retval );

	return;
}

// SIExportIges
void SIExportIges( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_filename )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"SIExportIges", args, retval );

	return;
}

// SIFilter
CValue SIFilter( const CValue& /*undefined*/ in_inputobjs, const CString& in_type, bool in_include, siFilterMode in_mode )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_type;
	args[i++]= in_include;
	args[i++]= (LONG)in_mode;

	CStatus st = gApp.ExecuteCommand( L"SIFilter", args, retval );

	return retval;
}

// SIFindUniqueTextureName
void SIFindUniqueTextureName( const CValue& /*undefined*/ in_inputobj, const CString& in_propertyname )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_propertyname;

	CStatus st = gApp.ExecuteCommand( L"SIFindUniqueTextureName", args, retval );

	return;
}

// SIGetHelpPath
CValue SIGetHelpPath( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SIGetHelpPath", args, retval );

	return retval;
}

// SIGetMaterialShader
CValue SIGetMaterialShader( const CValue& /*undefined*/ in_inputobjs, siMaterialPorts in_port )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_port;

	CStatus st = gApp.ExecuteCommand( L"SIGetMaterialShader", args, retval );

	return retval;
}

// SIGetModel
CValue SIGetModel( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SIGetModel", args, retval );

	return retval;
}

// SIGetPreset
CValue SIGetPreset( const CString& in_path, const CString& in_family )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_path;
	args[i++]= in_family;

	CStatus st = gApp.ExecuteCommand( L"SIGetPreset", args, retval );

	return retval;
}

// SIGetPrim
void SIGetPrim( const CValue& /*undefined*/ in_presetobj, const CString& in_name, const CValue& /*undefined*/ in_parent, bool in_addtoscene, const CRef& /*object*/ in_value, const CRef& /*object*/ in_primitive )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_addtoscene;
	args[i++]= in_value;
	args[i++]= in_primitive;

	CStatus st = gApp.ExecuteCommand( L"SIGetPrim", args, retval );

	return;
}

// SIGetPrimCamera
void SIGetPrimCamera( const CValue& /*undefined*/ in_presetobj, const CString& in_name, const CString& in_interestname, const CValue& /*undefined*/ in_parent, const CRef& /*object*/ in_3dobjcamera, const CRef& /*object*/ in_3dobjcamerainterest, const CRef& /*object*/ in_primobjcamera, const CRef& /*object*/ in_primobjcamerainterest )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_interestname;
	args[i++]= in_parent;
	args[i++]= in_3dobjcamera;
	args[i++]= in_3dobjcamerainterest;
	args[i++]= in_primobjcamera;
	args[i++]= in_primobjcamerainterest;

	CStatus st = gApp.ExecuteCommand( L"SIGetPrimCamera", args, retval );

	return;
}

// SIGetPrimLattice
CValue SIGetPrimLattice( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, const CValue& /*undefined*/ in_parent, const CRef& /*object*/ in_primobj )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_primobj;

	CStatus st = gApp.ExecuteCommand( L"SIGetPrimLattice", args, retval );

	return retval;
}

// SIGetPrimLight
void SIGetPrimLight( const CValue& /*undefined*/ in_presetobj, const CString& in_name, const CString& in_parent, const CRef& /*object*/ in_3dobject, const CRef& /*object*/ in_3dobjspotinterest, const CRef& /*object*/ in_primobj, const CRef& /*object*/ in_primobjspotinterest )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_3dobject;
	args[i++]= in_3dobjspotinterest;
	args[i++]= in_primobj;
	args[i++]= in_primobjspotinterest;

	CStatus st = gApp.ExecuteCommand( L"SIGetPrimLight", args, retval );

	return;
}

// SIGetPrimSpotLight
void SIGetPrimSpotLight( const CValue& /*undefined*/ in_presetobj, const CString& in_name, const CString& in_interestname, const CString& in_parent, const CRef& /*object*/ in_3dobjspot, const CRef& /*object*/ in_3dobjspotinterest, const CRef& /*object*/ in_primobjspot, const CRef& /*object*/ in_primobjspotinterest )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_name;
	args[i++]= in_interestname;
	args[i++]= in_parent;
	args[i++]= in_3dobjspot;
	args[i++]= in_3dobjspotinterest;
	args[i++]= in_primobjspot;
	args[i++]= in_primobjspotinterest;

	CStatus st = gApp.ExecuteCommand( L"SIGetPrimSpotLight", args, retval );

	return;
}

// SIGetPrimVolumeDeform
CValue SIGetPrimVolumeDeform( const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_inputobjs, const CString& in_name, const CValue& /*undefined*/ in_parent, const CRef& /*object*/ in_primobj )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_inputobjs;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_primobj;

	CStatus st = gApp.ExecuteCommand( L"SIGetPrimVolumeDeform", args, retval );

	return retval;
}

// SIGetRigidCns
void SIGetRigidCns( const CValue& /*undefined*/ in_presetobj, const CString& in_filename, const CString& in_name, const CValue& /*undefined*/ in_parent, bool in_addtoscene, const CRef& /*object*/ in_value, const CRef& /*object*/ in_primitive )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_presetobj;
	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_addtoscene;
	args[i++]= in_value;
	args[i++]= in_primitive;

	CStatus st = gApp.ExecuteCommand( L"SIGetRigidCns", args, retval );

	return;
}

// SIGetShaderOnCnxPoint
CValue SIGetShaderOnCnxPoint( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"SIGetShaderOnCnxPoint", args, retval );

	return retval;
}

// SIGetTextureUVDomains
CValue SIGetTextureUVDomains( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SIGetTextureUVDomains", args, retval );

	return retval;
}

// SIHideObjects
void SIHideObjects( const CValue& /*undefined*/ in_target, const CString& in_propertyname )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_propertyname;

	CStatus st = gApp.ExecuteCommand( L"SIHideObjects", args, retval );

	return;
}

// SIImportDotXSIFile
void SIImportDotXSIFile( const CValue& /*undefined*/ in_filename, const CValue& /*undefined*/ in_parent )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_parent;

	CStatus st = gApp.ExecuteCommand( L"SIImportDotXSIFile", args, retval );

	return;
}

// SIImportIges
void SIImportIges( const CValue& /*undefined*/ in_filename, const CValue& /*undefined*/ in_parent, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_parent;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SIImportIges", args, retval );

	return;
}

// SIImportMatLib
void SIImportMatLib( const CValue& /*undefined*/ in_filename, const CValue& /*number*/ in_shareoptions, const CRef& /*object*/ in_value )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_shareoptions;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"SIImportMatLib", args, retval );

	return;
}

// SIImportModel
void SIImportModel( const CValue& /*undefined*/ in_filename, const CValue& /*undefined*/ in_parent, const CRef& /*object*/ in_value, const CString& in_name, const CValue& /*number*/ in_shareoptions )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_parent;
	args[i++]= in_value;
	args[i++]= in_name;
	args[i++]= in_shareoptions;

	CStatus st = gApp.ExecuteCommand( L"SIImportModel", args, retval );

	return;
}

// SIInspectUserData
void SIInspectUserData( const CValue& /*undefined*/ in_userdatamap, const CValue& /*undefined*/ in_subcomp )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_userdatamap;
	args[i++]= in_subcomp;

	CStatus st = gApp.ExecuteCommand( L"SIInspectUserData", args, retval );

	return;
}

// SIInstantiate
CValue SIInstantiate( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_nbitems, const CString& in_name )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_nbitems;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"SIInstantiate", args, retval );

	return retval;
}

// SILoadPreset
void SILoadPreset( const CString& in_filename, const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"SILoadPreset", args, retval );

	return;
}

// SILocalAssignFlexEnv
void SILocalAssignFlexEnv( const CValue& /*undefined*/ in_subcomp, const CValue& /*undefined*/ in_connectionset, bool in_interactive, const CValue& /*number*/ in_nbdeformersperpoint )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_subcomp;
	args[i++]= in_connectionset;
	args[i++]= in_interactive;
	args[i++]= in_nbdeformersperpoint;

	CStatus st = gApp.ExecuteCommand( L"SILocalAssignFlexEnv", args, retval );

	return;
}

// SIMakeLocal
CValue SIMakeLocal( const CValue& /*undefined*/ in_inputobjs, siPropagationType in_propagationtype )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_propagationtype;

	CStatus st = gApp.ExecuteCommand( L"SIMakeLocal", args, retval );

	return retval;
}

// SIMatchSubComponent
void SIMatchSubComponent( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_subcomp, bool in_result )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_subcomp;
	args[i++]= in_result;

	CStatus st = gApp.ExecuteCommand( L"SIMatchSubComponent", args, retval );

	return;
}

// SIMergeScene
void SIMergeScene( const CValue& /*undefined*/ in_filename, const CString& in_name, const CValue& /*undefined*/ in_parent, const CValue& /*number*/ in_shareoptions, const CRef& /*object*/ in_createdobj )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;
	args[i++]= in_name;
	args[i++]= in_parent;
	args[i++]= in_shareoptions;
	args[i++]= in_createdobj;

	CStatus st = gApp.ExecuteCommand( L"SIMergeScene", args, retval );

	return;
}

// SIModifyFlexEnvWght
CValue SIModifyFlexEnvWght( const CValue& /*undefined*/ in_propobj, const CString& in_name, const CValue& /*undefined*/ in_subcomp, const CValue& /*number*/ in_mode, const CValue& /*number*/ in_value, bool in_normalize )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_propobj;
	args[i++]= in_name;
	args[i++]= in_subcomp;
	args[i++]= in_mode;
	args[i++]= in_value;
	args[i++]= in_normalize;

	CStatus st = gApp.ExecuteCommand( L"SIModifyFlexEnvWght", args, retval );

	return retval;
}

// SIObjExport
void SIObjExport( const CRef& /*object*/ in_object, const CString& in_filename, const CValue& /*number*/ in_coordinatesystem, bool in_tesselation, bool in_material, bool in_uv, bool in_usernormal )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_object;
	args[i++]= in_filename;
	args[i++]= in_coordinatesystem;
	args[i++]= in_tesselation;
	args[i++]= in_material;
	args[i++]= in_uv;
	args[i++]= in_usernormal;

	CStatus st = gApp.ExecuteCommand( L"SIObjExport", args, retval );

	return;
}

// SIRebindByNameUserData
void SIRebindByNameUserData( const CValue& /*undefined*/ in_material, const CRef& /*object*/ in_source_user_data_collection, const CRef& /*object*/ in_target_user_data_collection )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_material;
	args[i++]= in_source_user_data_collection;
	args[i++]= in_target_user_data_collection;

	CStatus st = gApp.ExecuteCommand( L"SIRebindByNameUserData", args, retval );

	return;
}

// SIRemoveArrayElement
void SIRemoveArrayElement( const CValue& /*undefined*/ in_object, const CValue& /*number*/ in_index )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_object;
	args[i++]= in_index;

	CStatus st = gApp.ExecuteCommand( L"SIRemoveArrayElement", args, retval );

	return;
}

// SIRemoveEntryFromOverride
void SIRemoveEntryFromOverride( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SIRemoveEntryFromOverride", args, retval );

	return;
}

// SIRemoveFromCluster
void SIRemoveFromCluster( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_subcomp )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_subcomp;

	CStatus st = gApp.ExecuteCommand( L"SIRemoveFromCluster", args, retval );

	return;
}

// SIRemoveFromGroup
void SIRemoveFromGroup( const CValue& /*undefined*/ in_target, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SIRemoveFromGroup", args, retval );

	return;
}

// SIRenderPass
void SIRenderPass( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_presetobj, const CValue& /*undefined*/ in_startframe, const CValue& /*undefined*/ in_endframe, const CValue& /*undefined*/ in_stepframe, bool in_disposerendering )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_presetobj;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_stepframe;
	args[i++]= in_disposerendering;

	CStatus st = gApp.ExecuteCommand( L"SIRenderPass", args, retval );

	return;
}

// SISetCurvePoints
void SISetCurvePoints( const CValue& /*undefined*/ in_inputobj, const CString& in_points, bool in_local )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_points;
	args[i++]= in_local;

	CStatus st = gApp.ExecuteCommand( L"SISetCurvePoints", args, retval );

	return;
}

// SISetResolutionOfRefModels
void SISetResolutionOfRefModels( const CValue& /*undefined*/ in_target, const CValue& /*number*/ in_resolution )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_resolution;

	CStatus st = gApp.ExecuteCommand( L"SISetResolutionOfRefModels", args, retval );

	return;
}

// SISmoothEnvWght
CValue SISmoothEnvWght( const CValue& /*undefined*/ in_propobj, const CValue& /*undefined*/ in_subcomp )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_propobj;
	args[i++]= in_subcomp;

	CStatus st = gApp.ExecuteCommand( L"SISmoothEnvWght", args, retval );

	return retval;
}

// SIStoreAction
CValue SIStoreAction( const CValue& /*undefined*/ in_model, const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_mode, const CString& in_name, bool in_removeanim, bool in_consider_time, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, bool in_ripple )
{
	CValueArray args(9);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_inputobj;
	args[i++]= in_mode;
	args[i++]= in_name;
	args[i++]= in_removeanim;
	args[i++]= in_consider_time;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_ripple;

	CStatus st = gApp.ExecuteCommand( L"SIStoreAction", args, retval );

	return retval;
}

// SIStoreEnvironmentCacheToMixer
CValue SIStoreEnvironmentCacheToMixer( const CValue& /*undefined*/ in_environmentcache, const CString& in_name, bool in_addclip )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_environmentcache;
	args[i++]= in_name;
	args[i++]= in_addclip;

	CStatus st = gApp.ExecuteCommand( L"SIStoreEnvironmentCacheToMixer", args, retval );

	return retval;
}

// SIStoreProp
void SIStoreProp( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"SIStoreProp", args, retval );

	return;
}

// SISwapArrayElements
void SISwapArrayElements( const CValue& /*undefined*/ in_object, const CValue& /*number*/ in_indexa, const CValue& /*number*/ in_indexb )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_object;
	args[i++]= in_indexa;
	args[i++]= in_indexb;

	CStatus st = gApp.ExecuteCommand( L"SISwapArrayElements", args, retval );

	return;
}

// SISymEnvWeight
CValue SISymEnvWeight( const CValue& /*undefined*/ in_propobj, const CValue& /*undefined*/ in_subcomp, const CValue& /*undefined*/ in_mappingtemplate )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_propobj;
	args[i++]= in_subcomp;
	args[i++]= in_mappingtemplate;

	CStatus st = gApp.ExecuteCommand( L"SISymEnvWeight", args, retval );

	return retval;
}

// SITogglePolygonVisibility
void SITogglePolygonVisibility( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"SITogglePolygonVisibility", args, retval );

	return;
}

// SITransferCenter
void SITransferCenter( const CValue& /*undefined*/ in_inputobj, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;

	CStatus st = gApp.ExecuteCommand( L"SITransferCenter", args, retval );

	return;
}

// siTrimWith
void siTrimWith( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_trimwithcrvindexarray, bool in_trimwithglobaltolocalbool )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_trimwithcrvindexarray;
	args[i++]= in_trimwithglobaltolocalbool;

	CStatus st = gApp.ExecuteCommand( L"siTrimWith", args, retval );

	return;
}

// SIUIRenderInspectedPasses
void SIUIRenderInspectedPasses( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SIUIRenderInspectedPasses", args, retval );

	return;
}

// SIUnAssignMaterial
void SIUnAssignMaterial( const CValue& /*undefined*/ in_objects )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_objects;

	CStatus st = gApp.ExecuteCommand( L"SIUnAssignMaterial", args, retval );

	return;
}

// SIUnhideObjects
void SIUnhideObjects( const CValue& /*undefined*/ in_target, const CString& in_propertyname )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_propertyname;

	CStatus st = gApp.ExecuteCommand( L"SIUnhideObjects", args, retval );

	return;
}

// SketchCurveTool
void SketchCurveTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"SketchCurveTool", args, retval );

	return;
}

// SMDExport
CValue SMDExport( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SMDExport", args, retval );

	return retval;
}

// SMDImport
CValue SMDImport( const CValue& /*number*/ in_arg0 )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_arg0;

	CStatus st = gApp.ExecuteCommand( L"SMDImport", args, retval );

	return retval;
}

// SnapOffsetToPointForSlicePolygons
void SnapOffsetToPointForSlicePolygons( const CValue& /*undefined*/ in_operator, const CValue& /*undefined*/ in_target )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"SnapOffsetToPointForSlicePolygons", args, retval );

	return;
}

// SplitEdge
CValue SplitEdge( const CValue& /*undefined*/ in_connectionset, const CValue& /*number*/ in_ratio, bool in_edgeparallelloop, bool in_edgeconnect, siOperationMode in_immediatemode, const CValue& /*number*/ in_newpointid )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= in_ratio;
	args[i++]= in_edgeparallelloop;
	args[i++]= in_edgeconnect;
	args[i++]= (LONG)in_immediatemode;
	args[i++]= in_newpointid;

	CStatus st = gApp.ExecuteCommand( L"SplitEdge", args, retval );

	return retval;
}

// SplitEdgeTool
void SplitEdgeTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"SplitEdgeTool", args, retval );

	return;
}

// SplitPolygon
CValue SplitPolygon( const CString& in_connectionset, const CValue& /*number*/ in_tanx, const CValue& /*number*/ in_tany, const CValue& /*number*/ in_tanz, siSplitPolygonType in_splitpolygontype, siOperationMode in_immediatemode, const CValue& /*number*/ in_newpointid )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= in_tanx;
	args[i++]= in_tany;
	args[i++]= in_tanz;
	args[i++]= (LONG)in_splitpolygontype;
	args[i++]= (LONG)in_immediatemode;
	args[i++]= in_newpointid;

	CStatus st = gApp.ExecuteCommand( L"SplitPolygon", args, retval );

	return retval;
}

// StartChannelCapture
void StartChannelCapture( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"StartChannelCapture", args, retval );

	return;
}

// StartFxTracking
void StartFxTracking( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"StartFxTracking", args, retval );

	return;
}

// StepFxTracking
void StepFxTracking( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"StepFxTracking", args, retval );

	return;
}

// StopChannelCapture
void StopChannelCapture( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"StopChannelCapture", args, retval );

	return;
}

// StoreAction
CValue StoreAction( const CString& in_model, const CString& in_inputobj, const CValue& /*number*/ in_mode, const CString& in_name, bool in_removeanim, const CValue& /*number*/ in_startframe, const CValue& /*number*/ in_endframe, bool in_consider_time, bool in_ripple, bool in_addclip, const CValue& /*number*/ in_clipoffset )
{
	CValueArray args(11);
	CValue retval;
	LONG i(0);

	args[i++]= in_model;
	args[i++]= in_inputobj;
	args[i++]= in_mode;
	args[i++]= in_name;
	args[i++]= in_removeanim;
	args[i++]= in_startframe;
	args[i++]= in_endframe;
	args[i++]= in_consider_time;
	args[i++]= in_ripple;
	args[i++]= in_addclip;
	args[i++]= in_clipoffset;

	CStatus st = gApp.ExecuteCommand( L"StoreAction", args, retval );

	return retval;
}

// StoreEnvironmentCachesToMixer
CValue StoreEnvironmentCachesToMixer( const CValue& /*undefined*/ in_environment, const CString& in_name, bool in_addclip )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_environment;
	args[i++]= in_name;
	args[i++]= in_addclip;

	CStatus st = gApp.ExecuteCommand( L"StoreEnvironmentCachesToMixer", args, retval );

	return retval;
}

// StoreEnvironmentCacheToMixer
CValue StoreEnvironmentCacheToMixer( const CValue& /*undefined*/ in_environmentcache, const CString& in_name, bool in_addclip )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_environmentcache;
	args[i++]= in_name;
	args[i++]= in_addclip;

	CStatus st = gApp.ExecuteCommand( L"StoreEnvironmentCacheToMixer", args, retval );

	return retval;
}

// StoreGeomApproxProperties
void StoreGeomApproxProperties( const CValue& /*undefined*/ in_inputobjs, const CString& in_name )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"StoreGeomApproxProperties", args, retval );

	return;
}

// StoreMaterialProperties
void StoreMaterialProperties( const CValue& /*undefined*/ in_inputobjs, const CString& in_name )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"StoreMaterialProperties", args, retval );

	return;
}

// StoreShapeKey
CValue StoreShapeKey( const CValue& /*undefined*/ in_inputobj, const CValue& /*undefined*/ in_name, siShapeReferenceMode in_refmode, const CValue& /*undefined*/ in_frame, const CValue& /*number*/ in_duration, const CValue& /*number*/ in_sampling, siShapeContent in_content )
{
	CValueArray args(7);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_name;
	args[i++]= (LONG)in_refmode;
	args[i++]= in_frame;
	args[i++]= in_duration;
	args[i++]= in_sampling;
	args[i++]= (LONG)in_content;

	CStatus st = gApp.ExecuteCommand( L"StoreShapeKey", args, retval );

	return retval;
}

// StoreVisibilityProperties
void StoreVisibilityProperties( const CValue& /*undefined*/ in_inputobjs, const CString& in_name )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_name;

	CStatus st = gApp.ExecuteCommand( L"StoreVisibilityProperties", args, retval );

	return;
}

// SubstituteOpInput
void SubstituteOpInput( const CValue& /*undefined*/ in_reference, const CValue& /*undefined*/ in_new )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_reference;
	args[i++]= in_new;

	CStatus st = gApp.ExecuteCommand( L"SubstituteOpInput", args, retval );

	return;
}

// SwapOperators
void SwapOperators( const CValue& /*undefined*/ in_object, const CValue& /*undefined*/ in_operator1, const CValue& /*undefined*/ in_operator2 )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_object;
	args[i++]= in_operator1;
	args[i++]= in_operator2;

	CStatus st = gApp.ExecuteCommand( L"SwapOperators", args, retval );

	return;
}

// SwapPaintColors
void SwapPaintColors( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SwapPaintColors", args, retval );

	return;
}

// SwapRelationMasterAndSlaveClip
void SwapRelationMasterAndSlaveClip( const CValue& /*undefined*/ in_relation )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_relation;

	CStatus st = gApp.ExecuteCommand( L"SwapRelationMasterAndSlaveClip", args, retval );

	return;
}

// SwitchToolbar
void SwitchToolbar( const CValue& /*number*/ in_target )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"SwitchToolbar", args, retval );

	return;
}

// SwitchToView
void SwitchToView( const CString& in_viewname )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_viewname;

	CStatus st = gApp.ExecuteCommand( L"SwitchToView", args, retval );

	return;
}

// SyClearCache
void SyClearCache( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyClearCache", args, retval );

	return;
}

// SyClearState
void SyClearState( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyClearState", args, retval );

	return;
}

// SyClearVelocityState
void SyClearVelocityState( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyClearVelocityState", args, retval );

	return;
}

// SyCollideGetMesh
void SyCollideGetMesh( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyCollideGetMesh", args, retval );

	return;
}

// SyCreateAir
void SyCreateAir( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyCreateAir", args, retval );

	return;
}

// SyCreateCloth
void SyCreateCloth( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyCreateCloth", args, retval );

	return;
}

// SyCreateCollideMesh
void SyCreateCollideMesh( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyCreateCollideMesh", args, retval );

	return;
}

// SyCreateDamp
void SyCreateDamp( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyCreateDamp", args, retval );

	return;
}

// SyCreateGravity
void SyCreateGravity( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyCreateGravity", args, retval );

	return;
}

// SyCreateNail
void SyCreateNail( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyCreateNail", args, retval );

	return;
}

// SyCreatePin
void SyCreatePin( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyCreatePin", args, retval );

	return;
}

// SyCreateSelfCollide
void SyCreateSelfCollide( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyCreateSelfCollide", args, retval );

	return;
}

// SyCreateSpring
void SyCreateSpring( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyCreateSpring", args, retval );

	return;
}

// SyCreateWind
void SyCreateWind( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyCreateWind", args, retval );

	return;
}

// SyGetCollidePnts
void SyGetCollidePnts( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyGetCollidePnts", args, retval );

	return;
}

// SyGetCollidePols
void SyGetCollidePols( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyGetCollidePols", args, retval );

	return;
}

// SyGetMassMap
void SyGetMassMap( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyGetMassMap", args, retval );

	return;
}

// SyGetNail
void SyGetNail( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyGetNail", args, retval );

	return;
}

// SyGetPin
void SyGetPin( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyGetPin", args, retval );

	return;
}

// SyGetSelfCollidePols
void SyGetSelfCollidePols( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyGetSelfCollidePols", args, retval );

	return;
}

// SyGetSpringMap
void SyGetSpringMap( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyGetSpringMap", args, retval );

	return;
}

// SymEnvWeight
CValue SymEnvWeight( const CValue& /*undefined*/ in_propobj, const CValue& /*undefined*/ in_subcomp, const CValue& /*undefined*/ in_mappingtemplate, const CValue& /*number*/ in_symmetryaxis )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_propobj;
	args[i++]= in_subcomp;
	args[i++]= in_mappingtemplate;
	args[i++]= in_symmetryaxis;

	CStatus st = gApp.ExecuteCommand( L"SymEnvWeight", args, retval );

	return retval;
}

// SyPinGetMesh
void SyPinGetMesh( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SyPinGetMesh", args, retval );

	return;
}

// SySetCollidePnts
void SySetCollidePnts( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SySetCollidePnts", args, retval );

	return;
}

// SySetCollidePols
void SySetCollidePols( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SySetCollidePols", args, retval );

	return;
}

// SySetMassMap
void SySetMassMap( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SySetMassMap", args, retval );

	return;
}

// SySetNail
void SySetNail( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SySetNail", args, retval );

	return;
}

// SySetPin
void SySetPin( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SySetPin", args, retval );

	return;
}

// SySetSelfCollidePols
void SySetSelfCollidePols( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SySetSelfCollidePols", args, retval );

	return;
}

// SySetSpringMap
void SySetSpringMap( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SySetSpringMap", args, retval );

	return;
}

// SySetState
void SySetState( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"SySetState", args, retval );

	return;
}

// System
CValue System( const CString& in_pathname, const CValue& /*number*/ in_value )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_pathname;
	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"System", args, retval );

	return retval;
}

// Tag
void Tag( const CValue& /*undefined*/ in_inputobjs, siTags in_level )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_level;

	CStatus st = gApp.ExecuteCommand( L"Tag", args, retval );

	return;
}

// TagForDelete
void TagForDelete( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"TagForDelete", args, retval );

	return;
}

// TagObject
CValue TagObject( const CValue& /*undefined*/ in_inputobj, const CString& in_tagstr )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_tagstr;

	CStatus st = gApp.ExecuteCommand( L"TagObject", args, retval );

	return retval;
}

// TagStepMarks
CValue TagStepMarks( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"TagStepMarks", args, retval );

	return retval;
}

// TerminateAddEdgeWithInteriorPoints
void TerminateAddEdgeWithInteriorPoints( const CValue& /*undefined*/ in_operator, const CValue& /*number*/ in_polygonid, const CValue& /*number*/ in_edgeid, const CValue& /*number*/ in_ratio )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_polygonid;
	args[i++]= in_edgeid;
	args[i++]= in_ratio;

	CStatus st = gApp.ExecuteCommand( L"TerminateAddEdgeWithInteriorPoints", args, retval );

	return;
}

// TestPluginHost
void TestPluginHost( bool in_destroy )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_destroy;

	CStatus st = gApp.ExecuteCommand( L"TestPluginHost", args, retval );

	return;
}

// TestRenTasks
void TestRenTasks( const CValue& /*number*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"TestRenTasks", args, retval );

	return;
}

// ToggleActiveElements
void ToggleActiveElements( const CValue& /*undefined*/ in_selectionlist )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;

	CStatus st = gApp.ExecuteCommand( L"ToggleActiveElements", args, retval );

	return;
}

// ToggleAlphaCompose
void ToggleAlphaCompose( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ToggleAlphaCompose", args, retval );

	return;
}

// ToggleAlphaOnly
void ToggleAlphaOnly( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ToggleAlphaOnly", args, retval );

	return;
}

// ToggleAreaLights
void ToggleAreaLights( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ToggleAreaLights", args, retval );

	return;
}

// ToggleBooleanUserPref
CValue ToggleBooleanUserPref( const CString& in_userpref )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_userpref;

	CStatus st = gApp.ExecuteCommand( L"ToggleBooleanUserPref", args, retval );

	return retval;
}

// ToggleBooleanUserPrefProc
CValue ToggleBooleanUserPrefProc( const CString& in_userpref )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_userpref;

	CStatus st = gApp.ExecuteCommand( L"ToggleBooleanUserPrefProc", args, retval );

	return retval;
}

// ToggleContinuousDraw
void ToggleContinuousDraw( const CString& in_paramname, const CString& in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_paramname;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ToggleContinuousDraw", args, retval );

	return;
}

// ToggleCurrentLayerProperty
void ToggleCurrentLayerProperty( const CString& in_propname )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_propname;

	CStatus st = gApp.ExecuteCommand( L"ToggleCurrentLayerProperty", args, retval );

	return;
}

// ToggleFreezeRefresh
void ToggleFreezeRefresh( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ToggleFreezeRefresh", args, retval );

	return;
}

// ToggleGhosting
void ToggleGhosting( const CValue& /*undefined*/ in_target )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;

	CStatus st = gApp.ExecuteCommand( L"ToggleGhosting", args, retval );

	return;
}

// ToggleMuteSolo
void ToggleMuteSolo( const CValue& /*number*/ in_target, const CValue& /*number*/ in_mode )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_mode;

	CStatus st = gApp.ExecuteCommand( L"ToggleMuteSolo", args, retval );

	return;
}

// TogglePaintShapePointSelection
void TogglePaintShapePointSelection( const CValue& /*undefined*/ in_shape, const CValue& /*undefined*/ in_indices )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_indices;

	CStatus st = gApp.ExecuteCommand( L"TogglePaintShapePointSelection", args, retval );

	return;
}

// ToggleParameterValue
void ToggleParameterValue( const CValue& /*undefined*/ in_inputobjects, const CString& in_parameters )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjects;
	args[i++]= in_parameters;

	CStatus st = gApp.ExecuteCommand( L"ToggleParameterValue", args, retval );

	return;
}

// TogglePinObjectInPosition
void TogglePinObjectInPosition( const CValue& /*undefined*/ in_constrainedobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_constrainedobj;

	CStatus st = gApp.ExecuteCommand( L"TogglePinObjectInPosition", args, retval );

	return;
}

// ToggleProjectionTool
void ToggleProjectionTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"ToggleProjectionTool", args, retval );

	return;
}

// TogglePropVolume
void TogglePropVolume( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"TogglePropVolume", args, retval );

	return;
}

// ToggleRegionVisibility
void ToggleRegionVisibility( const CValue& /*number*/ in_value )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_value;

	CStatus st = gApp.ExecuteCommand( L"ToggleRegionVisibility", args, retval );

	return;
}

// ToggleSelectability
void ToggleSelectability( const CValue& /*undefined*/ in_inputobjects, const CString& in_parameter )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjects;
	args[i++]= in_parameter;

	CStatus st = gApp.ExecuteCommand( L"ToggleSelectability", args, retval );

	return;
}

// ToggleSelection
void ToggleSelection( const CValue& /*undefined*/ in_selectionlist, const CString& in_hierarchylevel, bool in_checkobjectselectability )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_selectionlist;
	args[i++]= in_hierarchylevel;
	args[i++]= in_checkobjectselectability;

	CStatus st = gApp.ExecuteCommand( L"ToggleSelection", args, retval );

	return;
}

// ToggleSelectionTracking
void ToggleSelectionTracking( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ToggleSelectionTracking", args, retval );

	return;
}

// ToggleSnap0D
void ToggleSnap0D( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnap0D", args, retval );

	return;
}

// ToggleSnap1D
void ToggleSnap1D( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnap1D", args, retval );

	return;
}

// ToggleSnap2D
void ToggleSnap2D( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnap2D", args, retval );

	return;
}

// ToggleSnapAllObjects
void ToggleSnapAllObjects( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapAllObjects", args, retval );

	return;
}

// ToggleSnapBoundaries
void ToggleSnapBoundaries( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapBoundaries", args, retval );

	return;
}

// ToggleSnapCenters
void ToggleSnapCenters( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapCenters", args, retval );

	return;
}

// ToggleSnapCurveObjects
void ToggleSnapCurveObjects( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapCurveObjects", args, retval );

	return;
}

// ToggleSnapCurves
void ToggleSnapCurves( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapCurves", args, retval );

	return;
}

// ToggleSnapEdges
void ToggleSnapEdges( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapEdges", args, retval );

	return;
}

// ToggleSnapFaces
void ToggleSnapFaces( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapFaces", args, retval );

	return;
}

// ToggleSnapGrid
void ToggleSnapGrid( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapGrid", args, retval );

	return;
}

// ToggleSnapHairObjects
void ToggleSnapHairObjects( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapHairObjects", args, retval );

	return;
}

// ToggleSnapIntersections
void ToggleSnapIntersections( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapIntersections", args, retval );

	return;
}

// ToggleSnapKnots
void ToggleSnapKnots( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapKnots", args, retval );

	return;
}

// ToggleSnapLatticeObjects
void ToggleSnapLatticeObjects( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapLatticeObjects", args, retval );

	return;
}

// ToggleSnapMeshObjects
void ToggleSnapMeshObjects( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapMeshObjects", args, retval );

	return;
}

// ToggleSnapMidpoints
void ToggleSnapMidpoints( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapMidpoints", args, retval );

	return;
}

// ToggleSnapNullObjects
void ToggleSnapNullObjects( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapNullObjects", args, retval );

	return;
}

// ToggleSnapParticleObjects
void ToggleSnapParticleObjects( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapParticleObjects", args, retval );

	return;
}

// ToggleSnapPoints
void ToggleSnapPoints( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapPoints", args, retval );

	return;
}

// ToggleSnapSurfaceObjects
void ToggleSnapSurfaceObjects( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapSurfaceObjects", args, retval );

	return;
}

// ToggleSnapSurfaces
void ToggleSnapSurfaces( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapSurfaces", args, retval );

	return;
}

// ToggleSnapUknotcurves
void ToggleSnapUknotcurves( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapUknotcurves", args, retval );

	return;
}

// ToggleSnapVknotcurves
void ToggleSnapVknotcurves( const CString& in_property )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_property;

	CStatus st = gApp.ExecuteCommand( L"ToggleSnapVknotcurves", args, retval );

	return;
}

// ToggleValue
void ToggleValue( const CString& in_paramname, const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_paramname;
	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"ToggleValue", args, retval );

	return;
}

// ToggleVisibility
void ToggleVisibility( const CValue& /*undefined*/ in_target, const CString& in_propertyname, bool in_threestate )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_target;
	args[i++]= in_propertyname;
	args[i++]= in_threestate;

	CStatus st = gApp.ExecuteCommand( L"ToggleVisibility", args, retval );

	return;
}

// ToggleVisibilityOfTextureUVDomains
void ToggleVisibilityOfTextureUVDomains( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_type )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_type;

	CStatus st = gApp.ExecuteCommand( L"ToggleVisibilityOfTextureUVDomains", args, retval );

	return;
}

// TransferAllPropertiesAcrossGenOp
CValue TransferAllPropertiesAcrossGenOp( const CValue& /*undefined*/ in_operator, const CValue& /*undefined*/ in_object, const CString& in_propertytransfertype, bool in_shareproperty )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_object;
	args[i++]= in_propertytransfertype;
	args[i++]= in_shareproperty;

	CStatus st = gApp.ExecuteCommand( L"TransferAllPropertiesAcrossGenOp", args, retval );

	return retval;
}

// TransferClusterPropertiesAcrossGenOp
CValue TransferClusterPropertiesAcrossGenOp( const CValue& /*undefined*/ in_operator, const CValue& /*undefined*/ in_object, const CValue& /*undefined*/ in_clusterpropstotransfer, const CString& in_clusterpropertyname, bool in_rebindclusterproperties )
{
	CValueArray args(5);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_object;
	args[i++]= in_clusterpropstotransfer;
	args[i++]= in_clusterpropertyname;
	args[i++]= in_rebindclusterproperties;

	CStatus st = gApp.ExecuteCommand( L"TransferClusterPropertiesAcrossGenOp", args, retval );

	return retval;
}

// TransferPropertiesAcrossGenOp
CValue TransferPropertiesAcrossGenOp( const CValue& /*undefined*/ in_operator, const CValue& /*undefined*/ in_object, const CValue& /*undefined*/ in_propertiestotransfer, const CString& in_propertyname, bool in_shareproperty, bool in_transferclusterproperties )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_object;
	args[i++]= in_propertiestotransfer;
	args[i++]= in_propertyname;
	args[i++]= in_shareproperty;
	args[i++]= in_transferclusterproperties;

	CStatus st = gApp.ExecuteCommand( L"TransferPropertiesAcrossGenOp", args, retval );

	return retval;
}

// Translate
void Translate( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z, siDeltaMode in_delta, siRefMode in_refmode, siCenterMode in_center, siAxesFilter in_axesfilter, bool in_snap, const CString& in_snapreference, siAxesFilter in_snapfilter, bool in_splitlocalcomponents, bool in_proptagonly, bool in_pivot, const CValue& /*number*/ in_pivotx, const CValue& /*number*/ in_pivoty, const CValue& /*number*/ in_pivotz, const CValue& /*undefined*/ in_constructionmode )
{
	CValueArray args(18);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= (LONG)in_delta;
	args[i++]= (LONG)in_refmode;
	args[i++]= (LONG)in_center;
	args[i++]= (LONG)in_axesfilter;
	args[i++]= in_snap;
	args[i++]= in_snapreference;
	args[i++]= (LONG)in_snapfilter;
	args[i++]= in_splitlocalcomponents;
	args[i++]= in_proptagonly;
	args[i++]= in_pivot;
	args[i++]= in_pivotx;
	args[i++]= in_pivoty;
	args[i++]= in_pivotz;
	args[i++]= in_constructionmode;

	CStatus st = gApp.ExecuteCommand( L"Translate", args, retval );

	return;
}

// TranslatePaintShapePoint
void TranslatePaintShapePoint( const CValue& /*undefined*/ in_shape, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, siDeltaMode in_delta )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_shape;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= (LONG)in_delta;

	CStatus st = gApp.ExecuteCommand( L"TranslatePaintShapePoint", args, retval );

	return;
}

// TranslateTool
void TranslateTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"TranslateTool", args, retval );

	return;
}

// TranslateUVW
void TranslateUVW( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_textureprojection, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y, const CValue& /*number*/ in_z, siDeltaMode in_delta, siUVWAxesFilter in_axesfilter, bool in_splitlocalcomponents )
{
	CValueArray args(8);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_textureprojection;
	args[i++]= in_x;
	args[i++]= in_y;
	args[i++]= in_z;
	args[i++]= (LONG)in_delta;
	args[i++]= (LONG)in_axesfilter;
	args[i++]= in_splitlocalcomponents;

	CStatus st = gApp.ExecuteCommand( L"TranslateUVW", args, retval );

	return;
}

// TrimAfter
void TrimAfter( const CValue& /*undefined*/ in_clip, siCutClipOptions in_mode, const CValue& /*undefined*/ in_time )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;
	args[i++]= (LONG)in_mode;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"TrimAfter", args, retval );

	return;
}

// TrimBefore
void TrimBefore( const CValue& /*undefined*/ in_clip, siCutClipOptions in_mode, const CValue& /*undefined*/ in_time )
{
	CValueArray args(3);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;
	args[i++]= (LONG)in_mode;
	args[i++]= in_time;

	CStatus st = gApp.ExecuteCommand( L"TrimBefore", args, retval );

	return;
}

// TrimWith
void TrimWith( const CValue& /*undefined*/ in_inputobjs, const CValue& /*undefined*/ in_trimwithsrfcrv )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_trimwithsrfcrv;

	CStatus st = gApp.ExecuteCommand( L"TrimWith", args, retval );

	return;
}

// UIApplyShaderToCnxPoints
void UIApplyShaderToCnxPoints( const CRef& /*object*/ in_inputobj, const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_x, const CValue& /*number*/ in_y )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;
	args[i++]= in_inputobjs;
	args[i++]= in_x;
	args[i++]= in_y;

	CStatus st = gApp.ExecuteCommand( L"UIApplyShaderToCnxPoints", args, retval );

	return;
}

// UIAssembleNurbsMesh
void UIAssembleNurbsMesh( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"UIAssembleNurbsMesh", args, retval );

	return;
}

// UIMapButton
void UIMapButton( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"UIMapButton", args, retval );

	return;
}

// UnAssignMaterial
void UnAssignMaterial( const CValue& /*undefined*/ in_inputobjs )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;

	CStatus st = gApp.ExecuteCommand( L"UnAssignMaterial", args, retval );

	return;
}

// Undo
void Undo( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"Undo", args, retval );

	return;
}

// UnghostAll
void UnghostAll( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"UnghostAll", args, retval );

	return;
}

// UnhideAll
void UnhideAll( const CString& in_propertyname )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_propertyname;

	CStatus st = gApp.ExecuteCommand( L"UnhideAll", args, retval );

	return;
}

// UnhideAllPolygonClusters
void UnhideAllPolygonClusters( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"UnhideAllPolygonClusters", args, retval );

	return;
}

// UnhideAllPolygons
void UnhideAllPolygons( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"UnhideAllPolygons", args, retval );

	return;
}

// UnhideTextureUVDomains
void UnhideTextureUVDomains( const CValue& /*undefined*/ in_inputobjs, const CValue& /*number*/ in_type )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= in_type;

	CStatus st = gApp.ExecuteCommand( L"UnhideTextureUVDomains", args, retval );

	return;
}

// UnInstallAddon
void UnInstallAddon( const CString& in_filename )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"UnInstallAddon", args, retval );

	return;
}

// UnloadAddonLib
void UnloadAddonLib( const CValue& /*undefined*/ in_filename )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_filename;

	CStatus st = gApp.ExecuteCommand( L"UnloadAddonLib", args, retval );

	return;
}

// UnloadAllAddonLibs
void UnloadAllAddonLibs( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"UnloadAllAddonLibs", args, retval );

	return;
}

// UnloadAllCOMLibs
void UnloadAllCOMLibs( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"UnloadAllCOMLibs", args, retval );

	return;
}

// Unlock
void Unlock( const CValue& /*undefined*/ in_inputobjs, siLockLevel in_level, const CString& in_password, bool in_removeall )
{
	CValueArray args(4);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_level;
	args[i++]= in_password;
	args[i++]= in_removeall;

	CStatus st = gApp.ExecuteCommand( L"Unlock", args, retval );

	return;
}

// Untag
void Untag( const CValue& /*undefined*/ in_inputobjs, siTags in_level )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobjs;
	args[i++]= (LONG)in_level;

	CStatus st = gApp.ExecuteCommand( L"Untag", args, retval );

	return;
}

// UpdateConnectOnOp
void UpdateConnectOnOp( const CValue& /*undefined*/ in_operator, const CValue& /*number*/ in_groupindex, const CValue& /*number*/ in_groupinstanceindex, const CValue& /*undefined*/ in_inputobjs, bool in_alreadyrefined, bool in_forcedisconnect )
{
	CValueArray args(6);
	CValue retval;
	LONG i(0);

	args[i++]= in_operator;
	args[i++]= in_groupindex;
	args[i++]= in_groupinstanceindex;
	args[i++]= in_inputobjs;
	args[i++]= in_alreadyrefined;
	args[i++]= in_forcedisconnect;

	CStatus st = gApp.ExecuteCommand( L"UpdateConnectOnOp", args, retval );

	return;
}

// UpdateParticleTypeConnections
void UpdateParticleTypeConnections( const CValue& /*undefined*/ in_inputobj )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_inputobj;

	CStatus st = gApp.ExecuteCommand( L"UpdateParticleTypeConnections", args, retval );

	return;
}

// UpdateReferencedModel
void UpdateReferencedModel( const CValue& /*undefined*/ in_clip )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clip;

	CStatus st = gApp.ExecuteCommand( L"UpdateReferencedModel", args, retval );

	return;
}

// UpgradeShaders
CValue UpgradeShaders( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"UpgradeShaders", args, retval );

	return retval;
}

// ValveCopyShapeKey
CValue ValveCopyShapeKey( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ValveCopyShapeKey", args, retval );

	return retval;
}

// ValveFixBrokenFootRoll
CValue ValveFixBrokenFootRoll( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ValveFixBrokenFootRoll", args, retval );

	return retval;
}

// ValveRemoveCenterOp
CValue ValveRemoveCenterOp( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ValveRemoveCenterOp", args, retval );

	return retval;
}

// ValveSetCopyShapeCluster
CValue ValveSetCopyShapeCluster( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"ValveSetCopyShapeCluster", args, retval );

	return retval;
}

// VirtualSliderTool
void VirtualSliderTool( const CString& in_clsid )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_clsid;

	CStatus st = gApp.ExecuteCommand( L"VirtualSliderTool", args, retval );

	return;
}

// VMFExport
CValue VMFExport( const CValue& /*number*/ in_arg0 )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_arg0;

	CStatus st = gApp.ExecuteCommand( L"VMFExport", args, retval );

	return retval;
}

// VMFImport
CValue VMFImport( void )
{
	CValueArray args(0);
	CValue retval;
	LONG i(0);


	CStatus st = gApp.ExecuteCommand( L"VMFImport", args, retval );

	return retval;
}

// WeightExport
CValue WeightExport( const CValue& /*number*/ in_arg0 )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_arg0;

	CStatus st = gApp.ExecuteCommand( L"WeightExport", args, retval );

	return retval;
}

// WeightImport
CValue WeightImport( const CValue& /*number*/ in_arg0 )
{
	CValueArray args(1);
	CValue retval;
	LONG i(0);

	args[i++]= in_arg0;

	CStatus st = gApp.ExecuteCommand( L"WeightImport", args, retval );

	return retval;
}

// WeldPoints
CValue WeldPoints( const CValue& /*undefined*/ in_connectionset, siOperationMode in_immediatemode )
{
	CValueArray args(2);
	CValue retval;
	LONG i(0);

	args[i++]= in_connectionset;
	args[i++]= (LONG)in_immediatemode;

	CStatus st = gApp.ExecuteCommand( L"WeldPoints", args, retval );

	return retval;
}

};
};
