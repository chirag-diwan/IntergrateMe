#include "windowUtilis.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <arpa/inet.h>
#include <cctype>
#include <imgui.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

// Global active window identifier (X11 Window ID)
Window globalActiveWindowId = Window();

// Global copy of active window data
_window_ globalActiveWindowInfo;

_window_ getActiveWindow(Display* display) {
    _window_ currentWindowInfo{};

    if (display == nullptr) {
        std::cerr << "Failed to initialize X11 display";
        return {};
    }

    Window rootWindow = DefaultRootWindow(display);

    Atom           actualType;
    int            actualFormat;
    unsigned long  itemCount;
    unsigned long  bytesAfter;
    unsigned char* propertyData = nullptr;
    int            revertTo;
    Atom           netActiveWindowAtom = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);
    unsigned long  activeWindowId;

    if (netActiveWindowAtom == None) {
        std::cerr << "_NET_ACTIVE_WINDOW not supported, falling back to XGetInputFocus";
        XGetInputFocus(display, &activeWindowId, &revertTo);
    } else {
        int status = XGetWindowProperty(display,
                                        rootWindow,
                                        netActiveWindowAtom,
                                        0,
                                        1,
                                        False,
                                        XA_WINDOW,
                                        &actualType,
                                        &actualFormat,
                                        &itemCount,
                                        &bytesAfter,
                                        &propertyData);
        if (status == Success && propertyData != nullptr) {
            activeWindowId = *(Window*) propertyData;
        } else {
            std::cerr << "XGetWindowProperty failed or returned nullptr";
        }
    }

    if (globalActiveWindowId != activeWindowId)
        globalActiveWindowId = activeWindowId;

    int          posX, posY;
    unsigned int winWidth, winHeight;
    unsigned int borderWidth;
    unsigned int depth;
    Window       rootWindowReturn;

    XGetGeometry(display,
                 activeWindowId,
                 &rootWindowReturn,
                 &posX,
                 &posY,
                 &winWidth,
                 &winHeight,
                 &borderWidth,
                 &depth);

    unsigned char* classNameData = nullptr;
    Atom           wmClassAtom   = XInternAtom(display, "WM_CLASS", False);
    std::string    windowClassName;

    int status = XGetWindowProperty(display,
                                    activeWindowId,
                                    wmClassAtom,
                                    0,
                                    (~0L),
                                    False,
                                    AnyPropertyType,
                                    &actualType,
                                    &actualFormat,
                                    &itemCount,
                                    &bytesAfter,
                                    &classNameData);

    windowClassName = std::string(reinterpret_cast<char*>(classNameData), itemCount);

    XFree(propertyData);
    XFree(classNameData);

    currentWindowInfo.x           = posX;
    currentWindowInfo.y           = posY;
    currentWindowInfo.width       = winWidth;
    currentWindowInfo.height      = winHeight;
    currentWindowInfo.programName = windowClassName;

    return currentWindowInfo;
}

bool hasWindowChanged(_window_& newWindowInfo) {
    if (globalActiveWindowInfo.x != newWindowInfo.x ||
        globalActiveWindowInfo.y != newWindowInfo.y ||
        globalActiveWindowInfo.width != newWindowInfo.width ||
        globalActiveWindowInfo.height != newWindowInfo.height) {
        globalActiveWindowInfo = newWindowInfo;
        return true;
    }
    return false;
}

std::string receiveDataFromClient(int socketFd) {
    char buffer[512];
    recv(socketFd, (void*) &buffer, sizeof(buffer), 0);
    return std::string(buffer, sizeof(buffer));
}

// format: programName/x/y/w/h
_window_ parseWindowCommand(const std::string& command) {
    _window_     windowInfo;
    unsigned int sepIndex  = command.find("/");
    windowInfo.programName = command.substr(0, sepIndex);

    int index  = sepIndex + 1;
    int number = 0;

    while (index < command.size()) {
        if (std::isdigit(command[index])) {
            number = number * 10 + command[index] - '0';
            index++;
            continue;
        }
        if (windowInfo.x == NULL_VALUE) {
            windowInfo.x = number;
        } else if (windowInfo.y == NULL_VALUE) {
            windowInfo.y = number;
        } else if (windowInfo.width == NULL_VALUE) {
            windowInfo.width = number;
        } else {
            windowInfo.height = number;
        }
        number = 0;
        index++;
    }

    if (number != 0) {
        if (windowInfo.x == NULL_VALUE)
            windowInfo.x = number;
        else if (windowInfo.y == NULL_VALUE)
            windowInfo.y = number;
        else if (windowInfo.width == NULL_VALUE)
            windowInfo.width = number;
        else
            windowInfo.height = number;
    }
    return windowInfo;
}

std::string buildWindowCommand(const _window_& windowInfo) {
    return (windowInfo.programName + "/" + std::to_string(windowInfo.x) + "/" +
            std::to_string(windowInfo.y) + "/" + std::to_string(windowInfo.width) + "/" +
            std::to_string(windowInfo.height));
}

void sendDataToClient(const _window_& windowInfo, int socketFd) {
    std::string command = buildWindowCommand(windowInfo);
    send(socketFd, (void*) command.c_str(), strlen(command.c_str()), 0);
}

Window getWindowIdByClass(Display* display, std::string& targetClassName) {
    Window       root = DefaultRootWindow(display);
    Window       rootReturn, parentReturn;
    Window*      children;
    unsigned int childCount;

    Atom wmClassAtom = XInternAtom(display, "WM_CLASS", False);

    XQueryTree(display, root, &rootReturn, &parentReturn, &children, &childCount);
    for (int i = 0; i < childCount; i++) {
        Window         child = children[i];
        Atom           type;
        int            format;
        unsigned long  nitems;
        unsigned long  bytesAfter;
        unsigned char* propData;

        int status = XGetWindowProperty(display,
                                        child,
                                        wmClassAtom,
                                        0,
                                        (~0L),
                                        False,
                                        AnyPropertyType,
                                        &type,
                                        &format,
                                        &nitems,
                                        &bytesAfter,
                                        &propData);

        if (status == Success) {
            std::string className(reinterpret_cast<char*>(propData), nitems);
            if (targetClassName == className) {
                XFree(propData);
                return child;
            }
            XFree(propData);
        } else {
            char* fallbackName;
            XFetchName(display, child, &fallbackName);
            std::string windowNameFallback(fallbackName);
            if (targetClassName == windowNameFallback) {
                XFree(fallbackName);
                return child;
            }
            XFree(fallbackName);
        }
    }
    return None;
}

void resizeWindow(Display* display, _window_& windowInfo) {
    Window targetWindow = getWindowIdByClass(display, windowInfo.programName);
    XMoveResizeWindow(
        display, targetWindow, windowInfo.x, windowInfo.y, windowInfo.width, windowInfo.height);
}
