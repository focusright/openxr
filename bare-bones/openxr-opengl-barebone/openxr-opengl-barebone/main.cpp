#pragma comment( lib, "OpenGL32.lib" )

#define no_init_all

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#define OPENGL_VERSION_MAJOR 4
#define OPENGL_VERSION_MINOR 3
#define GLSL_VERSION "430"
#define SPIRV_VERSION "99"
#define USE_SYNC_OBJECT 0  // 0 = GLsync, 1 = EGLSyncKHR, 2 = storage buffer
#define GRAPHICS_API_OPENGL 1
#define _USE_MATH_DEFINES

#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>
#include <GL/gl_format.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <common/xr_linear.h>
#include <thread>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <array>
#include <cmath>

using namespace std;

#define OPENGL_VERSION_MAJOR 4
#define OPENGL_VERSION_MINOR 3

#define MAX_QUEUES 16
#define BIT(x) (1 << (x))
#define UNUSED_PARM(x) { (void)(x); }
#define APPLICATION_NAME "OpenGL SI"
#define WINDOW_TITLE "OpenGL SI"
#define GL(func) func;

typedef INT_PTR (WINAPI *PROC)();
typedef uint64_t ksNanoseconds;

typedef enum {
    KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5,
    KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5,
    KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8,
    KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8,
    KS_GPU_SURFACE_COLOR_FORMAT_MAX
} ksGpuSurfaceColorFormat;

typedef enum {
    KS_GPU_SURFACE_DEPTH_FORMAT_NONE,
    KS_GPU_SURFACE_DEPTH_FORMAT_D16,
    KS_GPU_SURFACE_DEPTH_FORMAT_D24,
    KS_GPU_SURFACE_DEPTH_FORMAT_MAX
} ksGpuSurfaceDepthFormat;

typedef enum {
    KS_GPU_SAMPLE_COUNT_1 = 1,
    KS_GPU_SAMPLE_COUNT_2 = 2,
    KS_GPU_SAMPLE_COUNT_4 = 4,
    KS_GPU_SAMPLE_COUNT_8 = 8,
    KS_GPU_SAMPLE_COUNT_16 = 16,
    KS_GPU_SAMPLE_COUNT_32 = 32,
    KS_GPU_SAMPLE_COUNT_64 = 64,
} ksGpuSampleCount;

typedef enum {
    KS_GPU_QUEUE_PROPERTY_GRAPHICS = BIT(0),
    KS_GPU_QUEUE_PROPERTY_COMPUTE = BIT(1),
    KS_GPU_QUEUE_PROPERTY_TRANSFER = BIT(2)
} ksGpuQueueProperty;

typedef enum { KS_GPU_QUEUE_PRIORITY_LOW, KS_GPU_QUEUE_PRIORITY_MEDIUM, KS_GPU_QUEUE_PRIORITY_HIGH } ksGpuQueuePriority;

typedef struct {
    int placeholder;
} ksDriverInstance;

typedef struct {
    unsigned char redBits;
    unsigned char greenBits;
    unsigned char blueBits;
    unsigned char alphaBits;
    unsigned char colorBits;
    unsigned char depthBits;
} ksGpuSurfaceBits;

typedef struct {
    bool keyInput[256];
    bool mouseInput[8];
    int mouseInputX[8];
    int mouseInputY[8];
} ksGpuWindowInput;

typedef struct {
    int queueCount;                                  // number of queues
    ksGpuQueueProperty queueProperties;              // desired queue family properties
    ksGpuQueuePriority queuePriorities[MAX_QUEUES];  // individual queue priorities
} ksGpuQueueInfo;

typedef struct {
    ksDriverInstance *instance;
    ksGpuQueueInfo queueInfo;
} ksGpuDevice;

typedef struct {
    const ksGpuDevice *device;
    HDC hDC;
    HGLRC hGLRC;
} ksGpuContext;

typedef struct {
    ksGpuDevice device;
    ksGpuContext context;
    ksGpuSurfaceColorFormat colorFormat;
    ksGpuSurfaceDepthFormat depthFormat;
    ksGpuSampleCount sampleCount;
    int windowWidth;
    int windowHeight;
    int windowSwapInterval;
    float windowRefreshRate;
    bool windowFullscreen;
    bool windowActive;
    bool windowExit;
    ksGpuWindowInput input;
    ksNanoseconds lastSwapTime;

    HINSTANCE hInstance;
    HDC hDC;
    HWND hWnd;
    bool windowActiveState;
} ksGpuWindow;

typedef struct {
    bool timer_query;                       // GL_ARB_timer_query, GL_EXT_disjoint_timer_query
    bool texture_clamp_to_border;           // GL_EXT_texture_border_clamp, GL_OES_texture_border_clamp
    bool buffer_storage;                    // GL_ARB_buffer_storage
    bool multi_sampled_storage;             // GL_ARB_texture_storage_multisample
    bool multi_view;                        // GL_OVR_multiview, GL_OVR_multiview2
    bool multi_sampled_resolve;             // GL_EXT_multisampled_render_to_texture
    bool multi_view_multi_sampled_resolve;  // GL_OVR_multiview_multisampled_render_to_texture

    int texture_clamp_to_border_id;
} ksOpenGLExtensions;

ksOpenGLExtensions glExtensions;


PROC GetExtension(const char *functionName) { return wglGetProcAddress(functionName); }

GLint glGetInteger(GLenum pname) {
    GLint i;
    GL(glGetIntegerv(pname, &i));
    return i;
}

static bool GlCheckExtension(const char *extension) {
    PFNGLGETSTRINGIPROC glGetStringi = (PFNGLGETSTRINGIPROC)GetExtension("glGetStringi");
    GL(const GLint numExtensions = glGetInteger(GL_NUM_EXTENSIONS));
    for (int i = 0; i < numExtensions; i++) {
        GL(const GLubyte *string = glGetStringi(GL_EXTENSIONS, i));
        if (strcmp((const char *)string, extension) == 0) {
            return true;
        }
    }
    return false;
}

PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
PFNGLISRENDERBUFFERPROC glIsRenderbuffer;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC glRenderbufferStorageMultisampleEXT;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer;
//PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC glFramebufferTexture2DMultisampleEXT;
PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC glFramebufferTextureMultiviewOVR;
//PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC glFramebufferTextureMultisampleMultiviewOVR;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC glCheckNamedFramebufferStatus;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBINDBUFFERBASEPROC glBindBufferBase;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
PFNGLBUFFERSTORAGEPROC glBufferStorage;
PFNGLMAPBUFFERPROC glMapBuffer;
PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
PFNGLUNMAPBUFFERPROC glUnmapBuffer;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLACTIVETEXTUREPROC glActiveTexture;
PFNGLTEXIMAGE3DPROC glTexImage3D;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D;
PFNGLCOMPRESSEDTEXIMAGE3DPROC glCompressedTexImage3D;
PFNGLTEXSUBIMAGE3DPROC glTexSubImage3D;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glCompressedTexSubImage3D;
PFNGLTEXSTORAGE2DPROC glTexStorage2D;
PFNGLTEXSTORAGE3DPROC glTexStorage3D;
PFNGLTEXIMAGE2DMULTISAMPLEPROC glTexImage2DMultisample;
PFNGLTEXIMAGE3DMULTISAMPLEPROC glTexImage3DMultisample;
PFNGLTEXSTORAGE2DMULTISAMPLEPROC glTexStorage2DMultisample;
PFNGLTEXSTORAGE3DMULTISAMPLEPROC glTexStorage3DMultisample;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
PFNGLBINDIMAGETEXTUREPROC glBindImageTexture;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLCREATESHADERPROC glCreateShader;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex;
PFNGLGETPROGRAMRESOURCEINDEXPROC glGetProgramResourceIndex;
PFNGLUNIFORMBLOCKBINDINGPROC glUniformBlockBinding;
PFNGLSHADERSTORAGEBLOCKBINDINGPROC glShaderStorageBlockBinding;
PFNGLPROGRAMUNIFORM1IPROC glProgramUniform1i;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM1IVPROC glUniform1iv;
PFNGLUNIFORM2IVPROC glUniform2iv;
PFNGLUNIFORM3IVPROC glUniform3iv;
PFNGLUNIFORM4IVPROC glUniform4iv;
PFNGLUNIFORM1FPROC glUniform1f;
PFNGLUNIFORM1FVPROC glUniform1fv;
PFNGLUNIFORM2FVPROC glUniform2fv;
PFNGLUNIFORM3FVPROC glUniform3fv;
PFNGLUNIFORM4FVPROC glUniform4fv;
PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;
PFNGLUNIFORMMATRIX2X3FVPROC glUniformMatrix2x3fv;
PFNGLUNIFORMMATRIX2X4FVPROC glUniformMatrix2x4fv;
PFNGLUNIFORMMATRIX3X2FVPROC glUniformMatrix3x2fv;
PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv;
PFNGLUNIFORMMATRIX4X2FVPROC glUniformMatrix4x2fv;
PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;
PFNGLDISPATCHCOMPUTEPROC glDispatchCompute;
PFNGLMEMORYBARRIERPROC glMemoryBarrier;
PFNGLGENQUERIESPROC glGenQueries;
PFNGLDELETEQUERIESPROC glDeleteQueries;
PFNGLISQUERYPROC glIsQuery;
PFNGLBEGINQUERYPROC glBeginQuery;
PFNGLENDQUERYPROC glEndQuery;
PFNGLQUERYCOUNTERPROC glQueryCounter;
PFNGLGETQUERYIVPROC glGetQueryiv;
PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv;
PFNGLGETQUERYOBJECTUIVPROC glGetQueryObjectuiv;
PFNGLGETQUERYOBJECTI64VPROC glGetQueryObjecti64v;
PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64v;
PFNGLFENCESYNCPROC glFenceSync;
PFNGLCLIENTWAITSYNCPROC glClientWaitSync;
PFNGLDELETESYNCPROC glDeleteSync;
PFNGLISSYNCPROC glIsSync;
PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
PFNGLBLENDEQUATIONSEPARATEPROC glBlendEquationSeparate;
PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl;
PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;
PFNGLBLENDCOLORPROC glBlendColor;
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
PFNWGLDELAYBEFORESWAPNVPROC wglDelayBeforeSwapNV;



void GlInitExtensions() {
    glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)GetExtension("glGenFramebuffers");
    glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)GetExtension("glDeleteFramebuffers");
    glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)GetExtension("glBindFramebuffer");
    glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)GetExtension("glBlitFramebuffer");
    glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)GetExtension("glGenRenderbuffers");
    glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)GetExtension("glDeleteRenderbuffers");
    glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)GetExtension("glBindRenderbuffer");
    glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)GetExtension("glIsRenderbuffer");
    glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)GetExtension("glRenderbufferStorage");
    glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)GetExtension("glRenderbufferStorageMultisample");
    glRenderbufferStorageMultisampleEXT = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC)GetExtension("glRenderbufferStorageMultisampleEXT");
    glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)GetExtension("glFramebufferRenderbuffer");
    glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)GetExtension("glFramebufferTexture2D");
    glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)GetExtension("glFramebufferTextureLayer");
    //glFramebufferTexture2DMultisampleEXT = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC)GetExtension("glFramebufferTexture2DMultisampleEXT");
    glFramebufferTextureMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTIVIEWOVRPROC)GetExtension("glFramebufferTextureMultiviewOVR");
    //glFramebufferTextureMultisampleMultiviewOVR = (PFNGLFRAMEBUFFERTEXTUREMULTISAMPLEMULTIVIEWOVRPROC)GetExtension("glFramebufferTextureMultisampleMultiviewOVR");
    glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)GetExtension("glCheckFramebufferStatus");
    glCheckNamedFramebufferStatus = (PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC)GetExtension("glCheckNamedFramebufferStatus");

    glGenBuffers = (PFNGLGENBUFFERSPROC)GetExtension("glGenBuffers");
    glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)GetExtension("glDeleteBuffers");
    glBindBuffer = (PFNGLBINDBUFFERPROC)GetExtension("glBindBuffer");
    glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)GetExtension("glBindBufferBase");
    glBufferData = (PFNGLBUFFERDATAPROC)GetExtension("glBufferData");
    glBufferSubData = (PFNGLBUFFERSUBDATAPROC)GetExtension("glBufferSubData");
    glBufferStorage = (PFNGLBUFFERSTORAGEPROC)GetExtension("glBufferStorage");
    glMapBuffer = (PFNGLMAPBUFFERPROC)GetExtension("glMapBuffer");
    glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)GetExtension("glMapBufferRange");
    glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)GetExtension("glUnmapBuffer");

    glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)GetExtension("glGenVertexArrays");
    glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)GetExtension("glDeleteVertexArrays");
    glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)GetExtension("glBindVertexArray");
    glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)GetExtension("glVertexAttribPointer");
    glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)GetExtension("glVertexAttribDivisor");
    glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)GetExtension("glDisableVertexAttribArray");
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)GetExtension("glEnableVertexAttribArray");

    glActiveTexture = (PFNGLACTIVETEXTUREPROC)GetExtension("glActiveTexture");
    glTexImage3D = (PFNGLTEXIMAGE3DPROC)GetExtension("glTexImage3D");
    glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)GetExtension("glCompressedTexImage2D ");
    glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)GetExtension("glCompressedTexImage3D ");
    glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)GetExtension("glTexSubImage3D");
    glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)GetExtension("glCompressedTexSubImage2D");
    glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)GetExtension("glCompressedTexSubImage3D");

    glTexStorage2D = (PFNGLTEXSTORAGE2DPROC)GetExtension("glTexStorage2D");
    glTexStorage3D = (PFNGLTEXSTORAGE3DPROC)GetExtension("glTexStorage3D");
    glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)GetExtension("glTexImage2DMultisample");
    glTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC)GetExtension("glTexImage3DMultisample");
    glTexStorage2DMultisample = (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)GetExtension("glTexStorage2DMultisample");
    glTexStorage3DMultisample = (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)GetExtension("glTexStorage3DMultisample");
    glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)GetExtension("glGenerateMipmap");
    glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)GetExtension("glBindImageTexture");

    glCreateProgram = (PFNGLCREATEPROGRAMPROC)GetExtension("glCreateProgram");
    glDeleteProgram = (PFNGLDELETEPROGRAMPROC)GetExtension("glDeleteProgram");
    glCreateShader = (PFNGLCREATESHADERPROC)GetExtension("glCreateShader");
    glDeleteShader = (PFNGLDELETESHADERPROC)GetExtension("glDeleteShader");
    glShaderSource = (PFNGLSHADERSOURCEPROC)GetExtension("glShaderSource");
    glCompileShader = (PFNGLCOMPILESHADERPROC)GetExtension("glCompileShader");
    glGetShaderiv = (PFNGLGETSHADERIVPROC)GetExtension("glGetShaderiv");
    glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)GetExtension("glGetShaderInfoLog");
    glUseProgram = (PFNGLUSEPROGRAMPROC)GetExtension("glUseProgram");
    glAttachShader = (PFNGLATTACHSHADERPROC)GetExtension("glAttachShader");
    glLinkProgram = (PFNGLLINKPROGRAMPROC)GetExtension("glLinkProgram");
    glGetProgramiv = (PFNGLGETPROGRAMIVPROC)GetExtension("glGetProgramiv");
    glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)GetExtension("glGetProgramInfoLog");
    glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)GetExtension("glGetAttribLocation");
    glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)GetExtension("glBindAttribLocation");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)GetExtension("glGetUniformLocation");
    glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)GetExtension("glGetUniformBlockIndex");
    glProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)GetExtension("glProgramUniform1i");
    glUniform1i = (PFNGLUNIFORM1IPROC)GetExtension("glUniform1i");
    glUniform1iv = (PFNGLUNIFORM1IVPROC)GetExtension("glUniform1iv");
    glUniform2iv = (PFNGLUNIFORM2IVPROC)GetExtension("glUniform2iv");
    glUniform3iv = (PFNGLUNIFORM3IVPROC)GetExtension("glUniform3iv");
    glUniform4iv = (PFNGLUNIFORM4IVPROC)GetExtension("glUniform4iv");
    glUniform1f = (PFNGLUNIFORM1FPROC)GetExtension("glUniform1f");
    glUniform1fv = (PFNGLUNIFORM1FVPROC)GetExtension("glUniform1fv");
    glUniform2fv = (PFNGLUNIFORM2FVPROC)GetExtension("glUniform2fv");
    glUniform3fv = (PFNGLUNIFORM3FVPROC)GetExtension("glUniform3fv");
    glUniform4fv = (PFNGLUNIFORM4FVPROC)GetExtension("glUniform4fv");
    glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)GetExtension("glUniformMatrix3fv");
    glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)GetExtension("glUniformMatrix2x3fv");
    glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)GetExtension("glUniformMatrix2x4fv");
    glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)GetExtension("glUniformMatrix3x2fv");
    glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)GetExtension("glUniformMatrix3fv");
    glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)GetExtension("glUniformMatrix3x4fv");
    glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)GetExtension("glUniformMatrix4x2fv");
    glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)GetExtension("glUniformMatrix4x3fv");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)GetExtension("glUniformMatrix4fv");
    glGetProgramResourceIndex = (PFNGLGETPROGRAMRESOURCEINDEXPROC)GetExtension("glGetProgramResourceIndex");
    glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)GetExtension("glUniformBlockBinding");
    glShaderStorageBlockBinding = (PFNGLSHADERSTORAGEBLOCKBINDINGPROC)GetExtension("glShaderStorageBlockBinding");

    glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)GetExtension("glDrawElementsInstanced");
    glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)GetExtension("glDispatchCompute");
    glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)GetExtension("glMemoryBarrier");

    glGenQueries = (PFNGLGENQUERIESPROC)GetExtension("glGenQueries");
    glDeleteQueries = (PFNGLDELETEQUERIESPROC)GetExtension("glDeleteQueries");
    glIsQuery = (PFNGLISQUERYPROC)GetExtension("glIsQuery");
    glBeginQuery = (PFNGLBEGINQUERYPROC)GetExtension("glBeginQuery");
    glEndQuery = (PFNGLENDQUERYPROC)GetExtension("glEndQuery");
    glQueryCounter = (PFNGLQUERYCOUNTERPROC)GetExtension("glQueryCounter");
    glGetQueryiv = (PFNGLGETQUERYIVPROC)GetExtension("glGetQueryiv");
    glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)GetExtension("glGetQueryObjectiv");
    glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)GetExtension("glGetQueryObjectuiv");
    glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC)GetExtension("glGetQueryObjecti64v");
    glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)GetExtension("glGetQueryObjectui64v");

    glFenceSync = (PFNGLFENCESYNCPROC)GetExtension("glFenceSync");
    glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)GetExtension("glClientWaitSync");
    glDeleteSync = (PFNGLDELETESYNCPROC)GetExtension("glDeleteSync");
    glIsSync = (PFNGLISSYNCPROC)GetExtension("glIsSync");

    glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)GetExtension("glBlendFuncSeparate");
    glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)GetExtension("glBlendEquationSeparate");

    glBlendColor = (PFNGLBLENDCOLORPROC)GetExtension("glBlendColor");

    glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)GetExtension("glDebugMessageControl");
    glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)GetExtension("glDebugMessageCallback");

    glExtensions.timer_query = GlCheckExtension("GL_EXT_timer_query");
    glExtensions.texture_clamp_to_border = true;  // always available
    glExtensions.buffer_storage = GlCheckExtension("GL_EXT_buffer_storage") || (OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR >= 44);
    glExtensions.multi_sampled_storage = GlCheckExtension("GL_ARB_texture_storage_multisample") || (OPENGL_VERSION_MAJOR * 10 + OPENGL_VERSION_MINOR >= 43);
    glExtensions.multi_view = GlCheckExtension("GL_OVR_multiview2");
    glExtensions.multi_sampled_resolve = GlCheckExtension("GL_EXT_multisampled_render_to_texture");
    glExtensions.multi_view_multi_sampled_resolve = GlCheckExtension("GL_OVR_multiview_multisampled_render_to_texture");

    glExtensions.texture_clamp_to_border_id = GL_CLAMP_TO_BORDER;
}

