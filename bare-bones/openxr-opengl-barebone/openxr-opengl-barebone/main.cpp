#pragma comment( lib, "OpenGL32.lib" )

#include <iostream>
#include <vector>
#include <thread>
#include <windows.h>
#include <stdio.h>
#include <gl/GL.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

XrInstance xr_instance = {};
XrSystemId xr_system_id = XR_NULL_SYSTEM_ID;

void openxr_init() {
    XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    xrGetSystem(xr_instance, &systemInfo, &xr_system_id);


    uint32_t ext_count = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &ext_count, nullptr);
    std::vector<XrExtensionProperties> xr_exts(ext_count, { XR_TYPE_EXTENSION_PROPERTIES });
    xrEnumerateInstanceExtensionProperties(nullptr, ext_count, &ext_count, xr_exts.data());
}

int main(int argc, char* argv[]) {
    static bool quitKeyPressed = false;
    auto exitPollingThread = std::thread{[] {
        std::cout << "Press Enter to shutdown...";
        (void)getchar();
        quitKeyPressed = true;
    }}; exitPollingThread.detach();

    openxr_init();

    do {

    } while (!quitKeyPressed);
    return 0;
}