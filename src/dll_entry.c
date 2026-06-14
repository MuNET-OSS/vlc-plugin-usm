// dll_entry.c - Windows DLL入口点
#ifdef _WIN32

#include <windows.h>

/* MinGW-w64 的 dllcrt2.o 启动代码期望找到 DllEntryPoint，
 * 而非 DllMain。函数签名与 DllMain 完全一致。 */
BOOL WINAPI DllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
    (void)hinstDLL;
    (void)fdwReason;
    (void)lpReserved;
    return TRUE;
}

#endif
