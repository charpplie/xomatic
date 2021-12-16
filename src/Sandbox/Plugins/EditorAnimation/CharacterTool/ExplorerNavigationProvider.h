#pragma once

namespace Serialization { struct INavigationProvider; }

namespace CharacterTool
{
struct System;
Serialization::INavigationProvider* CreateExplorerNavigationProvider(System* system);
}
