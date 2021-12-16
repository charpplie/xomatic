#pragma once

class ISerialiser
{
public:
	virtual ~ISerialiser() {}

	template <typename T>
	void Write(const T& val) { WriteRaw(&val, sizeof(val)); }

	template <typename IteratorT>
	void Write(IteratorT begin, const IteratorT& end)
	{
		for (; begin != end; ++ begin)
			WriteRaw(&*begin, sizeof(typename std::iterator_traits<IteratorT>::value_type));
	}

	virtual void WriteRaw(const void* data, size_t dataSize) = 0;
};

class IDeserialiser
{
public:
	virtual ~IDeserialiser(){}

	template <typename T>
	void Read(T& val) { ReadRaw(&val, sizeof(val)); }

	template <typename T>
	T Read()
	{
		T val;
		ReadRaw(&val, sizeof(T));
		return val;
	}

	template <typename IteratorT>
	void Read(IteratorT out, size_t count)
	{
		for (size_t i = 0; i != count; ++ i)
		{
			typename StripReference<typename std::iterator_traits<IteratorT>::reference>::value_type val;
			ReadRaw(&val, sizeof(val));

			*out ++ = val;
		}
	}

	virtual void ReadRaw(void* data, size_t dataSize) = 0;
};

class FileSerialiser : public ISerialiser
{
public:
	FileSerialiser(const char* filename);
	~FileSerialiser();

	bool IsOpen() const { return m_fp != NULL; }

	void WriteRaw(const void* data, size_t dataSize);

private:
	FileSerialiser(const FileSerialiser&);
	FileSerialiser& operator = (const FileSerialiser&);

private:
	FILE* m_fp;
};

class FileDeserialiser : public IDeserialiser
{
public:
	FileDeserialiser(const char* filename);
	~FileDeserialiser();

	bool IsOpen() const { return m_fp != NULL; }

	void ReadRaw(void* data, size_t dataSize);

private:
	FileDeserialiser(const FileDeserialiser&);
	FileDeserialiser& operator = (const FileDeserialiser&);

private:
	FILE* m_fp;
};