static ksNanoseconds GetTimeNanoseconds() {
    static ksNanoseconds ticksPerSecond = 0;
    static ksNanoseconds timeBase = 0;

    if (ticksPerSecond == 0) {
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        ticksPerSecond = (ksNanoseconds)li.QuadPart;
        QueryPerformanceCounter(&li);
        timeBase = (ksNanoseconds)li.LowPart + 0xFFFFFFFFULL * li.HighPart;
    }

    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    ksNanoseconds counter = (ksNanoseconds)li.LowPart + 0xFFFFFFFFULL * li.HighPart;
    return (counter - timeBase) * 1000ULL * 1000ULL * 1000ULL / ticksPerSecond;
}

static void Error(const char *format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf_s(buffer, 4096, _TRUNCATE, format, args);
    va_end(args);
    OutputDebugStringA(buffer);
}

ksGpuSurfaceBits ksGpuContext_BitsForSurfaceFormat(const ksGpuSurfaceColorFormat colorFormat,
                                                   const ksGpuSurfaceDepthFormat depthFormat) {
    ksGpuSurfaceBits bits;
    bits.redBits = ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8)
                        ? 8
                        : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8)
                               ? 8
                               : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5)
                                      ? 5
                                      : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5) ? 5 : 8))));
    bits.greenBits = ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8)
                          ? 8
                          : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8)
                                 ? 8
                                 : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5)
                                        ? 6
                                        : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5) ? 6 : 8))));
    bits.blueBits = ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8)
                         ? 8
                         : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8)
                                ? 8
                                : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5)
                                       ? 5
                                       : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5) ? 5 : 8))));
    bits.alphaBits = ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R8G8B8A8)
                          ? 8
                          : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8)
                                 ? 8
                                 : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_R5G6B5)
                                        ? 0
                                        : ((colorFormat == KS_GPU_SURFACE_COLOR_FORMAT_B5G6R5) ? 0 : 8))));
    bits.colorBits = bits.redBits + bits.greenBits + bits.blueBits + bits.alphaBits;
    bits.depthBits =
        ((depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_D16) ? 16 : ((depthFormat == KS_GPU_SURFACE_DEPTH_FORMAT_D24) ? 24 : 0));
    return bits;
}

void GlBootstrapExtensions() {
    wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)GetExtension("wglChoosePixelFormatARB");
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GetExtension("wglCreateContextAttribsARB");
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)GetExtension("wglSwapIntervalEXT");
    wglDelayBeforeSwapNV = (PFNWGLDELAYBEFORESWAPNVPROC)GetExtension("wglDelayBeforeSwapNV");
}






typedef enum {
    KEY_A = 0x41,
    KEY_B = 0x42,
    KEY_C = 0x43,
    KEY_D = 0x44,
    KEY_E = 0x45,
    KEY_F = 0x46,
    KEY_G = 0x47,
    KEY_H = 0x48,
    KEY_I = 0x49,
    KEY_J = 0x4A,
    KEY_K = 0x4B,
    KEY_L = 0x4C,
    KEY_M = 0x4D,
    KEY_N = 0x4E,
    KEY_O = 0x4F,
    KEY_P = 0x50,
    KEY_Q = 0x51,
    KEY_R = 0x52,
    KEY_S = 0x53,
    KEY_T = 0x54,
    KEY_U = 0x55,
    KEY_V = 0x56,
    KEY_W = 0x57,
    KEY_X = 0x58,
    KEY_Y = 0x59,
    KEY_Z = 0x5A,
    KEY_RETURN = VK_RETURN,
    KEY_TAB = VK_TAB,
    KEY_ESCAPE = VK_ESCAPE,
    KEY_SHIFT_LEFT = VK_LSHIFT,
    KEY_CTRL_LEFT = VK_LCONTROL,
    KEY_ALT_LEFT = VK_LMENU,
    KEY_CURSOR_UP = VK_UP,
    KEY_CURSOR_DOWN = VK_DOWN,
    KEY_CURSOR_LEFT = VK_LEFT,
    KEY_CURSOR_RIGHT = VK_RIGHT
} ksKeyboardKey;

typedef enum { MOUSE_LEFT = 0, MOUSE_RIGHT = 1 } ksMouseButton;

LRESULT APIENTRY WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    ksGpuWindow *window = (ksGpuWindow *)GetWindowLongPtrA(hWnd, GWLP_USERDATA);

    switch (message) {
        case WM_SIZE: {
            if (window != NULL) {
                window->windowWidth = (int)LOWORD(lParam);
                window->windowHeight = (int)HIWORD(lParam);
            }
            return 0;
        }
        case WM_ACTIVATE: {
            if (window != NULL) {
                window->windowActiveState = !HIWORD(wParam);
            }
            return 0;
        }
        case WM_ERASEBKGND: {
            return 0;
        }
        case WM_CLOSE: {
            PostQuitMessage(0);
            return 0;
        }
        case WM_KEYDOWN: {
            if (window != NULL) {
                if ((int)wParam >= 0 && (int)wParam < 256) {
                    if ((int)wParam != KEY_SHIFT_LEFT && (int)wParam != KEY_CTRL_LEFT && (int)wParam != KEY_ALT_LEFT &&
                        (int)wParam != KEY_CURSOR_UP && (int)wParam != KEY_CURSOR_DOWN && (int)wParam != KEY_CURSOR_LEFT &&
                        (int)wParam != KEY_CURSOR_RIGHT) {
                        window->input.keyInput[(int)wParam] = true;
                    }
                }
            }
            break;
        }
        case WM_LBUTTONDOWN: {
            window->input.mouseInput[MOUSE_LEFT] = true;
            window->input.mouseInputX[MOUSE_LEFT] = LOWORD(lParam);
            window->input.mouseInputY[MOUSE_LEFT] = window->windowHeight - HIWORD(lParam);
            break;
        }
        case WM_RBUTTONDOWN: {
            window->input.mouseInput[MOUSE_RIGHT] = true;
            window->input.mouseInputX[MOUSE_RIGHT] = LOWORD(lParam);
            window->input.mouseInputY[MOUSE_RIGHT] = window->windowHeight - HIWORD(lParam);
            break;
        }
    }
    return DefWindowProcA(hWnd, message, wParam, lParam);
}

bool ksGpuDevice_Create(ksGpuDevice *device, ksDriverInstance *instance, const ksGpuQueueInfo *queueInfo) {
    memset(device, 0, sizeof(ksGpuDevice));
    device->instance = instance;
    device->queueInfo = *queueInfo;
    return true;
}

void ksGpuContext_SetCurrent(ksGpuContext *context) { wglMakeCurrent(context->hDC, context->hGLRC); }

void ksGpuDevice_Destroy(ksGpuDevice *device) { memset(device, 0, sizeof(ksGpuDevice)); }

void ksGpuContext_Destroy(ksGpuContext *context) {
    if (context->hGLRC) {
        if (!wglMakeCurrent(NULL, NULL)) {
            DWORD error = GetLastError();
            Error("Failed to release context error code (%d).", error);
        }
        if (!wglDeleteContext(context->hGLRC)) {
            DWORD error = GetLastError();
            Error("Failed to delete context error code (%d).", error);
        }
        context->hGLRC = NULL;
    }
}

void ksGpuWindow_Destroy(ksGpuWindow *window) {
    ksGpuContext_Destroy(&window->context);
    ksGpuDevice_Destroy(&window->device);

    if (window->windowFullscreen) {
        ChangeDisplaySettingsA(NULL, 0);
        ShowCursor(TRUE);
    }

    if (window->hDC) {
        if (!ReleaseDC(window->hWnd, window->hDC)) {
            Error("Failed to release device context.");
        }
        window->hDC = NULL;
    }

    if (window->hWnd) {
        if (!DestroyWindow(window->hWnd)) {
            Error("Failed to destroy the window.");
        }
        window->hWnd = NULL;
    }

    if (window->hInstance) {
        if (!UnregisterClassA(APPLICATION_NAME, window->hInstance)) {
            Error("Failed to unregister window class.");
        }
        window->hInstance = NULL;
    }
}

static bool ksGpuContext_CreateForSurface(ksGpuContext *context, const ksGpuDevice *device, const int queueIndex,
                                          const ksGpuSurfaceColorFormat colorFormat, const ksGpuSurfaceDepthFormat depthFormat,
                                          const ksGpuSampleCount sampleCount, HINSTANCE hInstance, HDC hDC) {
    UNUSED_PARM(queueIndex);

    context->device = device;

    const ksGpuSurfaceBits bits = ksGpuContext_BitsForSurfaceFormat(colorFormat, depthFormat);

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                        // version
        PFD_DRAW_TO_WINDOW |      // must support windowed
            PFD_SUPPORT_OPENGL |  // must support OpenGL
            PFD_DOUBLEBUFFER,     // must support double buffering
        PFD_TYPE_RGBA,            // iPixelType
        bits.colorBits,           // cColorBits
        0,
        0,  // cRedBits, cRedShift
        0,
        0,  // cGreenBits, cGreenShift
        0,
        0,  // cBlueBits, cBlueShift
        0,
        0,               // cAlphaBits, cAlphaShift
        0,               // cAccumBits
        0,               // cAccumRedBits
        0,               // cAccumGreenBits
        0,               // cAccumBlueBits
        0,               // cAccumAlphaBits
        bits.depthBits,  // cDepthBits
        0,               // cStencilBits
        0,               // cAuxBuffers
        PFD_MAIN_PLANE,  // iLayerType
        0,               // bReserved
        0,               // dwLayerMask
        0,               // dwVisibleMask
        0                // dwDamageMask
    };

    HWND localWnd = NULL;
    HDC localDC = hDC;

    if (sampleCount > KS_GPU_SAMPLE_COUNT_1) {
        localWnd = CreateWindowA(APPLICATION_NAME, "temp", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
        localDC = GetDC(localWnd);
    }

    int pixelFormat = ChoosePixelFormat(localDC, &pfd);
    if (pixelFormat == 0) {
        Error("Failed to find a suitable pixel format.");
        return false;
    }

    if (!SetPixelFormat(localDC, pixelFormat, &pfd)) {
        Error("Failed to set the pixel format.");
        return false;
    }

    {
        HGLRC hGLRC = wglCreateContext(localDC);
        wglMakeCurrent(localDC, hGLRC);

        GlBootstrapExtensions();

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hGLRC);
    }

    if (sampleCount > KS_GPU_SAMPLE_COUNT_1) {
        ReleaseDC(localWnd, localDC);
        DestroyWindow(localWnd);

        int pixelFormatAttribs[] = {WGL_DRAW_TO_WINDOW_ARB,
                                    GL_TRUE,
                                    WGL_SUPPORT_OPENGL_ARB,
                                    GL_TRUE,
                                    WGL_DOUBLE_BUFFER_ARB,
                                    GL_TRUE,
                                    WGL_PIXEL_TYPE_ARB,
                                    WGL_TYPE_RGBA_ARB,
                                    WGL_COLOR_BITS_ARB,
                                    bits.colorBits,
                                    WGL_DEPTH_BITS_ARB,
                                    bits.depthBits,
                                    WGL_SAMPLE_BUFFERS_ARB,
                                    1,
                                    WGL_SAMPLES_ARB,
                                    sampleCount,
                                    0};

        unsigned int numPixelFormats = 0;

        if (!wglChoosePixelFormatARB(hDC, pixelFormatAttribs, NULL, 1, &pixelFormat, &numPixelFormats) || numPixelFormats == 0) {
            Error("Failed to find MSAA pixel format.");
            return false;
        }

        memset(&pfd, 0, sizeof(pfd));

        if (!DescribePixelFormat(hDC, pixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &pfd)) {
            Error("Failed to describe the pixel format.");
            return false;
        }

        if (!SetPixelFormat(hDC, pixelFormat, &pfd)) {
            Error("Failed to set the pixel format.");
            return false;
        }
    }

    int contextAttribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
                            OPENGL_VERSION_MAJOR,
                            WGL_CONTEXT_MINOR_VERSION_ARB,
                            OPENGL_VERSION_MINOR,
                            WGL_CONTEXT_PROFILE_MASK_ARB,
                            WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                            WGL_CONTEXT_FLAGS_ARB,
                            WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
                            0};

    context->hDC = hDC;
    context->hGLRC = wglCreateContextAttribsARB(hDC, NULL, contextAttribs);
    if (!context->hGLRC) {
        Error("Failed to create GL context.");
        return false;
    }

    wglMakeCurrent(hDC, context->hGLRC);

    GlInitExtensions();

    return true;
}

