/*
 * QEMU 3Dfx Glide Pass-Through
 * Guest wrapper DLL - GLIDE3X.DLL
 *
 */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>

#include "fxlib.h"
#include "g2xfuncs.h"
#include "szgrdata.h"

#define INLINE inline
#define PT_CALL __stdcall
#define LOG_FNAME "C:\\WRAPPER.LOG"
//#define DEBUG_WRAPPER

#ifdef DEBUG_WRAPPER
static FILE *logfp = NULL;
#define DPRINTF(fmt, ...) \
    do {time_t curr = time(NULL); fprintf(logfp, "%s ", ctime(&curr)); \
	fprintf(logfp, "wrapper: " fmt , ## __VA_ARGS__); } while(0)
#else
#define DPRINTF(fmt, ...) 
#endif

#define GLIDEVER 0x301
#define GLIDEPT_MM_BASE 0xfbdff000
#define GLIDE_FIFO_BASE 0xfb400000
#define GLIDE_LFB_BASE  0xfb000000
#define PAGE_SIZE 0x1000
#define PUSH_F3DF 1
#define MAX_3DF 256*1024

typedef struct {
    uint32_t small;
    uint32_t large;
    uint32_t aspect;
    uint32_t format;
    void* data;
} wrTexInfo;

typedef struct {
    uint8_t header[SIZE_GU3DFHEADER];
    uint8_t table[SIZE_GUTEXTABLE];
    void *data;
    uint32_t mem_required;
} wr3dfInfo;

typedef struct { 
    int size; 
    void *lfbPtr; 
    uint32_t stride; 
    uint32_t writeMode; 
    uint32_t origin;
} wrLfbInfo;

static volatile uint32_t *ft = 0;
static volatile uint32_t *ptm = 0;
static volatile uint32_t *pt0 = 0;
static uint32_t *pt = 0;
static uint32_t *lfb = 0;
static uint32_t *m3df;
static uint32_t *mdata;
static uint32_t *mfifo;
static uint32_t *vgLfb;
static int InitGlidePTMMBase(void) { 
    //return (uint32_t *)GLIDEPT_MM_BASE; 
    FxU32 linear_addr, length;

    length = PAGE_SIZE;
    if (fxMapLinear(0, GLIDEPT_MM_BASE, &linear_addr, &length)) {
        ptm = (uint32_t *)linear_addr;
    }
    length = GRSHM_SIZE;
    if (fxMapLinear(0, GLIDE_FIFO_BASE, &linear_addr, &length)) {
        mfifo = (uint32_t *)linear_addr;
    }
    length = GRLFB_SIZE >> 1;
    if (fxMapLinear(0, GLIDE_LFB_BASE, &linear_addr, &length)) {
        lfb = (uint32_t *)linear_addr;
    }
    length = GRLFB_SIZE >> 1;
    if (fxMapLinear(0, (GLIDE_LFB_BASE + (GRLFB_SIZE >> 1)), &linear_addr, &length)) {
        vgLfb = (uint32_t *)linear_addr;
    }

    if (ptm == 0)
        return 1;
    mfifo[0] = FIRST_FIFO;
    mdata = &mfifo[MAX_FIFO];
    mdata[0] = ALIGNED(1) >> 2;
    m3df = &mfifo[(GRSHM_SIZE - MAX_3DF) >> 2];
    ft = ptm + (0xfb0U >> 2);
    pt = &mfifo[1];
    pt[0] = (uint32_t)(ptm + (0xfc0U >> 2));

    return 0;
}

static INLINE void forcedPageIn(const uint32_t addr, const uint32_t size, const char *func)
{
    (void *)func;
    int i;
    uint32_t *start = (uint32_t *)(addr & ~(PAGE_SIZE - 1));
    uint32_t cnt = (((addr + size) & ~(PAGE_SIZE - 1)) - (addr & ~(PAGE_SIZE - 1))) / PAGE_SIZE;

    //DPRINTF("%s forced paging addr 0x%08x len 0x%lx\n", func, addr, size);
    for (i = 0; i < cnt; i++) {
	*(volatile uint32_t *)start;
	start += (PAGE_SIZE >> 2);
    }
    *(volatile uint32_t *)start;
}

static INLINE void fifoAddEntry(uint32_t *pt, int FEnum, int numArgs)
{
    int i, j;

    j = mfifo[0];
    mfifo[j++] = FEnum;
    for (i = 0; i < numArgs; i++)
        mfifo[j++] = pt[i];
    mfifo[0] = j;
}

static INLINE void fifoAddData(int nArg, uint32_t argData, int cbData)
{
    int i, j;
    uint32_t *data = (uint32_t *)argData;
    uint32_t numData = (cbData & 0x03)? ((cbData >> 2) + 1):(cbData >> 2);

    j = mdata[0];
    for (i = 0; i < numData; i++)
        mdata[j++] = data[i];
    mdata[0] = j;
    if (nArg)
        pt[nArg] = argData;
}

static INLINE void fifoOutData(int offs, uint32_t darg, int cbData)
{
    int i;
    uint32_t *src = &mfifo[(GRSHM_SIZE - PAGE_SIZE + offs) >> 2];
    uint32_t *dst = (uint32_t *)darg;
    uint32_t numData = (cbData & 0x03)? ((cbData >> 2) + 1):(cbData >> 2);

    for (i = 0; i < numData; i++)
        dst[i] = src[i];
}

uint32_t PT_CALL grTexTextureMemRequired(uint32_t arg0, uint32_t arg1);
static int grGlidePresent = 0;
static int grGlideWnd = 0;
uint32_t PT_CALL grSstWinClose(uint32_t arg0);
char *basename(const char *name);

#define FIFO_GRFUNC(_func,_nargs) \
    if (((mfifo[0] + (_nargs + 1)) < MAX_FIFO) && (mdata[0] < MAX_DATA))  \
        fifoAddEntry(&pt[1], _func, _nargs); \
    else *pt0 = _func \


/* Start - generated by wrapper_genfuncs */

void PT_CALL grAADrawTriangle(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    fifoAddData(1, arg0, ALIGNED(SIZE_GRVERTEX)); fifoAddData(2, arg1, ALIGNED(SIZE_GRVERTEX)); fifoAddData(3, arg2, ALIGNED(SIZE_GRVERTEX)); 
    pt[4] = arg3; pt[5] = arg4; pt[6] = arg5; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grAADrawTriangle, 6);
}
void PT_CALL grAlphaBlendFunction(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grAlphaBlendFunction, 4);
}
void PT_CALL grAlphaCombine(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; pt[5] = arg4; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grAlphaCombine, 5);
}
void PT_CALL grAlphaControlsITRGBLighting(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grAlphaControlsITRGBLighting, 1);
}
void PT_CALL grAlphaTestFunction(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grAlphaTestFunction, 1);
}
void PT_CALL grAlphaTestReferenceValue(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grAlphaTestReferenceValue, 1);
}
void PT_CALL grBufferClear(uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grBufferClear;
}
void PT_CALL grBufferSwap(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grBufferSwap;
}
void PT_CALL grCheckForRoom(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grCheckForRoom;
}
void PT_CALL grChromakeyMode(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grChromakeyMode, 1);
}
void PT_CALL grChromakeyValue(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grChromakeyValue, 1);
}
void PT_CALL grClipWindow(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grClipWindow, 4);
}
void PT_CALL grColorCombine(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; pt[5] = arg4; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grColorCombine, 5);
}
void PT_CALL grColorMask(uint32_t arg0, uint32_t arg1) {
    pt[1] = arg0; pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grColorMask, 2);
}
void PT_CALL grConstantColorValue(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grConstantColorValue, 1);
}
void PT_CALL grCullMode(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grCullMode, 1);
}
void PT_CALL grDepthBiasLevel(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grDepthBiasLevel, 1);
}
void PT_CALL grDepthBufferFunction(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grDepthBufferFunction, 1);
}
void PT_CALL grDepthBufferMode(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grDepthBufferMode, 1);
}
void PT_CALL grDepthMask(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grDepthMask, 1);
}
void PT_CALL grDisableAllEffects(void) {
    
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grDisableAllEffects;
}
void PT_CALL grDitherMode(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grDitherMode, 1);
}
void PT_CALL grDrawLine(uint32_t arg0, uint32_t arg1) {
    fifoAddData(1, arg0, ALIGNED(SIZE_GRVERTEX)); fifoAddData(2, arg1, ALIGNED(SIZE_GRVERTEX)); 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grDrawLine, 2);
}
void PT_CALL grDrawPoint(uint32_t arg0) {
    fifoAddData(1, arg0, ALIGNED(SIZE_GRVERTEX)); 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grDrawPoint, 1);
}
void PT_CALL grDrawTriangle(uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    fifoAddData(1, arg0, ALIGNED(SIZE_GRVERTEX)); fifoAddData(2, arg1, ALIGNED(SIZE_GRVERTEX)); fifoAddData(3, arg2, ALIGNED(SIZE_GRVERTEX)); 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grDrawTriangle, 3);
}
void PT_CALL grErrorSetCallback(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grErrorSetCallback;
}
void PT_CALL grFogColorValue(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grFogColorValue, 1);
}
void PT_CALL grFogMode(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grFogMode, 1);
}
void PT_CALL grFogTable(uint32_t arg0) {
    fifoAddData(0, arg0, sizeof(uint8_t[64]));
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grFogTable, 1);
}
void PT_CALL grGlideGetState(uint32_t arg0) {
  /*  pt[1] = arg0;  */
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grGlideGetState;
}
void PT_CALL grGlideInit(void) {

    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grGlideInit;
    grGlidePresent = 1;
}
void PT_CALL grGlideSetState(uint32_t arg0) {
  /*  pt[1] = arg0;  */
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grGlideSetState;
}
void PT_CALL grGlideShutdown(void) {
    grSstWinClose(grGlideWnd); 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grGlideShutdown;
    grGlidePresent = 0;
}
void PT_CALL grLfbConstantAlpha(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grLfbConstantAlpha, 1);
}
void PT_CALL grLfbConstantDepth(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grLfbConstantDepth, 1);
}
uint32_t PT_CALL grLfbLock(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5) {
    int ret;
    uint32_t shmaddr;
    fifoAddData(0, arg5, ALIGNED(SIZE_GRLFBINFO));
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; pt[5] = arg4; pt[6] = arg5; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grLfbLock;
    ret = *pt0;
    fifoOutData(0, arg5, SIZE_GRLFBINFO);
    if (ret & 0x10U) {
        shmaddr = (uint32_t)vgLfb + (uint32_t)(((wrLfbInfo *)arg5)->lfbPtr);
        ((wrLfbInfo *)arg5)->lfbPtr = (uint32_t *)shmaddr;
        ((wrLfbInfo *)arg5)->stride = 0x800;
        ((wrLfbInfo *)arg5)->writeMode = arg2;
        ((wrLfbInfo *)arg5)->origin = arg3;
        ret = 1;
    }
    return ret;
}
uint32_t PT_CALL grLfbReadRegion(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6) {
    int ret;
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; pt[5] = arg4; pt[6] = arg5; pt[7] = arg6; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grLfbReadRegion;
    ret = *pt0;
    if (ret)
        memcpy((uint8_t*)arg6, &mdata[ALIGNED(1) >> 2], (arg4 * arg5));
    return ret;
}
uint32_t PT_CALL grLfbUnlock(uint32_t arg0, uint32_t arg1) {
    pt[1] = arg0; pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grLfbUnlock;
    return *pt0;
}
void PT_CALL grLfbWriteColorFormat(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grLfbWriteColorFormat;
}
void PT_CALL grLfbWriteColorSwizzle(uint32_t arg0, uint32_t arg1) {
    pt[1] = arg0; pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grLfbWriteColorSwizzle;
}
uint32_t PT_CALL grLfbWriteRegion(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6, uint32_t arg7, uint32_t arg8) {
    int ret;
    fifoAddData(0, arg8, (arg5 * arg7));
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; pt[5] = arg4; pt[6] = arg5; pt[7] = arg6; pt[8] = arg7; pt[9] = arg8; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grLfbWriteRegion3x;
    ret = *pt0;
    return ret;
}
void PT_CALL grRenderBuffer(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grRenderBuffer;
}
void PT_CALL grSplash(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; pt[5] = arg4; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grSplash;
}
void PT_CALL grSstConfigPipeline(uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grSstConfigPipeline;
}
void PT_CALL grSstOrigin(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grSstOrigin;
}
void PT_CALL grSstSelect(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grSstSelect;
}
void PT_CALL grSstVidMode(uint32_t arg0, uint32_t arg1) {
    pt[1] = arg0; pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grSstVidMode;
}
uint32_t PT_CALL grSstWinClose(uint32_t arg0) {
    uint32_t ret, wait = 1;
    if (!grGlideWnd)
	return 1;
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grSstWinClose3x;
    ret = *pt0;
    grGlideWnd = 0;
    while (ret && wait)
        wait = ptm[0xfb8U >> 2];
    return ret;
}
uint32_t PT_CALL grSstWinOpen(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6) {
    uint32_t ret, wait = 1;
    if (grGlideWnd)
	grSstWinClose(grGlideWnd);
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; pt[5] = arg4; pt[6] = arg5; pt[7] = arg6;
    pt[8] = (uint32_t)lfb;
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grSstWinOpen;
    ret = *pt0;
    while (ret && wait)
        wait = ptm[0xfb8U >> 2];
    grGlideWnd = ret;
    return ret;
}
uint32_t PT_CALL grTexCalcMemRequired(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grTexCalcMemRequired;
    return *pt0;
}
void PT_CALL grTexClampMode(uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexClampMode, 3);
}
void PT_CALL grTexCombine(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; pt[5] = arg4; pt[6] = arg5; pt[7] = arg6; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexCombine, 7);
}
void PT_CALL grTexDetailControl(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexDetailControl, 4);
}
void PT_CALL grTexDownloadMipMap(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    wrTexInfo info;
    uint32_t addr = (uint32_t)((wrTexInfo *)arg3)->data, dlen;

    info.small = ((wrTexInfo *)arg3)->small;
    info.large = ((wrTexInfo *)arg3)->large;
    info.aspect = ((wrTexInfo *)arg3)->aspect;
    info.format = ((wrTexInfo *)arg3)->format;
    dlen =  grTexTextureMemRequired(arg2, (uint32_t)&info);

    fifoAddData(0, arg3, ALIGNED(SIZE_GRTEXINFO));
    fifoAddData(0, addr, ALIGNED(dlen));

    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; pt[5] = dlen;
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexDownloadMipMap, 5);
}
void PT_CALL grTexDownloadMipMapLevel(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6, uint32_t arg7) {
    wrTexInfo info;
    uint32_t dlen;

    info.small = arg2;
    info.large = arg2;
    info.aspect = arg4;
    info.format = arg5;
    dlen =  grTexTextureMemRequired(arg6, (uint32_t)&info);

    fifoAddData(0, arg7, ALIGNED(dlen));

    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; pt[5] = arg4; pt[6] = arg5; pt[7] = arg6; pt[8] = arg7; pt[9] = dlen;
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexDownloadMipMapLevel, 9);
}
void PT_CALL grTexDownloadMipMapLevelPartial(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5, uint32_t arg6, uint32_t arg7, uint32_t arg8, uint32_t arg9) {
    int texBytes, thisLOD, aspect;
    
    thisLOD = G3_LOD_TRANSLATE(arg2);
    aspect = G3_ASPECT_TRANSLATE(arg4);
    texBytes = 256 >> thisLOD;
    if (aspect & 0x04)
        texBytes >>= ((aspect & 0x03) + 1);
    texBytes >>= (arg5 < 0x08)? 2:1;
    if (texBytes <= 0)
        texBytes = 1;
    texBytes = texBytes * (arg9 - arg8 + 1) * 4;

    fifoAddData(0, arg7, ALIGNED(texBytes));

    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; pt[5] = arg4; pt[6] = arg5; pt[7] = arg6; pt[8] = arg7; pt[9] = arg8; pt[10] = arg9; pt[11] = texBytes; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexDownloadMipMapLevelPartial, 11);
}
void PT_CALL grTexDownloadTable(uint32_t arg0, uint32_t arg1) {
    fifoAddData(0, arg1, (arg0 >= 0x02)? SIZE_GUTEXPALETTE:SIZE_GUNCCTABLE);
    pt[1] = arg0; pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexDownloadTable3x, 2);
}
void PT_CALL grTexDownloadTablePartial(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    fifoAddData(0, arg1, (arg0 >= 0x02)? ALIGNED((arg3 + 1)*sizeof(uint32_t)):SIZE_GUNCCTABLE);
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexDownloadTablePartial3x, 4);
}
void PT_CALL grTexFilterMode(uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexFilterMode, 3);
}
void PT_CALL grTexLodBiasValue(uint32_t arg0, uint32_t arg1) {
    pt[1] = arg0; pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexLodBiasValue, 2);
}
uint32_t PT_CALL grTexMaxAddress(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grTexMaxAddress;
    return *pt0;
}
uint32_t PT_CALL grTexMinAddress(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grTexMinAddress;
    return *pt0;
}
void PT_CALL grTexMipMapMode(uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexMipMapMode, 3);
}
void PT_CALL grTexMultibase(uint32_t arg0, uint32_t arg1) {
    pt[1] = arg0; pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grTexMultibase;
}
void PT_CALL grTexMultibaseAddress(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; pt[5] = arg4; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grTexMultibaseAddress;
}
void PT_CALL grTexNCCTable(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexNCCTable3x, 1);
}
void PT_CALL grTexSource(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    fifoAddData(0, arg3, ALIGNED(SIZE_GRTEXINFO));
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grTexSource, 4);
}
uint32_t PT_CALL grTexTextureMemRequired(uint32_t arg0, uint32_t arg1) {
    fifoAddData(0, arg1, ALIGNED(SIZE_GRTEXINFO));
    pt[1] = arg0; pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grTexTextureMemRequired;
    return *pt0;
}
uint32_t PT_CALL gu3dfGetInfo(uint32_t arg0, uint32_t arg1) {
    int ret;
#if PUSH_F3DF    
    int i, fd;
    uint32_t len;

    fd = open((char *)arg0, O_BINARY | O_RDONLY);
    if (fd == -1)
	return 0;
    len = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    if (len > MAX_3DF)
        len = MAX_3DF;
    read(fd, &m3df[ALIGNED(1) >> 2], len);
    close(fd);
    m3df[0] = len;
    ft[0] = MAX_3DF;
#endif
    fifoAddData(0, (uint32_t)(basename((const char *)arg0)), sizeof(char[64]));
    pt[1] = arg0; pt[2] = arg1;
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_gu3dfGetInfo;
    ret = *pt0;
    if (ret)
        fifoOutData(0, arg1, SIZE_GU3DFINFO);
    return ret;
}
uint32_t PT_CALL gu3dfLoad(uint32_t arg0, uint32_t arg1) {
    int ret;
    wr3dfInfo *info = (wr3dfInfo *)arg1;
    fifoAddData(0, (uint32_t)(basename((const char *)arg0)), sizeof(char[64]));
    pt[1] = arg0; pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_gu3dfLoad;
    ret = *pt0;
    if (ret)
        memcpy(info->data, &m3df[ALIGNED(1) >> 2], info->mem_required);
    return ret;
}
void PT_CALL guFogGenerateExp2(uint32_t arg0, uint32_t arg1) {
    pt[1] = arg0; pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_guFogGenerateExp2;
    fifoOutData(0, arg0, sizeof(uint8_t[64]));
}
void PT_CALL guFogGenerateExp(uint32_t arg0, uint32_t arg1) {
    pt[1] = arg0; pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_guFogGenerateExp;
    fifoOutData(0, arg0, sizeof(uint8_t[64]));
}
void PT_CALL guFogGenerateLinear(uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_guFogGenerateLinear;
    fifoOutData(0, arg0, sizeof(uint8_t[64]));
}
float PT_CALL guFogTableIndexToW(uint32_t arg0) {
    float ret;
    uint32_t r;
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_guFogTableIndexToW;
    r = *pt0;
    memcpy(&ret, &r, sizeof(uint32_t));
    return ret;
}
void PT_CALL grCoordinateSpace(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grCoordinateSpace, 1);
}
void PT_CALL grDepthRange(uint32_t arg0, uint32_t arg1) {
    pt[1] = arg0; pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grDepthRange, 2);
}
void PT_CALL grDisable(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grDisable, 1);
}
void PT_CALL grDrawVertexArray(uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    int i;
    uint8_t **vp = (uint8_t **)arg2;
    for (i = 0; i < arg1; i++)
        fifoAddData(0, (uint32_t)vp[i], ALIGNED(arg1 * SIZE_GRVERTEX));
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grDrawVertexArray, 3);
}
void PT_CALL grDrawVertexArrayContiguous(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    fifoAddData(0, arg2, ALIGNED(arg1 * arg3));
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grDrawVertexArrayContiguous, 4);
}
void PT_CALL grEnable(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grEnable, 1);
}
void PT_CALL grFinish(void) {
    
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grFinish;
}
void PT_CALL grFlush(void) {
    
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grFlush;
}
uint32_t PT_CALL grGet(uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    int ret;
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grGet;
    ret = *pt0;
    fifoOutData(0, arg2, arg1);
    return ret;
}
static void PT_CALL grChromaRangeExt(uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2;
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grChromaRangeExt;
}
static void PT_CALL grChromaRangeModeExt(uint32_t arg0) {
    pt[1] = arg0;
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grChromaRangeModeExt;
}
uint32_t PT_CALL grGetProcAddress(uint32_t arg0) {
    void *fptr = 0;
    if (!memcmp((const void *)arg0, "grChromaRangeModeExt", sizeof("grChromaRangeModeExt")))
	fptr = &grChromaRangeModeExt;
    if (!memcmp((const void *)arg0, "grChromaRangeExt", sizeof("grChromaRangeExt")))
	fptr = &grChromaRangeExt;
    return (uint32_t)fptr;
}
const char * PT_CALL grGetString(uint32_t arg0) {
    static const char *cstrTbl[] = {
	"GETGAMMA CHROMARANGE TEXMIRROR PALETTE6666",
	"Voodoo2",
	"Glide",
	"3Dfx Interactive",
	"3.01",
    };
    if ((arg0 & 0x0F) < 0x05)
	return cstrTbl[arg0 & 0x0F];
    else
	return 0;
}
void PT_CALL grGlideGetVertexLayout(uint32_t arg0) {
  /*  pt[1] = arg0; */
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grGlideGetVertexLayout;
}
void PT_CALL grGlideSetVertexLayout(uint32_t arg0) {
  /*  pt[1] = arg0; */
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grGlideSetVertexLayout;
}
void PT_CALL grLoadGammaTable(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    pt[1] = arg0; 
    fifoAddData(2, arg1, ALIGNED(arg0 * sizeof(uint32_t))); 
    fifoAddData(3, arg2, ALIGNED(arg0 * sizeof(uint32_t))); 
    fifoAddData(4, arg3, ALIGNED(arg0 * sizeof(uint32_t))); 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grLoadGammaTable;
}
uint32_t PT_CALL grQueryResolutions(uint32_t arg0, uint32_t arg1) {
    int ret;
    static int listSize;
    fifoAddData(1, arg0, SIZE_GRRESOLUTION);
    pt[2] = arg1; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grQueryResolutions;
    ret = *pt0;
    if (arg1 == 0)
        listSize = ret;
    else {
        fifoOutData(0, arg1, listSize);
        listSize = 0;
    }
    return ret;
}
uint32_t PT_CALL grReset(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grReset;
    return *pt0;
}
uint32_t PT_CALL grSelectContext(uint32_t arg0) {
    pt[1] = arg0; 
    pt0 = (uint32_t *)pt[0]; *pt0 = FEnum_grSelectContext;
    return *pt0;
}
void PT_CALL grVertexLayout(uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grVertexLayout, 3);
}
void PT_CALL grViewport(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; pt[4] = arg3; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_grViewport, 4);
}
void PT_CALL guGammaCorrectionRGB(uint32_t arg0, uint32_t arg1, uint32_t arg2) {
    pt[1] = arg0; pt[2] = arg1; pt[3] = arg2; 
    pt0 = (uint32_t *)pt[0]; FIFO_GRFUNC(FEnum_guGammaCorrectionRGB, 3);
}
/* End - generated by wrapper_genfuncs */


