#pragma comment( lib, "OpenGL32.lib" )

#include <iostream>
#include <thread>
#include <windows.h>
#include <stdio.h>
#include <gl/GL.h>

int main(int argc, char* argv[]) {
    static bool quitKeyPressed = false;
    auto exitPollingThread = std::thread{[] {
        std::cout << "Press any key to shutdown...";
        (void)getchar();
        quitKeyPressed = true;
    }}; exitPollingThread.detach();

    do {

    } while (!quitKeyPressed);
    return 0;
}