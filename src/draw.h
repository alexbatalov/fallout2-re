#ifndef DRAW_H
#define DRAW_H

void bufferDrawLine(unsigned char* buf, int pitch, int left, int top, int right, int bottom, int color); 
void bufferDrawRect(unsigned char* buf, int a2, int a3, int a4, int a5, int a6, int a7);
void bufferDrawRectShadowed(unsigned char* buf, int a2, int a3, int a4, int a5, int a6, int a7, int a8);
void blitBufferToBufferStretch(unsigned char* src, int srcWidth, int srcHeight, int srcPitch, unsigned char* dest, int destWidth, int destHeight, int destPitch);
void blitBufferToBufferStretchTrans(unsigned char* src, int srcWidth, int srcHeight, int srcPitch, unsigned char* dest, int destWidth, int destHeight, int destPitch);
void blitBufferToBuffer(unsigned char* src, int width, int height, int srcPitch, unsigned char* dest, int destPitch);
void blitBufferToBufferTrans(unsigned char* src, int width, int height, int srcPitch, unsigned char* dest, int destPitch);
void bufferFill(unsigned char* buf, int width, int height, int pitch, int a5);
void sub_4D38E0(unsigned char* buf, int width, int height, int pitch, void* a5, int a6, int a7);
void sub_4D3A48(unsigned char* buf, int width, int height, int pitch);
void bufferOutline(unsigned char* buf, int width, int height, int pitch, int a5);

#endif /* DRAW_H */
