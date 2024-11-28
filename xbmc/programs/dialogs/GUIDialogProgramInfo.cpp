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
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIImage.h"
#include "utils/URIUtils.h"
#include "programs/windows/GUIWindowProgramNav.h"
#include "messaging/ApplicationMessenger.h"
#include "guilib/GUIWindowManager.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "guilib/Key.h"
#include "filesystem/Directory.h"

using namespace XFILE;
using namespace KODI::MESSAGING;

#define CONTROL_IMAGE                3
#define CONTROL_TEXTAREA             4
#define CONTROL_BTN_REFRESH          6
#define CONTROL_BTN_PLAY_TRAILER    11
#define CONTROL_BTN_GET_PATCHES     12

#define CONTROL_LIST                50

CGUIDialogProgramInfo::CGUIDialogProgramInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PROGRAM_INFO, "DialogProgramInfo.xml")
    , m_programItem(new CFileItem)
{
  m_bRefresh = false;
  m_screenshotList = new CFileItemList;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogProgramInfo::~CGUIDialogProgramInfo(void)
{
  delete m_screenshotList;
}

bool CGUIDialogProgramInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      ClearScreenshotList();
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_REFRESH)
      {
        m_bRefresh = true;
        Close();
        return true;
      }
      if (iControl == CONTROL_BTN_PLAY_TRAILER)
      {
        PlayTrailer();
      }
      if (iControl == CONTROL_BTN_GET_PATCHES)
      {
        CFileItemList items;
        std::string strRootPath = URIUtils::GetParentPath(m_programItem->GetPath());
        CDirectory::GetDirectory(strRootPath, items, ".xbe", DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO | DIR_FLAG_NO_FILE_DIRS);
        if (items.Size())
        {
          CGUIDialogSelect *pDlgSelect = static_cast<CGUIDialogSelect*>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
          if (pDlgSelect)
          {
            pDlgSelect->Reset();
            pDlgSelect->SetHeading(22080);

            for (int i = 0; i < items.Size(); i++)
            {
              CFileItemPtr item = items[i];
              if (item->IsXBE() && !item->IsDefaultXBE())
              {
                std::string strLabel = URIUtils::GetFileName(item->GetPath());
                URIUtils::RemoveExtension(strLabel);
                StringUtils::Replace(strLabel, "_", " + ");
                item->SetLabel(strLabel);
                pDlgSelect->Add(*item);
              }
            }

            pDlgSelect->Open();
            if (pDlgSelect->GetSelectedItem() < 0)
              return true;

            CFileItemPtr item = pDlgSelect->GetSelectedFileItem();
            // TODO: launch this XBE using IProgramLauncher
            return true;
          }
        }
        else
          CGUIDialogOK::ShowAndGetInput(m_programItem->GetLabel(), "Patches are not available!");
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogProgramInfo::OnInitWindow()
{
  m_bRefresh = false;

  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_REFRESH, CProfilesManager::Get().GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser);

  Update();

  CGUIDialog::OnInitWindow();
}

void CGUIDialogProgramInfo::SetProgram(const CFileItem *item)
{
  *m_programItem = *item;

  // setup screenshot list
  ClearScreenshotList();

  // When the scraper throws an error, the program tag can be null here
  if (!item->HasProgramInfoTag())
    return;

  CFileItemList items;
  std::string strScreenshots = URIUtils::AddFileToFolder(URIUtils::GetParentPath(item->GetPath()), "_resources", "screenshots");
  CDirectory::GetDirectory(strScreenshots, items, g_advancedSettings.m_pictureExtensions, DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO | DIR_FLAG_NO_FILE_DIRS);
  for (int i = 0; i < items.Size(); i++)
  {
    std::string strLabel = URIUtils::GetFileName(items[i]->GetPath());
    CFileItemPtr item(new CFileItem(strLabel));
    item->SetPath(items[i]->GetPath());
    item->SetArt("thumb", items[i]->GetPath());
    m_screenshotList->Add(item);
  }

  MediaType type = item->GetProgramInfoTag()->m_type;

  if (type == MediaTypeGame)
  {
    // local trailers should always override non-local, so check
    // for a local one if the registered trailer is online
    if (m_programItem->GetProgramInfoTag()->m_strTrailer.empty() ||
        URIUtils::IsInternetStream(m_programItem->GetProgramInfoTag()->m_strTrailer))
    {
      std::string localTrailer = m_programItem->FindTrailer();
      if (!localTrailer.empty())
      {
        m_programItem->GetProgramInfoTag()->m_strTrailer = localTrailer;
        CProgramDatabase database;
        if(database.Open())
        {
          database.SetSingleValue(PROGRAMDB_CONTENT_GAMES,
                                  m_programItem->GetProgramInfoTag()->m_iDbId,
                                  PROGRAMDB_ID_TRAILER,
                                  m_programItem->GetProgramInfoTag()->m_strTrailer);
          database.Close();
          CUtil::DeleteProgramDatabaseDirectoryCache();
        }
      }
    }
  }

  m_screenshotList->SetContent(CMediaTypes::ToPlural(type));

  CProgramThumbLoader loader;
  loader.LoadItem(m_programItem.get());
}

