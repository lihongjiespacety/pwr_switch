// tscvc6Dlg.h : header file
//

#if !defined(AFX_TSCVC6DLG_H__88F02647_53D9_4B3B_B272_46DDC9036174__INCLUDED_)
#define AFX_TSCVC6DLG_H__88F02647_53D9_4B3B_B272_46DDC9036174__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CTscvc6Dlg dialog

#define WN_SHOW_UPDATE            WM_USER+1  //升级消息
#define WN_EXIT_UPDATE            WM_USER+2  //升级EXIT消息
#define WN_SHOW_UARTRCV           WM_USER+3  //串口接收

class CTscvc6Dlg : public CDialog
{
// Construction
public:
	CString SelectFile();
	CTscvc6Dlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CTscvc6Dlg)
	enum { IDD = IDD_TSCVC6_DIALOG };
	CComboBox	m_ComBoCh;
	CComboBox	m_ComBoParity;
	CComboBox	m_ComBoStopBit;
	CComboBox	m_ComBoBaud;
	CComboBox	m_ComBoData;
	CComboBox	m_ComBoCom;
	UINT	m_ONTimeMin;
	UINT	m_ONTimeMax;
	UINT	m_OFFTimeMin;
	UINT	m_OFFTimeMax;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTscvc6Dlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CTscvc6Dlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButton2();
	afx_msg void OnButton25();
	afx_msg void OnButton24();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TSCVC6DLG_H__88F02647_53D9_4B3B_B272_46DDC9036174__INCLUDED_)
