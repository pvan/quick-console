// 
// put this in your startup folder to always have a quick and easy cmd via windows key + tilde
// 
// when started, this program creates a cmd window in the top left corner
// and registers a hotkey to hide/show this new cmd window 
// this program will continue to run, waiting for hotkeys,
// until the child cmd is closed, then this will cleanup and close as well
//
// on launch the cmd will run shell.bat if it exists in the same directory as this exe
// this can be used to set the default starting directory, set shortcuts, run vcvarsall, etc..
// you can specify a different .bat file by passing a path as a command line arg
//
// window stores colors/fonts for cmd windows in the registry by name of the cmd window
// to set the colors/font, create a cmd shortcut with the same name as set below
// and customize the properties of that shortcut to save those settings to the registry
// or just edit the registrey at HKEY_CURRENT_USER\Console directly
//
// this is mostly a bare-bones program but it does handle a few special cases:
// 1 - if child cmd is forced closed or exited...
//     ...it should trigger a callback which exits this process
//
// 2 - this process is forced closed...
//     ...the cmd process will dangle, parentless 
//     but when this program starts again, it will kill any previously dangling cmds
//     alt: http://stackoverflow.com/questions/3342941/kill-child-process-when-parent-process-is-killed
//
// 3 - a second instance of this is started...
//     ...we message user and exit (could instead replace old instance?)
//
// 
// todo: reduce mem footprint? worth calling EmptyWorkingSet(GetCurrentProcess()); ?
// consider: hiding console after every command? (unless opened a certain way?) ehh... dunno
// consider: rename.. is shorter is better? "quick console" "quick ddco"?
//
// todo: add to system tray?
// consider: other cmdline options (doesn't seem important)
//
// idea: somehow detect ctrl+v for paste and simulate rightclick on the cmd window?
//
// bug: when hiding desktop (win+D), the cmd gets 'hidden' but then the next hotkey will 'really hide' it
//
// any optimization compiler flags we want?
//

#include <windows.h>
// #include <Psapi.h>  // for EmptyWorkingSet

#pragma comment( lib, "user32" )
#pragma comment( lib, "gdi32" )

// this is used to find the cmd window we open at startup
// AND used if you want to set the font/colors via shortcut, see notes
char *GlobalCmdTitle = "phil's quick console";


// global so we can close the handles when getting a callback
PROCESS_INFORMATION pi;

// global so we can access in GetChildCmdWindow.. not super clean but w/e
STARTUPINFO si;


// in seconds i believe
float GetWallClockTime()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    return (float)counter.QuadPart / (float)freq.QuadPart;
}


// a way to detect when the cmd process closes, via
// http://stackoverflow.com/questions/3556048/how-to-detect-win32-process-creation-termination-in-c
VOID CALLBACK WaitOrTimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
    UnregisterHotKey(0, 1);

    // Close process and thread handles.
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );

    exit(1);
}


// relying on the text+class of the window to find it,
// this WILL FAIL if other cmd with this name is already open
// can we make our own custom class name?
// EnumWindows or EnumThreadWindows is cannonical soln it seems?
// maybe there is another way if we make it somehow a child of this proc?
HWND GetChildCmdWindow()
{
    return FindWindow("ConsoleWindowClass", si.lpTitle);
}


float lerp(float a, float b, float p)
{
    return (b-a)*p + a;
}


void AnimateFromTo(HWND ConsoleWindow, RECT startR, RECT endR, float animLength)
{
    float startTime = GetWallClockTime();

    if (animLength <= 0)
    {
        HRGN rgnClnt = CreateRectRgnIndirect(&endR);
        SetWindowRgn(ConsoleWindow, rgnClnt, 1);
        return;
    }

    while (true)
    {
        float proportionElapsed = (GetWallClockTime() - startTime) / animLength;

        if (proportionElapsed >= 1.0f)
        {
            HRGN rgnClnt = CreateRectRgnIndirect(&endR);
            SetWindowRgn(ConsoleWindow, rgnClnt, 1);
            break;
        }

        float left   = lerp(startR.left  , endR.left  , proportionElapsed);
        float top    = lerp(startR.top   , endR.top   , proportionElapsed);
        float right  = lerp(startR.right , endR.right , proportionElapsed);
        float bottom = lerp(startR.bottom, endR.bottom, proportionElapsed);
        RECT currentRect = {LONG(left), LONG(top), LONG(right), LONG(bottom)};

        HRGN rgnClnt = CreateRectRgnIndirect(&currentRect);
        SetWindowRgn(ConsoleWindow, rgnClnt, 1);
    }
}




