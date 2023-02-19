class DataStream
{
    LPSTR m_p;
    INT m_size;
public:
    DataStream() : m_p(NULL), m_size(0) {}
    DataStream(LPVOID p, INT size)
    {
        m_size = size;
        m_p = (LPSTR)malloc(size);
        if (m_p == NULL && size != 0) throw bad_alloc();
        CopyMemory(m_p, p, size);
    }
    DataStream(const DataStream& s)
    {
        m_size = s.m_size;
        m_p = (LPSTR)malloc(s.m_size);
        if (m_p == NULL && m_size != 0) throw bad_alloc();
        CopyMemory(m_p, s.m_p, s.m_size);
    }
    ~DataStream() { if (m_p != NULL) free(m_p); }

    LPSTR Ptr() { return m_p; }
    INT Size() { return m_size; }
    VOID Append(LPCVOID p, INT size)
    {
        m_p = (LPSTR)realloc(m_p, m_size + size);
        if (m_p == NULL && m_size + size != 0) throw bad_alloc();
        CopyMemory(m_p + m_size, p, size);
        m_size += size;
    }
    VOID Skip(INT size)
    {
        INT newsize = m_size - size;
        MoveMemory(m_p, m_p + size, newsize);
        m_p = (LPSTR)realloc(m_p, newsize);
        m_size = newsize;
    }
    INT Find(LPCVOID p, INT size, INT start = 0);
    VOID Clear(VOID) {
        if (m_p != NULL) free(m_p);
        m_p = NULL;
        m_size = 0;
    }
};
