#ifndef ITABPAGE_H
#define ITABPAGE_H

class ExcelExport;

class ITabPage
{
public:
	enum FindFlags
	{
		FF_Forward = 1,
		FF_CaseInsensitive = 2,
	};

	enum GatherType
	{
		GT_Address,
		GT_Function,
		GT_Line,
		GT_File,
	};

public:
	virtual ~ITabPage() {}

	virtual void OnTabActivate() {}

	virtual void OnInspectClicked() {}
	virtual void OnReverseClicked(GatherType gt) {}
	virtual void OnGatherTDClicked(GatherType gt) {}
	virtual void OnGatherBUClicked(GatherType gt) {}
	virtual void OnExcludeClicked(GatherType gt) {}
	virtual void OnPlotClicked(GatherType gt) {}
	virtual void SetMemoryType(int type) = 0;
	virtual int GetMemoryType() const = 0;
	virtual void ToggleCountScaling() {}
	virtual bool GetCountScaling() {return false;}

	virtual void OnFindNext(const char* searchString, u32 flags) {}
	virtual void OnEditCopy() {}

	virtual void Export(ExcelExport& xls) {}
};

#endif