void CGUIDialogProgramInfo::Update()
{
  // setup plot text area
  std::string strTmp = m_programItem->GetProgramInfoTag()->m_strPlot;
  StringUtils::Trim(strTmp);
  SetLabel(CONTROL_TEXTAREA, strTmp);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_screenshotList);
  OnMessage(msg);

  if (!m_programItem->IsXBE())
  {
    SET_CONTROL_HIDDEN(CONTROL_BTN_GET_PATCHES);
  }
  else
  {
    SET_CONTROL_VISIBLE(CONTROL_BTN_GET_PATCHES);
  }

  // update the thumbnail
  const CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = (CGUIImage*)pControl;
    pImageControl->FreeResources();
    pImageControl->SetFileName(m_programItem->GetArt("thumb"));
  }
}

bool CGUIDialogProgramInfo::NeedRefresh() const
{
  return m_bRefresh;
}

void CGUIDialogProgramInfo::ClearScreenshotList()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
  OnMessage(msg);
  m_screenshotList->Clear();
}

void CGUIDialogProgramInfo::PlayTrailer()
{
  CFileItem item;
  item.SetPath(m_programItem->GetProgramInfoTag()->m_strTrailer);
  *item.GetProgramInfoTag() = *m_programItem->GetProgramInfoTag();
  item.GetProgramInfoTag()->m_strTitle = StringUtils::Format("%s (%s)",
                                                           m_programItem->GetProgramInfoTag()->m_strTitle.c_str(),
                                                           g_localizeStrings.Get(20410).c_str());
  CProgramThumbLoader::SetArt(item, m_programItem->GetArt());
  item.GetProgramInfoTag()->m_iDbId = -1;
  item.GetProgramInfoTag()->m_iFileId = -1;

  // Close the dialog.
  Close(true);

  if (item.IsPlayList())
  {
    CFileItemList *l = new CFileItemList; //don't delete,
    l->Add(boost::make_shared<CFileItem>(item));
    CApplicationMessenger::Get().PostMsg(TMSG_MEDIA_PLAY, -1, -1, static_cast<void*>(l));
  }
  else
    CApplicationMessenger::Get().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(item)));
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

std::string CGUIDialogProgramInfo::GetThumbnail() const
{
  return m_programItem->GetArt("thumb");
}

bool CGUIDialogProgramInfo::GetItemsForTag(const std::string &strHeading, const std::string &type, CFileItemList &items, int idTag /* = -1 */, bool showAll /* = true */)
{
  CProgramDatabase programdb;
  if (!programdb.Open())
    return false;

  MediaType mediaType = MediaTypeNone;
  std::string baseDir = "programdb://";
  std::string idColumn;
  if (type.compare(MediaTypeGame) == 0)
  {
    mediaType = MediaTypeGame;
    baseDir += "games";
    idColumn = "idGame";
  }

  baseDir += "/titles/";
  CProgramDbUrl programUrl;
  if (!programUrl.FromString(baseDir))
    return false;

  CProgramDatabase::Filter filter;
  if (idTag > 0)
  {
    if (!showAll)
      programUrl.AddOption("tagid", idTag);
    else
      filter.where = programdb.PrepareSQL("%s_view.%s NOT IN (SELECT tag_link.media_id FROM tag_link WHERE tag_link.tag_id = %d AND tag_link.media_type = '%s')", type.c_str(), idColumn.c_str(), idTag, type.c_str());
  }

  CFileItemList listItems;
  if (!programdb.GetSortedPrograms(mediaType, programUrl.ToString(), SortDescription(), listItems, filter) || listItems.Size() <= 0)
    return false;

  CGUIDialogSelect *dialog = static_cast<CGUIDialogSelect *>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
  if (dialog == nullptr)
    return false;

  listItems.Sort(SortByLabel, SortOrderAscending, CSettings::GetInstance().GetBool("filelists.ignorethewhensorting") ? SortAttributeIgnoreArticle : SortAttributeNone);

  dialog->Reset();
  dialog->SetMultiSelection(true);
  dialog->SetHeading(strHeading);
  dialog->SetItems(listItems);
  dialog->EnableButton(true, 186);
  dialog->Open();

  for (std::vector<int>::const_iterator it = dialog->GetSelectedItems().begin(); it != dialog->GetSelectedItems().end(); ++it)
    items.Add(listItems.Get(*it));
  return items.Size() > 0;
}

std::string CGUIDialogProgramInfo::GetLocalizedProgramType(const std::string &strType)
{
  if (CMediaTypes::IsMediaType(strType, MediaTypeGame))
    return g_localizeStrings.Get(38928);

  return "";
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
