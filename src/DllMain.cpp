/*
    DllMain.h
    Copyright 2011 Michael Foster (http://mfoster.com/npp/)

    This file is part of SessionMgr, A Plugin for Notepad++.

    SessionMgr is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "System.h"
#include "SessionMgr.h"
#include "Menu.h"

using namespace NppPlugin::api;

//------------------------------------------------------------------------------

namespace {

INT dllCount = 0;

} // end namespace

//------------------------------------------------------------------------------

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            if (++dllCount == 1) {
                sys_onLoad(hInstance);
                app_onLoad();
                mnu_onLoad();
            }
            break;
        case DLL_PROCESS_DETACH:
            if (--dllCount == 0) {
                mnu_onUnload();
                app_onUnload();
                sys_onUnload();
            }
            break;
    }

    return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData nppd)
{
    sys_init(nppd);
    app_init();
    mnu_init();
}

extern "C" __declspec(dllexport) const TCHAR* getName()
{
    return app_getName();
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(INT *pnbFuncItems)
{
    return mnu_getItems(pnbFuncItems);
}

extern "C" __declspec(dllexport) void beNotified(SCNotification *pscn)
{
    app_onNotify(pscn);
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
    return app_msgProc(Message, wParam, lParam);
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
    return TRUE;
}
#endif
