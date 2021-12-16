#pragma once

#include <Cry_Math.h>

namespace Serialization { class IArchive; }

struct SkeletonAlias;

bool Serialize(Serialization::IArchive& ar, SkeletonAlias& value, const char* name, const char* label);

#include <Serialization/STL.h>
#include <Serialization/Decorators/Range.h>
#include <Serialization/Decorators/OutputFilePath.h>
using Serialization::OutputFilePath;
#include <Serialization/Decorators/ResourceFilePath.h>
using Serialization::ResourceFilePath;
#include <Serialization/Decorators/JointName.h>
using Serialization::JointName;

#include <Serialization/IArchive.h>

#include <Serialization/SmartPtr.h>
#include <Serialization/STLImpl.h>
#include <Serialization/SmartPtrImpl.h>
#include <Serialization/Decorators/SliderImpl.h>
#include <Serialization/Decorators/OutputFilePathImpl.h>
#include <Serialization/Decorators/ResourceFilePathImpl.h>
#include <Serialization/Decorators/JointNameImpl.h>

#include <Serialization/ClassFactory.h>
#include <Serialization/Enum.h>
using Serialization::IArchive;
