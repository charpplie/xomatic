////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek Studios, 2008.
// -------------------------------------------------------------------------
//  File name:   AVCompression.h
//  Created:     3/5/2010 by John Stewart.
//  Description: This dialog is used to configure the options for AV Compression
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef AVCompressionDialog_h__
#define AVCompressionDialog_h__

#pragma once

#define AVCRECORDSIZECUSTOM -2
#define AVCRECORDSIZEFULLSCREEN -1
#define AVCRECORDSIZESTANDARD0 0
#define AVCRECORDSIZESTANDARD1 1
#define AVCRECORDSIZESTANDARD2 2
#define AVCRECORDSIZESTANDARD3 3
#define AVCRECORDSIZESTANDARD4 4
#define AVCRECORDSIZESTANDARD5 5
#define AVCRECORDSIZESTANDARD6 6

#define AVCRECORDMODECONTINUOUS 0
#define AVCRECORDMODELIMITED 1

#define QUALITYMAXVALUE 9

#define TRANSCODEFAST 3
#define TRANSCODEFULL 4
#define TRANSCODENULL 5

struct AVCRecordParameters;

class CAVCompressionDialog: public CDialog
{
public:

	enum {IDD = IDD_AVC_DIALOG1};

	CAVCompressionDialog(int RecordSize,unsigned int RecordMode,unsigned int CustomWidth, unsigned int CustomHeight,
		int MaxFileSize,unsigned int RecordTime,float RecordFrameRate,unsigned int RecordQuality);

	virtual ~CAVCompressionDialog() {}

	// These routines allow the modified parameter values to be accessed
	void copyParms(AVCRecordParameters *recordParameters);


protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	void AVC_clear_size_button(bool seteditbox);

	DECLARE_MESSAGE_MAP()

private:



	int m_AVC_RecordSize;
	unsigned int m_AVC_RecordMode;
	unsigned int m_AVC_CustomWidth;
	unsigned int m_AVC_CustomHeight;
	int m_AVC_Maxfilesize;
	unsigned int m_AVC_RecordTime;
	float m_AVC_RecordFrameRate;
	unsigned int m_AVC_RecordQuality;


	CEdit	m_EditCustom_Width;
	CEdit	m_EditCustom_Height;
	CEdit	m_EditRecordTime;
	CEdit	m_EditRecordSize;
	CEdit	m_EditFrameRate;


	CButton	m_RecordSizeButton0;
	CButton	m_RecordSizeButton1;
	CButton	m_RecordSizeButton2;
	CButton	m_RecordSizeButton3;
	CButton	m_RecordSizeButton4;
	CButton	m_RecordSizeButton5;
	CButton	m_RecordSizeButton6;
	CButton	m_RecordSizeButton7;
	CButton	m_RecordSizeButton8;

	CButton	m_RecordModeButton0;
	CButton	m_RecordModeButton1;

	CSliderCtrl m_QualitySlider;


public:


	afx_msg void OnBnClickedAVCRecordsize0Radio();
	afx_msg void OnBnClickedAVCRecordsize1Radio();
	afx_msg void OnBnClickedAVCRecordsize2Radio();
	afx_msg void OnBnClickedAVCRecordsize3Radio();
	afx_msg void OnBnClickedAVCRecordsize4Radio();
	afx_msg void OnBnClickedAVCRecordsize5Radio();
	afx_msg void OnBnClickedAVCRecordsize6Radio();
	afx_msg void OnBnClickedAVCRecordsize7Radio();
	afx_msg void OnBnClickedAVCRecordsize8Radio();
	afx_msg void OnBnClickedAVCRecordMode0Radio();
	afx_msg void OnBnClickedAVCRecordMode1Radio();
	afx_msg void OnBnClickedAVCOptionsOkButton();
	afx_msg void OnBnClickedAVCOptionsCancelButton();

};




class CAVCompressionReencodeDialog: public CDialog
{
public:

	enum {IDD2 = IDD_AVC_DIALOG2};

	CAVCompressionReencodeDialog();

	virtual ~CAVCompressionReencodeDialog() {}

	int getMode(void) {return(m_compressMode);}



protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()

private:

	int m_compressMode;

public:

	afx_msg void OnBnClickedAVCOptionsFast();
	afx_msg void OnBnClickedAVCOptionsFull();
	afx_msg void OnBnClickedAVCOptionsDiscard();

};





#endif // ConsoleOptionsDialog_h__
