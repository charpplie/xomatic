#ifndef __COLOURFROMXML_H__
#define __COLOURFROMXML_H__

class CColourFromXml : public ColorF
{
	public:
	CColourFromXml();
	void Read(const IItemParamsNode * xml);
};

#endif
