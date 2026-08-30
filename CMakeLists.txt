cmake_minimum_required(VERSION 3.10)
project(GPD4Rotate)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Runtime tools (not link-time deps): iio-sensor-proxy (monitor-sensor), dbus (dbus-send).
# Inotify is the kernel API from <sys/inotify.h>; inotify-tools is not required.
add_executable(GPD4Rotate gpd4rotate.cpp)
