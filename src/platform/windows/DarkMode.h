// Adapted from https://github.com/ysc3839/win32-darkmode/blob/master/win32-darkmode/DarkMode.h

#pragma once

#include <Windows.h>

#include "ImmersiveColor.h"

// 1903 18362
enum class PreferredAppMode
{
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max
};

// 1809 17763
typedef bool (WINAPI *ShouldAppsUseDarkMode_t)(); // ordinal 132
typedef bool (WINAPI *AllowDarkModeForApp_t)(bool allow); // ordinal 135, in 1809
typedef bool (WINAPI *IsDarkModeAllowedForWindow_t)(HWND hWnd); // ordinal 137
typedef PreferredAppMode (WINAPI *SetPreferredAppMode_t)(PreferredAppMode appMode); // ordinal 135, in 1903

inline ShouldAppsUseDarkMode_t g_pfnShouldAppsUseDarkMode = nullptr;
inline AllowDarkModeForApp_t g_pfnAllowDarkModeForApp = nullptr;
inline IsDarkModeAllowedForWindow_t g_pfnIsDarkModeAllowedForWindow = nullptr;
inline SetPreferredAppMode_t g_pfnSetPreferredAppMode = nullptr;

inline bool g_darkModeSupported = false;
extern RTL_OSVERSIONINFOW g_windowsVersionInfo;

inline BOOL IsHighContrast()
{
    HIGHCONTRASTW pvParam = { sizeof(pvParam) };
    return SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(pvParam), &pvParam, FALSE) && pvParam.dwFlags & HCF_HIGHCONTRASTON;
}

inline bool DarkMode_IsDarkMode(HWND hWnd)
{
    return g_darkModeSupported
        // && g_pfnIsDarkModeAllowedForWindow(hWnd)
        && g_pfnShouldAppsUseDarkMode()
        && !IsHighContrast();
}

inline void DarkMode_AllowDarkModeForApp(bool allow)
{
    if (g_pfnAllowDarkModeForApp)
    {
        g_pfnAllowDarkModeForApp(allow);
    }
    else if (g_pfnSetPreferredAppMode)
    {
        g_pfnSetPreferredAppMode(allow ? PreferredAppMode::AllowDark : PreferredAppMode::Default);
    }
}

inline void DarkMode_Init()
{
    if (g_windowsVersionInfo.dwBuildNumber < 17763)
        return;

    HMODULE hUxtheme = GetModuleHandleW(L"uxtheme.dll");
    if (hUxtheme)
    {
        g_pfnShouldAppsUseDarkMode = reinterpret_cast<ShouldAppsUseDarkMode_t>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));

        FARPROC pfnUxtheme135 = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
        if (g_windowsVersionInfo.dwBuildNumber < 18362)
            g_pfnAllowDarkModeForApp = reinterpret_cast<AllowDarkModeForApp_t>(pfnUxtheme135);
        else
            g_pfnSetPreferredAppMode = reinterpret_cast<SetPreferredAppMode_t>(pfnUxtheme135);

        g_pfnIsDarkModeAllowedForWindow = reinterpret_cast<IsDarkModeAllowedForWindow_t>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(137)));
    }

    if (g_pfnShouldAppsUseDarkMode
        && (g_pfnAllowDarkModeForApp || g_pfnSetPreferredAppMode)
        && g_pfnIsDarkModeAllowedForWindow)
    {
        g_darkModeSupported = true;

        DarkMode_AllowDarkModeForApp(true);
        RefreshImmersiveColorPolicyState();
    }
}
