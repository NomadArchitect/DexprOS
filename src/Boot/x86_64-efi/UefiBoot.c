#include "DexprOS/Boot/x86_64-efi/EndBootPhase.h"
#include "DexprOS/Kernel/ErrorDisplay.h"
#include "DexprOS/Kernel/Memory/MemoryDef.h"
#include "DexprOS/Kernel/Memory/PhysicalMemStructsGen.h"
#include "DexprOS/Kernel/efi/InitialMemMapGenEfi.h"
#include "DexprOS/Kernel/x86_64/Interrupts.h"
#include "DexprOS/Kernel/x86_64/TaskStateSegment.h"
#include "DexprOS/Kernel/x86_64/IdtCreator.h"
#include "DexprOS/Kernel/x86_64/GdtSetup.h"
#include "DexprOS/Kernel/x86_64/SyscallHandler.h"
#include "DexprOS/Kernel/x86_64/CpuFeatures.h"
#include "DexprOS/Kernel/x86_64/MemoryProtectionCpuSetup.h"
#include "DexprOS/Kernel/x86_64/FloatingPointInit.h"
#include "DexprOS/Kernel/x86_64/PageMapSwitching.h"
#include "DexprOS/Kernel/x86_64/PagingSettings.h"
#include "DexprOS/Drivers/Graphics/CpuGraphicsDriver.h"
#include "DexprOS/Drivers/PICDriver.h"
#include "DexprOS/Drivers/PS2ControllerDriver.h"
#include "DexprOS/Drivers/Keyboard/USKeyboardLayout.h"
#include "DexprOS/Drivers/Keyboard/PS2KeyboardDriver.h"
#include "DexprOS/Shell.h"
#include "DexprOS/Kernel/kstdlib/string.h"

#include <efi.h>

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


/*inline static int DexprOS_CompareUEFIGUID(const EFI_GUID* pGuid0,
                                          const EFI_GUID* pGuid1)
{
    return pGuid0->Data1 == pGuid1->Data1 &&
           pGuid0->Data2 == pGuid1->Data2 &&
           pGuid0->Data3 == pGuid1->Data3 &&
           pGuid0->Data4[0] == pGuid1->Data4[0] &&
           pGuid0->Data4[1] == pGuid1->Data4[1] &&
           pGuid0->Data4[2] == pGuid1->Data4[2] &&
           pGuid0->Data4[3] == pGuid1->Data4[3] &&
           pGuid0->Data4[4] == pGuid1->Data4[4] &&
           pGuid0->Data4[5] == pGuid1->Data4[5] &&
           pGuid0->Data4[6] == pGuid1->Data4[6] &&
           pGuid0->Data4[7] == pGuid1->Data4[7];
}*/


static void testDisplayUint64Hex(uint64_t value);


static DexprOS_GraphicsDriver g_graphicsDriver = {0};

static EFI_SYSTEM_TABLE* g_pUefiSystemTable = NULL;

DexprOS_Shell g_shell;



