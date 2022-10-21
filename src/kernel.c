
typedef unsigned long long size_t;


typedef struct {
    
    void * BaseAddress;
    size_t BufferSize;
    unsigned int Width;
    unsigned int Height;
    unsigned int PixelsPerScanline;
    
}Framebuffer;



void _start(Framebuffer * frameBuffer){
    
    unsigned int y = 50;
    unsigned int BBP = 4;
    
    for
    (
     unsigned int x = 0; x  < frameBuffer->Width / 2 * BBP; x+=BBP
     
     )
    {
        *(unsigned int *)(x + (y * frameBuffer->PixelsPerScanline * BBP) + frameBuffer->BaseAddress) = 0xff00ffff;
        
        
    }
    
    
    return;
    
}