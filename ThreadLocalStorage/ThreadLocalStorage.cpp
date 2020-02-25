#include <windows.h> 
#include <stdio.h> 

#define THREADCOUNT 4 
DWORD dwTlsIndex;
__declspec(thread) DWORD pTlsBuff;

VOID ErrorExit(LPSTR);

VOID CommonFunc(VOID)
{
    LPVOID lpvData;

    // Retrieve a data pointer for the current thread. 

    lpvData = TlsGetValue(dwTlsIndex);
    if ((lpvData == 0) && (GetLastError() != ERROR_SUCCESS))
        ErrorExit((LPSTR)"TlsGetValue error");

    // Use the data stored for the current thread. 

    printf("common: thread %d: lpvData=%lx\n",
        GetCurrentThreadId(), lpvData);

    Sleep(5000);
}

DWORD WINAPI ThreadFunc(VOID)
{
    // wait thread message
    MSG msg;
    for (;;) {
        if (PeekMessage(&msg, NULL, WM_USER, WM_USER + 100, PM_NOREMOVE)) {
            printf("thread %d: got message from main thread msgid=%u lparam=%u wparam=%u\n",
                GetCurrentThreadId(),
                msg.message,
                msg.lParam,
                msg.wParam);
            if (msg.message == WM_USER + 1) {
                break;
            }
        }
    }

    LPVOID lpvData;

    // Initialize the TLS index for this thread. 

    lpvData = (LPVOID)LocalAlloc(LPTR, 256);
    if (!TlsSetValue(dwTlsIndex, lpvData))
        ErrorExit((LPSTR)"TlsSetValue error");

    printf("thread %d: lpvData=%lx\n", GetCurrentThreadId(), lpvData);
    printf("thread %d: pTlsBuf=%lu before set\n", GetCurrentThreadId(), pTlsBuff);
    if (!pTlsBuff) { pTlsBuff = (DWORD)malloc(1024); }
    printf("thread %d: pTlsBuf=%lu after initialize\n", GetCurrentThreadId(), pTlsBuff);

    CommonFunc();

    // Release the dynamic memory before the thread returns. 

    lpvData = TlsGetValue(dwTlsIndex);
    if (lpvData != 0)
        LocalFree((HLOCAL)lpvData);

    return 0;
}

int main(VOID)
{
    DWORD IDThread;
    HANDLE hThread[THREADCOUNT];
    int i;

    printf("Tls before set=%lu\n", pTlsBuff);
    pTlsBuff = (DWORD)malloc(1024);
    printf("Tls after set=%lu\n", pTlsBuff);

    // Allocate a TLS index. 

    if ((dwTlsIndex = TlsAlloc()) == TLS_OUT_OF_INDEXES)
        ErrorExit((LPSTR)"TlsAlloc failed");

    // Create multiple threads. 

    for (i = 0; i < THREADCOUNT; i++)
    {
        hThread[i] = CreateThread(NULL, // default security attributes 
            0,                           // use default stack size 
            (LPTHREAD_START_ROUTINE)ThreadFunc, // thread function 
            NULL,                    // no thread function argument 
            0,                       // use default creation flags 
            &IDThread);              // returns thread identifier 

      // Check the return value for success. 
        if (hThread[i] == NULL)
            ErrorExit((LPSTR)"CreateThread error\n");
    }

    // post thread message
    Sleep(2000);
    printf("blocking main thread is over, now start to post thread message\n");
    for (i = 0; i < THREADCOUNT; i++) {
        DWORD threadId = GetThreadId(hThread[i]);
        PostThreadMessage(threadId, WM_USER+1, GetCurrentThreadId(), i);
    }

    for (i = 0; i < THREADCOUNT; i++)
        WaitForSingleObject(hThread[i], INFINITE);

    TlsFree(dwTlsIndex);

    return 0;
}

VOID ErrorExit(LPSTR lpszMessage)
{
    fprintf(stderr, "%s\n", lpszMessage);
    ExitProcess(0);
}