static const char* MemTypeToString(DexprOS_PhysicalMemoryType type)
{
    switch (type)
    {
    case DEXPROS_PHYSICAL_MEMORY_TYPE_UNUSABLE:
        return "DEXPROS_PHYSICAL_MEMORY_TYPE_UNUSABLE";
    case DEXPROS_PHYSICAL_MEMORY_TYPE_FIRMWARE_RESERVED:
        return "DEXPROS_PHYSICAL_MEMORY_TYPE_FIRMWARE_RESERVED";
    case DEXPROS_PHYSICAL_MEMORY_TYPE_UNACCEPTED:
        return "DEXPROS_PHYSICAL_MEMORY_TYPE_UNACCEPTED";
    case DEXPROS_PHYSICAL_MEMORY_TYPE_USABLE:
        return "DEXPROS_PHYSICAL_MEMORY_TYPE_USABLE";
    case DEXPROS_PHYSICAL_MEMORY_TYPE_USABLE_PERSISTENT:
        return "DEXPROS_PHYSICAL_MEMORY_TYPE_PERSISTENT";
    case DEXPROS_PHYSICAL_MEMORY_TYPE_ACPI_RECLAIM:
        return "DEXPROS_PHYSICAL_MEMORY_TYPE_ACPI_RECLAIM";
    case DEXPROS_PHYSICAL_MEMORY_TYPE_ACPI_NVS:
        return "DEXPROS_PHYSICAL_MEMORY_TYPE_ACPI_NVS";
    case DEXPROS_PHYSICAL_MEMORY_TYPE_EFI_BOOT_SERVICES_CODE:
        return "DEXPROS_PHYSICAL_MEMORY_TYPE_EFI_BOOT_SERVICES_CODE";
    case DEXPROS_PHYSICAL_MEMORY_TYPE_EFI_BOOT_SERVICES_DATA:
        return "DEXPROS_PHYSICAL_MEMORY_TYPE_EFI_BOOT_SERVICES_DATA";
    case DEXPROS_PHYSICAL_MEMORY_TYPE_EFI_RUNTIME_SERVICES_CODE:
        return "DEXPROS_PHYSICAL_MEMORY_TYPE_EFI_RUNTIME_SERVICES_CODE";
    case DEXPROS_PHYSICAL_MEMORY_TYPE_EFI_RUNTIME_SERVICES_DATA:
        return "DEXPROS_PHYSICAL_MEMORY_TYPE_EFI_RUNTIME_SERVICES_DATA";
    case DEXPROS_PHYSICAL_MEMORY_TYPE_Max:
        return "DEXPROS_PHYSICAL_MEMORY_TYPE_Max";
    }
    return "DEXPROS_PHYSICAL_MEMORY_TYPE_Max";
}
/*static const char* InitMemUsageToString(DexprOS_InitialMemMapMappedUsage usage)
{
    switch (usage)
    {
    case DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_NONE:
        return "DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_NONE";
    case DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_MAPPED:
        return "DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_MAPPED";
    case DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_KERNEL_CODE:
        return "DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_KERNEL_CODE";
    case DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_KERNEL_DATA:
        return "DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_KERNEL_DATA";
    case DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_BOOT_CODE:
        return "DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_BOOT_CODE";
    case DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_BOOT_DATA:
        return "DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_BOOT_DATA";
    case DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_INITIAL_MEMMAP:
        return "DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_INITIAL_MEMMAP";
    }
    return "DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_NONE";
}*/


