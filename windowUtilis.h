#ifndef WINDOWUTILIS_H
#define WINDOWUTILIS_H

#include <SFML/Graphics.hpp>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <imgui.h>
#include <SFML/Window/WindowEnums.hpp>
#include <cctype>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

const int NULL_VALUE = std::numeric_limits<int>::min();

struct _window_ {
  private:
  public:
    int          x;
    int          y;
    unsigned int width;
    unsigned int height;
    std::string  programName;

    _window_()
        : x(NULL_VALUE)
        , y(NULL_VALUE)
        , width(NULL_VALUE)
        , height(NULL_VALUE)
        , programName() {}
};

_window_ getActiveWindow(Display* display);

bool hasWindowChanged(_window_& newWindowInfo);

std::string receiveDataFromClient(int socketFd);

_window_ parseWindowCommand(const std::string& command);

std::string buildWindowCommand(const _window_& windowInfo);

void sendDataToClient(const _window_& windowInfo, int socketFd);

Window getWindowIdByClass(Display* display, std::string& targetClassName);

void resizeWindow(Display* display, _window_& windowInfo);

#endif
