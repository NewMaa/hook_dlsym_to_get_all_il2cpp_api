#include <stdio.h>
#include <inttypes.h>
#include <android/log.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <elf.h>
#include <ctype.h>
#include "dobby.h"

#define LOG_TAG "IL2CPP_HOOK"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

static void* (*g_orig_dlsym)(void* handle, const char* symbol) = NULL;
static void* g_il2cpp_base = NULL;
static char g_file_path[256] = {0};

static bool is_valid_elf_header(Elf32_Ehdr* ehdr) {
    return ehdr && 
           ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
           ehdr->e_ident[EI_MAG1] == ELFMAG1 &&
           ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
           ehdr->e_ident[EI_MAG3] == ELFMAG3;
}

static void* get_il2cpp_base_from_maps() {
    FILE* maps = fopen("/proc/self/maps", "r");
    if (!maps) return NULL;

    char line[1024];
    void* base_addr = NULL;

    while (fgets(line, sizeof(line), maps)) {
        if (strstr(line, "libil2cpp.so")) {
            sscanf(line, "%p", &base_addr);
            break;
        }
    }
    fclose(maps);
    
    return base_addr;
}

static bool is_il2cpp_symbol(const char* symbol) {
    if (!symbol) return false;
    
    const char* ptr = symbol;
    while (*ptr) {
        if ((ptr[0] == 'i' || ptr[0] == 'I') &&
            (ptr[1] == 'l' || ptr[1] == 'L') &&
            (ptr[2] == '2') &&
            (ptr[3] == 'c' || ptr[3] == 'C') &&
            (ptr[4] == 'p' || ptr[4] == 'P') &&
            (ptr[5] == 'p' || ptr[5] == 'P')) {
            return true;
        }
        ptr++;
    }
    return false;
}

static void get_package_name() {
    int fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd == -1) return;
    
    char package_name[128] = {0};
    if (read(fd, package_name, sizeof(package_name) - 1) <= 0) {
        close(fd);
        return;
    }
    close(fd);
    
    snprintf(g_file_path, sizeof(g_file_path), 
             "/sdcard/Android/data/%s/il2cpp_offsets.txt", package_name);
}

static void init_output_file() {
    unlink(g_file_path);
    int fd = open(g_file_path, O_CREAT | O_WRONLY, 0644);
    if (fd != -1) close(fd);
}

static void write_symbol_info(const char* symbol_name, void* address) {
    int fd = open(g_file_path, O_WRONLY | O_APPEND);
    if (fd == -1) return;
    
    char buffer[512];
    int len;
    
    if (g_il2cpp_base) {
        uintptr_t offset = (uintptr_t)address - (uintptr_t)g_il2cpp_base;
        len = snprintf(buffer, sizeof(buffer), 
                      "名字：%s\n地址：%p\n偏移：0x%" PRIxPTR "\n\n", 
                      symbol_name, address, offset);
    } else {
        len = snprintf(buffer, sizeof(buffer), 
                      "名字：%s\n地址：%p\n偏移：unknown\n\n", 
                      symbol_name, address);
    }
    
    write(fd, buffer, len);
    close(fd);
}

static void* hook_dlsym(void* handle, const char* symbol) {
    void* result = g_orig_dlsym(handle, symbol);
    
    if (symbol && is_il2cpp_symbol(symbol) && result) {
        if (!g_il2cpp_base) {
            g_il2cpp_base = get_il2cpp_base_from_maps();
        }
        write_symbol_info(symbol, result);
    }
    
    return result;
}

static void hook_with_dobby() {
    void* dlsym_addr = DobbySymbolResolver("libdl.so", "dlsym");
    if (!dlsym_addr) {
        dlsym_addr = DobbySymbolResolver(NULL, "dlsym");
    }
    
    if (dlsym_addr) {
        DobbyHook(dlsym_addr, (void*)hook_dlsym, (void**)&g_orig_dlsym);
    }
}

__attribute__((constructor)) 
static void init() {
    get_package_name();
    init_output_file();
    hook_with_dobby();
}