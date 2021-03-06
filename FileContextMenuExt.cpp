/****************************** Module Header ******************************\
Module Name:  FileContextMenuExt.cpp
Project:      CppShellExtContextMenuHandler
Copyright (c) Microsoft Corporation.

The code sample demonstrates creating a Shell context menu handler with C++. 

A context menu handler is a shell extension handler that adds commands to an 
existing context menu. Context menu handlers are associated with a particular 
file class and are called any time a context menu is displayed for a member 
of the class. While you can add items to a file class context menu with the 
registry, the items will be the same for all members of the class. By 
implementing and registering such a handler, you can dynamically add items to 
an object's context menu, customized for the particular object.

The example context menu handler adds the menu item "Display File Name (C++)"
to the context menu when you right-click a .cpp file in the Windows Explorer. 
Clicking the menu item brings up a message box that displays the full path 
of the .cpp file.

This source is subject to the Microsoft Public License.
See http://www.microsoft.com/opensource/licenses.mspx#Ms-PL.
All other rights reserved.

THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, 
EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#include "FileContextMenuExt.h"
#include <strsafe.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

extern UINT g_QCMFlags;
extern void HookShell();
extern HBITMAP Icon_Get();

extern HINSTANCE g_hInst;
extern long g_cDllRef;

#define IDM_DISPLAY             0  // The command's identifier offset

FileContextMenuExt::FileContextMenuExt(void) : m_cRef(1)
{
    InterlockedIncrement(&g_cDllRef);

    // Load the bitmap for the menu item. 
    // If you want the menu item bitmap to be transparent, the color depth of 
    // the bitmap must not be greater than 8bpp.
    //m_hMenuBmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_OK), 
    //    IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADTRANSPARENT);
}

FileContextMenuExt::~FileContextMenuExt(void)
{
    //if (m_hMenuBmp)
    //{
    //    DeleteObject(m_hMenuBmp);
    //    m_hMenuBmp = NULL;
    //}

    InterlockedDecrement(&g_cDllRef);
}


void FileContextMenuExt::OnVerbDisplayFileName(HWND hWnd)
{
    wchar_t szMessage[300];
    if (SUCCEEDED(StringCchPrintf(szMessage, ARRAYSIZE(szMessage), 
        L"The selected file is:\r\n\r\n%s", this->m_szSelectedFile)))
    {
        MessageBox(hWnd, szMessage, L"CppShellExtContextMenuHandler", MB_OK);
    }
}


#pragma region IUnknown

// Query to the interface the component supported.
IFACEMETHODIMP FileContextMenuExt::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(FileContextMenuExt, IContextMenu),
		QITABENT(FileContextMenuExt, IContextMenu2),
		QITABENT(FileContextMenuExt, IContextMenu3),
        QITABENT(FileContextMenuExt, IShellExtInit), 
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

// Increase the reference count for an interface on an object.
IFACEMETHODIMP_(ULONG) FileContextMenuExt::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// Decrease the reference count for an interface on an object.
IFACEMETHODIMP_(ULONG) FileContextMenuExt::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}

#pragma endregion


#pragma region IShellExtInit
extern LPCITEMIDLIST g_pidl;

extern void StartBuildPIDLArray(DWORD length);
extern void AddToPIDLArray(LPCITEMIDLIST pidl);
extern void MarkAsBackgroundMenu();
extern void SetCurrentPIDL();
// Initialize the context menu handler.
IFACEMETHODIMP FileContextMenuExt::Initialize(
    LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID)
{
	extern void LoadHookDLL();
	LoadHookDLL();

    if (NULL == pDataObj)
    {
		if (pidlFolder != NULL) {
			// Background
			StartBuildPIDLArray(1);
			AddToPIDLArray(pidlFolder);
			MarkAsBackgroundMenu();
			SetCurrentPIDL();
		}
        return S_OK;
    }

	// Item(s) selected
	// https://blogs.msdn.microsoft.com/oldnewthing/20130204-00/?p=5363
	// https://blogs.msdn.microsoft.com/oldnewthing/20160620-00/?p=93705
	// https://blogs.msdn.microsoft.com/oldnewthing/20130617-00/?p=4073
	// https://blogs.msdn.microsoft.com/oldnewthing/20110830-00/?p=9773/
	IShellItemArray* hSIA;
	if (SUCCEEDED( SHCreateShellItemArrayFromDataObject(pDataObj, IID_PPV_ARGS(&hSIA)) )) 
	{
		DWORD numOfItems;
		hSIA->GetCount(&numOfItems);
		StartBuildPIDLArray(numOfItems);

		IEnumShellItems* hESI;
		hSIA->EnumItems(&hESI);
		if (hESI) {
			for (IShellItem* hSI; hESI->Next(1, &hSI, nullptr) == S_OK; ) {//hSI.Release
				//LPWSTR spszName;
				//if (SUCCEEDED( hSI->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &spszName) ))

				LPITEMIDLIST pidl;
				SHGetIDListFromObject(hSI, &pidl);
				AddToPIDLArray(pidl);

				//LPWSTR name;
				//SHGetNameFromIDList(pidl, SIGDN_DESKTOPABSOLUTEPARSING, &name);
				//StrCpyW(m_szSelectedFile, name);

				g_pidl = pidl;
			}
		}
	}

	SetCurrentPIDL();
	return S_OK;
}

#pragma endregion

#pragma region IContextMenu
//
//   FUNCTION: FileContextMenuExt::QueryContextMenu
//
//   PURPOSE: The Shell calls IContextMenu::QueryContextMenu to allow the 
//            context menu handler to add its menu items to the menu. It 
//            passes in the HMENU handle in the hmenu parameter. The 
//            indexMenu parameter is set to the index to be used for the 
//            first menu item that is to be added.
//
IFACEMETHODIMP FileContextMenuExt::QueryContextMenu(
    HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
	g_QCMFlags = uFlags;
	//HookShell();
	extern void SetContextMenuFlags(UINT flags);
	SetContextMenuFlags(uFlags);
	

	/*


    // If uFlags include CMF_DEFAULTONLY then we should not do anything.
    if (CMF_DEFAULTONLY & uFlags)
    {
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));
    }

    // Use either InsertMenu or InsertMenuItem to add menu items.
    // Learn how to add sub-menu from:
    // http://www.codeproject.com/KB/shell/ctxextsubmenu.aspx

	
    MENUITEMINFO mii = { sizeof(mii) };
    mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_BITMAP;
    mii.wID = idCmdFirst + IDM_DISPLAY;
    mii.fType = MFT_STRING;
    mii.dwTypeData = L"&Display File Name (C++)";
    mii.fState = MFS_ENABLED;
    // mii.hbmpItem = static_cast<HBITMAP>(m_hMenuBmp);
	mii.hbmpItem = Icon_Get();
    
	if (!InsertMenuItem(hMenu, indexMenu, TRUE, &mii))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
	
    // Add a separator.
    MENUITEMINFO sep = { sizeof(sep) };
    sep.fMask = MIIM_TYPE;
    sep.fType = MFT_SEPARATOR;
    if (!InsertMenuItem(hMenu, indexMenu + 1, TRUE, &sep))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
	
    // Return an HRESULT value with the severity set to SEVERITY_SUCCESS. 
    // Set the code value to the offset of the largest command identifier 
    // that was assigned, plus one (1).
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(IDM_DISPLAY + 1));

	*/

	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
}


