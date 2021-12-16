#pragma once

#include <vector>
#include <map>
#include "Strings.h"

namespace CharacterTool
{
using std::vector;
using std::map;

class DependencyManager
{
public:
	void SetDependencies(const char* asset, vector<string>& usedAssets);

	void FindUsers(vector<string>* users, const char* asset) const;
	void FindDepending(vector<string>* assets, const char* user) const;
private:
	typedef map<string, vector<string>, stl::less_stricmp<string> > UsedAssets;
	UsedAssets m_usedAssets;
	
	typedef map<string, vector<string>, stl::less_stricmp<string> > AssetUsers;
	AssetUsers m_assetUsers;
};

}
