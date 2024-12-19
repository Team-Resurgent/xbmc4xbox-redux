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

#include "GUIWindowProgramBase.h"
#include "Util.h"
#include "addons/GUIDialogAddonInfo.h"
#include "programs/ProgramInfoScanner.h"
#include "programs/ProgramLibraryQueue.h"
#include "programs/dialogs/GUIDialogProgramInfo.h"
#include "programs/launchers/ProgramLauncher.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogYesNo.h"
#include "Application.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "profiles/ProfilesManager.h"
#include "settings/dialogs/GUIDialogContentSettings.h"
#include "settings/AdvancedSettings.h"
#include "utils/FileUtils.h"
#include "utils/URIUtils.h"

using namespace PROGRAM;
using namespace ADDON;

CGUIWindowProgramBase::CGUIWindowProgramBase(int id, const std::string &xmlFile)
    : CGUIMediaWindow(id, xmlFile.c_str())
{
  m_thumbLoader.SetObserver(this);
}

CGUIWindowProgramBase::~CGUIWindowProgramBase()
{
}

bool CGUIWindowProgramBase::OnAction(const CAction &action)
{
  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowProgramBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    m_database.Close();
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_database.Open();
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowProgramBase::OnItemInfo(const CFileItem& fileItem, ADDON::ScraperPtr& scraper)
{
  if (fileItem.IsParentFolder() || fileItem.m_bIsShareOrDrive || fileItem.IsPath("add") || fileItem.IsPlayList())
    return;

  CFileItem item(fileItem);
  if ((item.IsProgramDb() && item.HasProgramInfoTag()) ||
      (item.HasProgramInfoTag() && item.GetProgramInfoTag()->m_iDbId != -1))
  {
    item.SetPath(item.GetProgramInfoTag()->GetPath());
  }
  else
  {
    // TODO: item is not in library, look for programs and scrape them
  }

  bool modified = ShowIGDB(CFileItemPtr(new CFileItem(item)), scraper);
  if (modified)
  {
    int itemNumber = m_viewControl.GetSelectedItem();
    Refresh();
    m_viewControl.SetSelectedItem(itemNumber);
  }
}

bool CGUIWindowProgramBase::ShowIGDB(CFileItemPtr item, const ScraperPtr &info2)
{
  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  CGUIDialogProgramInfo* pDlgInfo = (CGUIDialogProgramInfo*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_INFO);

  ScraperPtr info(info2); // use this as nfo might change it..

  if (!pDlgProgress) return false;
  if (!pDlgSelect) return false;
  if (!pDlgInfo) return false;

  // 1.  Check for already downloaded information, and if we have it, display our dialog
  //     Return if no Refresh is needed.
  bool bHasInfo=false;

  CProgramInfoTag programDetails;
  if (info)
  {
    m_database.Open();

    int dbId = item->HasProgramInfoTag() ? item->GetProgramInfoTag()->m_iDbId : -1;
    if (info->Content() == CONTENT_GAMES)
    {
      bHasInfo = m_database.GetGameInfo(item->GetPath(), programDetails, dbId);
    }
    m_database.Close();
  }
  else if(item->HasProgramInfoTag())
  {
    bHasInfo = true;
    programDetails = *item->GetProgramInfoTag();
  }

  bool needsRefresh = false;
  if (bHasInfo)
  {
    *item->GetProgramInfoTag() = programDetails;
    pDlgInfo->SetProgram(item.get());
    pDlgInfo->Open();
    needsRefresh = pDlgInfo->NeedRefresh();
    if (!needsRefresh)
      return false;
    // check if the item in the program info dialog has changed and if so, get the new item
    else if (pDlgInfo->GetCurrentListItem() != NULL)
    {
      item = pDlgInfo->GetCurrentListItem();

      if (item->IsProgramDb() && item->HasProgramInfoTag())
        item->SetPath(item->GetProgramInfoTag()->GetPath());
    }
  }

  // quietly return if Internet lookups are disabled
  if (!CProfilesManager::Get().GetCurrentProfile().canWriteDatabases() && !g_passwordManager.bMasterUser)
    return false;

  if (!info)
    return false;

  if (g_application.IsProgramScanning())
  {
    CGUIDialogOK::ShowAndGetInput(13346, 14057);
    return false;
  }

  bool listNeedsUpdating = false;
  // 3. Run a loop so that if we Refresh we re-run this block
  do
  {
    if (!CProgramLibraryQueue::GetInstance().RefreshItemModal(item, needsRefresh))
      return listNeedsUpdating;

    // remove directory caches and reload images
    CUtil::DeleteProgramDatabaseDirectoryCache();
    CGUIMessage reload(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
    OnMessage(reload);

    pDlgInfo->SetProgram(item.get());
    pDlgInfo->Open();
    item->SetArt("thumb", pDlgInfo->GetThumbnail());
    needsRefresh = pDlgInfo->NeedRefresh();
    listNeedsUpdating = true;
  } while (needsRefresh);

  return listNeedsUpdating;
}

void CGUIWindowProgramBase::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  // contextual buttons
  if (item && !item->IsParentFolder())
  {
    if (item->IsSmartPlayList() || m_vecItems->IsSmartPlayList())
      buttons.Add(CONTEXT_BUTTON_EDIT_SMART_PLAYLIST, 586);
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowProgramBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  switch (button)
  {
  case CONTEXT_BUTTON_SET_CONTENT:
    {
      OnAssignContent(item->HasProgramInfoTag() && !item->GetProgramInfoTag()->m_strPath.empty() ? item->GetProgramInfoTag()->m_strPath : static_cast<const std::string&>(item->GetPath()));
      return true;
    }

  case CONTEXT_BUTTON_SCAN:
    {
      if (!item)
        return false;
      ADDON::ScraperPtr info;
      SScanSettings settings;
      GetScraperForItem(item.get(), info, settings);
      std::string strPath = item->GetPath();
      if (item->IsProgramDb() && (!item->m_bIsFolder || item->GetProgramInfoTag()->m_strPath.empty()))
        return false;

      if (item->IsProgramDb())
        strPath = item->GetProgramInfoTag()->m_strPath;

      if (!info || info->Content() == CONTENT_NONE)
        return false;

      if (item->m_bIsFolder)
      {
        OnScan(strPath, true);
      }
      else
        // TODO: implement on item info

      return true;
    }

  case CONTEXT_BUTTON_EDIT_SMART_PLAYLIST:
    {
      std::string playlist = m_vecItems->Get(itemNumber)->IsSmartPlayList() ? m_vecItems->Get(itemNumber)->GetPath() : m_vecItems->GetPath(); // save path as activatewindow will destroy our items
      if (CGUIDialogSmartPlaylistEditor::EditPlaylist(playlist, "program"))
        Refresh(true); // need to update
      return true;
    }
  default:
    break;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowProgramBase::OnPlayMedia(int iItem, const std::string &player)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() )
    return false;

  CFileItemPtr pItem = m_vecItems->Get(iItem);

  return LAUNCHERS::CProgramLauncher::LaunchProgram(pItem->HasProgramInfoTag() ? pItem->GetProgramInfoTag()->m_strFileNameAndPath : pItem->GetPath());
}

bool CGUIWindowProgramBase::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory, updateFilterPath))
    return false;

  // might already be running from GetGroupedItems
  if (!m_thumbLoader.IsLoading())
    m_thumbLoader.Load(*m_vecItems);

  return true;
}

