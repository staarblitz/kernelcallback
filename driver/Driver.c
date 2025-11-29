#include <ntifs.h>

DRIVER_INITIALIZE KcDriverEntry;
EXTERN_C NTSTATUS KeUserModeCallback(
	IN ULONG ApiNumber,
	IN PVOID InputBuffer,
	IN ULONG InputLength,
	OUT PVOID* OutputBuffer,
	IN PULONG OutputLength
);

typedef struct _LOVELY_MESSAGE {
	UINT32 Constant;
	UINT32 Zero;
	UINT32 Constant2;
} LOVELY_MESSAGE, *PLOVELY_MESSAGE;

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
KcTheRealDeal(

)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PEPROCESS Process = NULL;
	KAPC_STATE ApcState = { 0 };
	PLOVELY_MESSAGE Request = NULL, Response = NULL;
	SIZE_T RegionSize = PAGE_SIZE;
	ULONG ResponseSize;

	DbgBreakPoint();

	Status = PsLookupProcessByProcessId((HANDLE)6804, &Process);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("Failed to lookup process by process id.\n");
		return Status;
	}

	// attach to the target process' address space so we can use KeUserModeCallback
	KeStackAttachProcess(Process, &ApcState);

	// alloc memory in user-mode process
	Status = ZwAllocateVirtualMemory(NtCurrentProcess(), (PVOID*)&Request, 0, &RegionSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("Failed to allocate virtual memory.\n");
		goto end;
	}

	Request->Constant = 0x2009;
	Request->Zero = 0;
	Request->Constant2 = 0x9002;

	Status = KeUserModeCallback(0x2009, Request, sizeof(LOVELY_MESSAGE), &Response, &ResponseSize);
	DbgPrint("KeUserModeCallback: %x\n", Status);

	DbgPrint("Received message with size: %ul", ResponseSize);
	DbgPrint("Zero must be constant: %ul", Response->Zero);

	ZwFreeVirtualMemory(NtCurrentProcess(), (PVOID*)&Request, &RegionSize, MEM_RELEASE);

end:
	KeUnstackDetachProcess(&ApcState);
	DbgPrint("Status: %x\n", Status);

	return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
KcDriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
)
{
	UNREFERENCED_PARAMETER(DriverObject);
	// do nothing lol
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
KcDriverEntry(
	_Inout_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	PAGED_CODE();

	DriverObject->DriverUnload = KcDriverUnload;

	DbgPrint("Kc driver initialized.\n");

	return KcTheRealDeal();
}