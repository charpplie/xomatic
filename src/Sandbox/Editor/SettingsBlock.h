// (c) 2001-2012 Crytek GmbH
#pragma once

// ---------------------------------------------------------------------------
// Following utility can be used to add blocks of per-project settings. 
// Example:
//
// MyComponent.cpp:
//  
//   struct SProjectSettingsMy : SProjectSettingsBlock
//   {
//     bool bMyOption;
//
//     SProjectSettingsMy()
//     : SProjectSettingsBlock("my", "My")
//     , bMyOption(false)
//     {}
//     
//     void Serialize(Serialization::IArchive& ar)
//     {
//        ar(bMyOption, "myOption", "My Option");
//     }
//
//   } static gMySettings;
//
//
// Now gMySettings will be loaded and saved automatically and available for
// editing through:
//
//   GetIEditor()->OpenProjectSettings("my");
//
// ---------------------------------------------------------------------------

namespace Serialization
{
	class IArchive;
	struct SStruct;
};

struct SProjectSettingsBlock
{
	SProjectSettingsBlock(const char* name, const char* label);
	
	virtual void Serialize(Serialization::IArchive& ar) = 0;

	const char* GetName() const{ return m_name; }
	const char* GetLabel() const{ return m_label; }

	static void GetAllSettingsSerializer(Serialization::SStruct* serializer);
	static SProjectSettingsBlock* Find(const char* name);
	static bool Load();
	static bool Save();
	static const char* GetFilename();
private:
	const char* m_name;
	const char* m_label;
	SProjectSettingsBlock* m_pPrevious;
	static SProjectSettingsBlock* s_pLastBlock;
	friend struct SAllSettingsSerializer;
};
