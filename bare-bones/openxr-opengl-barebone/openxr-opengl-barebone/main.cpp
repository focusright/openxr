#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#define OPENGL_VERSION_MAJOR 4
#define OPENGL_VERSION_MINOR 3

#if defined(XR_USE_PLATFORM_WIN32) && defined(XR_USE_GRAPHICS_API_OPENGL)

#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <common/xr_linear.h>
#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <array>
#include <cmath>

using namespace std;

HDC hDC;
HGLRC hGLRC;
HINSTANCE hInstance;
HWND hWnd;
const int windowWidth = 640;
const int windowHeight = 480;

PFNGLATTACHSHADERPROC             glAttachShader            ;
PFNGLBLITFRAMEBUFFERPROC          glBlitFramebuffer         ;
PFNGLBINDBUFFERPROC               glBindBuffer              ;
PFNGLBUFFERDATAPROC               glBufferData              ;
PFNGLBINDFRAMEBUFFERPROC          glBindFramebuffer         ;
PFNGLBINDVERTEXARRAYPROC          glBindVertexArray         ;
PFNGLCOMPILESHADERPROC            glCompileShader           ;
PFNGLCREATEPROGRAMPROC            glCreateProgram           ;
PFNGLCREATESHADERPROC             glCreateShader            ;
PFNGLDELETEBUFFERSPROC            glDeleteBuffers           ;
PFNGLDELETEFRAMEBUFFERSPROC       glDeleteFramebuffers      ;
PFNGLDELETEPROGRAMPROC            glDeleteProgram           ;
PFNGLDELETESHADERPROC             glDeleteShader            ;
PFNGLDELETEVERTEXARRAYSPROC       glDeleteVertexArrays      ;
PFNGLENABLEVERTEXATTRIBARRAYPROC  glEnableVertexAttribArray ;
PFNGLFRAMEBUFFERTEXTURE2DPROC     glFramebufferTexture2D    ;
PFNGLGETATTRIBLOCATIONPROC        glGetAttribLocation       ;
PFNGLGENBUFFERSPROC               glGenBuffers              ;
PFNGLGENFRAMEBUFFERSPROC          glGenFramebuffers         ;
PFNGLGENVERTEXARRAYSPROC          glGenVertexArrays         ;
PFNGLGETUNIFORMLOCATIONPROC       glGetUniformLocation      ;
PFNGLLINKPROGRAMPROC              glLinkProgram             ;
PFNGLSHADERSOURCEPROC             glShaderSource            ;
PFNGLTEXIMAGE2DMULTISAMPLEPROC    glTexImage2DMultisample   ;
PFNGLVERTEXATTRIBPOINTERPROC      glVertexAttribPointer     ;
PFNGLUNIFORMMATRIX4FVPROC         glUniformMatrix4fv        ;
PFNGLUSEPROGRAMPROC               glUseProgram              ;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

PROC GetExtension(const char *functionName) { return wglGetProcAddress(functionName); }

void init_opengl_extensions() {
    glAttachShader            = (PFNGLATTACHSHADERPROC)            GetExtension("glAttachShader"           );
    glBlitFramebuffer         = (PFNGLBLITFRAMEBUFFERPROC)         GetExtension("glBlitFramebuffer"        );
    glBindBuffer              = (PFNGLBINDBUFFERPROC)              GetExtension("glBindBuffer"             );
    glBufferData              = (PFNGLBUFFERDATAPROC)              GetExtension("glBufferData"             );
    glBindFramebuffer         = (PFNGLBINDFRAMEBUFFERPROC)         GetExtension("glBindFramebuffer"        );
    glBindVertexArray         = (PFNGLBINDVERTEXARRAYPROC)         GetExtension("glBindVertexArray"        );
    glCompileShader           = (PFNGLCOMPILESHADERPROC)           GetExtension("glCompileShader"          );
    glCreateProgram           = (PFNGLCREATEPROGRAMPROC)           GetExtension("glCreateProgram"          );
    glCreateShader            = (PFNGLCREATESHADERPROC)            GetExtension("glCreateShader"           );
    glDeleteBuffers           = (PFNGLDELETEBUFFERSPROC)           GetExtension("glDeleteBuffers"          );
    glDeleteFramebuffers      = (PFNGLDELETEFRAMEBUFFERSPROC)      GetExtension("glDeleteFramebuffers"     );
    glDeleteProgram           = (PFNGLDELETEPROGRAMPROC)           GetExtension("glDeleteProgram"          );
    glDeleteShader            = (PFNGLDELETESHADERPROC)            GetExtension("glDeleteShader"           );
    glDeleteVertexArrays      = (PFNGLDELETEVERTEXARRAYSPROC)      GetExtension("glDeleteVertexArrays"     );
    glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC) GetExtension("glEnableVertexAttribArray");
    glFramebufferTexture2D    = (PFNGLFRAMEBUFFERTEXTURE2DPROC)    GetExtension("glFramebufferTexture2D"   );
    glGetAttribLocation       = (PFNGLGETATTRIBLOCATIONPROC)       GetExtension("glGetAttribLocation"      );
    glGenBuffers              = (PFNGLGENBUFFERSPROC)              GetExtension("glGenBuffers"             );
    glGenFramebuffers         = (PFNGLGENFRAMEBUFFERSPROC)         GetExtension("glGenFramebuffers"        );
    glGenVertexArrays         = (PFNGLGENVERTEXARRAYSPROC)         GetExtension("glGenVertexArrays"        );
    glGetUniformLocation      = (PFNGLGETUNIFORMLOCATIONPROC)      GetExtension("glGetUniformLocation"     );
    glLinkProgram             = (PFNGLLINKPROGRAMPROC)             GetExtension("glLinkProgram"            );
    glShaderSource            = (PFNGLSHADERSOURCEPROC)            GetExtension("glShaderSource"           );
    glTexImage2DMultisample   = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)   GetExtension("glTexImage2DMultisample"  );
    glVertexAttribPointer     = (PFNGLVERTEXATTRIBPOINTERPROC)     GetExtension("glVertexAttribPointer"    );
    glUniformMatrix4fv        = (PFNGLUNIFORMMATRIX4FVPROC)        GetExtension("glUniformMatrix4fv"       );
    glUseProgram              = (PFNGLUSEPROGRAMPROC)              GetExtension("glUseProgram"             );
}

LRESULT APIENTRY wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcA(hWnd, message, wParam, lParam);
}

void create_app_context() {
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,  PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 24, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 };
    int pixelFormat = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, pixelFormat, &pfd);
    hGLRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hGLRC);
    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GetExtension("wglCreateContextAttribsARB");
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hGLRC);
    int contextAttribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, OPENGL_VERSION_MAJOR, WGL_CONTEXT_MINOR_VERSION_ARB, OPENGL_VERSION_MINOR, WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB, 0};
    hGLRC = wglCreateContextAttribsARB(hDC, NULL, contextAttribs);
    wglMakeCurrent(hDC, hGLRC);
    init_opengl_extensions();
}

void create_app_window() {
    hInstance = GetModuleHandleA(NULL);
    WNDCLASSA wc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC)wnd_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "app";
    RegisterClassA(&wc);
    DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    RECT windowRect;
    windowRect.left = (long)0;
    windowRect.right = (long)windowWidth;
    windowRect.top = (long)0;
    windowRect.bottom = (long)windowHeight;

    RECT desktopRect;
    GetWindowRect(GetDesktopWindow(), &desktopRect);
    const int offsetX = (desktopRect.right - (windowRect.right - windowRect.left)) / 2;
    const int offsetY = (desktopRect.bottom - (windowRect.bottom - windowRect.top)) / 2;
    windowRect.left += offsetX;
    windowRect.right += offsetX;
    windowRect.top += offsetY;
    windowRect.bottom += offsetY;

    hWnd = CreateWindowExA(dwExStyle, "app", "", 
                                   dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                   windowRect.left, windowRect.top,
                                   windowRect.right - windowRect.left,
                                   windowRect.bottom - windowRect.top,
                                   NULL, NULL, hInstance, NULL);
    hDC = GetDC(hWnd);
    create_app_context();
    wglMakeCurrent(hDC, hGLRC);
    ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);
}

void destroy_app_window() {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hGLRC);
    ReleaseDC(hWnd, hDC);
    DestroyWindow(hWnd);
    UnregisterClassA("app", hInstance);
}

void device_init();
void opengl_render_layer(const XrCompositionLayerProjectionView& layerView, const XrSwapchainImageOpenGLKHR* swapchainImage, int index);

struct swapchain_t {
	XrSwapchain handle;
	int32_t width;
	int32_t height;
	vector<XrSwapchainImageOpenGLKHR> surface_images;
};

