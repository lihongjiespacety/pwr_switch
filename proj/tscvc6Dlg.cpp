// tscvc6Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "tscvc6.h"
#include "tscvc6Dlg.h"
#include "uart.h"
#include "crc32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define UPDATE_START 1
#define UPDATE_SEND  2
#define UPDATE_DONE  3
#define UPDATE_ERR   4
#define UPDATE_SENDCRC   6
#define UPDATE_OK    5

char UpdateFileName[256]={0};
UINT UpdateFileAddr=0;
UINT UpdateFileBlockSize=4096;

CWinThread*    Update_Thread=0;//声明线程
UINT StartUpdateThread(void *param);//声明线程函数
CWinThread*    UartRcv_Thread=0;//声明线程
UINT UartRcvThread(void *param);//声明线程函数

char SaveFileName[256]={0};
char CmdFileName[256]={0};
CFile SaveFile;      // CFile对象 
CStdioFile CmdFile;      // CFile对象 


#define CMD_MAXNUM 64
#define CMD_BUFFNUM 128
typedef enum
{
	CMD_LOOP,
	CMD_SEND,
	CMD_DELAY,
	CMD_RCV,
	CMD_END,
}CMD_e;

typedef struct
{
	UINT  loop;      /*循环次数*/
	UINT  cmdnum;    /*一条循环的个数*/
	UINT  cmdindex;  /*当前指令索引*/

	CMD_e cmd[CMD_MAXNUM];
	BYTE cmd_str[CMD_MAXNUM][CMD_BUFFNUM];  /*一个循环最多64条命令*/
	UINT cmd_val[CMD_MAXNUM];
}CMD_Ctrl_t;

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTscvc6Dlg dialog

CTscvc6Dlg::CTscvc6Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTscvc6Dlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTscvc6Dlg)
	m_ONTimeMin = 0;
	m_ONTimeMax = 0;
	m_OFFTimeMin = 0;
	m_OFFTimeMax = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTscvc6Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTscvc6Dlg)
	DDX_Control(pDX, IDC_COMBO6, m_ComBoCh);
	DDX_Control(pDX, IDC_COMBO5, m_ComBoParity);
	DDX_Control(pDX, IDC_COMBO4, m_ComBoStopBit);
	DDX_Control(pDX, IDC_COMBO2, m_ComBoBaud);
	DDX_Control(pDX, IDC_COMBO3, m_ComBoData);
	DDX_Control(pDX, IDC_COMBO1, m_ComBoCom);
	DDX_Text(pDX, IDC_EDIT3, m_ONTimeMin);
	DDX_Text(pDX, IDC_EDIT7, m_ONTimeMax);
	DDX_Text(pDX, IDC_EDIT8, m_OFFTimeMin);
	DDX_Text(pDX, IDC_EDIT36, m_OFFTimeMax);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTscvc6Dlg, CDialog)
	//{{AFX_MSG_MAP(CTscvc6Dlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_BN_CLICKED(IDC_BUTTON25, OnButton25)
	ON_BN_CLICKED(IDC_BUTTON24, OnButton24)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTscvc6Dlg message handlers

