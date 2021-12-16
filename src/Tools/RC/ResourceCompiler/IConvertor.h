////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   convertor.h
//  Version:     v1.00
//  Created:     4/11/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __convertor_h__
#define __convertor_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "ConvertContext.h"

// [11/5/2007 MichaelS]
// In order to support multithreading on particular convertors (initially the TIF convertor), I have split up
// the convertor class into a Convertor and a Compiler. The Converter does no actual work, it merely describes
// the work that the compiler will do, and can create compiler instances to do the actual processing.
//
// For converters that support it, it is possible to create more than one compiler and run them in multiple
// threads.

/** Compiler interface, all compilers must implement this interface.
*/
struct ICompiler
{
	//! Release memory of interface.
	virtual void Release() = 0;

	//! Process file
	//! \return success
	virtual bool Process(ConvertContext &cc) = 0;

	//! Construct the name of the file that will be produced from the source file
	//! and call ConvertContext::SetOutputFile().
	// @param cc is the context of conversion; it contains valid sourceFile.
	virtual void ConstructAndSetOutputFile(ConvertContext &cc) = 0;

	//! Returns name of the file which will be used by RC in
	//! its "up-t-to-date check": RC will compare timestamp of this
	//! file and timestamp of the source file (RC will not perform 
	//! compilation if timestamps are equal). If compilation
	//! finished successfully, then RC will change timestamp
	//! of this file to the timestamp of the source.
	//! If the filename is "", then RC skips the "up-to-date check".
	// @param cc is the context of conversion; it contains valid sourceFile.
	// @param filenameBuffer is the buffer for the filename
	// @param bufferSize size of the buffer in bytes
	virtual void GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const = 0;
};

/** Convertor interface, all convertor must implement this interface.
*/
struct IConvertor
{
	//! Release memory of interface.
	virtual void Release() = 0;

	//! Initialize the convertor.
	// @param cc is the context of conversion; it contains valid directory and may contain fallback instruction.
	virtual void Init(IConfig* config, const char* exePath) = 0;

	//! Finalize the convertor.
	// @param cc is the context of conversion; it contains valid directory and may contain fallback instruction.
	virtual void DeInit() = 0;

	//! Return an object that will do the actual processing. For some implementations, this may be
	//! the original object. Should not be called more than once if SupportsMultithreading() returns false.
	//! Otherwise multiple compilers can be called and run from separate threads.
	virtual ICompiler* CreateCompiler() = 0;

	//! Check whether the convertor supports multithreading. See CreateCompiler() for more details.
	virtual bool SupportsMultithreading() const = 0;

	//! Return platforms supported by this convertor.
	virtual int GetNumPlatforms() const = 0;
	//! Get supported platform.
	//! @param index Index of platform must be in range 0 < index < GetNumPlatforms().
	virtual EPlatform GetPlatform(int index) const = 0;

	//! Get number of supported extensions.
	virtual int GetNumExt() const = 0;
	//! Get supported extension.
	//! @param index Index of extension must be in range 0 < index < GetNumExt().
	virtual const char* GetExt(int index) const = 0;

	// this should retrieve the timestamp of the convertor executable:
	// when it was created by the linker, normally. This date/time is used to
	// compare with the compiled file date/time and even if the compiled file
	// is not older than the source file, it will be recompiled if it's older than the
	// convertor
	virtual DWORD GetTimestamp() const = 0;

	// Create a convert context object. It is necessary to have the convertor
	// create this object rather than creating it in the main EXE to work
	// correctly across DLL boundaries.
	virtual IConvertContext* CreateConvertContext() const = 0;
};

#endif // __convertor_h__
