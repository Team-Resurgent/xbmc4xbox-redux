/*
 *      Copyright (C) 2014 Team XBMC
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

#include "ProgramLibraryRefreshingJob.h"
#include "NfoFile.h"
#include "TextureDatabase.h"
#include "addons/Scraper.h"
#include "dialogs/GUIDialogOK.h"
#include "utils/URIUtils.h"
#include "programs/ProgramDatabase.h"
#include "programs/ProgramInfoScanner.h"

CProgramLibraryRefreshingJob::CProgramLibraryRefreshingJob(CFileItemPtr item, bool forceRefresh, bool ignoreNfo /* = false */, const std::string& searchTitle /* = "" */)
  : CProgramLibraryProgressJob(nullptr),
    m_item(item),
    m_forceRefresh(forceRefresh),
    m_ignoreNfo(ignoreNfo),
    m_searchTitle(searchTitle)
{ }

CProgramLibraryRefreshingJob::~CProgramLibraryRefreshingJob()
{ }

bool CProgramLibraryRefreshingJob::operator==(const CJob* job) const
{
  if (strcmp(job->GetType(), GetType()) != 0)
    return false;

  const CProgramLibraryRefreshingJob* refreshingJob = dynamic_cast<const CProgramLibraryRefreshingJob*>(job);
  if (refreshingJob == nullptr)
    return false;

  return m_item->GetPath() == refreshingJob->m_item->GetPath();
}

// TODO: add support for online scrapers
bool CProgramLibraryRefreshingJob::Work(CProgramDatabase &db)
{
  if (m_item.get() == nullptr)
    return false;

  // determine the scraper for the item's path
  PROGRAM::SScanSettings scanSettings;
  ADDON::ScraperPtr scraper = db.GetScraperForPath(m_item->GetPath(), scanSettings);
  if (scraper.get() == nullptr)
    return false;

  // copy the scraper in case we need it again
  ADDON::ScraperPtr originalScraper(scraper);

  // get the item's correct title
  std::string itemTitle = m_searchTitle;
  if (itemTitle.empty())
    itemTitle = m_item->GetProgramName(scanSettings.parent_name);

  CScraperUrl scraperUrl;
  PROGRAM::CProgramInfoScanner scanner;
  bool needsRefresh = m_forceRefresh;
  bool ignoreNfo = m_ignoreNfo;

  // run this in a loop in case we need to refresh again
  bool failure = false;
  do
  {
    if (!ignoreNfo)
    {
      // check if there's an NFO for the item
      CNfoFile::NFOResult nfoResult = scanner.CheckForNFOFile(m_item.get(), scanSettings.parent_name_root, scraper, scraperUrl);
      // if there's no NFO remember it in case we have to refresh again
      if (nfoResult == CNfoFile::ERROR_NFO)
        ignoreNfo = true;
    }

    // before we start downloading all the necessary information cleanup any existing artwork and hashes
    CTextureDatabase textureDb;
    if (textureDb.Open())
    {
      for (CGUIListItem::ArtMap::const_iterator it = m_item->GetArt().begin(); it != m_item->GetArt().end(); ++it)
        textureDb.InvalidateCachedTexture((*it).second);

      textureDb.Close();
    }
    m_item->ClearArt();

    // put together the list of items to refresh
    std::string path = m_item->GetPath();
    CFileItemList items;
    if (m_item->HasProgramInfoTag() && m_item->GetProgramInfoTag()->m_iDbId > 0)
    {
      items.Add(CFileItemPtr(new CFileItem(*m_item->GetProgramInfoTag())));

      // update the path to the real path (instead of a programdb:// one)
      path = m_item->GetProgramInfoTag()->m_strPath;
    }
    else
      items.Add(CFileItemPtr(new CFileItem(*m_item)));

    // set the proper path of the list of items to lookup
    items.SetPath(m_item->m_bIsFolder ? URIUtils::GetParentPath(path) : URIUtils::GetDirectory(path));

    // prepare the progress dialog for downloading all the necessary information
    SetTitle(g_localizeStrings.Get(38941));
    SetText(scraperUrl.strTitle);
    SetProgress(0);

    // remove any existing data for the item we're going to refresh
    if (m_item->GetProgramInfoTag()->m_iDbId > 0)
    {
      int dbId = m_item->GetProgramInfoTag()->m_iDbId;
      if (scraper->Content() == CONTENT_GAMES)
        db.DeleteGame(dbId);
    }

    // finally download the information for the item
    if (!scanner.RetrieveProgramInfo(items, scanSettings.parent_name,
                                     scraper->Content(), !ignoreNfo,
                                     scraperUrl.m_url.empty() ? NULL : &scraperUrl,
                                     GetProgressDialog()))
    {
      // something went wrong
      MarkFinished();

      // check if the user cancelled
      if (!IsCancelled() && IsModal())
        CGUIDialogOK::ShowAndGetInput(195, itemTitle);

      return false;
    }

    // retrieve the updated information from the database
    if (scraper->Content() == CONTENT_GAMES)
      db.GetGameInfo(m_item->GetPath(), *m_item->GetProgramInfoTag());

    // we're finally done
    MarkFinished();
    break;
  } while (needsRefresh);

  if (failure && IsModal())
    CGUIDialogOK::ShowAndGetInput(195, itemTitle);

  return true;
}
