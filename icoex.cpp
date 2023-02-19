#include <windows.h>
#include <shlobj.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <new>
using namespace std;

#include "icoex.h"
#include "resource.h"
#include "stream.h"
#include "ne.h"

HINSTANCE g_hInstance;
HWND g_hMainWnd;
HACCEL g_hAccel;
INT g_i;
BOOL g_fSuccess;
TCHAR g_szFilter1[512];
TCHAR g_szFilter2[512];
TCHAR g_szExeFileName[MAX_PATH] = TEXT("");
TCHAR g_szDir[MAX_PATH];
TCHAR g_szCaption[256];
static const TCHAR g_szWndClass[] = TEXT("Extract Icon App");

vector<HICON> g_icons;

LPTSTR LoadStringDx(INT ids)
{
    static TCHAR sz[1024];
    LoadString(g_hInstance, ids, sz, 1024);
    return sz;
}

BOOL CALLBACK CountResNameProc(HMODULE hModule, LPCTSTR pszType, LPTSTR pszName, LONG_PTR lParam)
{
    INT *pi = (INT *)lParam;
    *pi += 1;
    return TRUE;
}

BOOL BrowseForFolderDx(LPTSTR szDir)
{
    BROWSEINFO bi;
    LPITEMIDLIST pidl;
    TCHAR szDisplayName[MAX_PATH];
    ZeroMemory(&bi, sizeof(BROWSEINFO));
    bi.hwndOwner = g_hMainWnd;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = szDisplayName;
    bi.lpszTitle = LoadStringDx(1);
    bi.ulFlags = BIF_RETURNONLYFSDIRS;
    pidl = SHBrowseForFolder(&bi);
    if (pidl == NULL)
    {
        return FALSE;
    }
    SHGetPathFromIDList(pidl, szDir);
    CoTaskMemFree(pidl);
    return TRUE;
}

BOOL GetIcoFileName(LPTSTR pszFileName)
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.hwndOwner       = g_hMainWnd;
    ofn.hInstance       = g_hInstance;
    ofn.lpstrFilter     = g_szFilter2;
    ofn.nFilterIndex    = 0;
    ofn.lpstrFile       = pszFileName;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrTitle      = LoadStringDx(11);
    ofn.Flags           = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
                          OFN_HIDEREADONLY;
    ofn.lpstrDefExt     = TEXT("ico");
    if (!GetSaveFileName(&ofn))
    {
        if (CommDlgExtendedError() != 0)
            MessageBox(g_hMainWnd, TEXT("CommDlgExtendedError"), NULL, MB_ICONERROR);
        return FALSE;
    }
    return TRUE;
}

