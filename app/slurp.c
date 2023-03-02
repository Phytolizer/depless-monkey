#include "./slurp.h"

#include <monkey/string.h>
#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

struct platform_file {
#ifdef _WIN32
    HANDLE handle;
#else
    int fd;
#endif
    bool valid;
};

static struct platform_file platform_fopen(struct string path) {
#ifdef _WIN32
    HANDLE handle = CreateFileA(
        path.data,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    return (struct platform_file){
        .handle = handle,
        .valid = handle != INVALID_HANDLE_VALUE,
    };
#else
    int fd = open(path.data, O_RDONLY);
    return (struct platform_file){.fd = fd, .valid = fd != -1};
#endif
}

static void platform_fclose(struct platform_file file) {
    if (file.valid) {
#ifdef _WIN32
        CloseHandle(file.handle);
#else
        close(file.fd);
#endif
    }
}

static size_t get_filelen(struct platform_file f) {
#ifdef _WIN32
    LARGE_INTEGER size;
    GetFileSizeEx(f.handle, &size);
    return (size_t)size.QuadPart;
#else
    struct stat st;
    fstat(f.fd, &st);
    return (size_t)st.st_size;
#endif
}

static size_t platform_fread(struct platform_file f, char* buf, size_t len) {
#ifdef _WIN32
    DWORD read;
    ReadFile(f.handle, buf, (DWORD)len, &read, NULL);
    return (size_t)read;
#else
    return (size_t)read(f.fd, buf, len);
#endif
}

struct string slurp_file(struct string path) {
    struct platform_file f = platform_fopen(path);
    if (!f.valid) {
        return STRING_REF("");
    }

    size_t len = get_filelen(f);

    char* buf = malloc(len);
    if (buf == NULL) {
        return STRING_REF("");
    }

    size_t read = platform_fread(f, buf, len);
    if (read != len) {
        return STRING_REF("");
    }

    platform_fclose(f);
    return STRING_OWN_DATA(buf, len);
}
