#include "stdafx.h"
#include "plugin_stub.h"
#include <commdlg.h>

#include <gl/GL.h>

//****************************************************************************************
// Crytek begin render 
//****************************************************************************************

_XSI_EXTERN_ void	CrytekDisplay_Execute	( XSI::CRef in_pSequencer, LPVOID *in_pInstanceData )
{ 
	XSI::GraphicSequencerContext l_vGraphicSequencerContext = in_pSequencer;
	assert ( l_vGraphicSequencerContext.IsValid() );

	glDisable(GL_BLEND);

	l_vGraphicSequencerContext.GetGraphicSequencer().RenderSceneUsingMaterial ( L"CrytekVertexColor", XSI::siRenderDefault );
}

_XSI_EXTERN_ void	CrytekDisplay_Init ( XSI::CRef in_pSequencer, LPVOID *in_pInstanceData )
{
	XSI::GraphicSequencerContext l_vGraphicSequencerContext(in_pSequencer);
	assert ( l_vGraphicSequencerContext.IsValid() );

	l_vGraphicSequencerContext.GetGraphicSequencer().RegisterDisplayCallback (L"CrytekDisplay", 0, XSI::siPass, XSI::siCustom, L"CrytekDisplay" );
}

_XSI_EXTERN_ void	CrytekDisplay_Term ( XSI::CRef in_pSequencer, LPVOID *in_pInstanceData )
{

}

_XSI_EXTERN_ void	CrytekDisplay_InitInstance ( XSI::CRef in_pSequencer, LPVOID *in_pInstanceData )
{

}

/****************************************************************************************
XSI plugin custom properties function
****************************************************************************************/
_XSI_EXTERN_ CStatus CrytekDisplayOptions_Define( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;
	Application app ;
	CustomProperty prop = Context(in_Ctx).GetSource() ;
	Parameter param ;

	// Default capabilities for most of these parameters
	int caps = siPersistable;
	CValue dft ;	// Used for arguments we don't want to set

	dft = (LONG)0;
	prop.AddParameter(	L"Channel",CValue::siString, caps,
		L"Channel", L"",
		dft, param ) ;

	prop.AddParameter(	L"Channel",CValue::siUInt4 , caps, 
		L"Channel", L"", 
		dft, param ) ;	

	param.PutValue(CValue((LONG)0L));

	return status;
}

_XSI_EXTERN_ CStatus CrytekDisplayOptions_RebuildLayout( PPGLayout &in_Layout, CustomProperty &in_CustomProp )
{
	CStatus status = CStatus::OK;
	CString l_Empty;

	// XSI will call this to define the visual appearance of the CustomProperty
	// The layout is shared between all instances of the CustomProperty
	// and is cached.  You can force the code to re-execute by using the 
	// XSIUtils.Refresh feature.

	PPGItem item ;

	in_Layout.Clear();
	in_Layout.AddRow();
	in_Layout.AddGroup(L"Vertex Color Display Channel", true, 75);
	CValueArray comboItemsFormat( 10 ) ;
	comboItemsFormat[0] = L"Red"; comboItemsFormat[1] = CValue((LONG)0) ;
	comboItemsFormat[2] = L"Green"; comboItemsFormat[3] = CValue((LONG)1) ;
	comboItemsFormat[4] = L"Blue";  comboItemsFormat[5] = CValue((LONG)2) ;
	comboItemsFormat[6] = L"Alpha"; comboItemsFormat[7] = CValue((LONG)3) ;
	comboItemsFormat[8] = L"All (RGB)"; comboItemsFormat[9] = CValue((LONG)4) ;
	item = in_Layout.AddEnumControl( L"Channel", comboItemsFormat, l_Empty, siControlCombo ) ;
	item.PutAttribute( siUINoLabel, true ) ;
	in_Layout.EndGroup();
	in_Layout.EndRow();

	in_Layout.AddRow();
	in_Layout.AddGroup(L"Vertex Color Commands", true, 75);
	in_Layout.AddButton( L"SetBrushColortoRed", L"Set Brush Color to Red" );
	in_Layout.AddButton( L"SetBrushColortoGreen", L"Set Brush Color to Green" );
	in_Layout.AddButton( L"SetBrushColortoBlue", L"Set Brush Color to Blue" );
	in_Layout.EndGroup();

	return status;
}

