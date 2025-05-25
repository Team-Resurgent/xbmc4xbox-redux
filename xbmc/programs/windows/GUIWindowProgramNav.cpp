/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowProgramNav.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "FileItem.h"
#include "utils/StringUtils.h"

CGUIWindowProgramNav::CGUIWindowProgramNav(void)
    : CGUIWindowProgramBase(WINDOW_PROGRAM_NAV, "MyProgramNav.xml")
{

}

CGUIWindowProgramNav::~CGUIWindowProgramNav(void)
{
}

bool CGUIWindowProgramNav::OnAction(const CAction &action)
{
  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowProgramNav::OnMessage(CGUIMessage& message)
{
  return CGUIWindowProgramBase::OnMessage(message);
}

bool CGUIWindowProgramNav::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  items.ClearArt();
  items.ClearProperties();

  bool bResult = CGUIWindowProgramBase::GetDirectory(strDirectory, items);
  if (bResult)
  {
    // TODO: list nodes, get node items, list playlists, etc.
  }
  return bResult;
}

void CGUIWindowProgramNav::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  CGUIWindowProgramBase::GetContextButtons(itemNumber, buttons);

  if (!item)
  {
    // nothing to do here
  }
  else if (m_vecItems->IsPath("sources://program/"))
  {
    // get the usual shares
    CGUIDialogContextMenu::GetContextButtons("program", item, buttons);
  }
}

bool CGUIWindowProgramNav::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if (CGUIDialogContextMenu::OnContextButton("program", item, button))
  {
    Refresh();
    return true;
  }
  return CGUIWindowProgramBase::OnContextButton(itemNumber, button);
}

bool CGUIWindowProgramNav::OnAddMediaSource()
{
  return CGUIDialogMediaSource::ShowAndAddMediaSource("program");
}

std::string CGUIWindowProgramNav::GetStartFolder(const std::string &dir)
{
  std::string lower(dir); StringUtils::ToLower(lower);
  if (lower == "files")
    return "sources://program/";
  return CGUIWindowProgramBase::GetStartFolder(dir);
}
