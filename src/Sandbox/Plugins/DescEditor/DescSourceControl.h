#ifndef __DescSourceControl_h__
#define __DescSourceControl_h__

#if _MSC_VER > 1000
# pragma once
#endif

namespace CryGame
{
	class CDescSourceControl
	{
	public:
		bool AddFile(const char* pPath);
		bool CheckoutFile(const char* pPath);
		bool DeleteFile(const char* pPath);
		bool RenameFile(const char* pOldPath, const char* pNewPath);

	protected:
		bool P4Command(const char* pCommand, const char* pPath, string& response);
	};
}  // namespace CryGame

#endif