PFN_xrGetOpenGLGraphicsRequirementsKHR ext_xrGetOpenGLGraphicsRequirementsKHR = nullptr;
XrViewConfigurationType app_config_view = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
XrGraphicsBindingOpenGLWin32KHR xr_graphicsBinding{XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR};
XrSessionState xr_session_state = XR_SESSION_STATE_UNKNOWN;
XrSystemId xr_system_id = XR_NULL_SYSTEM_ID;
const XrPosef xr_pose_identity = { {0,0,0,1}, {0,0,0} };
XrInstance xr_instance = {};
XrSession xr_session = {};
XrSpace xr_app_space = {};
XrEnvironmentBlendMode xr_blend = {};
vector<XrView> xr_views;
vector<XrViewConfigurationView> xr_config_views;
vector<swapchain_t> xr_swapchains;
bool xr_running = false;

bool openxr_init() {
	vector<const char*> use_extensions;
	const char *ask_extension = XR_KHR_OPENGL_ENABLE_EXTENSION_NAME;

	uint32_t ext_count = 0;
	xrEnumerateInstanceExtensionProperties(nullptr, 0, &ext_count, nullptr);
	vector<XrExtensionProperties> xr_exts(ext_count, { XR_TYPE_EXTENSION_PROPERTIES });
	xrEnumerateInstanceExtensionProperties(nullptr, ext_count, &ext_count, xr_exts.data());

	printf("OpenXR extensions available:\n");
	for (size_t i = 0; i < xr_exts.size(); i++) {
		printf("- %s\n", xr_exts[i].extensionName);
		if (strcmp(ask_extension, xr_exts[i].extensionName) == 0) {
			use_extensions.push_back(ask_extension);
		}
	}

	if (!std::any_of( use_extensions.begin(), use_extensions.end(), [] (const char *ext) {
        return strcmp(ext, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME) == 0;
	})) { return false; }

	XrInstanceCreateInfo createInfo = { XR_TYPE_INSTANCE_CREATE_INFO };
	createInfo.enabledExtensionCount = use_extensions.size();
	createInfo.enabledExtensionNames = use_extensions.data();
	createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	strcpy_s(createInfo.applicationInfo.applicationName, "app");
	xrCreateInstance(&createInfo, &xr_instance);

    if (xr_instance == nullptr) { return false; }

	xrGetInstanceProcAddr(xr_instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction *)(&ext_xrGetOpenGLGraphicsRequirementsKHR));
	
	XrSystemGetInfo systemInfo = { XR_TYPE_SYSTEM_GET_INFO };
	systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	xrGetSystem(xr_instance, &systemInfo, &xr_system_id);

	uint32_t blend_count = 0;
	xrEnumerateEnvironmentBlendModes(xr_instance, xr_system_id, app_config_view, 1, &blend_count, &xr_blend);

	XrGraphicsRequirementsOpenGLKHR requirement = { XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
	ext_xrGetOpenGLGraphicsRequirementsKHR(xr_instance, xr_system_id, &requirement);

    device_init();

	XrSessionCreateInfo sessionInfo = { XR_TYPE_SESSION_CREATE_INFO };
	sessionInfo.next = &xr_graphicsBinding;
	sessionInfo.systemId = xr_system_id;
	xrCreateSession(xr_instance, &sessionInfo, &xr_session);

	if (xr_session == nullptr) { return false; }

    XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    XrSpace space;
    XrResult res = xrCreateReferenceSpace(xr_session, &referenceSpaceCreateInfo, &space);

	XrReferenceSpaceCreateInfo ref_space = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	ref_space.poseInReferenceSpace = xr_pose_identity;
	ref_space.referenceSpaceType   = XR_REFERENCE_SPACE_TYPE_LOCAL;
	xrCreateReferenceSpace(xr_session, &ref_space, &xr_app_space);

	uint32_t view_count = 0;
	xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, app_config_view, 0, &view_count, nullptr);
	xr_config_views.resize(view_count, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
	xr_views.resize(view_count, { XR_TYPE_VIEW });
	xrEnumerateViewConfigurationViews(xr_instance, xr_system_id, app_config_view, view_count, &view_count, xr_config_views.data());

	for (uint32_t i = 0; i < view_count; i++) {
		XrViewConfigurationView &view = xr_config_views[i];
		XrSwapchainCreateInfo swapchain_info = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
		XrSwapchain handle;
		swapchain_info.arraySize   = 1;
		swapchain_info.mipCount    = 1;
		swapchain_info.faceCount   = 1;
		swapchain_info.format      = GL_RGBA8;
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

void openxr_poll_events(bool &exit) {
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
			        case XR_SESSION_STATE_EXITING:      exit = true; break;
			        case XR_SESSION_STATE_LOSS_PENDING: exit = true; break;
			    }
		    } break;
		    case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: exit = true; return;
		}
		event_buffer = { XR_TYPE_EVENT_DATA_BUFFER };
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
        opengl_render_layer(views[i], swapchainImage, i);

		XrSwapchainImageReleaseInfo release_info = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
		xrReleaseSwapchainImage(xr_swapchains[i].handle, &release_info);
	}

	layer.space = xr_app_space;
	layer.viewCount = (uint32_t)views.size();
	layer.views = views.data();
	return true;
}

void openxr_render_frame() {
	XrFrameState frame_state = { XR_TYPE_FRAME_STATE };
	xrWaitFrame (xr_session, nullptr, &frame_state);
	xrBeginFrame(xr_session, nullptr);

	XrCompositionLayerBaseHeader *layer = nullptr;
	XrCompositionLayerProjection layer_proj = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	vector<XrCompositionLayerProjectionView> views;
	bool session_active = xr_session_state == XR_SESSION_STATE_VISIBLE || xr_session_state == XR_SESSION_STATE_FOCUSED;

	if (session_active && openxr_render_layer(frame_state.predictedDisplayTime, views, layer_proj)) {
		layer = (XrCompositionLayerBaseHeader*)&layer_proj;
	}

	XrFrameEndInfo end_info{ XR_TYPE_FRAME_END_INFO };
	end_info.displayTime = frame_state.predictedDisplayTime;
	end_info.environmentBlendMode = xr_blend;
	end_info.layerCount = layer == nullptr ? 0 : 1;
	end_info.layers = &layer;
	xrEndFrame(xr_session, &end_info);
}

void openxr_shutdown() {
	for (int32_t i = 0; i < xr_swapchains.size(); i++) {
		xrDestroySwapchain(xr_swapchains[i].handle);
	}
	xr_swapchains.clear();
    xrDestroySpace(xr_app_space);
    xrDestroySession(xr_session);
    xrDestroyInstance(xr_instance);
    destroy_app_window();
}

void device_init() {
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
    xrGetInstanceProcAddr(xr_instance, "xrGetOpenGLGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&pfnGetOpenGLGraphicsRequirementsKHR));
    XrGraphicsRequirementsOpenGLKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR};
    pfnGetOpenGLGraphicsRequirementsKHR(xr_instance, xr_system_id, &graphicsRequirements);
    create_app_window();
    xr_graphicsBinding.hDC = hDC;
    xr_graphicsBinding.hGLRC = hGLRC;
}

GLuint m_swapchainFramebuffer{0};
GLuint m_program{0};
GLint m_modelViewProjectionUniformLocation{0};
GLint m_vertexAttribCoords{0};
GLint m_vertexAttribColor{0};
GLuint m_vertexArrayObject{0};
GLuint m_cubeVertexBuffer{0};
GLuint m_cubeIndexBuffer{0};
GLuint m_planeVertexBuffer{0};
GLuint m_planeIndexBuffer{0};
GLuint m_sphereVertexBuffer{0};
GLuint m_sphereIndexBuffer{0};
std::map<uint32_t, uint32_t> m_colorToDepthMap;

static const char* VertexShaderGlsl = "#version 410\n\nin vec3 VertexPos;in vec3 VertexColor;out vec3 PSVertexColor;uniform mat4 ModelViewProjection;void main() {gl_Position = ModelViewProjection * vec4(VertexPos, 1.0);PSVertexColor = VertexColor;}";
static const char* FragmentShaderGlsl = "#version 410\n\nin vec3 PSVertexColor;out vec4 FragColor;void main() {FragColor = vec4(PSVertexColor, 1);}";

