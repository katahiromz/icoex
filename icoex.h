typedef struct tagICONDIRHEADER
{
    WORD            idReserved;   // Reserved
    WORD            idType;       // resource type (1 for icons)
    WORD            idCount;      // how many images?
} ICONDIRHEADER, FAR * LPICONDIRHEADER;

typedef struct tagICONDIRENTRY
{
    BYTE    bWidth;               // Width of the image
    BYTE    bHeight;              // Height of the image (times 2)
    BYTE    bColorCount;          // Number of colors in image (0 if >=8bpp)
    BYTE    bReserved;            // Reserved
    WORD    wPlanes;              // Color Planes
    WORD    wBitCount;            // Bits per pixel
    DWORD   dwBytesInRes;         // how many bytes in this resource?
    DWORD   dwImageOffset;        // where in the file is this image
} ICONDIRENTRY, FAR * LPICONDIRENTRY;


#pragma pack( push )
#pragma pack( 2 )
typedef struct tagMEMICONDIRHEADER
{
    WORD            idReserved;   // Reserved
    WORD            idType;       // resource type (1 for icons)
    WORD            idCount;      // how many images?
} MEMICONDIRHEADER, *PMEMICONDIRHEADER;

typedef struct tagMEMICONDIRENTRY
{
    BYTE    bWidth;               // Width of the image
    BYTE    bHeight;              // Height of the image (times 2)
    BYTE    bColorCount;          // Number of colors in image (0 if >=8bpp)
    BYTE    bReserved;            // Reserved
    WORD    wPlanes;              // Color Planes
    WORD    wBitCount;            // Bits per pixel
    DWORD   dwBytesInRes;         // how many bytes in this resource?
    WORD    nID;                  // the ID
} MEMICONDIRENTRY, *PMEMICONDIRENTRY;
#pragma pack( pop )
