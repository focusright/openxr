[VC++ project setting values]
openxr_loader.lib
$(SolutionDir)include
$(SolutionDir)lib
xcopy /y /d  "$(SolutionDir)lib\*.dll" "$(OutDir)"





[OpenXR SDK initalize sequence]

xrEnumerateInstanceExtensionProperties
XrExtensionProperties
XrInstanceCreateInfo

XrInstanceProperties

xrGetSystem

xrEnumerateViewConfigurations
xrGetViewConfigurationProperties
xrEnumerateViewConfigurationViews
xrEnumerateEnvironmentBlendModes

xrGetInstanceProcAddr

(Create window)
(initialize OpenGL)

xrCreateSession
xrEnumerateReferenceSpaces
xrCreateActionSet
xrCreateAction
xrSuggestInteractionProfileBindings
xrCreateActionSpace
xrAttachSessionActionSets

xrCreateReferenceSpace

xrGetSystemProperties

xrEnumerateViewConfigurationViews
xrCreateSwapchain
xrEnumerateSwapchainImages

(poll events)
    xrPollEvent
    xrBeginSession
    xrEndSession

(poll actions)
    xrSyncActions
    xrGetActionStateFloat
    xrGetActionStatePose
    xrGetActionStateBoolean
    xrRequestExitSession

(Render frame)
    xrWaitFrame
    xrBeginFrame
    (Render layer)
        xrLocateViews
        xrLocateSpace
        xrAcquireSwapchainImage
        xrWaitSwapchainImage
        (OpenGL Render)
        xrReleaseSwapchainImage
    xrEndFrame

(Destructor)
    xrDestroySpace
    xrDestroyActionSet
    xrDestroySwapchain
    xrDestroySession
    xrDestroyInstance






[Single main.cpp DX11 project initalize sequence]

xrEnumerateInstanceExtensionProperties
XrExtensionProperties
XrInstanceCreateInfo

xrGetInstanceProcAddr
XrDebugUtilsMessengerCreateInfoEXT

xrGetSystem

xrEnumerateEnvironmentBlendModes
xrCreateSession
xrCreateReferenceSpace
xrEnumerateViewConfigurationViews
xrCreateSwapchain
xrEnumerateSwapchainImages

xrCreateActionSet
xrSuggestInteractionProfileBindings
xrCreateActionSpace
xrAttachSessionActionSets

(initialize D3D)

(poll events)
    xrPollEvent
    xrBeginSession
    xrEndSession

(poll actions)
    xrSyncActions
    xrGetActionStatePose
    xrGetActionStateBoolean
    xrLocateSpace

(update D3D)

(Render frame)
    xrWaitFrame
    xrBeginFrame
    (Render layer)
        xrLocateViews
        xrAcquireSwapchainImage
        xrWaitSwapchainImage
        (D3D render)
        xrReleaseSwapchainImage
    xrEndFrame

(openxr shutdown)
    xrDestroySwapchain
    xrDestroyActionSet

(D3D shutdown)