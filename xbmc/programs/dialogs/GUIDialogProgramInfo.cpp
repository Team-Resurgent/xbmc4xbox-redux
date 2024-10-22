/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogProgramInfo.h"
#include "programs/windows/GUIWindowProgramNav.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"

CGUIDialogProgramInfo::CGUIDialogProgramInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PROGRAM_INFO, "DialogProgramInfo.xml")
    , m_programItem(new CFileItem)
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogProgramInfo::~CGUIDialogProgramInfo(void)
{
}

void CGUIDialogProgramInfo::SetProgram(const CFileItem *item)
{
  // TODO: implement this
}

void CGUIDialogProgramInfo::ShowFor(const CFileItem& item)
{
  CGUIWindowProgramNav *window = static_cast<CGUIWindowProgramNav*>(g_windowManager.GetWindow(WINDOW_PROGRAM_NAV));
  if (window)
  {
    ADDON::ScraperPtr info;
    window->OnItemInfo(item, info);
  }
}
