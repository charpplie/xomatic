#include "stdafx.h"
#include "SizeInfo.h"

void SizeInfoGroups::Serialise(ISerialiser& ser) const
{
	for (int i = 0; i != MemGroups::Count; ++ i)
	{
		ser.Write((int) m_groups[i].allocCount);
		ser.Write((int) m_groups[i].freeCount);
		ser.Write((int) m_groups[i].requested);
		ser.Write((int) m_groups[i].consumed);
		ser.Write((int) m_groups[i].global);
	}

	ser.Write((int) m_total.allocCount);
	ser.Write((int) m_total.freeCount);
	ser.Write((int) m_total.requested);
	ser.Write((int) m_total.consumed);
	ser.Write((int) m_total.global);
}

void SizeInfoGroups::Deserialise(IDeserialiser& ser)
{
	for (int i = 0; i != MemGroups::Count; ++ i)
	{
		int allocCount = ser.Read<int>();
		int freeCount = ser.Read<int>();
		int requested = ser.Read<int>();
		int consumed = ser.Read<int>();
		int global = ser.Read<int>();
		m_groups[i] = SizeInfo(allocCount, freeCount, requested, consumed, global);
	}

	{
		int allocCount = ser.Read<int>();
		int freeCount = ser.Read<int>();
		int requested = ser.Read<int>();
		int consumed = ser.Read<int>();
		int global = ser.Read<int>();
		m_total = SizeInfo(allocCount, freeCount, requested, consumed, global);
	}
}