BOOL APIENTRY DllMain( HINSTANCE hModule,
        DWORD dwReason,
        LPVOID lpReserved
        )
{
    uint32_t HostRet;

    switch(dwReason) {
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_ATTACH:
            if (fxlibInit()) {
                if (InitGlidePTMMBase()) {
                    fxlibFini();
                    return FALSE;
                }
            }
            else
                return FALSE;
#ifdef DEBUG_WRAPPER
	    logfp = fopen(LOG_FNAME, "w");
#endif
	    DPRINTF("ptm 0x%08x, lfb 0x%08x\n", (uint32_t)ptm, (uint32_t)lfb);
	    ptm[(0xfbcU >> 2)] = (0xa0UL << 12) | GLIDEVER;
	    HostRet = ptm[(0xfbcU >> 2)];
	    if (HostRet != ((GLIDEVER << 8) | 0xa0UL)) {
		DPRINTF("Error - Glide3x init failed 0x%08x\n", HostRet);
		return FALSE;
	    }
            break;
        case DLL_PROCESS_DETACH:
	    if (grGlidePresent)
		grGlideShutdown();
	    ptm[(0xfbcU >> 2)] = (0xd0UL << 12) | GLIDEVER;
#ifdef DEBUG_WRAPPER
	    fclose(logfp);
#endif
            fxlibFini();
            break;
    }

    return TRUE;
}
