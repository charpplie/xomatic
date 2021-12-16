// ScriptObject.h: interface for the CScriptTable class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCRIPTOBJECT_H__6EA3E6D6_4FF9_4709_BD62_D5A97C40DB68__INCLUDED_)
#define AFX_SCRIPTOBJECT_H__6EA3E6D6_4FF9_4709_BD62_D5A97C40DB68__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IScriptSystem.h"
extern "C"{
//#include <lua.h>
#include "lua/lua.h"

}
class CScriptSystem;

enum {
	DELETED_REF = -1,
	NULL_REF = 0,
};

#ifdef DEBUG_LUA_STATE
extern std::set<class CScriptTable*> gAllScriptTables;
#endif

/*! IScriptTable implementation
	@see IScriptTable
*/
class CScriptTable : public IScriptTable  
{
public:
	//! constructor
	CScriptTable() { m_nRef = NULL_REF; m_nRefCount = 0; } // Dont need constructor (using custom pool alloc)

	// interface IScriptTable ----------------------------------------------------------------
	virtual void AddRef() { m_nRefCount++; }
	virtual void Release() { if (--m_nRefCount <= 0) DeleteThis(); };
	
	VIRTUAL IScriptSystem* GetScriptSystem() const;
	VIRTUAL void Delegate( IScriptTable *pMetatable );

	// quick workaround for user data
	VIRTUAL void * GetUserDataValue();

	//////////////////////////////////////////////////////////////////////////
	// Set/Get chain.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL bool BeginSetGetChain(); 
	VIRTUAL void EndSetGetChain(); 

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SetValueAny( const char *sKey,const ScriptAnyValue &any,bool bChain=false );
	VIRTUAL bool GetValueAny( const char *sKey,ScriptAnyValue &any,bool bChain=false );

	//////////////////////////////////////////////////////////////////////////
	VIRTUAL void SetAtAny( int nIndex,const ScriptAnyValue &any );
	VIRTUAL bool GetAtAny( int nIndex,ScriptAnyValue &any );
	
	VIRTUAL ScriptVarType GetValueType( const char *sKey );
	VIRTUAL ScriptVarType GetAtType( int nIdx );
	
	//////////////////////////////////////////////////////////////////////////
	// Iteration.
	//////////////////////////////////////////////////////////////////////////
	VIRTUAL IScriptTable::Iterator BeginIteration();
	VIRTUAL bool MoveNext( Iterator &iter );
	VIRTUAL void EndIteration( const Iterator &iter );
	//////////////////////////////////////////////////////////////////////////

	VIRTUAL void Clear();
	VIRTUAL int  Count();
	VIRTUAL bool Clone( IScriptTable *pSrcTable,bool bDeepCopy=false );
	VIRTUAL void Dump(IScriptTableDumpSink *p);
	
	VIRTUAL bool AddFunction( const SUserFunctionDesc &fd );

	virtual bool GetValueRecursive( const char *szPath, IScriptTable *pObj );

	// --------------------------------------------------------------------------
	void CreateNew();

	int  GetRef();
	void Attach();
	void AttachToObject( IScriptTable *so );
	void DeleteThis();

	// Create object from pool.
	void Recreate() { m_nRef = NULL_REF; m_nRefCount=1; };
	// Assign a metatable to a table.
	void SetMetatable( IScriptTable *pMetatable );
	// Push reference of this object to the stack.
	void PushRef();
	// Push reference to specified script table to the stack.
	void PushRef( IScriptTable *pObj );

	//////////////////////////////////////////////////////////////////////////
	// Custom new/delete.
	//////////////////////////////////////////////////////////////////////////
	void* operator new( size_t nSize );
	void operator delete( void *ptr );

public:
	// Lua state, set by CScriptSystem::Init
	static lua_State* L;
	// Pointer to ScriptSystem, set by CScriptSystem::Init
	static CScriptSystem *m_pSS;

private:
	static int StdCFunction( lua_State *L );
	static int StdCUserDataFunction( lua_State *L );

	static void CloneTable( int srcTable,int trgTable );
	static void CloneTable_r( int srcTable,int trgTable );

private:
	int m_nRefCount;
	int m_nRef;
};

//////////////////////////////////////////////////////////////////////////
typedef stl::PoolAllocatorNoMT<sizeof(CScriptTable)> CScriptTable_PoolAlloc;
extern CScriptTable_PoolAlloc *g_pScriptTablePoolAlloc;

//////////////////////////////////////////////////////////////////////////
inline void* CScriptTable::operator new( size_t nSize )
{
	void *ptr = g_pScriptTablePoolAlloc->Allocate();
	if (ptr)
		memset( ptr,0,nSize ); // Clear objects memory.
	return ptr;
}
//////////////////////////////////////////////////////////////////////////
inline void CScriptTable::operator delete( void *ptr )
{
	if (ptr)
		g_pScriptTablePoolAlloc->Deallocate(ptr);
}

namespace ScriptTableAllocator
{
	inline void GetMemoryUsage(ICrySizer *pSizer )
	{
		pSizer->AddObject(g_pScriptTablePoolAlloc);
	}
}

#endif // !defined(AFX_SCRIPTOBJECT_H__6EA3E6D6_4FF9_4709_BD62_D5A97C40DB68__INCLUDED_)
