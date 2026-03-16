// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

typedef void Work_Callback(void *Param);

struct Work 
{
    Work_Callback *Callback;
    void *Param;
};

struct Work_Queue 
{
    Work            Works[1024];

    u32 volatile    IndexToWrite;
    u32 volatile    IndexToRead;

    u32 volatile    CompletionCount;
    u32 volatile    CompletionGoal;

    HANDLE          Semaphore;
};

struct OS_State {
    Work_Queue work_queue;
    f32 rcp_qpc_freq;
};


// Initialize
//
void os_init(OS_State *ws);

// System Info.
//
u32 os_get_num_logical_cores(void);

// Work Queue
//
void AddWork(Work_Queue *Queue, Work_Callback *Callback, void *Param);
b32 DoWorkOrShouldSleep(Work_Queue *Queue);
void CompleteAllWork(Work_Queue *Queue);
DWORD WINAPI WorkerThreadProc(LPVOID Param);
void InitWorkQueue(Work_Queue *Queue, u32 CoreCount);

// Timer
//
u64 os_qpc(void);
u64 os_qpc_freq(void);

// Memory
//
void *os_page_alloc(u64 size);
void *os_heap_alloc(u64 size);