bool ksGpuWindow_Create(ksGpuWindow *window, ksDriverInstance *instance, const ksGpuQueueInfo *queueInfo, int queueIndex,
                        ksGpuSurfaceColorFormat colorFormat, ksGpuSurfaceDepthFormat depthFormat, ksGpuSampleCount sampleCount,
                        int width, int height, bool fullscreen) {
    memset(window, 0, sizeof(ksGpuWindow));

    window->colorFormat = colorFormat;
    window->depthFormat = depthFormat;
    window->sampleCount = sampleCount;
    window->windowWidth = width;
    window->windowHeight = height;
    window->windowSwapInterval = 1;
    window->windowRefreshRate = 60.0f;
    window->windowFullscreen = fullscreen;
    window->windowActive = false;
    window->windowExit = false;
    window->windowActiveState = false;
    window->lastSwapTime = GetTimeNanoseconds();

    const LPCSTR displayDevice = NULL;

    if (window->windowFullscreen) {
        DEVMODEA dmScreenSettings;
        memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
        dmScreenSettings.dmSize = sizeof(dmScreenSettings);
        dmScreenSettings.dmPelsWidth = width;
        dmScreenSettings.dmPelsHeight = height;
        dmScreenSettings.dmBitsPerPel = 32;
        dmScreenSettings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

        if (ChangeDisplaySettingsExA(displayDevice, &dmScreenSettings, NULL, CDS_FULLSCREEN, NULL) != DISP_CHANGE_SUCCESSFUL) {
            Error("The requested fullscreen mode is not supported.");
            return false;
        }
    }

    DEVMODEA lpDevMode;
    memset(&lpDevMode, 0, sizeof(DEVMODEA));
    lpDevMode.dmSize = sizeof(DEVMODEA);
    lpDevMode.dmDriverExtra = 0;

    if (EnumDisplaySettingsA(displayDevice, ENUM_CURRENT_SETTINGS, &lpDevMode) != FALSE) {
        window->windowRefreshRate = (float)lpDevMode.dmDisplayFrequency;
    }

    window->hInstance = GetModuleHandleA(NULL);

    WNDCLASSA wc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = window->hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = APPLICATION_NAME;

    if (!RegisterClassA(&wc)) {
        Error("Failed to register window class.");
        return false;
    }

    DWORD dwExStyle = 0;
    DWORD dwStyle = 0;
    if (window->windowFullscreen) {
        dwExStyle = WS_EX_APPWINDOW;
        dwStyle = WS_POPUP;
        ShowCursor(FALSE);
    } else {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    }

    RECT windowRect;
    windowRect.left = (long)0;
    windowRect.right = (long)width;
    windowRect.top = (long)0;
    windowRect.bottom = (long)height;

    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

    if (!window->windowFullscreen) {
        RECT desktopRect;
        GetWindowRect(GetDesktopWindow(), &desktopRect);

        const int offsetX = (desktopRect.right - (windowRect.right - windowRect.left)) / 2;
        const int offsetY = (desktopRect.bottom - (windowRect.bottom - windowRect.top)) / 2;

        windowRect.left += offsetX;
        windowRect.right += offsetX;
        windowRect.top += offsetY;
        windowRect.bottom += offsetY;
    }

    window->hWnd = CreateWindowExA(dwExStyle,                           // Extended style for the window
                                   APPLICATION_NAME,                    // Class name
                                   WINDOW_TITLE,                        // Window title
                                   dwStyle |                            // Defined window style
                                       WS_CLIPSIBLINGS |                // Required window style
                                       WS_CLIPCHILDREN,                 // Required window style
                                   windowRect.left,                     // Window X position
                                   windowRect.top,                      // Window Y position
                                   windowRect.right - windowRect.left,  // Window width
                                   windowRect.bottom - windowRect.top,  // Window height
                                   NULL,                                // No parent window
                                   NULL,                                // No menu
                                   window->hInstance,                   // Instance
                                   NULL);                               // No WM_CREATE parameter
    if (!window->hWnd) {
        ksGpuWindow_Destroy(window);
        Error("Failed to create window.");
        return false;
    }

    SetWindowLongPtrA(window->hWnd, GWLP_USERDATA, (LONG_PTR)window);

    window->hDC = GetDC(window->hWnd);
    if (!window->hDC) {
        ksGpuWindow_Destroy(window);
        Error("Failed to acquire device context.");
        return false;
    }

    ksGpuDevice_Create(&window->device, instance, queueInfo);
    ksGpuContext_CreateForSurface(&window->context, &window->device, queueIndex, colorFormat, depthFormat, sampleCount,
                                  window->hInstance, window->hDC);
    ksGpuContext_SetCurrent(&window->context);

    ShowWindow(window->hWnd, SW_SHOW);
    SetForegroundWindow(window->hWnd);
    SetFocus(window->hWnd);

    return true;
}

void ksGpuWindow_SwapBuffers(ksGpuWindow *window) {
    SwapBuffers(window->context.hDC);
    ksNanoseconds newTimeNanoseconds = GetTimeNanoseconds();
    const float frameTimeNanoseconds = 1000.0f * 1000.0f * 1000.0f / window->windowRefreshRate;
    const float deltaTimeNanoseconds = (float)newTimeNanoseconds - window->lastSwapTime - frameTimeNanoseconds;
    if (fabsf(deltaTimeNanoseconds) < frameTimeNanoseconds * 0.75f) {
        newTimeNanoseconds = (ksNanoseconds)(window->lastSwapTime + frameTimeNanoseconds + 0.025f * deltaTimeNanoseconds);
    }
    window->lastSwapTime = newTimeNanoseconds;
}






























































struct Cube {
    XrPosef Pose;
    XrVector3f Scale;
};

void device_init(); //function protocols for openxr_init() to see
void opengl_render_layer(const XrCompositionLayerProjectionView& layerView, const XrSwapchainImageOpenGLKHR* swapchainImage, const std::vector<Cube>& cubes, int index);

struct swapchain_t {
	XrSwapchain handle;
	int32_t width;
	int32_t height;
	vector<XrSwapchainImageOpenGLKHR> surface_images;
};

struct input_state_t {
	XrActionSet actionSet;
	XrAction    poseAction;
	XrAction    selectAction;
	XrPath   handSubactionPath[2];
	XrSpace  handSpace[2];
	XrPosef  handPose[2];
	XrBool32 renderHand[2];
	XrBool32 handSelect[2];
    std::array<XrBool32, 2> handActive;
};

PFN_xrGetOpenGLGraphicsRequirementsKHR ext_xrGetOpenGLGraphicsRequirementsKHR = nullptr;
PFN_xrCreateDebugUtilsMessengerEXT     ext_xrCreateDebugUtilsMessengerEXT     = nullptr;
PFN_xrDestroyDebugUtilsMessengerEXT    ext_xrDestroyDebugUtilsMessengerEXT    = nullptr;

XrFormFactor            app_config_form = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

vector<XrPosef> app_cubes;

const XrPosef  xr_pose_identity = { {0,0,0,1}, {0,0,0} };
XrInstance     xr_instance      = {};
XrSession      xr_session       = {};
XrSessionState xr_session_state = XR_SESSION_STATE_UNKNOWN;
bool           xr_running       = false;
XrSpace        xr_app_space     = {};
XrSystemId     xr_system_id     = XR_NULL_SYSTEM_ID;
input_state_t  xr_input         = { };
XrEnvironmentBlendMode   xr_blend = {};
XrDebugUtilsMessengerEXT xr_debug = {};
XrSpaceLocation xr_fixed_space = {};

vector<XrView>                  xr_views;
vector<XrViewConfigurationView> xr_config_views;
vector<swapchain_t>             xr_swapchains;

int64_t gl_swapchain_fmt = GL_RGBA8;
XrGraphicsBindingOpenGLWin32KHR m_graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
std::vector<XrSpace> m_visualizedSpaces;

namespace Side {
    const int LEFT = 0;
    const int RIGHT = 1;
    const int COUNT = 2;
}

namespace Math {
    namespace Pose {
        XrPosef Identity() {
            XrPosef t{};
            t.orientation.w = 1;
            return t;
        }

        XrPosef Translation(const XrVector3f& translation) {
            XrPosef t = Identity();
            t.position = translation;
            return t;
        }

        XrPosef RotateCCWAboutYAxis(float radians, XrVector3f translation) {
            XrPosef t = Identity();
            t.orientation.x = 0.f;
            t.orientation.y = std::sin(radians * 0.5f);
            t.orientation.z = 0.f;
            t.orientation.w = std::cos(radians * 0.5f);
            t.position = translation;
            return t;
        }
    }
}

inline bool EqualsIgnoreCase(const std::string& s1, const std::string& s2, const std::locale& loc = std::locale()) {
    const std::ctype<char>& ctype = std::use_facet<std::ctype<char>>(loc);
    const auto compareCharLower = [&](char c1, char c2) { return ctype.tolower(c1) == ctype.tolower(c2); };
    return s1.size() == s2.size() && std::equal(s1.begin(), s1.end(), s2.begin(), compareCharLower);
}

XrReferenceSpaceCreateInfo GetXrReferenceSpaceCreateInfo(const std::string& referenceSpaceTypeStr) {
    XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Identity();
    if (EqualsIgnoreCase(referenceSpaceTypeStr, "View")) {
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "ViewFront")) { // Render head-locked 2m in front of device.
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::Translation({0.f, 0.f, -2.f}),
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Local")) {
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Stage")) {
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeft")) {
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {-2.f, 0.f, -2.f});
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRight")) {
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {2.f, 0.f, -2.f});
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageLeftRotated")) {
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(3.14f / 3.f, {-2.f, 0.5f, -2.f});
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "StageRightRotated")) {
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(-3.14f / 3.f, {2.f, 0.5f, -2.f});
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    } else if (EqualsIgnoreCase(referenceSpaceTypeStr, "Custom")) {
        referenceSpaceCreateInfo.poseInReferenceSpace = Math::Pose::RotateCCWAboutYAxis(0.f, {-.3f, 1.5f, -1.f});
        referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    } else {
        throw std::logic_error("Unknown reference space type " + referenceSpaceTypeStr);
    }
    return referenceSpaceCreateInfo;
}

bool openxr_init(const char *app_name, int64_t swapchain_format) {
	vector<const char*> use_extensions;
	const char         *ask_extensions[] = { 
		XR_KHR_OPENGL_ENABLE_EXTENSION_NAME,
		XR_EXT_DEBUG_UTILS_EXTENSION_NAME,
	};

	uint32_t ext_count = 0;
	xrEnumerateInstanceExtensionProperties(nullptr, 0, &ext_count, nullptr);
	vector<XrExtensionProperties> xr_exts(ext_count, { XR_TYPE_EXTENSION_PROPERTIES });
	xrEnumerateInstanceExtensionProperties(nullptr, ext_count, &ext_count, xr_exts.data());

	printf("OpenXR extensions available:\n");
	for (size_t i = 0; i < xr_exts.size(); i++) {
		printf("- %s\n", xr_exts[i].extensionName);

		for (int32_t ask = 0; ask < _countof(ask_extensions); ask++) {
			if (strcmp(ask_extensions[ask], xr_exts[i].extensionName) == 0) {
				use_extensions.push_back(ask_extensions[ask]);
				break;
			}
		}
	}

	if (!std::any_of( use_extensions.begin(), use_extensions.end(), 
		[] (const char *ext) {
			return strcmp(ext, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME)==0;
		}))
		return false;

	XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
	createInfo.enabledExtensionCount      = use_extensions.size();
	createInfo.enabledExtensionNames      = use_extensions.data();
	createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	strcpy_s(createInfo.applicationInfo.applicationName, app_name);
	xrCreateInstance(&createInfo, &xr_instance);

	if (xr_instance == nullptr)
		return false;

	xrGetInstanceProcAddr(xr_instance, "xrCreateDebugUtilsMessengerEXT",     (PFN_xrVoidFunction *)(&ext_xrCreateDebugUtilsMessengerEXT   ));
	xrGetInstanceProcAddr(xr_instance, "xrDestroyDebugUtilsMessengerEXT",    (PFN_xrVoidFunction *)(&ext_xrDestroyDebugUtilsMessengerEXT  ));
	xrGetInstanceProcAddr(xr_instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction *)(&ext_xrGetOpenGLGraphicsRequirementsKHR));

	XrDebugUtilsMessengerCreateInfoEXT debug_info = { XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debug_info.messageTypes =
		XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
		XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
		XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
	debug_info.messageSeverities =
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_info.userCallback = [](XrDebugUtilsMessageSeverityFlagsEXT severity, XrDebugUtilsMessageTypeFlagsEXT types, const XrDebugUtilsMessengerCallbackDataEXT *msg, void* user_data) {
		printf("%s: %s\n", msg->functionName, msg->message);
		char text[512];
		sprintf_s(text, "%s: %s", msg->functionName, msg->message);
		OutputDebugStringA(text);

		return (XrBool32)XR_FALSE;
	};

	if (ext_xrCreateDebugUtilsMessengerEXT)
		ext_xrCreateDebugUtilsMessengerEXT(xr_instance, &debug_info, &xr_debug);
	
	XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
	systemInfo.formFactor = app_config_form;
	xrGetSystem(xr_instance, &systemInfo, &xr_system_id);

	uint32_t blend_count = 0;
	xrEnumerateEnvironmentBlendModes(xr_instance, xr_system_id, app_config_view, 1, &blend_count, &xr_blend);

	XrGraphicsRequirementsOpenGLKHR requirement = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
	ext_xrGetOpenGLGraphicsRequirementsKHR(xr_instance, xr_system_id, &requirement);

    device_init();

	XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
	sessionInfo.next     = &m_graphicsBinding;
	sessionInfo.systemId = xr_system_id;
	xrCreateSession(xr_instance, &sessionInfo, &xr_session);

	if (xr_session == nullptr)
		return false;

    //std::string visualizedSpaces[] = {"ViewFront", "Local", "Stage", "StageLeft", "StageRight", "StageLeftRotated", "StageRightRotated"};
    std::string visualizedSpaces[] = {"Custom"};
    for (const auto& visualizedSpace : visualizedSpaces) {
        XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo(visualizedSpace);
        XrSpace space;
        XrResult res = xrCreateReferenceSpace(xr_session, &referenceSpaceCreateInfo, &space);
        if (XR_SUCCEEDED(res)) {
            m_visualizedSpaces.push_back(space);
        } else {
            printf("Failed to create reference space %s with error %d", visualizedSpace.c_str(), res);
        }
    }
	XrReferenceSpaceCreateInfo ref_space = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	ref_space.poseInReferenceSpace = xr_pose_identity;
	ref_space.referenceSpaceType   = XR_REFERENCE_SPACE_TYPE_LOCAL;
	xrCreateReferenceSpace(xr_session, &ref_space, &xr_app_space);

	uint32_t view_count = 0;
	xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, app_config_view, 0, &view_count, nullptr);
	xr_config_views.resize(view_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
	xr_views       .resize(view_count, { XR_TYPE_VIEW });
	xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, app_config_view, view_count, &view_count, xr_config_views.data());

	for (uint32_t i = 0; i < view_count; i++) {
		XrViewConfigurationView &view = xr_config_views[i];
		XrSwapchainCreateInfo swapchain_info = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
		XrSwapchain handle;
		swapchain_info.arraySize   = 1;
		swapchain_info.mipCount    = 1;
		swapchain_info.faceCount   = 1;
		swapchain_info.format      = swapchain_format;
		swapchain_info.width       = view.recommendedImageRectWidth;
		swapchain_info.height      = view.recommendedImageRectHeight;
		swapchain_info.sampleCount = view.recommendedSwapchainSampleCount;
		swapchain_info.usageFlags  = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		xrCreateSwapchain(xr_session, &swapchain_info, &handle);

		uint32_t surface_count = 0;
		xrEnumerateSwapchainImages(handle, 0, &surface_count, nullptr);

		swapchain_t swapchain = {};
		swapchain.width  = swapchain_info.width;
		swapchain.height = swapchain_info.height;
		swapchain.handle = handle;
		swapchain.surface_images.resize(surface_count, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR } );
		xrEnumerateSwapchainImages(swapchain.handle, surface_count, &surface_count, (XrSwapchainImageBaseHeader*)swapchain.surface_images.data());
		xr_swapchains.push_back(swapchain);
	}

	return true;
}

