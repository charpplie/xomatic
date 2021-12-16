#pragma once

class ExcelExport
{
public:
	ExcelExport();
	~ExcelExport();

	bool Open(const char* filename);

	int BeginStyle();
	void EndStyle();

	void BeginWorksheet(const char* name);
	void AddColumn(int width);

	void BeginRow(int style = -1);

	void AddCell(const char* str, int style = -1);
	void AddCell(int n, int style = -1);
	void AddCell(float f, int style = -1);

	void EndRow();

	void EndWorksheet();

	void Close();

private:
	ExcelExport(const ExcelExport&);
	ExcelExport& operator = (const ExcelExport&);

private:
	void PushTag(const char* tag, const char* attrFmt, ...);
	void PopTag();

private:
	FILE* m_fp;

	int m_nextStyleId;
	bool m_inStyles;

	std::vector<const char*> m_tagStack;
};

class ExcelExportWorksheet
{
public:
	ExcelExportWorksheet(ExcelExport& xls, const char* name)
		: m_xls(&xls)
	{
		m_xls->BeginWorksheet(name);
	}

	~ExcelExportWorksheet()
	{
		m_xls->EndWorksheet();
	}

	void AddColumn(int width)															{ m_xls->AddColumn(width); }

	void BeginRow(int style = -1)													{ m_xls->BeginRow(style); }

	void AddCell(const char* str, int style = -1)					{ m_xls->AddCell(str, style); }
	void AddCell(int n, int style = -1)										{ m_xls->AddCell(n, style); }
	void AddCell(float f, int style = -1)									{ m_xls->AddCell(f, style); }

	void EndRow()																					{ m_xls->EndRow(); }

private:
	ExcelExportWorksheet(const ExcelExportWorksheet&);
	ExcelExportWorksheet& operator = (const ExcelExportWorksheet&);

private:
	ExcelExport* m_xls;
};

class ExcelExportRow
{
public:
	explicit ExcelExportRow(ExcelExportWorksheet& ws, int style = -1)
		: m_ws(&ws)
	{
		m_ws->BeginRow(style);
	}

	~ExcelExportRow()
	{
		m_ws->EndRow();
	}

	void AddCell(const char* str, int style = -1)					{ m_ws->AddCell(str, style); }
	void AddCell(int n, int style = -1)										{ m_ws->AddCell(n, style); }
	void AddCell(float f, int style = -1)									{ m_ws->AddCell(f, style); }

private:
	ExcelExportRow(const ExcelExportRow&);
	ExcelExportRow& operator = (const ExcelExportRow&);

private:
	ExcelExportWorksheet* m_ws;
};