int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    // check whether we already have an instance running. If so, exit
    LPCTSTR DetectionName = "PhilsQuickConsole";
    HANDLE h_sem = CreateSemaphore (NULL, 0, 1, DetectionName);
    DWORD err = GetLastError ();
    if (err == ERROR_ALREADY_EXISTS)
    {
        MessageBox(0, "Process already running.", "^-^", MB_OK);
        return FALSE;
    }


    // start main program...

    si = {};
    si.cb = sizeof(si);
    si.lpTitle = GlobalCmdTitle; // note this is used to find the cmd window!
    si.dwFlags = STARTF_USESHOWWINDOW; // wShowWindow is ignored unless we set this?
    si.wShowWindow = SW_HIDE;

    pi = {};

    // first check if any previously opened cmd windows exist, close them if they do
    // (or should we use them instead? consider if it's running some long process...)
    HWND OldConsoleWindow = GetChildCmdWindow();
    // could loop this but... if we have more than one dangling, something is majorly messed up
    if (OldConsoleWindow) {
        // 3 part process: first window handle -> process id
        DWORD oldCmdPid;
        GetWindowThreadProcessId(OldConsoleWindow, &oldCmdPid);
        // then create a handle to the process (is this is a bit like openign a file handle?)
        HANDLE handleToOldCmd = OpenProcess(PROCESS_TERMINATE, FALSE, oldCmdPid);
        // finally, kill the process via the handle
        TerminateProcess(handleToOldCmd, 0);
        // still have to close the handle even tho the process is ended/ending
        CloseHandle(handleToOldCmd);
    }

    // command line option ideas...
    // /b for batch file? (eg "shell.bat")
    // /a alpha
    // /cd for current directory? (not really needed, instead add a line to the batch file)
    // that's it? (/v initial visibility override?)

    // afaiu, /k will run the cmd in another window and exec text after the /k in that window
    char cmdOptionsOutput[MAX_PATH] = "/k";

    // if no cmd options passed in
    if (lpCmdLine[0] == '\0')
    {
        strcat (cmdOptionsOutput, " \"");

        // first get full path
        char path[MAX_PATH];
        GetModuleFileName(0, path, MAX_PATH);

        // then trim out the exe name so we just have the directory
        for (int i = strlen(path); i >= 0; i--)
        {
            if (path[i] == '\\') {
                path[i+1] = '\0';
                break;
            }
        }

        strcat (cmdOptionsOutput, path);
        strcat (cmdOptionsOutput, "shell.bat\"");
    }
    else
    {
        strcat (cmdOptionsOutput, " \"");
        strcat (cmdOptionsOutput, lpCmdLine);
        strcat (cmdOptionsOutput, "\"");
    }


    // check if can't find shell.bat ?
    // doesn't seem nec, error msg will show up anyway


    if (!CreateProcess("C:/Windows/System32/cmd.exe",

                       // command line options, should have at least "/k"
                       cmdOptionsOutput,

                       // could totally elim scroll by setting h/w equal to window like so:
                       // "/k mode con: cols=120 lines=30",

                       0, 0, FALSE,
                       0, // flags
                       0,
                       0, // setting starting dir, you can do this in the shell.bat
                       &si, &pi))
    {
        MessageBox(0, "failed to open cmd. exiting..", ":?", MB_OK);
        return 0;
    }


    HWND ConsoleWindow = GetChildCmdWindow();
    {
        float startTime = GetWallClockTime();
        float secondsOnProcess = 0.0f;
        while (!ConsoleWindow && secondsOnProcess < 3.0f)
        {
            ConsoleWindow = GetChildCmdWindow();
            secondsOnProcess = GetWallClockTime() - startTime;
        }

        if (!ConsoleWindow)
        {
            MessageBox(0, "timeout finding handle to cmd window. exiting..", "; ;", MB_OK);
            return 0;
        }
    }

    // helps with some intermittent bugs
    // like sometimes showing up in the task bar or not showing up at all
    // since this only happens once (at startup) maybe even increase this?
    Sleep(100);

    // setup cmd window visuals here... (nested for readability only)
    RECT nominalRect;
    {
        // at the very least needed for WS_EX_TOOLWINDOW (possibly essential for other reasons)
        ShowWindow(ConsoleWindow, SW_HIDE);

        // grab our client rect before we resize anything (by hiding borders etc)
        // RECT nominalRect;
        GetClientRect(ConsoleWindow, &nominalRect);

        // override styles like WS_OVERLAPPED etc.. just setting to 0 seems to also work?
        SetWindowLong(ConsoleWindow, GWL_STYLE, WS_POPUP);

        // WS_EX_TOOLWINDOW means don't show up in taskbar or alt+tab (window has to be hidden)
        LONG exStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW;
        SetWindowLongPtr(ConsoleWindow, GWL_EXSTYLE, exStyle);

        SetLayeredWindowAttributes(ConsoleWindow, 0, 180, LWA_ALPHA);


        // this is the magic sauce right here
        // http://stackoverflow.com/questions/15059707/console-window-gets-bugged-if-i-get-the-window-handle-too-early?rq=1
        // this clips the messed up areas left over after removing the border
        // unfortunately it also hides the scroll bars
        nominalRect.right += 1;
        HRGN rgnClnt = CreateRectRgnIndirect(&nominalRect);
        SetWindowRgn(ConsoleWindow, rgnClnt, 1);

        // setwindowlong things won't get applied until this
        SetWindowPos(ConsoleWindow, HWND_TOP, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOSIZE);
    }

    // showing cmd once here even if we want to hide as default
    // will fix some gfx bugs the first time we animate reveal
    ShowWindow(ConsoleWindow, SW_SHOW);
    bool ConsoleIsShown = true;


    // by default hide cmd after startup
    bool startHidden = true;
    if (startHidden)
    {
        ShowWindow(ConsoleWindow, SW_HIDE);
        ConsoleIsShown = false;
    }
    else
    {
        // since we CreateProcess as hidden, it won't have focus by default
        // except "focus" isn't the right term, i guess, b/c SetFocus didn't work
        if (!SetForegroundWindow(ConsoleWindow))
        {
            MessageBox(0, "couldn't activate window", ":(", MB_OK);
        }
    }


    // register to get a callback when the cmd proc ends...
    // (not sure what hNewHandle is used for)
    HANDLE hNewHandle;
    RegisterWaitForSingleObject(&hNewHandle, pi.hProcess, WaitOrTimerCallback,
                                NULL, INFINITE, WT_EXECUTEONLYONCE);


    RegisterHotKey(0, 1, MOD_WIN, VK_OEM_3);  // VK_OEM_3 = tilde key


    // EmptyWorkingSet(GetCurrentProcess());


    // animation settings
    RECT visibleRect = nominalRect;
    RECT hiddenRect = nominalRect; hiddenRect.bottom = 0;
    float showHideAnimLength = 0.1f;


    while (true)
    {
        MSG Message;
        while (GetMessage(&Message, 0, 0, 0))
        {
            if (Message.message == WM_HOTKEY)
            {
                ConsoleIsShown = !ConsoleIsShown;

                if (ConsoleIsShown)
                {
                    AnimateFromTo(ConsoleWindow, hiddenRect, hiddenRect, 0);
                    ShowWindow(ConsoleWindow, SW_SHOW);
                    SetForegroundWindow(ConsoleWindow);
                    AnimateFromTo(ConsoleWindow, hiddenRect, visibleRect, showHideAnimLength);
                }
                else
                {
                    AnimateFromTo(ConsoleWindow, visibleRect, hiddenRect, showHideAnimLength);
                    ShowWindow(ConsoleWindow, SW_HIDE);
                }

            }
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
    }

    return 0;
}