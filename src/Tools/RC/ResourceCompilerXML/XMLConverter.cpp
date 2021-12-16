#include "stdafx.h"
#include "XMLConverter.h"
#include "IRCLog.h"
#include "IXmlSerializer.h"
#include "XmlBinaryHeaders.h"
#include "XMLBinaryReader.h"
#include "XMLBinaryWriter.h"
#include "DbgHelp.h"

const string XMLConverter::sk_sInputExtensions[] = {
	"xml"
};

XMLConverter::XMLConverter(ICryXML* pCryXML)
: pCryXML(pCryXML), m_refCount(1)
{
	pCryXML->AddRef();
}

XMLConverter::~XMLConverter()
{
	pCryXML->Release();
}

void XMLConverter::Release()
{
	if (--m_refCount <= 0)
		delete this;
}

//////////////////////////////////////////////////////////////////////////
class CXmlBinaryDataWriterFile : public XMLBinary::IDataWriter
{
public:
	CXmlBinaryDataWriterFile( const char *file ) { m_file = fopen( file,"wb" ); }
	~CXmlBinaryDataWriterFile() { if (m_file) fclose( m_file ); };
	virtual bool IsOk() { return m_file != 0; };
	virtual void Write(const void* pData, size_t size) { if (m_file) fwrite( pData,size,1,m_file ); }
private:
	FILE *m_file;
};


static bool xmlsAreEqual(XmlNodeRef node0, XmlNodeRef node1)
{
	{
		const char* const tag0 = node0->getTag();
		const char* const tag1 = node1->getTag();

		if (strcmp(tag0, tag1) != 0)
		{
			return false;
		}
	}

	{
		const char* const content0 = node0->getContent();
		const char* const content1 = node1->getContent();

		if (strcmp(content0, content1) != 0)
		{
			return false;
		}
	}

	const int attributeCount = node0->getNumAttributes();

	if (attributeCount != node1->getNumAttributes())
	{
		return false;
	}

	for (int i = 0; i < attributeCount; ++i)
	{
		const char* key0;
		const char* value0;
		node0->getAttributeByIndex(i, &key0, &value0);

		const char* key1;
		const char* value1;
		node1->getAttributeByIndex(i, &key1, &value1);

		if ((strcmp(key0, key1) != 0) ||
			(strcmp(value0, value1) != 0))
		{
			return false;
		}
	}

	const int childCount = node0->getChildCount();

	if (childCount != node1->getChildCount())
	{
		return false;
	}

	for (int i = 0; i < childCount; ++i)
	{
		XmlNodeRef child0 = node0->getChild(i);
		XmlNodeRef child1 = node1->getChild(i);
		const bool bEqual = xmlsAreEqual(child0, child1);
		if (!bEqual)
		{
			return false;
		}
	}

	return true;
}


