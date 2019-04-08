//-------------------------------------------------------------------------------------------------------    
// Copyright (C) Microsoft Corporation. All rights reserved.    
// Licensed under the MIT license. See LICENSE file in the project root for full license information.    
//-------------------------------------------------------------------------------------------------------    
//    
// Function pointers that always point to the underlying function implementation (at least so far as TraceFixup is    
// concerned) e.g. to avoid calling a fixed function during the implementation of another fixup function.    
#pragma once    
    
#include <cassert>    
#include <ntstatus.h>    
#include <windows.h>    
#include <winternl.h>    
    
// NOTE: Some enums/structs/functions are declared in winternl.h, but are incomplete. Namespace is to disambiguate    
namespace winternl    
{    
    // Enum definitions not present in winternl.h    
    typedef enum _FILE_INFORMATION_CLASS    
    {    
        FileDirectoryInformation = 1,    
        FileFullDirectoryInformation,    
        FileBothDirectoryInformation,    
        FileBasicInformation,    
        FileStandardInformation,    
        FileInternalInformation,    
        FileEaInformation,    
        FileAccessInformation,    
        FileNameInformation,    
        FileRenameInformation,    
        FileLinkInformation,    
        FileNamesInformation,    
        FileDispositionInformation,    
        FilePositionInformation,    
        FileFullEaInformation,    
        FileModeInformation,    
        FileAlignmentInformation,    
        FileAllInformation,    
        FileAllocationInformation,    
        FileEndOfFileInformation,    
        FileAlternateNameInformation,    
        FileStreamInformation,    
        FilePipeInformation,    
        FilePipeLocalInformation,    
        FilePipeRemoteInformation,    
        FileMailslotQueryInformation,    
        FileMailslotSetInformation,    
        FileCompressionInformation,    
        FileObjectIdInformation,    
        FileCompletionInformation,    
        FileMoveClusterInformation,    
        FileQuotaInformation,    
        FileReparsePointInformation,    
        FileNetworkOpenInformation,    
        FileAttributeTagInformation,    
        FileTrackingInformation,    
        FileIdBothDirectoryInformation,    
        FileIdFullDirectoryInformation,    
        FileValidDataLengthInformation,    
        FileShortNameInformation,    
        FileIoCompletionNotificationInformation,    
        FileIoStatusBlockRangeInformation,    
        FileIoPriorityHintInformation,    
        FileSfioReserveInformation,    
        FileSfioVolumeInformation,    
        FileHardLinkInformation,    
        FileProcessIdsUsingFileInformation,    
        FileNormalizedNameInformation,    
        FileNetworkPhysicalNameInformation,    
        FileIdGlobalTxDirectoryInformation,    
        FileIsRemoteDeviceInformation,    
        FileUnusedInformation,    
        FileNumaNodeInformation,    
        FileStandardLinkInformation,    
        FileRemoteProtocolInformation,    
        FileRenameInformationBypassAccessCheck,    
        FileLinkInformationBypassAccessCheck,    
        FileVolumeNameInformation,    
        FileIdInformation,    
        FileIdExtdDirectoryInformation,    
        FileReplaceCompletionInformation,    
        FileHardLinkFullIdInformation,    
        FileIdExtdBothDirectoryInformation,    
        FileDispositionInformationEx,    
        FileRenameInformationEx,    
        FileRenameInformationExBypassAccessCheck,    
        FileDesiredStorageClassInformation,    
        FileStatInformation,    
        FileMemoryPartitionInformation,    
        FileMaximumInformation,    
        FileStatLxInformation,    
        FileCaseSensitiveInformation    
    } FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;    
    
    typedef enum _KEY_INFORMATION_CLASS    
    {    
        KeyBasicInformation,    
        KeyNodeInformation,    
        KeyFullInformation,    
        KeyNameInformation,    
        KeyCachedInformation,    
        KeyFlagsInformation,    
        KeyVirtualizationInformation,    
        KeyHandleTagsInformation,    
        KeyTrustInformation,    
        KeyLayerInformation,    
        MaxKeyInfoClass    
    } KEY_INFORMATION_CLASS;    
    