void openxr_make_actions() {
	XrActionSetCreateInfo actionset_info = { XR_TYPE_ACTION_SET_CREATE_INFO };
	strcpy_s(actionset_info.actionSetName,          "gameplay");
	strcpy_s(actionset_info.localizedActionSetName, "Gameplay");
	xrCreateActionSet(xr_instance, &actionset_info, &xr_input.actionSet);
	xrStringToPath(xr_instance, "/user/hand/left",  &xr_input.handSubactionPath[0]);
	xrStringToPath(xr_instance, "/user/hand/right", &xr_input.handSubactionPath[1]);

	XrActionCreateInfo action_info = { XR_TYPE_ACTION_CREATE_INFO };
	action_info.countSubactionPaths = _countof(xr_input.handSubactionPath);
	action_info.subactionPaths      = xr_input.handSubactionPath;
	action_info.actionType          = XR_ACTION_TYPE_POSE_INPUT;
	strcpy_s(action_info.actionName,          "hand_pose");
	strcpy_s(action_info.localizedActionName, "Hand Pose");
	xrCreateAction(xr_input.actionSet, &action_info, &xr_input.poseAction);

	action_info.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
	strcpy_s(action_info.actionName,          "select");
	strcpy_s(action_info.localizedActionName, "Select");
	xrCreateAction(xr_input.actionSet, &action_info, &xr_input.selectAction);

	XrPath profile_path;
	XrPath pose_path  [2];
	XrPath select_path[2];
	xrStringToPath(xr_instance, "/user/hand/left/input/grip/pose",     &pose_path[0]);
	xrStringToPath(xr_instance, "/user/hand/right/input/grip/pose",    &pose_path[1]);
	xrStringToPath(xr_instance, "/user/hand/left/input/select/click",  &select_path[0]);
	xrStringToPath(xr_instance, "/user/hand/right/input/select/click", &select_path[1]);
	xrStringToPath(xr_instance, "/interaction_profiles/khr/simple_controller", &profile_path);
	XrActionSuggestedBinding bindings[] = {
		{ xr_input.poseAction,   pose_path[0]   },
		{ xr_input.poseAction,   pose_path[1]   },
		{ xr_input.selectAction, select_path[0] },
		{ xr_input.selectAction, select_path[1] }, };
	XrInteractionProfileSuggestedBinding suggested_binds = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
	suggested_binds.interactionProfile     = profile_path;
	suggested_binds.suggestedBindings      = &bindings[0];
	suggested_binds.countSuggestedBindings = _countof(bindings);
	xrSuggestInteractionProfileBindings(xr_instance, &suggested_binds);

	for (int32_t i = 0; i < 2; i++) {
		XrActionSpaceCreateInfo action_space_info = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
		action_space_info.action            = xr_input.poseAction;
		action_space_info.poseInActionSpace = xr_pose_identity;
		action_space_info.subactionPath     = xr_input.handSubactionPath[i];
		xrCreateActionSpace(xr_session, &action_space_info, &xr_input.handSpace[i]);
	}

	XrSessionActionSetsAttachInfo attach_info = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
	attach_info.countActionSets = 1;
	attach_info.actionSets      = &xr_input.actionSet;
	xrAttachSessionActionSets(xr_session, &attach_info);
}

void openxr_poll_events(bool &exit) {
	exit = false;

	XrEventDataBuffer event_buffer = { XR_TYPE_EVENT_DATA_BUFFER };

	while (xrPollEvent(xr_instance, &event_buffer) == XR_SUCCESS) {
		switch (event_buffer.type) {
		case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
			XrEventDataSessionStateChanged *changed = (XrEventDataSessionStateChanged*)&event_buffer;
			xr_session_state = changed->state;

			switch (xr_session_state) {
			    case XR_SESSION_STATE_READY: {
				    XrSessionBeginInfo begin_info = { XR_TYPE_SESSION_BEGIN_INFO };
				    begin_info.primaryViewConfigurationType = app_config_view;
				    xrBeginSession(xr_session, &begin_info);
				    xr_running = true;
			    } break;
			    case XR_SESSION_STATE_STOPPING: {
				    xr_running = false;
				    xrEndSession(xr_session); 
			    } break;
			    case XR_SESSION_STATE_EXITING:      exit = true;              break;
			    case XR_SESSION_STATE_LOSS_PENDING: exit = true;              break;
			}
		} break;
		case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: exit = true; return;
		}
		event_buffer = { XR_TYPE_EVENT_DATA_BUFFER };
	}
}

void openxr_poll_actions() {
	if (xr_session_state != XR_SESSION_STATE_FOCUSED)
		return;

    xr_input.handActive = {XR_FALSE, XR_FALSE};

	XrActiveActionSet action_set = { };
	action_set.actionSet     = xr_input.actionSet;
	action_set.subactionPath = XR_NULL_PATH;

	XrActionsSyncInfo sync_info = { XR_TYPE_ACTIONS_SYNC_INFO };
	sync_info.countActiveActionSets = 1;
	sync_info.activeActionSets      = &action_set;

	xrSyncActions(xr_session, &sync_info);

	for (uint32_t hand = 0; hand < 2; hand++) {
		XrActionStateGetInfo get_info = { XR_TYPE_ACTION_STATE_GET_INFO };
		get_info.subactionPath = xr_input.handSubactionPath[hand];

		XrActionStatePose pose_state = { XR_TYPE_ACTION_STATE_POSE };
		get_info.action = xr_input.poseAction;
		xrGetActionStatePose(xr_session, &get_info, &pose_state);
		xr_input.renderHand[hand] = pose_state.isActive;
        xr_input.handActive[hand] = pose_state.isActive;

		XrActionStateBoolean select_state = { XR_TYPE_ACTION_STATE_BOOLEAN };
		get_info.action = xr_input.selectAction;
		xrGetActionStateBoolean(xr_session, &get_info, &select_state);
		xr_input.handSelect[hand] = select_state.currentState && select_state.changedSinceLastSync;

		if (xr_input.handSelect[hand]) {
			XrSpaceLocation space_location = { XR_TYPE_SPACE_LOCATION };
			XrResult res = xrLocateSpace(xr_input.handSpace[hand], xr_app_space, select_state.lastChangeTime, &space_location);
			if (XR_UNQUALIFIED_SUCCESS(res) &&
				(space_location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT   ) != 0 &&
				(space_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
				xr_input.handPose[hand] = space_location.pose;
			}
		}
	}
}

bool openxr_render_layer(XrTime predictedDisplayTime, vector<XrCompositionLayerProjectionView> &views, XrCompositionLayerProjection &layer) {
	uint32_t view_count  = 0;
	XrViewState view_state  = { XR_TYPE_VIEW_STATE };
	XrViewLocateInfo locate_info = { XR_TYPE_VIEW_LOCATE_INFO };
	locate_info.viewConfigurationType = app_config_view;
	locate_info.space = xr_app_space;
	xrLocateViews(xr_session, &locate_info, &view_state, (uint32_t)xr_views.size(), &view_count, xr_views.data());
	views.resize(view_count);

    std::vector<Cube> cubes;
    XrResult res;
    for (XrSpace visualizedSpace : m_visualizedSpaces) {
        XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
        res = xrLocateSpace(visualizedSpace, xr_app_space, predictedDisplayTime, &spaceLocation);
        if (XR_UNQUALIFIED_SUCCESS(res)) {
            if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
                cubes.push_back(Cube{spaceLocation.pose, {0.25f, 0.25f, 0.25f}});
            }
            xr_fixed_space = spaceLocation;
        } else {
            printf("Unable to locate a visualized reference space in app space: %d", res);
        }
    }

    for (auto hand : {Side::LEFT, Side::RIGHT}) {
        XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
        res = xrLocateSpace(xr_input.handSpace[hand], xr_app_space, predictedDisplayTime, &spaceLocation);
        if (XR_UNQUALIFIED_SUCCESS(res)) {
            if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
                cubes.push_back(Cube{spaceLocation.pose, {0.1f, 0.1f, 0.1f}});
            }
        } else {
            if (xr_input.handActive[hand] == XR_TRUE) {
                const char* handName[] = {"left", "right"};
                printf("Unable to locate %s hand action space in app space: %d", handName[hand], res);
            }
        }
    }

	for (uint32_t i = 0; i < view_count; i++) {
		uint32_t img_id;
		XrSwapchainImageAcquireInfo acquire_info = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
		xrAcquireSwapchainImage(xr_swapchains[i].handle, &acquire_info, &img_id);

		XrSwapchainImageWaitInfo wait_info = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
		wait_info.timeout = XR_INFINITE_DURATION;
		xrWaitSwapchainImage(xr_swapchains[i].handle, &wait_info);

		views[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
		views[i].pose = xr_views[i].pose;
		views[i].fov  = xr_views[i].fov;
		views[i].subImage.swapchain = xr_swapchains[i].handle;
		views[i].subImage.imageRect.offset = { 0, 0 };
		views[i].subImage.imageRect.extent = { xr_swapchains[i].width, xr_swapchains[i].height };

        const XrSwapchainImageOpenGLKHR* const swapchainImage = &xr_swapchains[i].surface_images[img_id];
        opengl_render_layer(views[i], swapchainImage, cubes, i);

		XrSwapchainImageReleaseInfo release_info = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
		xrReleaseSwapchainImage(xr_swapchains[i].handle, &release_info);
	}

	layer.space     = xr_app_space;
	layer.viewCount = (uint32_t)views.size();
	layer.views     = views.data();
	return true;
}

void openxr_render_frame() {
	XrFrameState frame_state = { XR_TYPE_FRAME_STATE };
	xrWaitFrame (xr_session, nullptr, &frame_state);
	xrBeginFrame(xr_session, nullptr);

	XrCompositionLayerBaseHeader            *layer      = nullptr;
	XrCompositionLayerProjection             layer_proj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	vector<XrCompositionLayerProjectionView> views;
	bool session_active = xr_session_state == XR_SESSION_STATE_VISIBLE || xr_session_state == XR_SESSION_STATE_FOCUSED;

	if (session_active && openxr_render_layer(frame_state.predictedDisplayTime, views, layer_proj)) {
		layer = (XrCompositionLayerBaseHeader*)&layer_proj;
	}

	XrFrameEndInfo end_info{ XR_TYPE_FRAME_END_INFO };
	end_info.displayTime          = frame_state.predictedDisplayTime;
	end_info.environmentBlendMode = xr_blend;
	end_info.layerCount           = layer == nullptr ? 0 : 1;
	end_info.layers               = &layer;
	xrEndFrame(xr_session, &end_info);
}

void openxr_shutdown() {
	for (int32_t i = 0; i < xr_swapchains.size(); i++) {
		xrDestroySwapchain(xr_swapchains[i].handle);
	}
	xr_swapchains.clear();
    
    for (XrSpace visualizedSpace : m_visualizedSpaces) {
        xrDestroySpace(visualizedSpace);
    }

	if (xr_input.actionSet != XR_NULL_HANDLE) {
		if (xr_input.handSpace[0] != XR_NULL_HANDLE) xrDestroySpace(xr_input.handSpace[0]);
		if (xr_input.handSpace[1] != XR_NULL_HANDLE) xrDestroySpace(xr_input.handSpace[1]);
		xrDestroyActionSet(xr_input.actionSet);
	}
    
	if (xr_app_space != XR_NULL_HANDLE) xrDestroySpace   (xr_app_space);
	if (xr_session   != XR_NULL_HANDLE) xrDestroySession (xr_session);
	if (xr_debug     != XR_NULL_HANDLE) ext_xrDestroyDebugUtilsMessengerEXT(xr_debug);
	if (xr_instance  != XR_NULL_HANDLE) xrDestroyInstance(xr_instance);
}














ksGpuWindow window{};

void device_init() {
        PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
        xrGetInstanceProcAddr(xr_instance, "xrGetOpenGLGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLGraphicsRequirementsKHR));

        XrGraphicsRequirementsOpenGLKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
        pfnGetOpenGLGraphicsRequirementsKHR(xr_instance, xr_system_id, &graphicsRequirements);

        ksDriverInstance driverInstance{};
        ksGpuQueueInfo queueInfo{};
        ksGpuSurfaceColorFormat colorFormat{KS_GPU_SURFACE_COLOR_FORMAT_B8G8R8A8};
        ksGpuSurfaceDepthFormat depthFormat{KS_GPU_SURFACE_DEPTH_FORMAT_D24};
        ksGpuSampleCount sampleCount{KS_GPU_SAMPLE_COUNT_1};
        if (!ksGpuWindow_Create(&window, &driverInstance, &queueInfo, 0, colorFormat, depthFormat, sampleCount, 640, 480, false)) {
            throw std::logic_error("Unable to create GL context");
        }

        GLint major = 0;
        GLint minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);

        const XrVersion desiredApiVersion = XR_MAKE_VERSION(major, minor, 0);
        if (graphicsRequirements.minApiVersionSupported > desiredApiVersion) {
            throw std::logic_error("Runtime does not support desired Graphics API and/or version");
        }

        m_graphicsBinding.hDC = window.context.hDC;
        m_graphicsBinding.hGLRC = window.context.hGLRC;
        glEnable(GL_DEBUG_OUTPUT); //may need to import code for gl debugging
}



