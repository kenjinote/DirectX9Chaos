﻿#pragma comment(lib,"comctl32")

#ifdef _WIN64
#pragma comment(lib,"lib\\x64\\d3d9")
#pragma comment(lib,"lib\\x64\\d3dx9")
#else
#pragma comment(lib,"lib\\x86\\d3d9")
#pragma comment(lib,"lib\\x86\\d3dx9")
#endif

#include<windows.h>
#include<commctrl.h>
#include<d3d9.h>
#include<d3dx9.h>
#include<stdio.h>
#include<math.h>
#include"resource.h"

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

#define POINTS 5000
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
#define WINDOW_ASPECT ((float)WINDOW_HEIGHT/(float)WINDOW_WIDTH)
#define D3DFVF_MY_VERTEX ( D3DFVF_XYZ | D3DFVF_DIFFUSE )
#define WM_RESET (WM_APP)

TCHAR szClassName[] = TEXT("Window");
LPDIRECT3D9       g_pD3D = NULL;
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
LPDIRECT3DVERTEXBUFFER9 g_pPointList_VB = NULL;
LPDIRECT3DTEXTURE9      m_ptexParticle; // Particle's texture

double a = -1.08;
double b = 1.491;
double c = 1.36;
double d = -1.68;
HWND hLightDialogWnd;

struct Vertex
{
	float x, y, z;
	DWORD color;
};

inline DWORD FtoDW(FLOAT f) { return *((DWORD*)&f); }