//
//   FUNCTION: FileContextMenuExt::InvokeCommand
//
//   PURPOSE: This method is called when a user clicks a menu item to tell 
//            the handler to run the associated command. The lpcmi parameter 
//            points to a structure that contains the needed information.
//
IFACEMETHODIMP FileContextMenuExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    BOOL fUnicode = FALSE;

    // Determine which structure is being passed in, CMINVOKECOMMANDINFO or 
    // CMINVOKECOMMANDINFOEX based on the cbSize member of lpcmi. Although 
    // the lpcmi parameter is declared in Shlobj.h as a CMINVOKECOMMANDINFO 
    // structure, in practice it often points to a CMINVOKECOMMANDINFOEX 
    // structure. This struct is an extended version of CMINVOKECOMMANDINFO 
    // and has additional members that allow Unicode strings to be passed.
    if (pici->cbSize == sizeof(CMINVOKECOMMANDINFOEX))
    {
        if (pici->fMask & CMIC_MASK_UNICODE)
        {
            fUnicode = TRUE;
        }
    }

    // Determines whether the command is identified by its offset or verb.
    // There are two ways to identify commands:
    // 
    //   1) The command's verb string 
    //   2) The command's identifier offset
    // 
    // If the high-order word of lpcmi->lpVerb (for the ANSI case) or 
    // lpcmi->lpVerbW (for the Unicode case) is nonzero, lpVerb or lpVerbW 
    // holds a verb string. If the high-order word is zero, the command 
    // offset is in the low-order word of lpcmi->lpVerb.

    // For the ANSI case, if the high-order word is not zero, the command's 
    // verb string is in lpcmi->lpVerb. 
    if (!fUnicode && HIWORD(pici->lpVerb))
    {
        // Is the verb supported by this context menu extension?
        if (StrCmpIA(pici->lpVerb, "cppdisplay") == 0)
        {
            OnVerbDisplayFileName(pici->hwnd);
        }
        else
        {
            // If the verb is not recognized by the context menu handler, it 
            // must return E_FAIL to allow it to be passed on to the other 
            // context menu handlers that might implement that verb.
            return E_FAIL;
        }
    }

    // For the Unicode case, if the high-order word is not zero, the 
    // command's verb string is in lpcmi->lpVerbW. 
    else if (fUnicode && HIWORD(((CMINVOKECOMMANDINFOEX*)pici)->lpVerbW))
    {
        // Is the verb supported by this context menu extension?
        if (StrCmpIW(((CMINVOKECOMMANDINFOEX*)pici)->lpVerbW, L"cppdisplay") == 0)
        {
            OnVerbDisplayFileName(pici->hwnd);
        }
        else
        {
            // If the verb is not recognized by the context menu handler, it 
            // must return E_FAIL to allow it to be passed on to the other 
            // context menu handlers that might implement that verb.
            return E_FAIL;
        }
    }

    // If the command cannot be identified through the verb string, then 
    // check the identifier offset.
    else
    {
        // Is the command identifier offset supported by this context menu 
        // extension?
        if (LOWORD(pici->lpVerb) == IDM_DISPLAY)
        {
            OnVerbDisplayFileName(pici->hwnd);
        }
        else
        {
            // If the verb is not recognized by the context menu handler, it 
            // must return E_FAIL to allow it to be passed on to the other 
            // context menu handlers that might implement that verb.
            return E_FAIL;
        }
    }

    return S_OK;
}