EFI_STATUS efi_main(EFI_HANDLE imageHandle, EFI_SYSTEM_TABLE* pSystemTable)
{
    EFI_STATUS status = 0;


    g_pUefiSystemTable = pSystemTable;


    pSystemTable->BootServices->SetWatchdogTimer(0, 0, 0, NULL);
    if (EFI_ERROR(status))
        return status;


    // Setup graphics

    EFI_GUID gopProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* pGop = NULL;
    status = pSystemTable->BootServices->LocateProtocol(&gopProtocolGuid, NULL, (VOID**)&pGop);
    if (EFI_ERROR(status))
        return status;

    UINT32 currentVideoModeIndex = 0;
    if (pGop->Mode != NULL)
        currentVideoModeIndex = pGop->Mode->Mode;
    
    status = pGop->SetMode(pGop, currentVideoModeIndex);
    if (EFI_ERROR(status))
        return status;


    unsigned horizontalResolution = pGop->Mode->Info->HorizontalResolution;
    unsigned verticalResolution = pGop->Mode->Info->VerticalResolution;

    if (DexprOS_InitCpuGraphicsDriver(&g_graphicsDriver,
                                      pGop,
                                      pSystemTable->BootServices) != DEXPROS_CPU_GRAPHICS_DRV_INIT_SUCCESS)
        return EFI_OUT_OF_RESOURCES;

    DexprOS_InitKernelErrorDisplay(&g_graphicsDriver,
                                   horizontalResolution);
    
    /*
    // Find the current FAT32 volume
    EFI_FILE_IO_INTERFACE* pVolumeIO = NULL;
    status = DexprOSBoot_FindBootloaderVolume(imageHandle,
                                              pSystemTable,
                                              &pVolumeIO);
    if (EFI_ERROR(status))
        return status;

    EFI_FILE_HANDLE volumeRoot = NULL;
    status = pVolumeIO->OpenVolume(pVolumeIO, &volumeRoot);
    if (EFI_ERROR(status))
        return status;


    CHAR16* testFileName = L"EFI\\BOOT\\TESTFILE.TXT";
    EFI_FILE_HANDLE testFileHandle = NULL;
    status = volumeRoot->Open(volumeRoot, &testFileHandle, testFileName, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status))
    {
        DexprOS_Puts("Could not load a test file.\n");
    }
    else
    {
        UINTN readBufferSize = 512;
        char readBuffer[513] = {0};

        status = testFileHandle->Read(testFileHandle,
                                      &readBufferSize,
                                      readBuffer);
        readBuffer[readBufferSize] = '\0';

        status = testFileHandle->Close(testFileHandle);
        if (EFI_ERROR(status))
            return status;


        DexprOS_Puts("File loading test, content of /EFI/BOOT/TESTFILE.TXT: ");
        DexprOS_Puts(readBuffer);
    }


    status = volumeRoot->Close(volumeRoot);
    if (EFI_ERROR(status))
        return status;
    */


    // Find the ACPI table
    /*const EFI_GUID acpiTableGuid = ACPI_20_TABLE_GUID;
    VOID* pAcpiTable = NULL;
    for (UINTN i = 0; i < pSystemTable->NumberOfTableEntries; ++i)
    {
        const EFI_GUID* pTableGuid = &pSystemTable->ConfigurationTable[i].VendorGuid;

        if (DexprOS_CompareUEFIGUID(pTableGuid, &acpiTableGuid))
        {
            pAcpiTable = pSystemTable->ConfigurationTable[i].VendorTable;
        }
    }
    // Make sure we have the ACPI table to read data from
    if (pAcpiTable == NULL)
        return EFI_OUT_OF_RESOURCES;*/

    // Claim memory ownership and exit boot services


    UINTN memoryMapSize = 0;
    UINTN memoryDescriptorSize = 0;
    UINT32 memoryDescriptorVersion = 0;

    UINTN memoryMapKey = 0;

    void* pMemoryMapBuffer = NULL;


    status = DexprOSBoot_EndBootPhase(imageHandle,
                                      pSystemTable,
                                      pGop->Mode->FrameBufferBase,
                                      pGop->Mode->FrameBufferSize,
                                      &pMemoryMapBuffer,
                                      &memoryMapSize,
                                      &memoryMapKey,
                                      &memoryDescriptorSize,
                                      &memoryDescriptorVersion);
    if (EFI_ERROR(status))
        return status;
    

    DexprOS_EnableMemoryProtectionCpuFeatures();
    // Fill global paging settings struct
    g_DexprOS_PagingSettings.pagingMode = DEXPROS_PAGING_MODE_4_LEVEL;
    if (DexprOS_CheckCpu5LevelPagingSupport() && DexprOS_Is5LevelPagingActive())
        g_DexprOS_PagingSettings.pagingMode = DEXPROS_PAGING_MODE_5_LEVEL;
    g_DexprOS_PagingSettings.noExecuteAvailable = DexprOS_CheckCpuHasNX();


    DexprOS_InitFloatingPointOperations();


    // Create (temporary) initial memory map

    DexprOS_InitialMemMap initialMemMap;
    if (!DexprOS_CreateInitialMemMapFromEfi(pMemoryMapBuffer,
                                            memoryMapSize,
                                            memoryDescriptorSize,
                                            memoryDescriptorVersion,
                                            &initialMemMap))
        return EFI_OUT_OF_RESOURCES;


    // Now conventional memory is identity mapped. Find a region big enough to
    // hold a physical memory map and tree in the OS's format.

    DexprOS_PhysMemStructsSizeData physMemReq = DexprOS_GetPhysicalMemStructsSizeData(&initialMemMap);


    DexprOS_VirtualMemoryAddress memStructsAddress = 0;
    bool rangeFound = false;
    for (size_t i = 0; i < initialMemMap.numEntries; ++i)
    {
        const DexprOS_InitialMemMapEntry* pEntry = &initialMemMap.pEntries[i];
        if (pEntry->memoryType == DEXPROS_PHYSICAL_MEMORY_TYPE_USABLE &&
            pEntry->usage == DEXPROS_INITIAL_MEMMAP_MAPPED_USAGE_MAPPED &&
            pEntry->numPhysicalPages * DEXPROS_PHYSICAL_PAGE_SIZE >= physMemReq.bufferSize)
        {
            memStructsAddress = pEntry->virtualAddress;
            rangeFound = true;
            break;
        }
    }

    if (!rangeFound)
        return EFI_OUT_OF_RESOURCES;


    void* pPhysicalMemData = (void*)memStructsAddress;
    memset(pPhysicalMemData, 0, physMemReq.bufferSize);

    
    DexprOS_PhysicalMemTree physicalMemTree;
    DexprOS_PhysicalMemMap physicalMemMap;
    if (!DexprOS_CreatePhysicalMemStructs(&physicalMemTree,
                                          &physicalMemMap,
                                          &initialMemMap,
                                          pPhysicalMemData,
                                          &physMemReq))
        return EFI_OUT_OF_RESOURCES;
    


    uint64_t tssAdresses[1];
    uint16_t tssSizes[1];
    DexprOS_TaskStateSegment taskStateSegments[1] = {0};
    DexprOS_SetupTaskStateSegments(taskStateSegments,
                                   tssAdresses,
                                   tssSizes,
                                   1);
    uint16_t kernelCodeSegmentOffset = 0;
    uint16_t userBaseSegmentOffset = 0;
    uint16_t tssSegmentOffsets[1];

    DexprOS_SetupGDT(tssAdresses, tssSizes, 1,
                     &kernelCodeSegmentOffset,
                     &userBaseSegmentOffset,
                     tssSegmentOffsets);

    DexprOS_SetupIDT(kernelCodeSegmentOffset);


    DexprOS_EnableSyscallExtension(kernelCodeSegmentOffset,
                                   userBaseSegmentOffset);


    DexprOS_InitialisePIC(32, 40);

    DexprOS_PS2ControllerInitResult ps2Result = DexprOS_InitialisePS2Controller();
    if (ps2Result.hasKeyboard)
        DexprOS_InitialisePS2KeyboardDriver(DexprOS_GetKeyboardLayout_US());


    DexprOS_CreateShell(&g_shell,
                        pSystemTable,
                        &g_graphicsDriver,
                        horizontalResolution,
                        verticalResolution);


    DexprOS_EnableInterrupts();


    DexprOS_ShellPuts(&g_shell, "Welcome to DexprOS!\n\n");
    DexprOS_ShellPuts(&g_shell, "DexprOS version: pre-release 0.1.2\n");
    DexprOS_ShellPuts(&g_shell, "If you don't know what to do, type \"help\" and hit Enter.\n\n");


    if (DexprOS_CheckCpu5LevelPagingSupport())
    {
        DexprOS_ShellPuts(&g_shell, "5-level paging supported!\n\n");
        if (DexprOS_Is5LevelPagingActive())
            DexprOS_ShellPuts(&g_shell, "5-level paging active!\n\n");
    }


    for (size_t i = 0; i < physicalMemMap.numEntries; ++i)
    {
        if (physicalMemMap.pEntries[i].memoryType != DEXPROS_PHYSICAL_MEMORY_TYPE_USABLE &&
            physicalMemMap.pEntries[i].memoryType != DEXPROS_PHYSICAL_MEMORY_TYPE_USABLE_PERSISTENT)
            continue;

        DexprOS_ShellPuts(&g_shell, "Memory range start: ");
        testDisplayUint64Hex(physicalMemMap.pEntries[i].physicalAddress);
        DexprOS_ShellPuts(&g_shell, ", size: ");
        testDisplayUint64Hex(physicalMemMap.pEntries[i].rangeSize);
        DexprOS_ShellPuts(&g_shell, ", type: ");
        DexprOS_ShellPuts(&g_shell, MemTypeToString(physicalMemMap.pEntries[i].memoryType));
        DexprOS_ShellPuts(&g_shell, ", flags: ");
        testDisplayUint64Hex(physicalMemMap.pEntries[i].flags);
        DexprOS_ShellPuts(&g_shell, "\n");
    }
    

    DexprOS_ShellPuts(&g_shell, "\nInitial memory map address: ");
    testDisplayUint64Hex((uint64_t)initialMemMap.pEntries);
    DexprOS_ShellPuts(&g_shell, "\nNum initial memory map entries: ");
    testDisplayUint64Hex((uint64_t)initialMemMap.numEntries);
    DexprOS_ShellPuts(&g_shell, "\nPhysical memory management structs address: ");
    testDisplayUint64Hex((uint64_t)pPhysicalMemData);
    DexprOS_ShellPuts(&g_shell, ", size: ");
    testDisplayUint64Hex((uint64_t)physMemReq.bufferSize);
    DexprOS_ShellPuts(&g_shell, "\nNum memory map range entries: ");
    testDisplayUint64Hex(physicalMemMap.numEntries);
    DexprOS_ShellPuts(&g_shell, ", size: ");
    testDisplayUint64Hex(physicalMemMap.numEntries * sizeof(DexprOS_PhysicalMemoryRange));


    DexprOS_ShellPuts(&g_shell, "\n\n");
    

    DexprOS_ShellActivatePrompt(&g_shell);


    while (1)
        __asm__ volatile("hlt");


    //DexprOS_DestroyShell(&g_shell);

    // Unload the graphics driver
    //DexprOS_DestroyCpuGraphicsDriver(&g_graphicsDriver);

    return EFI_SUCCESS;
}

