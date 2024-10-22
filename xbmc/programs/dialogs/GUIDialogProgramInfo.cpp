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
#include "Util.h"
#include "guilib/GUIImage.h"
#include "programs/windows/GUIWindowProgramNav.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"

#define CONTROL_IMAGE                3
#define CONTROL_TEXTAREA             4

CGUIDialogProgramInfo::CGUIDialogProgramInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PROGRAM_INFO, "DialogProgramInfo.xml")
    , m_programItem(new CFileItem)
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogProgramInfo::~CGUIDialogProgramInfo(void)
{
}

void CGUIDialogProgramInfo::OnInitWindow()
{
  Update();

  CGUIDialog::OnInitWindow();
}

void CGUIDialogProgramInfo::SetProgram(const CFileItem *item)
{
  *m_programItem = *item;

  // TODO: add support for screenshots (look how cast is implemented for videos)

  // When the scraper throws an error, the program tag can be null here
  if (!item->HasProgramInfoTag())
    return;

  // TODO: check for local trailers and override remote ones (http://)

  CProgramThumbLoader loader;
  loader.LoadItem(m_programItem.get());
}

void CGUIDialogProgramInfo::Update()
{
  // setup plot text area
  std::string strTmp = m_programItem->GetProgramInfoTag()->m_strPlot;
  StringUtils::Trim(strTmp);
  SetLabel(CONTROL_TEXTAREA, strTmp);

  // update the thumbnail
  const CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = (CGUIImage*)pControl;
    pImageControl->FreeResources();
    pImageControl->SetFileName(m_programItem->GetArt("thumb"));
  }
}

void CGUIDialogProgramInfo::SetLabel(int iControl, const std::string &strLabel)
{
  if (strLabel.empty())
  {
    SET_CONTROL_LABEL(iControl, 416);  // "Not available"
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
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
