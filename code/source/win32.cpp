// Copyright Seong Woo Lee. All Rights Reserved.

// Initialize
//
void os_init(OS_State *s)
{
    s->rcp_qpc_freq = (1.f / (f32)os_qpc_freq());

    InitWorkQueue(&s->work_queue, os_get_num_logical_cores());
}

// System Info.
//
u32 os_get_num_logical_cores(void) 
{
    SYSTEM_INFO sys_info = {};
    GetSystemInfo(&sys_info);
    DWORD core_count = sys_info.dwNumberOfProcessors;
    return core_count;
}

// Work Queue
//
void AddWork(Work_Queue *Queue, Work_Callback *Callback, void *Param) 
{
    // @Note: We have a single producer atm. We gonna have to switch to cmpxchg 
    //        if we want mpmc, ultimately.
    //
    u32 Index = Queue->IndexToWrite;
    u32 NewWriteIndex = (Queue->IndexToWrite + 1) % array_count(Queue->Works);

    assert(NewWriteIndex != Queue->IndexToRead);

    Queue->Works[Index].Callback = Callback;
    Queue->Works[Index].Param    = Param;
    _WriteBarrier();
    Queue->IndexToWrite = NewWriteIndex;
    Queue->CompletionGoal++;
    ReleaseSemaphore(Queue->Semaphore, 1, 0);
}

b32 DoWorkOrShouldSleep(Work_Queue *Queue) 
{
    b32 ShouldSleep = false;

    u32 OldIndex = Queue->IndexToRead;
    u32 NewIndex = (OldIndex + 1) % array_count(Queue->Works);

    if (OldIndex != Queue->IndexToWrite) 
    {
        u32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->IndexToRead, NewIndex, OldIndex);
        if (Index == OldIndex) 
        {
            Work work = Queue->Works[Index];
            work.Callback(work.Param);
            InterlockedIncrement(&Queue->CompletionCount);
        }
    }
    else 
    {
        ShouldSleep = true;
    }

    return ShouldSleep;
}

void CompleteAllWork(Work_Queue *Queue) 
{
    while (Queue->CompletionCount != Queue->CompletionGoal) 
    {
        DoWorkOrShouldSleep(Queue);
    }

    Queue->CompletionCount = 0;
    Queue->CompletionGoal  = 0;
}

DWORD WINAPI WorkerThreadProc(LPVOID Param) 
{
    Work_Queue *Queue = (Work_Queue *)Param;

    for (;;) 
    {
        if (DoWorkOrShouldSleep(Queue)) 
        {
            WaitForSingleObject(Queue->Semaphore, INFINITE);
        }
    }
}

void InitWorkQueue(Work_Queue *Queue, u32 CoreCount) 
{
    u32 ThreadCount = CoreCount - 1;
    memset(Queue, 0, sizeof(*Queue));
    Queue->Semaphore = CreateSemaphore(NULL, 0, ThreadCount, 0);

    for (u32 i = 0; i < ThreadCount; ++i) 
    {
        DWORD ThreadID;
        HANDLE ThreadHandle = CreateThread(NULL, 0, WorkerThreadProc, Queue, 0, &ThreadID);
        CloseHandle(ThreadHandle);
    }
}


// Timer
//
u64 os_qpc(void)
{
    LARGE_INTEGER i;
    QueryPerformanceCounter(&i);
    return i.QuadPart;
}

u64 os_qpc_freq(void)
{
    LARGE_INTEGER i;
    QueryPerformanceFrequency(&i);
    return i.QuadPart;
}


// Memory
//
void *os_page_alloc(u64 size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

void *os_heap_alloc(u64 size)
{
    HANDLE default_heap = GetProcessHeap();
    void *ptr = HeapAlloc(default_heap, HEAP_ZERO_MEMORY, size);
    return ptr;
}