/*
EFI_STATUS DexprOSBoot_FindBootloaderVolume(EFI_HANDLE imageHandle,
                                            EFI_SYSTEM_TABLE* pSystemTable,
                                            EFI_FILE_IO_INTERFACE** pVolumeIO)
{
    EFI_STATUS status = 0;


    EFI_GUID imageProtocolGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_GUID filesystemProtocolGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    EFI_LOADED_IMAGE* pImage = NULL;

    status = pSystemTable->BootServices->HandleProtocol(imageHandle,
                                                        &imageProtocolGuid,
                                                        (VOID**)&pImage);
    if (EFI_ERROR(status))
        return status;

    
    status = pSystemTable->BootServices->HandleProtocol(pImage->DeviceHandle,
                                                        &filesystemProtocolGuid,
                                                        (VOID**)pVolumeIO);
    if (EFI_ERROR(status))
        return status;
    

    return status;
}
*/

void testDisplayUint64Hex(uint64_t value)
{
    DexprOS_ShellPuts(&g_shell, "0x");

    for (int i = 0; i < 16; ++i)
    {
        uint8_t singleCharValue = ((value >> (60 - i * 4)) & 0xF);

        switch (singleCharValue)
        {
        case 0:
            DexprOS_ShellPutChar(&g_shell, '0');
            break;
        case 1:
            DexprOS_ShellPutChar(&g_shell, '1');
            break;
        case 2:
            DexprOS_ShellPutChar(&g_shell, '2');
            break;
        case 3:
            DexprOS_ShellPutChar(&g_shell, '3');
            break;
        case 4:
            DexprOS_ShellPutChar(&g_shell, '4');
            break;
        case 5:
            DexprOS_ShellPutChar(&g_shell, '5');
            break;
        case 6:
            DexprOS_ShellPutChar(&g_shell, '6');
            break;
        case 7:
            DexprOS_ShellPutChar(&g_shell, '7');
            break;
        case 8:
            DexprOS_ShellPutChar(&g_shell, '8');
            break;
        case 9:
            DexprOS_ShellPutChar(&g_shell, '9');
            break;
        case 10:
            DexprOS_ShellPutChar(&g_shell, 'A');
            break;
        case 11:
            DexprOS_ShellPutChar(&g_shell, 'B');
            break;
        case 12:
            DexprOS_ShellPutChar(&g_shell, 'C');
            break;
        case 13:
            DexprOS_ShellPutChar(&g_shell, 'D');
            break;
        case 14:
            DexprOS_ShellPutChar(&g_shell, 'E');
            break;
        case 15:
            DexprOS_ShellPutChar(&g_shell, 'F');
            break;
        
        default:
            break;
        }
    }
}


