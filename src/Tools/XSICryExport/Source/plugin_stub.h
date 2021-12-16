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

/*! \file plugin_stub.h   
    XSI plugin stub functions. This is where the plugin glue is defined.
*/

#ifndef _PLUGIN_STUB_H
#define _PLUGIN_STUB_H

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DOTXSICONVERTER_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DOTXSICONVERTER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

// Note that we use extern "C" to avoid having to use def files for unmangling the C entry points
// For this purpose we just ignore the 4190 warning 
#pragma warning( disable : 4190)
#include <xsi_status.h>
#include <xsi_pluginregistrar.h>

#include <stdio.h>
#include <stdlib.h>
#include "cmdstubs.h"

/****************************************************************************************
To make sure we use the XSI namespace
****************************************************************************************/
using namespace XSI;


/****************************************************************************************
Global defines
****************************************************************************************/
#define TOOL_VERSION L"6.0.0"


/****************************************************************************************
XSI SDK registration functions
****************************************************************************************/
//! Entry point for registering XSI plugins
/*!
    \param in_reg the PluginRegistrar instance
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus XSILoadPlugin( PluginRegistrar& in_reg );

/****************************************************************************************

EXPORT SECTION

****************************************************************************************/
/****************************************************************************************
XSI plugin Command definition functions
****************************************************************************************/
//! Entry point for initializing the ExportFTK command
/*!
    \param in_context the context in which the command operates
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus ExportFTK_Init( const CRef& in_context );


//! Entry point for executing the ExportdotXSI command
/*!
    \param in_context the context in which the command operates
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus ExportFTK_Execute( CRef& in_context );

/****************************************************************************************
XSI plugin menu event functions
****************************************************************************************/
//! Entry point for the event response to the Export->dotXSI menu item
/*!
    \param in_ref 
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus OnExportDotXSI5Menu( CRef& in_ref );

//! Entry point for registering Export->dotXSI menu item
/*!
    \param in_ref 
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus ExportDotXSI5Menu_Init( CRef& in_ref );

/****************************************************************************************
XSI plugin custom properties function
****************************************************************************************/

//! Entry point for defining the ExportFTK options
/*!
    \param in_Ctx the context
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus ExportFTKOptions_Define( const CRef & in_Ctx );

//! Entry point for defining the layout for the ExportFTK options
/*!
    \param in_Ctx the context
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus ExportFTKOptions_DefineLayout( const CRef & in_Ctx );

//! Entry point for defining the ExportFTK options PPG events response
/*!
    \param io_Ctx the context
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus ExportFTKOptions_PPGEvent( const CRef& io_Ctx );

/****************************************************************************************
XSI plugin export CreateOptions Command definition functions
****************************************************************************************/
//! Entry point for initializing the CreateExportFTKOptions command
/*!
    \param in_context the context in which the command operates
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus CreateExportFTKOptions_Init( const CRef& in_context );


//! Entry point for executing the CreateExportFTKOptions command
/*!
    \param in_context the context in which the command operates
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus CreateExportFTKOptions_Execute( CRef& in_context );

/****************************************************************************************

IMPORT SECTION

****************************************************************************************/
/****************************************************************************************
XSI plugin Command definition functions
****************************************************************************************/
//! Entry point for initializing the ImportFTK command
/*!
    \param in_context the context in which the command operates
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus ImportFTK_Init( const CRef& in_context );

//! Entry point for executing the ImportFTK command
/*!
    \param in_context the context in which the command operates
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus ImportFTK_Execute( CRef& in_context );

/****************************************************************************************
XSI plugin menu event functions
****************************************************************************************/
//! Entry point for the event response to the Import->dotXSI menu item
/*!
    \param in_ref 
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus OnImportDotXSI5Menu( CRef& in_ref );

//! Entry point for registering Import->dotXSI menu item
/*!
    \param in_ref 
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus ImportDotXSI5Menu_Init( CRef& in_ref );

/****************************************************************************************
XSI plugin custom properties function
****************************************************************************************/
//! Entry point for defining the ImportFTK options
/*!
    \param in_Ctx the context
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus ImportFTKOptions_Define( const CRef & in_Ctx );

//! Entry point for defining the layout for the ImportFTK options
/*!
    \param in_Ctx the context
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus ImportFTKOptions_DefineLayout( const CRef & in_Ctx );

//! Entry point for defining the ImportFTK options PPG events response
/*!
    \param io_Ctx the context
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus ImportFTKOptions_PPGEvent( const CRef& io_Ctx );

/****************************************************************************************
XSI plugin import CreateOptions Command definition functions
****************************************************************************************/
//! Entry point for initializing the CreateImportFTKOptions command
/*!
    \param in_context the context in which the command operates
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus CreateImportFTKOptions_Init( const CRef& in_context );

//! Entry point for executing the CreateImportFTKOptions command
/*!
    \param in_context the context in which the command operates
    \return CStatus to know whether the function failed or succeeded
*/
_XSI_EXTERN_ CStatus CreateImportFTKOptions_Execute( CRef& in_context );

extern bool IsCryExportNode(ProjectItem &in_ProjectItem);
extern ProjectItem GetCryExportNode(ProjectItem &in_ProjectItem);
extern void GetNodesUnderNode(X3DObject in_Object, CRefArray &out_NodeList);
extern void GetNodeNamesUnderNode(X3DObject in_Object, CStringArray &out_NodeList);
extern CStringArray GetSelectedMaterials();

extern XSI::CString	gOldPath;

#define VERSION_MAJOR	3
#define VERSION_MINOR	4

#endif _PLUGIN_STUB_H