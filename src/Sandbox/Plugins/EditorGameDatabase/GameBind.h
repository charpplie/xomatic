////////////////////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2014.
// ----------------------------------------------------------------------------------------
//  File name:   GameBind
//  Description: Helper class to keep references to game descriptor database
//
//////////////////////////////////////////////////////////////////////////////////////////// 

#ifndef _GAME_BIND_H_
#define _GAME_BIND_H_

#pragma once

namespace Descriptor
{
	struct ITypeLibrary;
	struct IDatabaseEdit;
}

struct IEditor;

class GameBind
{
public:
	GameBind();
	~GameBind();

	bool Init( IEditor* pEditor );

	const Descriptor::ITypeLibrary& GetDescriptorLibrary() const 
	{ 
		return *m_pDescriptorLibrary; 
	}
	Descriptor::ITypeLibrary& GetDescriptorLibrary() 
	{ 
		return *m_pDescriptorLibrary; 
	}

	const Descriptor::IDatabaseEdit& GetDatabase() const 
	{ 
		return *m_pDatabase;
	}
	Descriptor::IDatabaseEdit& GetDatabase() 
	{ 
		return *m_pDatabase; 
	}

	ISourceControl* GetSourceControl() const;

	static GameBind& Get()
	{
		assert(s_pThis != NULL);

		return *s_pThis;
	}

private:
	Descriptor::ITypeLibrary*      m_pDescriptorLibrary;
	Descriptor::IDatabaseEdit*     m_pDatabase;

	IEditor*                       m_pEditor;

	static GameBind* s_pThis;
};

#endif 

