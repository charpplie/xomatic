#pragma once

// Helper class
//
// clipping is done in integer
//

class CSimpleTriangleRasterizer
{
public:

	class IRasterizeSink
	{
	public:

		//! is called once per triangel for the first possible visible line
		//! /param iniStartY 
		virtual void Triangle( const int iniStartY )
		{
		}

		//! callback function
		//! /param infXLeft included - not clipped against left and reight border
		//! /param infXRight excluded - not clipped against left and reight border
		//! /param iniXLeft included
		//! /param iniXRight excluded
		//! /param iniY 
		virtual void Line( const float infXLeft, const float infXRight,
						   const int iniXLeft, const int iniXRight, const int iniY )=0;
	};

	typedef unsigned long DWORD;

	// -----------------------------------------------------

	//! implementation sink sample
	template <class T>
	class CTFlatFill: public IRasterizeSink
	{
	public:

		//! constructor
		CTFlatFill( T *inpBuffer, const DWORD indwPitchInPixels, T indwValue )
		{
			m_dwValue=indwValue;
			m_pBuffer=inpBuffer;
			m_dwPitchInPixels=indwPitchInPixels;
		}

		virtual void Triangle( const int iniY )
		{
			m_pBufferLine=&m_pBuffer[iniY*m_dwPitchInPixels];
		}

		virtual void Line( const float infXLeft, const float infXRight,
						   const int iniLeft, const int iniRight, const int iniY )
		{
			T *mem=&m_pBufferLine[iniLeft];

			for(int x=iniLeft;x<iniRight;x++)
				*mem++=m_dwValue;

			m_pBufferLine+=m_dwPitchInPixels;
		}

	private: // -------------------------------------------------------

		T m_dwValue;							//!< fill value
		T *m_pBufferLine;					//!< to get rid of the multiplication per line

		DWORD m_dwPitchInPixels;	//!< in Ts, not in Bytes
		T *m_pBuffer;							//!< pointer to the buffer
	};

	// -----------------------------------------------------

	//! constructor
	//! /param iniWidth excluded
	//! /param iniHeight excluded
	CSimpleTriangleRasterizer( const int iniWidth, const int iniHeight )
	{
		m_iMinX=0;
		m_iMinY=0;
		m_iMaxX=iniWidth-1;
		m_iMaxY=iniHeight-1;
	}
/*
	//! constructor 
	//! /param iniMinX included
	//! /param iniMinY included
	//! /param iniMaxX included
	//! /param iniMaxY included
	CSimpleTriangleRasterizer( const int iniMinX, const int iniMinY, const int iniMaxX, const int iniMaxY )
	{
		m_iMinX=iniMinX;
		m_iMinY=iniMinY;
		m_iMaxX=iniMaxX;
		m_iMaxY=iniMaxY;
	}
*/
	//! simple triangle filler with clipping (optimizable), not subpixel correct
	//! /param pBuffer pointer o the color buffer
	//! /param indwWidth width of the color buffer
	//! /param indwHeight height of the color buffer
	//! /param x array of the x coordiantes of the three vertices
	//! /param y array of the x coordiantes of the three vertices
	//! /param inValue value of the triangle
	template <class T>
	void DTFlatFill( T *inpBuffer, const DWORD indwPitchInPixels, float x[3], float y[3], T inValue, bool inbConservative )
	{
		CTFlatFill<T> pix(inpBuffer,indwPitchInPixels,inValue);

		// reject bad input
//		if(x[0]<m_iMinX-100 || x[1]<m_iMinX-100 || x[2]<m_iMinX-100 || x[0]>m_iMaxX+100 || x[1]>m_iMaxX+100 || x[2]>m_iMaxX+100
//		|| y[0]<m_iMinY-100 || y[1]<m_iMinY-100 || y[2]<m_iMinY-100 || y[0]>m_iMaxY+100 || y[1]>m_iMaxY+100 || y[2]>m_iMaxY+100)
		if(_isnan(x[0]) || _isnan(x[1]) || _isnan(x[2]) 
		|| _isnan(y[0]) || _isnan(y[1]) || _isnan(y[2]))
		{
			assert(0);
			return;
		}

		if(inbConservative)
			CallbackFillConservative(x,y,&pix);
		 else
			CallbackFillSubpixelCorrect(x,y,&pix);
	}

	// Rectangle around triangle - more stable - use for debugging purpose
	void CallbackFillRectConservative( float x[3], float y[3], IRasterizeSink *inpSink );


	//! subpixel correct triangle filler (conservative or not conservative)
	//! \param pBuffer pointe to the DWORD
	//! \param indwWidth width of the buffer pBuffer pointes to
	//! \param indwHeight height of the buffer pBuffer pointes to
	//! \param x array of the x coordiantes of the three vertices
	//! \param y array of the x coordiantes of the three vertices
	//! \param inpSink pointer to the sink interface (is called per triangle and per triangle line)
	void CallbackFillConservative( float x[3], float y[3], IRasterizeSink *inpSink );

	void CallbackFillConservative2( float x[3], float y[3], IRasterizeSink *inpSink );

	//! subpixel correct triangle filler (conservative or not conservative)
	//! \param pBuffer pointe to the DWORD
	//! \param indwWidth width of the buffer pBuffer pointes to
	//! \param indwHeight height of the buffer pBuffer pointes to
	//! \param x array of the x coordiantes of the three vertices
	//! \param y array of the x coordiantes of the three vertices
	//! \param inpSink pointer to the sink interface (is called per triangle and per triangle line)
	void CallbackFillSubpixelCorrect( float x[3], float y[3], IRasterizeSink *inpSink );

	//!
	//! /param inoutfX
	//! /param inoutfY
	//! /param infAmount could be positive or negative
	static void ShrinkTriangle( float inoutfX[3], float inoutfY[3], float infAmount );

private:

	// Clipping Rect;

	int		m_iMinX;		//!< minimum x value included 
	int		m_iMinY;		//!< minimum y value included
	int		m_iMaxX;		//!< maximum x value included
	int		m_iMaxY;		//!< maximum x value included

	void HorizlineConservative( float fx1, float fx2, int y, IRasterizeSink *inpSink );
	void HorizlineSubpixelCorrect( float fx1, float fx2, int y, IRasterizeSink *inpSink );
	void CopyAndSortY( const float infX[3], const float infY[3], float outfX[3],float outfY[3] );
};


// extension ideas:
// * callback with coverage mask (possible non ordered sampling)
// * z-buffer behaviour
// * gouraud shading
// * texture mapping with nearest/bicubic/bilinear filter
// * further primitives: thick line, ellipse
// * build a template version
// * 
