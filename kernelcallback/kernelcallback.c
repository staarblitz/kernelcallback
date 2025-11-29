#include <Windows.h>

EXTERN_C
NTSYSCALLAPI
NTSTATUS
NTAPI
ZwCallbackReturn(
	_In_reads_bytes_opt_(OutputLength) PVOID OutputBuffer,
	_In_ ULONG OutputLength,
	_In_ NTSTATUS Status
);

typedef struct _LOVELY_MESSAGE {
	UINT32 Constant;
	UINT32 Zero;
	UINT32 Constant2;
} LOVELY_MESSAGE, * PLOVELY_MESSAGE;

PUINT8 TargetFunction;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_COMMAND:
		if (LOWORD(wParam) == 1) {
			MessageBox(hwnd, L"Button clicked!", L"Notification", MB_OK);
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


LONG LovelyHandler(struct _EXCEPTION_POINTERS* info) {
	if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ILLEGAL_INSTRUCTION) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (info->ContextRecord->Rip == TargetFunction) {
		// great, we now can inspect registers to get our api number and do fancy stuff

		
		if (info->ContextRecord->Rcx == 0x2009) {
			// lovely!

			LOVELY_MESSAGE msg;
			msg.Zero = 0x2009;
			ZwCallbackReturn(&msg, sizeof(LOVELY_MESSAGE), 0);
		}


		// emulate normal behavior
		info->ContextRecord->Rcx = *(PUINT64)(info->ContextRecord->Rsp + 0x20); // mov rcx, dword ptr [rsp+20h]

		info->ContextRecord->Rip += 5; // advance the ptr to next instruction.

		RtlRestoreContext(info->ContextRecord, NULL); // restore the context, apply the info.

		return EXCEPTION_CONTINUE_EXECUTION;

	}


	return EXCEPTION_CONTINUE_SEARCH;
}


INT main() {
	// setup exception handler to catch our illegal instruction
	AddVectoredExceptionHandler(1, LovelyHandler);

	PUINT8 dispatchAddress = GetProcAddress(GetModuleHandle(L"ntdll"), "KiUserCallbackDispatcher");

	DWORD oldProtect = 0;
	if (!VirtualProtect((PVOID)((UINT64)dispatchAddress & ~(0xFFF) /*page aligning*/), 4096, PAGE_EXECUTE_READWRITE, &oldProtect)) {
		return GetLastError();
	}

	TargetFunction = dispatchAddress;

	// RSM. this is always guaranteed to throw illegal instruction exception.
	dispatchAddress[0] = 0x0F;
	dispatchAddress[1] = 0xAA;

	// purely for getting win32k messages lul

	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = L"WhyIsUsingAWindowSoHard";
	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		0,
		wc.lpszClassName,
		L"Window with Button",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		500, 300,
		NULL,
		NULL,
		wc.hInstance,
		NULL
	);

	if (hwnd == NULL) {
		return 0;
	}

	HWND hButton = CreateWindowEx(
		0,
		L"BUTTON",
		L"Click Me (or dont)",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		100, 100,
		100, 30,
		hwnd,
		(HMENU)1,
		wc.hInstance,
		NULL
	);

	if (hButton == NULL) {
		return 0;
	}

	ShowWindow(hwnd, SW_SHOWNORMAL);
	UpdateWindow(hwnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
