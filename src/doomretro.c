/*
========================================================================

                           D O O M  R e t r o
         The classic, refined DOOM source port. For Windows PC.

========================================================================

  Copyright © 1993-2012 id Software LLC, a ZeniMax Media company.
  Copyright © 2013-2017 Brad Harding.

  DOOM Retro is a fork of Chocolate DOOM.
  For a list of credits, see <http://wiki.doomretro.com/credits>.

  This file is part of DOOM Retro.

  DOOM Retro is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation, either version 3 of the License, or (at your
  option) any later version.

  DOOM Retro is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with DOOM Retro. If not, see <https://www.gnu.org/licenses/>.

  DOOM is a registered trademark of id Software LLC, a ZeniMax Media
  company, in the US and/or other countries and is used without
  permission. All other trademarks are the property of their respective
  holders. DOOM Retro is in no way affiliated with nor endorsed by
  id Software.

========================================================================
*/

#include "c_console.h"
#include "d_main.h"
#include "doomstat.h"
#include "i_gamepad.h"
#include "i_midirpc.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_controls.h"
#include "version.h"

int windowborderwidth;
int windowborderheight;

#if defined(_WIN32)

#include "SDL_syswm.h"

#include <Windows.h>
#include <ShellAPI.h>

#if !defined(SM_CXPADDEDBORDER)
#define SM_CXPADDEDBORDER   92
#endif

void I_SetProcessDPIAware(void)
{
    HMODULE hLibrary = LoadLibrary("user32.dll");

    if (hLibrary)
    {
        typedef dboolean (*SETPROCESSDPIAWARE)();

        SETPROCESSDPIAWARE  pSetProcessDPIAware = (SETPROCESSDPIAWARE)GetProcAddress(hLibrary,
                                "SetProcessDPIAware");

        if (pSetProcessDPIAware)
            pSetProcessDPIAware();

        FreeLibrary(hLibrary);
    }
}

HHOOK   g_hKeyboardHook;

void G_ScreenShot(void);

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    dboolean    bEatKeystroke = false;

    if (nCode == HC_ACTION)
        switch (wParam)
        {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP:
                if (windowfocused)
                {
                    DWORD   vkCode = ((KBDLLHOOKSTRUCT *)lParam)->vkCode;

                    if (vkCode == VK_LWIN || vkCode == VK_RWIN)
                        bEatKeystroke = ((!menuactive && !paused && !consoleactive) || vid_fullscreen);
                    else if (keyboardscreenshot == KEY_PRINTSCREEN && vkCode == VK_SNAPSHOT)
                    {
                        if (wParam == WM_KEYDOWN)
                            G_ScreenShot();

                        bEatKeystroke = true;
                    }
                }

                break;
        }

    return (bEatKeystroke ? 1 : CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam));
}

WNDPROC oldProc;
HICON   icon;

dboolean MouseShouldBeGrabbed(void);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_SETCURSOR)
    {
        if (LOWORD(lParam) == HTCLIENT && !MouseShouldBeGrabbed())
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return true;
        }
    }
    else if (msg == WM_SYSCOMMAND)
    {
        if ((wParam & 0xFFF0) == SC_MAXIMIZE)
        {
            I_ToggleFullscreen();
            return true;
        }
        else if ((wParam & 0xFFF0) == SC_KEYMENU)
            return false;
    }
    else if (msg == WM_SYSKEYDOWN && wParam == VK_RETURN && !(lParam & 0x40000000))
    {
        I_ToggleFullscreen();
        return true;
    }
    else if (msg == WM_DEVICECHANGE)
        I_InitGamepad();
    else if (msg == WM_SIZE && !vid_fullscreen)
        blitfunc();
    else if (msg == WM_GETMINMAXINFO)
    {
        LPMINMAXINFO    minmaxinfo = (LPMINMAXINFO)lParam;

        minmaxinfo->ptMinTrackSize.x = ORIGINALWIDTH + windowborderwidth;
        minmaxinfo->ptMinTrackSize.y = ORIGINALWIDTH * 3 / 4 + windowborderheight;

        return false;
    }

    return CallWindowProc(oldProc, hwnd, msg, wParam, lParam);
}

HANDLE      hInstanceMutex;

STICKYKEYS  g_StartupStickyKeys = { sizeof(STICKYKEYS), 0 };
TOGGLEKEYS  g_StartupToggleKeys = { sizeof(TOGGLEKEYS), 0 };
FILTERKEYS  g_StartupFilterKeys = { sizeof(FILTERKEYS), 0 };

