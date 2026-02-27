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

struct Win32_State {
    Work_Queue work_queue;
    f32 rec_qpc_freq;
};

typedef BOOLEAN _RtlGenRandom_(PVOID RandomBuffer, ULONG RandomBufferLength);
static _RtlGenRandom_ *RtlGenRandom;

static void win32_init(Win32_State *ws);
static void win32_rand(void *out, u32 bytes);

u32 GetLogicalCoreCount(void);
void AddWork(Work_Queue *Queue, Work_Callback *Callback, void *Param);
b32 DoWorkOrShouldSleep(Work_Queue *Queue);
void CompleteAllWork(Work_Queue *Queue);
DWORD WINAPI WorkerThreadProc(LPVOID Param);
void InitWorkQueue(Work_Queue *Queue, u32 CoreCount);

u64 ReadOSTimer(void);
u64 GetOSTimerFrequency(void);
