#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

static bool system_main(void)
{
    MTY_SO* soctx = MTY_SOLoad("..\\test\\assets\\MatoyaTestDll.dll");
    test_cmp("MTY_SOLoad", soctx != NULL);

    void* value = MTY_SOGetSymbol(soctx, "LoadedFuncSymbol");
    test_cmp("MTY_SOGetSymbol", value && ((uint32_t(*)(void))value)()==31337);

    MTY_SOUnload(&soctx);
    test_cmp("MTY_SOUnload", soctx == NULL);

    const char* hostname = MTY_GetHostname();
    test_cmp("MTY_GetHostname", strcmp(hostname, "noname"));

    uint32_t plat = MTY_GetPlatform();
    test_cmpi32("MTY_GetPlatform", plat > 0, plat);

    plat = MTY_GetPlatformNoWeb();
    test_cmpi32("MTY_GetPlatformNoWeb", plat > 0, plat);

    bool r = MTY_IsSupported();
    test_cmp("MTY_IsSupported", r);

    const char* procpath = MTY_GetProcessPath(); //Needs checks depending on the platform
    test_cmps("MTY_GetProcessPath", procpath, procpath);

#ifdef _WIN32
    MTY_SetRunOnStartup("TestApp", procpath, NULL); //TODO: Check by manually opening the reg key
    r = MTY_GetRunOnStartup("TestApp");
    test_cmp("MTY_GetRunOnStartup", r);
    MTY_SetRunOnStartup("TestApp", NULL, NULL);

    //MTY_OpenConsole("TestConsole");
    //MTY_CloseConsole();
#else

#endif
    const char* verstring = MTY_GetVersionString(plat);
    test_cmps("MTY_GetVersionString", verstring, verstring);

    return true;
}