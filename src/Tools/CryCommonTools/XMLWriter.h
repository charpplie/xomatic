#ifndef __XMLWRITER_H__
#define __XMLWRITER_H__

#include "Exceptions.h"

#include <cstdio>
#include <vector>
#include <cassert>

class IXMLSink
{
public:
	// Define an exception type to throw when file opening fails.
	struct OpenFailedErrorTag {};
	typedef Exception<OpenFailedErrorTag> OpenFailedError;

	virtual void Write(const char* text) = 0;
};

class XMLFileSink : public IXMLSink
{
public:
	XMLFileSink(const string& name);
	~XMLFileSink();

	virtual void Write(const char* text);

private:
	FILE* m_file;
};

class XMLWriter
{
public:
	XMLWriter(IXMLSink* sink);

	class Element
	{
	public:
		Element(XMLWriter& writer, const string& name, bool output=true);
		~Element();

		template <typename T> void Attribute(const string& name, const T& value);
		void Child(const string& name, const string& value);
		void Content(const string& text);
		void ContentLine(const string& text);
		template <typename T> void ContentArrayElement(const T& value);

	private:
		XMLWriter& m_writer;
		string m_name;
		bool isParent;
		bool m_output;
	};

private:
	void IncreaseIndentation();
	void DecreaseIndentation();

	void BeginElement(const string& name);
	void EndElement(const string& name);
	void CloseElement(const string& name, bool newLine);
	void CloseLeafElement(const string& name);

	void WriteAttribute(const string& name, const string& value);
	static void SerializeAttribute(char* buffer, const string& value);
	static void SerializeAttribute(char* buffer, float value);
	static void SerializeAttribute(char* buffer, int value);
	static void SerializeArrayElement(char* buffer, float value);
	static void SerializeArrayElement(char* buffer, const string& value);
	static void SerializeArrayElement(char* buffer, int value);
	void WriteContent(const string& text);
	void WriteContentLine(const string& text);

	void WriteText(const char* format, ...);

	IXMLSink* m_sink;
	int m_indentationSize;

	std::vector<Element*> m_elements;
	bool m_newLine;
};

template <typename T> void XMLWriter::Element::Attribute(const string& name, const T& value)
{
	assert(!isParent);
	char buffer[1024];
	XMLWriter::SerializeAttribute(buffer, value);
	if (m_output)
		m_writer.WriteAttribute(name, buffer);
}

template <typename T> void XMLWriter::Element::ContentArrayElement(const T& value)
{
	if (!isParent)
	{
		isParent = true;
		if (m_output)
			m_writer.CloseElement(m_name, false);
	}
	else
	{
		if (m_output)
			m_writer.WriteContent(" ");
	}

	char buffer[1024];
	XMLWriter::SerializeArrayElement(buffer, value);
	if (m_output)
		m_writer.WriteContent(buffer);
}

#endif //__XMLWRITER_H__
