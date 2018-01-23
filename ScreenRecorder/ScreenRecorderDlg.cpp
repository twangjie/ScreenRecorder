
// ScreenRecorderDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ScreenRecorder.h"
#include "ScreenRecorderDlg.h"
#include "afxdialogex.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include "SCWinFilter.h"
#include <boost/lexical_cast.hpp>

#include <Wincodec.h>             // we use WIC for saving images
#include <d3d9.h>                 // DirectX 9 header
#pragma comment(lib, "d3d9.lib")  // link to DirectX 9 library

#include "FFmpegEncoder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static bool startCapture = false;

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CScreenRecorderDlg 对话框



CScreenRecorderDlg::CScreenRecorderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_SCREENRECORDER_DIALOG, pParent)
	, m_strSelectedWndTitle(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	currentBaseWnd = NULL;
}

CScreenRecorderDlg::~CScreenRecorderDlg()
{
	UnhookWindowsHookEx(KeyboardHook);

}

void CScreenRecorderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CScreenRecorderDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_BUTTON1, &CScreenRecorderDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CScreenRecorderDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CScreenRecorderDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CScreenRecorderDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CScreenRecorderDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CScreenRecorderDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &CScreenRecorderDlg::OnBnClickedButton7)
END_MESSAGE_MAP()


// CScreenRecorderDlg 消息处理程序

BOOL CScreenRecorderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CScreenRecorderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CScreenRecorderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CScreenRecorderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

