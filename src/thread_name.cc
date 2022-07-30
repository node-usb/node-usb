// Definition of a SetThreadName function with an emphasis in "portability",
// meaning we prefer cautiously failing over creating build errors (or crashes)
// on exotic platforms. To do this, we only include/link against very
// few & widely available symbols.
#include "thread_name.h"

#if defined(__linux__)

    // For Linux use the prctl API directly. prctl symbol
    // should be available under any libc.
    #include <sys/prctl.h>

    // Define here to avoid relying on kernel headers being present
    #define PR_SET_NAME    15 /* Set process name */

    bool SetThreadName(const char* name) {
        return prctl(PR_SET_NAME, name, 0, 0, 0) >= 0;
    }

#elif defined(__APPLE__)

    // For MacOS use the dynamic linker because I don't
    // want to take any risks
    #include <dlfcn.h>

    extern "C" typedef int (*SetNameFn)(const char*);

    bool SetThreadName(const char* name) {
        auto pthread_setname_np = reinterpret_cast<SetNameFn>(
            dlsym(RTLD_DEFAULT, "pthread_setname_np"));
        if (pthread_setname_np == nullptr)
            return false;
        return pthread_setname_np(name) == 0;
    }

#elif defined(_WIN32)

    // For Windows, we use the new SetThreadDescription API which
    // is only available in newish versions. To avoid taking any
    // risks (and because on certain versions it's the only
    // option to access the API), we use the dynamic linker
    #include <windows.h>

    extern "C" typedef HRESULT (WINAPI *SetThreadDescriptionFn)(HANDLE, PCWSTR);

    static SetThreadDescriptionFn RetrieveSymbol(const char* objectName) {
        auto mod = GetModuleHandleA(objectName);
        if (mod == nullptr) return nullptr;
        auto symbol = GetProcAddress(mod, "SetThreadDescription");
        return reinterpret_cast<SetThreadDescriptionFn>(symbol);
    }

    #include <locale>
    #include <codecvt>
    #include <string>

    bool SetThreadName(const char* name) {
        auto SetThreadDescription = RetrieveSymbol("Kernel32.dll");
        // apparently, MSDN is wrong and the symbol is defined in
        // KernelBase.dll, so try that too
        if (SetThreadDescription == nullptr)
            SetThreadDescription = RetrieveSymbol("KernelBase.dll");

        if (SetThreadDescription == nullptr) return false;

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring wide_name = converter.from_bytes(name);

        auto result = SetThreadDescription(GetCurrentThread(), wide_name.c_str());
        return SUCCEEDED(result);
    }

#else

    bool SetThreadName(const char* name) {
        return false;
    }

#endif
