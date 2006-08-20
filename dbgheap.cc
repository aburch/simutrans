// DbgHeap   1999 by Matthias Withopf / c't 10/99
// Getestet mit Windows 95, Windows NT 4.0 (Intel+Alpha)

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL ErrDlg(char *s)
{
  char tmp[512];
  lstrcpy(tmp,"Heap-Fehler: ");
  lstrcat(tmp,s);
  lstrcat(tmp,"\nBreakpoint aktivieren?");
  return MessageBox(0,tmp,"DbgHeap",MB_YESNO) == IDYES;
}

BOOL             MemStatistics_CS_Initialized = FALSE;
CRITICAL_SECTION MemStatistics_CS;
unsigned long    _TotalMemMax   = 0;
unsigned long    _TotalMem      = 0;
unsigned long    _TotalMemBlock = 0;

static void Heap_Init(void)
{
  if (!MemStatistics_CS_Initialized)
    {
      InitializeCriticalSection(&MemStatistics_CS);
      MemStatistics_CS_Initialized = TRUE;
    }
}

typedef struct
  {
    void         *dhi_StartAddr;
    unsigned long dhi_Size;
    unsigned long dhi_Pages;
    unsigned long dhi_Magic;
  } TDbgHeapInternal,* PDbgHeapInternal;

static unsigned long PageSize     = 0;
const  unsigned long DbgHeapMagic = 0xAB01CD02;

static PDbgHeapInternal GetDbgHeapInternal(void *p)
{
  char *p1 = (char *)((char *)p - sizeof(TDbgHeapInternal));
  while (((unsigned long)p1) & (sizeof(int) - 1)) --p1;
  PDbgHeapInternal p2 = (PDbgHeapInternal)p1;     // Alignment!
  MEMORY_BASIC_INFORMATION I;
  VirtualQuery(p1,&I,sizeof I);
  if ((I.State != MEM_COMMIT) || (p2->dhi_Magic != DbgHeapMagic))
    return NULL;  // ungültiger Heap-Block!
  return p2;
}

static void *AllocDbgHeap(unsigned int Size,BOOL Clear)
{
  if (!PageSize)
    {
      SYSTEM_INFO SI;
      GetSystemInfo(&SI);
      PageSize = SI.dwPageSize;
    }
  unsigned long OrigSize = Size;
  const int InternalSize = (sizeof(int) - 1) + sizeof(TDbgHeapInternal);
  Size += InternalSize;
  int PageCount = (Size + PageSize - 1) / PageSize;   // auf Pagegröße runden
  LPVOID p = VirtualAlloc(NULL,(PageCount + 1) * PageSize,MEM_COMMIT,PAGE_READWRITE);
  if (p)
    {
      LPVOID OrigPtr = p;
      DWORD LastAccess;
      VirtualProtect((char *)p + PageCount * PageSize,PageSize,PAGE_NOACCESS,&LastAccess);
      memset(p,0xAA,PageCount * PageSize);
      unsigned long OfsInLastPage = Size % PageSize;
      if (OfsInLastPage)
        p = (char *)p + (PageSize - OfsInLastPage);    // Block am Seitenende positionieren
      char *p1 = (char *)p;                            // Alignment!
      while (((unsigned long)p1) & (sizeof(int) - 1)) ++p1;
      PDbgHeapInternal p2 = (PDbgHeapInternal)p1;
      p2->dhi_StartAddr = OrigPtr;
      p2->dhi_Size      = OrigSize;
      p2->dhi_Pages     = PageCount;
      p2->dhi_Magic     = DbgHeapMagic;
      p = (char *)p + InternalSize;
      if (Clear) memset(p,0,OrigSize);
    }
  return p;
}

static BOOL FreeDbgHeap(void *p)
{
  PDbgHeapInternal p1 = GetDbgHeapInternal(p);
  if (!p1) return FALSE;
  // ändere Status auf 'Reserved'
  if (!VirtualFree(p1->dhi_StartAddr,p1->dhi_Pages * PageSize,MEM_DECOMMIT))
    return FALSE;
  return TRUE;
}

extern "C" void * _cdecl malloc(unsigned int Size)
{
  Heap_Init();
  if (!Size) ++Size;
  void *p = AllocDbgHeap(Size,FALSE);
  if (!p) return NULL;
  EnterCriticalSection(&MemStatistics_CS);
  _TotalMem += Size;
  ++_TotalMemBlock;
  if (_TotalMemMax < _TotalMem) _TotalMemMax = _TotalMem;
  LeaveCriticalSection(&MemStatistics_CS);
  return p;
}

extern "C" void * _cdecl calloc(int ItemCount,unsigned int Size1)
{
  Heap_Init();
  unsigned long Size = ItemCount * Size1;
  void *p = AllocDbgHeap(Size,TRUE);
  EnterCriticalSection(&MemStatistics_CS);
  _TotalMem += Size;
  ++_TotalMemBlock;
  if (_TotalMemMax < _TotalMem) _TotalMemMax = _TotalMem;
  LeaveCriticalSection(&MemStatistics_CS);
  return p;
}

extern "C" void _cdecl free(void *p)
{
  if (!p)
    {
      if (ErrDlg("NULL-Zeiger in free()!"))
        DebugBreak();
      return;
    }
  unsigned long Size = 0;
  PDbgHeapInternal p1 = GetDbgHeapInternal(p);
  if (p1) Size = p1->dhi_Size;
  if (!FreeDbgHeap(p))
    {
      if (ErrDlg("Ungültiger Speicherblock in free()!"))
        DebugBreak();
    }
  else
    {
      EnterCriticalSection(&MemStatistics_CS);
      _TotalMem -= Size;
      --_TotalMemBlock;
      LeaveCriticalSection(&MemStatistics_CS);
    }
}

extern "C" void * _cdecl realloc(void *Block,unsigned int Size)
{
  Heap_Init();
  if (!Size && Block) { free(Block); return NULL; }
  if (!Block) return malloc(Size);
  unsigned long OldSize = 0;
  void *p = malloc(Size);
  PDbgHeapInternal p1 = GetDbgHeapInternal(Block);
  if (p1)
    OldSize = p1->dhi_Size;
  else
    if (ErrDlg("Ungültiger Speicherblock in realloc()!"))
      DebugBreak();
  if (p)
    {
      unsigned int S = OldSize;
      if (S > Size)
        S = Size;
      if (S) memcpy(p,Block,S);
    }
  free(Block);
  return p;
}

void * _cdecl operator new(unsigned int Size)
{
  return malloc(Size);
}

#if defined(__BORLANDC__)
void * _cdecl operator new[](unsigned int Size)
{
  return ::operator new(Size);
}
#endif

void _cdecl operator delete(void *p)
{
  free(p);
}

#if defined(__BORLANDC__)
void _cdecl operator delete[](void *p)
{
  ::operator delete(p);
}
#endif