    typedef enum _IO_PRIORITY_HINT    
    {    
        IoPriorityVeryLow,    
        IoPriorityLow,    
        IoPriorityNormal,    
        IoPriorityHigh,    
        IoPriorityCritical,    
        MaxIoPriorityTypes    
    } IO_PRIORITY_HINT;    
    
    typedef enum _KEY_VALUE_INFORMATION_CLASS    
    {    
        KeyValueBasicInformation,    
        KeyValueFullInformation,    
        KeyValuePartialInformation,    
        KeyValueFullInformationAlign64,    
        KeyValuePartialInformationAlign64,    
        KeyValueLayerInformation,    
        MaxKeyValueInfoClass    
    } KEY_VALUE_INFORMATION_CLASS;    
    
    // Struct definitions not present in winternl.h    
    typedef struct _FILE_DIRECTORY_INFORMATION    
    {    
        ULONG         NextEntryOffset;    
        ULONG         FileIndex;    
        LARGE_INTEGER CreationTime;    
        LARGE_INTEGER LastAccessTime;    
        LARGE_INTEGER LastWriteTime;    
        LARGE_INTEGER ChangeTime;    
        LARGE_INTEGER EndOfFile;    
        LARGE_INTEGER AllocationSize;    
        ULONG         FileAttributes;    
        ULONG         FileNameLength;    
        WCHAR         FileName[1];    
    } FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;    
    
    typedef struct _FILE_FULL_DIR_INFORMATION    
    {    
        ULONG         NextEntryOffset;    
        ULONG         FileIndex;    
        LARGE_INTEGER CreationTime;    
        LARGE_INTEGER LastAccessTime;    
        LARGE_INTEGER LastWriteTime;    
        LARGE_INTEGER ChangeTime;    
        LARGE_INTEGER EndOfFile;    
        LARGE_INTEGER AllocationSize;    
        ULONG         FileAttributes;    
        ULONG         FileNameLength;    
        ULONG         EaSize;    
        WCHAR         FileName[1];    
    } FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;    
    
    typedef struct _FILE_BOTH_DIR_INFORMATION    
    {    
        ULONG         NextEntryOffset;    
        ULONG         FileIndex;    
        LARGE_INTEGER CreationTime;    
        LARGE_INTEGER LastAccessTime;    
        LARGE_INTEGER LastWriteTime;    
        LARGE_INTEGER ChangeTime;    
        LARGE_INTEGER EndOfFile;    
        LARGE_INTEGER AllocationSize;    
        ULONG         FileAttributes;    
        ULONG         FileNameLength;    
        ULONG         EaSize;    
        CCHAR         ShortNameLength;    
        WCHAR         ShortName[12];    
        WCHAR         FileName[1];    
    } FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;    
    
    typedef struct _FILE_BASIC_INFORMATION    
    {    
        LARGE_INTEGER CreationTime;    
        LARGE_INTEGER LastAccessTime;    
        LARGE_INTEGER LastWriteTime;    
        LARGE_INTEGER ChangeTime;    
        ULONG         FileAttributes;    
    } FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;    
    
    typedef struct _FILE_STANDARD_INFORMATION    
    {    
        LARGE_INTEGER AllocationSize;    
        LARGE_INTEGER EndOfFile;    
        ULONG         NumberOfLinks;    
        BOOLEAN       DeletePending;    
        BOOLEAN       Directory;    
    } FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;    
    
    typedef struct _FILE_INTERNAL_INFORMATION    
    {    
        LARGE_INTEGER IndexNumber;    
    } FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;    
    
    typedef struct _FILE_EA_INFORMATION    
    {    
        ULONG EaSize;    
    } FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;    
    
    typedef struct _FILE_ACCESS_INFORMATION    
    {    
        ACCESS_MASK AccessFlags;    
    } FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;    
    
    typedef struct _FILE_NAME_INFORMATION    
    {    
        ULONG FileNameLength;    
        WCHAR FileName[1];    
    } FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;    
    
    typedef struct _FILE_RENAME_INFORMATION    
    {    
        union {    
            BOOLEAN ReplaceIfExists;    
            ULONG   Flags;    
        } DUMMYUNIONNAME;    
        BOOLEAN ReplaceIfExists;    
        HANDLE  RootDirectory;    
        ULONG   FileNameLength;    
        WCHAR   FileName[1];    
    } FILE_RENAME_INFORMATION, *PFILE_RENAME_INFORMATION;    
    
