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
#include "Util.h"
#include "filesystem/ProgramDatabaseDirectory.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "FileItem.h"
#include "Application.h"
#include "profiles/ProfilesManager.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "guilib/GUIKeyboardFactory.h"
#include "programs/dialogs/GUIDialogProgramInfo.h"

using namespace XFILE;
using namespace PROGRAMDATABASEDIRECTORY;

CGUIWindowProgramNav::CGUIWindowProgramNav(void)
    : CGUIWindowProgramBase(WINDOW_PROGRAM_NAV, "MyProgramNav.xml")
{
  m_thumbLoader.SetObserver(this);
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
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    break;
  // update the display
  case GUI_MSG_REFRESH_THUMBS:
    Refresh();
    break;
  }
  return CGUIWindowProgramBase::OnMessage(message);
}

bool CGUIWindowProgramNav::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  items.ClearArt();
  items.ClearProperties();

  bool bResult = CGUIWindowProgramBase::GetDirectory(strDirectory, items);
  if (bResult)
  {
    if (items.IsProgramDb())
    {
      XFILE::CProgramDatabaseDirectory dir;
      PROGRAMDATABASEDIRECTORY::NODE_TYPE node = dir.GetDirectoryChildType(items.GetPath());
      if (node == NODE_TYPE_TITLE_GAMES ||
          node == NODE_TYPE_RECENTLY_ADDED_GAMES)
        items.SetContent("games");
      else if (node == NODE_TYPE_TITLE_APPS)
        items.SetContent("apps");
      else if (node == NODE_TYPE_GENRE ||
               node == NODE_TYPE_DEVELOPER  ||
               node == NODE_TYPE_PUBLISHER  ||
               node == NODE_TYPE_DESCRIPTOR ||
               node == NODE_TYPE_GENERALFEATURE ||
               node == NODE_TYPE_ONLINEFEATURE ||
               node == NODE_TYPE_PLATFORM)
        items.SetContent("genres");
      else if (node == NODE_TYPE_YEAR)
        items.SetContent("years");
      else if (node == NODE_TYPE_TAGS)
        items.SetContent("tags");
      else
        items.SetContent("");
    }
    else if (URIUtils::PathEquals(items.GetPath(), "special://programplaylists/"))
      items.SetContent("playlists");
    else if (URIUtils::IsDOSPath(strDirectory))
      items.SetContent("programs");

    CProgramDbUrl programUrl;
    if (programUrl.FromString(items.GetPath()) && items.GetContent() == "tags" &&
       !items.Contains("newtag://" + programUrl.GetType()))
    {
      CFileItemPtr newTag(new CFileItem("newtag://" + programUrl.GetType(), false));
      newTag->SetLabel(g_localizeStrings.Get(20462));
      newTag->SetLabelPreformated(true);
      newTag->SetSpecialSort(SortSpecialOnTop);
      items.Add(newTag);
    }
  }
  return bResult;
}