//
//   FUNCTION: CFileContextMenuExt::GetCommandString
//
//   PURPOSE: If a user highlights one of the items added by a context menu 
//            handler, the handler's IContextMenu::GetCommandString method is 
//            called to request a Help text string that will be displayed on 
//            the Windows Explorer status bar. This method can also be called 
//            to request the verb string that is assigned to a command. 
//            Either ANSI or Unicode verb strings can be requested. This 
//            example only implements support for the Unicode values of 
//            uFlags, because only those have been used in Windows Explorer 
//            since Windows 2000.
//
IFACEMETHODIMP FileContextMenuExt::GetCommandString(UINT_PTR idCommand, 
    UINT uFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    HRESULT hr = E_INVALIDARG;

    if (idCommand == IDM_DISPLAY)
    {
        switch (uFlags)
        {
        case GCS_HELPTEXTW:
            // Only useful for pre-Vista versions of Windows that have a 
            // Status bar.
            hr = StringCchCopy(reinterpret_cast<PWSTR>(pszName), cchMax, 
				L"Display File Name (C++)");
            break;

        case GCS_VERBW:
            // GCS_VERBW is an optional feature that enables a caller to 
            // discover the canonical name for the verb passed in through 
            // idCommand.
            hr = StringCchCopy(reinterpret_cast<PWSTR>(pszName), cchMax, 
				L"CppDisplayFileName");
            break;

        default:
            hr = S_OK;
        }
    }

    // If the command (idCommand) is not supported by this context menu 
    // extension handler, return E_INVALIDARG.

    return hr;
}

#pragma endregion

HRESULT MenuMessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	//if (uMsg == WM_MEASUREITEM)
		//ClassicMenu(g_hMenu);

	return S_OK;
}

IFACEMETHODIMP FileContextMenuExt::HandleMenuMsg
	(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT res;
	return MenuMessageHandler(uMsg, wParam, lParam, &res);
}

IFACEMETHODIMP FileContextMenuExt::HandleMenuMsg2
(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
	if (NULL == plResult)
	{
		LRESULT res;
		return MenuMessageHandler(uMsg, wParam, lParam, &res);
	}
	else
	{
		return MenuMessageHandler(uMsg, wParam, lParam, plResult);
	}
}