    typedef struct _FILE_LINK_INFORMATION    
    {    
        BOOLEAN ReplaceIfExists;    
        HANDLE  RootDirectory;    
        ULONG   FileNameLength;    
        WCHAR   FileName[1];    
    } FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;    
    
    typedef struct _FILE_NAMES_INFORMATION    
    {    
        ULONG NextEntryOffset;    
        ULONG FileIndex;    
        ULONG FileNameLength;    
        WCHAR FileName[1];    
    } FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;    
    
    typedef struct _FILE_DISPOSITION_INFORMATION    
    {    
        BOOLEAN DeleteFile;    
    } FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;    
    
    typedef struct _FILE_POSITION_INFORMATION    
    {    
        LARGE_INTEGER CurrentByteOffset;    
    } FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;    
    
    typedef struct _FILE_FULL_EA_INFORMATION    
    {    
        ULONG  NextEntryOffset;    
        UCHAR  Flags;    
        UCHAR  EaNameLength;    
        USHORT EaValueLength;    
        CHAR   EaName[1];    
    } FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;    
    
    typedef struct _FILE_MODE_INFORMATION    
    {    
        ULONG Mode;    
    } FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;    
    
    typedef struct _FILE_ALIGNMENT_INFORMATION    
    {    
        ULONG AlignmentRequirement;    
    } FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;    
    
    typedef struct _FILE_ALL_INFORMATION    
    {    
        FILE_BASIC_INFORMATION     BasicInformation;    
        FILE_STANDARD_INFORMATION  StandardInformation;    
        FILE_INTERNAL_INFORMATION  InternalInformation;    
        FILE_EA_INFORMATION        EaInformation;    
        FILE_ACCESS_INFORMATION    AccessInformation;    
        FILE_POSITION_INFORMATION  PositionInformation;    
        FILE_MODE_INFORMATION      ModeInformation;    
        FILE_ALIGNMENT_INFORMATION AlignmentInformation;    
        FILE_NAME_INFORMATION      NameInformation;    
    } FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;    
    
    typedef struct _FILE_ALLOCATION_INFORMATION    
    {    
        LARGE_INTEGER AllocationSize;    
    } FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;    
    
    typedef struct _FILE_END_OF_FILE_INFORMATION    
    {    
        LARGE_INTEGER EndOfFile;    
    } FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;    
    
    typedef struct _FILE_STREAM_INFORMATION    
    {    
        ULONG         NextEntryOffset;    
        ULONG         StreamNameLength;    
        LARGE_INTEGER StreamSize;    
        LARGE_INTEGER StreamAllocationSize;    
        WCHAR         StreamName[1];    
    } FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;    
    
    typedef struct _FILE_PIPE_INFORMATION    
    {    
        ULONG ReadMode;    
        ULONG CompletionMode;    
    } FILE_PIPE_INFORMATION, *PFILE_PIPE_INFORMATION;    
    
    typedef struct _FILE_PIPE_LOCAL_INFORMATION    
    {    
        ULONG NamedPipeType;    
        ULONG NamedPipeConfiguration;    
        ULONG MaximumInstances;    
        ULONG CurrentInstances;    
        ULONG InboundQuota;    
        ULONG ReadDataAvailable;    
        ULONG OutboundQuota;    
        ULONG WriteQuotaAvailable;    
        ULONG NamedPipeState;    
        ULONG NamedPipeEnd;    
    } FILE_PIPE_LOCAL_INFORMATION, *PFILE_PIPE_LOCAL_INFORMATION;    
    
    typedef struct _FILE_PIPE_REMOTE_INFORMATION    
    {    
        LARGE_INTEGER CollectDataTime;    
        ULONG         MaximumCollectionCount;    
    } FILE_PIPE_REMOTE_INFORMATION, *PFILE_PIPE_REMOTE_INFORMATION;    
    
