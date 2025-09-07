#include <SFML/Graphics.hpp>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <imgui.h>
#include <SFML/Window/WindowEnums.hpp>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "windowUtilis.h"

int main() {
    Display* display              = XOpenDisplay(NULL);
    int      serverFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFileDescriptor == -1) {
        std::cerr << strerror(errno); // for linux
        return -1;
    } // for window WSAGetLastError()
    struct sockaddr_in address;
    address.sin_family      = AF_INET;
    address.sin_port        = htons(8000);
    address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(serverFileDescriptor, (struct sockaddr*) &address, sizeof(address)) == -1) {
        std::cerr << strerror(errno);
        return -1;
    }
    _window_ currentWindow = getActiveWindow(display);
    int      cycleCount;
    while (true) {
        if (cycleCount % 17 == 0 && hasWindowChanged(currentWindow)) {
            sendDataToClient(currentWindow, serverFileDescriptor);
            currentWindow = getActiveWindow(display);
        }

        char revicedData[512];
        int  bytesRecived = recv(serverFileDescriptor, revicedData, sizeof(revicedData), 0);
        if (bytesRecived > 0) {
            std::string command = std::string(revicedData, bytesRecived);
            std::cout << command;
            _window_ newWindow = parseWindowCommand(command);
            resizeWindow(display, newWindow);
        }

        sleep(1);
        cycleCount++;
    }
    close(serverFileDescriptor);
    XCloseDisplay(display);
}
