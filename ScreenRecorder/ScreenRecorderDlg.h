
// ScreenRecorderDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CScreenRecorderDlg 对话框
class CScreenRecorderDlg : public CDialogEx
{
// 构造
public:
	CScreenRecorderDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CScreenRecorderDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SCREENRECORDER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	CString m_strSelectedWndTitle;
	afx_msg void OnBnClickedButton3();

private:
	HWND	currentBaseWnd;
public:
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();

	void ScreenShot(char *filename);

	HRESULT SavePixelsToFile32bppPBGRA(UINT width, UINT height, UINT stride, LPBYTE pixels, LPWSTR filePath, const GUID &format);
	HRESULT Direct3D9TakeScreenshots(UINT adapter, UINT count);
	afx_msg void OnBnClickedButton7();

	HHOOK  KeyboardHook;
	bool recording = false;
};