/* Look for executable inside folder and return if exists */
std::string GetExecutable(const std::string& strFolder)
{
  // default.xbe have priority
  std::string strExecutable = URIUtils::AddFileToFolder(strFolder, "default.xbe");
  if (XFILE::CFile::Exists(strExecutable))
    return strExecutable;

  std::string strFilename = strFolder;
  URIUtils::RemoveSlashAtEnd(strFilename);
  int iPos = strFilename.rfind('\\');
  if (iPos == std::string::npos)
    return "";
  strFilename = strFilename.substr(iPos);

  // look for first executable with supported extension
  std::vector<std::string> vecProgramExtensions = StringUtils::Split(g_advancedSettings.m_programExtensions, "|");
  for (unsigned int i = 0; i < vecProgramExtensions.size(); ++i)
  {
    strExecutable = URIUtils::AddFileToFolder(strFolder, strFilename + vecProgramExtensions.at(i));
    if (XFILE::CFile::Exists(strExecutable))
      return strExecutable;
  }

  return "";
}

bool CGUIWindowProgramBase::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  bool bResult = CGUIMediaWindow::GetDirectory(strDirectory, items);

  // add in the "New Playlist" item if we're in the playlists folder
  if ((items.GetPath() == "special://programplaylists/") && !items.Contains("newplaylist://"))
  {
    CFileItemPtr newPlaylist(new CFileItem("newsmartplaylist://program", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(21437));  // "new smart playlist..."
    newPlaylist->SetLabelPreformated(true);
    items.Add(newPlaylist);
  }

  if (URIUtils::IsDOSPath(strDirectory))
  { // listing programs by sources (ex. F:\Games\)
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr item = items[i];
      // flatten any folder
      if (item->m_bIsFolder && !item->IsParentFolder())
      {
        std::string strExecutable = GetExecutable(item->GetPath());
        if (!strExecutable.empty())
        {
          item->SetPath(strExecutable);
          item->m_bIsFolder = false;
        }
      }

      if (!item->m_bIsFolder)
      {
        // remove unsupported executables
        if (!URIUtils::HasExtension(item->GetPath(), g_advancedSettings.m_programExtensions))
        {
          items.Remove(i--);
          continue;
        }

        // add program executable to database
        m_database.AddFile(item->GetPath());

        // load XBMC4Gamers artwork
        CFileItemList artwork;
        std::string strPath = URIUtils::AddFileToFolder(URIUtils::GetParentPath(item->GetPath()), "_resources\\artwork\\");
        if(XFILE::CDirectory::Exists(strPath) && XFILE::CDirectory::GetDirectory(strPath, artwork, g_advancedSettings.m_pictureExtensions, XFILE::DIR_FLAG_DEFAULTS))
        {
          for (int i = 0; i < artwork.Size(); i++)
          {
            std::string strProperty(artwork[i]->GetLabel());
            URIUtils::RemoveExtension(strProperty);
            item->SetArt(strProperty, artwork[i]->GetPath());
          }
        }

        CProgramInfoTag tag;
        // look for local information
        strPath = URIUtils::ReplaceExtension(item->GetPath(), ".nfo");
        if (XFILE::CFile::Exists(strPath))
        {
          CXBMCTinyXML doc;
          if (doc.LoadFile(strPath) && doc.RootElement())
          {
            const TiXmlElement* metadata = doc.RootElement();
            tag.Load(metadata);
          }
        }

        // look for local trailer
        std::string strTrailer = item->FindTrailer();
        if (!strTrailer.empty())
          tag.SetTrailer(strTrailer);

        // set some default values
        if (tag.m_strTitle.empty())
        {
          std::string strTitle;
          if (item->IsXBE())
            CUtil::GetXBEDescription(item->GetPath(), strTitle);
          tag.SetTitle(strTitle);
        }
        tag.m_strFileNameAndPath = item->GetPath();

        // set program information
        item->SetFromProgramInfoTag(tag);
      }
    }
  }
  return bResult;
}