GLuint m_swapchainFramebuffer{0};
GLuint m_program{0};
GLint m_modelViewProjectionUniformLocation{0};
GLint m_vertexAttribCoords{0};
GLint m_vertexAttribColor{0};
GLuint m_vao_plane{0};
GLuint m_vao_cube{0};
GLuint m_vao_sphere{0};
GLuint m_cubeVertexBuffer{0};
GLuint m_cubeIndexBuffer{0};
GLuint m_planeVertexBuffer{0};
GLuint m_planeIndexBuffer{0};
GLuint m_sphereVertexBuffer{0};
GLuint m_sphereIndexBuffer{0};
std::map<uint32_t, uint32_t> m_colorToDepthMap;

constexpr float DarkSlateGray[] = {0.184313729f, 0.309803933f, 0.309803933f, 1.0f};
static const char* VertexShaderGlsl = R"_(
    #version 410
    in vec3 VertexPos;
    in vec3 VertexColor;
    out vec3 PSVertexColor;
    uniform mat4 ModelViewProjection;
    void main() {
       gl_Position = ModelViewProjection * vec4(VertexPos, 1.0);
       PSVertexColor = VertexColor;
    }
    )_";
static const char* FragmentShaderGlsl = R"_(
    #version 410
    in vec3 PSVertexColor;
    out vec4 FragColor;
    void main() {
       FragColor = vec4(PSVertexColor, 1);
    }
    )_";

namespace Geometry {
    struct Vertex {
        XrVector3f Position;
        XrVector3f Color;
    };

    constexpr XrVector3f Black{0, 0, 0};
    constexpr XrVector3f White{1, 1, 1};
    constexpr XrVector3f Grey{.5f, .5f, .5f};
    constexpr XrVector3f Red{1, 0, 0};
    constexpr XrVector3f MidRed{0.5f, 0, 0};
    constexpr XrVector3f DarkRed{0.25f, 0, 0};
    constexpr XrVector3f Green{0, 1, 0};
    constexpr XrVector3f MidGreen{0, 0.5f, 0};
    constexpr XrVector3f DarkGreen{0, 0.25f, 0};
    constexpr XrVector3f Blue{0, 0, 1};
    constexpr XrVector3f MidBlue{0, 0, 0.5f};
    constexpr XrVector3f DarkBlue{0, 0, 0.25f};
    constexpr XrVector3f Yellow{1.f, 1.f, 0.f};
    constexpr XrVector3f Cyan{0.f, 1.f, 1.f};
    constexpr XrVector3f Purple{1.f, 0.f, 1.f};

    constexpr XrVector3f LBB{-0.5f, -0.5f, -0.5f};
    constexpr XrVector3f LBF{-0.5f, -0.5f, 0.5f};
    constexpr XrVector3f LTB{-0.5f, 0.5f, -0.5f};
    constexpr XrVector3f LTF{-0.5f, 0.5f, 0.5f};
    constexpr XrVector3f RBB{0.5f, -0.5f, -0.5f};
    constexpr XrVector3f RBF{0.5f, -0.5f, 0.5f};
    constexpr XrVector3f RTB{0.5f, 0.5f, -0.5f};
    constexpr XrVector3f RTF{0.5f, 0.5f, 0.5f};

    #define CUBE_SIDE(V1, V2, V3, V4, V5, V6, COLOR) {V1, COLOR}, {V2, COLOR}, {V3, COLOR}, {V4, COLOR}, {V5, COLOR}, {V6, COLOR},

    Vertex c_cubeVertices[] = {
        {LTB, DarkRed}, {LBF, DarkRed}, {LBB, DarkRed}, {LTB, DarkRed}, {LTF, DarkRed}, {LBF, DarkRed},
        {RTB, Red}, {RBB, Red}, {RBF, Red}, {RTB, Red}, {RBF, Red}, {RTF, Red},
        {LBB, MidGreen}, {LBF, MidGreen}, {RBF, MidGreen}, {LBB, MidGreen}, {RBF, MidGreen}, {RBB, MidGreen},
        {LTB, Green}, {RTB, Green}, {RTF, Green}, {LTB, Green}, {RTF, Green}, {LTF, Green},
        {LBB, DarkBlue}, {RBB, DarkBlue}, {RTB, DarkBlue}, {LBB, DarkBlue}, {RTB, DarkBlue}, {LTB, DarkBlue},
        {LBF, Blue}, {LTF, Blue}, {RTF, Blue}, {LBF, Blue}, {RTF, Blue}, {RBF, Blue}
    };
    unsigned short c_cubeIndices[] = {
        0,  1,  2,  3,  4,  5, 6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
    };

    Vertex c_planeVertices[] = { CUBE_SIDE(LTB, RTB, RTF, LTB, RTF, LTF, DarkGreen) };
    unsigned short c_planeIndices[] = { 0,  1,  2,  3,  4,  5 };

