#include "say.h"
#include "schedule.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

static int grok_bot_is_driving(void) {
#ifdef _WIN32
    char appdata[MAX_PATH];
    char path[MAX_PATH];
    FILE *file;
    char buf[512];
    size_t nread;
    const char *signed_in;
    const char *pid_key;
    unsigned long pid = 0;
    HANDLE process;

    DWORD got = GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH);
    if (got == 0 || got >= MAX_PATH) {
        fprintf(stderr, "peer: APPDATA is missing\n");
        return 0;
    }
    snprintf(path, sizeof(path), "%s\\Grok Bot\\desktop-status.json", appdata);
    file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "peer: Grok Bot status file is missing\n");
        return 0;
    }
    nread = fread(buf, 1, sizeof(buf) - 1, file);
    fclose(file);
    buf[nread] = '\0';
    signed_in = strstr(buf, "\"signedIn\":true");
    if (signed_in == NULL) {
        signed_in = strstr(buf, "\"signedIn\": true");
    }
    if (signed_in == NULL) {
        fprintf(stderr, "peer: Grok Bot is not signed in\n");
        return 0;
    }
    pid_key = strstr(buf, "\"pid\":");
    if (pid_key == NULL) {
        fprintf(stderr, "peer: Grok Bot status has no pid\n");
        return 0;
    }
    pid = strtoul(pid_key + 6, NULL, 10);
    if (pid == 0) {
        fprintf(stderr, "peer: Grok Bot pid is empty\n");
        return 0;
    }
    process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)pid);
    if (process == NULL) {
        fprintf(stderr, "peer: Grok Bot pid %lu is not running\n", pid);
        return 0;
    }
    CloseHandle(process);
    fprintf(stderr, "driver=grok-bot pid=%lu signedIn=true\n", pid);
    return 1;
#else
    fprintf(stderr, "peer: this stand-in expects the Windows Grok Bot app\n");
    return 0;
#endif
}

static int orca_is_driving(void) {
#ifdef _WIN32
    HANDLE snap;
    PROCESSENTRY32 entry;
    int found = 0;

    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "peer: cannot list processes\n");
        return 0;
    }
    entry.dwSize = sizeof(entry);
    if (Process32First(snap, &entry)) {
        do {
            if (_stricmp(entry.szExeFile, "Orca.exe") == 0) {
                found = 1;
                break;
            }
        } while (Process32Next(snap, &entry));
    }
    CloseHandle(snap);
    if (!found) {
        fprintf(stderr, "peer: Orca process is not running\n");
        return 0;
    }
    fprintf(stderr, "driver=orca\n");
    return 1;
#else
    fprintf(stderr, "peer: Orca process check expects Windows\n");
    return 0;
#endif
}

static const char *cli_driver(int argc, char **argv) {
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--driver") == 0) {
            if (i + 1 >= argc) {
                return "";
            }
            return argv[i + 1];
        }
        if (strncmp(argv[i], "--driver=", 9) == 0) {
            return argv[i] + 9;
        }
    }
    return NULL;
}

static const char *chosen_driver(int argc, char **argv) {
    const char *from_cli = cli_driver(argc, argv);
    const char *from_env;
    if (from_cli != NULL) {
        return from_cli;
    }
    from_env = getenv("POTETOBOT_DRIVER");
    if (from_env != NULL && from_env[0] != '\0') {
        return from_env;
    }
    return "grok-bot";
}

#ifdef _WIN32
static HANDLE open_board_port(void) {
    char name[32];
    char path[64];
    HANDLE handle;
    DCB dcb;
    COMMTIMEOUTS timeouts;
    DWORD got = GetEnvironmentVariableA("POTETOBOT_PORT", name, sizeof(name));
    if (got == 0 || got >= sizeof(name)) {
        return INVALID_HANDLE_VALUE;
    }
    snprintf(path, sizeof(path), "\\\\.\\%s", name);
    handle = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "peer: cannot open %s\n", name);
        return INVALID_HANDLE_VALUE;
    }
    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(handle, &dcb)) {
        CloseHandle(handle);
        return INVALID_HANDLE_VALUE;
    }
    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    SetCommState(handle, &dcb);
    memset(&timeouts, 0, sizeof(timeouts));
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 400;
    timeouts.WriteTotalTimeoutConstant = 400;
    SetCommTimeouts(handle, &timeouts);
    return handle;
}

static void forward_line(HANDLE port, const char *line) {
    DWORD written = 0;
    char ack[256];
    DWORD got = 0;
    ULONGLONG start;
    if (port == INVALID_HANDLE_VALUE) {
        return;
    }
    WriteFile(port, line, (DWORD)strlen(line), &written, NULL);
    start = GetTickCount64();
    while (GetTickCount64() - start < 2500) {
        if (!ReadFile(port, ack, sizeof(ack) - 1, &got, NULL) || got == 0) {
            continue;
        }
        ack[got] = '\0';
        fprintf(stderr, "board: %s", ack);
        if (ack[got - 1] != '\n') {
            fputc('\n', stderr);
        }
        if (strstr(ack, "ACK ") != NULL) {
            break;
        }
    }
}
#endif

int main(int argc, char **argv) {
    char line[512];
    const char *driver;
#ifdef _WIN32
    HANDLE port = INVALID_HANDLE_VALUE;
#endif

    driver = chosen_driver(argc, argv);
    if (strcmp(driver, "grok-bot") == 0) {
        if (!grok_bot_is_driving()) {
            return 2;
        }
    } else if (strcmp(driver, "orca") == 0) {
        if (!orca_is_driving()) {
            return 2;
        }
    } else {
        fprintf(stderr, "peer: unknown driver\n");
        return 2;
    }
#ifdef _WIN32
    port = open_board_port();
#endif
    while (fgets(line, sizeof(line), stdin) != NULL) {
        poteto_frame_t frame;
        if (line[0] == '\0' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        if (poteto_apply_line(line, &frame) == 0) {
            printf("%s %s\n", frame.character, frame.expression);
            fflush(stdout);
#ifdef _WIN32
            forward_line(port, line);
            if (strcmp(frame.expression, "thinking") == 0) {
                forward_line(port, "TXT v1 thinking\n");
            }
#endif
            continue;
        }
        if (strncmp(line, "TXT v1", 6) == 0 && (line[6] == ' ' || line[6] == '\t' || line[6] == '\r' ||
                                                 line[6] == '\n' || line[6] == '\0')) {
#ifdef _WIN32
            forward_line(port, line);
#endif
            continue;
        }
        {
            char phase[POTETO_SAY_PHASE_MAX];
            char text[POTETO_SAY_TEXT_MAX];
            if (poteto_apply_say(line, phase, sizeof(phase), text, sizeof(text)) != 0) {
                fprintf(stderr, "peer: rejected line\n");
                return 3;
            }
            printf("say %s\n", phase);
            fflush(stdout);
#ifdef _WIN32
            forward_line(port, line);
#endif
        }
    }
#ifdef _WIN32
    if (port != INVALID_HANDLE_VALUE) {
        CloseHandle(port);
    }
#endif
    return 0;
}