BOOL GetExeFileName(LPTSTR pszFileName)
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ofn.lStructSize     = sizeof(OPENFILENAME);
    ofn.hwndOwner       = g_hMainWnd;
    ofn.hInstance       = g_hInstance;
    ofn.lpstrFilter     = g_szFilter1;
    ofn.nFilterIndex    = 0;
    ofn.lpstrFile       = pszFileName;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrTitle      = LoadStringDx(10);
    ofn.Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt     = TEXT("dll");
    if (!GetOpenFileName(&ofn))
    {
        if (CommDlgExtendedError() != 0)
            MessageBox(g_hMainWnd, TEXT("CommDlgExtendedError"), NULL, MB_ICONERROR);
        return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK EnumResNameProc(HMODULE hModule, LPCTSTR pszType, LPTSTR pszName, LONG_PTR lParam)
{
    INT i;
    TCHAR szFileName[MAX_PATH];
    HANDLE hFile;
    DWORD cbWritten;
    MEMICONDIRHEADER *pHeader;
    MEMICONDIRENTRY *pEntries;
    ICONDIRHEADER id;
    ICONDIRENTRY ide;
    INT *pi = (INT *)lParam;

    if (g_i == -1 || *pi == g_i)
    {
        if (g_i == -1)
        {
            wsprintf(szFileName, LoadStringDx(2), g_szDir, *pi);
        }
        else
        {
            wsprintf(szFileName, LoadStringDx(3), *pi);
            if (!GetIcoFileName(szFileName))
            {
                g_fSuccess = FALSE;
                return FALSE;
            }
        }
        hFile = CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            MessageBox(g_hMainWnd, LoadStringDx(4), NULL, MB_ICONERROR);
            g_fSuccess = FALSE;
            return FALSE;
        }

        HRSRC hGroupIconRsrc = FindResource(hModule, pszName, RT_GROUP_ICON);
        HGLOBAL hglbGroupIcon = LoadResource(hModule, hGroupIconRsrc);
        LPVOID pGroupIcon = LockResource(hglbGroupIcon);
        pHeader = (MEMICONDIRHEADER *)pGroupIcon;
        pEntries = (MEMICONDIRENTRY *)(pHeader + 1);

        id.idReserved   = 0;
        id.idType       = 1;
        id.idCount      = pHeader->idCount;
        ide.bReserved       = 0;
        ide.wPlanes         = 1;
        if (WriteFile(hFile, &id, sizeof(ICONDIRHEADER), &cbWritten, NULL))
        {
            ide.dwImageOffset = sizeof(ICONDIRHEADER) + id.idCount * sizeof(ICONDIRENTRY);
            for(i = 0; i < id.idCount; i++)
            {
                ide.bWidth          = pEntries[i].bWidth;
                ide.bHeight         = pEntries[i].bHeight;
                ide.bColorCount     = pEntries[i].bColorCount;
                ide.wBitCount       = pEntries[i].wBitCount;
                ide.dwBytesInRes    = pEntries[i].dwBytesInRes;
                if (WriteFile(hFile, &ide, sizeof(ICONDIRENTRY), &cbWritten, NULL))
                {
                    ;
                }
                else
                {
                    CloseHandle(hFile);
                    DeleteFile(szFileName);
                    MessageBox(g_hMainWnd, LoadStringDx(5), NULL, MB_ICONERROR);
                    g_fSuccess = FALSE;
                    return FALSE;
                }
                ide.dwImageOffset += ide.dwBytesInRes;
            }
            for(i = 0; i < id.idCount; i++)
            {
                HRSRC hIconRsrc = FindResource(hModule, MAKEINTRESOURCE(pEntries[i].nID), RT_ICON);
                DWORD cbIcon = SizeofResource(hModule, hIconRsrc);
                HGLOBAL hglbIcon = LoadResource(hModule, hIconRsrc);
                LPVOID pIcon = LockResource(hglbIcon);
                if (WriteFile(hFile, pIcon, cbIcon, &cbWritten, NULL))
                {
                    ;
                }
                else
                {
                    CloseHandle(hFile);
                    DeleteFile(szFileName);
                    MessageBox(g_hMainWnd, LoadStringDx(5), NULL, MB_ICONERROR);
                    g_fSuccess = FALSE;
                    return FALSE;
                }
            }
        }
        else
        {
            CloseHandle(hFile);
            DeleteFile(szFileName);
            MessageBox(g_hMainWnd, LoadStringDx(5), NULL, MB_ICONERROR);
            g_fSuccess = FALSE;
            return FALSE;
        }
        CloseHandle(hFile);
    }
    *pi += 1;
    return TRUE;
}

