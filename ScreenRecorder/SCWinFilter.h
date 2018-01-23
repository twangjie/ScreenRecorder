#pragma once

#include <vector>

class CSCWinFilter
{
public:
	static BOOL IsFilterWindow(HWND hWnd)
	{
		_ASSERTE(hWnd != NULL);
		DWORD dwProcessID = GetCurrentProcessId();
		if (hWnd != NULL && IsWindow(hWnd))
		{
			DWORD dwWinProcessId(0);
			GetWindowThreadProcessId(hWnd, &dwWinProcessId);
			if (dwProcessID == dwWinProcessId)
			{
				return TRUE;
			}
		}

		return FALSE;
	}

	static DWORD GetIncludeStyle()
	{
		return WS_VISIBLE;
	}

	static DWORD GetExcludeStyle()
	{
		return WS_MINIMIZE;
	}

	static DWORD GetExcludeStyleEx()
	{
		return  WS_EX_TRANSPARENT;
	}

	static BOOL IsTargetPopupWindow()
	{
		return FALSE;
	}
};

class CSCWinInfo
{
public:
	HWND m_hWnd;
	CRect m_rtWin;    //window rect

	INT m_nLevel;    // 1 - pop up window  ;  2N - child window
};

//pop up win 1 (level 1).. first Z order
//        child11 (level 2)
//        child12 (level 2)
//                chilld121 (level 3)
//                chilld122 (level 3)
//                
//        child3 (level 2)
//pop up win2
//        child21 (level 2)
//        child21 (level 2)
// .
// .
//pop up winN . last Z order


template<typename CWinFilterTraits = CSCWinFilter>
class CSCWinSpy
{
public:
	static CSCWinSpy* GetInstance()
	{
		static CSCWinSpy instance;
		return &instance;
	}

public:
	BOOL SnapshotAllWinRect()
	{
		ClearData();

		// cache current window Z order when call this function
		EnumWindows(EnumWindowsSnapshotProc, 1);

		return TRUE;
	}

	//get from current Z order of desktop
	HWND GetHWNDByPoint(CPoint pt)
	{
		m_hWndTarget = NULL;

		EnumWindows(EnumWindowsRealTimeProc, MAKELPARAM(pt.x, pt.y));

		return m_hWndTarget;
	}