_XSI_EXTERN_ CStatus CrytekDisplayOptions_DefineLayout( const CRef & in_Ctx )
{
	CStatus status = CStatus::OK;

	// XSI will call this to define the visual appearance of the CustomProperty
	// The layout is shared between all instances of the CustomProperty
	// and is cached.  You can force the code to re-execute by using the 
	// XSIUtils.Refresh feature.

	PPGLayout oLayout = Context( in_Ctx ).GetSource() ;

	CustomProperty cusProp;
	status = CrytekDisplayOptions_RebuildLayout(oLayout, cusProp);

	return status;
}

_XSI_EXTERN_ CStatus CrytekDisplayOptions_PPGEvent( const CRef& io_Ctx )
{
	CStatus status = CStatus::OK;
	Application app ;

	PPGEventContext ctx( io_Ctx ) ;

	//Redraw the PPG to show the new combo items
	ctx.PutAttribute(L"Refresh",true);

	PPGEventContext::PPGEvent eventID = ctx.GetEventID() ;

	if ( eventID == PPGEventContext::siParameterChange )
	{		
		Parameter changed = ctx.GetSource() ;	
		CustomProperty prop = changed.GetParent() ;	
		CString   paramName = changed.GetScriptName() ;
	}
	if ( eventID == PPGEventContext::siButtonClicked )
	{
		CustomProperty prop = ctx.GetSource();

		CValue l_ButtonName = ctx.GetAttribute( L"Button" );
		if (l_ButtonName.GetAsText() == L"SetBrushColortoRed")
		{
			XSI::COMMANDS::SetValue(L"BrushProperties.ColorPaintMode", 1, 0.0f);
			XSI::COMMANDS::SetValue(L"BrushProperties.Red", CValue(1.0f), 0.0f);
			XSI::COMMANDS::SetValue(L"BrushProperties.Green", CValue(0.0f), 0.0f);
			XSI::COMMANDS::SetValue(L"BrushProperties.Blue", CValue(0.0f), 0.0f);
			XSI::COMMANDS::SetValue(L"BrushProperties.Alpha", CValue(0.0f), 0.0f);
		}
		if (l_ButtonName.GetAsText() == L"SetBrushColortoGreen")
		{
			XSI::COMMANDS::SetValue(L"BrushProperties.ColorPaintMode", 1, 0.0f);
			XSI::COMMANDS::SetValue(L"BrushProperties.Red", CValue(0.0f), 0.0f);
			XSI::COMMANDS::SetValue(L"BrushProperties.Green", CValue(1.0f), 0.0f);
			XSI::COMMANDS::SetValue(L"BrushProperties.Blue", CValue(0.0f), 0.0f);
			XSI::COMMANDS::SetValue(L"BrushProperties.Alpha", CValue(0.0f), 0.0f);
		}
		if (l_ButtonName.GetAsText() == L"SetBrushColortoBlue")
		{
			XSI::COMMANDS::SetValue(L"BrushProperties.ColorPaintMode", 1, 0.0f);
			XSI::COMMANDS::SetValue(L"BrushProperties.Red", CValue(0.0f), 0.0f);
			XSI::COMMANDS::SetValue(L"BrushProperties.Green", CValue(0.0f), 0.0f);
			XSI::COMMANDS::SetValue(L"BrushProperties.Blue", CValue(1.0f), 0.0f);
			XSI::COMMANDS::SetValue(L"BrushProperties.Alpha", CValue(0.0f), 0.0f);
		}
	}
	if( eventID == PPGEventContext::siOnInit )
	{
		CustomProperty prop = ctx.GetSource();
		PPGLayout oPPGLayout = prop.GetPPGLayout();		
		CrytekDisplayOptions_RebuildLayout(oPPGLayout, prop);

		//Redraw the PPG to show the new combo items
		ctx.PutAttribute(L"Refresh",true);
	}

	return status;
}

_XSI_EXTERN_ CStatus CryVertexColorPainting_Execute( CRef& in_ref )
{
	CStatus status = CStatus::OK;
	Application app;

	Property prop = app.GetActiveSceneRoot().GetProperties().GetItem( L"CrytekDisplayOptions" ) ;

	if (!prop.IsValid())
	{
		prop = app.GetActiveSceneRoot().AddProperty( L"CrytekDisplayOptions" ) ;
	}

	CValue retval = XSI::COMMANDS::InspectObj(L"CrytekDisplayOptions",CValue(),L"CrytekDisplayOptions",(LONG)siLock,false);

	return status;
}
