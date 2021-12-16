// Copyright (c) 2012 Crytek GmbH
// Authors: Evgeny Andreeshchev, Alexander Kotliar
// Based on: Yasli - the serialization library.

#include "StdAfx.h"
#include <platform.h>
#include "Serialization/Assert.h"
#include "MemoryReader.h"
#include <stdlib.h>
#include <memory.h>

namespace Serialization{

MemoryReader::MemoryReader()
: size_(0)
, position_(0)
, memory_(0)
, ownedMemory_(false)
{
}


MemoryReader::MemoryReader(const void* memory, std::size_t size, bool ownAndFree)
: size_(size)
, position_((const char*)(memory))
, memory_((const char*)(memory))
, ownedMemory_(ownAndFree)
{

}

MemoryReader::~MemoryReader()
{
    if(ownedMemory_){
        free(const_cast<char*>(memory_));
        memory_ = 0;
        size_ = 0;
    }
}

void MemoryReader::setPosition(const char* position)
{
    position_ = position;
}

void MemoryReader::read(void* data, std::size_t size)
{
    YASLI_ASSERT(memory_ && position_);
    YASLI_ASSERT(position_ - memory_ + size <= size_);
    memcpy(data, position_, size);
    position_ += size;
}

bool MemoryReader::checkedRead(void* data, std::size_t size)
{
    if(!memory_ || !position_)
        return false;
    if(position_ - memory_ + size > size_)
        return false;

    memcpy(data, position_, size);
    position_ += size;
    return true;
}

bool MemoryReader::checkedSkip(std::size_t size)
{
    if(!memory_ || !position_)
        return false;
    if(position_ - memory_ + size > size_)
        return false;

    position_ += size;
    return true;
}

}
