#pragma once

#include <Cry_Math.h>
#include <Cry_Color.h>

namespace Serialization { class IArchive; }

struct SkeletonAlias;
bool Serialize(Serialization::IArchive& ar, SkeletonAlias& value, const char* name, const char* label);

#include <Serialization/STL.h>
#include <Serialization/Math.h>
#include <Serialization/Color.h>
#include <Serialization/Decorators/BitFlags.h>
using Serialization::BitFlags;
#include <Serialization/Decorators/Slider.h>
#include <Serialization/Decorators/Range.h>
#include "Serialization/Decorators/ToggleButton.h"
#include "Serialization/Qt.h"
#include <Serialization/ClassFactory.h>
#include <Serialization/Enum.h>

#include <Serialization/IArchive.h>

using Serialization::IArchive;
