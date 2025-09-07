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
        std::cerr << strerror(errno);
        return -1;
    } // for linux
      // for window WSAGetLastError()
    struct sockaddr_in address;
    address.sin_family      = AF_INET;
    address.sin_port        = htons(8000);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverFileDescriptor, (struct sockaddr*) &address, sizeof(address)) == -1) {
        std::cerr << strerror(errno);
        return -1;
    }
    listen(serverFileDescriptor, 6);
    int                client_socket;
    struct sockaddr_in client_address;
    socklen_t          client_addrlen = sizeof(client_address);
    client_socket =
        accept(serverFileDescriptor, (struct sockaddr*) &client_address, &client_addrlen);
    std::cerr << strerror(errno);

    _window_ currentWindow = getActiveWindow(display);
    int      cycleCount    = 0;
    while (true) {
        if (cycleCount % 13 == 0 && hasWindowChanged(currentWindow)) {
            sendDataToClient(currentWindow, client_socket);
            currentWindow = getActiveWindow(display);
        }
        char revicedData[512];
        int  bytesRecived = recv(client_socket, revicedData, sizeof(revicedData), 0);
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
