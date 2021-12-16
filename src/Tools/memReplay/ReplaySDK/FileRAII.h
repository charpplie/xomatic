#pragma once

class FileRAII
{
public:
	FileRAII(FILE* fp)
		: m_fp(fp)
	{
	}

	~FileRAII()
	{
		if (m_fp != NULL)
			fclose(m_fp);
	}

	operator FILE* () const			{ return m_fp; }
	bool IsValid() const				{ return m_fp != NULL; }

private:
	FileRAII(const FileRAII&);
	FileRAII& operator = (const FileRAII&);

	FILE* m_fp;
};