void CGUIWindowProgramNav::OnItemInfo(const CFileItem& fileItem, ADDON::ScraperPtr& scraper)
{
  if (!scraper || scraper->Content() == CONTENT_NONE)
  {
    m_database.Open();
    if (fileItem.IsProgramDb())
      scraper = m_database.GetScraperForPath(fileItem.GetProgramInfoTag()->m_strPath);
    else
    {
      std::string strPath,strFile;
      URIUtils::Split(fileItem.GetPath(),strPath,strFile);
      scraper = m_database.GetScraperForPath(strPath);
    }
    m_database.Close();
  }
  CGUIWindowProgramBase::OnItemInfo(fileItem, scraper);
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
  else
  {
    if (!item->IsParentFolder())
    {
      // can we update the database?
      if (CProfilesManager::Get().GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser)
      {
        if (!g_application.IsProgramScanning() && item->IsProgramDb() && item->HasProgramInfoTag() &&
           (item->GetProgramInfoTag()->m_type == MediaTypeGame ||   // games
            item->GetProgramInfoTag()->m_type == "tag"))            // tags
        {
          buttons.Add(CONTEXT_BUTTON_EDIT, 16106);
        }
      }

      if (!m_vecItems->IsProgramDb() && !m_vecItems->IsVirtualDirectoryRoot())
      { // non-program db items, file operations are allowed
        bool inPlaylists = m_vecItems->IsPath(CUtil::ProgramPlaylistsLocation()) ||
                          m_vecItems->IsPath("special://programplaylists/");
        if (inPlaylists && (item->IsPlayList() || item->IsSmartPlayList()))
        {
          buttons.Add(CONTEXT_BUTTON_DELETE, 117);
          buttons.Add(CONTEXT_BUTTON_RENAME, 118);
        }
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
  switch (button)
  {
  case CONTEXT_BUTTON_EDIT:
    {
      CONTEXT_BUTTON ret = (CONTEXT_BUTTON)CGUIDialogProgramInfo::ManageProgramItem(item);
      if (ret >= 0)
      {
        Refresh(true);
      }
      return true;
    }

  default:
    break;
  }
  return CGUIWindowProgramBase::OnContextButton(itemNumber, button);
}

bool CGUIWindowProgramNav::OnAddMediaSource()
{
  return CGUIDialogMediaSource::ShowAndAddMediaSource("program");
}

bool CGUIWindowProgramNav::OnClick(int iItem, const std::string &player)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (StringUtils::StartsWithNoCase(item->GetPath(), "newtag://"))
  {
    // dont allow update while scanning
    if (g_application.IsProgramScanning())
    {
      CGUIDialogOK::ShowAndGetInput(257, 14057);
      return true;
    }

    //Get the new title
    std::string strTag;
    if (!CGUIKeyboardFactory::ShowAndGetInput(strTag, g_localizeStrings.Get(20462), false))
      return true;

    CProgramDatabase programdb;
    if (!programdb.Open())
      return true;

    // get the media type and convert from plural to singular (by removing the trailing "s")
    std::string mediaType = item->GetPath().substr(9);
    mediaType = mediaType.substr(0, mediaType.size() - 1);
    std::string localizedType = CGUIDialogProgramInfo::GetLocalizedProgramType(mediaType);
    if (localizedType.empty())
      return true;

    if (!programdb.GetSingleValue("tag", "tag.tag_id", programdb.PrepareSQL("tag.name = '%s' AND tag.tag_id IN (SELECT tag_link.tag_id FROM tag_link WHERE tag_link.media_type = '%s')", strTag.c_str(), mediaType.c_str())).empty())
    {
      std::string strError = StringUtils::Format(g_localizeStrings.Get(20463).c_str(), strTag.c_str());
      CGUIDialogOK::ShowAndGetInput(20462, boost::move(strError));
      return true;
    }

    int idTag = programdb.AddTag(strTag);
    CFileItemList items;
    std::string strLabel = StringUtils::Format(g_localizeStrings.Get(20464).c_str(), localizedType.c_str());
    if (CGUIDialogProgramInfo::GetItemsForTag(strLabel, mediaType, items, idTag))
    {
      for (int index = 0; index < items.Size(); index++)
      {
        if (!items[index]->HasProgramInfoTag() || items[index]->GetProgramInfoTag()->m_iDbId <= 0)
          continue;

        programdb.AddTagToItem(items[index]->GetProgramInfoTag()->m_iDbId, idTag, mediaType);
      }
    }

    Refresh(true);
    return true;
  }

  return CGUIWindowProgramBase::OnClick(iItem, player);
}

std::string CGUIWindowProgramNav::GetStartFolder(const std::string &dir)
{
  std::string lower(dir); StringUtils::ToLower(lower);
  if (lower == "files")
    return "sources://program/";
  return CGUIWindowProgramBase::GetStartFolder(dir);
}
