#include "stdafx.h"
#include "ExcelExport.h"

ExcelExport::ExcelExport()
	: m_fp(NULL)
	, m_nextStyleId(22)
	, m_inStyles(false)
{
}

ExcelExport::~ExcelExport()
{
	Close();
}

bool ExcelExport::Open(const char* filename)
{
	if (m_fp)
		return false;
	m_fp = fopen(filename, "wt");

	if (m_fp == NULL)
		return false;

	fprintf(m_fp, "<?xml version=\"1.0\"?>\n");
	fprintf(m_fp, "<?mso-application progid=\"Excel.Sheet\"?>\n");

	PushTag("Workbook", "xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\"");

	fprintf(m_fp, "<ExcelWorkbook xmlns=\"urn:schemas-microsoft-com:office:excel\" />\n");

	PushTag("Styles", "");
	m_inStyles = true;

	return true;
}

int ExcelExport::BeginStyle()
{
	if (m_inStyles)
	{
		PushTag("Style", "ss:ID=\"s%i\"", m_nextStyleId);
		return m_nextStyleId ++;
	}

	return -1;
}

void ExcelExport::EndStyle()
{
	if (m_inStyles)
		PopTag();
}

void ExcelExport::BeginWorksheet(const char* name)
{
	if (m_inStyles)
	{
		PopTag();
		m_inStyles = false;
	}

	PushTag("Worksheet", "ss:Name=\"%s\"", name);
	PushTag("Table", "");
}

void ExcelExport::AddColumn(int width)
{
	fprintf(m_fp, "<Column ss:Width=\"%i\" />\n", width);
}

void ExcelExport::BeginRow(int style)
{
	if (style != -1)
		PushTag("Row", "ss:StyleID=\"s%i\"", style);
	else
		PushTag("Row", "");
}

void ExcelExport::AddCell(const char* str, int style)
{
	if (style != -1)
		PushTag("Cell", "ss:StyleID=\"s%i\"", style);
	else
		PushTag("Cell", "");

	fprintf(m_fp, "<Data ss:Type=\"String\">%s</Data>\n", str);

	PopTag();
}

void ExcelExport::AddCell(int n, int style)
{
	if (style != -1)
		PushTag("Cell", "ss:StyleID=\"s%i\"", style);
	else
		PushTag("Cell", "");

	fprintf(m_fp, "<Data ss:Type=\"Number\">%i</Data>\n", n);

	PopTag();
}

void ExcelExport::AddCell(float f, int style)
{
	if (style != -1)
		PushTag("Cell", "ss:StyleID=\"s%i\"", style);
	else
		PushTag("Cell", "");

	fprintf(m_fp, "<Data ss:Type=\"Number\">%f</Data>\n", f);

	PopTag();
}

void ExcelExport::EndRow()
{
	PopTag();
}

void ExcelExport::EndWorksheet()
{
	PopTag();
	PopTag();
}

void ExcelExport::Close()
{
	if (m_fp)
	{
		while (m_tagStack.empty() == false)
			PopTag();

		fclose(m_fp);
		m_fp = NULL;
	}
}

void ExcelExport::PushTag(const char* tag, const char* attrFmt, ...)
{
	assert (m_fp != NULL);

	m_tagStack.push_back(tag);

	if (attrFmt[0])
	{
		va_list args;
		va_start(args, attrFmt);

		char tmp[2048];
		vsprintf_s(tmp, 2048, attrFmt, args);

		va_end(args);

		fprintf(m_fp, "<%s %s>\n", tag, tmp);
	}
	else
	{
		fprintf(m_fp, "<%s>\n", tag);
	}
}

void ExcelExport::PopTag()
{
	assert (m_fp != NULL);
	assert (m_tagStack.empty() == false);

	const char* tag = m_tagStack.back();
	m_tagStack.pop_back();

	fprintf(m_fp, "</%s>\n", tag);
}