namespace Geometry {
    struct Vertex {
        XrVector3f Position;
        XrVector3f Color;
    };
    XrVector3f Red{ 1, 0, 0 }, DarkRed{ 0.25f, 0, 0 }, Green{ 0, 1, 0 }, MidGreen{ 0, 0.5f, 0 }, DarkGreen{ 0, 0.25f, 0 }, Blue{ 0, 0, 1 }, DarkBlue{ 0, 0, 0.25f }, Yellow{ 1.f, 1.f, 0.f }, Cyan{ 0.f, 1.f, 1.f }, Purple{ 1.f, 0.f, 1.f };
    XrVector3f LBB{-0.5f, -0.5f, -0.5f}, LBF{-0.5f, -0.5f, 0.5f}, LTB{-0.5f, 0.5f, -0.5f}, LTF{-0.5f, 0.5f, 0.5f}, RBB{0.5f, -0.5f, -0.5f}, RBF{0.5f, -0.5f, 0.5f}, RTB{0.5f, 0.5f, -0.5f}, RTF{0.5f, 0.5f, 0.5f};
    Vertex c_cubeVertices[] = { {LTB, DarkRed}, {LBF, DarkRed}, {LBB, DarkRed}, {LTB, DarkRed}, {LTF, DarkRed}, {LBF, DarkRed}, {RTB, Red}, {RBB, Red}, {RBF, Red}, {RTB, Red}, {RBF, Red}, {RTF, Red}, {LBB, MidGreen}, {LBF, MidGreen}, {RBF, MidGreen}, {LBB, MidGreen}, {RBF, MidGreen}, {RBB, MidGreen}, {LTB, Green}, {RTB, Green}, {RTF, Green}, {LTB, Green}, {RTF, Green}, {LTF, Green}, {LBB, DarkBlue}, {RBB, DarkBlue}, {RTB, DarkBlue}, {LBB, DarkBlue}, {RTB, DarkBlue}, {LTB, DarkBlue}, {LBF, Blue}, {LTF, Blue}, {RTF, Blue}, {LBF, Blue}, {RTF, Blue}, {RBF, Blue} };
    unsigned short c_cubeIndices[] = { 0,  1,  2,  3,  4,  5, 6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35 };
    Vertex c_planeVertices[] = { {LTB,DarkGreen}, {RTB,DarkGreen}, {RTF,DarkGreen}, {LTB,DarkGreen}, {RTF,DarkGreen}, {LTF,DarkGreen} };
    unsigned short c_planeIndices[] = { 0,  1,  2,  3,  4,  5 };
    Vertex c_sphereVertices[] = { {{-0.577350, -0.577350, -0.577350},Red}, {{0.577350, -0.577350, -0.577350},Red}, {{0.577350, 0.577350, -0.577350},Red}, {{-0.577350, 0.577350, -0.577350},Red}, {{-0.577350, -0.577350, 0.577350},Green}, {{0.577350, -0.577350, 0.577350},Green}, {{0.577350, 0.577350, 0.577350},Green}, {{-0.577350, 0.577350, 0.577350},Green}, {{-0.577350, -0.577350, -0.577350},Blue}, {{-0.577350, 0.577350, -0.577350},Blue}, {{-0.577350, 0.577350, 0.577350},Blue}, {{-0.577350, -0.577350, 0.577350},Blue}, {{0.577350, -0.577350, -0.577350},Yellow}, {{0.577350, 0.577350, -0.577350},Yellow}, {{0.577350, 0.577350, 0.577350},Yellow}, {{0.577350, -0.577350, 0.577350},Yellow}, {{-0.577350, -0.577350, -0.577350},Cyan}, {{0.577350, -0.577350, -0.577350},Cyan}, {{0.577350, -0.577350, 0.577350},Cyan}, {{-0.577350, -0.577350, 0.577350},Cyan}, {{-0.577350, 0.577350, -0.577350},Purple}, {{0.577350, 0.577350, -0.577350},Purple}, {{0.577350, 0.577350, 0.577350},Purple}, {{-0.577350, 0.577350, 0.577350},Purple}, {{0.000000, 0.000000, -1.000000},Red}, {Blue,Green}, {{-1.000000, 0.000000, 0.000000},Blue}, {Red,Yellow}, {{0.000000, -1.000000, 0.000000},Cyan}, {Green,Purple}, {{0.707107, 0.000000, -0.707107},Red}, {{0.000000, -0.707107, -0.707107},Red}, {{-0.707107, 0.000000, -0.707107},Red}, {{0.000000, 0.707107, -0.707107},Red}, {{0.000000, -0.707107, 0.707107},Green}, {{0.707107, 0.000000, 0.707107},Green}, {{0.000000, 0.707107, 0.707107},Green}, {{-0.707107, 0.000000, 0.707107},Green}, {{-0.707107, 0.707107, 0.000000},Blue}, {{-0.707107, 0.000000, -0.707107},Blue}, {{-0.707107, -0.707107, 0.000000},Blue}, {{-0.707107, 0.000000, 0.707107},Blue}, {{0.707107, 0.000000, -0.707107},Yellow}, {{0.707107, 0.707107, 0.000000},Yellow}, {{0.707107, 0.000000, 0.707107},Yellow}, {{0.707107, -0.707107, 0.000000},Yellow}, {{0.707107, -0.707107, 0.000000},Cyan}, {{0.000000, -0.707107, -0.707107},Cyan}, {{-0.707107, -0.707107, 0.000000},Cyan}, {{0.000000, -0.707107, 0.707107},Cyan}, {{0.000000, 0.707107, -0.707107},Purple}, {{0.707107, 0.707107, 0.000000},Purple}, {{0.000000, 0.707107, 0.707107},Purple}, {{-0.707107, 0.707107, 0.000000},Purple}, {{0.325058, -0.325058, -0.888074},Red}, {{0.325058, 0.325058, -0.888074},Red}, {{-0.325058, -0.325058, -0.888074},Red}, {{-0.325058, 0.325058, -0.888074},Red}, {{0.325058, -0.325058, 0.888074},Green}, {{-0.325058, -0.325058, 0.888074},Green}, {{0.325058, 0.325058, 0.888074},Green}, {{-0.325058, 0.325058, 0.888074},Green}, {{-0.888074, 0.325058, -0.325058},Blue}, {{-0.888074, 0.325058, 0.325058},Blue}, {{-0.888074, -0.325058, -0.325058},Blue}, {{-0.888074, -0.325058, 0.325058},Blue}, {{0.888074, 0.325058, -0.325058},Yellow}, {{0.888074, -0.325058, -0.325058},Yellow}, {{0.888074, 0.325058, 0.325058},Yellow}, {{0.888074, -0.325058, 0.325058},Yellow}, {{0.325058, -0.888074, -0.325058},Cyan}, {{0.325058, -0.888074, 0.325058},Cyan}, {{-0.325058, -0.888074, -0.325058},Cyan}, {{-0.325058, -0.888074, 0.325058},Cyan}, {{0.325058, 0.888074, -0.325058},Purple}, {{-0.325058, 0.888074, -0.325058},Purple}, {{0.325058, 0.888074, 0.325058},Purple}, {{-0.325058, 0.888074, 0.325058},Purple}, {{0.382683, 0.000000, -0.923880},Red}, {{0.673887, -0.302905, -0.673887},Red}, {{0.673887, 0.302905, -0.673887},Red}, {{0.000000, -0.382683, -0.923880},Red}, {{-0.302905, -0.673887, -0.673887},Red}, {{0.302905, -0.673887, -0.673887},Red}, {{-0.382683, 0.000000, -0.923880},Red}, {{-0.673887, 0.302905, -0.673887},Red}, {{-0.673887, -0.302905, -0.673887},Red}, {{0.000000, 0.382683, -0.923880},Red}, {{0.302905, 0.673887, -0.673887},Red}, {{-0.302905, 0.673887, -0.673887},Red}, {{0.000000, -0.382683, 0.923880},Green}, {{0.302905, -0.673887, 0.673887},Green}, {{-0.302905, -0.673887, 0.673887},Green}, {{0.382683, 0.000000, 0.923880},Green}, {{0.673887, 0.302905, 0.673887},Green}, {{0.673887, -0.302905, 0.673887},Green}, {{0.000000, 0.382683, 0.923880},Green}, {{-0.302905, 0.673887, 0.673887},Green}, {{0.302905, 0.673887, 0.673887},Green}, {{-0.382683, 0.000000, 0.923880},Green}, {{-0.673887, -0.302905, 0.673887},Green}, {{-0.673887, 0.302905, 0.673887},Green}, {{-0.923880, 0.382683, 0.000000},Blue}, {{-0.673887, 0.673887, -0.302905},Blue}, {{-0.673887, 0.673887, 0.302905},Blue}, {{-0.923880, 0.000000, -0.382683},Blue}, {{-0.673887, -0.302905, -0.673887},Blue}, {{-0.673887, 0.302905, -0.673887},Blue}, {{-0.923880, -0.382683, 0.000000},Blue}, {{-0.673887, -0.673887, 0.302905},Blue}, {{-0.673887, -0.673887, -0.302905},Blue}, {{-0.923880, 0.000000, 0.382683},Blue}, {{-0.673887, 0.302905, 0.673887},Blue}, {{-0.673887, -0.302905, 0.673887},Blue}, {{0.923880, 0.000000, -0.382683},Yellow}, {{0.673887, 0.302905, -0.673887},Yellow}, {{0.673887, -0.302905, -0.673887},Yellow}, {{0.923880, 0.382683, 0.000000},Yellow}, {{0.673887, 0.673887, 0.302905},Yellow}, {{0.673887, 0.673887, -0.302905},Yellow}, {{0.923880, 0.000000, 0.382683},Yellow}, {{0.673887, -0.302905, 0.673887},Yellow}, {{0.673887, 0.302905, 0.673887},Yellow}, {{0.923880, -0.382683, 0.000000},Yellow}, {{0.673887, -0.673887, -0.302905},Yellow}, {{0.673887, -0.673887, 0.302905},Yellow}, {{0.382683, -0.923880, 0.000000},Cyan}, {{0.673887, -0.673887, -0.302905},Cyan}, {{0.673887, -0.673887, 0.302905},Cyan}, {{0.000000, -0.923880, -0.382683},Cyan}, {{-0.302905, -0.673887, -0.673887},Cyan}, {{0.302905, -0.673887, -0.673887},Cyan}, {{-0.382683, -0.923880, 0.000000},Cyan}, {{-0.673887, -0.673887, 0.302905},Cyan}, {{-0.673887, -0.673887, -0.302905},Cyan}, {{0.000000, -0.923880, 0.382683},Cyan}, {{0.302905, -0.673887, 0.673887},Cyan}, {{-0.302905, -0.673887, 0.673887},Cyan}, {{0.000000, 0.923880, -0.382683},Purple}, {{0.302905, 0.673887, -0.673887},Purple}, {{-0.302905, 0.673887, -0.673887},Purple}, {{0.382683, 0.923880, 0.000000},Purple}, {{0.673887, 0.673887, 0.302905},Purple}, {{0.673887, 0.673887, -0.302905},Purple}, {{0.000000, 0.923880, 0.382683},Purple}, {{-0.302905, 0.673887, 0.673887},Purple}, {{0.302905, 0.673887, 0.673887},Purple}, {{-0.382683, 0.923880, 0.000000},Purple}, {{-0.673887, 0.673887, -0.302905},Purple}, {{-0.673887, 0.673887, 0.302905},Purple}, {{0.535467, -0.168634, -0.827549},Red}, {{0.167277, -0.167277, -0.971616},Red}, {{0.464385, -0.464385, -0.754117},Red}, {{0.535467, 0.168634, -0.827549},Red}, {{0.464385, 0.464385, -0.754117},Red}, {{0.167277, 0.167277, -0.971616},Red}, {{-0.168634, -0.535467, -0.827549},Red}, {{-0.167277, -0.167277, -0.971616},Red}, {{-0.464385, -0.464385, -0.754117},Red}, {{0.168634, -0.535467, -0.827549},Red}, {{-0.535467, 0.168634, -0.827549},Red}, {{-0.167277, 0.167277, -0.971616},Red}, {{-0.464385, 0.464385, -0.754117},Red}, {{-0.535467, -0.168634, -0.827549},Red}, {{0.168634, 0.535467, -0.827549},Red}, {{-0.168634, 0.535467, -0.827549},Red}, {{0.168634, -0.535467, 0.827549},Green}, {{0.167277, -0.167277, 0.971616},Green}, {{0.464385, -0.464385, 0.754117},Green}, {{-0.168634, -0.535467, 0.827549},Green}, {{-0.464385, -0.464385, 0.754117},Green}, {{-0.167277, -0.167277, 0.971616},Green}, {{0.535467, 0.168634, 0.827549},Green}, {{0.167277, 0.167277, 0.971616},Green}, {{0.464385, 0.464385, 0.754117},Green}, {{0.535467, -0.168634, 0.827549},Green}, {{-0.168634, 0.535467, 0.827549},Green}, {{-0.167277, 0.167277, 0.971616},Green}, {{-0.464385, 0.464385, 0.754117},Green}, {{0.168634, 0.535467, 0.827549},Green}, {{-0.535467, -0.168634, 0.827549},Green}, {{-0.535467, 0.168634, 0.827549},Green}, {{-0.827549, 0.535467, -0.168634},Blue}, {{-0.971616, 0.167277, -0.167277},Blue}, {{-0.754117, 0.464385, -0.464385},Blue}, {{-0.827549, 0.535467, 0.168634},Blue}, {{-0.754117, 0.464385, 0.464385},Blue}, {{-0.971616, 0.167277, 0.167277},Blue}, {{-0.827549, -0.168634, -0.535467},Blue}, {{-0.971616, -0.167277, -0.167277},Blue}, {{-0.754117, -0.464385, -0.464385},Blue}, {{-0.827549, 0.168634, -0.535467},Blue}, {{-0.827549, -0.535467, 0.168634},Blue}, {{-0.971616, -0.167277, 0.167277},Blue}, {{-0.754117, -0.464385, 0.464385},Blue}, {{-0.827549, -0.535467, -0.168634},Blue}, {{-0.827549, 0.168634, 0.535467},Blue}, {{-0.827549, -0.168634, 0.535467},Blue}, {{0.827549, 0.168634, -0.535467},Yellow}, {{0.971616, 0.167277, -0.167277},Yellow}, {{0.754117, 0.464385, -0.464385},Yellow}, {{0.827549, -0.168634, -0.535467},Yellow}, {{0.754117, -0.464385, -0.464385},Yellow}, {{0.971616, -0.167277, -0.167277},Yellow}, {{0.827549, 0.535467, 0.168634},Yellow}, {{0.971616, 0.167277, 0.167277},Yellow}, {{0.754117, 0.464385, 0.464385},Yellow}, {{0.827549, 0.535467, -0.168634},Yellow}, {{0.827549, -0.168634, 0.535467},Yellow}, {{0.971616, -0.167277, 0.167277},Yellow}, {{0.754117, -0.464385, 0.464385},Yellow}, {{0.827549, 0.168634, 0.535467},Yellow}, {{0.827549, -0.535467, -0.168634},Yellow}, {{0.827549, -0.535467, 0.168634},Yellow}, {{0.535467, -0.827549, -0.168634},Cyan}, {{0.167277, -0.971616, -0.167277},Cyan}, {{0.464385, -0.754117, -0.464385},Cyan}, {{0.535467, -0.827549, 0.168634},Cyan}, {{0.464385, -0.754117, 0.464385},Cyan}, {{0.167277, -0.971616, 0.167277},Cyan}, {{-0.168634, -0.827549, -0.535467},Cyan}, {{-0.167277, -0.971616, -0.167277},Cyan}, {{-0.464385, -0.754117, -0.464385},Cyan}, {{0.168634, -0.827549, -0.535467},Cyan}, {{-0.535467, -0.827549, 0.168634},Cyan}, {{-0.167277, -0.971616, 0.167277},Cyan}, {{-0.464385, -0.754117, 0.464385},Cyan}, {{-0.535467, -0.827549, -0.168634},Cyan}, {{0.168634, -0.827549, 0.535467},Cyan}, {{-0.168634, -0.827549, 0.535467},Cyan}, {{0.168634, 0.827549, -0.535467},Purple}, {{0.167277, 0.971616, -0.167277},Purple}, {{0.464385, 0.754117, -0.464385},Purple}, {{-0.168634, 0.827549, -0.535467},Purple}, {{-0.464385, 0.754117, -0.464385},Purple}, {{-0.167277, 0.971616, -0.167277},Purple}, {{0.535467, 0.827549, 0.168634},Purple}, {{0.167277, 0.971616, 0.167277},Purple}, {{0.464385, 0.754117, 0.464385},Purple}, {{0.535467, 0.827549, -0.168634},Purple}, {{-0.168634, 0.827549, 0.535467},Purple}, {{-0.167277, 0.971616, 0.167277},Purple}, {{-0.464385, 0.754117, 0.464385},Purple}, {{0.168634, 0.827549, 0.535467},Purple}, {{-0.535467, 0.827549, -0.168634},Purple}, {{-0.535467, 0.827549, 0.168634},Purple}, {{0.358851, -0.164816, -0.918728},Red}, {{0.555570, 0.000000, -0.831470},Red}, {{0.195090, 0.000000, -0.980785},Red}, {{0.510307, -0.320792, -0.797922},Red}, {{0.633099, -0.445390, -0.633099},Red}, {{0.698753, -0.153263, -0.698753},Red}, {{0.510307, 0.320792, -0.797922},Red}, {{0.698753, 0.153263, -0.698753},Red}, {{0.633099, 0.445390, -0.633099},Red}, {{0.358851, 0.164816, -0.918728},Red}, {{-0.164816, -0.358851, -0.918728},Red}, {{0.000000, -0.555570, -0.831470},Red}, {{0.000000, -0.195090, -0.980785},Red}, {{-0.320792, -0.510307, -0.797922},Red}, {{-0.445390, -0.633099, -0.633099},Red}, {{-0.153263, -0.698753, -0.698753},Red}, {{0.320792, -0.510307, -0.797922},Red}, {{0.153263, -0.698753, -0.698753},Red}, {{0.445390, -0.633099, -0.633099},Red}, {{0.164816, -0.358851, -0.918728},Red}, {{-0.358851, 0.164816, -0.918728},Red}, {{-0.555570, 0.000000, -0.831470},Red}, {{-0.195090, 0.000000, -0.980785},Red}, {{-0.510307, 0.320792, -0.797922},Red}, {{-0.633099, 0.445390, -0.633099},Red}, {{-0.698753, 0.153263, -0.698753},Red}, {{-0.510307, -0.320792, -0.797922},Red}, {{-0.698753, -0.153263, -0.698753},Red}, {{-0.633099, -0.445390, -0.633099},Red}, {{-0.358851, -0.164816, -0.918728},Red}, {{0.164816, 0.358851, -0.918728},Red}, {{0.000000, 0.555570, -0.831470},Red}, {{0.000000, 0.195090, -0.980785},Red}, {{0.320792, 0.510307, -0.797922},Red}, {{0.445390, 0.633099, -0.633099},Red}, {{0.153263, 0.698753, -0.698753},Red}, {{-0.320792, 0.510307, -0.797922},Red}, {{-0.153263, 0.698753, -0.698753},Red}, {{-0.445390, 0.633099, -0.633099},Red}, {{-0.164816, 0.358851, -0.918728},Red}, {{0.164816, -0.358851, 0.918728},Green}, {{0.000000, -0.555570, 0.831470},Green}, {{0.000000, -0.195090, 0.980785},Green}, {{0.320792, -0.510307, 0.797922},Green}, {{0.445390, -0.633099, 0.633099},Green}, {{0.153263, -0.698753, 0.698753},Green}, {{-0.320792, -0.510307, 0.797922},Green}, {{-0.153263, -0.698753, 0.698753},Green}, {{-0.445390, -0.633099, 0.633099},Green}, {{-0.164816, -0.358851, 0.918728},Green}, {{0.358851, 0.164816, 0.918728},Green}, {{0.555570, 0.000000, 0.831470},Green}, {{0.195090, 0.000000, 0.980785},Green}, {{0.510307, 0.320792, 0.797922},Green}, {{0.633099, 0.445390, 0.633099},Green}, {{0.698753, 0.153263, 0.698753},Green}, {{0.510307, -0.320792, 0.797922},Green}, {{0.698753, -0.153263, 0.698753},Green}, {{0.633099, -0.445390, 0.633099},Green}, {{0.358851, -0.164816, 0.918728},Green}, {{-0.164816, 0.358851, 0.918728},Green}, {{0.000000, 0.555570, 0.831470},Green}, {{0.000000, 0.195090, 0.980785},Green}, {{-0.320792, 0.510307, 0.797922},Green}, {{-0.445390, 0.633099, 0.633099},Green}, {{-0.153263, 0.698753, 0.698753},Green}, {{0.320792, 0.510307, 0.797922},Green}, {{0.153263, 0.698753, 0.698753},Green}, {{0.445390, 0.633099, 0.633099},Green}, {{0.164816, 0.358851, 0.918728},Green}, {{-0.358851, -0.164816, 0.918728},Green}, {{-0.555570, 0.000000, 0.831470},Green}, {{-0.195090, 0.000000, 0.980785},Green}, {{-0.510307, -0.320792, 0.797922},Green}, {{-0.633099, -0.445390, 0.633099},Green}, {{-0.698753, -0.153263, 0.698753},Green}, {{-0.510307, 0.320792, 0.797922},Green}, {{-0.698753, 0.153263, 0.698753},Green}, {{-0.633099, 0.445390, 0.633099},Green}, {{-0.358851, 0.164816, 0.918728},Green}, {{-0.918728, 0.358851, -0.164816},Blue}, {{-0.831470, 0.555570, 0.000000},Blue}, {{-0.980785, 0.195090, 0.000000},Blue}, {{-0.797922, 0.510307, -0.320792},Blue}, {{-0.633099, 0.633099, -0.445390},Blue}, {{-0.698753, 0.698753, -0.153263},Blue}, {{-0.797922, 0.510307, 0.320792},Blue}, {{-0.698753, 0.698753, 0.153263},Blue}, {{-0.633099, 0.633099, 0.445390},Blue}, {{-0.918728, 0.358851, 0.164816},Blue}, {{-0.918728, -0.164816, -0.358851},Blue}, {{-0.831470, 0.000000, -0.555570},Blue}, {{-0.980785, 0.000000, -0.195090},Blue}, {{-0.797922, -0.320792, -0.510307},Blue}, {{-0.633099, -0.445390, -0.633099},Blue}, {{-0.698753, -0.153263, -0.698753},Blue}, {{-0.797922, 0.320792, -0.510307},Blue}, {{-0.698753, 0.153263, -0.698753},Blue}, {{-0.633099, 0.445390, -0.633099},Blue}, {{-0.918728, 0.164816, -0.358851},Blue}, {{-0.918728, -0.358851, 0.164816},Blue}, {{-0.831470, -0.555570, 0.000000},Blue}, {{-0.980785, -0.195090, 0.000000},Blue}, {{-0.797922, -0.510307, 0.320792},Blue}, {{-0.633099, -0.633099, 0.445390},Blue}, {{-0.698753, -0.698753, 0.153263},Blue}, {{-0.797922, -0.510307, -0.320792},Blue}, {{-0.698753, -0.698753, -0.153263},Blue}, {{-0.633099, -0.633099, -0.445390},Blue}, {{-0.918728, -0.358851, -0.164816},Blue}, {{-0.918728, 0.164816, 0.358851},Blue}, {{-0.831470, 0.000000, 0.555570},Blue}, {{-0.980785, 0.000000, 0.195090},Blue}, {{-0.797922, 0.320792, 0.510307},Blue}, {{-0.633099, 0.445390, 0.633099},Blue}, {{-0.698753, 0.153263, 0.698753},Blue}, {{-0.797922, -0.320792, 0.510307},Blue}, {{-0.698753, -0.153263, 0.698753},Blue}, {{-0.633099, -0.445390, 0.633099},Blue}, {{-0.918728, -0.164816, 0.358851},Blue}, {{0.918728, 0.164816, -0.358851},Yellow}, {{0.831470, 0.000000, -0.555570},Yellow}, {{0.980785, 0.000000, -0.195090},Yellow}, {{0.797922, 0.320792, -0.510307},Yellow}, {{0.633099, 0.445390, -0.633099},Yellow}, {{0.698753, 0.153263, -0.698753},Yellow}, {{0.797922, -0.320792, -0.510307},Yellow}, {{0.698753, -0.153263, -0.698753},Yellow}, {{0.633099, -0.445390, -0.633099},Yellow}, {{0.918728, -0.164816, -0.358851},Yellow}, {{0.918728, 0.358851, 0.164816},Yellow}, {{0.831470, 0.555570, 0.000000},Yellow}, {{0.980785, 0.195090, 0.000000},Yellow}, {{0.797922, 0.510307, 0.320792},Yellow}, {{0.633099, 0.633099, 0.445390},Yellow}, {{0.698753, 0.698753, 0.153263},Yellow}, {{0.797922, 0.510307, -0.320792},Yellow}, {{0.698753, 0.698753, -0.153263},Yellow}, {{0.633099, 0.633099, -0.445390},Yellow}, {{0.918728, 0.358851, -0.164816},Yellow}, {{0.918728, -0.164816, 0.358851},Yellow}, {{0.831470, 0.000000, 0.555570},Yellow}, {{0.980785, 0.000000, 0.195090},Yellow}, {{0.797922, -0.320792, 0.510307},Yellow}, {{0.633099, -0.445390, 0.633099},Yellow}, {{0.698753, -0.153263, 0.698753},Yellow}, {{0.797922, 0.320792, 0.510307},Yellow}, {{0.698753, 0.153263, 0.698753},Yellow}, {{0.633099, 0.445390, 0.633099},Yellow}, {{0.918728, 0.164816, 0.358851},Yellow}, {{0.918728, -0.358851, -0.164816},Yellow}, {{0.831470, -0.555570, 0.000000},Yellow}, {{0.980785, -0.195090, 0.000000},Yellow}, {{0.797922, -0.510307, -0.320792},Yellow}, {{0.633099, -0.633099, -0.445390},Yellow}, {{0.698753, -0.698753, -0.153263},Yellow}, {{0.797922, -0.510307, 0.320792},Yellow}, {{0.698753, -0.698753, 0.153263},Yellow}, {{0.633099, -0.633099, 0.445390},Yellow}, {{0.918728, -0.358851, 0.164816},Yellow}, {{0.358851, -0.918728, -0.164816},Cyan}, {{0.555570, -0.831470, 0.000000},Cyan}, {{0.195090, -0.980785, 0.000000},Cyan}, {{0.510307, -0.797922, -0.320792},Cyan}, {{0.633099, -0.633099, -0.445390},Cyan}, {{0.698753, -0.698753, -0.153263},Cyan}, {{0.510307, -0.797922, 0.320792},Cyan}, {{0.698753, -0.698753, 0.153263},Cyan}, {{0.633099, -0.633099, 0.445390},Cyan}, {{0.358851, -0.918728, 0.164816},Cyan}, {{-0.164816, -0.918728, -0.358851},Cyan}, {{0.000000, -0.831470, -0.555570},Cyan}, {{0.000000, -0.980785, -0.195090},Cyan}, {{-0.320792, -0.797922, -0.510307},Cyan}, {{-0.445390, -0.633099, -0.633099},Cyan}, {{-0.153263, -0.698753, -0.698753},Cyan}, {{0.320792, -0.797922, -0.510307},Cyan}, {{0.153263, -0.698753, -0.698753},Cyan}, {{0.445390, -0.633099, -0.633099},Cyan}, {{0.164816, -0.918728, -0.358851},Cyan}, {{-0.358851, -0.918728, 0.164816},Cyan}, {{-0.555570, -0.831470, 0.000000},Cyan}, {{-0.195090, -0.980785, 0.000000},Cyan}, {{-0.510307, -0.797922, 0.320792},Cyan}, {{-0.633099, -0.633099, 0.445390},Cyan}, {{-0.698753, -0.698753, 0.153263},Cyan}, {{-0.510307, -0.797922, -0.320792},Cyan}, {{-0.698753, -0.698753, -0.153263},Cyan}, {{-0.633099, -0.633099, -0.445390},Cyan}, {{-0.358851, -0.918728, -0.164816},Cyan}, {{0.164816, -0.918728, 0.358851},Cyan}, {{0.000000, -0.831470, 0.555570},Cyan}, {{0.000000, -0.980785, 0.195090},Cyan}, {{0.320792, -0.797922, 0.510307},Cyan}, {{0.445390, -0.633099, 0.633099},Cyan}, {{0.153263, -0.698753, 0.698753},Cyan}, {{-0.320792, -0.797922, 0.510307},Cyan}, {{-0.153263, -0.698753, 0.698753},Cyan}, {{-0.445390, -0.633099, 0.633099},Cyan}, {{-0.164816, -0.918728, 0.358851},Cyan}, {{0.164816, 0.918728, -0.358851},Purple}, {{0.000000, 0.831470, -0.555570},Purple}, {{0.000000, 0.980785, -0.195090},Purple}, {{0.320792, 0.797922, -0.510307},Purple}, {{0.445390, 0.633099, -0.633099},Purple}, {{0.153263, 0.698753, -0.698753},Purple}, {{-0.320792, 0.797922, -0.510307},Purple}, {{-0.153263, 0.698753, -0.698753},Purple}, {{-0.445390, 0.633099, -0.633099},Purple}, {{-0.164816, 0.918728, -0.358851},Purple}, {{0.358851, 0.918728, 0.164816},Purple}, {{0.555570, 0.831470, 0.000000},Purple}, {{0.195090, 0.980785, 0.000000},Purple}, {{0.510307, 0.797922, 0.320792},Purple}, {{0.633099, 0.633099, 0.445390},Purple}, {{0.698753, 0.698753, 0.153263},Purple}, {{0.510307, 0.797922, -0.320792},Purple}, {{0.698753, 0.698753, -0.153263},Purple}, {{0.633099, 0.633099, -0.445390},Purple}, {{0.358851, 0.918728, -0.164816},Purple}, {{-0.164816, 0.918728, 0.358851},Purple}, {{0.000000, 0.831470, 0.555570},Purple}, {{0.000000, 0.980785, 0.195090},Purple}, {{-0.320792, 0.797922, 0.510307},Purple}, {{-0.445390, 0.633099, 0.633099},Purple}, {{-0.153263, 0.698753, 0.698753},Purple}, {{0.320792, 0.797922, 0.510307},Purple}, {{0.153263, 0.698753, 0.698753},Purple}, {{0.445390, 0.633099, 0.633099},Purple}, {{0.164816, 0.918728, 0.358851},Purple}, {{-0.358851, 0.918728, -0.164816},Purple}, {{-0.555570, 0.831470, 0.000000},Purple}, {{-0.195090, 0.980785, 0.000000},Purple}, {{-0.510307, 0.797922, -0.320792},Purple}, {{-0.633099, 0.633099, -0.445390},Purple}, {{-0.698753, 0.698753, -0.153263},Purple}, {{-0.510307, 0.797922, 0.320792},Purple}, {{-0.698753, 0.698753, 0.153263},Purple}, {{-0.633099, 0.633099, 0.445390},Purple}, {{-0.358851, 0.918728, 0.164816},Purple} };
    unsigned short c_sphereIndices[] = { 78, 246, 150, 150, 246, 54, 30, 247, 150, 150, 247, 78, 78, 248, 151, 151, 248, 24, 54, 246, 151, 151, 246, 78, 79, 249, 152, 152, 249, 54, 1, 250, 152, 152, 250, 79, 79, 251, 150, 150, 251, 30, 54, 249, 150, 150, 249, 79, 80, 252, 153, 153, 252, 55, 30, 253, 153, 153, 253, 80, 80, 254, 154, 154, 254, 2, 55, 252, 154, 154, 252, 80, 78, 255, 155, 155, 255, 55, 24, 248, 155, 155, 248, 78, 78, 247, 153, 153, 247, 30, 55, 255, 153, 153, 255, 78, 81, 256, 156, 156, 256, 56, 31, 257, 156, 156, 257, 81, 81, 258, 157, 157, 258, 24, 56, 256, 157, 157, 256, 81, 82, 259, 158, 158, 259, 56, 0, 260, 158, 158, 260, 82, 82, 261, 156, 156, 261, 31, 56, 259, 156, 156, 259, 82, 83, 262, 159, 159, 262, 54, 31, 263, 159, 159, 263, 83, 83, 264, 152, 152, 264, 1, 54, 262, 152, 152, 262, 83, 81, 265, 151, 151, 265, 54, 24, 258, 151, 151, 258, 81, 81, 257, 159, 159, 257, 31, 54, 265, 159, 159, 265, 81, 84, 266, 160, 160, 266, 57, 32, 267, 160, 160, 267, 84, 84, 268, 161, 161, 268, 24, 57, 266, 161, 161, 266, 84, 85, 269, 162, 162, 269, 57, 3, 270, 162, 162, 270, 85, 85, 271, 160, 160, 271, 32, 57, 269, 160, 160, 269, 85, 86, 272, 163, 163, 272, 56, 32, 273, 163, 163, 273, 86, 86, 274, 158, 158, 274, 0, 56, 272, 158, 158, 272, 86, 84, 275, 157, 157, 275, 56, 24, 268, 157, 157, 268, 84, 84, 267, 163, 163, 267, 32, 56, 275, 163, 163, 275, 84, 87, 276, 164, 164, 276, 55, 33, 277, 164, 164, 277, 87, 87, 278, 155, 155, 278, 24, 55, 276, 155, 155, 276, 87, 88, 279, 154, 154, 279, 55, 2, 280, 154, 154, 280, 88, 88, 281, 164, 164, 281, 33, 55, 279, 164, 164, 279, 88, 89, 282, 165, 165, 282, 57, 33, 283, 165, 165, 283, 89, 89, 284, 162, 162, 284, 3, 57, 282, 162, 162, 282, 89, 87, 285, 161, 161, 285, 57, 24, 278, 161, 161, 278, 87, 87, 277, 165, 165, 277, 33, 57, 285, 165, 165, 285, 87, 90, 286, 166, 166, 286, 58, 34, 287, 166, 166, 287, 90, 90, 288, 167, 167, 288, 25, 58, 286, 167, 167, 286, 90, 91, 289, 168, 168, 289, 58, 5, 290, 168, 168, 290, 91, 91, 291, 166, 166, 291, 34, 58, 289, 166, 166, 289, 91, 92, 292, 169, 169, 292, 59, 34, 293, 169, 169, 293, 92, 92, 294, 170, 170, 294, 4, 59, 292, 170, 170, 292, 92, 90, 295, 171, 171, 295, 59, 25, 288, 171, 171, 288, 90, 90, 287, 169, 169, 287, 34, 59, 295, 169, 169, 295, 90, 93, 296, 172, 172, 296, 60, 35, 297, 172, 172, 297, 93, 93, 298, 173, 173, 298, 25, 60, 296, 173, 173, 296, 93, 94, 299, 174, 174, 299, 60, 6, 300, 174, 174, 300, 94, 94, 301, 172, 172, 301, 35, 60, 299, 172, 172, 299, 94, 95, 302, 175, 175, 302, 58, 35, 303, 175, 175, 303, 95, 95, 304, 168, 168, 304, 5, 58, 302, 168, 168, 302, 95, 93, 305, 167, 167, 305, 58, 25, 298, 167, 167, 298, 93, 93, 297, 175, 175, 297, 35, 58, 305, 175, 175, 305, 93, 96, 306, 176, 176, 306, 61, 36, 307, 176, 176, 307, 96, 96, 308, 177, 177, 308, 25, 61, 306, 177, 177, 306, 96, 97, 309, 178, 178, 309, 61, 7, 310, 178, 178, 310, 97, 97, 311, 176, 176, 311, 36, 61, 309, 176, 176, 309, 97, 98, 312, 179, 179, 312, 60, 36, 313, 179, 179, 313, 98, 98, 314, 174, 174, 314, 6, 60, 312, 174, 174, 312, 98, 96, 315, 173, 173, 315, 60, 25, 308, 173, 173, 308, 96, 96, 307, 179, 179, 307, 36, 60, 315, 179, 179, 315, 96, 99, 316, 180, 180, 316, 59, 37, 317, 180, 180, 317, 99, 99, 318, 171, 171, 318, 25, 59, 316, 171, 171, 316, 99, 100, 319, 170, 170, 319, 59, 4, 320, 170, 170, 320, 100, 100, 321, 180, 180, 321, 37, 59, 319, 180, 180, 319, 100, 101, 322, 181, 181, 322, 61, 37, 323, 181, 181, 323, 101, 101, 324, 178, 178, 324, 7, 61, 322, 178, 178, 322, 101, 99, 325, 177, 177, 325, 61, 25, 318, 177, 177, 318, 99, 99, 317, 181, 181, 317, 37, 61, 325, 181, 181, 325, 99, 102, 326, 182, 182, 326, 62, 38, 327, 182, 182, 327, 102, 102, 328, 183, 183, 328, 26, 62, 326, 183, 183, 326, 102, 103, 329, 184, 184, 329, 62, 9, 330, 184, 184, 330, 103, 103, 331, 182, 182, 331, 38, 62, 329, 182, 182, 329, 103, 104, 332, 185, 185, 332, 63, 38, 333, 185, 185, 333, 104, 104, 334, 186, 186, 334, 10, 63, 332, 186, 186, 332, 104, 102, 335, 187, 187, 335, 63, 26, 328, 187, 187, 328, 102, 102, 327, 185, 185, 327, 38, 63, 335, 185, 185, 335, 102, 105, 336, 188, 188, 336, 64, 39, 337, 188, 188, 337, 105, 105, 338, 189, 189, 338, 26, 64, 336, 189, 189, 336, 105, 106, 339, 190, 190, 339, 64, 8, 340, 190, 190, 340, 106, 106, 341, 188, 188, 341, 39, 64, 339, 188, 188, 339, 106, 107, 342, 191, 191, 342, 62, 39, 343, 191, 191, 343, 107, 107, 344, 184, 184, 344, 9, 62, 342, 184, 184, 342, 107, 105, 345, 183, 183, 345, 62, 26, 338, 183, 183, 338, 105, 105, 337, 191, 191, 337, 39, 62, 345, 191, 191, 345, 105, 108, 346, 192, 192, 346, 65, 40, 347, 192, 192, 347, 108, 108, 348, 193, 193, 348, 26, 65, 346, 193, 193, 346, 108, 109, 349, 194, 194, 349, 65, 11, 350, 194, 194, 350, 109, 109, 351, 192, 192, 351, 40, 65, 349, 192, 192, 349, 109, 110, 352, 195, 195, 352, 64, 40, 353, 195, 195, 353, 110, 110, 354, 190, 190, 354, 8, 64, 352, 190, 190, 352, 110, 108, 355, 189, 189, 355, 64, 26, 348, 189, 189, 348, 108, 108, 347, 195, 195, 347, 40, 64, 355, 195, 195, 355, 108, 111, 356, 196, 196, 356, 63, 41, 357, 196, 196, 357, 111, 111, 358, 187, 187, 358, 26, 63, 356, 187, 187, 356, 111, 112, 359, 186, 186, 359, 63, 10, 360, 186, 186, 360, 112, 112, 361, 196, 196, 361, 41, 63, 359, 196, 196, 359, 112, 113, 362, 197, 197, 362, 65, 41, 363, 197, 197, 363, 113, 113, 364, 194, 194, 364, 11, 65, 362, 194, 194, 362, 113, 111, 365, 193, 193, 365, 65, 26, 358, 193, 193, 358, 111, 111, 357, 197, 197, 357, 41, 65, 365, 197, 197, 365, 111, 114, 366, 198, 198, 366, 66, 42, 367, 198, 198, 367, 114, 114, 368, 199, 199, 368, 27, 66, 366, 199, 199, 366, 114, 115, 369, 200, 200, 369, 66, 13, 370, 200, 200, 370, 115, 115, 371, 198, 198, 371, 42, 66, 369, 198, 198, 369, 115, 116, 372, 201, 201, 372, 67, 42, 373, 201, 201, 373, 116, 116, 374, 202, 202, 374, 12, 67, 372, 202, 202, 372, 116, 114, 375, 203, 203, 375, 67, 27, 368, 203, 203, 368, 114, 114, 367, 201, 201, 367, 42, 67, 375, 201, 201, 375, 114, 117, 376, 204, 204, 376, 68, 43, 377, 204, 204, 377, 117, 117, 378, 205, 205, 378, 27, 68, 376, 205, 205, 376, 117, 118, 379, 206, 206, 379, 68, 14, 380, 206, 206, 380, 118, 118, 381, 204, 204, 381, 43, 68, 379, 204, 204, 379, 118, 119, 382, 207, 207, 382, 66, 43, 383, 207, 207, 383, 119, 119, 384, 200, 200, 384, 13, 66, 382, 200, 200, 382, 119, 117, 385, 199, 199, 385, 66, 27, 378, 199, 199, 378, 117, 117, 377, 207, 207, 377, 43, 66, 385, 207, 207, 385, 117, 120, 386, 208, 208, 386, 69, 44, 387, 208, 208, 387, 120, 120, 388, 209, 209, 388, 27, 69, 386, 209, 209, 386, 120, 121, 389, 210, 210, 389, 69, 15, 390, 210, 210, 390, 121, 121, 391, 208, 208, 391, 44, 69, 389, 208, 208, 389, 121, 122, 392, 211, 211, 392, 68, 44, 393, 211, 211, 393, 122, 122, 394, 206, 206, 394, 14, 68, 392, 206, 206, 392, 122, 120, 395, 205, 205, 395, 68, 27, 388, 205, 205, 388, 120, 120, 387, 211, 211, 387, 44, 68, 395, 211, 211, 395, 120, 123, 396, 212, 212, 396, 67, 45, 397, 212, 212, 397, 123, 123, 398, 203, 203, 398, 27, 67, 396, 203, 203, 396, 123, 124, 399, 202, 202, 399, 67, 12, 400, 202, 202, 400, 124, 124, 401, 212, 212, 401, 45, 67, 399, 212, 212, 399, 124, 125, 402, 213, 213, 402, 69, 45, 403, 213, 213, 403, 125, 125, 404, 210, 210, 404, 15, 69, 402, 210, 210, 402, 125, 123, 405, 209, 209, 405, 69, 27, 398, 209, 209, 398, 123, 123, 397, 213, 213, 397, 45, 69, 405, 213, 213, 405, 123, 126, 406, 214, 214, 406, 70, 46, 407, 214, 214, 407, 126, 126, 408, 215, 215, 408, 28, 70, 406, 215, 215, 406, 126, 127, 409, 216, 216, 409, 70, 17, 410, 216, 216, 410, 127, 127, 411, 214, 214, 411, 46, 70, 409, 214, 214, 409, 127, 128, 412, 217, 217, 412, 71, 46, 413, 217, 217, 413, 128, 128, 414, 218, 218, 414, 18, 71, 412, 218, 218, 412, 128, 126, 415, 219, 219, 415, 71, 28, 408, 219, 219, 408, 126, 126, 407, 217, 217, 407, 46, 71, 415, 217, 217, 415, 126, 129, 416, 220, 220, 416, 72, 47, 417, 220, 220, 417, 129, 129, 418, 221, 221, 418, 28, 72, 416, 221, 221, 416, 129, 130, 419, 222, 222, 419, 72, 16, 420, 222, 222, 420, 130, 130, 421, 220, 220, 421, 47, 72, 419, 220, 220, 419, 130, 131, 422, 223, 223, 422, 70, 47, 423, 223, 223, 423, 131, 131, 424, 216, 216, 424, 17, 70, 422, 216, 216, 422, 131, 129, 425, 215, 215, 425, 70, 28, 418, 215, 215, 418, 129, 129, 417, 223, 223, 417, 47, 70, 425, 223, 223, 425, 129, 132, 426, 224, 224, 426, 73, 48, 427, 224, 224, 427, 132, 132, 428, 225, 225, 428, 28, 73, 426, 225, 225, 426, 132, 133, 429, 226, 226, 429, 73, 19, 430, 226, 226, 430, 133, 133, 431, 224, 224, 431, 48, 73, 429, 224, 224, 429, 133, 134, 432, 227, 227, 432, 72, 48, 433, 227, 227, 433, 134, 134, 434, 222, 222, 434, 16, 72, 432, 222, 222, 432, 134, 132, 435, 221, 221, 435, 72, 28, 428, 221, 221, 428, 132, 132, 427, 227, 227, 427, 48, 72, 435, 227, 227, 435, 132, 135, 436, 228, 228, 436, 71, 49, 437, 228, 228, 437, 135, 135, 438, 219, 219, 438, 28, 71, 436, 219, 219, 436, 135, 136, 439, 218, 218, 439, 71, 18, 440, 218, 218, 440, 136, 136, 441, 228, 228, 441, 49, 71, 439, 228, 228, 439, 136, 137, 442, 229, 229, 442, 73, 49, 443, 229, 229, 443, 137, 137, 444, 226, 226, 444, 19, 73, 442, 226, 226, 442, 137, 135, 445, 225, 225, 445, 73, 28, 438, 225, 225, 438, 135, 135, 437, 229, 229, 437, 49, 73, 445, 229, 229, 445, 135, 138, 446, 230, 230, 446, 74, 50, 447, 230, 230, 447, 138, 138, 448, 231, 231, 448, 29, 74, 446, 231, 231, 446, 138, 139, 449, 232, 232, 449, 74, 21, 450, 232, 232, 450, 139, 139, 451, 230, 230, 451, 50, 74, 449, 230, 230, 449, 139, 140, 452, 233, 233, 452, 75, 50, 453, 233, 233, 453, 140, 140, 454, 234, 234, 454, 20, 75, 452, 234, 234, 452, 140, 138, 455, 235, 235, 455, 75, 29, 448, 235, 235, 448, 138, 138, 447, 233, 233, 447, 50, 75, 455, 233, 233, 455, 138, 141, 456, 236, 236, 456, 76, 51, 457, 236, 236, 457, 141, 141, 458, 237, 237, 458, 29, 76, 456, 237, 237, 456, 141, 142, 459, 238, 238, 459, 76, 22, 460, 238, 238, 460, 142, 142, 461, 236, 236, 461, 51, 76, 459, 236, 236, 459, 142, 143, 462, 239, 239, 462, 74, 51, 463, 239, 239, 463, 143, 143, 464, 232, 232, 464, 21, 74, 462, 232, 232, 462, 143, 141, 465, 231, 231, 465, 74, 29, 458, 231, 231, 458, 141, 141, 457, 239, 239, 457, 51, 74, 465, 239, 239, 465, 141, 144, 466, 240, 240, 466, 77, 52, 467, 240, 240, 467, 144, 144, 468, 241, 241, 468, 29, 77, 466, 241, 241, 466, 144, 145, 469, 242, 242, 469, 77, 23, 470, 242, 242, 470, 145, 145, 471, 240, 240, 471, 52, 77, 469, 240, 240, 469, 145, 146, 472, 243, 243, 472, 76, 52, 473, 243, 243, 473, 146, 146, 474, 238, 238, 474, 22, 76, 472, 238, 238, 472, 146, 144, 475, 237, 237, 475, 76, 29, 468, 237, 237, 468, 144, 144, 467, 243, 243, 467, 52, 76, 475, 243, 243, 475, 144, 147, 476, 244, 244, 476, 75, 53, 477, 244, 244, 477, 147, 147, 478, 235, 235, 478, 29, 75, 476, 235, 235, 476, 147, 148, 479, 234, 234, 479, 75, 20, 480, 234, 234, 480, 148, 148, 481, 244, 244, 481, 53, 75, 479, 244, 244, 479, 148, 149, 482, 245, 245, 482, 77, 53, 483, 245, 245, 483, 149, 149, 484, 242, 242, 484, 23, 77, 482, 242, 242, 482, 149, 147, 485, 241, 241, 485, 77, 29, 478, 241, 241, 478, 147, 147, 477, 245, 245, 477, 53, 77, 485, 245, 245, 485, 147 };
}