int CGUIWindowProgramBase::GetScraperForItem(CFileItem *item, ADDON::ScraperPtr &info, SScanSettings& settings)
{
  if (!item)
    return 0;

  if (m_vecItems->IsPlugin())
  {
    info.reset();
    return 0;
  }

  bool foundDirectly = false;
  info = m_database.GetScraperForPath(item->HasProgramInfoTag() && !item->GetProgramInfoTag()->m_strPath.empty() ? std::string(item->GetProgramInfoTag()->m_strPath) : item->GetPath(), settings, foundDirectly);
  return foundDirectly ? 1 : 0;
}

void CGUIWindowProgramBase::OnScan(const std::string& strPath, bool scanAll)
{
    g_application.StartProgramScan(strPath, true, scanAll);
}

bool CGUIWindowProgramBase::OnUnAssignContent(const std::string &path, int header, int text)
{
  bool bCanceled;
  CProgramDatabase db;
  db.Open();
  if (CGUIDialogYesNo::ShowAndGetInput(header, text, bCanceled, "", "", CGUIDialogYesNo::NO_TIMEOUT))
  {
    CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    db.RemoveContentForPath(path, progress);
    db.Close();
    CUtil::DeleteProgramDatabaseDirectoryCache();
    return true;
  }
  else
  {
    if (!bCanceled)
    {
      ADDON::ScraperPtr info;
      SScanSettings settings;
      settings.exclude = true;
      db.SetScraperForPath(path,info,settings);
    }
  }
  db.Close();

  return false;
}

void CGUIWindowProgramBase::OnAssignContent(const std::string &path)
{
  bool bScan=false;
  CProgramDatabase db;
  db.Open();

  SScanSettings settings;
  ADDON::ScraperPtr info = db.GetScraperForPath(path, settings);

  ADDON::ScraperPtr info2(info);

  if (CGUIDialogContentSettings::Show(info, settings))
  {
    if(settings.exclude || (!info && info2))
    {
      OnUnAssignContent(path, 20375, 20340);
    }
    else if (info != info2)
    {
      if (OnUnAssignContent(path, 20442, 20443))
        bScan = true;
    }
    db.SetScraperForPath(path, info, settings);
  }

  if (bScan)
  {
    g_application.StartProgramScan(path, true, true);
  }
}
