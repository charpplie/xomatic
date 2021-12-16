#include "stdafx.h"
#include "XMLPakFileSink.h"
#include "StringHelpers.h"

XMLPakFileSink::XMLPakFileSink(IPakSystem* pakSystem, const string& archivePath, const string& filePath)
:	pakSystem(pakSystem), filePath(filePath)
{
	archive = pakSystem->OpenArchive(archivePath.c_str());
}

XMLPakFileSink::~XMLPakFileSink()
{
	if (archive && pakSystem)
	{
		pakSystem->AddToArchive(archive, filePath.c_str(), &data[0], int(data.size()));
		pakSystem->CloseArchive(archive);
	}
}

void XMLPakFileSink::Write(const char* text)
{
	string asciiText = text;
	int len = int(asciiText.size());
	int start = int(data.size());
	data.resize(data.size() + len);
	memcpy(&data[start], asciiText.c_str(), len);
}
