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
GLuint m_vao{0};
GLuint m_cubeVertexBuffer{0};
GLuint m_cubeIndexBuffer{0};
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

    constexpr XrVector3f Red{1, 0, 0};
    constexpr XrVector3f DarkRed{0.25f, 0, 0};
    constexpr XrVector3f Green{0, 1, 0};
    constexpr XrVector3f DarkGreen{0, 0.25f, 0};
    constexpr XrVector3f Blue{0, 0, 1};
    constexpr XrVector3f DarkBlue{0, 0, 0.25f};

    constexpr XrVector3f LBB{-0.5f, -0.5f, -0.5f}; // Vertices for a 1x1x1 meter cube. (Left/Right, Top/Bottom, Front/Back)
    constexpr XrVector3f LBF{-0.5f, -0.5f, 0.5f};
    constexpr XrVector3f LTB{-0.5f, 0.5f, -0.5f};
    constexpr XrVector3f LTF{-0.5f, 0.5f, 0.5f};
    constexpr XrVector3f RBB{0.5f, -0.5f, -0.5f};
    constexpr XrVector3f RBF{0.5f, -0.5f, 0.5f};
    constexpr XrVector3f RTB{0.5f, 0.5f, -0.5f};
    constexpr XrVector3f RTF{0.5f, 0.5f, 0.5f};

    #define CUBE_SIDE(V1, V2, V3, V4, V5, V6, COLOR) {V1, COLOR}, {V2, COLOR}, {V3, COLOR}, {V4, COLOR}, {V5, COLOR}, {V6, COLOR},

    constexpr Vertex c_cubeVertices[] = {
        CUBE_SIDE(LTB, LBF, LBB, LTB, LTF, LBF, DarkRed)    // -X
        CUBE_SIDE(RTB, RBB, RBF, RTB, RBF, RTF, Red)        // +X
        CUBE_SIDE(LBB, LBF, RBF, LBB, RBF, RBB, DarkGreen)  // -Y
        CUBE_SIDE(LTB, RTB, RTF, LTB, RTF, LTF, Green)      // +Y
        CUBE_SIDE(LBB, RBB, RTB, LBB, RTB, LTB, DarkBlue)   // -Z
        CUBE_SIDE(LBF, LTF, RTF, LBF, RTF, RBF, Blue)       // +Z
    };

    constexpr unsigned short c_cubeIndices[] = { // Winding order is clockwise. Each side uses a different color.
        0,  1,  2,  3,  4,  5,   // -X
        6,  7,  8,  9,  10, 11,  // +X
        12, 13, 14, 15, 16, 17,  // -Y
        18, 19, 20, 21, 22, 23,  // +Y
        24, 25, 26, 27, 28, 29,  // -Z
        30, 31, 32, 33, 34, 35,  // +Z
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
        m_vertexAttribColor = glGetAttribLocation(m_program, "VertexColor");

        glGenBuffers(1, &m_cubeVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Geometry::c_cubeVertices), Geometry::c_cubeVertices, GL_STATIC_DRAW);

        glGenBuffers(1, &m_cubeIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Geometry::c_cubeIndices), Geometry::c_cubeIndices, GL_STATIC_DRAW);

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);
        glEnableVertexAttribArray(m_vertexAttribCoords);
        glEnableVertexAttribArray(m_vertexAttribColor);
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);
        glVertexAttribPointer(m_vertexAttribCoords, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), nullptr);
        glVertexAttribPointer(m_vertexAttribColor, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), reinterpret_cast<const void*>(sizeof(XrVector3f)));
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
        XrMatrix4x4f_CreateTranslationRotationScale(&toView, &pose.position, &pose.orientation, &scale);
        XrMatrix4x4f view;
        XrMatrix4x4f_InvertRigidBody(&view, &toView);
        XrMatrix4x4f vp;
        XrMatrix4x4f_Multiply(&vp, &proj, &view);

        glBindVertexArray(m_vao);

        for (const Cube& cube : cubes) {
            XrMatrix4x4f model;
            XrMatrix4x4f_CreateTranslationRotationScale(&model, &cube.Pose.position, &cube.Pose.orientation, &cube.Scale);
            XrMatrix4x4f mvp;
            XrMatrix4x4f_Multiply(&mvp, &vp, &model);
            glUniformMatrix4fv(m_modelViewProjectionUniformLocation, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&mvp));

            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ArraySize(Geometry::c_cubeIndices)), GL_UNSIGNED_SHORT, nullptr);
        }

        glBindVertexArray(0);
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
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_cubeVertexBuffer);
    glDeleteBuffers(1, &m_cubeIndexBuffer);

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