	void ShakeWindow(HWND hwnd, int SHAKE = 5) {

		RECT rect;

		GetWindowRect(hwnd, &rect);

		for (int i = 0; i < 3; i++)
		{
			MoveWindow(hwnd, rect.left + SHAKE, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
			Sleep(20);
			MoveWindow(hwnd, rect.left + SHAKE, rect.top - SHAKE, rect.right - rect.left, rect.bottom - rect.top, TRUE);
			Sleep(20);
			MoveWindow(hwnd, rect.left, rect.top - SHAKE, rect.right - rect.left, rect.bottom - rect.top, TRUE);
			Sleep(20);
			MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
			Sleep(20);
		}
	}

	CRect GetWinRectByPoint(CPoint ptHit, BOOL bGetInRealTime = FALSE)
	{
		CRect rtRect(0, 0, 0, 0);
		if (bGetInRealTime) //get from current Z order
		{
			HWND hWndTarget = GetHWNDByPoint(ptHit);
			if (hWndTarget != NULL)
			{
				GetWindowRect(hWndTarget, &rtRect);

				TCHAR szText[1024] = { 0 };

				TCHAR windowText[255];
				int lenRet = GetWindowText(hWndTarget, windowText, 255);

				_snprintf_s(szText, 1024, _T("GetRectByPoint: pt(%d, %d), hWnd(%x), RECT(%d,%d) %s\n"),
					ptHit.x, ptHit.y, (UINT)hWndTarget, rtRect.right - rtRect.left, rtRect.bottom - rtRect.top, windowText);
				OutputDebugString(szText);

				ShakeWindow(hWndTarget);

				//HighlightFoundWindow(hWndTarget);
			}

		}
		else //get from snapshot cache
		{
			GetRectByPointFromSnapshot(ptHit, rtRect);
		}

		return rtRect;
	}

	BOOL GetWindowByPoint(CPoint ptHit, CRect &rtRect, CString &strTitle, HWND &hWnd)
	{
		HWND hWndTarget = GetHWNDByPoint(ptHit);
		if (hWndTarget != NULL)
		{
			hWnd = hWndTarget;

			GetWindowRect(hWndTarget, &rtRect);

			TCHAR szText[1024] = { 0 };

			TCHAR windowText[255];
			int lenRet = GetWindowText(hWndTarget, windowText, 255);

			_snprintf_s(szText, 1024, _T("GetRectByPoint: pt(%d, %d), hWnd(%x), RECT(%d,%d) %s"),
				ptHit.x, ptHit.y, (UINT)hWndTarget, rtRect.right - rtRect.left, rtRect.bottom - rtRect.top, windowText);
			//OutputDebugString(szText);

			strTitle = szText;

			//ShakeWindow(hWndTarget);

			//HighlightFoundWindow(hWndTarget);

			return TRUE;
		}

	
		return FALSE;
	}


	static long HighlightFoundWindow(HWND hwndFoundWindow)
	{

		static  HGDIOBJ g_hRectanglePen = NULL;

		// The DC of the found window.
		HDC     hWindowDC = NULL;

		// Handle of the existing pen in the DC of the found window.
		HGDIOBJ hPrevPen = NULL;

		// Handle of the existing brush in the DC of the found window.
		HGDIOBJ hPrevBrush = NULL;

		RECT        rect; // Rectangle area of the found window.
		long        lRet = 0;

		// Get the screen coordinates of the rectangle 
		// of the found window.
		GetWindowRect(hwndFoundWindow, &rect);

		// Get the window DC of the found window.
		hWindowDC = GetWindowDC(hwndFoundWindow);

		if (hWindowDC)
		{
			// Select our created pen into the DC and 
			// backup the previous pen.
			hPrevPen = SelectObject(hWindowDC, g_hRectanglePen);

			// Select a transparent brush into the DC and 
			// backup the previous brush.
			hPrevBrush = SelectObject(hWindowDC, GetStockObject(HOLLOW_BRUSH));

			// Draw a rectangle in the DC covering 
			// the entire window area of the found window.
			Rectangle(hWindowDC, 0, 0,
				rect.right - rect.left, rect.bottom - rect.top);

			// Reinsert the previous pen and brush 
			// into the found window's DC.
			SelectObject(hWindowDC, hPrevPen);

			SelectObject(hWindowDC, hPrevBrush);

			// Finally release the DC.
			ReleaseDC(hwndFoundWindow, hWindowDC);
		}

		return lRet;
	}

	//static long HighlightFoundWindow(HWND hwndFoundWindow)
	//{
	//	// The DC of the found window.
	//	HDC     hWindowDC = NULL;

	//	// Handle of the existing pen in the DC of the found window.
	//	HGDIOBJ hPrevPen = NULL;

	//	// Handle of the existing brush in the DC of the found window.
	//	HGDIOBJ hPrevBrush = NULL;

	//	RECT        rect; // Rectangle area of the found window.
	//	long        lRet = 0;

	//	// Get the screen coordinates of the rectangle 
	//	// of the found window.
	//	GetWindowRect(hwndFoundWindow, &rect);

	//	// Get the window DC of the found window.
	//	hWindowDC = GetWindowDC(hwndFoundWindow);

	//	if (hWindowDC)
	//	{
	//		HGDIOBJ redPen = CreatePen(PS_INSIDEFRAME, 5, RGB(255, 0, 0));
	//		HGDIOBJ greenPen = CreatePen(PS_INSIDEFRAME, 5, RGB(0, 255, 0));

	//		hPrevPen = SelectObject(hWindowDC, redPen);
	//		hPrevBrush = SelectObject(hWindowDC, GetStockObject(NULL_BRUSH));

	//		for (int i = 0; i < 10; i++)
	//		{
	//			// Select our created pen into the DC and 
	//			// backup the previous pen.
	//			SelectObject(hWindowDC, i % 2 == 0 ? redPen : greenPen);

	//			// Select a transparent brush into the DC and 
	//			// backup the previous brush.
	//			SelectObject(hWindowDC, GetStockObject(NULL_BRUSH));

	//			// Draw a rectangle in the DC covering 
	//			// the entire window area of the found window.
	//			Rectangle(hWindowDC, 2, 2, rect.right - rect.left - 2, rect.bottom - rect.top - 2);

	//			Sleep(200);

	//			//// Reinsert the previous pen and brush 
	//			//// into the found window's DC.
	//			//SelectObject(hWindowDC, hPrevPen);
	//			//SelectObject(hWindowDC, hPrevBrush);

	//			//Rectangle(hWindowDC, 0, 0, rect.right - rect.left, rect.bottom - rect.top);
	//		}

	//		//SelectObject(hWindowDC, hPrevPen);
	//		//SelectObject(hWindowDC, hPrevBrush);

	//		//Rectangle(hWindowDC, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

	//		// Finally release the DC.
	//		ReleaseDC(hwndFoundWindow, hWindowDC);

	//		DeleteObject(redPen);
	//		DeleteObject(greenPen);
	//	}

	//	return lRet;
	//}


protected:
	static BOOL CALLBACK EnumWindowsRealTimeProc(HWND hwnd, LPARAM lParam)
	{
		if (!PtInWinRect(hwnd, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))) return TRUE;

		if (ShouldWinBeFiltered(hwnd))  return TRUE;

		m_hWndTarget = hwnd;

		if (CWinFilterTraits::IsTargetPopupWindow()) return FALSE; //this is the target window, exit search

		EnumChildWindows(hwnd, EnumChildRealTimeProc, lParam);

		return FALSE;
	}

	static BOOL CALLBACK EnumChildRealTimeProc(HWND hwnd, LPARAM lParam)
	{
		if (!PtInWinRect(hwnd, CPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))) return TRUE;

		if (ShouldWinBeFiltered(hwnd)) return TRUE;

		m_hWndTarget = hwnd;
		EnumChildWindows(hwnd, EnumChildRealTimeProc, lParam);

