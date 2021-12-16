#pragma once

#include <Serialization/IArchive.h>

namespace Serialization
{

class CContextList
{
public:
	template<class T>
	void Update(T* contextObject)
	{
		for (size_t i = 0; i < links_.size(); ++i)
			if (links_[i]->type == TypeID::get<T>()) {			
				links_[i]->contextObject = (void*)contextObject;
				return;
			}

		SContextLink* newLink = new SContextLink;
		newLink->type = TypeID::get<T>();
		newLink->outer = links_.empty() ? connectedList_ : links_.back();
		newLink->contextObject = (void*)contextObject;
		tail_.outer = newLink;			 
		links_.push_back(newLink);
	}

	CContextList()
	{
		tail_.outer = 0;
		tail_.contextObject = 0;
		connectedList_ = 0;
	}

	explicit CContextList(SContextLink* connectedList)
	{
		tail_.outer = 0;
		tail_.contextObject = 0;
		connectedList_ = connectedList;
	}

	~CContextList()
	{
		for (size_t i = 0; i < links_.size(); ++i)
			delete links_[i];
		links_.clear();
	}

	SContextLink* Tail() { return &tail_; }
private:
	SContextLink tail_;
	std::vector<SContextLink*> links_;
	SContextLink* connectedList_;
};

}
