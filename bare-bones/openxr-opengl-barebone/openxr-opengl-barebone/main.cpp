#pragma comment( lib, "OpenGL32.lib" )

#include <iostream>
#include <vector>
#include <thread>
#include <windows.h>
#include <stdio.h>
#include <gl/GL.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

void openxr_init() {
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