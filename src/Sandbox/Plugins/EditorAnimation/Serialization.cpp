#include "Serialization.h"

namespace CharacterTool
{

void SerializeToMemory(std::vector<char>* buffer, const Serialization::SStruct& obj)
{
	MemoryOArchive oa;
	oa(obj);
	buffer->assign(oa.buffer(), oa.buffer() + oa.length());
}

void SerializeToMemory(DynArray<char>* buffer, const Serialization::SStruct& obj)
{
	MemoryOArchive oa;
	oa(obj);
	buffer->assign(oa.buffer(), oa.buffer() + oa.length());
}

void SerializeFromMemory(const Serialization::SStruct& obj, const std::vector<char>& buffer)
{
	MemoryIArchive ia;
	if (!ia.open(buffer.data(), buffer.size()))
		return;
	ia(obj);
}

}