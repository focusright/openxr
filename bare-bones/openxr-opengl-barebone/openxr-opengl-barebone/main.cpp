#pragma comment( lib, "OpenGL32.lib" )

#define no_init_all

#include <iostream>
#include <vector>
#include <thread>
#include <map>
#include <array>
#include <windows.h>
#include <gl/GL.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

namespace Side {
    const int LEFT = 0;
    const int RIGHT = 1;
    const int COUNT = 2;
}

struct Swapchain {
    XrSwapchain handle;
    int32_t width;
    int32_t height;
};

struct InputState {
    XrActionSet actionSet{XR_NULL_HANDLE};
    XrAction grabAction{XR_NULL_HANDLE};
    XrAction poseAction{XR_NULL_HANDLE};
    XrAction vibrateAction{XR_NULL_HANDLE};
    XrAction quitAction{XR_NULL_HANDLE};
    std::array<XrPath, Side::COUNT> handSubactionPath;
    std::array<XrSpace, Side::COUNT> handSpace;
    std::array<float, Side::COUNT> handScale = {{1.0f, 1.0f}};
    std::array<XrBool32, Side::COUNT> handActive;
};

XrInstance m_instance{XR_NULL_HANDLE};
XrSession m_session{XR_NULL_HANDLE};
XrSpace m_appSpace{XR_NULL_HANDLE};
XrFormFactor m_formFactor{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
XrViewConfigurationType m_viewConfigType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
XrEnvironmentBlendMode m_environmentBlendMode{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};
XrSystemId m_systemId{XR_NULL_SYSTEM_ID};
std::vector<XrViewConfigurationView> m_configViews;
std::vector<Swapchain> m_swapchains;
std::map<XrSwapchain, std::vector<XrSwapchainImageBaseHeader*>> m_swapchainImages;
std::vector<XrView> m_views;
int64_t m_colorSwapchainFormat{-1};
std::vector<XrSpace> m_visualizedSpaces;
XrSessionState m_sessionState{XR_SESSION_STATE_UNKNOWN};
bool m_sessionRunning{false};
XrEventDataBuffer m_eventDataBuffer;
InputState m_input;

void program_CreateInstance() {

}

void program_InitializeSystem() {

}

void program_InitializeSession() {

}

void program_CreateSwapchains() {

}

void program_PollEvents(bool* exitRenderLoop) {

}

void program_PollActions() {

}

void program_RenderFrame() {

}

int main(int argc, char* argv[]) {
    static bool quitKeyPressed = false;
    auto exitPollingThread = std::thread{[] {
        std::cout << "Press any key to shutdown..." << std::endl;
        (void)getchar();
        quitKeyPressed = true;
    }}; exitPollingThread.detach();

    program_CreateInstance();
    program_InitializeSystem();
    program_InitializeSession();
    program_CreateSwapchains();

    do {
        bool exitRenderLoop = false;
        program_PollEvents(&exitRenderLoop);
        if (exitRenderLoop) { break; }
        if (m_sessionRunning) {
            program_PollActions();
            program_RenderFrame();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    } while (!quitKeyPressed);
    return 0;
}