bool XMLConverter::Process( ConvertContext &cc )
{
	const bool bNeedSwapEndian = (cc.platform == ePlatform_X360 || cc.platform == ePlatform_PS3);
	if (bNeedSwapEndian)
	{
		RCLog("Endian conversion specified");
	}

	// Get the files to process.
	const string sOutputFile = cc.getOutputPath();
	const string sInputFile = cc.getSourcePath();

	// Get the xml serializer.
	IXMLSerializer* pSerializer = pCryXML->GetXMLSerializer();

	// Read in the input file.
	XmlNodeRef root;
	{
		FILE* pSourceFile = fopen(sInputFile.c_str(), "rb");
		if (pSourceFile == 0)
		{
			RCLogError("Cannot open file \"%s\": %s\n", sInputFile.c_str(), strerror(errno));
			return false;
		}
		fclose(pSourceFile);
		const bool bRemoveNonessentialSpacesFromContent = true;
		char szErrorBuffer[1024];
		szErrorBuffer[0] = 0;
		root = pSerializer->Read(FileXmlBufferSource(sInputFile.c_str()), bRemoveNonessentialSpacesFromContent, sizeof(szErrorBuffer), szErrorBuffer);
		if (!root)
		{
			const char* const pErrorStr = 
				(szErrorBuffer[0])
				? &szErrorBuffer[0] 
				: "Probably this file has bad XML syntax or it's not XML file at all.";
			RCLogError("Cannot read file \"%s\": %s.\n", sInputFile.c_str(), pErrorStr);
			return false;
		}
	}

	// Write out the destination file.
	{
		FILE* pDestinationFile = fopen(sOutputFile.c_str(), "wb");
		if (pDestinationFile == 0)
		{
			RCLogError("Cannot open file \"%s\": %s\n", sInputFile.c_str(), strerror(errno));
			return false;
		}
		fclose(pDestinationFile);

		CXmlBinaryDataWriterFile outputFile(sOutputFile.c_str());
		XMLBinary::CXMLBinaryWriter xmlBinaryWriter;
		string error;
		const bool ok = xmlBinaryWriter.WriteNode(&outputFile, root, bNeedSwapEndian, error);
		if (!ok)
		{
			remove(sOutputFile.c_str());
			RCLogError("Failed to write binary xml file \"%s\": %s\n", sOutputFile.c_str(), error.c_str());
			return false;
		}
	}

	// Verify that the output file was written
	{
		FILE* pSourceFile = fopen(sOutputFile.c_str(), "rb");
		if (pSourceFile == 0)
		{
			RCLogError("Failed to write file \"%s\": %s\n", sOutputFile.c_str(), strerror(errno));
			return false;
		}
		fclose(pSourceFile);
	}

	// Check that the output binary XML file has same content as the input text XML file
	if (!bNeedSwapEndian) 
	{
		// Read in the input file.
		XmlNodeRef rootTxt;
		{
			const bool bRemoveNonessentialSpacesFromContent = true;
			char szErrorBuffer[1024];
			szErrorBuffer[0] = 0;
			rootTxt = pSerializer->Read(FileXmlBufferSource(sInputFile.c_str()), bRemoveNonessentialSpacesFromContent, sizeof(szErrorBuffer), szErrorBuffer);
			if (!rootTxt)
			{
				RCLogError("Cannot read file \"%s\" in second pass: %s.\n", sInputFile.c_str(), &szErrorBuffer[0]);
				return false;
			}
		}

		XMLBinary::XMLBinaryReader binReader;
		XmlNodeRef rootBin = binReader.Parse(sOutputFile.c_str());
		if (!rootBin)
		{
			RCLogError("Cannot read binary XML file \"%s\".\n", sOutputFile.c_str());
			return false;
		}

		if (!xmlsAreEqual(rootTxt, rootBin))
		{
			RCLogError("Source XML file \"%s\" and result binary XML file \"%s\" are different.", sInputFile.c_str(), sOutputFile.c_str());
			return false;
		}
	}

	return true;
}

void XMLConverter::ConstructAndSetOutputFile(ConvertContext &cc)
{
	string sExtension = PathHelpers::FindExtension(cc.sourceFileFinal);
	sExtension = "bin" + sExtension;
	cc.SetOutputFile(PathHelpers::ReplaceExtension(cc.sourceFileFinal, sExtension));
}

void XMLConverter::GetFilenameForUpToDateCheck(ConvertContext &cc, char* filenameBuffer, size_t bufferSize) const
{
	string const filename(cc.getOutputPath());

	if (filenameBuffer && (filename.length() < bufferSize))
	{
		strcpy(filenameBuffer, filename.c_str());		
	}
}

int XMLConverter::GetNumPlatforms() const
{
	return 3;
}

EPlatform XMLConverter::GetPlatform( int index ) const
{
	switch (index)
	{
	case 0:
		return ePlatform_PC;
	case 1:
		return ePlatform_X360;
	case 2:
		return ePlatform_PS3;
	default:
		return ePlatform_UNKNOWN;
	}
}

int XMLConverter::GetNumExt() const
{
	return sizeof(sk_sInputExtensions) / sizeof(sk_sInputExtensions[0]);
}

const char* XMLConverter::GetExt( int index ) const
{
	return sk_sInputExtensions[index].c_str();
}

DWORD XMLConverter::GetTimestamp() const
{
	return GetTimestampForLoadedLibrary(g_hInst);
}

//////////////////////////////////////////////////////////////////////////
ICompiler* XMLConverter::CreateCompiler()
{
	// Only ever return one compiler, since we don't support multithreading. Since
	// the compiler is just this object, we can tell whether we have already returned
	// a compiler by checking the ref count.
	if (m_refCount >= 2)
		return 0;

	// Until we support multithreading for this convertor, the compiler and the
	// convertor may as well just be the same object.
	++m_refCount;
	return this;
}

bool XMLConverter::SupportsMultithreading() const
{
	return false;
}
