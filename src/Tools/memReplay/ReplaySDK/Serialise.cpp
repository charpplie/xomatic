#include "stdafx.h"

FileSerialiser::FileSerialiser(const char* filename)
{
	m_fp = fopen(filename, "wb");
}

FileSerialiser::~FileSerialiser()
{
	if (m_fp)
		fclose(m_fp);
}

void FileSerialiser::WriteRaw(const void* data, size_t dataSize)
{
	if (m_fp)
		fwrite(data, dataSize, 1, m_fp);
}

FileDeserialiser::FileDeserialiser(const char* filename)
{
	m_fp = fopen(filename, "rb");
}

FileDeserialiser::~FileDeserialiser()
{
	if (m_fp)
		fclose(m_fp);
}

void FileDeserialiser::ReadRaw(void* data, size_t dataSize)
{
	if (m_fp)
		fread(data, dataSize, 1, m_fp);
}