    typedef struct _FILE_MAILSLOT_QUERY_INFORMATION    
    {    
        ULONG         MaximumMessageSize;    
        ULONG         MailslotQuota;    
        ULONG         NextMessageSize;    
        ULONG         MessagesAvailable;    
        LARGE_INTEGER ReadTimeout;    
    } FILE_MAILSLOT_QUERY_INFORMATION, *PFILE_MAILSLOT_QUERY_INFORMATION;    
    
    typedef struct _FILE_MAILSLOT_SET_INFORMATION    
    {    
        PLARGE_INTEGER ReadTimeout;    
    } FILE_MAILSLOT_SET_INFORMATION, *PFILE_MAILSLOT_SET_INFORMATION;    
    
    typedef struct _FILE_COMPRESSION_INFORMATION    
    {    
        LARGE_INTEGER CompressedFileSize;    
        USHORT        CompressionFormat;    
        UCHAR         CompressionUnitShift;    
        UCHAR         ChunkShift;    
        UCHAR         ClusterShift;    
        UCHAR         Reserved[3];    
    } FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;    
    
    typedef struct _FILE_OBJECTID_INFORMATION    
    {    
        LONGLONG FileReference;    
        UCHAR    ObjectId[16];    
        union {    
            struct {    
                UCHAR BirthVolumeId[16];    
                UCHAR BirthObjectId[16];    
                UCHAR DomainId[16];    
            } DUMMYSTRUCTNAME;    
            UCHAR ExtendedInfo[48];    
        } DUMMYUNIONNAME;    
    } FILE_OBJECTID_INFORMATION, *PFILE_OBJECTID_INFORMATION;    
    
    typedef struct _FILE_QUOTA_INFORMATION    
    {    
        ULONG         NextEntryOffset;    
        ULONG         SidLength;    
        LARGE_INTEGER ChangeTime;    
        LARGE_INTEGER QuotaUsed;    
        LARGE_INTEGER QuotaThreshold;    
        LARGE_INTEGER QuotaLimit;    
        SID           Sid;    
    } FILE_QUOTA_INFORMATION, *PFILE_QUOTA_INFORMATION;    
    
    typedef struct _FILE_REPARSE_POINT_INFORMATION    
    {    
        LONGLONG FileReference;    
        ULONG    Tag;    
    } FILE_REPARSE_POINT_INFORMATION, *PFILE_REPARSE_POINT_INFORMATION;    
    
    typedef struct _FILE_NETWORK_OPEN_INFORMATION    
    {    
        LARGE_INTEGER CreationTime;    
        LARGE_INTEGER LastAccessTime;    
        LARGE_INTEGER LastWriteTime;    
        LARGE_INTEGER ChangeTime;    
        LARGE_INTEGER AllocationSize;    
        LARGE_INTEGER EndOfFile;    
        ULONG         FileAttributes;    
    } FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;    
    
    typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION    
    {    
        ULONG FileAttributes;    
        ULONG ReparseTag;    
    } FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;    
    
    typedef struct _FILE_ID_BOTH_DIR_INFORMATION    
    {    
        ULONG         NextEntryOffset;    
        ULONG         FileIndex;    
        LARGE_INTEGER CreationTime;    
        LARGE_INTEGER LastAccessTime;    
        LARGE_INTEGER LastWriteTime;    
        LARGE_INTEGER ChangeTime;    
        LARGE_INTEGER EndOfFile;    
        LARGE_INTEGER AllocationSize;    
        ULONG         FileAttributes;    
        ULONG         FileNameLength;    
        ULONG         EaSize;    
        CCHAR         ShortNameLength;    
        WCHAR         ShortName[12];    
        LARGE_INTEGER FileId;    
        WCHAR         FileName[1];    
    } FILE_ID_BOTH_DIR_INFORMATION, *PFILE_ID_BOTH_DIR_INFORMATION;    
    
    typedef struct _FILE_ID_FULL_DIR_INFORMATION    
    {    
        ULONG         NextEntryOffset;    
        ULONG         FileIndex;    
        LARGE_INTEGER CreationTime;    
        LARGE_INTEGER LastAccessTime;    
        LARGE_INTEGER LastWriteTime;    
        LARGE_INTEGER ChangeTime;    
        LARGE_INTEGER EndOfFile;    
        LARGE_INTEGER AllocationSize;    
        ULONG         FileAttributes;    
        ULONG         FileNameLength;    
        ULONG         EaSize;    
        LARGE_INTEGER FileId;    
        WCHAR         FileName[1];    
    } FILE_ID_FULL_DIR_INFORMATION, *PFILE_ID_FULL_DIR_INFORMATION;    
    
    typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION    
    {    
        LARGE_INTEGER ValidDataLength;    
    } FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;    
    
    typedef struct _FILE_IO_PRIORITY_HINT_INFORMATION    
    {    
        IO_PRIORITY_HINT PriorityHint;    
    } FILE_IO_PRIORITY_HINT_INFORMATION, *PFILE_IO_PRIORITY_HINT_INFORMATION;    
    
    typedef struct _FILE_LINK_ENTRY_INFORMATION    
    {    
        ULONG    NextEntryOffset;    
        LONGLONG ParentFileId;    
        ULONG    FileNameLength;    
        WCHAR    FileName[1];    
    } FILE_LINK_ENTRY_INFORMATION, *PFILE_LINK_ENTRY_INFORMATION;    
    
    typedef struct _FILE_LINKS_INFORMATION    
    {    
        ULONG                       BytesNeeded;    
        ULONG                       EntriesReturned;    
        FILE_LINK_ENTRY_INFORMATION Entry;    
    } FILE_LINKS_INFORMATION, *PFILE_LINKS_INFORMATION;    
    
    typedef struct _FILE_NETWORK_PHYSICAL_NAME_INFORMATION    
    {    
        ULONG FileNameLength;    
        WCHAR FileName[1];    
    } FILE_NETWORK_PHYSICAL_NAME_INFORMATION, *PFILE_NETWORK_PHYSICAL_NAME_INFORMATION;    
    
    typedef struct _FILE_ID_GLOBAL_TX_DIR_INFORMATION    
    {    
        ULONG         NextEntryOffset;    
        ULONG         FileIndex;    
        LARGE_INTEGER CreationTime;    
        LARGE_INTEGER LastAccessTime;    
        LARGE_INTEGER LastWriteTime;    
        LARGE_INTEGER ChangeTime;    
        LARGE_INTEGER EndOfFile;    
        LARGE_INTEGER AllocationSize;    
        ULONG         FileAttributes;    
        ULONG         FileNameLength;    
        LARGE_INTEGER FileId;    
        GUID          LockingTransactionId;    
        ULONG         TxInfoFlags;    
        WCHAR         FileName[1];    
    } FILE_ID_GLOBAL_TX_DIR_INFORMATION, *PFILE_ID_GLOBAL_TX_DIR_INFORMATION;    
    
    typedef struct _FILE_COMPLETION_INFORMATION    
    {    
        HANDLE Port;    
        PVOID  Key;    
    } FILE_COMPLETION_INFORMATION, *PFILE_COMPLETION_INFORMATION;    
    
    typedef struct _FILE_DISPOSITION_INFORMATION_EX    
    {    
        ULONG Flags;    
    } FILE_DISPOSITION_INFORMATION_EX, *PFILE_DISPOSITION_INFORMATION_EX;    
    
    typedef struct _KEY_BASIC_INFORMATION    
    {    
        LARGE_INTEGER LastWriteTime;    
        ULONG         TitleIndex;    
        ULONG         NameLength;    
        WCHAR         Name[1];    
    } KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;    
    
    typedef struct _KEY_NODE_INFORMATION    
    {    
        LARGE_INTEGER LastWriteTime;    
        ULONG         TitleIndex;    
        ULONG         ClassOffset;    
        ULONG         ClassLength;    
        ULONG         NameLength;    
        WCHAR         Name[1];    
    } KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;    
    
    typedef struct _KEY_FULL_INFORMATION    
    {    
        LARGE_INTEGER LastWriteTime;    
        ULONG         TitleIndex;    
        ULONG         ClassOffset;    
        ULONG         ClassLength;    
        ULONG         SubKeys;    
        ULONG         MaxNameLen;    
        ULONG         MaxClassLen;    
        ULONG         Values;    
        ULONG         MaxValueNameLen;    
        ULONG         MaxValueDataLen;    
        WCHAR         Class[1];    
    } KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;    
    
