#pragma once

#include "Pointers.h"
#include "Serialization/IArchive.h"
#include "Serialization/MemoryReader.h"
#include "Token.h"
#include "EditorCommonAPI.h"
#include <memory>

namespace Serialization{

class MemoryReader;

class JSONIArchive : public IArchive{
public:
	JSONIArchive();
	~JSONIArchive();

	bool load(const char* filename);
	bool open(const char* buffer, size_t length, bool free = false);

	// virtuals:
	bool operator()(bool& value, const char* name = "", const char* label = 0);
	bool operator()(IString& value, const char* name = "", const char* label = 0);
	bool operator()(IWString& value, const char* name = "", const char* label = 0);
	bool operator()(float& value, const char* name = "", const char* label = 0);
	bool operator()(double& value, const char* name = "", const char* label = 0);
	bool operator()(int16& value, const char* name = "", const char* label = 0);
	bool operator()(uint16& value, const char* name = "", const char* label = 0);
	bool operator()(int32& value, const char* name = "", const char* label = 0);
	bool operator()(uint32& value, const char* name = "", const char* label = 0);
	bool operator()(int64& value, const char* name = "", const char* label = 0);
	bool operator()(uint64& value, const char* name = "", const char* label = 0);

	bool operator()(int8& value, const char* name = "", const char* label = 0);
	bool operator()(uint8& value, const char* name = "", const char* label = 0);
	bool operator()(char& value, const char* name = "", const char* label = 0);

	bool operator()(const SStruct& ser, const char* name = "", const char* label = 0);
	bool operator()(const SBlackBox& ser, const char* name = "", const char* label = 0);
	bool operator()(IContainer& ser, const char* name = "", const char* label = 0);
	bool operator()(IKeyValue& ser, const char* name = "", const char* label = 0);
	bool operator()(IPointer& ser, const char* name = "", const char* label = 0);

	using IArchive::operator();
private:
	bool findName(const char* name, Token* outName = 0);
	bool openBracket();
	bool closeBracket();

	bool openContainerBracket();
	bool closeContainerBracket();

	void checkValueToken();
	bool checkStringValueToken();
	void readToken();
	void putToken();
	int line(const char* position) const; 
	bool isName(Token token) const;

	bool expect(char token);
	void skipBlock();

	struct Level{
		const char* start;
		const char* firstToken;
		bool isContainer;
		bool isKeyValue;
		Level() : isContainer(false), isKeyValue(false) {}
	};
	typedef std::vector<Level> Stack;
	Stack stack_;

	std::auto_ptr<MemoryReader> reader_;
	Token token_;
	std::vector<char> unescapeBuffer_;
	std::string filename_;
	void* buffer_;
};

}