    Vertex c_sphereVertices[] = {
        {{-0.577350, -0.577350, -0.577350},Red}, {{0.577350, -0.577350, -0.577350},Red}, {{0.577350, 0.577350, -0.577350},Red}, {{-0.577350, 0.577350, -0.577350},Red}, {{-0.577350, -0.577350, 0.577350},Green}, {{0.577350, -0.577350, 0.577350},Green}, {{0.577350, 0.577350, 0.577350},Green}, {{-0.577350, 0.577350, 0.577350},Green}, {{-0.577350, -0.577350, -0.577350},Blue}, {{-0.577350, 0.577350, -0.577350},Blue}, {{-0.577350, 0.577350, 0.577350},Blue}, {{-0.577350, -0.577350, 0.577350},Blue}, {{0.577350, -0.577350, -0.577350},Yellow}, {{0.577350, 0.577350, -0.577350},Yellow}, {{0.577350, 0.577350, 0.577350},Yellow}, {{0.577350, -0.577350, 0.577350},Yellow}, {{-0.577350, -0.577350, -0.577350},Cyan}, {{0.577350, -0.577350, -0.577350},Cyan}, {{0.577350, -0.577350, 0.577350},Cyan}, {{-0.577350, -0.577350, 0.577350},Cyan}, {{-0.577350, 0.577350, -0.577350},Purple}, {{0.577350, 0.577350, -0.577350},Purple}, {{0.577350, 0.577350, 0.577350},Purple}, {{-0.577350, 0.577350, 0.577350},Purple}, {{0.000000, 0.000000, -1.000000},Red}, {Blue,Green}, {{-1.000000, 0.000000, 0.000000},Blue}, {Red,Yellow}, {{0.000000, -1.000000, 0.000000},Cyan}, {Green,Purple}, {{0.707107, 0.000000, -0.707107},Red}, {{0.000000, -0.707107, -0.707107},Red}, {{-0.707107, 0.000000, -0.707107},Red}, {{0.000000, 0.707107, -0.707107},Red}, {{0.000000, -0.707107, 0.707107},Green}, {{0.707107, 0.000000, 0.707107},Green}, {{0.000000, 0.707107, 0.707107},Green}, {{-0.707107, 0.000000, 0.707107},Green}, {{-0.707107, 0.707107, 0.000000},Blue}, {{-0.707107, 0.000000, -0.707107},Blue}, {{-0.707107, -0.707107, 0.000000},Blue}, {{-0.707107, 0.000000, 0.707107},Blue}, {{0.707107, 0.000000, -0.707107},Yellow}, {{0.707107, 0.707107, 0.000000},Yellow}, {{0.707107, 0.000000, 0.707107},Yellow}, {{0.707107, -0.707107, 0.000000},Yellow}, {{0.707107, -0.707107, 0.000000},Cyan}, {{0.000000, -0.707107, -0.707107},Cyan}, {{-0.707107, -0.707107, 0.000000},Cyan}, {{0.000000, -0.707107, 0.707107},Cyan}, {{0.000000, 0.707107, -0.707107},Purple}, {{0.707107, 0.707107, 0.000000},Purple}, {{0.000000, 0.707107, 0.707107},Purple}, {{-0.707107, 0.707107, 0.000000},Purple}, {{0.325058, -0.325058, -0.888074},Red}, {{0.325058, 0.325058, -0.888074},Red}, {{-0.325058, -0.325058, -0.888074},Red}, {{-0.325058, 0.325058, -0.888074},Red}, {{0.325058, -0.325058, 0.888074},Green}, {{-0.325058, -0.325058, 0.888074},Green}, {{0.325058, 0.325058, 0.888074},Green}, {{-0.325058, 0.325058, 0.888074},Green}, {{-0.888074, 0.325058, -0.325058},Blue}, {{-0.888074, 0.325058, 0.325058},Blue}, {{-0.888074, -0.325058, -0.325058},Blue}, {{-0.888074, -0.325058, 0.325058},Blue}, {{0.888074, 0.325058, -0.325058},Yellow}, {{0.888074, -0.325058, -0.325058},Yellow}, {{0.888074, 0.325058, 0.325058},Yellow}, {{0.888074, -0.325058, 0.325058},Yellow}, {{0.325058, -0.888074, -0.325058},Cyan}, {{0.325058, -0.888074, 0.325058},Cyan}, {{-0.325058, -0.888074, -0.325058},Cyan}, {{-0.325058, -0.888074, 0.325058},Cyan}, {{0.325058, 0.888074, -0.325058},Purple}, {{-0.325058, 0.888074, -0.325058},Purple}, {{0.325058, 0.888074, 0.325058},Purple}, {{-0.325058, 0.888074, 0.325058},Purple}, {{0.382683, 0.000000, -0.923880},Red}, {{0.673887, -0.302905, -0.673887},Red}, {{0.673887, 0.302905, -0.673887},Red}, {{0.000000, -0.382683, -0.923880},Red}, {{-0.302905, -0.673887, -0.673887},Red}, {{0.302905, -0.673887, -0.673887},Red}, {{-0.382683, 0.000000, -0.923880},Red}, {{-0.673887, 0.302905, -0.673887},Red}, {{-0.673887, -0.302905, -0.673887},Red}, {{0.000000, 0.382683, -0.923880},Red}, {{0.302905, 0.673887, -0.673887},Red}, {{-0.302905, 0.673887, -0.673887},Red}, {{0.000000, -0.382683, 0.923880},Green}, {{0.302905, -0.673887, 0.673887},Green}, {{-0.302905, -0.673887, 0.673887},Green}, {{0.382683, 0.000000, 0.923880},Green}, {{0.673887, 0.302905, 0.673887},Green}, {{0.673887, -0.302905, 0.673887},Green}, {{0.000000, 0.382683, 0.923880},Green}, {{-0.302905, 0.673887, 0.673887},Green}, {{0.302905, 0.673887, 0.673887},Green}, {{-0.382683, 0.000000, 0.923880},Green}, {{-0.673887, -0.302905, 0.673887},Green}, {{-0.673887, 0.302905, 0.673887},Green}, {{-0.923880, 0.382683, 0.000000},Blue}, {{-0.673887, 0.673887, -0.302905},Blue}, {{-0.673887, 0.673887, 0.302905},Blue}, {{-0.923880, 0.000000, -0.382683},Blue}, {{-0.673887, -0.302905, -0.673887},Blue}, {{-0.673887, 0.302905, -0.673887},Blue}, {{-0.923880, -0.382683, 0.000000},Blue}, {{-0.673887, -0.673887, 0.302905},Blue}, {{-0.673887, -0.673887, -0.302905},Blue}, {{-0.923880, 0.000000, 0.382683},Blue}, {{-0.673887, 0.302905, 0.673887},Blue}, {{-0.673887, -0.302905, 0.673887},Blue}, {{0.923880, 0.000000, -0.382683},Yellow}, {{0.673887, 0.302905, -0.673887},Yellow}, {{0.673887, -0.302905, -0.673887},Yellow}, {{0.923880, 0.382683, 0.000000},Yellow}, {{0.673887, 0.673887, 0.302905},Yellow}, {{0.673887, 0.673887, -0.302905},Yellow}, {{0.923880, 0.000000, 0.382683},Yellow}, {{0.673887, -0.302905, 0.673887},Yellow}, {{0.673887, 0.302905, 0.673887},Yellow}, {{0.923880, -0.382683, 0.000000},Yellow}, {{0.673887, -0.673887, -0.302905},Yellow}, {{0.673887, -0.673887, 0.302905},Yellow}, {{0.382683, -0.923880, 0.000000},Cyan}, {{0.673887, -0.673887, -0.302905},Cyan}, {{0.673887, -0.673887, 0.302905},Cyan}, {{0.000000, -0.923880, -0.382683},Cyan}, {{-0.302905, -0.673887, -0.673887},Cyan}, {{0.302905, -0.673887, -0.673887},Cyan}, {{-0.382683, -0.923880, 0.000000},Cyan}, {{-0.673887, -0.673887, 0.302905},Cyan}, {{-0.673887, -0.673887, -0.302905},Cyan}, {{0.000000, -0.923880, 0.382683},Cyan}, {{0.302905, -0.673887, 0.673887},Cyan}, {{-0.302905, -0.673887, 0.673887},Cyan}, {{0.000000, 0.923880, -0.382683},Purple}, {{0.302905, 0.673887, -0.673887},Purple}, {{-0.302905, 0.673887, -0.673887},Purple}, {{0.382683, 0.923880, 0.000000},Purple}, {{0.673887, 0.673887, 0.302905},Purple}, {{0.673887, 0.673887, -0.302905},Purple}, {{0.000000, 0.923880, 0.382683},Purple}, {{-0.302905, 0.673887, 0.673887},Purple}, {{0.302905, 0.673887, 0.673887},Purple}, {{-0.382683, 0.923880, 0.000000},Purple}, {{-0.673887, 0.673887, -0.302905},Purple}, {{-0.673887, 0.673887, 0.302905},Purple}, {{0.535467, -0.168634, -0.827549},Red}, {{0.167277, -0.167277, -0.971616},Red}, {{0.464385, -0.464385, -0.754117},Red}, {{0.535467, 0.168634, -0.827549},Red}, {{0.464385, 0.464385, -0.754117},Red}, {{0.167277, 0.167277, -0.971616},Red}, {{-0.168634, -0.535467, -0.827549},Red}, {{-0.167277, -0.167277, -0.971616},Red}, {{-0.464385, -0.464385, -0.754117},Red}, {{0.168634, -0.535467, -0.827549},Red}, {{-0.535467, 0.168634, -0.827549},Red}, {{-0.167277, 0.167277, -0.971616},Red}, {{-0.464385, 0.464385, -0.754117},Red}, {{-0.535467, -0.168634, -0.827549},Red}, {{0.168634, 0.535467, -0.827549},Red}, {{-0.168634, 0.535467, -0.827549},Red}, {{0.168634, -0.535467, 0.827549},Green}, {{0.167277, -0.167277, 0.971616},Green}, {{0.464385, -0.464385, 0.754117},Green}, {{-0.168634, -0.535467, 0.827549},Green}, {{-0.464385, -0.464385, 0.754117},Green}, {{-0.167277, -0.167277, 0.971616},Green}, {{0.535467, 0.168634, 0.827549},Green}, {{0.167277, 0.167277, 0.971616},Green}, {{0.464385, 0.464385, 0.754117},Green}, {{0.535467, -0.168634, 0.827549},Green}, {{-0.168634, 0.535467, 0.827549},Green}, {{-0.167277, 0.167277, 0.971616},Green}, {{-0.464385, 0.464385, 0.754117},Green}, {{0.168634, 0.535467, 0.827549},Green}, {{-0.535467, -0.168634, 0.827549},Green}, {{-0.535467, 0.168634, 0.827549},Green}, {{-0.827549, 0.535467, -0.168634},Blue}, {{-0.971616, 0.167277, -0.167277},Blue}, {{-0.754117, 0.464385, -0.464385},Blue}, {{-0.827549, 0.535467, 0.168634},Blue}, {{-0.754117, 0.464385, 0.464385},Blue}, {{-0.971616, 0.167277, 0.167277},Blue}, {{-0.827549, -0.168634, -0.535467},Blue}, {{-0.971616, -0.167277, -0.167277},Blue}, {{-0.754117, -0.464385, -0.464385},Blue}, {{-0.827549, 0.168634, -0.535467},Blue}, {{-0.827549, -0.535467, 0.168634},Blue}, {{-0.971616, -0.167277, 0.167277},Blue}, {{-0.754117, -0.464385, 0.464385},Blue}, {{-0.827549, -0.535467, -0.168634},Blue}, {{-0.827549, 0.168634, 0.535467},Blue}, {{-0.827549, -0.168634, 0.535467},Blue}, {{0.827549, 0.168634, -0.535467},Yellow}, {{0.971616, 0.167277, -0.167277},Yellow}, {{0.754117, 0.464385, -0.464385},Yellow}, {{0.827549, -0.168634, -0.535467},Yellow}, {{0.754117, -0.464385, -0.464385},Yellow}, {{0.971616, -0.167277, -0.167277},Yellow}, {{0.827549, 0.535467, 0.168634},Yellow}, {{0.971616, 0.167277, 0.167277},Yellow}, {{0.754117, 0.464385, 0.464385},Yellow}, {{0.827549, 0.535467, -0.168634},Yellow}, {{0.827549, -0.168634, 0.535467},Yellow}, {{0.971616, -0.167277, 0.167277},Yellow}, {{0.754117, -0.464385, 0.464385},Yellow}, {{0.827549, 0.168634, 0.535467},Yellow}, {{0.827549, -0.535467, -0.168634},Yellow}, {{0.827549, -0.535467, 0.168634},Yellow}, {{0.535467, -0.827549, -0.168634},Cyan}, {{0.167277, -0.971616, -0.167277},Cyan}, {{0.464385, -0.754117, -0.464385},Cyan}, {{0.535467, -0.827549, 0.168634},Cyan}, {{0.464385, -0.754117, 0.464385},Cyan}, {{0.167277, -0.971616, 0.167277},Cyan}, {{-0.168634, -0.827549, -0.535467},Cyan}, {{-0.167277, -0.971616, -0.167277},Cyan}, {{-0.464385, -0.754117, -0.464385},Cyan}, {{0.168634, -0.827549, -0.535467},Cyan}, {{-0.535467, -0.827549, 0.168634},Cyan}, {{-0.167277, -0.971616, 0.167277},Cyan}, {{-0.464385, -0.754117, 0.464385},Cyan}, {{-0.535467, -0.827549, -0.168634},Cyan}, {{0.168634, -0.827549, 0.535467},Cyan}, {{-0.168634, -0.827549, 0.535467},Cyan}, {{0.168634, 0.827549, -0.535467},Purple}, {{0.167277, 0.971616, -0.167277},Purple}, {{0.464385, 0.754117, -0.464385},Purple}, {{-0.168634, 0.827549, -0.535467},Purple}, {{-0.464385, 0.754117, -0.464385},Purple}, {{-0.167277, 0.971616, -0.167277},Purple}, {{0.535467, 0.827549, 0.168634},Purple}, {{0.167277, 0.971616, 0.167277},Purple}, {{0.464385, 0.754117, 0.464385},Purple}, {{0.535467, 0.827549, -0.168634},Purple}, {{-0.168634, 0.827549, 0.535467},Purple}, {{-0.167277, 0.971616, 0.167277},Purple}, {{-0.464385, 0.754117, 0.464385},Purple}, {{0.168634, 0.827549, 0.535467},Purple}, {{-0.535467, 0.827549, -0.168634},Purple}, {{-0.535467, 0.827549, 0.168634},Purple}, {{0.358851, -0.164816, -0.918728},Red}, {{0.555570, 0.000000, -0.831470},Red}, {{0.195090, 0.000000, -0.980785},Red}, {{0.510307, -0.320792, -0.797922},Red}, {{0.633099, -0.445390, -0.633099},Red}, {{0.698753, -0.153263, -0.698753},Red}, {{0.510307, 0.320792, -0.797922},Red}, {{0.698753, 0.153263, -0.698753},Red}, {{0.633099, 0.445390, -0.633099},Red}, {{0.358851, 0.164816, -0.918728},Red}, {{-0.164816, -0.358851, -0.918728},Red}, {{0.000000, -0.555570, -0.831470},Red}, {{0.000000, -0.195090, -0.980785},Red}, {{-0.320792, -0.510307, -0.797922},Red}, {{-0.445390, -0.633099, -0.633099},Red}, {{-0.153263, -0.698753, -0.698753},Red}, {{0.320792, -0.510307, -0.797922},Red}, {{0.153263, -0.698753, -0.698753},Red}, {{0.445390, -0.633099, -0.633099},Red}, {{0.164816, -0.358851, -0.918728},Red}, {{-0.358851, 0.164816, -0.918728},Red}, {{-0.555570, 0.000000, -0.831470},Red}, {{-0.195090, 0.000000, -0.980785},Red}, {{-0.510307, 0.320792, -0.797922},Red}, {{-0.633099, 0.445390, -0.633099},Red}, {{-0.698753, 0.153263, -0.698753},Red}, {{-0.510307, -0.320792, -0.797922},Red}, {{-0.698753, -0.153263, -0.698753},Red}, {{-0.633099, -0.445390, -0.633099},Red}, {{-0.358851, -0.164816, -0.918728},Red}, {{0.164816, 0.358851, -0.918728},Red}, {{0.000000, 0.555570, -0.831470},Red}, {{0.000000, 0.195090, -0.980785},Red}, {{0.320792, 0.510307, -0.797922},Red}, {{0.445390, 0.633099, -0.633099},Red}, {{0.153263, 0.698753, -0.698753},Red}, {{-0.320792, 0.510307, -0.797922},Red}, {{-0.153263, 0.698753, -0.698753},Red}, {{-0.445390, 0.633099, -0.633099},Red}, {{-0.164816, 0.358851, -0.918728},Red}, {{0.164816, -0.358851, 0.918728},Green}, {{0.000000, -0.555570, 0.831470},Green}, {{0.000000, -0.195090, 0.980785},Green}, {{0.320792, -0.510307, 0.797922},Green}, {{0.445390, -0.633099, 0.633099},Green}, {{0.153263, -0.698753, 0.698753},Green}, {{-0.320792, -0.510307, 0.797922},Green}, {{-0.153263, -0.698753, 0.698753},Green}, {{-0.445390, -0.633099, 0.633099},Green}, {{-0.164816, -0.358851, 0.918728},Green}, {{0.358851, 0.164816, 0.918728},Green}, {{0.555570, 0.000000, 0.831470},Green}, {{0.195090, 0.000000, 0.980785},Green}, {{0.510307, 0.320792, 0.797922},Green}, {{0.633099, 0.445390, 0.633099},Green}, {{0.698753, 0.153263, 0.698753},Green}, {{0.510307, -0.320792, 0.797922},Green}, {{0.698753, -0.153263, 0.698753},Green}, {{0.633099, -0.445390, 0.633099},Green}, {{0.358851, -0.164816, 0.918728},Green}, {{-0.164816, 0.358851, 0.918728},Green}, {{0.000000, 0.555570, 0.831470},Green}, {{0.000000, 0.195090, 0.980785},Green}, {{-0.320792, 0.510307, 0.797922},Green}, {{-0.445390, 0.633099, 0.633099},Green}, {{-0.153263, 0.698753, 0.698753},Green}, {{0.320792, 0.510307, 0.797922},Green}, {{0.153263, 0.698753, 0.698753},Green}, {{0.445390, 0.633099, 0.633099},Green}, {{0.164816, 0.358851, 0.918728},Green}, {{-0.358851, -0.164816, 0.918728},Green}, {{-0.555570, 0.000000, 0.831470},Green}, {{-0.195090, 0.000000, 0.980785},Green}, {{-0.510307, -0.320792, 0.797922},Green}, {{-0.633099, -0.445390, 0.633099},Green}, {{-0.698753, -0.153263, 0.698753},Green}, {{-0.510307, 0.320792, 0.797922},Green}, {{-0.698753, 0.153263, 0.698753},Green}, {{-0.633099, 0.445390, 0.633099},Green}, {{-0.358851, 0.164816, 0.918728},Green}, {{-0.918728, 0.358851, -0.164816},Blue}, {{-0.831470, 0.555570, 0.000000},Blue}, {{-0.980785, 0.195090, 0.000000},Blue}, {{-0.797922, 0.510307, -0.320792},Blue}, {{-0.633099, 0.633099, -0.445390},Blue}, {{-0.698753, 0.698753, -0.153263},Blue}, {{-0.797922, 0.510307, 0.320792},Blue}, {{-0.698753, 0.698753, 0.153263},Blue}, {{-0.633099, 0.633099, 0.445390},Blue}, {{-0.918728, 0.358851, 0.164816},Blue}, {{-0.918728, -0.164816, -0.358851},Blue}, {{-0.831470, 0.000000, -0.555570},Blue}, {{-0.980785, 0.000000, -0.195090},Blue}, {{-0.797922, -0.320792, -0.510307},Blue}, {{-0.633099, -0.445390, -0.633099},Blue}, {{-0.698753, -0.153263, -0.698753},Blue}, {{-0.797922, 0.320792, -0.510307},Blue}, {{-0.698753, 0.153263, -0.698753},Blue}, {{-0.633099, 0.445390, -0.633099},Blue}, {{-0.918728, 0.164816, -0.358851},Blue}, {{-0.918728, -0.358851, 0.164816},Blue}, {{-0.831470, -0.555570, 0.000000},Blue}, {{-0.980785, -0.195090, 0.000000},Blue}, {{-0.797922, -0.510307, 0.320792},Blue}, {{-0.633099, -0.633099, 0.445390},Blue}, {{-0.698753, -0.698753, 0.153263},Blue}, {{-0.797922, -0.510307, -0.320792},Blue}, {{-0.698753, -0.698753, -0.153263},Blue}, {{-0.633099, -0.633099, -0.445390},Blue}, {{-0.918728, -0.358851, -0.164816},Blue}, {{-0.918728, 0.164816, 0.358851},Blue}, {{-0.831470, 0.000000, 0.555570},Blue}, {{-0.980785, 0.000000, 0.195090},Blue}, {{-0.797922, 0.320792, 0.510307},Blue}, {{-0.633099, 0.445390, 0.633099},Blue}, {{-0.698753, 0.153263, 0.698753},Blue}, {{-0.797922, -0.320792, 0.510307},Blue}, {{-0.698753, -0.153263, 0.698753},Blue}, {{-0.633099, -0.445390, 0.633099},Blue}, {{-0.918728, -0.164816, 0.358851},Blue}, {{0.918728, 0.164816, -0.358851},Yellow}, {{0.831470, 0.000000, -0.555570},Yellow}, {{0.980785, 0.000000, -0.195090},Yellow}, {{0.797922, 0.320792, -0.510307},Yellow}, {{0.633099, 0.445390, -0.633099},Yellow}, {{0.698753, 0.153263, -0.698753},Yellow}, {{0.797922, -0.320792, -0.510307},Yellow}, {{0.698753, -0.153263, -0.698753},Yellow}, {{0.633099, -0.445390, -0.633099},Yellow}, {{0.918728, -0.164816, -0.358851},Yellow}, {{0.918728, 0.358851, 0.164816},Yellow}, {{0.831470, 0.555570, 0.000000},Yellow}, {{0.980785, 0.195090, 0.000000},Yellow}, {{0.797922, 0.510307, 0.320792},Yellow}, {{0.633099, 0.633099, 0.445390},Yellow}, {{0.698753, 0.698753, 0.153263},Yellow}, {{0.797922, 0.510307, -0.320792},Yellow}, {{0.698753, 0.698753, -0.153263},Yellow}, {{0.633099, 0.633099, -0.445390},Yellow}, {{0.918728, 0.358851, -0.164816},Yellow}, {{0.918728, -0.164816, 0.358851},Yellow}, {{0.831470, 0.000000, 0.555570},Yellow}, {{0.980785, 0.000000, 0.195090},Yellow}, {{0.797922, -0.320792, 0.510307},Yellow}, {{0.633099, -0.445390, 0.633099},Yellow}, {{0.698753, -0.153263, 0.698753},Yellow}, {{0.797922, 0.320792, 0.510307},Yellow}, {{0.698753, 0.153263, 0.698753},Yellow}, {{0.633099, 0.445390, 0.633099},Yellow}, {{0.918728, 0.164816, 0.358851},Yellow}, {{0.918728, -0.358851, -0.164816},Yellow}, {{0.831470, -0.555570, 0.000000},Yellow}, {{0.980785, -0.195090, 0.000000},Yellow}, {{0.797922, -0.510307, -0.320792},Yellow}, {{0.633099, -0.633099, -0.445390},Yellow}, {{0.698753, -0.698753, -0.153263},Yellow}, {{0.797922, -0.510307, 0.320792},Yellow}, {{0.698753, -0.698753, 0.153263},Yellow}, {{0.633099, -0.633099, 0.445390},Yellow}, {{0.918728, -0.358851, 0.164816},Yellow}, {{0.358851, -0.918728, -0.164816},Cyan}, {{0.555570, -0.831470, 0.000000},Cyan}, {{0.195090, -0.980785, 0.000000},Cyan}, {{0.510307, -0.797922, -0.320792},Cyan}, {{0.633099, -0.633099, -0.445390},Cyan}, {{0.698753, -0.698753, -0.153263},Cyan}, {{0.510307, -0.797922, 0.320792},Cyan}, {{0.698753, -0.698753, 0.153263},Cyan}, {{0.633099, -0.633099, 0.445390},Cyan}, {{0.358851, -0.918728, 0.164816},Cyan}, {{-0.164816, -0.918728, -0.358851},Cyan}, {{0.000000, -0.831470, -0.555570},Cyan}, {{0.000000, -0.980785, -0.195090},Cyan}, {{-0.320792, -0.797922, -0.510307},Cyan}, {{-0.445390, -0.633099, -0.633099},Cyan}, {{-0.153263, -0.698753, -0.698753},Cyan}, {{0.320792, -0.797922, -0.510307},Cyan}, {{0.153263, -0.698753, -0.698753},Cyan}, {{0.445390, -0.633099, -0.633099},Cyan}, {{0.164816, -0.918728, -0.358851},Cyan}, {{-0.358851, -0.918728, 0.164816},Cyan}, {{-0.555570, -0.831470, 0.000000},Cyan}, {{-0.195090, -0.980785, 0.000000},Cyan}, {{-0.510307, -0.797922, 0.320792},Cyan}, {{-0.633099, -0.633099, 0.445390},Cyan}, {{-0.698753, -0.698753, 0.153263},Cyan}, {{-0.510307, -0.797922, -0.320792},Cyan}, {{-0.698753, -0.698753, -0.153263},Cyan}, {{-0.633099, -0.633099, -0.445390},Cyan}, {{-0.358851, -0.918728, -0.164816},Cyan}, {{0.164816, -0.918728, 0.358851},Cyan}, {{0.000000, -0.831470, 0.555570},Cyan}, {{0.000000, -0.980785, 0.195090},Cyan}, {{0.320792, -0.797922, 0.510307},Cyan}, {{0.445390, -0.633099, 0.633099},Cyan}, {{0.153263, -0.698753, 0.698753},Cyan}, {{-0.320792, -0.797922, 0.510307},Cyan}, {{-0.153263, -0.698753, 0.698753},Cyan}, {{-0.445390, -0.633099, 0.633099},Cyan}, {{-0.164816, -0.918728, 0.358851},Cyan}, {{0.164816, 0.918728, -0.358851},Purple}, {{0.000000, 0.831470, -0.555570},Purple}, {{0.000000, 0.980785, -0.195090},Purple}, {{0.320792, 0.797922, -0.510307},Purple}, {{0.445390, 0.633099, -0.633099},Purple}, {{0.153263, 0.698753, -0.698753},Purple}, {{-0.320792, 0.797922, -0.510307},Purple}, {{-0.153263, 0.698753, -0.698753},Purple}, {{-0.445390, 0.633099, -0.633099},Purple}, {{-0.164816, 0.918728, -0.358851},Purple}, {{0.358851, 0.918728, 0.164816},Purple}, {{0.555570, 0.831470, 0.000000},Purple}, {{0.195090, 0.980785, 0.000000},Purple}, {{0.510307, 0.797922, 0.320792},Purple}, {{0.633099, 0.633099, 0.445390},Purple}, {{0.698753, 0.698753, 0.153263},Purple}, {{0.510307, 0.797922, -0.320792},Purple}, {{0.698753, 0.698753, -0.153263},Purple}, {{0.633099, 0.633099, -0.445390},Purple}, {{0.358851, 0.918728, -0.164816},Purple}, {{-0.164816, 0.918728, 0.358851},Purple}, {{0.000000, 0.831470, 0.555570},Purple}, {{0.000000, 0.980785, 0.195090},Purple}, {{-0.320792, 0.797922, 0.510307},Purple}, {{-0.445390, 0.633099, 0.633099},Purple}, {{-0.153263, 0.698753, 0.698753},Purple}, {{0.320792, 0.797922, 0.510307},Purple}, {{0.153263, 0.698753, 0.698753},Purple}, {{0.445390, 0.633099, 0.633099},Purple}, {{0.164816, 0.918728, 0.358851},Purple}, {{-0.358851, 0.918728, -0.164816},Purple}, {{-0.555570, 0.831470, 0.000000},Purple}, {{-0.195090, 0.980785, 0.000000},Purple}, {{-0.510307, 0.797922, -0.320792},Purple}, {{-0.633099, 0.633099, -0.445390},Purple}, {{-0.698753, 0.698753, -0.153263},Purple}, {{-0.510307, 0.797922, 0.320792},Purple}, {{-0.698753, 0.698753, 0.153263},Purple}, {{-0.633099, 0.633099, 0.445390},Purple}, {{-0.358851, 0.918728, 0.164816},Purple}
    };
    unsigned short c_sphereIndices[] = {
        78, 246, 150, 150, 246, 54, 30, 247, 150, 150, 247, 78, 78, 248, 151, 151, 248, 24, 54, 246, 151, 151, 246, 78, 79, 249, 152, 152, 249, 54, 1, 250, 152, 152, 250, 79, 79, 251, 150, 150, 251, 30, 54, 249, 150, 150, 249, 79, 80, 252, 153, 153, 252, 55, 30, 253, 153, 153, 253, 80, 80, 254, 154, 154, 254, 2, 55, 252, 154, 154, 252, 80, 78, 255, 155, 155, 255, 55, 24, 248, 155, 155, 248, 78, 78, 247, 153, 153, 247, 30, 55, 255, 153, 153, 255, 78, 81, 256, 156, 156, 256, 56, 31, 257, 156, 156, 257, 81, 81, 258, 157, 157, 258, 24, 56, 256, 157, 157, 256, 81, 82, 259, 158, 158, 259, 56, 0, 260, 158, 158, 260, 82, 82, 261, 156, 156, 261, 31, 56, 259, 156, 156, 259, 82, 83, 262, 159, 159, 262, 54, 31, 263, 159, 159, 263, 83, 83, 264, 152, 152, 264, 1, 54, 262, 152, 152, 262, 83, 81, 265, 151, 151, 265, 54, 24, 258, 151, 151, 258, 81, 81, 257, 159, 159, 257, 31, 54, 265, 159, 159, 265, 81, 84, 266, 160, 160, 266, 57, 32, 267, 160, 160, 267, 84, 84, 268, 161, 161, 268, 24, 57, 266, 161, 161, 266, 84, 85, 269, 162, 162, 269, 57, 3, 270, 162, 162, 270, 85, 85, 271, 160, 160, 271, 32, 57, 269, 160, 160, 269, 85, 86, 272, 163, 163, 272, 56, 32, 273, 163, 163, 273, 86, 86, 274, 158, 158, 274, 0, 56, 272, 158, 158, 272, 86, 84, 275, 157, 157, 275, 56, 24, 268, 157, 157, 268, 84, 84, 267, 163, 163, 267, 32, 56, 275, 163, 163, 275, 84, 87, 276, 164, 164, 276, 55, 33, 277, 164, 164, 277, 87, 87, 278, 155, 155, 278, 24, 55, 276, 155, 155, 276, 87, 88, 279, 154, 154, 279, 55, 2, 280, 154, 154, 280, 88, 88, 281, 164, 164, 281, 33, 55, 279, 164, 164, 279, 88, 89, 282, 165, 165, 282, 57, 33, 283, 165, 165, 283, 89, 89, 284, 162, 162, 284, 3, 57, 282, 162, 162, 282, 89, 87, 285, 161, 161, 285, 57, 24, 278, 161, 161, 278, 87, 87, 277, 165, 165, 277, 33, 57, 285, 165, 165, 285, 87, 90, 286, 166, 166, 286, 58, 34, 287, 166, 166, 287, 90, 90, 288, 167, 167, 288, 25, 58, 286, 167, 167, 286, 90, 91, 289, 168, 168, 289, 58, 5, 290, 168, 168, 290, 91, 91, 291, 166, 166, 291, 34, 58, 289, 166, 166, 289, 91, 92, 292, 169, 169, 292, 59, 34, 293, 169, 169, 293, 92, 92, 294, 170, 170, 294, 4, 59, 292, 170, 170, 292, 92, 90, 295, 171, 171, 295, 59, 25, 288, 171, 171, 288, 90, 90, 287, 169, 169, 287, 34, 59, 295, 169, 169, 295, 90, 93, 296, 172, 172, 296, 60, 35, 297, 172, 172, 297, 93, 93, 298, 173, 173, 298, 25, 60, 296, 173, 173, 296, 93, 94, 299, 174, 174, 299, 60, 6, 300, 174, 174, 300, 94, 94, 301, 172, 172, 301, 35, 60, 299, 172, 172, 299, 94, 95, 302, 175, 175, 302, 58, 35, 303, 175, 175, 303, 95, 95, 304, 168, 168, 304, 5, 58, 302, 168, 168, 302, 95, 93, 305, 167, 167, 305, 58, 25, 298, 167, 167, 298, 93, 93, 297, 175, 175, 297, 35, 58, 305, 175, 175, 305, 93, 96, 306, 176, 176, 306, 61, 36, 307, 176, 176, 307, 96, 96, 308, 177, 177, 308, 25, 61, 306, 177, 177, 306, 96, 97, 309, 178, 178, 309, 61, 7, 310, 178, 178, 310, 97, 97, 311, 176, 176, 311, 36, 61, 309, 176, 176, 309, 97, 98, 312, 179, 179, 312, 60, 36, 313, 179, 179, 313, 98, 98, 314, 174, 174, 314, 6, 60, 312, 174, 174, 312, 98, 96, 315, 173, 173, 315, 60, 25, 308, 173, 173, 308, 96, 96, 307, 179, 179, 307, 36, 60, 315, 179, 179, 315, 96, 99, 316, 180, 180, 316, 59, 37, 317, 180, 180, 317, 99, 99, 318, 171, 171, 318, 25, 59, 316, 171, 171, 316, 99, 100, 319, 170, 170, 319, 59, 4, 320, 170, 170, 320, 100, 100, 321, 180, 180, 321, 37, 59, 319, 180, 180, 319, 100, 101, 322, 181, 181, 322, 61, 37, 323, 181, 181, 323, 101, 101, 324, 178, 178, 324, 7, 61, 322, 178, 178, 322, 101, 99, 325, 177, 177, 325, 61, 25, 318, 177, 177, 318, 99, 99, 317, 181, 181, 317, 37, 61, 325, 181, 181, 325, 99, 102, 326, 182, 182, 326, 62, 38, 327, 182, 182, 327, 102, 102, 328, 183, 183, 328, 26, 62, 326, 183, 183, 326, 102, 103, 329, 184, 184, 329, 62, 9, 330, 184, 184, 330, 103, 103, 331, 182, 182, 331, 38, 62, 329, 182, 182, 329, 103, 104, 332, 185, 185, 332, 63, 38, 333, 185, 185, 333, 104, 104, 334, 186, 186, 334, 10, 63, 332, 186, 186, 332, 104, 102, 335, 187, 187, 335, 63, 26, 328, 187, 187, 328, 102, 102, 327, 185, 185, 327, 38, 63, 335, 185, 185, 335, 102, 105, 336, 188, 188, 336, 64, 39, 337, 188, 188, 337, 105, 105, 338, 189, 189, 338, 26, 64, 336, 189, 189, 336, 105, 106, 339, 190, 190, 339, 64, 8, 340, 190, 190, 340, 106, 106, 341, 188, 188, 341, 39, 64, 339, 188, 188, 339, 106, 107, 342, 191, 191, 342, 62, 39, 343, 191, 191, 343, 107, 107, 344, 184, 184, 344, 9, 62, 342, 184, 184, 342, 107, 105, 345, 183, 183, 345, 62, 26, 338, 183, 183, 338, 105, 105, 337, 191, 191, 337, 39, 62, 345, 191, 191, 345, 105, 108, 346, 192, 192, 346, 65, 40, 347, 192, 192, 347, 108, 108, 348, 193, 193, 348, 26, 65, 346, 193, 193, 346, 108, 109, 349, 194, 194, 349, 65, 11, 350, 194, 194, 350, 109, 109, 351, 192, 192, 351, 40, 65, 349, 192, 192, 349, 109, 110, 352, 195, 195, 352, 64, 40, 353, 195, 195, 353, 110, 110, 354, 190, 190, 354, 8, 64, 352, 190, 190, 352, 110, 108, 355, 189, 189, 355, 64, 26, 348, 189, 189, 348, 108, 108, 347, 195, 195, 347, 40, 64, 355, 195, 195, 355, 108, 111, 356, 196, 196, 356, 63, 41, 357, 196, 196, 357, 111, 111, 358, 187, 187, 358, 26, 63, 356, 187, 187, 356, 111, 112, 359, 186, 186, 359, 63, 10, 360, 186, 186, 360, 112, 112, 361, 196, 196, 361, 41, 63, 359, 196, 196, 359, 112, 113, 362, 197, 197, 362, 65, 41, 363, 197, 197, 363, 113, 113, 364, 194, 194, 364, 11, 65, 362, 194, 194, 362, 113, 111, 365, 193, 193, 365, 65, 26, 358, 193, 193, 358, 111, 111, 357, 197, 197, 357, 41, 65, 365, 197, 197, 365, 111, 114, 366, 198, 198, 366, 66, 42, 367, 198, 198, 367, 114, 114, 368, 199, 199, 368, 27, 66, 366, 199, 199, 366, 114, 115, 369, 200, 200, 369, 66, 13, 370, 200, 200, 370, 115, 115, 371, 198, 198, 371, 42, 66, 369, 198, 198, 369, 115, 116, 372, 201, 201, 372, 67, 42, 373, 201, 201, 373, 116, 116, 374, 202, 202, 374, 12, 67, 372, 202, 202, 372, 116, 114, 375, 203, 203, 375, 67, 27, 368, 203, 203, 368, 114, 114, 367, 201, 201, 367, 42, 67, 375, 201, 201, 375, 114, 117, 376, 204, 204, 376, 68, 43, 377, 204, 204, 377, 117, 117, 378, 205, 205, 378, 27, 68, 376, 205, 205, 376, 117, 118, 379, 206, 206, 379, 68, 14, 380, 206, 206, 380, 118, 118, 381, 204, 204, 381, 43, 68, 379, 204, 204, 379, 118, 119, 382, 207, 207, 382, 66, 43, 383, 207, 207, 383, 119, 119, 384, 200, 200, 384, 13, 66, 382, 200, 200, 382, 119, 117, 385, 199, 199, 385, 66, 27, 378, 199, 199, 378, 117, 117, 377, 207, 207, 377, 43, 66, 385, 207, 207, 385, 117, 120, 386, 208, 208, 386, 69, 44, 387, 208, 208, 387, 120, 120, 388, 209, 209, 388, 27, 69, 386, 209, 209, 386, 120, 121, 389, 210, 210, 389, 69, 15, 390, 210, 210, 390, 121, 121, 391, 208, 208, 391, 44, 69, 389, 208, 208, 389, 121, 122, 392, 211, 211, 392, 68, 44, 393, 211, 211, 393, 122, 122, 394, 206, 206, 394, 14, 68, 392, 206, 206, 392, 122, 120, 395, 205, 205, 395, 68, 27, 388, 205, 205, 388, 120, 120, 387, 211, 211, 387, 44, 68, 395, 211, 211, 395, 120, 123, 396, 212, 212, 396, 67, 45, 397, 212, 212, 397, 123, 123, 398, 203, 203, 398, 27, 67, 396, 203, 203, 396, 123, 124, 399, 202, 202, 399, 67, 12, 400, 202, 202, 400, 124, 124, 401, 212, 212, 401, 45, 67, 399, 212, 212, 399, 124, 125, 402, 213, 213, 402, 69, 45, 403, 213, 213, 403, 125, 125, 404, 210, 210, 404, 15, 69, 402, 210, 210, 402, 125, 123, 405, 209, 209, 405, 69, 27, 398, 209, 209, 398, 123, 123, 397, 213, 213, 397, 45, 69, 405, 213, 213, 405, 123, 126, 406, 214, 214, 406, 70, 46, 407, 214, 214, 407, 126, 126, 408, 215, 215, 408, 28, 70, 406, 215, 215, 406, 126, 127, 409, 216, 216, 409, 70, 17, 410, 216, 216, 410, 127, 127, 411, 214, 214, 411, 46, 70, 409, 214, 214, 409, 127, 128, 412, 217, 217, 412, 71, 46, 413, 217, 217, 413, 128, 128, 414, 218, 218, 414, 18, 71, 412, 218, 218, 412, 128, 126, 415, 219, 219, 415, 71, 28, 408, 219, 219, 408, 126, 126, 407, 217, 217, 407, 46, 71, 415, 217, 217, 415, 126, 129, 416, 220, 220, 416, 72, 47, 417, 220, 220, 417, 129, 129, 418, 221, 221, 418, 28, 72, 416, 221, 221, 416, 129, 130, 419, 222, 222, 419, 72, 16, 420, 222, 222, 420, 130, 130, 421, 220, 220, 421, 47, 72, 419, 220, 220, 419, 130, 131, 422, 223, 223, 422, 70, 47, 423, 223, 223, 423, 131, 131, 424, 216, 216, 424, 17, 70, 422, 216, 216, 422, 131, 129, 425, 215, 215, 425, 70, 28, 418, 215, 215, 418, 129, 129, 417, 223, 223, 417, 47, 70, 425, 223, 223, 425, 129, 132, 426, 224, 224, 426, 73, 48, 427, 224, 224, 427, 132, 132, 428, 225, 225, 428, 28, 73, 426, 225, 225, 426, 132, 133, 429, 226, 226, 429, 73, 19, 430, 226, 226, 430, 133, 133, 431, 224, 224, 431, 48, 73, 429, 224, 224, 429, 133, 134, 432, 227, 227, 432, 72, 48, 433, 227, 227, 433, 134, 134, 434, 222, 222, 434, 16, 72, 432, 222, 222, 432, 134, 132, 435, 221, 221, 435, 72, 28, 428, 221, 221, 428, 132, 132, 427, 227, 227, 427, 48, 72, 435, 227, 227, 435, 132, 135, 436, 228, 228, 436, 71, 49, 437, 228, 228, 437, 135, 135, 438, 219, 219, 438, 28, 71, 436, 219, 219, 436, 135, 136, 439, 218, 218, 439, 71, 18, 440, 218, 218, 440, 136, 136, 441, 228, 228, 441, 49, 71, 439, 228, 228, 439, 136, 137, 442, 229, 229, 442, 73, 49, 443, 229, 229, 443, 137, 137, 444, 226, 226, 444, 19, 73, 442, 226, 226, 442, 137, 135, 445, 225, 225, 445, 73, 28, 438, 225, 225, 438, 135, 135, 437, 229, 229, 437, 49, 73, 445, 229, 229, 445, 135, 138, 446, 230, 230, 446, 74, 50, 447, 230, 230, 447, 138, 138, 448, 231, 231, 448, 29, 74, 446, 231, 231, 446, 138, 139, 449, 232, 232, 449, 74, 21, 450, 232, 232, 450, 139, 139, 451, 230, 230, 451, 50, 74, 449, 230, 230, 449, 139, 140, 452, 233, 233, 452, 75, 50, 453, 233, 233, 453, 140, 140, 454, 234, 234, 454, 20, 75, 452, 234, 234, 452, 140, 138, 455, 235, 235, 455, 75, 29, 448, 235, 235, 448, 138, 138, 447, 233, 233, 447, 50, 75, 455, 233, 233, 455, 138, 141, 456, 236, 236, 456, 76, 51, 457, 236, 236, 457, 141, 141, 458, 237, 237, 458, 29, 76, 456, 237, 237, 456, 141, 142, 459, 238, 238, 459, 76, 22, 460, 238, 238, 460, 142, 142, 461, 236, 236, 461, 51, 76, 459, 236, 236, 459, 142, 143, 462, 239, 239, 462, 74, 51, 463, 239, 239, 463, 143, 143, 464, 232, 232, 464, 21, 74, 462, 232, 232, 462, 143, 141, 465, 231, 231, 465, 74, 29, 458, 231, 231, 458, 141, 141, 457, 239, 239, 457, 51, 74, 465, 239, 239, 465, 141, 144, 466, 240, 240, 466, 77, 52, 467, 240, 240, 467, 144, 144, 468, 241, 241, 468, 29, 77, 466, 241, 241, 466, 144, 145, 469, 242, 242, 469, 77, 23, 470, 242, 242, 470, 145, 145, 471, 240, 240, 471, 52, 77, 469, 240, 240, 469, 145, 146, 472, 243, 243, 472, 76, 52, 473, 243, 243, 473, 146, 146, 474, 238, 238, 474, 22, 76, 472, 238, 238, 472, 146, 144, 475, 237, 237, 475, 76, 29, 468, 237, 237, 468, 144, 144, 467, 243, 243, 467, 52, 76, 475, 243, 243, 475, 144, 147, 476, 244, 244, 476, 75, 53, 477, 244, 244, 477, 147, 147, 478, 235, 235, 478, 29, 75, 476, 235, 235, 476, 147, 148, 479, 234, 234, 479, 75, 20, 480, 234, 234, 480, 148, 148, 481, 244, 244, 481, 53, 75, 479, 244, 244, 479, 148, 149, 482, 245, 245, 482, 77, 53, 483, 245, 245, 483, 149, 149, 484, 242, 242, 484, 23, 77, 482, 242, 242, 482, 149, 147, 485, 241, 241, 485, 77, 29, 478, 241, 241, 478, 147, 147, 477, 245, 245, 477, 53, 77, 485, 245, 245, 485, 147
    };
}

