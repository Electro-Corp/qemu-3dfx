/*
 * QEMU MESA GL Pass-Through
 *
 *  Copyright (c) 2020
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "qemu/timer.h"
#include "qemu-common.h"
#include "cpu.h"
#include "ui/console.h"

#include "mesagl_impl.h"

#define DPRINTF(fmt, ...) \
    do { fprintf(stderr, "glcntx: " fmt "\n" , ## __VA_ARGS__); fflush(stderr); } while(0)

#if defined(CONFIG_LINUX) && CONFIG_LINUX
#include <GL/glx.h>

typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint8_t BYTE;
typedef struct tagPIXELFORMATDESCRIPTOR {
  WORD  nSize;
  WORD  nVersion;
  DWORD dwFlags;
  BYTE  iPixelType;
  BYTE  cColorBits;
  BYTE  cRedBits;
  BYTE  cRedShift;
  BYTE  cGreenBits;
  BYTE  cGreenShift;
  BYTE  cBlueBits;
  BYTE  cBlueShift;
  BYTE  cAlphaBits;
  BYTE  cAlphaShift;
  BYTE  cAccumBits;
  BYTE  cAccumRedBits;
  BYTE  cAccumGreenBits;
  BYTE  cAccumBlueBits;
  BYTE  cAccumAlphaBits;
  BYTE  cDepthBits;
  BYTE  cStencilBits;
  BYTE  cAuxBuffers;
  BYTE  iLayerType;
  BYTE  bReserved;
  DWORD dwLayerMask;
  DWORD dwVisibleMask;
  DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, *LPPIXELFORMATDESCRIPTOR;

typedef struct tagFakePBuffer {
    int width;
    int height;
} HPBUFFERARB;

static const PIXELFORMATDESCRIPTOR pfd = {
    .nSize = sizeof(PIXELFORMATDESCRIPTOR),
    .nVersion = 1,
    .dwFlags = 0x225,
    .cColorBits = 32,
    .cRedBits = 8, .cGreenBits = 8, .cBlueBits = 8, .cAlphaBits = 8,
    .cRedShift = 16, .cGreenShift = 8, .cBlueShift = 0, .cAlphaShift = 24,
    .cDepthBits = 24,
    .cStencilBits = 8,
};
static Display     *dpy;
static Window       win;
static XVisualInfo *xvi;
static GLXContext   ctx;

HPBUFFERARB hPbuffer[MAX_PBUFFER];
static GLXPbuffer PBDC[MAX_PBUFFER];
static GLXContext PBRC[MAX_PBUFFER];

void SetMesaFuncPtr(void *hDLL)
{
}

void *MesaGLGetProc(const char *proc)
{
    return (void *)glXGetProcAddress((const GLubyte *)proc);
}

void MGLTmpContext(void) { }

void MGLDeleteContext(void)
{
    glXMakeContextCurrent(dpy, None, None, NULL);
    glXDestroyContext(dpy, ctx);
    ctx = 0;
    mesa_enabled_reset();
}

void MGLWndRelease(void)
{
    if (win) {
        XFree(xvi);
        XCloseDisplay(dpy);
        mesa_release_window();
        xvi = 0;
        dpy = 0;
        win = 0;
    }
}

int MGLCreateContext(uint32_t gDC)
{
    int i, ret;
    i = gDC & (MAX_PBUFFER -1);
    if (gDC == ((MESAGL_HPBDC & 0xFFFFFFF0U) | i)) { ret = 0; }
    else { 
        ctx = glXCreateContext(dpy, xvi, NULL, true);
        ret = (ctx)? 0:1;
    }
    return ret;
}

int MGLMakeCurrent(uint32_t cntxRC)
{
    uint32_t i = cntxRC & (MAX_PBUFFER - 1);
    if (cntxRC == MESAGL_MAGIC) {
        glXMakeContextCurrent(dpy, win, win, ctx);
        InitMesaGLExt();
        mesa_enabled_set();
    }
    if (cntxRC == (((MESAGL_MAGIC & 0xFFFFFFFU) << 4) | i))
        glXMakeContextCurrent(dpy, PBDC[i], PBDC[i], PBRC[i]);

    return 0;
}

int MGLSwapBuffers(void)
{
    glXSwapBuffers(dpy, win);
    return 1;
}

void MGLKickFrameProc(int k)
{
    if (k == GetKickFrame())
        mesa_enabled_reset();
    if (k == 1)
        mesa_enabled_set();
    DPRINTF(">>>>>>>> frame (%d) <<<<<<<<", k);
}

static int MGLPresetPixelFormat(void)
{
    dpy = XOpenDisplay(NULL);
    win = mesa_prepare_window();

    int attrib[] = {
      GLX_X_RENDERABLE    , True,
      GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
      GLX_RENDER_TYPE     , GLX_RGBA_BIT,
      GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
      GLX_RED_SIZE        , 8,
      GLX_GREEN_SIZE      , 8,
      GLX_BLUE_SIZE       , 8,
      GLX_ALPHA_SIZE      , 8,
      GLX_DEPTH_SIZE      , 24,
      GLX_STENCIL_SIZE    , 8,
      GLX_DOUBLEBUFFER    , True,
      None
    };

    int fbid, fbcnt;
    GLXFBConfig *fbcnf = glXChooseFBConfig(dpy, DefaultScreen(dpy), attrib, &fbcnt);
    xvi = glXGetVisualFromFBConfig(dpy, fbcnf[0]);
    glXGetFBConfigAttrib(dpy, fbcnf[0], GLX_FBCONFIG_ID, &fbid);
    DPRINTF("FBConfig id 0x%03x visual 0x%03lx", fbid, xvi->visualid);
    XFree(fbcnf);
    XFlush(dpy);
    return 1;
}

int MGLChoosePixelFormat(void)
{
    DPRINTF("ChoosePixelFormat()");
    if (xvi == 0)
        return MGLPresetPixelFormat();
    return 1;
}

int MGLSetPixelFormat(int fmt, const void *p)
{
    DPRINTF("SetPixelFormat()");
    if (xvi == 0)
        return MGLPresetPixelFormat();
    return 1;
}

int MGLDescribePixelFormat(int fmt, unsigned int sz, void *p)
{
    DPRINTF("DescribePixelFormat()");
    if (xvi == 0)
        MGLPresetPixelFormat();
    memcpy(p, &pfd, sizeof(PIXELFORMATDESCRIPTOR));
    return 1;
}

void MGLActivateHandler(int i)
{
#define WA_ACTIVE 1
#define WA_INACTIVE 0
    DPRINTF("                                                   \r"
            "glcntx: wm_activate %d", i);
    switch (i) {
        case WA_ACTIVE:
            mesa_enabled_set();
            break;
        case WA_INACTIVE:
            mesa_enabled_reset();
            break;
    }
}

static int CheckAttribArray(const int *attrib, const int attr)
{
    int ret = 0;
    for (int i = 0; (attrib[i] && attrib[i+1]); i+=2) {
        if ((attrib[i] == attr) &&
            (attrib[i+1] == 1)) {
            ret = 1;
            break;
        }
    }
    return ret;
}

void MGLFuncHandler(const char *name)
{
    char fname[64];
    uint32_t *argsp = (uint32_t *)(name + ALIGNED(strnlen(name, sizeof(fname))));
    strncpy(fname, name, sizeof(fname));

#define FUNCP_HANDLER(a) \
    if (!memcmp(fname, a, sizeof(a)))

    FUNCP_HANDLER("wglShareLists") {
        uint32_t i, ret = 0;
        i = argsp[1] & (MAX_PBUFFER - 1);
        if ((argsp[0] == MESAGL_MAGIC) && (argsp[1] == ((MESAGL_MAGIC & 0xFFFFFFFU) << 4 | i)))
            ret = 1;
        else {
            DPRINTF("  *WARN* ShareLists called with unknown contexts, %x %x", argsp[0], argsp[1]);
        }
        argsp[0] = ret;
        return;
    }
    FUNCP_HANDLER("wglUseFontBitmapsA") {
       DPRINTF("wglUseFontBitmapsA() %x %x %x", argsp[1], argsp[2], argsp[3]);
       argsp[0] = 0;
       return;
    }
    FUNCP_HANDLER("wglSwapIntervalEXT") {
        strncpy(fname, "glXSwapIntervalMESA", sizeof(fname));
        void (*fp)(uint32_t) =
           (void (*)(uint32_t)) MesaGLGetProc(fname);
        if (fp) {
            fp(argsp[0]);
            DPRINTF("wglSwapIntervalEXT(%u)", argsp[0]);
            argsp[0] = 1;
            return;
        }
    }
    FUNCP_HANDLER("wglGetSwapIntervalEXT") {
        strncpy(fname, "glXGetSwapIntervalMESA", sizeof(fname));
        uint32_t (*fp)(void) =
            (uint32_t (*)(void)) MesaGLGetProc(fname);
        if (fp) {
            argsp[0] = fp();
            DPRINTF("wglGetSwapIntervalEXT() ret %u", argsp[0]);
            return;
        }
    }
    FUNCP_HANDLER("wglGetExtensionsStringARB") {
        const char *tmp = "WGL_EXT_swap_control "
            "WGL_EXT_extensions_string " "WGL_ARB_extensions_string "
            "WGL_ARB_pixel_format " "WGL_ARB_pbuffer "
            "WGL_ARB_render_texture";
        strncpy((char *)name, tmp, TARGET_PAGE_SIZE);
        return;
    }
    FUNCP_HANDLER("wglChoosePixelFormatARB") {
#define WGL_DRAW_TO_PBUFFER_ARB 0x202D
        const int *ia = (const int *)argsp;
        if (CheckAttribArray(ia, WGL_DRAW_TO_PBUFFER_ARB)) {
            argsp[1] = 0x02;
        }
        else {
            DPRINTF("wglChoosePixelFormatARB()");
            argsp[1] = MGLChoosePixelFormat();
        }
        argsp[0] = 1;
        return;
    }
    FUNCP_HANDLER("wglBindTexImageARB") {
        argsp[0] = 1;
        return;
    }
    FUNCP_HANDLER("wglReleaseTexImageARB") {
        argsp[0] = 1;
        return;
    }
    FUNCP_HANDLER("wglCreatePbufferARB") {
        uint32_t i;
        i = 0; while(hPbuffer[i].width) i++;
        if (i == MAX_PBUFFER) {
            DPRINTF("MAX_PBUFFER reached %d", i);
            argsp[0] = 0;
            return;
        }
        hPbuffer[i].width = argsp[1];
        hPbuffer[i].height = argsp[2];
        int ia[] = {
            GLX_X_RENDERABLE    , True,
            GLX_DRAWABLE_TYPE   , GLX_PBUFFER_BIT,
            GLX_RENDER_TYPE     , GLX_RGBA_BIT,
            GLX_RED_SIZE        , 8,
            GLX_GREEN_SIZE      , 8,
            GLX_BLUE_SIZE       , 8,
            None,
        };
        int pbcnt, pa[] = {
            GLX_PBUFFER_WIDTH, hPbuffer[i].width,
            GLX_PBUFFER_HEIGHT, hPbuffer[i].height,
            None,
        };
        GLXFBConfig *pbcnf = glXChooseFBConfig(dpy, DefaultScreen(dpy), ia, &pbcnt);
        PBDC[i] = glXCreatePbuffer(dpy, pbcnf[0], pa);
        PBRC[i] = glXCreateNewContext(dpy, pbcnf[0], GLX_RGBA_TYPE, ctx, true);
        XFree(pbcnf);
        argsp[0] = 1;
        argsp[1] = i;
        return;
    }
    FUNCP_HANDLER("wglDestroyPbufferARB") {
        uint32_t i;
        i = argsp[0] & (MAX_PBUFFER - 1);
        glXDestroyContext(dpy, PBRC[i]);
        glXDestroyPbuffer(dpy, PBDC[i]);
        PBRC[i] = 0; PBDC[i] = 0;
        memset(&hPbuffer[i], 0, sizeof(HPBUFFERARB));
        argsp[0] = 1;
        return;
    }
    FUNCP_HANDLER("wglQueryPbufferARB") {
        uint32_t i;
#define WGL_PBUFFER_WIDTH_ARB   0x2034
#define WGL_PBUFFER_HEIGHT_ARB  0x2035
        int attr = argsp[1];
        i = argsp[0] & (MAX_PBUFFER - 1);
        argsp[1] = (attr == WGL_PBUFFER_WIDTH_ARB)? hPbuffer[i].width:argsp[1];
        argsp[1] = (attr == WGL_PBUFFER_HEIGHT_ARB)? hPbuffer[i].height:argsp[1];
        argsp[0] = (argsp[1] == attr)? 0:1;
        return;
    }

    DPRINTF("  *WARN* Unhandled GLFunc %s", name);
    argsp[0] = 0;
}

#endif //CONFIG_LINUX