		return FALSE;
	}

protected:
	static BOOL CALLBACK EnumWindowsSnapshotProc(HWND hwnd, LPARAM lParam)
	{
		INT nLevel = lParam;
		if (ShouldWinBeFiltered(hwnd))  return TRUE;

		SaveSnapshotWindow(hwnd, nLevel);

		if (!CWinFilterTraits::IsTargetPopupWindow())
		{
			++nLevel;
			EnumChildWindows(hwnd, EnumChildSnapshotProc, nLevel);
		}

		return TRUE;
	}

	static BOOL CALLBACK EnumChildSnapshotProc(HWND hwnd, LPARAM lParam)
	{
		INT nLevel = lParam;

		if (ShouldWinBeFiltered(hwnd)) return TRUE;

		SaveSnapshotWindow(hwnd, nLevel);

		++nLevel;
		EnumChildWindows(hwnd, EnumChildSnapshotProc, nLevel);

		return TRUE;
	}

protected:
	static BOOL PtInWinRect(HWND hWnd, CPoint pt)
	{
		CRect rtWin(0, 0, 0, 0);
		GetWindowRect(hWnd, &rtWin);
		return PtInRect(&rtWin, pt);
	}

	static BOOL ShouldWinBeFiltered(HWND hWnd)
	{
		if (CWinFilterTraits::IsFilterWindow(hWnd)) return TRUE;

		DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
		DWORD dwStyleMust = CWinFilterTraits::GetIncludeStyle();
		if ((dwStyle & dwStyleMust) != dwStyleMust) return TRUE;

		DWORD dwStyleMustNot = CWinFilterTraits::GetExcludeStyle();
		if ((dwStyleMustNot & dwStyle) != 0) return TRUE;

		DWORD dwStyleEx = GetWindowLong(hWnd, GWL_EXSTYLE);
		DWORD dwStyleExMustNot = CWinFilterTraits::GetExcludeStyleEx();
		if ((dwStyleExMustNot & dwStyleEx) != 0) return TRUE;

		return FALSE;
	}

	//find the first window that level is biggest
	static BOOL  GetRectByPointFromSnapshot(CPoint ptHit, CRect& rtRet)
	{
		int nCount = m_arSnapshot.size();
		_ASSERTE(nCount > 0);
		CSCWinInfo* pInfo = NULL;
		CSCWinInfo* pTarget = NULL;

		for (int i = 0; i<nCount; ++i)
		{
			pInfo = m_arSnapshot[i];
			_ASSERTE(pInfo != NULL);

			//target window is found 
			//and level is not increasing, 
			//that is checking its sibling or parent window, exit search
			if (pTarget != NULL
				&& pInfo->m_nLevel <= pTarget->m_nLevel)
			{
				break;
			}

			if (PtInRect(&pInfo->m_rtWin, ptHit))
			{
				if (pTarget == NULL)
				{
					pTarget = pInfo;
				}
				else
				{
					if (pInfo->m_nLevel > pTarget->m_nLevel)
					{
						pTarget = pInfo;
					}
				}
			}
		}

		if (pTarget != NULL)
		{
			//#ifdef _DEBUG
			if (pTarget != NULL)
			{
				HWND hWnd = pTarget->m_hWnd;

				HighlightFoundWindow(hWnd);

				TCHAR szText[1024] = { 0 };

				TCHAR windowText[255];
				int lenRet = GetWindowText(hWnd, windowText, 255);

				_snprintf_s(szText, 1024, _T("GetRectByPointFromSnapshot: pt(%d, %d), hWnd(%x), %s\n"),
					ptHit.x, ptHit.y, (UINT)(pInfo->m_hWnd), windowText);
				OutputDebugString(szText);
			}
			//#endif

			rtRet.CopyRect(&pTarget->m_rtWin);
			return TRUE;
		}

		return FALSE;
	}

	static VOID SaveSnapshotWindow(HWND hWnd, INT nLevel)
	{
		_ASSERTE(hWnd != NULL && IsWindow(hWnd));
		CRect rtWin(0, 0, 0, 0);
		GetWindowRect(hWnd, &rtWin);
		if (rtWin.IsRectEmpty()) return;

		CSCWinInfo* pInfo = new CSCWinInfo;
		if (pInfo == NULL) return;

		pInfo->m_hWnd = hWnd;
		pInfo->m_nLevel = nLevel;
		pInfo->m_rtWin = rtWin;

		m_arSnapshot.push_back(pInfo);
	}

	static VOID ClearData()
	{
		int nCount = m_arSnapshot.size();
		for (int i = 0; i<nCount; ++i)
		{
			delete m_arSnapshot[i];
		}

		m_arSnapshot.clear();
	}


protected:
	friend class CSCWinSpy;

	CSCWinSpy() { NULL; }
	~CSCWinSpy() { ClearData(); }

	static HWND m_hWndTarget;
	static std::vector<CSCWinInfo*> m_arSnapshot;
};

template<typename T> HWND CSCWinSpy<T>::m_hWndTarget = NULL;
template<typename T> std::vector<CSCWinInfo*> CSCWinSpy<T>::m_arSnapshot;
