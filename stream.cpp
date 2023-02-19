#include <windows.h>

#include <string>
#include <cstdlib>
#include <cstdio>
using namespace std;

#include "stream.h"

INT DataStream::Find(LPCVOID p, INT size, INT start)
{
    for(INT i = start; i <= m_size - size; i++)
    {
        if (memcmp(m_p + i, p, size) == 0)
            return i;
    }
    return -1;
}