void opengl_init() {
        glGenFramebuffers(1, &m_swapchainFramebuffer);

        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &VertexShaderGlsl, nullptr);
        glCompileShader(vertexShader);
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &FragmentShaderGlsl, nullptr);
        glCompileShader(fragmentShader);
        m_program = glCreateProgram();
        glAttachShader(m_program, vertexShader);
        glAttachShader(m_program, fragmentShader);
        glLinkProgram(m_program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        m_modelViewProjectionUniformLocation = glGetUniformLocation(m_program, "ModelViewProjection");
        m_vertexAttribCoords = glGetAttribLocation(m_program, "VertexPos");
        m_vertexAttribColor  = glGetAttribLocation(m_program, "VertexColor");
        glGenVertexArrays(1, &m_vertexArrayObject);
        glBindVertexArray(m_vertexArrayObject);
        glEnableVertexAttribArray(m_vertexAttribCoords);
        glEnableVertexAttribArray(m_vertexAttribColor);

        glGenBuffers(1, &m_planeVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_planeVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Geometry::c_planeVertices), Geometry::c_planeVertices, GL_STATIC_DRAW);
        glGenBuffers(1, &m_planeIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_planeIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Geometry::c_planeIndices), Geometry::c_planeIndices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, m_planeVertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_planeIndexBuffer);

        glGenBuffers(1, &m_cubeVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Geometry::c_cubeVertices), Geometry::c_cubeVertices, GL_STATIC_DRAW);
        glGenBuffers(1, &m_cubeIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Geometry::c_cubeIndices), Geometry::c_cubeIndices, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);

        glGenBuffers(1, &m_sphereVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, m_sphereVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Geometry::c_sphereVertices), Geometry::c_sphereVertices, GL_STATIC_DRAW);
        glGenBuffers(1, &m_sphereIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_sphereIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Geometry::c_sphereIndices), Geometry::c_sphereIndices, GL_STATIC_DRAW);
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