BOOL CTscvc6Dlg::OnInitDialog()
{
		char strtmp[64];
		int i;
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	for(i=0;i<100;i++)
	{
	    _snprintf(strtmp,10,"COM%d",i);
		m_ComBoCom.AddString(strtmp);
	}
    m_ComBoCom.SetCurSel(0);

	for(i=0;i<8;i++)
	{
	    _snprintf(strtmp,10,"CH%d",i);
		m_ComBoCh.AddString(strtmp);
	}
    m_ComBoCh.SetCurSel(0);

	m_ComBoBaud.AddString("256000");
	m_ComBoBaud.AddString("115200");
	m_ComBoBaud.AddString("57600");
    m_ComBoBaud.AddString("38400");
	m_ComBoBaud.AddString("19200");
	m_ComBoBaud.AddString("9600");
    m_ComBoBaud.AddString("4800");
    m_ComBoBaud.SetCurSel(0);

	m_ComBoData.AddString("8");
    m_ComBoData.AddString("7");
    m_ComBoData.AddString("6");
    m_ComBoData.SetCurSel(0);

    m_ComBoStopBit.AddString("1");
    m_ComBoStopBit.AddString("1.5");
    m_ComBoStopBit.AddString("2");
    m_ComBoStopBit.SetCurSel(0);

    m_ComBoParity.AddString("无");
    m_ComBoParity.AddString("奇校验");
    m_ComBoParity.AddString("偶校验");
    m_ComBoParity.SetCurSel(0);
	
	m_ONTimeMin = 24*60*60*1000;
	m_ONTimeMax = 24*60*60*1000;
	m_OFFTimeMin = 5*1000;
	m_OFFTimeMax = 5*1000;
	//((CButton *)GetDlgItem(IDC_CHECK3))->SetCheck(FALSE);

	UpdateData(FALSE);  // 变量->控件
    uart_init();
	/*创建接收线程*/
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTscvc6Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTscvc6Dlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTscvc6Dlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

static int ComOpenState = 0;
static int ComEnableState = 0;

static int FileSaveState = 0;
static int FileCmdState = 0;
static char ComStr[64]={0};
static int ComBaud=0;
static int ComData=0;
static int ComStop=0;
static int ComParity=0;
static CString EditStr = _T("");
static BYTE gReadBuff[64]={0};
static char gReadStr[64]={0};
static char gWriteBuff[32]={(char)0xFF,(char)0x3C,(char)0x01,(char)0x00,(char)0x3D,(char)0x0D};
static char xPointStr[64];
static char yPointStr[64];
static DWORD gReadLen;
static char gPosition = 0;
static unsigned int gPrintNum =0;
static gRaiodSel = 1;
static int xOffset;
char DigToChar(unsigned char val)
{
	if((val>=0) && (val<=9))
	{
	    return (val+'0');
	}
	else if((val>9) && (val<=15))
	{
	    return (val-10 +'A');
	}
	else
	{
	    return ' ';
	}
}

BYTE CharToHex(char val)
{
	if((val>='0') && (val<='9'))
	{
	    return (val-'0');
	}
	else if((val>='A') && (val<='F'))
	{
	    return (val-'A' + 10);
	}
	else if((val>='a') && (val<='f'))
	{
	    return (val-'a' + 10);
	}
	else
	{
	    return 0;
	}
}

void CTscvc6Dlg::OnButton2() 
{
	// TODO: Add your control notification handler code here
	int nIndex;
	if(0 == ComOpenState)
	{
		nIndex = m_ComBoCom.GetCurSel();
		if(nIndex>=10)
		{
			_snprintf(ComStr,20,"\\\\.\\COM%d",nIndex);
		}
		else
		{
			_snprintf(ComStr,20,"COM%d",nIndex);
		}
		if(uart_open(ComStr) != 0)
		{
			MessageBox("打开失败");
		}
		else
		{
            nIndex = m_ComBoBaud.GetCurSel();
            switch(nIndex)
			{
			case 0:
				ComBaud = 256000;
				break;
			case 1:
				ComBaud = 115200;
				break;
			case 2:
				ComBaud = 57600;
				break;
			case 3:
				ComBaud = 38400;
				break;
			case 4:
				ComBaud = 19200;
				break;
			case 5:
				ComBaud = 9600;
				break;
			case 6:
				ComBaud = 4800;
				break;
			default:
				ComBaud = 9600;
				break;
			}
            nIndex = m_ComBoData.GetCurSel();
            switch(nIndex)
			{
			case 0:
				ComData = 8;
				break;
			case 1:
				ComData = 7;
				break;
			case 2:
				ComData = 6;
				break;
			default:
				ComData = 8;
				break;
			}
            nIndex = m_ComBoParity.GetCurSel();
            switch(nIndex)
			{
			case 0:
				ComParity = NOPARITY;
				break;
			case 1:
				ComParity = ODDPARITY;
				break;
			case 2:
				ComParity = EVENPARITY;
				break;
			default:
				ComParity = NOPARITY;
				break;
			}
            nIndex = m_ComBoStopBit.GetCurSel();
            switch(nIndex)
			{
			case 0:
				ComStop = ONESTOPBIT;
				break;
			case 1:
				ComStop = ONE5STOPBITS;
				break;
			case 2:
				ComStop = TWOSTOPBITS;
				break;
			default:
				ComStop = ONESTOPBIT;
				break;
			}
			if(uart_config(ComBaud,ComData,ComParity,ComStop,300) != 0)
			{
				uart_close(ComStr);
				MessageBox("配置失败");
			}
			else
			{
				ComOpenState = 1;
				SetDlgItemText(IDC_BUTTON2,"关闭");
				((CButton *)GetDlgItem(IDC_RADIO1))->SetCheck(TRUE);
			}
		}
	}
	else
	{
		if(uart_close(ComStr) != 0)
		{
			MessageBox("关闭失败");
		}
		else
		{
			ComOpenState = 0;
			SetDlgItemText(IDC_BUTTON2,"打开");
			((CButton *)GetDlgItem(IDC_RADIO1))->SetCheck(FALSE);
		}
	}
}


BOOL CTscvc6Dlg::DestroyWindow() 
{
	// TODO: Add your specialized code here and/or call the base class
	UINT nIndex;
	if(1 == ComOpenState)
	{
		nIndex = m_ComBoCom.GetCurSel();
		if(nIndex>=10)
		{
			_snprintf(ComStr,20,"\\\\.\\COM%d",nIndex);
		}
		else
		{
			_snprintf(ComStr,20,"COM%d",nIndex);
		}
		uart_close(ComStr);
	}
	return CDialog::DestroyWindow();
}



BYTE buff_sum(void* buff,BYTE len)
{
  BYTE sum=0;
  BYTE i;
  BYTE* p;
  p = (BYTE*)buff;
  for(i=0;i<len;i++)
  {
      sum += p[i];
  }
  return sum;
}

void CTscvc6Dlg::OnButton25() 
{
	// TODO: Add your control notification handler code here
	unsigned char buff[128];
	BOOL writestat;
	int len=0;
	int wrlen=25;
	UpdateData(TRUE);  // 控件->变量
	buff[0] = (BYTE)0xAA;
	buff[1] = 0x55;
	buff[2] = 0;  /*长度 大端*/
	buff[3] = 25;

	buff[4] = 1;    /*命令帧*/

	/*子块*/
	buff[5] = 19;
	buff[6] = 2;                       /*子块类型 命令类型*/
	buff[7] = m_ComBoCh.GetCurSel();   /*子块子类型 继电器序号*/
    buff[8] = (m_ONTimeMin>>24) & 0xFF;        /*On最小时间 大端*/
    buff[9] = (m_ONTimeMin>>16) & 0xFF; 
	buff[10] = (m_ONTimeMin>>8) & 0xFF; 
	buff[11] = (m_ONTimeMin>>0) & 0xFF; 
    buff[12] = (m_ONTimeMax>>24) & 0xFF;        /*On最大时间 大端*/
    buff[13] = (m_ONTimeMax>>16) & 0xFF; 
	buff[14] = (m_ONTimeMax>>8) & 0xFF; 
	buff[15] = (m_ONTimeMax>>0) & 0xFF; 
    buff[16] = (m_OFFTimeMin>>24) & 0xFF;        /*Off最小时间 大端*/
    buff[17] = (m_OFFTimeMin>>16) & 0xFF; 
	buff[18] = (m_OFFTimeMin>>8) & 0xFF; 
	buff[19] = (m_OFFTimeMin>>0) & 0xFF; 
    buff[20] = (m_OFFTimeMax>>24) & 0xFF;        /*Off最大时间 大端*/
    buff[21] = (m_OFFTimeMax>>16) & 0xFF; 
	buff[22] = (m_OFFTimeMax>>8) & 0xFF; 
	buff[23] = (m_OFFTimeMax>>0) & 0xFF; 
	buff[24] = buff_sum(buff,24);
	len = uart_send(buff, wrlen, &writestat);

	if(len != wrlen)
	{
		MessageBox("发送失败");
	}
}

void CTscvc6Dlg::OnButton24() 
{
	// TODO: Add your control notification handler code here
	unsigned char buff[128];
	BOOL writestat;
	int len=0;
	int wrlen=9;
	UpdateData(TRUE);  // 控件->变量
	buff[0] = (unsigned char)0xAA;
	buff[1] = 0x55;
	buff[2] = 0;  /*长度 大端*/
	buff[3] = 9;

	buff[4] = 1;    /*命令帧*/
	uart_flush();
	/*子块*/
	buff[5] = 3;
	buff[6] = 3;                       /*子块类型 命令类型*/
	buff[7] = m_ComBoCh.GetCurSel();   /*子块子类型 继电器序号*/
	buff[8] = buff_sum(buff,8);
	len = uart_send(buff, wrlen, &writestat);

	if(len != wrlen)
	{
		MessageBox("发送失败");
		return;
	}	
    len = 0;
	memset(buff,0,sizeof(buff));

	/*读返回值*/
    wrlen = 25;
	len = uart_read(buff, wrlen, &writestat);
	if(len != wrlen)
	{
		MessageBox("读取失败");
		return;
	}	

	/*解析数据*/
	if((buff[0]!=0xAA)||(buff[1]!=0x55))
	{
		MessageBox("包头错误");
	}
	else if((buff[2]!=0)||(buff[3]!=25))
	{
		MessageBox("长度错误");
	}
	else if(buff_sum(buff,24) != buff[24])
	{
		MessageBox("校验错误");
	}
	else if((buff[4]!=2)||(buff[6]!=8))
	{
		MessageBox("校验错误");
	}
	else
	{
		m_ONTimeMin = ((UINT)buff[8]<<24)|((UINT)buff[9]<<16)|((UINT)buff[10]<<8)|((UINT)buff[11]<<0);
		m_ONTimeMax = ((UINT)buff[12]<<24)|((UINT)buff[13]<<16)|((UINT)buff[14]<<8)|((UINT)buff[15]<<0);
		m_OFFTimeMin = ((UINT)buff[16]<<24)|((UINT)buff[17]<<16)|((UINT)buff[18]<<8)|((UINT)buff[19]<<0);
		m_OFFTimeMax = ((UINT)buff[20]<<24)|((UINT)buff[21]<<16)|((UINT)buff[22]<<8)|((UINT)buff[23]<<0);
		/*变量-控件*/
		UpdateData(FALSE);  // 变量->控件
	}
}
