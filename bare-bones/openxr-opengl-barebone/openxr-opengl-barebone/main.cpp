#pragma comment( lib, "OpenGL32.lib" )

#include <iostream>
#include <vector>
#include <thread>
#include <windows.h>
#include <stdio.h>
#include <gl/GL.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

bool m_sessionRunning{false};

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
        std::cout << "Press Enter to shutdown...";
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