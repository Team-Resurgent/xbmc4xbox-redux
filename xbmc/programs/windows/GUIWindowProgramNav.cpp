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
#include "filesystem/ProgramDatabaseDirectory.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "FileItem.h"
#include "profiles/ProfilesManager.h"
#include "utils/StringUtils.h"

using namespace XFILE;
using namespace PROGRAMDATABASEDIRECTORY;

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
    if (items.IsProgramDb())
    {
      XFILE::CProgramDatabaseDirectory dir;
      PROGRAMDATABASEDIRECTORY::NODE_TYPE node = dir.GetDirectoryChildType(items.GetPath());
      if (node == NODE_TYPE_TITLE_GAMES)
        items.SetContent("games");
      else if (node == NODE_TYPE_GENRE)
        items.SetContent("genres");
      else if (node == NODE_TYPE_YEAR)
        items.SetContent("years");
      else
        items.SetContent("");
    }
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
    if (!item->IsDVD() && item->GetPath() != "add" && !item->IsParentFolder() &&
        (CProfilesManager::Get().GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser))
    {
      CProgramDatabase database;
      database.Open();
      ADDON::ScraperPtr info = database.GetScraperForPath(item->GetPath());

      if (!item->IsPlugin() && !item->IsAddonsPath())
      {
        if (info && info->Content() != CONTENT_NONE)
        {
          buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20442);
          buttons.Add(CONTEXT_BUTTON_SCAN, 13349);
        }
        else
          buttons.Add(CONTEXT_BUTTON_SET_CONTENT, 20333);
      }
    }
  }
}

bool CGUIWindowProgramNav::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if (CGUIDialogContextMenu::OnContextButton("program", item, button))
  {
    //! @todo should we search DB for entries from plugins?
    if (button == CONTEXT_BUTTON_REMOVE_SOURCE && !item->IsPlugin())
    {
      // if the source has been properly removed, remove the cached source list because the list has changed
      if (OnUnAssignContent(item->GetPath(), 20375, 20340))
        m_vecItems->RemoveDiscCache(GetID());
    }
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
