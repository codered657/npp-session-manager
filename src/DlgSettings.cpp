/*
    DlgSettings.cpp
    Copyright 2011,2012,2013 Michael Foster (http://mfoster.com/npp/)

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
#include "Config.h"
#include "DlgSettings.h"
#include "Util.h"
#include "res\resource.h"
#include <strsafe.h>
#include <commdlg.h>
#include <shlobj.h>

//------------------------------------------------------------------------------

namespace NppPlugin {

//------------------------------------------------------------------------------

namespace {

TCHAR *MSG_NO_CHANGES = _T("There were no changes.");
TCHAR *MSG_DIR_ERROR = _T("An error occurred while creating the new session directory.\nThis setting was not changed.");

INT _minWidth = 0, _minHeight = 0;
bool _inInit, _opChanged, _dirChanged;

INT onOk(HWND hDlg);
bool onInit(HWND hDlg);
void onResize(HWND hDlg, INT w, INT h);
void onGetMinSize(HWND hDlg, LPMINMAXINFO p);
bool getFolderName(HWND parent, TCHAR *buf);

} // end namespace

//------------------------------------------------------------------------------

INT_PTR CALLBACK dlgCfg_msgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    INT okStat;

    if (uMessage == WM_COMMAND) {
        WORD ctrl = LOWORD(wParam);
        WORD ntfy = HIWORD(wParam);
        switch (ctrl) {
            case IDOK:
                okStat = onOk(hDlg);
                EndDialog(hDlg, 1);
                if (okStat == 1) {
                    msgBox(MSG_NO_CHANGES, M_INFO);
                }
                else if (okStat == 2) {
                    msgBox(MSG_DIR_ERROR, M_WARN);
                }
                return TRUE;
                break;
            case IDCANCEL:
                EndDialog(hDlg, 0);
                return TRUE;
            case IDC_CFG_CHK_ASV:
            case IDC_CFG_CHK_ALD:
            case IDC_CFG_CHK_LIC:
            case IDC_CFG_CHK_LWC:
            case IDC_CFG_CHK_SITB:
            case IDC_CFG_CHK_SISB:
                if (!_inInit && ntfy == BN_CLICKED) {
                    _opChanged = true;
                }
                return TRUE;
            case IDC_CFG_EDT_DIR:
            case IDC_CFG_EDT_EXT:
                if (!_inInit && ntfy == EN_CHANGE) {
                    _dirChanged = true;
                }
                return TRUE;
            case IDC_CFG_BTN_BRW:
                if (!_inInit && ntfy == BN_CLICKED) {
                    TCHAR pthBuf[MAX_PATH + 1];
                    if (getFolderName(hDlg, pthBuf)) {
                        _dirChanged = true;
                        dlg::setText(hDlg, IDC_CFG_EDT_DIR, pthBuf);
                    }
                }
                return TRUE;
        } // end switch
    }
    else if (uMessage == WM_SIZE) {
        onResize(hDlg, LOWORD(lParam), HIWORD(lParam));
    }
    else if (uMessage == WM_GETMINMAXINFO) {
        onGetMinSize(hDlg, (LPMINMAXINFO)lParam);
    }
    else if (uMessage == WM_INITDIALOG) {
        if (onInit(hDlg)) {
            return TRUE;
        }
    }
    return FALSE;
}

//------------------------------------------------------------------------------

namespace {

/* Determines minimum dialog size. Populates controls with current values from
   gCfg. Resizes, centers and displays the dialog window.
   XXX Adding 6 and 24 is magic. */
bool onInit(HWND hDlg)
{
    _inInit = true;
    _opChanged = false;
    _dirChanged = false;
    if (_minWidth == 0) {
        RECT rect = {0, 0, IDD_CFG_W, IDD_CFG_H};
        MapDialogRect(hDlg, &rect);
        _minWidth = rect.right - rect.left + 6;
        _minHeight = rect.bottom - rect.top + 24;
    }
    // init control values
    dlg::setCheck(hDlg, IDC_CFG_CHK_ASV, gCfg.getAutoSave());
    dlg::setCheck(hDlg, IDC_CFG_CHK_ALD, gCfg.getAutoLoad());
    dlg::setCheck(hDlg, IDC_CFG_CHK_LIC, gCfg.getLoadIntoCurrent());
    dlg::setCheck(hDlg, IDC_CFG_CHK_LWC, gCfg.getLoadWithoutClosing());
    dlg::setCheck(hDlg, IDC_CFG_CHK_SITB, gCfg.getShowInTitlebar());
    dlg::setCheck(hDlg, IDC_CFG_CHK_SISB, gCfg.getShowInStatusbar());
    dlg::setText(hDlg, IDC_CFG_EDT_DIR, gCfg.getSesDir());
    dlg::setText(hDlg, IDC_CFG_EDT_EXT, gCfg.getSesExt());
    // focus the first edit control
    dlg::focus(hDlg, IDC_CFG_EDT_DIR);
    // resize, center and show the window
    INT w, h;
    gCfg.readCfgDlgSize(&w, &h);
    if (w <= 0 || h <= 0) {
        w = 0;
        h = 0;
    }
    else {
        w += 6;
        h += 24;
    }
    dlg::centerWnd(hDlg, sys_getNppHwnd(), 0, 0, w, h, true);
    ShowWindow(hDlg, SW_SHOW);

    _inInit = false;
    return true;
}