VOID DrawGLScene()
{
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		D3DCOLOR_COLORVALUE(0.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0);
	g_pd3dDevice->BeginScene();
	g_pd3dDevice->SetTexture(0, m_ptexParticle);
	g_pd3dDevice->DrawPrimitive(D3DPT_POINTLIST, 0, POINTS);
	g_pd3dDevice->EndScene();
	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

typedef struct
{
	int nEditID;
	int nSliderID;
	double*fOutValue;
} data_t;

void Function0(const Vertex* pPrevious, Vertex* pNext)
{
	pNext->x = (float)(sin(a * pPrevious->y) - cos(b * pPrevious->x));
	pNext->y = (float)(sin(c * pPrevious->x) - cos(d * pPrevious->y));
}

void Function1(const Vertex* pPrevious, Vertex* pNext)
{
	pNext->x = (float)(d*sin(a * pPrevious->x) - sin(b * pPrevious->y));
	pNext->y = (float)(c*cos(a * pPrevious->x) + cos(b * pPrevious->y));
}

void(*pFunction)(const Vertex* pPrevious, Vertex* pNext) = &Function0;

INT_PTR CALLBACK DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static const data_t data[] =
	{ { IDC_EDIT_A, IDC_SLIDER_A, &a },
	{ IDC_EDIT_B, IDC_SLIDER_B, &b },
	{ IDC_EDIT_C, IDC_SLIDER_C, &c },
	{ IDC_EDIT_D, IDC_SLIDER_D, &d }, };
	static TCHAR szText[256];
	switch (msg)
	{
	case WM_HSCROLL:
	{
		data_t* p = (data_t*)GetWindowLongPtr((HWND)lParam, GWLP_USERDATA);
		*(p->fOutValue) = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0) / 25.0 - 2.0;
		swprintf_s(szText, TEXT("%f"), *(p->fOutValue));
		SetDlgItemText(hWnd, p->nEditID, szText);
	}
	InvalidateRect(GetParent(hWnd), 0, 0);
	break;
	case WM_COMMAND:
		if (HIWORD(wParam) == EN_CHANGE)
		{
			data_t* p = (data_t*)GetWindowLongPtr((HWND)lParam, GWLP_USERDATA);
			GetWindowText((HWND)lParam, szText, 256);
			*(p->fOutValue) = wcstod(szText, 0);
			SendDlgItemMessage(hWnd, p->nSliderID, TBM_SETPOS, 1, (INT)((*(p->fOutValue) + 2.0f) * 25.0f));
			InvalidateRect(GetParent(hWnd), 0, 0);
		}
		else if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			switch (SendDlgItemMessage(hWnd, IDC_COMBO1, CB_GETCURSEL, 0, 0))
			{
			case 0:
				pFunction = Function0;
				break;
			case 1:
				pFunction = Function1;
				break;
			}
		}
		return 0;
		break;
	case WM_DRAWITEM:
		if ((UINT)wParam == IDC_COMBO1)
		{
			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
			if (lpdis->itemID == -1)break;
			HBITMAP hBitmap = LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP1 + lpdis->itemID));
			HDC hMemDC = CreateCompatibleDC(lpdis->hDC);
			SelectObject(hMemDC, hBitmap);
			BitBlt(lpdis->hDC, lpdis->rcItem.left, lpdis->rcItem.top + 1, 256, 60, hMemDC, 0, 0, SRCCOPY);
			DeleteDC(hMemDC);
			DeleteObject(hBitmap);
			if (lpdis->itemState & ODS_FOCUS)
			{
				DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
			}
		}
		break;
	case WM_MEASUREITEM:
	{
		LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
		if (lpmis->itemHeight < 62)
			lpmis->itemHeight = 62;
	}
	break;
	case WM_INITDIALOG:
	{
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, 0);
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, 0);
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_SETCURSEL, 0, 0);
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_SETITEMHEIGHT, -1, 62);

		for (int i = 0; i < sizeof data / sizeof data_t; i++)
		{
			SetWindowLongPtr(GetDlgItem(hWnd, data[i].nEditID), GWLP_USERDATA, (LONG_PTR)&data[i]);
			SetWindowLongPtr(GetDlgItem(hWnd, data[i].nSliderID), GWLP_USERDATA, (LONG_PTR)&data[i]);
		}
		for (int i = 0; i < sizeof data / sizeof data_t; i++)
		{
			swprintf_s(szText, TEXT("%f"), *(data[i].fOutValue));
			SetDlgItemText(hWnd, data[i].nEditID, szText);
			SendDlgItemMessage(hWnd, data[i].nSliderID, TBM_SETPOS, 1, (INT)((*(data[i].fOutValue) + 2.0f)*25.0f));
		}
	}
	return 1;
	}
	return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static Vertex *pVertex;
	switch (msg)
	{
	case WM_CREATE:
		InitCommonControls();
		{
			g_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
			D3DDISPLAYMODE d3ddm;

			g_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

			D3DPRESENT_PARAMETERS d3dpp;
			ZeroMemory(&d3dpp, sizeof(d3dpp));

			d3dpp.Windowed = TRUE;
			d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
			d3dpp.BackBufferFormat = d3ddm.Format;
			d3dpp.EnableAutoDepthStencil = TRUE;
			d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
			d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

			g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING,
				&d3dpp, &g_pd3dDevice);

			g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
			g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

			g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
			g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			D3DXMATRIX mProjection;
			D3DXMatrixPerspectiveFovLH(&mProjection, D3DXToRadian(60.0f), 1.0f, 1.0f, 100.0f);
			g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &mProjection);

			g_pd3dDevice->CreateVertexBuffer(POINTS * sizeof(Vertex), 0, D3DFVF_MY_VERTEX,
				D3DPOOL_DEFAULT, &g_pPointList_VB,
				NULL);

			pVertex = (Vertex*)GlobalAlloc(0, POINTS * sizeof(Vertex));

			D3DXMATRIX mWorld;
			D3DXMatrixTranslation(&mWorld, 0.0f, 0.0f, 5.0f);
			g_pd3dDevice->SetTransform(D3DTS_WORLD, &mWorld);

			g_pd3dDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
			g_pd3dDevice->SetRenderState(D3DRS_POINTSIZE, FtoDW(6.0f));

			g_pd3dDevice->SetStreamSource(0, g_pPointList_VB, 0, sizeof(Vertex));
			g_pd3dDevice->SetFVF(D3DFVF_MY_VERTEX);

			for (int i = 0; i < POINTS; i++)
			{
				pVertex[i].z = 0.0f;
				pVertex[i].color = D3DCOLOR_COLORVALUE(157.0f / 255.0f, 1.0, 157.0f / 255.0f, 0.8f);
			}

			g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

			D3DXCreateTextureFromResource(g_pd3dDevice, GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP3), &m_ptexParticle);
		}
		hLightDialogWnd = CreateDialog(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DialogProc);
		SetTimer(hWnd, 0x1234, 1, 0);
		break;
	case WM_RBUTTONDOWN:
		{
			//こっからスクリーンショット
			IDirect3DSurface9 *pSurface;
			const TCHAR*fileName = TEXT("hoge.bmp");

			int deskTopX = GetSystemMetrics(SM_CXSCREEN);
			int deskTopY = GetSystemMetrics(SM_CYSCREEN);
			g_pd3dDevice->CreateOffscreenPlainSurface(deskTopX, deskTopY, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pSurface, NULL);
			g_pd3dDevice->GetFrontBufferData(0, pSurface);
			RECT rect;
			GetWindowRect(hWnd, &rect);
			rect.bottom -= GetSystemMetrics(SM_CYDLGFRAME);
			rect.top += GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION);
			rect.left += GetSystemMetrics(SM_CXDLGFRAME);
			rect.right -= GetSystemMetrics(SM_CXDLGFRAME);
			D3DXSaveSurfaceToFile(fileName, D3DXIFF_BMP, pSurface, NULL, &rect);
			pSurface->Release();
		}
		break;
	case WM_TIMER:
		KillTimer(hWnd, 0x1234);
		{
			pVertex[0].x = pVertex[POINTS - 1].x;
			pVertex[0].y = pVertex[POINTS - 1].y;

			for (int i = 1; i < POINTS; i++)
			{
				pFunction(&(pVertex[i - 1]), &(pVertex[i]));
			}

			Vertex *pVertices = NULL;
			g_pPointList_VB->Lock(0, POINTS * sizeof(Vertex), (void**)&pVertices, 0);
			memcpy(pVertices, pVertex, POINTS * sizeof(Vertex));
			g_pPointList_VB->Unlock();
		}
		SetTimer(hWnd, 0x1234, 1, 0);
		break;
	case WM_MOVE:
		{
			RECT rect;
			GetWindowRect(hWnd, &rect);
			SetWindowPos(hLightDialogWnd, 0, rect.right, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
		}
		break;
	case WM_DESTROY:
		KillTimer(hWnd, 0x1234);
		GlobalFree(pVertex);
		if (m_ptexParticle != NULL)
			m_ptexParticle->Release();
		if (g_pPointList_VB != NULL)
			g_pPointList_VB->Release();
		if (g_pd3dDevice != NULL)
			g_pd3dDevice->Release();
		if (g_pD3D != NULL)
			g_pD3D->Release();
		PostQuitMessage(0);
		break;
	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 1;
		}
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = { 0, WndProc, 0, 0, hInstance, 0, LoadCursor(0, IDC_ARROW), 0, 0, szClassName };
	RegisterClass(&wndclass);
	RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
	AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, FALSE);
	HWND hWnd = CreateWindow(szClassName, TEXT("Chaos"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, 0, rect.right - rect.left, rect.bottom - rect.top, 0, 0, hInstance, 0);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	BOOL done = FALSE;
	INT_PTR ret = 0;
	for (;;)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			ret = ::GetMessage(&msg, 0, 0, 0);
			if (ret == 0 || ret == -1)
			{
				ret = msg.wParam;
				break;
			}
			if (!hLightDialogWnd || !IsDialogMessage(hLightDialogWnd, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			DrawGLScene();
		}
	}
	return (int)msg.wParam;
}
