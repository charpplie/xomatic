// CryEngine Header File.
// Copyright (C), Crytek, 1999-2014.

#ifndef BehaviorTreeDescriptor_h
#define BehaviorTreeDescriptor_h

#pragma once

#include <BehaviorTree/IBehaviorTree.h>

class BehaviorTreeDocument
{
public:
	BehaviorTreeDocument();

	void Reset();
	bool Loaded();

	bool Changed();
	void SetChanged();

	void Serialize( Serialization::IArchive& archive );

	void NewFile( const char* behaviorTreeName, const char* absoluteFilePath );
	bool OpenFile( const char* behaviorTreeName, const char* absoluteFilePath );
	bool Save();
	bool SaveToFile( const char* behaviorTreeName, const char* absoluteFilePath );

private:
	XmlNodeRef GenerateBehaviorTreeXml();

	bool m_changed;
	string m_behaviorTreeName;
	string m_absoluteFilePath;
	BehaviorTree::BehaviorTreeTemplatePtr m_behaviorTreeTemplate;
};

#endif // BehaviorTreeDescriptor_h