void CheckShader(GLuint shader) {
    GLint r = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &r);
    if (r == GL_FALSE) {
        GLchar msg[4096] = {};
        GLsizei length;
        glGetShaderInfoLog(shader, sizeof(msg), &length, msg);
        throw std::logic_error("Compile shader failed: " + string(msg));
    }
}

void CheckProgram(GLuint prog) {
    GLint r = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &r);
    if (r == GL_FALSE) {
        GLchar msg[4096] = {};
        GLsizei length;
        glGetProgramInfoLog(prog, sizeof(msg), &length, msg);
        throw std::logic_error("Link program failed: " + string(msg));
    }
}

void opengl_init() {
        glGenFramebuffers(1, &m_swapchainFramebuffer);

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &VertexShaderGlsl, nullptr);
        glCompileShader(vertexShader);
        CheckShader(vertexShader);

        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &FragmentShaderGlsl, nullptr);
        glCompileShader(fragmentShader);
        CheckShader(fragmentShader);

        m_program = glCreateProgram();
        glAttachShader(m_program, vertexShader);
        glAttachShader(m_program, fragmentShader);
        glLinkProgram(m_program);
        CheckProgram(m_program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        m_modelViewProjectionUniformLocation = glGetUniformLocation(m_program, "ModelViewProjection");

        m_vertexAttribCoords = glGetAttribLocation(m_program, "VertexPos");
        m_vertexAttribColor  = glGetAttribLocation(m_program, "VertexColor");

        glGenBuffers(1, &m_planeVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_planeVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Geometry::c_planeVertices), Geometry::c_planeVertices, GL_STATIC_DRAW);
        glGenBuffers(1, &m_planeIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_planeIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Geometry::c_planeIndices), Geometry::c_planeIndices, GL_STATIC_DRAW);
        glGenVertexArrays(1, &m_vao_plane);
        glBindVertexArray(m_vao_plane);
        glBindBuffer(GL_ARRAY_BUFFER, m_planeVertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_planeIndexBuffer);

        glEnableVertexAttribArray(m_vertexAttribCoords);
        glEnableVertexAttribArray(m_vertexAttribColor);

        glGenBuffers(1, &m_cubeVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Geometry::c_cubeVertices), Geometry::c_cubeVertices, GL_STATIC_DRAW);
        glGenBuffers(1, &m_cubeIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Geometry::c_cubeIndices), Geometry::c_cubeIndices, GL_STATIC_DRAW);
        glGenVertexArrays(1, &m_vao_cube);
        glBindVertexArray(m_vao_cube);
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);

        //glEnableVertexAttribArray(m_vertexAttribCoords);
        //glEnableVertexAttribArray(m_vertexAttribColor);

        glGenBuffers(1, &m_sphereVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_sphereVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Geometry::c_sphereVertices), Geometry::c_sphereVertices, GL_STATIC_DRAW);
        glGenBuffers(1, &m_sphereIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Geometry::c_sphereIndices), Geometry::c_sphereIndices, GL_STATIC_DRAW);
        glGenVertexArrays(1, &m_vao_sphere);
        glBindVertexArray(m_vao_sphere);
        glBindBuffer(GL_ARRAY_BUFFER, m_sphereVertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereIndexBuffer);
}

