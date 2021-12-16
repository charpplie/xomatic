#ifndef __MORPHDATA_H__
#define __MORPHDATA_H__

#include "IMorphData.h"

class MorphData : public IMorphData
{
public:
	MorphData();

	virtual void SetHandle(void* handle);
	virtual void AddMorph(void* handle, const char* name);
	virtual void* GetHandle() const;
	virtual int GetMorphCount() const;
	virtual void* GetMorphHandle(int morphIndex) const;

	std::string GetMorphName(int morphIndex) const;

private:
	struct Entry
	{
		Entry(void* handle, const std::string& name): handle(handle), name(name) {}
		void* handle;
		std::string name;
	};

	void* m_handle;
	std::vector<Entry> m_morphs;
};

#endif //__MORPHDATA_H__