void I_AccessibilityShortcutKeys(dboolean bAllowKeys)
{
    if (bAllowKeys)
    {
        // Restore StickyKeys/etc to original state
        SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
        SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
        SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);
    }
    else
    {
        // Disable StickyKeys/etc shortcuts
        STICKYKEYS  skOff = g_StartupStickyKeys;
        TOGGLEKEYS  tkOff = g_StartupToggleKeys;
        FILTERKEYS  fkOff = g_StartupFilterKeys;

        if (!(skOff.dwFlags & SKF_STICKYKEYSON))
        {
            // Disable the hotkey and the confirmation
            skOff.dwFlags &= ~SKF_HOTKEYACTIVE;
            skOff.dwFlags &= ~SKF_CONFIRMHOTKEY;

            SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &skOff, 0);
        }

        if (!(tkOff.dwFlags & TKF_TOGGLEKEYSON))
        {
            // Disable the hotkey and the confirmation
            tkOff.dwFlags &= ~TKF_HOTKEYACTIVE;
            tkOff.dwFlags &= ~TKF_CONFIRMHOTKEY;

            SystemParametersInfo(SPI_SETTOGGLEKEYS, sizeof(TOGGLEKEYS), &tkOff, 0);
        }

        if (!(fkOff.dwFlags & FKF_FILTERKEYSON))
        {
            // Disable the hotkey and the confirmation
            fkOff.dwFlags &= ~FKF_HOTKEYACTIVE;
            fkOff.dwFlags &= ~FKF_CONFIRMHOTKEY;

            SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &fkOff, 0);
        }
    }
}

#if !defined(_DEBUG)
LONG WINAPI ExceptionHandler(LPEXCEPTION_POINTERS info)
{
    char *msg = PACKAGE_NAME" has crashed.";

    const SDL_MessageBoxButtonData buttons[] =
    {
        {                                       0, 0, "&Report" },
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "&OK"     }
    };

    const SDL_MessageBoxData messageboxdata =
    {
        SDL_MESSAGEBOX_INFORMATION,
        NULL,
        PACKAGE_NAME,
        msg,
        SDL_arraysize(buttons),
        buttons,
        NULL
    };

    int buttonid;

    I_MidiRPCClientShutDown();

    if (SDL_ShowMessageBox(&messageboxdata, &buttonid) >= 0)
        if (buttons[buttonid].buttonid == 0)
            ShellExecute(GetActiveWindow(), "open", PACKAGE_REPORT_URL, NULL, NULL, SW_SHOWNORMAL);

    I_Quit(false);
}
#endif

void I_InitWindows32(void)
{
    HINSTANCE       handle = GetModuleHandle(NULL);
    SDL_SysWMinfo   info;
    HWND            hwnd;

    SDL_VERSION(&info.version);

    SDL_GetWindowWMInfo(window, &info);
    hwnd = info.info.win.window;

    icon = LoadIcon(handle, "IDI_ICON1");
    SetClassLongPtr(hwnd, GCLP_HICON, (LONG)icon);

    oldProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG)WndProc);

    windowborderwidth = (GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER)) * 2;
    windowborderheight = (GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER)) * 2
        + GetSystemMetrics(SM_CYCAPTION);

#if !defined(_DEBUG)
    SetUnhandledExceptionFilter(ExceptionHandler);
#endif
}

void I_ShutdownWindows32(void)
{
    DestroyIcon(icon);
    UnhookWindowsHookEx(g_hKeyboardHook);
    ReleaseMutex(hInstanceMutex);
    CloseHandle(hInstanceMutex);
    I_AccessibilityShortcutKeys(true);
}
#endif

#ifdef __ANDROID__
int main_android(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
#if defined(_WIN32)
    hInstanceMutex = CreateMutex(NULL, true, PACKAGE_MUTEX);

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (hInstanceMutex)
            CloseHandle(hInstanceMutex);

        SetForegroundWindow(FindWindow(PACKAGE_MUTEX, NULL));
        return 1;
    }

    g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);

    // Save the current sticky/toggle/filter key settings so they can be restored them later
    SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &g_StartupStickyKeys, 0);
    SystemParametersInfo(SPI_GETTOGGLEKEYS, sizeof(TOGGLEKEYS), &g_StartupToggleKeys, 0);
    SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &g_StartupFilterKeys, 0);

    I_AccessibilityShortcutKeys(false);

    I_SetProcessDPIAware();
#endif

    myargc = argc;
    myargv = argv;

    D_DoomMain();

    return 0;
}