template <typename T, size_t Size>
constexpr size_t ArraySize(const T (&/*unused*/)[Size]) noexcept { return Size; }

void opengl_render_layer(const XrCompositionLayerProjectionView& layerView, const XrSwapchainImageOpenGLKHR* swapchainImage, int index) {
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

        glClearColor(0.184313729f, 0.309803933f, 0.309803933f, 1.0f); //DarkSlateGray
        glClearDepth(1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glUseProgram(m_program);

        const auto& pose = layerView.pose;
        XrMatrix4x4f proj, view, vp, mvp, model, toView;
        XrMatrix4x4f_CreateProjectionFov(&proj, GRAPHICS_OPENGL, layerView.fov, 0.05f, 100.0f);
        XrVector3f scale_one{1.f, 1.f, 1.f};
        XrVector3f zero_vector{ 0.f, 0.f, 0.f};
        XrQuaternionf zero_quaternion{ 0.f, 0.f, 0.f, 0.f};

        XrMatrix4x4f_CreateTranslationRotationScale(&toView, &pose.position, &pose.orientation, &scale_one);
        XrMatrix4x4f_InvertRigidBody(&view, &toView);
        XrMatrix4x4f_Multiply(&vp, &proj, &view);

        glBindVertexArray(m_vertexArrayObject);
        
        XrVector3f plane_scale{5.f, 5.f, 5.f}; //plane
        XrVector3f plane_position{0.f, -4.f, 0.f};
        glBindBuffer(GL_ARRAY_BUFFER, m_planeVertexBuffer);
        glVertexAttribPointer(m_vertexAttribCoords, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), nullptr);
        glVertexAttribPointer(m_vertexAttribColor, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), reinterpret_cast<const void*>(sizeof(XrVector3f)));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_planeIndexBuffer);
        XrMatrix4x4f_CreateTranslationRotationScale(&model, &plane_position, &zero_quaternion, &plane_scale);
        XrMatrix4x4f_Multiply(&mvp, &vp, &model);
        glUniformMatrix4fv(m_modelViewProjectionUniformLocation, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&mvp));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ArraySize(Geometry::c_planeIndices)), GL_UNSIGNED_SHORT, nullptr);

        XrVector3f cube_position{-.2f, 0.f, -1.f}; //cube
        XrVector3f cube_scale{ .25f, .25f, .25f };
        glBindBuffer(GL_ARRAY_BUFFER, m_cubeVertexBuffer);
        glVertexAttribPointer(m_vertexAttribCoords, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), nullptr);
        glVertexAttribPointer(m_vertexAttribColor, 3, GL_FLOAT, GL_FALSE, sizeof(Geometry::Vertex), reinterpret_cast<const void*>(sizeof(XrVector3f)));
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeIndexBuffer);
        XrMatrix4x4f_CreateTranslationRotationScale(&model, &cube_position, &zero_quaternion, &cube_scale);
        XrMatrix4x4f_Multiply(&mvp, &vp, &model);
        glUniformMatrix4fv(m_modelViewProjectionUniformLocation, 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&mvp));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(ArraySize(Geometry::c_cubeIndices)), GL_UNSIGNED_SHORT, nullptr);

        XrVector3f sphere_scale{.25f, .25f, .25f}; //sphere
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
             if (index == 0) { glBlitFramebuffer(0, 0, width, height, 0, 0, windowWidth/2, windowHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST); }
        else if (index == 1) { glBlitFramebuffer(0, 0, width, height, windowWidth/2, 0, windowWidth, windowHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST); }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        static int everyOther = 0;
        if ((everyOther++ & 1) != 0) { SwapBuffers(hDC);}
}

void opengl_shutdown() {
    glDeleteFramebuffers(1, &m_swapchainFramebuffer);
    glDeleteProgram(m_program);
    glDeleteVertexArrays(1, &m_vertexArrayObject);
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
    if (!openxr_init()) { return 1; }
    opengl_init();

    static bool quit = false;
	while (!quit) {
		openxr_poll_events(quit);
		if (xr_running) {
			openxr_render_frame();
		}
	}

	openxr_shutdown();
    opengl_shutdown();

	return 0;
}

#endif