int printLog(char *fmt, ...)
{
	char buffer[4096] = {};

	va_list argptr;
	int cnt;
	va_start(argptr, fmt);

	cnt = vsnprintf(buffer, 4096, fmt, argptr);

	va_end(argptr);

	OutputDebugString(buffer);

	return(cnt);
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {

	PKBDLLHOOKSTRUCT keyInfo = (PKBDLLHOOKSTRUCT)(lParam);
	POINT p;
	if (WM_KEYDOWN == wParam || WM_SYSKEYDOWN == wParam)
	{
		if (keyInfo->vkCode == VK_F2)
		{
			if (startCapture)
			{
				startCapture = false;
			}
			else
			{
				startCapture = true;
				//OnBnClickedButton2();
			}

			return TRUE;
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {

	PKBDLLHOOKSTRUCT k = (PKBDLLHOOKSTRUCT)(lParam);
	POINT p;

	if (wParam == WM_LBUTTONDOWN)
	{
		// right button clicked 
		GetCursorPos(&p);

		printLog("x:%d, y:%d\n", p.x, p.y);
	}

	return 0;
}

void CScreenRecorderDlg::OnMouseMove(UINT nFlags, CPoint point)
{


	//	printLog("GetCursorPos -> (%ld, %ld)\n", point.x, point.y);
	//
	//#if 0
	//	//获取桌面句柄
	//	HWND desktopHwnd = ::GetDesktopWindow();
	//
	//	ret = EnumChildWindows(desktopHwnd, EnumChildProcess, (LPARAM)&point);
	//#else
	//	CRect rtSelect;
	//	//rtSelect = CSCWinSpy<CSCWinFilter>::GetInstance()->GetWinRectByPoint(point, TRUE);
	//
	//	CString title;
	//	HWND hTargetWnd;
	//	BOOL ret = CSCWinSpy<CSCWinFilter>::GetInstance()->GetWindowByPoint(point, rtSelect, title, hTargetWnd);
	//
	//	if (ret)
	//	{
	//		printLog("CSCWinSpy -> (%ld, %ld, %s)\n", rtSelect.left, rtSelect.top, title);
	//
	//		CStatic  *pStatic = (CStatic*)GetDlgItem(IDC_STATIC_SELECTED_WND);
	//		pStatic->SetWindowText(title);
	//	}
	//
	//#endif


	CDialog::OnMouseMove(nFlags, point);
}

void CScreenRecorderDlg::OnBnClickedButton1()
{

	HHOOK  MouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);


	//1.先获得桌面窗口
	CWnd* pDesktopWnd = CWnd::GetDesktopWindow();
	//2.获得一个子窗口
	CWnd* pWnd = pDesktopWnd->GetWindow(GW_CHILD);
	//3.循环取得桌面下的所有子窗口
	while (pWnd != NULL)
	{
		TCHAR buffer[256] = {};

		HWND handle = pWnd->GetSafeHwnd();
		//获得窗口类名
		::GetClassName(handle, buffer, 256);
		CString strClassName = buffer;

		//获得窗口标题
		::GetWindowText(handle, buffer, 256);
		CString  strWindowText = buffer;

		RECT rect;
		::GetWindowRect(handle, &rect);

		if (strWindowText.GetLength() > 0)
		{
			CString strLog;
			strLog.Format(_T("0x%x, %s, x:%d, y:%d, w:%d, h:%d\n"),
				handle, (LPCTSTR)strWindowText, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
			OutputDebugString(strLog);
		}

		//继续下一个子窗口
		pWnd = pWnd->GetWindow(GW_HWNDNEXT);
	}


}

#define MAX_TEXT_LEN 255

BOOL CALLBACK EnumChildProcess(HWND hwnd, LPARAM lParam)
{
	if (hwnd == NULL) {
		return FALSE;
	}
	BOOL ret;
	RECT rect;
	ret = GetWindowRect(hwnd, &rect);
	if (!ret) {
		printLog("GetWindowRect hwnd=%p -> fail(%ld)\n", hwnd, GetLastError());
	}
	else {
		//printLog("GetWindowRect hwnd = %p -> rect=(left=%ld, top=%ld, right=%ld, bottom=%ld)\n", hwnd, rect.left, rect.top, rect.right, rect.bottom);
		ret = PtInRect(&rect, *(POINT *)lParam);
		if (ret) {
			printLog("GetWindowRect hwnd = %p -> rect=(left=%ld, top=%ld, right=%ld, bottom=%ld)\n", hwnd, rect.left, rect.top, rect.right, rect.bottom);
			//printLog("PtInRect\n");

			/*
			WINUSERAPI int WINAPI GetWindowText(
			_In_ HWND hWnd,
			_Out_writes_(nMaxCount) LPTSTR lpString,    //可能是标题名或者file:///打头的文件完整路径
			_In_ int nMaxCount
			);
			如果函数成功，返回值是拷贝的字符串的字符个数，不包括中断的空字符；如果窗口无标题栏或文本，或标题栏为空，或窗口或控制的句柄无效，则返回值为零。若想获得更多错误信息，请调用GetLastError函数。
			*/
			TCHAR windowText[MAX_TEXT_LEN];
			int lenRet = GetWindowText(hwnd, windowText, MAX_TEXT_LEN);
			if (lenRet == 0 && GetLastError() != 0) {
				//GetLastError()〖0〗-操作成功完成
				printLog("GetWindowText hwnd=%p -> fail(%ld)\n", hwnd, GetLastError());
			}
			else {
				printLog("GetWindowText hwnd=%p -> windowText=%s, lenRet=%d\n", hwnd, windowText, lenRet);
			}

			/*
			WINUSERAPI int WINAPI GetClassNameW(
			_In_ HWND hWnd,
			_Out_writes_to_(nMaxCount, return) LPWSTR lpClassName,
			_In_ int nMaxCount
			);

			如果函数成功，返回值为拷贝到指定缓冲区的字符个数：如果函数失败，返回值为0。若想获得更多错误信息，请调用GetLastError函数。
			*/
			TCHAR className[MAX_TEXT_LEN];
			lenRet = GetClassName(hwnd, className, MAX_TEXT_LEN);
			if (lenRet == 0) {
				printLog("GetClassName hwnd=%p -> fail(%ld)\n", hwnd, GetLastError());
			}
			else {
				printLog(_T("GetClassName hwnd=%p -> className=%s, lenRet=%d\n"), hwnd, className, lenRet);
			}

			/*
			找出某个窗口的创建者（线程或进程），返回创建者的标志符
			哪个线程创建了这个窗口,返回的就是这个线程的id号 （进程只有一个线程的话，那么线程标志符与进程标志符就是指同一个标志符）
			WINUSERAPI DWORD WINAPI GetWindowThreadProcessId(
			_In_ HWND hWnd,
			_Out_opt_ LPDWORD lpdwProcessId //进程号的存放地址（变量地址）
			);
			返回线程号
			*/
			DWORD dwProcessId;
			DWORD dwThreadId = GetWindowThreadProcessId(hwnd, &dwProcessId);
			printLog("GetWindowThreadProcessId hwnd=%p -> processId=%ld, threadId=%ld\n", hwnd, dwProcessId, dwThreadId);

			/*
			WINUSERAPI UINT WINAPI GetWindowModuleFileName(
			_In_ HWND hwnd,
			_Out_writes_to_(cchFileNameMax, return) LPTSTR pszFileName, //模块完整路径
			_In_ UINT cchFileNameMax
			);
			返回值是复制到缓冲区的字符总数。
			*/
			TCHAR fileName[MAX_PATH];
			lenRet = GetWindowModuleFileName(hwnd, fileName, MAX_PATH);
			if (lenRet == 0) {
				//错误码〖126〗-找不到指定的模块。
				printLog("GetWindowModuleFileName hwnd=%p -> fail(%ld)\n", hwnd, GetLastError());
			}
			else {
				printLog(_T("GetWindowModuleFileName hwnd=%p -> fileName=%s\n"), hwnd, fileName);
			}

			/*
			WINUSERAPI BOOL WINAPI GetWindowInfo(
			_In_ HWND hwnd,
			_Inout_ PWINDOWINFO pwi
			);

			typedef struct tagWINDOWINFO
			{
			DWORD cbSize;
			RECT rcWindow;
			RECT rcClient;
			DWORD dwStyle;
			DWORD dwExStyle;
			DWORD dwWindowStatus;
			UINT cxWindowBorders;
			UINT cyWindowBorders;
			ATOM atomWindowType;
			WORD wCreatorVersion;
			} WINDOWINFO, *PWINDOWINFO, *LPWINDOWINFO;
			*/
			WINDOWINFO windowInfo;
			windowInfo.cbSize = sizeof(WINDOWINFO);
			ret = GetWindowInfo(hwnd, &windowInfo);
			if (!ret) {
				printLog("GetWindowInfo hwnd=%p -> fail(%ld)\n", hwnd, GetLastError());
			}
			else {
				printLog("GetWindowInfo hwnd=%p -> dwStyle=%ld, dwExStyle=%ld, dwWindowStatus=%ld, cxWindowBorders=%d, cyWindowBorders=%d, wCreatorVersion=%d\n", hwnd, windowInfo.dwStyle, windowInfo.dwExStyle, windowInfo.dwWindowStatus, windowInfo.cxWindowBorders, windowInfo.cyWindowBorders, windowInfo.wCreatorVersion);
			}
			printLog("\n");
		}
	}

	return TRUE;
}


boost::thread *pThread = nullptr;

//
// Returns the real parent window
// Same as GetParent(), but doesn't return the owner
//
HWND GetRealParent(HWND hWnd)
{
	HWND hParent;

	hParent = GetAncestor(hWnd, GA_PARENT);
	if (!hParent || hParent == GetDesktopWindow())
		return NULL;

	return hParent;
}

void CScreenRecorderDlg::OnBnClickedButton2()
{
	startCapture = !startCapture;

	if (pThread)
	{
		pThread->join();
		delete pThread;
		pThread = nullptr;
	}

	KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, AfxGetInstanceHandle(), 0);

	CStatic *pStatic = (CStatic*)GetDlgItem(IDC_STATIC_SELECTED_WND);

	pThread = new boost::thread([pStatic, this]()
	{
		CSCWinSpy<CSCWinFilter>::GetInstance()->SnapshotAllWinRect();

		POINT lastPoint = {};
		HWND hLastTargetWnd = NULL;
		DWORD lastProcessId = 0;
		BOOL ret = 0;
		while (startCapture)
		{

			POINT point;
			ret = GetCursorPos(&point);
			if (!ret) {
				printLog("GetCursorPos -> fail(%ld)\n", GetLastError());
			}
			else
			{

				if (lastPoint.x + lastPoint.y == point.x + point.y)
				{
					Sleep(500);
					continue;
				}

				//printLog("GetCursorPos -> (%ld, %ld)\n", point.x, point.y);

				lastPoint = point;
#if 0
				//获取桌面句柄
				HWND desktopHwnd = ::GetDesktopWindow();

				ret = EnumChildWindows(desktopHwnd, EnumChildProcess, (LPARAM)&point);
#else
				CRect rtSelect;
				//rtSelect = CSCWinSpy<CSCWinFilter>::GetInstance()->GetWinRectByPoint(point, TRUE);

				CString title;
				HWND hTargetWnd;
				BOOL ret = CSCWinSpy<CSCWinFilter>::GetInstance()->GetWindowByPoint(point, rtSelect, title, hTargetWnd);

				if (ret && hTargetWnd != hLastTargetWnd)
				{

					DWORD processId = 0;
					::GetWindowThreadProcessId(hTargetWnd, &processId);

					if (lastProcessId == processId)
					{
						continue;
					}

					lastProcessId = processId;

					printLog("CSCWinSpy -> (Pid:%ld, %ld, %ld, %s)\n", processId, rtSelect.left, rtSelect.top, title);

					//pStatic->SetWindowText(title);

					HWND baseWnd = hTargetWnd;
					HWND parent = NULL;
					TCHAR buffer[4096];
					RECT rc;
					int level = 1;
					do
					{
						if (parent != NULL)
						{
							::GetWindowText(parent, buffer, 4096);
							::GetWindowRect(parent, &rc);

							for (int i = 0; i < level; i++) printLog("\t");
							printLog("Parent -> (%ld, %ld),(w:%ld, h:%ld), %s)\n", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, buffer);

							level++;
							baseWnd = parent;
						}

					} while ((parent = ::GetRealParent(baseWnd)) != NULL);

					if (level == 1)
					{
						::GetWindowRect(baseWnd, &rc);
						::GetWindowText(baseWnd, buffer, 4096);
					}

					title.Format("(%ld, %ld),(w:%ld, h:%ld), %s)", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, buffer);
					pStatic->SetWindowText(title);

					currentBaseWnd = baseWnd;
					//CSCWinSpy<CSCWinFilter>::GetInstance()->HighlightFoundWindow(hTargetWnd);
				}

				hLastTargetWnd = hTargetWnd;
#endif
			}

			Sleep(500);
		}

		printLog("Capture Finished\n");

		UnhookWindowsHookEx(KeyboardHook);

	});

}


void CScreenRecorderDlg::OnBnClickedButton3()
{
	boost::thread th([this]()
	{
		CSCWinSpy<CSCWinFilter>::GetInstance()->HighlightFoundWindow(currentBaseWnd);
	});

}


void CScreenRecorderDlg::OnBnClickedButton4()
{
	boost::thread th([this]()
	{
		CSCWinSpy<CSCWinFilter>::GetInstance()->ShakeWindow(currentBaseWnd);
	});
}


void CScreenRecorderDlg::OnBnClickedButton5()
{
	CStatic *pStatic = (CStatic*)GetDlgItem(IDC_STATIC_SELECTED_WND);

	boost::thread th([pStatic, this]()
	{
		while (!startCapture)
		{
			RECT lastRc = {};
			if (currentBaseWnd)
			{
				RECT rc = {};
				::GetWindowRect(currentBaseWnd, &rc);

				if (lastRc.left != rc.left || lastRc.right != rc.right
					|| lastRc.top != rc.top || lastRc.bottom != rc.bottom)
				{
					char buffer[4096];
					::GetWindowText(currentBaseWnd, buffer, 4096);

					CString title;
					title.Format("(%ld, %ld),(w:%ld, h:%ld), %s)", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, buffer);
					pStatic->SetWindowText(title);

					lastRc = rc;
				}
			}

			Sleep(40);
		}
	});
}

#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)
#define HRCHECK(__expr) {hr=(__expr);if(FAILED(hr)){wprintf(L"FAILURE 0x%08X (%i)\n\tline: %u file: '%s'\n\texpr: '" WIDEN(#__expr) L"'\n",hr, hr, __LINE__,__WFILE__);goto cleanup;}}
#define RELEASE(__p) {if(__p!=nullptr){__p->Release();__p=nullptr;}}

HRESULT CScreenRecorderDlg::SavePixelsToFile32bppPBGRA(UINT width, UINT height, UINT stride, LPBYTE pixels, LPWSTR filePath, const GUID &format)
{
	if (!filePath || !pixels)
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	IWICImagingFactory *factory = nullptr;
	IWICBitmapEncoder *encoder = nullptr;
	IWICBitmapFrameEncode *frame = nullptr;
	IWICStream *stream = nullptr;
	GUID pf = GUID_WICPixelFormat32bppPBGRA;
	BOOL coInit = CoInitialize(nullptr);

	HRCHECK(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));
	HRCHECK(factory->CreateStream(&stream));
	HRCHECK(stream->InitializeFromFilename(filePath, GENERIC_WRITE));
	HRCHECK(factory->CreateEncoder(format, nullptr, &encoder));
	HRCHECK(encoder->Initialize(stream, WICBitmapEncoderNoCache));
	HRCHECK(encoder->CreateNewFrame(&frame, nullptr)); // we don't use options here
	HRCHECK(frame->Initialize(nullptr)); // we dont' use any options here
	HRCHECK(frame->SetSize(width, height));
	HRCHECK(frame->SetPixelFormat(&pf));
	HRCHECK(frame->WritePixels(height, stride, stride * height, pixels));
	HRCHECK(frame->Commit());
	HRCHECK(encoder->Commit());

cleanup:
	RELEASE(stream);
	RELEASE(frame);
	RELEASE(encoder);
	RELEASE(factory);
	if (coInit) CoUninitialize();
	return hr;
}

HRESULT CScreenRecorderDlg::Direct3D9TakeScreenshots(UINT adapter, UINT count)
{
	HRESULT hr = S_OK;
	IDirect3D9 *d3d = nullptr;
	IDirect3DDevice9 *device = nullptr;
	IDirect3DSurface9 *surface = nullptr;
	D3DPRESENT_PARAMETERS parameters = { 0 };
	D3DDISPLAYMODE mode;
	D3DLOCKED_RECT rc;
	UINT pitch;
	SYSTEMTIME st;
	LPBYTE *shots = nullptr;

	// init D3D and get screen size
	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	HRCHECK(d3d->GetAdapterDisplayMode(adapter, &mode));

	mode.Height = 600;
	mode.Width = 800;

	parameters.Windowed = TRUE;
	parameters.BackBufferCount = 1;
	parameters.BackBufferHeight = mode.Height;
	parameters.BackBufferWidth = mode.Width;
	parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	parameters.hDeviceWindow = NULL;

	// create device & capture surface
	HRCHECK(d3d->CreateDevice(adapter, D3DDEVTYPE_HAL, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &parameters, &device));
	HRCHECK(device->CreateOffscreenPlainSurface(mode.Width, mode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, nullptr));

	// compute the required buffer size
	HRCHECK(surface->LockRect(&rc, NULL, 0));
	pitch = rc.Pitch;
	HRCHECK(surface->UnlockRect());

	// allocate screenshots buffers
	shots = new LPBYTE[count];
	for (UINT i = 0; i < count; i++)
	{
		shots[i] = new BYTE[pitch * mode.Height];
	}

	GetSystemTime(&st); // measure the time we spend doing <count> captures
	printf("%i:%i:%i.%i\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	for (UINT i = 0; i < count; i++)
	{
		// get the data
		HRCHECK(device->GetFrontBufferData(0, surface));

		// copy it into our buffers
		HRCHECK(surface->LockRect(&rc, NULL, 0));
		CopyMemory(shots[i], rc.pBits, rc.Pitch * mode.Height);
		HRCHECK(surface->UnlockRect());
	}
	GetSystemTime(&st);
	printf("%i:%i:%i.%i\n", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	// save all screenshots
	// save all screenshots
	for (UINT i = 0; i < count; i++)
	{
		WCHAR file[100];
		wsprintfW(file, L"cap%i.png", i);
		HRCHECK(SavePixelsToFile32bppPBGRA(mode.Width, mode.Height, pitch, shots[i], file, GUID_ContainerFormatPng));
	}

cleanup:
	if (shots != nullptr)
	{
		for (UINT i = 0; i < count; i++)
		{
			delete shots[i];
		}
		delete[] shots;
	}
	RELEASE(surface);
	RELEASE(device);
	RELEASE(d3d);
	return hr;
}

void CScreenRecorderDlg::ScreenShot(char *filename)
{
	//HWND hwnd = ::GetDesktopWindow();//截整个屏幕
	//rc.SetRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));//截整个屏幕

	HWND hwnd = currentBaseWnd;
	HDC hdc = ::GetDC(hwnd);
	CDC dc;
	dc.Attach(hdc);

	RECT rect;
	::GetWindowRect(hwnd, &rect);

	CRect rc(rect);

	//CClientDC dc(this);//只截对话框，用这句  
	//GetClientRect(&rc);//只截对话框，用这句  

	int iBitPerPixel = dc.GetDeviceCaps(BITSPIXEL);
	int iWidth = rc.Width();
	int iHeight = rc.Height();

	CDC memDC;
	memDC.CreateCompatibleDC(&dc);

	CBitmap memBitmap, *oldBitmap;

	memBitmap.CreateCompatibleBitmap(&dc, iWidth, iHeight);
	oldBitmap = memDC.SelectObject(&memBitmap);
	memDC.SetStretchBltMode(COLORONCOLOR);

	memDC.BitBlt(0, 0, iWidth, iHeight, &dc, 0, 0, SRCCOPY);

	BITMAP bmp;
	memBitmap.GetBitmap(&bmp);

	FILE *fp = fopen("snapshot.bmp", "wb");

	BITMAPINFOHEADER bih;
	memset(&bih, 0, sizeof(bih));
	bih.biBitCount = bmp.bmBitsPixel;
	bih.biCompression = BI_RGB;//表示不压缩  
	bih.biHeight = bmp.bmHeight;
	bih.biPlanes = 1;//位平面数，必须为1  
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biSizeImage = bmp.bmWidthBytes * bmp.bmHeight;
	bih.biWidth = bmp.bmWidth;
	BITMAPFILEHEADER bfh;
	memset(&bfh, 0, sizeof(bfh));
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	bfh.bfSize = bfh.bfOffBits + bmp.bmWidthBytes * bmp.bmHeight;
	bfh.bfType = (WORD)0x4d42;//必须表示"BM"  

	fwrite(&bfh, 1, sizeof(BITMAPFILEHEADER), fp);
	fwrite(&bih, 1, sizeof(bih), fp);

	byte * p = new byte[bmp.bmWidthBytes * bmp.bmHeight];
	GetDIBits(memDC.m_hDC, (HBITMAP)memBitmap.m_hObject, 0, iHeight, p, (LPBITMAPINFO)&bih, DIB_RGB_COLORS);
	fwrite(p, 1, bmp.bmWidthBytes * bmp.bmHeight, fp);
	delete[] p;

	fclose(fp);

	memDC.SelectObject(oldBitmap);
}

void CScreenRecorderDlg::OnBnClickedButton6()
{
	//HRESULT hr = Direct3D9TakeScreenshots(D3DADAPTER_DEFAULT, 1);
	ScreenShot(NULL);
}


void CScreenRecorderDlg::OnBnClickedButton7()
{
	if (recording)
	{
		recording = false;
	}
	else
	{
		recording = true;

		boost::thread th([this] {

			char offset_x[20];
			char offset_y[20];
			char video_size[20];
			char frameRate[20];

			RECT rc = {};
			::GetWindowRect(currentBaseWnd, &rc);

#define ALIGN(n, align) ((n + align - 1) & (~(align - 1)))

			int width = rc.right - rc.left;
			int height = rc.bottom - rc.top;

			width = ALIGN(width, 4);
			height = ALIGN(height, 4);

			sprintf(offset_x, "%d", rc.left);
			sprintf(offset_y, "%d", rc.top);
			sprintf(video_size, "%dx%d", width, height);
			sprintf(frameRate, "%d", 30);

			FFmpegEncoder *pEncoder = new FFmpegEncoder;
			pEncoder->init();
			pEncoder->start("output.mpg", boost::lexical_cast<int>(frameRate));

			AVFormatContext	*pFormatCtx;
			int				i, videoindex;
			AVCodecContext	*pCodecCtx;
			AVCodec			*pCodec;

			pFormatCtx = avformat_alloc_context();

			//Use gdigrab
			AVDictionary* options = NULL;

			//Set some options
			//grabbing frame rate
			av_dict_set(&options, "framerate", frameRate, 0);

			if (currentBaseWnd)
			{
				//The distance from the left edge of the screen or desktop
				av_dict_set(&options, "offset_x", offset_x, 0);
				//The distance from the top edge of the screen or desktop
				av_dict_set(&options, "offset_y", offset_y, 0);
				//Video frame size. The default is to capture the full screen
				av_dict_set(&options, "video_size", video_size, 0);
			}

			AVInputFormat *ifmt = av_find_input_format("gdigrab");
			if (avformat_open_input(&pFormatCtx, "desktop", ifmt, &options) != 0) {
				printf("Couldn't open input stream.\n");
				return;
			}

			if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
			{
				printf("Couldn't find stream information.\n");
				return;
			}

			videoindex = -1;
			for (i = 0; i < pFormatCtx->nb_streams; i++)
			{
				if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
				{
					videoindex = i;
					break;
				}
			}

			if (videoindex == -1)
			{
				printf("Didn't find a video stream.\n");
				return;
			}

			pCodecCtx = pFormatCtx->streams[videoindex]->codec;
			pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
			if (pCodec == NULL)
			{
				printf("Codec not found.\n");
				return;
			}

			if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
			{
				printf("Could not open codec.\n");
				return;
			}

			AVFrame	*pFrame = av_frame_alloc();
			AVFrame	*pFrameYUV = av_frame_alloc();

			unsigned char *out_buffer = (unsigned char *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
			avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

			int ret, got_picture;
			AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

			struct SwsContext *img_convert_ctx =
				sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
					AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

			FILE *fp_yuv = NULL;
			//fp_yuv = fopen("output.yuv", "wb+");

			while (this->recording)
			{
				if (av_read_frame(pFormatCtx, packet) >= 0)
				{
					if (packet->stream_index == videoindex) {
						ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
						if (ret < 0) {
							printf("Decode Error.\n");
							return;
						}
						if (got_picture) {

							sws_scale(img_convert_ctx, (const unsigned char* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

							pFrameYUV->height = pFrame->height;
							pFrameYUV->width = pFrame->width;
							pFrameYUV->format = AV_PIX_FMT_YUV420P;
							/*pFrameYUV->key_frame = pFrame->key_frame;*/

							av_frame_copy(pFrameYUV, pFrame);
							av_frame_copy_props(pFrameYUV, pFrame);

							pFrameYUV->pts = pFrame->pkt_pts;

							pEncoder->encode(pFrameYUV);

							//#if OUTPUT_YUV420P  
							if (fp_yuv)
							{
								int y_size = pCodecCtx->width*pCodecCtx->height;
								fwrite(pFrameYUV->data[0], 1, y_size, fp_yuv);    //Y   
								fwrite(pFrameYUV->data[1], 1, y_size / 4, fp_yuv);  //U  
								fwrite(pFrameYUV->data[2], 1, y_size / 4, fp_yuv);  //V  	
							}
							//#endif  

						}
					}
				}

				av_free_packet(packet);
			}

			pEncoder->flush_encoder();

			delete pEncoder;
			pEncoder = nullptr;

			av_free(out_buffer);
			av_frame_free(&pFrame);
			av_frame_free(&pFrameYUV);
			avformat_free_context(pFormatCtx);

			if (fp_yuv)
			{
				fclose(fp_yuv);
			}
		});
	}
}

