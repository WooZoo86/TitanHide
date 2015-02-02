#include "ntdll.h"
#include "log.h"
#include "misc.h"

unsigned char* Ntdll::FileData = 0;
ULONG Ntdll::FileSize = 0;

NTSTATUS Ntdll::Initialize()
{
	UNICODE_STRING FileName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	RtlInitUnicodeString(&FileName, L"\\SystemRoot\\system32\\ntdll.dll");
	InitializeObjectAttributes(&ObjectAttributes, &FileName,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL, NULL);

	if (KeGetCurrentIrql() != PASSIVE_LEVEL)
	{
#ifdef _DEBUG
		DbgPrint("[TITANHIDE] KeGetCurrentIrql != PASSIVE_LEVEL!\n");
#endif
		return STATUS_UNSUCCESSFUL;
	}

	HANDLE FileHandle;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS NtStatus = ZwCreateFile(&FileHandle,
		GENERIC_READ,
		&ObjectAttributes,
		&IoStatusBlock, NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL, 0);
	if (NT_SUCCESS(NtStatus))
	{
		FILE_STANDARD_INFORMATION StandardInformation = { 0 };
		NtStatus = ZwQueryInformationFile(FileHandle, &IoStatusBlock, &StandardInformation, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);
		if (NT_SUCCESS(NtStatus))
		{
			FileSize = StandardInformation.EndOfFile.LowPart;
			Log("[TITANHIDE] FileSize of ntdll.dll is %08X!\n", StandardInformation.EndOfFile.LowPart);
			FileData = (unsigned char*)RtlAllocateMemory(true, FileSize);
			
			LARGE_INTEGER ByteOffset;
			ByteOffset.LowPart = ByteOffset.HighPart = 0;
			NtStatus = ZwReadFile(FileHandle,
				NULL, NULL, NULL,
				&IoStatusBlock,
				FileData,
				FileSize,
				&ByteOffset, NULL);

			if (!NT_SUCCESS(NtStatus))
			{
				RtlFreeMemory(FileData);
				Log("[TITANHIDE] ZwReadFile failed with status %08X...\n", NtStatus);
			}
		}
		else
			Log("[TITANHIDE] ZwQueryInformationFile failed with status %08X...\n", NtStatus);
		ZwClose(FileHandle);
	}
	else
		Log("[TITANHIDE] ZwCreateFile failed with status %08X...\n", NtStatus);
	return NtStatus;
}

void Ntdll::Deinitialize()
{
	RtlFreeMemory(FileData);
}