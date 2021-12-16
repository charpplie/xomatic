//---------------------------------------------------------------------------
// Copyright 2006 Crytek GmbH
// Created by: Michael Smith
//---------------------------------------------------------------------------
#ifndef __XMLBINARYREADER_H__
#define __XMLBINARYREADER_H__

#include "XMLBinaryHeaders.h"
#include "IXml.h"
#include "CryFile.h"

namespace XMLBinary
{
	class XMLBinaryReader
	{
	public:
		XMLBinaryReader();
		~XMLBinaryReader();

		static bool HasSignature(CCryFile* pFile);
		XmlNodeRef Parse( const char *filename );
		IError* GetError();

	private:
		CCryFile m_xmlFile;

		class CorruptError : public IError
		{
		public:
			virtual void GetDescription(int nBufferSize, char* szDescriptionBuffer);
		};

		class FileTypeError : public IError
		{
		public:
			virtual void GetDescription(int nBufferSize, char* szDescriptionBuffer);
		};

		bool ReadHeader(CCryFile* pFile, BinaryFileHeader& layout);
		void SetError(IError* pError);

		IError* pError;
	};
}

#endif //__XMLBINARYREADER_H__
