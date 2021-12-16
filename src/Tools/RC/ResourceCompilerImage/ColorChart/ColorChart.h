#ifndef __COLORCHART_H__
#define __COLORCHART_H__


struct ImageObject;


struct IColorChart
{
public:
	virtual void Release() = 0;
	virtual void GenerateDefault() = 0;
	virtual bool GenerateFromInput(ImageObject* pImg) = 0;
	virtual ImageObject* GenerateChartImage() = 0;

protected:
	virtual ~IColorChart() {};
};


IColorChart* Create3dLutColorChart();
IColorChart* CreateLeastSquaresLinColorChart();


#endif //__COLORCHART_H__
