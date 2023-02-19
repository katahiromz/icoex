#pragma pack(push)
#pragma pack(2)
typedef struct
{
    BYTE bWidth;        // Width, in pixels, of the image
    BYTE bHeight;       // Height, in pixels, of the image
    BYTE bColorCount;   // Number of colors in image (0 if> = 8bpp)
    BYTE bReserved;     // Reserved
    WORD wPlanes;       // Color Planes
    WORD wBitCount;     // Bits per pixel
    DWORD dwBytesInRes; // how many bytes in this resource?
    WORD nID;           // the ID
} NE_GRPICONDIRENTRY;
#pragma pack(pop)

BOOL LoadNe(
    LPCSTR pszFileName, 
    vector<vector<NE_GRPICONDIRENTRY> >& egg,
    vector<DataStream>& images);