/* Gets values, if changed, from dialog box controls. Updates the global
   config object and save them to the ini file. */
INT onOk(HWND hDlg)
{
    INT stat = 0;
    bool change = false;
    TCHAR buf[MAX_PATH_1];

    if (_opChanged) {
        change = true;
        gCfg.setAutoSave(dlg::getCheck(hDlg, IDC_CFG_CHK_ASV));
        gCfg.setAutoLoad(dlg::getCheck(hDlg, IDC_CFG_CHK_ALD));
        gCfg.setLoadIntoCurrent(dlg::getCheck(hDlg, IDC_CFG_CHK_LIC));
        gCfg.setLoadWithoutClosing(dlg::getCheck(hDlg, IDC_CFG_CHK_LWC));
        gCfg.setShowInTitlebar(dlg::getCheck(hDlg, IDC_CFG_CHK_SITB));
        gCfg.setShowInStatusbar(dlg::getCheck(hDlg, IDC_CFG_CHK_SISB));
    }
    if (_dirChanged) {
        change = true;
        dlg::getText(hDlg, IDC_CFG_EDT_DIR, buf);
        if (!gCfg.setSesDir(buf)) {
            stat = 2; // error creating ses dir
        }
        dlg::getText(hDlg, IDC_CFG_EDT_EXT, buf);
        gCfg.setSesExt(buf);
    }

    if (change) {
        if (gCfg.save()) {
            if (_dirChanged) {
                app_readSesDir();
            }
        }
    }
    else {
        stat = 1; // no changes
    }

    return stat;
}

void onResize(HWND hDlg, INT dlgW, INT dlgH)
{
    HWND hCtrl = GetDlgItem(hDlg, IDC_CFG_EDT_DIR);
    if (hCtrl) {
        // Convert dialog units to pixels
        RECT r0 = {IDC_ALL_MARGIN, IDC_ALL_MARGIN, IDC_ALL_MARGIN + 10, IDC_ALL_MARGIN + 10};
        MapDialogRect(hDlg, &r0);
        // Resize the Directory edit
        RECT r;
        POINT p;
        GetWindowRect(hCtrl, &r);
        p.x = r.left;
        p.y = r.top;
        if (ScreenToClient(hDlg, &p)) {
            MoveWindow(hCtrl, p.x, p.y, dlgW - p.x - r0.left, r.bottom - r.top, TRUE);
            ShowWindow(hCtrl, SW_SHOW);
        }
        // Save new dialog size
        gCfg.saveCfgDlgSize(dlgW, dlgH);

        if (gCfg.debug) {
            TCHAR msgBuf[101];
            TCHAR numBuf[21];
            StringCchCopy(msgBuf, 100, _T("DlgSettings: w="));
            _itot_s(dlgW, numBuf, 20, 10);
            StringCchCat(msgBuf, 100, numBuf);
            StringCchCat(msgBuf, 100, _T(", h="));
            _itot_s(dlgH, numBuf, 20, 10);
            StringCchCat(msgBuf, 100, numBuf);
            SendMessage(sys_getNppHwnd(), NPPM_SETSTATUSBAR, STATUSBAR_DOC_TYPE, (LPARAM)msgBuf);
        }
    }
}

/* Sets the minimum size the user can resize to. */
void onGetMinSize(HWND hDlg, LPMINMAXINFO p)
{
    p->ptMinTrackSize.x = _minWidth;
    p->ptMinTrackSize.y = _minHeight;
}

/* Copied and slightly modifed from:
   npp.6.2.3.src\PowerEditor\src\MISC\Common\Common.cpp */
bool getFolderName(HWND parent, TCHAR *buf)
{
    bool ok = false;
    LPMALLOC pShellMalloc = 0;

    if (::SHGetMalloc(&pShellMalloc) == NO_ERROR) {
        BROWSEINFO info;
        memset(&info, 0, sizeof(info));
        info.hwndOwner = parent;
        info.pidlRoot = NULL;
        TCHAR szDisplayName[MAX_PATH];
        info.pszDisplayName = szDisplayName;
        info.lpszTitle = TEXT("Select a sessions folder");
        info.ulFlags = 0;
        // Execute the browsing dialog.
        LPITEMIDLIST pidl = ::SHBrowseForFolder(&info);
        // pidl will be null if they cancel the browse dialog, else not null if they select a folder.
        if (pidl) {
            if (::SHGetPathFromIDList(pidl, buf)) {
                ok = true;
            }
            pShellMalloc->Free(pidl);
        }
        pShellMalloc->Release();
    }
    return ok;
}

} // end namespace

} // end namespace NppPlugin
