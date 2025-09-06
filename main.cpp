#include <SFML/Graphics.hpp>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <imgui.h>
#include <SFML/Window/WindowEnums.hpp>
#include <cstdio>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

Window Active_Window_ID; // every program has its own active_window_id global , and other functions
                         // can look it up

struct _window_ {
    int          x;
    int          y;
    unsigned int width;
    unsigned int height;
    std::string  programName; // string is of 24 size
};

_window_ getActiveWindow(Display* display) {

    _window_ r{};

    Window parentRootWindowId = DefaultRootWindow(display);

    if (display == nullptr) {
        std::cerr << "Failed to initialize window";
        return {};
    }

    Atom           actual_type_return;
    int            actual_format_return;
    unsigned long  nitems_return;
    unsigned long  bytes_after_return;
    unsigned char* prop_return     = nullptr;
    Atom           netActiveWindow = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    unsigned long  activeWindowID;

    int status = XGetWindowProperty(display, parentRootWindowId, netActiveWindow, 0, 1, False,
                                    XA_WINDOW, &actual_type_return, &actual_format_return,
                                    &nitems_return, &bytes_after_return, &prop_return);

    if (status == Success && prop_return != nullptr) {
        activeWindowID = *(Window*) prop_return;
    }

    if (Active_Window_ID != activeWindowID)
        Active_Window_ID = activeWindowID;

    int          x;
    int          y;
    unsigned int width;
    unsigned int height;
    unsigned int borderWidth;
    unsigned int depth;
    Window       rootReturn; // contains root window ID

    XGetGeometry(display, activeWindowID, &rootReturn, &x, &y, &width, &height, &borderWidth,
                 &depth);

    unsigned char* program_name = nullptr;
    Atom           WMName       = XInternAtom(display, "WM_CLASS", False);
    std::string    windowName;

    status = XGetWindowProperty(display, activeWindowID, WMName, 0, (~0L), False, AnyPropertyType,
                                &actual_type_return, &actual_format_return, &nitems_return,
                                &bytes_after_return, &program_name);

    windowName = std::string(reinterpret_cast<char*>(program_name), nitems_return);

    XFree(prop_return);
    XFree(program_name);

    r.x           = x;
    r.y           = y;
    r.width       = width;
    r.height      = height;
    r.programName = windowName;

    return r;
}

void resizeWindow(Window windowId, Display* display, _window_ windowData) {
    XMoveResizeWindow(display, Active_Window_ID, windowData.x, windowData.y, windowData.width,
                      windowData.height);
}

// code sending type.
// programName
//_window_

void SendDataToClient() {}

std::string ReciveDataFromClient(int serverFD) {
    char buffer[512];
    read(serverFD, (void*) &buffer, sizeof(buffer));
    std::string command = std::string(buffer, sizeof(buffer));
    return command;
}

_window_ parseCommandForWindow(std::string command) {}

void executeClientCommand() {}

int main() {
    int serverFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    std::cerr << strerror(errno); // for linux
                                  // for window WSAGetLastError()
    struct sockaddr_in address;
    address.sin_family      = AF_INET;
    address.sin_port        = htons(8000);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverFileDescriptor, (struct sockaddr*) &address, sizeof(address)) == -1)
        std::cerr << strerror(errno);
    listen(serverFileDescriptor, 6);
    int                client_socket;
    struct sockaddr_in client_address;
    socklen_t          client_addrlen = sizeof(client_address);

    while (true) {
        client_socket =
            accept(serverFileDescriptor, (struct sockaddr*) &client_address, &client_addrlen);
    }
}
