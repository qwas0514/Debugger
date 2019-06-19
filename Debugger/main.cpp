#include <Windows.h>
#include <iostream>
#include <iomanip>

using namespace std;

HANDLE g_hProcess(NULL), g_hThread(NULL);
DWORD g_pidProcess(NULL), g_pidThread(NULL);

void OnProcessCreated(const CREATE_PROCESS_DEBUG_INFO *pInfo) {
	cout << TEXT("CREATE_PROCESS_DEBUG_EVENT") << endl;

	CloseHandle(pInfo->hProcess);
	CloseHandle(pInfo->hThread);
	CloseHandle(pInfo->hFile);
}

void OnThreadCreated(const CREATE_THREAD_DEBUG_INFO *pInfo) {
	cout << TEXT("CREATE_THREAD_DEBUG_EVENT") << endl;
	CloseHandle(pInfo->hThread);
}

void OnProcessExited(const EXIT_PROCESS_DEBUG_INFO *pInfo) {
	cout << TEXT("EXIT_PROCESS_DEBUG_EVENT ") << pInfo->dwExitCode << endl;
}

void OnThreadExited(const EXIT_THREAD_DEBUG_INFO *pInfo) {
	cout << TEXT("EXIT_THREAD_DEBUG_INFO ") << pInfo->dwExitCode << endl;
}

void OnException(const EXCEPTION_DEBUG_INFO *pInfo) {
	cout << TEXT("An exception was occured at ") << pInfo->ExceptionRecord.ExceptionAddress << endl
	<< TEXT("Exception code: ") << hex << uppercase << setw(8)
	<< setfill('0') << pInfo->ExceptionRecord.ExceptionCode << dec << endl;
	
	if (pInfo->dwFirstChance == TRUE) {
		cout << TEXT("First chance.") << endl;
	} else {
		cout << TEXT("Second chance.") << endl;
	}

	if(ContinueDebugEvent(g_pidProcess, g_pidThread, DBG_EXCEPTION_NOT_HANDLED) == TRUE) {
        cout << "DBG_EXCEPTION_NOT_HANDLED" << endl;
    } else {
        cout << "DBG_EXCEPTION_NOT_HANDLED failed " << GetLastError() << endl;
    }
}

void OnOutputDebugString(const OUTPUT_DEBUG_STRING_INFO *pInfo) {

	BYTE * pBuffer = (BYTE*)malloc(pInfo->nDebugStringLength);

	SIZE_T bytesRead;

	ReadProcessMemory(
		g_hProcess,
		pInfo->lpDebugStringData,
		pBuffer,
		pInfo->nDebugStringLength,
		&bytesRead);

	int requireLen = MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED,
		(LPCSTR)pBuffer,
		pInfo->nDebugStringLength,
		NULL,
		0);

	wchar_t *pWideStr = (wchar_t*)malloc(requireLen * sizeof(wchar_t));

	MultiByteToWideChar(
		CP_ACP,
		MB_PRECOMPOSED,
		(LPCSTR)pBuffer,
		pInfo->nDebugStringLength,
		pWideStr,
		requireLen);

	cout << TEXT("Debuggee debug string: ") << pWideStr << endl;

	free(pWideStr);
	free(pBuffer);
}

void OnRipEvent(const RIP_INFO *pInfo) {
	cout << TEXT("RIP_EVENT") << endl;
}

void OnDllLoaded(const LOAD_DLL_DEBUG_INFO *pInfo) {
	cout << TEXT("LOAD_DLL_DEBUG_EVENT") << endl;
}

void OnDllUnloaded(const UNLOAD_DLL_DEBUG_INFO *pInfo) {
	cout << TEXT("UNLOAD_DLL_DEBUG_EVENT:") << pInfo->lpBaseOfDll << endl;
}

int main(int argc, char *argv[]) {
	STARTUPINFO si = {0};
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi = {0};

	if (argc < 2) {
		cout << "Usage: <Path>" << endl;
		return 1;
	}

	if (CreateProcess(
		TEXT(argv[1]),
		NULL,
		NULL,
		NULL,
		FALSE,
		DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&si,
		&pi) == FALSE) {
		cout << TEXT("CreateProcess() failed ") << GetLastError() << endl;
		return 2;
	}

	g_hProcess = pi.hProcess;
	g_pidProcess = pi.dwProcessId;
	g_hThread = pi.hThread;
	g_pidThread = pi.dwThreadId;


	BOOL waitEvent = TRUE;
	DEBUG_EVENT debugEvent;
	while (waitEvent == TRUE && WaitForDebugEvent(&debugEvent, INFINITE)) {
		switch (debugEvent.dwDebugEventCode) {
		case CREATE_PROCESS_DEBUG_EVENT:
			OnProcessCreated(&debugEvent.u.CreateProcessInfo);
			break;
		case CREATE_THREAD_DEBUG_EVENT:
			OnThreadCreated(&debugEvent.u.CreateThread);
			break;
		case EXCEPTION_DEBUG_EVENT:
			OnException(&debugEvent.u.Exception);
			break;
		case EXIT_PROCESS_DEBUG_EVENT:
			OnProcessExited(&debugEvent.u.ExitProcess);
			waitEvent = FALSE;
			break;
		case EXIT_THREAD_DEBUG_EVENT:
			OnThreadExited(&debugEvent.u.ExitThread);
			break;
		case LOAD_DLL_DEBUG_EVENT:
			OnDllLoaded(&debugEvent.u.LoadDll);
			break;
		case UNLOAD_DLL_DEBUG_EVENT:
			OnDllUnloaded(&debugEvent.u.UnloadDll);
			break;
		case OUTPUT_DEBUG_STRING_EVENT:
			OnOutputDebugString(&debugEvent.u.DebugString);
			break;
		case RIP_EVENT:
			OnRipEvent(&debugEvent.u.RipInfo);
			break;
		default:
			cout << TEXT("Unknown debug event.") << endl;
			break;
		}
		if (waitEvent == TRUE) {
			ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
		} else {
			break;
		}
	}

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return 0;
}