#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <shlwapi.h>
#include <wininet.h>
#include <psapi.h>
#include <cstring>
#include <string>
#include <wmsdkidl.h>
#include <Wmsdk.h>
#include "iathook.h"

#pragma comment(lib, "Shlwapi.lib")
//#pragma comment(lib, "d3dx9.lib")
//#pragma comment(lib, "winmm.lib") // needed for timeBeginPeriod()/timeEndPeriod()


struct _mods {
    char folder[1024];
    int prior;
};
//_mods Mods[10] = { 0,0 };

char cstrBuffer[1024] = { 0 };
char cstrCurrentDir[1024] = { 0 };
char cstrReplaceDir[1024] = { 0 };

BOOL LogFunctions = 1;
BOOL LoadFromMods = 1;
BOOL HookFunctions = 1;

std::string str_replace2(const char* filename, const char* rep, const char* with) {
    std::string s = filename;
    size_t pos = s.find(rep); // encontra a posição da substring antiga
    if(pos != std::string::npos) // se encontrou
    {
        s.replace(pos, strlen(rep), with); // substitui pela nova substring
    }
    return s;
}
BOOL str_replace3(char* filename, const char* rep, const char* with) {
    BOOL ret = false;
    std::string s = filename;
    size_t pos = s.find(rep); // encontra a posição da substring antiga
    if(pos != std::string::npos) // se encontrou
    {
        ret = true;
        s.replace(pos, strlen(rep), with); // substitui pela nova substring
    }
    strcpy(filename, s.c_str());
    return ret;
}

//Define a helper function to get the address of a function from a DLL module
void* get_function_address(const char* module_name, const char* function_name) {
    //Load the DLL module
    HMODULE module = LoadLibraryA(module_name);
    if(module != NULL) {
        //Get the address of the function
        return GetProcAddress(module, function_name);
    }
    else {
        return NULL;
    }
}
void* get_function_address2(HMODULE module, const char* function_name) {
    if(module != NULL) {
        //Get the address of the function
        return GetProcAddress(module, function_name);
    }
    else {
        return NULL;
    }
}

//Define the prototype of the original function
typedef HANDLE(WINAPI* CreateFileAPtr)(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

// log file
FILE* f_CreateFileA = NULL;

//Declare a global variable to store the pointer to the original fopen function
CreateFileAPtr old_CreateFileA = NULL;

// dll modules
HMODULE kernel32_dll = nullptr;

int getOriginalFunctions() {
    int cnt = 0;
    cnt += int((old_CreateFileA = (CreateFileAPtr) GetProcAddress(kernel32_dll, "CreateFileA")) == NULL);
    return cnt;
}

BOOL moding(char* dir, const char* filename, const char* rep, const char* with) {
    strcpy(dir, rep);
    BOOL ret1 = PathAppendA(dir, filename);
    BOOL ret2 = str_replace3(dir, rep, with);
    return BOOL(ret1 && ret2);
}

//Define the modified fopen function
HANDLE  WINAPI  hk_CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    if(moding(cstrBuffer, lpFileName, cstrCurrentDir, cstrReplaceDir)) {
        if(PathFileExistsA(cstrBuffer)) {
            fprintf(f_CreateFileA, "MOD_FILE_LOAD: \"%s\", from \"%s\"\n", cstrBuffer, lpFileName);
            return old_CreateFileA(cstrBuffer, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        }
    }
    return old_CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
};

BOOL LoadSettings() {
    // load ini file
    return 0;
}

bool WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    static HMODULE d3d9dll = nullptr;
    switch(dwReason) {
        case DLL_PROCESS_ATTACH: {
            //MessageBox(0, TEXT("FileHook Load"), TEXT("ASI Loader working."), MB_ICONWARNING);
            LoadSettings();
            GetCurrentDirectoryA(1023, cstrCurrentDir);
            strcpy(cstrReplaceDir, cstrCurrentDir); //GTA IV folder
            strcat(cstrReplaceDir, "\\mods");
            kernel32_dll = LoadLibraryA("kernel32.dll");
            //int cnt = getOriginalFunctions();
            if(kernel32_dll)
                old_CreateFileA = (CreateFileAPtr) GetProcAddress(kernel32_dll, "CreateFileA");
            if(LogFunctions)
                f_CreateFileA = fopen("fileHook.log", "w");
            if(HookFunctions)
                //Detour the original function with the modified function
                Iat_hook::detour_iat_ptr("CreateFileA", (void*) hk_CreateFileA);
            break;
        }
        case DLL_PROCESS_DETACH: {
            if(LogFunctions)
                if(f_CreateFileA)
                    fclose(f_CreateFileA);

            FreeLibrary(kernel32_dll);
        }
        break;
    }
    return true;
}