uint32_t GetDepthTexture(uint32_t colorTexture) {
    auto depthBufferIt = m_colorToDepthMap.find(colorTexture);
    if (depthBufferIt != m_colorToDepthMap.end()) {
        return depthBufferIt->second;
    }

    GLint width;
    GLint height;
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

    uint32_t depthTexture;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    m_colorToDepthMap.insert(std::make_pair(colorTexture, depthTexture));

    return depthTexture;
}

template <typename T, size_t Size> // The equivalent of C++17 std::size. A helper to get the dimension for an array.
constexpr size_t ArraySize(const T (&/*unused*/)[Size]) noexcept { return Size; }

void opengl_render_layer(const XrCompositionLayerProjectionView& layerView, const XrSwapchainImageOpenGLKHR* swapchainImage, const std::vector<Cube>& cubes, int index) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_swapchainFramebuffer);

        const uint32_t colorTexture = reinterpret_cast<const XrSwapchainImageOpenGLKHR*>(swapchainImage)->image;

        glViewport(static_cast<GLint>(layerView.subImage.imageRect.offset.x),
                   static_cast<GLint>(layerView.subImage.imageRect.offset.y),
                   static_cast<GLsizei>(layerView.subImage.imageRect.extent.width),
                   static_cast<GLsizei>(layerView.subImage.imageRect.extent.height));

        glFrontFace(GL_CW);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        const uint32_t depthTexture = GetDepthTexture(colorTexture);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

        glClearColor(DarkSlateGray[0], DarkSlateGray[1], DarkSlateGray[2], DarkSlateGray[3]);
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glUseProgram(m_program);

        const auto& pose = layerView.pose;
        XrMatrix4x4f proj;
        XrMatrix4x4f_CreateProjectionFov(&proj, GRAPHICS_OPENGL, layerView.fov, 0.05f, 100.0f);
        XrMatrix4x4f toView;
        XrVector3f scale{1.f, 1.f, 1.f};
        XrVector3f zero_vector{ 0.f, 0.f, 0.f};
        XrQuaternionf zero_quaternion{ 0.f, 0.f, 0.f, 0.f};

        XrMatrix4x4f_CreateTranslationRotationScale(&toView, &pose.position, &pose.orientation, &scale);
        XrMatrix4x4f view;
        XrMatrix4x4f_InvertRigidBody(&view, &toView);
        XrMatrix4x4f vp;
        XrMatrix4x4f_Multiply(&vp, &proj, &view);

        glBindVertexArray(m_vao_sphere);
        glBindVertexArray(m_vao_cube);
        glBindVertexArray(m_vao_plane); //This order needs to be reversed for some reason

        XrMatrix4x4f model;
        XrMatrix4x4f mvp;
        
        //plane
        XrVector3f plane_scale{5.f, 5.f, 5.f};
        XrVector3f plane_position{0.f, -4.f, 0.f};

        glBindBuffer(GL_ARRAY_BUFFER, m_planeVertexBuffer);
        glVertexAttribPointer(m_vertexAttribCoords, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), nullptr);
        glVertexAttribPointer(m_vertexAttribColor, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), reinterpret_cast<const void*>(sizeof(XrVector3f)));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_planeIndexBuffer);
        XrMatrix4x4f_CreateTranslationRotationScale(&model, &plane_position, &zero_quaternion, &plane_scale);
        XrMatrix4x4f_Multiply(&mvp, &vp, &model);
        glUniformMatrix4fv(m_modelViewProjectionUniformLocation, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&mvp));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ArraySize(Geometry::c_planeIndices)), GL_UNSIGNED_SHORT, nullptr);

        //cube
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
        glVertexAttribPointer(m_vertexAttribCoords, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), nullptr);
        glVertexAttribPointer(m_vertexAttribColor, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), reinterpret_cast<const void*>(sizeof(XrVector3f)));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);
        for (const Cube& cube : cubes) {
            XrMatrix4x4f_CreateTranslationRotationScale(&model, &cube.Pose.position, &cube.Pose.orientation, &cube.Scale);
            XrMatrix4x4f_Multiply(&mvp, &vp, &model);
            glUniformMatrix4fv(m_modelViewProjectionUniformLocation, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&mvp));
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ArraySize(Geometry::c_cubeIndices)), GL_UNSIGNED_SHORT, nullptr);
        }

        //sphere
        XrVector3f sphere_scale{.25f, .25f, .25f};
        XrVector3f sphere_position{.5f, 0.f, -1.f};
        glBindBuffer(GL_ARRAY_BUFFER, m_sphereVertexBuffer);
        glVertexAttribPointer(m_vertexAttribCoords, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), nullptr);
        glVertexAttribPointer(m_vertexAttribColor, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), reinterpret_cast<const void*>(sizeof(XrVector3f)));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereIndexBuffer);
        XrMatrix4x4f_CreateTranslationRotationScale(&model, &sphere_position, &zero_quaternion, &sphere_scale);
        XrMatrix4x4f_Multiply(&mvp, &vp, &model);
        glUniformMatrix4fv(m_modelViewProjectionUniformLocation, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&mvp));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ArraySize(Geometry::c_sphereIndices)), GL_UNSIGNED_SHORT, nullptr);

        glUseProgram(0);

        int32_t width = layerView.subImage.imageRect.extent.width;
        int32_t height = layerView.subImage.imageRect.extent.height;
        
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        if (index == 0) {
            glBlitFramebuffer(0, 0, width, height, 0, 0, window.windowWidth/2, window.windowHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        } else if (index == 1) {
            glBlitFramebuffer(0, 0, width, height, window.windowWidth/2, 0, window.windowWidth, window.windowHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        static int everyOther = 0;
        if ((everyOther++ & 1) != 0) {
            ksGpuWindow_SwapBuffers(&window);
        }
}

void opengl_shutdown() {
    glDeleteFramebuffers(1, &m_swapchainFramebuffer);
    glDeleteProgram(m_program);
    glDeleteVertexArrays(1, &m_vao_plane);
    glDeleteVertexArrays(1, &m_vao_cube);
    glDeleteVertexArrays(1, &m_vao_sphere);
    glDeleteBuffers(1, &m_planeVertexBuffer);
    glDeleteBuffers(1, &m_planeIndexBuffer);
    glDeleteBuffers(1, &m_cubeVertexBuffer);
    glDeleteBuffers(1, &m_cubeIndexBuffer);
    glDeleteBuffers(1, &m_sphereVertexBuffer);
    glDeleteBuffers(1, &m_sphereIndexBuffer);

    for (auto& colorToDepth : m_colorToDepthMap) {
        if (colorToDepth.second != 0) {
            glDeleteTextures(1, &colorToDepth.second);
        }
    }
}


























int main(int argc, char* argv[]) {
	if (!openxr_init("Single file OpenXR", gl_swapchain_fmt)) {
		opengl_shutdown();
		MessageBox(nullptr, "OpenXR initialization failed\n", "Error", 1);
		return 1;
	}

    static bool quit = false;
    auto exitPollingThread = std::thread{[] {
        printf("Press Enter key to shutdown...");
        (void)getchar();
        quit = true;
    }}; exitPollingThread.detach();

	openxr_make_actions();
	opengl_init();

	while (!quit) {
		openxr_poll_events(quit);

		if (xr_running) {
			openxr_poll_actions();
			openxr_render_frame();

			if (xr_session_state != XR_SESSION_STATE_VISIBLE && 
				xr_session_state != XR_SESSION_STATE_FOCUSED) {
				this_thread::sleep_for(chrono::milliseconds(250));
			}
		}
	}

	openxr_shutdown();
    opengl_shutdown();

	return 0;
}