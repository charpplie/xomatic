#if !defined(__TREEPROFILE_MANAGER_H__)
#define __TREEPROFILE_MANAGER_H__

#include "NewBehaviourTree/Tree.h"

namespace NewBehaviourTree
{
	class TreeProfileManager
	{
		typedef std::map<string, TreeProfile> ProfileContainer;

		ProfileContainer				m_Profiles;

	public:

		TreeProfileManager();
		~TreeProfileManager();


		//load profiles from specified path
		bool								LoadProfiles		(const string &path);
		const TreeProfile*	GetProfile			(const string &name) const;
	};
};


#endif //#if !defined(__TREEPROFILE_MANAGER_H__)