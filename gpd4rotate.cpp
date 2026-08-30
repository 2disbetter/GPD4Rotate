#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <filesystem>
#include <sys/inotify.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cctype>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace fs = std::filesystem;

// Configuration specific to the GPD Pocket 4
static std::string homeDir() {
    const char* home = std::getenv("HOME");
    return home ? std::string(home) : std::string();
}

const std::string HYPR_DIR = homeDir() + "/.config/hypr";
const std::string TOGGLE_FILE = HYPR_DIR + "/rotation-toggle";
const std::string SCALE_FILE = HYPR_DIR + "/scale";
const std::string HOOK_FILE = HYPR_DIR + "/scripts/gpd4rotate-hook.sh";

const std::string MONITOR_NAME = "eDP-1";
const std::string RESOLUTION = "1600x2560@144";
const std::string TOUCH_DEVICE = "nvtk0603:00-0603:f001";
const std::string DEFAULT_SCALE = "2.0";

// GPD Pocket 4 panel is physically portrait. Laptop mode == transform 3.
const int DEFAULT_TRANSFORM = 3;

static std::string trim(std::string s) {
    auto not_space = [](unsigned char c) { return !std::isspace(c); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
    return s;
}

static bool validScale(const std::string& s) {
    if (s.empty()) return false;
    bool seen_dot = false;
    for (char c : s) {
        if (c == '.') {
            if (seen_dot) return false;
            seen_dot = true;
        } else if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return s != "." && s != "";
}

static std::string readScale() {
    std::ifstream file(SCALE_FILE);
    if (!file.is_open()) return DEFAULT_SCALE;
    std::string value;
    std::getline(file, value);
    value = trim(value);
    return validScale(value) ? value : DEFAULT_SCALE;
}

static void ensureScaleFile() {
    if (fs::exists(SCALE_FILE)) return;
    fs::create_directories(fs::path(SCALE_FILE).parent_path());
    std::ofstream file(SCALE_FILE);
    file << DEFAULT_SCALE << "\n";
}

static bool fileIsExecutable(const std::string& path) {
    struct stat st{};
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR);
}

static void runHook(int transform) {
    if (!fileIsExecutable(HOOK_FILE)) return;
    std::string cmd = HOOK_FILE + " " + std::to_string(transform);
    system(cmd.c_str());
}

// Apply monitor + touch + tablet transforms via the Hyprland 0.55+ Lua API.
// `hyprctl keyword` is a no-op when the config provider is Lua.
void setOrientation(int transform) {
    const std::string scale = readScale();
    const std::string t = std::to_string(transform);

    // Single-quoted Lua strings keep the shell quoting simple.
    std::string batch =
        "hyprctl --batch \""
        "eval hl.monitor({ output = '" + MONITOR_NAME +
        "', mode = '" + RESOLUTION +
        "', position = '0x0', scale = " + scale +
        ", transform = " + t + " }) ; "
        "eval hl.config({ input = { "
        "touchdevice = { transform = " + t + ", output = '" + MONITOR_NAME + "' }, "
        "tablet = { transform = " + t + ", output = '" + MONITOR_NAME + "' } "
        "} }) ; "
        "eval hl.device({ name = '" + TOUCH_DEVICE + "', transform = " + t + " })"
        "\"";

    system(batch.c_str());
    runHook(transform);
}

bool readToggleState() {
    std::ifstream file(TOGGLE_FILE);
    int state = 0;
    if (file.is_open()) {
        file >> state;
    }
    return state != 0;
}

void ensureToggleFile() {
    if (fs::exists(TOGGLE_FILE)) return;
    fs::create_directories(fs::path(TOGGLE_FILE).parent_path());
    std::ofstream file(TOGGLE_FILE);
    file << "1\n";
}

std::string getCurrentOrientation() {
    FILE* p = popen(
        "dbus-send --system --print-reply "
        "--dest=net.hadess.SensorProxy /net/hadess/SensorProxy "
        "org.freedesktop.DBus.Properties.Get "
        "string:\"net.hadess.SensorProxy\" "
        "string:\"AccelerometerOrientation\"",
        "r");
    if (!p) {
        std::cerr << "Failed to query current orientation\n";
        return "";
    }

    char buf[512] = {0};
    while (fgets(buf + strlen(buf), static_cast<int>(sizeof(buf) - strlen(buf)), p) != nullptr) {}
    pclose(p);

    std::string output(buf);
    size_t variant_pos = output.find("variant");
    if (variant_pos == std::string::npos) return "";

    size_t quote_pos = output.find('"', variant_pos);
    if (quote_pos == std::string::npos) return "";

    size_t end_quote_pos = output.find('"', quote_pos + 1);
    if (end_quote_pos == std::string::npos) return "";

    return output.substr(quote_pos + 1, end_quote_pos - quote_pos - 1);
}

// Pocket 4 accelerometer "normal" is laptop mode (keyboard toward user).
int transformForOrientation(const std::string& orientation) {
    if (orientation.find("normal") != std::string::npos) return 3;
    if (orientation.find("right-up") != std::string::npos) return 2;
    if (orientation.find("left-up") != std::string::npos) return 0;
    if (orientation.find("bottom-up") != std::string::npos) return 1;
    return -1;
}

void applyOrientationString(const std::string& orientation, std::string& last_orientation) {
    int transform = transformForOrientation(orientation);
    if (transform == -1) return;
    setOrientation(transform);
    last_orientation = orientation;
}

int main() {
    if (homeDir().empty()) {
        std::cerr << "HOME is not set\n";
        return 1;
    }

    ensureToggleFile();
    ensureScaleFile();

    int fd = inotify_init();
    if (fd < 0) {
        std::cerr << "Failed to initialize inotify: " << std::strerror(errno) << "\n";
        return 1;
    }

    int wd = inotify_add_watch(fd, TOGGLE_FILE.c_str(), IN_MODIFY);
    if (wd < 0) {
        std::cerr << "Failed to add inotify watch for " << TOGGLE_FILE << ": "
                  << std::strerror(errno) << "\n";
        close(fd);
        return 1;
    }

    FILE* pipe = popen("monitor-sensor", "r");
    if (!pipe) {
        std::cerr << "Failed to start monitor-sensor: " << std::strerror(errno) << "\n";
        inotify_rm_watch(fd, wd);
        close(fd);
        return 1;
    }

    int pipe_fd = fileno(pipe);
    int flags = fcntl(pipe_fd, F_GETFL, 0);
    fcntl(pipe_fd, F_SETFL, flags | O_NONBLOCK);

    bool rotation_enabled = readToggleState();
    std::string last_orientation;

    // Apply an orientation immediately so the portrait panel is usable
    // without having to tilt the device first.
    if (rotation_enabled) {
        std::string current = getCurrentOrientation();
        int transform = transformForOrientation(current);
        if (transform == -1) transform = DEFAULT_TRANSFORM;
        setOrientation(transform);
        if (!current.empty() && current != "undefined") last_orientation = current;
    } else {
        setOrientation(DEFAULT_TRANSFORM);
    }

    char line[256];

    while (true) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(fd, &set);
        int max_fd = fd;

        if (rotation_enabled) {
            FD_SET(pipe_fd, &set);
            max_fd = std::max(max_fd, pipe_fd);
        }

        struct timeval timeout = {0, 100000}; // 100ms
        int ret = select(max_fd + 1, &set, nullptr, nullptr, &timeout);
        if (ret < 0) continue;

        if (FD_ISSET(fd, &set)) {
            char event_buf[512];
            read(fd, event_buf, sizeof(event_buf));

            bool new_state = readToggleState();
            if (new_state != rotation_enabled) {
                rotation_enabled = new_state;
                if (rotation_enabled) {
                    while (fgets(line, sizeof(line), pipe) != nullptr) {}
                    std::string current = getCurrentOrientation();
                    if (!current.empty() && current != "undefined" && current != last_orientation) {
                        applyOrientationString(current, last_orientation);
                    }
                } else {
                    last_orientation.clear();
                }
            }
        }

        if (rotation_enabled && FD_ISSET(pipe_fd, &set)) {
            while (fgets(line, sizeof(line), pipe) != nullptr) {
                std::string orientation(line);
                if (!orientation.empty() && orientation.back() == '\n') orientation.pop_back();

                size_t changed_pos = orientation.find("changed: ");
                if (changed_pos != std::string::npos) {
                    orientation = orientation.substr(changed_pos + 9);
                }

                if (orientation != last_orientation) {
                    applyOrientationString(orientation, last_orientation);
                }
            }
        }
    }

    pclose(pipe);
    inotify_rm_watch(fd, wd);
    close(fd);
    return 0;
}