BOOL ExtractIconFromExe(LPCTSTR pszExeFile, INT index)
{
    INT i;
    HINSTANCE hInst;
    hInst = LoadLibraryEx(pszExeFile, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hInst != NULL)
    {
        i = 0;
        g_i = index;
        g_fSuccess = TRUE;
        EnumResourceNames(hInst, RT_GROUP_ICON, EnumResNameProc, (LONG_PTR)&i);
        FreeLibrary(hInst);
        if (g_fSuccess)
        {
            TCHAR szCaption[256];
            LoadString(g_hInstance, 14, szCaption, 256);
            MessageBox(g_hMainWnd, LoadStringDx(13), szCaption, MB_ICONINFORMATION);
            return TRUE;
        }
    }
    else
    {
        vector<vector<NE_GRPICONDIRENTRY> > egg;
        vector<DataStream> images;
        INT i, j;
        TCHAR szFileName[MAX_PATH];
        HANDLE hFile;
        DWORD cbWritten;
        if (LoadNe(pszExeFile, egg, images))
        {
            for(j = 0; j < egg.size(); j++)
            {
                if (index == -1 || j == index)
                {
                    if (index == -1)
                    {
                        wsprintf(szFileName, LoadStringDx(2), g_szDir, j);
                    }
                    else
                    {
                        wsprintf(szFileName, LoadStringDx(3), j);
                        if (!GetIcoFileName(szFileName))
                        {
                            g_fSuccess = FALSE;
                            return FALSE;
                        }
                    }
                    hFile = CreateFile(szFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
                    if (hFile == INVALID_HANDLE_VALUE)
                    {
                        MessageBox(g_hMainWnd, LoadStringDx(4), NULL, MB_ICONERROR);
                        g_fSuccess = FALSE;
                        return FALSE;
                    }
                    ICONDIRHEADER id;
                    ICONDIRENTRY ide;
                    id.idReserved   = 0;
                    id.idType       = 1;
                    id.idCount      = egg[j].size();
                    ide.bReserved       = 0;
                    ide.wPlanes         = 1;
                    if (WriteFile(hFile, &id, sizeof(ICONDIRHEADER), &cbWritten, NULL))
                    {
                        ide.dwImageOffset = sizeof(ICONDIRHEADER) + id.idCount * sizeof(ICONDIRENTRY);
                        for(i = 0; i < id.idCount; i++)
                        {
                            ide.bWidth          = egg[j][i].bWidth;
                            ide.bHeight         = egg[j][i].bHeight;
                            ide.bColorCount     = egg[j][i].bColorCount;
                            ide.wBitCount       = egg[j][i].wBitCount;
                            ide.dwBytesInRes    = egg[j][i].dwBytesInRes;
                            if (WriteFile(hFile, &ide, sizeof(ICONDIRENTRY), &cbWritten, NULL))
                            {
                                ;
                            }
                            else
                            {
                                CloseHandle(hFile);
                                DeleteFile(szFileName);
                                MessageBox(g_hMainWnd, LoadStringDx(5), NULL, MB_ICONERROR);
                                return FALSE;
                            }
                            ide.dwImageOffset += ide.dwBytesInRes;
                        }
                        INT iImage = 0;
                        for(INT k = 0; k < j; k++)
                        {
                            iImage += egg[k].size();
                        }
                        for(i = iImage; i < iImage + id.idCount; i++)
                        {
                            if (WriteFile(hFile, images[i].Ptr(), images[i].Size(), &cbWritten, NULL))
                            {
                                ;
                            }
                            else
                            {
                                CloseHandle(hFile);
                                DeleteFile(szFileName);
                                MessageBox(g_hMainWnd, LoadStringDx(5), NULL, MB_ICONERROR);
                                return FALSE;
                            }
                        }
                    }
                    else
                    {
                        CloseHandle(hFile);
                        DeleteFile(szFileName);
                        MessageBox(g_hMainWnd, LoadStringDx(5), NULL, MB_ICONERROR);
                        return FALSE;
                    }
                    CloseHandle(hFile);
                }
            }
            TCHAR szCaption[256];
            LoadString(g_hInstance, 14, szCaption, 256);
            MessageBox(g_hMainWnd, LoadStringDx(13), szCaption, MB_ICONINFORMATION);
            return TRUE;
        }
    }
    TCHAR szCaption[256];
    LoadString(g_hInstance, 14, szCaption, 256);
    MessageBox(g_hMainWnd, LoadStringDx(20), szCaption, MB_ICONINFORMATION);
    return FALSE;
}

BOOL CALLBACK LoadIconProc(HMODULE hModule, LPCTSTR pszType, LPTSTR pszName, LONG_PTR lParam)
{
    HRSRC hGroupIconRsrc = FindResource(hModule, pszName, RT_GROUP_ICON);
    HGLOBAL hglbGroupIcon = LoadResource(hModule, hGroupIconRsrc);
    LPVOID pGroupIcon = LockResource(hglbGroupIcon);
    INT nID = LookupIconIdFromDirectoryEx((LPBYTE)pGroupIcon, TRUE, 32, 32, LR_DEFAULTCOLOR);
    HRSRC hIconRsrc = FindResource(hModule, MAKEINTRESOURCE(nID), RT_ICON);
    HGLOBAL hglbIcon = LoadResource(hModule, hIconRsrc);
    LPVOID pIcon = LockResource(hglbIcon);
    HICON hIcon = CreateIconFromResourceEx((LPBYTE)pIcon, 
        SizeofResource(hModule, hIconRsrc), TRUE, 0x00030000, 32, 32,
        LR_DEFAULTCOLOR);
    g_icons.push_back(hIcon);
    return TRUE;
}

LPTSTR GetLastErrorMessage(void)
{
    static TCHAR sz[1024];
    FormatMessage(
        FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        0,
        sz,
        1024,
        NULL);
    return sz;
}

BOOL ResolveShortcut(HWND hWnd, LPCSTR pszLnkFile, LPTSTR pszPath)
{
    TCHAR           szPath[MAX_PATH];
#ifndef UNICODE
    WCHAR           wsz[MAX_PATH];
#endif
    IShellLink*     pShellLink;
    IPersistFile*   pPersistFile;
    WIN32_FIND_DATA find;
    BOOL            bRes = FALSE;
    
    szPath[0] = '\0';
    HRESULT hRes = CoInitialize(NULL);
    if (SUCCEEDED(hRes))
    {
        if (SUCCEEDED(hRes = CoCreateInstance(CLSID_ShellLink, NULL, 
            CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&pShellLink)))
        {
            if (SUCCEEDED(hRes = pShellLink->QueryInterface(IID_IPersistFile, 
                (void **)&pPersistFile)))
            {
#ifndef UNICODE
                MultiByteToWideChar(CP_ACP, 0, pszLnkFile, -1, wsz, MAX_PATH);
                hRes = pPersistFile->Load(wsz, STGM_READ);
#else
                hRes = pPersistFile->Load(pszLnkFile,  STGM_READ);
#endif
                if (SUCCEEDED(hRes))
                {
                    hRes = pShellLink->Resolve(hWnd, SLR_ANY_MATCH);
                    if (SUCCEEDED(hRes))
                    {
                        if (SUCCEEDED(hRes = pShellLink->GetPath(szPath, 
                            MAX_PATH, &find, SLGP_SHORTPATH)))
                        {
                            if ('\0' != szPath[0])
                            {
                                lstrcpy(pszPath, szPath);
                                bRes = TRUE;
                            }
                        }
                    }
                }
                pPersistFile->Release();
            }
            pShellLink->Release();
        }
        CoUninitialize();
    }
    return bRes;
}

VOID SetFileName(HWND hWnd, LPCTSTR pszFileName)
{
    TCHAR szFileTitle[MAX_PATH];
    TCHAR szCaption[1024];
    lstrcpyn(g_szExeFileName, pszFileName, MAX_PATH);
    GetFileTitle(pszFileName, szFileTitle, MAX_PATH);
    wsprintf(szCaption, LoadStringDx(19), szFileTitle);
    SetWindowText(hWnd, szCaption);
}

BOOL DoOpen(HWND hWnd, LPCTSTR pszFileName, HWND hListBox)
{
    INT i;
    LPCTSTR pch;
    TCHAR szFileName[MAX_PATH];
    
    pch = strrchr(pszFileName, '.');
    if (pch != NULL && lstrcmpi(pch, ".lnk") == 0)
    {
        TCHAR szFile[MAX_PATH];
        ResolveShortcut(hWnd, pszFileName, szFile);
        lstrcpyn(szFileName, szFile, MAX_PATH);
    }
    else
        lstrcpyn(szFileName, pszFileName, MAX_PATH);

    for(i = 0; i < (INT)g_icons.size(); i++)
        DestroyIcon(g_icons[i]);
    g_icons.clear();
    HINSTANCE hInst = LoadLibraryEx(szFileName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hInst != NULL)
    {
        i = 0;
        g_fSuccess = TRUE;
        EnumResourceNames(hInst, RT_GROUP_ICON, CountResNameProc, (LONG_PTR)&i);
        EnumResourceNames(hInst, RT_GROUP_ICON, LoadIconProc, (LONG_PTR)hListBox);
        FreeLibrary(hInst);
        if (i == 0)
            MessageBox(g_hMainWnd, LoadStringDx(16), NULL, MB_ICONINFORMATION);
        SendMessage(hListBox, LB_SETCOUNT, i, 0);
        SetFileName(hWnd, szFileName);
        return TRUE;
    }
    else
    {
        vector<vector<NE_GRPICONDIRENTRY> > egg;
        vector<DataStream> images;
        INT i, j, iImage;
        BOOL f;
        HICON hIcon;
        if (LoadNe(szFileName, egg, images))
        {
            iImage = 0;
            for(j = 0; j < egg.size(); j++)
            {
                f = FALSE;
                INT iImageOld = iImage;
                for(i = 0; i < egg[j].size(); i++)
                {
                    if (egg[j][i].bWidth == 32)
                    {
                        hIcon = CreateIconFromResourceEx(
                            (PBYTE)images[iImage].Ptr(),
                            images[iImage].Size(),
                            TRUE,
                            0x00030000,
                            32,
                            32,
                            LR_DEFAULTCOLOR);
                        f = TRUE;
                    }
                    iImage++;
                }
                if (!f)
                {
                    hIcon = CreateIconFromResourceEx(
                        (PBYTE)images[iImageOld].Ptr(),
                        images[iImageOld].Size(),
                        TRUE,
                        0x00030000,
                        32,
                        32,
                        LR_DEFAULTCOLOR);
                }
                g_icons.push_back(hIcon);
            }
            if (egg.size() == 0)
                MessageBox(g_hMainWnd, LoadStringDx(16), NULL, MB_ICONINFORMATION);
            SendMessage(hListBox, LB_SETCOUNT, egg.size(), 0);
            SetFileName(hWnd, szFileName);
            return TRUE;
        }
        else
        {
            MessageBox(g_hMainWnd, LoadStringDx(21), NULL, MB_ICONERROR);
        }
    }
    return FALSE;
}

void OnContextMenu(HWND hWnd, HWND hListBox, INT x, INT y)
{
    if (g_szExeFileName[0] != 0)
    {
        HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(2));
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        SetForegroundWindow(hWnd);
        TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
            x, y, 0, hWnd, NULL);
        PostMessage(hWnd, WM_NULL, 0, 0);
        DestroyMenu(hMenu);
    }
}

