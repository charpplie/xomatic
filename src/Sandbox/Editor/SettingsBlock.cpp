#include "StdAfx.h"
#include "SettingsBlock.h"
#include "Serialization.h"
#include <Serialization/ITextInputArchive.h>
#include <Serialization/ITextOutputArchive.h>

using std::vector;

SProjectSettingsBlock* SProjectSettingsBlock::s_pLastBlock;

SProjectSettingsBlock::SProjectSettingsBlock(const char* name, const char* label)
: m_name(name)
, m_label(label)
{
	m_pPrevious = s_pLastBlock;
	s_pLastBlock = this;
}


struct SAllSettingsSerializer
{
	void Serialize(Serialization::IArchive& ar)
	{
		SProjectSettingsBlock* pCurrent = SProjectSettingsBlock::s_pLastBlock;
		while (pCurrent != 0)
		{
			ar(*pCurrent, pCurrent->GetName(), pCurrent->GetLabel());
			pCurrent = pCurrent->m_pPrevious;
		}
	}
} static gAllSettingsSerializer;

void SProjectSettingsBlock::GetAllSettingsSerializer(Serialization::SStruct* pSerializer)
{
	*pSerializer = Serialization::SStruct(gAllSettingsSerializer);
}

SProjectSettingsBlock* SProjectSettingsBlock::Find(const char* blockName)
{
	SProjectSettingsBlock* pCurrent = SProjectSettingsBlock::s_pLastBlock;
	while (pCurrent != 0)
	{
		if (stricmp(pCurrent->GetName(), blockName) == 0)
			return pCurrent;
	}
	return 0;
}

static bool ReadFileContent(vector<char>* pBuffer, const char* filename)
{
	FILE* f = gEnv->pCryPak->FOpen(filename, "rb");
	if (!f)
		return false;

	size_t size = gEnv->pCryPak->FGetSize(f);
	pBuffer->resize(size);

	bool result = true;
	if (gEnv->pCryPak->FRead(&(*pBuffer)[0], size, f) != size)
	{
		result = false;
	}
	gEnv->pCryPak->FClose(f);
	return result;
}

static bool SaveFileContent(const char* filename, const char* pBuffer, size_t length)
{
	string fullpath = Path::GamePathToFullPath( filename, true ).GetString();

	FILE* f = fopen(fullpath.c_str(), "wb");
	if (!f)
		return false;

	bool result = true;
	if (fwrite(pBuffer, 1, length, f) != length)
		result = false;
	
	fclose(f);
	return result;
}

static bool SaveFileContentIfDiffers(const char* filename, const char* pBuffer, size_t length)
{
	vector<char> content;
	ReadFileContent(&content, filename);

	bool needToWrite = true;
	if (!content.empty() && content.size() == length)
		needToWrite = memcmp(&content[0], pBuffer, length) != 0;

	if (needToWrite)
		return SaveFileContent(filename, pBuffer, length);
	else
		return true;
}

bool SProjectSettingsBlock::Load()
{
	const char* filename = GetFilename();

	vector<char> content;
	if (!ReadFileContent(&content, filename))
		return false;

	auto pArchive(Serialization::CreateTextInputArchive());
	if (!pArchive)
		return false;

	if (!pArchive->AttachMemory(&content[0], content.size()))
		return false;

	Serialization::SStruct serializer;
	GetAllSettingsSerializer(&serializer);
	serializer(*pArchive);
	return true;
}

bool SProjectSettingsBlock::Save()
{
	const char* filename = GetFilename();
	auto pArchive(Serialization::CreateTextOutputArchive());
	if (!pArchive)
		return false;

	Serialization::SStruct serializer;
	GetAllSettingsSerializer(&serializer);
	serializer(*pArchive);

	return SaveFileContentIfDiffers(filename, pArchive->GetBuffer(), pArchive->GetBufferLength());
}

const char* SProjectSettingsBlock::GetFilename()
{
	return "SandboxSettings.json";
}