    typedef struct _KEY_NAME_INFORMATION    
    {    
        ULONG NameLength;    
        WCHAR Name[1];    
    } KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;    
    
    typedef struct _KEY_CACHED_INFORMATION    
    {    
        LARGE_INTEGER LastWriteTime;    
        ULONG         TitleIndex;    
        ULONG         SubKeys;    
        ULONG         MaxNameLen;    
        ULONG         Values;    
        ULONG         MaxValueNameLen;    
        ULONG         MaxValueDataLen;    
        ULONG         NameLength;    
    } KEY_CACHED_INFORMATION, *PKEY_CACHED_INFORMATION;    
    
    typedef struct _KEY_VIRTUALIZATION_INFORMATION    
    {    
        ULONG VirtualizationCandidate;    
        ULONG VirtualizationEnabled;    
        ULONG VirtualTarget;    
        ULONG VirtualStore;    
        ULONG VirtualSource;    
        ULONG Reserved;    
    } KEY_VIRTUALIZATION_INFORMATION, *PKEY_VIRTUALIZATION_INFORMATION;    
    
    typedef struct _KEY_VALUE_BASIC_INFORMATION    
    {    
        ULONG TitleIndex;    
        ULONG Type;    
        ULONG NameLength;    
        WCHAR Name[1];    
    } KEY_VALUE_BASIC_INFORMATION, PKEY_VALUE_BASIC_INFORMATION;    
    
    typedef struct _KEY_VALUE_FULL_INFORMATION    
    {    
        ULONG TitleIndex;    
        ULONG Type;    
        ULONG DataOffset;    
        ULONG DataLength;    
        ULONG NameLength;    
        WCHAR Name[1];    
    } KEY_VALUE_FULL_INFORMATION, PKEY_VALUE_FULL_INFORMATION;    
    
    typedef struct _KEY_VALUE_PARTIAL_INFORMATION    
    {    
        ULONG TitleIndex;    
        ULONG Type;    
        ULONG DataLength;    
        UCHAR Data[1];    
    } KEY_VALUE_PARTIAL_INFORMATION, PKEY_VALUE_PARTIAL_INFORMATION;    
    
    // Function declarations not present in winternl.h    
    NTSTATUS __stdcall NtQueryInformationFile(    
        HANDLE FileHandle,    
        PIO_STATUS_BLOCK IoStatusBlock,    
        PVOID FileInformation,    
        ULONG Length,    
        FILE_INFORMATION_CLASS FileInformationClass);    
    
    NTSTATUS __stdcall NtQueryKey(    
        HANDLE KeyHandle,    
        KEY_INFORMATION_CLASS KeyInformationClass,    
        PVOID KeyInformation,    
        ULONG Length,    
        PULONG ResultLength);    
    
    NTSTATUS __stdcall NtQueryValueKey(    
        HANDLE KeyHandle,    
        PUNICODE_STRING ValueName,    
        KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,    
        PVOID KeyValueInformation,    
        ULONG Length,    
        PULONG ResultLength);    
}    
    
// NOTE: The functions in winternl.h are not included in any import lib and therefore must be manually loaded in    
template <typename Func>    
inline Func GetNtDllInternalFunction(const char* functionName)    
{    
    static auto mod = ::LoadLibraryW(L"ntdll.dll");    
    assert(mod);    
    
    // Ignore namespaces    
    for (auto ptr = functionName; *ptr; ++ptr)    
    {    
        if (*ptr == ':')    
        {    
            functionName = ptr + 1;    
        }    
    }    
    
    auto result = reinterpret_cast<Func>(::GetProcAddress(mod, functionName));    
    assert(result);    
    return result;    
}    
#define WINTERNL_FUNCTION(Name) (GetNtDllInternalFunction<decltype(&Name)>(#Name));    
namespace impl    
{    
    inline auto NtQueryKey = WINTERNL_FUNCTION(winternl::NtQueryKey);    
    inline auto NtQueryInformationFile = WINTERNL_FUNCTION(winternl::NtQueryInformationFile);    
    inline auto NtQueryValueKey = WINTERNL_FUNCTION(winternl::NtQueryValueKey);    
}