void OnSelChange(HWND hWnd, HWND hStatusBar, HWND hListBox)
{
    INT i, c;
    TCHAR sz[256];
    i = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
    c = SendMessage(hListBox, LB_GETCOUNT, 0, 0);
    if (i == LB_ERR)
        wsprintf(sz, LoadStringDx(17), c);
    else
        wsprintf(sz, LoadStringDx(18), c, i);
    SendMessage(hStatusBar, SB_SETTEXT, 255, (LPARAM)sz);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HWND hListBox;
    static HWND hStatusBar;
    MEASUREITEMSTRUCT *pmis;
    DRAWITEMSTRUCT *pdis;
    INT i;
    BOOL f;
    RECT rc, rc2;
    TCHAR szFileName[MAX_PATH];
    
    switch(uMsg)
    {
    case WM_CREATE:
        hListBox = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("LISTBOX"), TEXT(""),
            LBS_MULTICOLUMN | LBS_NODATA | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY |
            LBS_OWNERDRAWFIXED | WS_CHILD | WS_VISIBLE | WS_HSCROLL, 0, 0, 0, 0, hWnd, 
            (HMENU)1, g_hInstance, NULL);
        if (hListBox == NULL)
            return -1;
        hStatusBar = CreateStatusWindow(WS_VISIBLE | WS_CHILD, "", hWnd, 2);
        if (hStatusBar == NULL)
            return -1;
        SendMessage(hListBox, LB_SETCOLUMNWIDTH, 32 + 8, 0);
        DragAcceptFiles(hWnd, TRUE);
        if (__argc != 1)
        {
            DoOpen(hWnd, __argv[1], hListBox);
            OnSelChange(hWnd, hStatusBar, hListBox);
        }
        break;

    case WM_DROPFILES:
        DragQueryFile((HDROP)wParam, 0, szFileName, MAX_PATH);
        DoOpen(hWnd, szFileName, hListBox);
        OnSelChange(hWnd, hStatusBar, hListBox);
        break;

    case WM_SIZE:
        GetClientRect(hWnd, &rc);
        SendMessage(hStatusBar, WM_SIZE, 0, 0);
        GetWindowRect(hStatusBar, &rc2);
        MoveWindow(hListBox, rc.left, rc.top, rc.right - rc.left, 
            rc.bottom - rc.top - (rc2.bottom - rc2.top), TRUE);
        break;

    case WM_MEASUREITEM:
        pmis = (MEASUREITEMSTRUCT *)lParam;
        pmis->itemHeight = 32 + 8;
        return TRUE;

    case WM_CONTEXTMENU:
        if (lParam == -1)
        {
            RECT rc;
            i = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
            SendMessage(hListBox, LB_GETITEMRECT, i, (LPARAM)&rc);
            MapWindowPoints(hListBox, NULL, (POINT *)&rc, 2);
            OnContextMenu(hWnd, hListBox, rc.right, rc.bottom);
        }
        else
        {
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            ScreenToClient(hListBox, &pt);
            SendMessage(hListBox, WM_LBUTTONDOWN, 0, MAKELPARAM(pt.x, pt.y));
            OnContextMenu(hWnd, hListBox, LOWORD(lParam), HIWORD(lParam));
        }
        break;

    case WM_DRAWITEM:
        pdis = (DRAWITEMSTRUCT *)lParam;
        i = pdis->itemID;
        if (i != -1)
        {
            if (pdis->itemState & ODS_SELECTED)
            {
                FillRect(pdis->hDC, &pdis->rcItem, (HBRUSH)(COLOR_HIGHLIGHT + 1));
            }
            else if (pdis->itemState & ODS_DISABLED)
            {
                FillRect(pdis->hDC, &pdis->rcItem, (HBRUSH)(COLOR_3DLIGHT + 1));
            }
            else
            {
                FillRect(pdis->hDC, &pdis->rcItem, (HBRUSH)(COLOR_WINDOW + 1));
            }
            RECT rc = pdis->rcItem;
            DrawIconEx(pdis->hDC, rc.left + 4, rc.top + 4, g_icons[i], 32, 32, 
                0, NULL, DI_NORMAL);
            if (pdis->itemState & ODS_FOCUS)
            {
                DrawFocusRect(pdis->hDC, &pdis->rcItem);
            }
        }
        return TRUE;

    case WM_INITMENUPOPUP:
        f = (g_szExeFileName[0] != 0 && 
            SendMessage(hListBox, LB_GETCURSEL, 0, 0) != LB_ERR);
        EnableMenuItem((HMENU)wParam, ID_EXTRACT, f ? MF_ENABLED : MF_GRAYED);
        f = g_szExeFileName[0] != 0;
        EnableMenuItem((HMENU)wParam, ID_EXTRACTALL, f ? MF_ENABLED : MF_GRAYED);
        break;

    case WM_SETFOCUS:
        SetFocus(hListBox);
        break;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case 1:
            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                OnSelChange(hWnd, hStatusBar, hListBox);
            }
            break;

        case ID_OPEN:
            szFileName[0] = 0;
            if (GetExeFileName(szFileName))
            {
                DoOpen(hWnd, szFileName, hListBox);
                OnSelChange(hWnd, hStatusBar, hListBox);
            }
            break;

        case ID_EXIT:
            SendMessage(hWnd, WM_CLOSE, 0, 0);
            break;

        case ID_EXTRACT:
            if (g_szExeFileName[0] != 0)
            {
                i = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
                if (i == LB_ERR)
                {
                    MessageBox(hWnd, LoadStringDx(15), NULL, MB_ICONWARNING);
                }
                else
                {
                    ExtractIconFromExe(g_szExeFileName, i);
                }
            }
            break;

        case ID_EXTRACTALL:
            if (g_szExeFileName[0] != 0)
            {
                if (BrowseForFolderDx(g_szDir))
                {
                    HCURSOR hcurWait = LoadCursor(NULL, IDC_WAIT);
                    HCURSOR hcurOld = SetCursor(hcurWait);
                    ExtractIconFromExe(g_szExeFileName, -1);
                    SetCursor(hcurOld);
                }
            }
            break;
        }
        break;

    case WM_DESTROY:
        for(i = 0; i < (INT)g_icons.size(); i++)
            DestroyIcon(g_icons[i]);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

