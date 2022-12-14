#include <efi.h>
#include <efilib.h>
#include <elf.h>


typedef unsigned long long size_t;


typedef struct {
    
    void * BaseAddress;
    size_t BufferSize;
    unsigned int Width;
    unsigned int Height;
    unsigned int PixelsPerScanline;
    
}Framebuffer;

#define PSF1_MAGIC0 0x36
#define PSF1_MAGIC1 0x04

typedef struct
{
    unsigned char magic[2];
    unsigned char mode;
    unsigned char charsize;
}
PSF1_HEADER;

typedef struct 
{
    PSF1_HEADER * psf1_Header;
    void* glyphBuffer;
}
PSF1_FONT;

Framebuffer frame_buffer;

PSF1_Font * load_psf1_font(EFI_FILE* Directory, CHAR16* Path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
    EFI_FILE* font = LoadFile(Directory, Path, ImageHandle, SystemTable);
    if(font == NULL) return NULL;
    PSF1_Header* fontHeader;
    SystemTable->BootServices->AllocatePool(EfiLoaderData, sizeof(PSF1_HEADER),(void**)&fontHeader);
    
    
}

Framebuffer* InitializeGOP()
{
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    EFI_STATUS status;
    
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gopGuid, NULL, (void**)&gop);
    if(EFI_ERROR(status))
    {
        Print(L"Unable to locate GOP\n\r");
        return NULL;
    }
    else
    {
        Print(L"GOP located\n\r");
    }
    
    frame_buffer.BaseAddress = (void *)gop->Mode->FrameBufferBase;
    frame_buffer.BufferSize = gop->Mode->FrameBufferSize;
    frame_buffer.Width = gop->Mode->Info->HorizontalResolution;
    frame_buffer.Height = gop->Mode->Info->VerticalResolution;
    frame_buffer.PixelsPerScanline = gop->Mode->Info->PixelsPerScanLine;
    
    
    return &frame_buffer;
}

EFI_FILE* LoadFile(EFI_FILE* Directory, CHAR16* Path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
    EFI_FILE* LoadedFile;
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
    SystemTable->BootServices->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;
    SystemTable->BootServices->HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&FileSystem);
    if(Directory == NULL)
    {
        FileSystem->OpenVolume(FileSystem, &Directory);
    }
    
    EFI_STATUS s = Directory->Open(Directory, &LoadedFile, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if(s != EFI_SUCCESS){
        
        return NULL;
        
    }
    
    return LoadedFile;
};

int pmemcmp(const void * aptr, const void * bptr, size_t n)
{
    
    const unsigned char*a = aptr, * b = bptr;
    for
    (
     size_t i = 0; i < n; i++
     
     )
    {
        
        if(
           a[i] < b[i]
           ) 
            
            return -1;
        
        else if(
                a[i] > b[i]
                )
            return 1;
        
    }
    return 0;
}

EFI_STATUS efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	InitializeLib(ImageHandle, SystemTable);
    Print(L"String test blag blah blah \n\r");
    
    
    EFI_FILE* Kernel = LoadFile(NULL, L"kernel.elf", ImageHandle, SystemTable);
    
    if(Kernel == NULL)
    {
        
        Print(L"Could not load kernel\n\r");
        
    }
    else
    {
        
        Print(L"Kernel loaded succesfully!\n\r");
    };
    
    Elf64_Ehdr header;
    {
        
        UINTN FileInfoSize;
        EFI_FILE_INFO* FileInfo;
        Kernel->GetInfo(Kernel, &gEfiFileInfoGuid, &FileInfoSize, NULL);
        
        SystemTable->BootServices->AllocatePool(EfiLoaderData, FileInfoSize,(void**)&FileInfo);
        Kernel->GetInfo(Kernel, &gEfiFileInfoGuid, &FileInfoSize, (void**)&FileInfo);
        UINTN size = sizeof(header);
        Kernel->Read(Kernel, &size, &header);
    }
    
    if
    (
     
     pmemcmp(&header.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
     header.e_ident[EI_CLASS] != ELFCLASS64 ||
     header.e_ident[EI_DATA] != ELFDATA2LSB ||
     header.e_type != ET_EXEC ||
     header.e_machine != EM_X86_64 ||
     header.e_version != EV_CURRENT
     )
	{
		Print(L"kernel format is bad\r\n");
	}
	else
	{
		Print(L"kernel header successfully verified\r\n");
	}
    
    Elf64_Phdr * phdrs;
    {
        
        Kernel->SetPosition(Kernel, header.e_phoff);
        UINTN size = header.e_phnum * header.e_phentsize;
        SystemTable->BootServices->AllocatePool(EfiLoaderData, size, (void **)&phdrs);
        Kernel->Read(Kernel, &size, phdrs);
    }
    
    for
    (
     Elf64_Phdr* phdr = phdrs;
     (char *)phdr < (char *) phdrs + header.e_phnum *
     header.e_phentsize;
     phdr = (Elf64_Phdr*)((char*)phdr + header.e_phentsize)
     
     )
    {
        switch(phdr->p_type)
        {
            case PT_LOAD:
            {
                int pages = (phdr->p_memsz + 0x1000 -1) / 0x1000;
                Elf64_Addr segment = phdr->p_vaddr;
                SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, pages, &segment);
                Kernel->SetPosition(Kernel, phdr->p_offset);
                UINTN size = phdr->p_filesz;
                Kernel->Read(Kernel, &size, (void*)segment);
                break;
            }
        }
        
    }
    
    Print(L"Kernel loaded\r\n");
    
    void (*KernelStart)(Framebuffer *) = ((__attribute__((sysv_abi)) void (*)(Framebuffer *) ) header.e_entry);
    
    Framebuffer * newBuffer = InitializeGOP();
    
    Print(L"Base: 0x%x\n\r   Size: 0x%x\n\r   Width: %d\n\r   Height:%d\n\r   PixelsPerScanline: %d\n\r   \n\r",newBuffer->BaseAddress,newBuffer->BufferSize,newBuffer->Width,newBuffer->Height,newBuffer->PixelsPerScanline);
    
    
    KernelStart(newBuffer);
    
    return EFI_SUCCESS; // Exit the UEFI application
}
