#include <windows.h>
#include <stdlib.h>
#include <vector>
#include <new>
using namespace std;

#include "stream.h"
#include "ne.h"

#pragma pack(push)
#pragma pack(2)
typedef struct
{
    WORD offset;
    WORD length;
    WORD flags;
    WORD id;
    DWORD reserved;
} NE_NAMEINFO;

typedef struct
{
    WORD type_id;
    WORD count;
    DWORD reserved;
} NE_TYPEINFO;

typedef struct
{
    WORD idReserved;    // Reserved (must be 0)
    WORD idType;        // Resource type (1 for icons)
    WORD idCount;       // How many images?
} NE_GRPICONDIR;
#pragma pack(pop)

#define NE_RT_CURSOR 0x8001
#define NE_RT_BITMAP 0x8002
#define NE_RT_ICON 0x8003
#define NE_RT_MENU 0x8004
#define NE_RT_DIALOG 0x8005
#define NE_RT_STRING 0x8006
#define NE_RT_FONTDIR 0x8007
#define NE_RT_FONT 0x8008
#define NE_RT_ACCELERATOR 0x8009
#define NE_RT_RCDATA 0x800a
#define NE_RT_GROUP_CURSOR 0x800c
#define NE_RT_GROUP_ICON 0x800e

BOOL LoadNe(
    LPCSTR pszFileName, 
    vector<vector<NE_GRPICONDIRENTRY> >& egg,
    vector<DataStream>& images)
{
    HANDLE hFile;
    DWORD cbDone;
    hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    IMAGE_DOS_HEADER dos;
    IMAGE_OS2_HEADER os2;
    if (SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == 0xFFFFFFFF ||
        !ReadFile (hFile, &dos, sizeof(IMAGE_DOS_HEADER), &cbDone, NULL) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE ||
        SetFilePointer(hFile, dos.e_lfanew, NULL, FILE_BEGIN) == 0xFFFFFFFF ||
        !ReadFile(hFile, &os2, sizeof (IMAGE_OS2_HEADER), &cbDone, NULL) &&
        os2.ne_magic != IMAGE_OS2_SIGNATURE)
    {
        CloseHandle(hFile);
        return FALSE;
    }

    DWORD nRsrcTab;
    WORD nAlignShift;
    NE_TYPEINFO ti;
    NE_NAMEINFO ni;
    
    nRsrcTab = dos.e_lfanew + os2.ne_rsrctab;
    if (SetFilePointer(hFile, nRsrcTab, NULL, FILE_BEGIN) == 0xFFFFFFFF ||
        !ReadFile(hFile, &nAlignShift, sizeof(WORD), &cbDone, NULL))
    {
        CloseHandle(hFile);
        return FALSE;
    }

    NE_GRPICONDIR dir;
    NE_GRPICONDIRENTRY entry;
    vector<NE_GRPICONDIRENTRY> entries;

    DWORD nPos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);;
    if (!ReadFile(hFile, &ti, sizeof(NE_TYPEINFO), &cbDone, NULL))
    {
        CloseHandle(hFile);
        return FALSE;
    }
    
    egg.clear();
    while(ti.type_id != 0)
    {
        for (INT i = 0; i < ti.count; i++)
        {
            if (!ReadFile(hFile, &ni, sizeof(NE_NAMEINFO), &cbDone, NULL))
                return FALSE;
            if (ti.type_id == NE_RT_GROUP_ICON)
            {
                DWORD nPosOld = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
                DWORD nContent = ni.offset * (1 << nAlignShift);
                SetFilePointer(hFile, nContent, NULL, FILE_BEGIN);
                if (!ReadFile(hFile, &dir, sizeof(NE_GRPICONDIR), &cbDone, NULL))
                {
                    CloseHandle(hFile);
                    return FALSE;
                }
                entries.clear();
                for(INT k = 0; k < dir.idCount; k++)
                {
                    if (!ReadFile(hFile, &entry, sizeof(NE_GRPICONDIRENTRY), &cbDone, NULL))
                    {
                        CloseHandle(hFile);
                        return FALSE;
                    }
                    entries.push_back(entry);
                }
                egg.push_back(entries);
                SetFilePointer(hFile, nPosOld, NULL, FILE_BEGIN);
            }
        }
        if (!ReadFile(hFile, &ti, sizeof(NE_TYPEINFO), &cbDone, NULL))
        {
            CloseHandle(hFile);
            return FALSE;
        }
    }

    entries.clear();
    for(INT j = 0; j < egg.size(); j++)
    {
        for(INT i = 0; i < egg[j].size(); i++)
            entries.push_back(egg[j][i]);
    }
    
    SetFilePointer(hFile, nPos, NULL, FILE_BEGIN);;
    DataStream ds;
    if (!ReadFile(hFile, &ti, sizeof(NE_TYPEINFO), &cbDone, NULL))
    {
        CloseHandle(hFile);
        return FALSE;
    }
    INT iImage = 0;
    while(ti.type_id != 0)
    {
        for (INT i = 0; i < ti.count; i++)
        {
            if (!ReadFile(hFile, &ni, sizeof(NE_NAMEINFO), &cbDone, NULL))
                return FALSE;
            if (ti.type_id == NE_RT_ICON)
            {
                DWORD nPosOld = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);
                DWORD cb = entries[iImage++].dwBytesInRes;
                LPVOID p = malloc(cb);
                if (p == NULL)
                {
                    MessageBox(NULL, "Okk2", NULL, 0);
                    CloseHandle(hFile);
                    return FALSE;
                }
                DWORD nContent = ni.offset * (1 << nAlignShift);
                SetFilePointer(hFile, nContent, NULL, FILE_BEGIN);
                if (!ReadFile(hFile, p, cb, &cbDone, NULL))
                {
                    CloseHandle(hFile);
                    return FALSE;
                }
                ds.Append(p, cb);
                images.push_back(ds);
                ds.Clear();
                free(p);
                SetFilePointer(hFile, nPosOld, NULL, FILE_BEGIN);
            }
        }
        if (!ReadFile(hFile, &ti, sizeof(NE_TYPEINFO), &cbDone, NULL))
        {
            CloseHandle(hFile);
            return FALSE;
        }
    }
    CloseHandle(hFile);
    return TRUE;
}