INT WINAPI WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPSTR       pszCmdLine,
    INT         nCmdShow)
{
    WNDCLASSEX wcx;
    BOOL f;
    MSG msg;
    
    LPTSTR pch;
    g_hInstance = hInstance;

    pch = g_szFilter1;
    LoadString(hInstance, 7, pch, 256);
    pch += lstrlen(pch) + 1;
    lstrcpy(pch, "*.dll;*.exe;*.ocx;*.scr;*.cpl;*.icl");
    pch += lstrlen(pch) + 1;
    LoadString(hInstance, 9, pch, 256);
    pch += lstrlen(pch) + 1;
    lstrcpy(pch, "*.*");
    pch += lstrlen(pch) + 1;
    *pch = 0;

    pch = g_szFilter2;
    LoadString(hInstance, 8, pch, 256);
    pch += lstrlen(pch) + 1;
    lstrcpy(pch, "*.ico");
    pch += lstrlen(pch) + 1;
    LoadString(hInstance, 9, pch, 256);
    lstrcpy(pch, "*.*");
    pch += lstrlen(pch) + 1;
    *pch = 0;

    wcx.cbSize          = sizeof(WNDCLASSEX);
    wcx.style           = 0;
    wcx.lpfnWndProc     = WindowProc;
    wcx.cbClsExtra      = 0;
    wcx.cbWndExtra      = 0;
    wcx.hInstance       = hInstance;
    wcx.hIcon           = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wcx.hCursor         = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground   = (HBRUSH)(COLOR_3DFACE);
    wcx.lpszMenuName    = MAKEINTRESOURCE(1);
    wcx.lpszClassName   = g_szWndClass;
    wcx.hIconSm         = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(1), 
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), 
        GetSystemMetrics(SM_CYSMICON), 0);
    if (!RegisterClassEx(&wcx))
        return 1;
    
    LoadString(hInstance, 14, g_szCaption, 256);
    g_hMainWnd = CreateWindow(g_szWndClass, g_szCaption, WS_OVERLAPPEDWINDOW |
        WS_CLIPCHILDREN, CW_USEDEFAULT, 0, 400, 350, NULL, NULL, 
        hInstance, NULL);
    if (g_hMainWnd == NULL)
        return 2;

    g_hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(1));

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    while((f = GetMessage(&msg, NULL, 0, 0)) != FALSE)
    {
        if (f == -1)
            return -1;
        if (TranslateAccelerator(g_hMainWnd, g_hAccel, &msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (INT)msg.wParam;
}
