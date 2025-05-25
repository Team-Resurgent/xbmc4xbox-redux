/*
 *      Copyright (C) 2005-2015 Team Kodi
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

#include "ProgramInfoScanner.h"

#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "URL.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/md5.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace XFILE;
using namespace ADDON;

namespace PROGRAM
{
  CProgramInfoScanner::CProgramInfoScanner()
  {
    m_bStop = false;
    m_scanAll = false;
  }

  CProgramInfoScanner::~CProgramInfoScanner()
  {
  }

  void CProgramInfoScanner::Process()
  {
    m_bStop = false;

    try
    {
      if (m_showDialog && !CSettings::GetInstance().GetBool("programlibrary.backgroundupdate"))
      {
        CGUIDialogExtendedProgressBar* dialog =
          (CGUIDialogExtendedProgressBar*)g_windowManager.GetWindow(WINDOW_DIALOG_EXT_PROGRESS);
        if (dialog)
           m_handle = dialog->GetHandle(g_localizeStrings.Get(314));
      }

      // check if we only need to perform a cleaning
      if (m_bClean && m_pathsToScan.empty())
      {
        // TODO: invoke library cleaning job

        return;
      }

      unsigned int tick = XbmcThreads::SystemClockMillis();

      m_database.Open();

      m_bCanInterrupt = true;

      CLog::Log(LOGNOTICE, "ProgramInfoScanner: Starting scan ..");
      // Do we need to announce here?

      // Database operations should not be canceled
      // using Interupt() while scanning as it could
      // result in unexpected behaviour.
      m_bCanInterrupt = false;

      bool bCancelled = false;
      while (!bCancelled && !m_pathsToScan.empty())
      {
        /*
         * A copy of the directory path is used because the path supplied is
         * immediately removed from the m_pathsToScan set in DoScan(). If the
         * reference points to the entry in the set a null reference error
         * occurs.
         */
        std::string directory = *m_pathsToScan.begin();
        if (m_bStop)
        {
          bCancelled = true;
        }
        else if (!CDirectory::Exists(directory))
        {
          /*
           * Note that this will skip clean (if m_bClean is enabled) if the directory really
           * doesn't exist rather than a NAS being switched off.  A manual clean from settings
           * will still pick up and remove it though.
           */
          CLog::Log(LOGWARNING, "%s directory '%s' does not exist - skipping scan%s.", __FUNCTION__, CURL::GetRedacted(directory).c_str(), m_bClean ? " and clean" : "");
          m_pathsToScan.erase(m_pathsToScan.begin());
        }
        else if (!DoScan(directory))
          bCancelled = true;
      }

      if (!bCancelled)
      {
        if (m_bClean)
        {
          // TODO: invoke library cleaning job
        }
        else
        {
          if (m_handle)
            m_handle->SetTitle(g_localizeStrings.Get(331));
          m_database.Compress(false);
        }
      }

      g_infoManager.ResetLibraryBools();
      m_database.Close();

      tick = XbmcThreads::SystemClockMillis() - tick;
      CLog::Log(LOGNOTICE, "ProgramInfoScanner: Finished scan. Scanning for program info took %s", StringUtils::SecondsToTimeString(tick / 1000).c_str());
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "ProgramInfoScanner: Exception while scanning.");
    }

    m_bRunning = false;
    // Do we need to announce here?

    if (m_handle)
      m_handle->MarkFinished();
    m_handle = NULL;
  }

  void CProgramInfoScanner::Start(const std::string& strDirectory, bool scanAll)
  {
    m_strStartDir = strDirectory;
    m_scanAll = scanAll;
    m_pathsToScan.clear();
    m_pathsToClean.clear();

    m_database.Open();
    if (strDirectory.empty())
    { // scan all paths in the database.  We do this by scanning all paths in the db, and crossing them off the list as
      // we go.
      m_database.GetPaths(m_pathsToScan);
    }
    else
    { // scan all the paths of this subtree that is in the database
      std::vector<std::string> rootDirs;
      if (URIUtils::IsMultiPath(strDirectory))
        CMultiPathDirectory::GetPaths(strDirectory, rootDirs);
      else
        rootDirs.push_back(strDirectory);

      for (std::vector<std::string>::const_iterator it = rootDirs.begin(); it < rootDirs.end(); ++it)
      {
        m_pathsToScan.insert(*it);
        std::vector<std::pair<int, std::string> > subpaths;
        m_database.GetSubPaths(*it, subpaths);
        for (std::vector<std::pair<int, std::string> >::iterator it = subpaths.begin(); it < subpaths.end(); ++it)
          m_pathsToScan.insert(it->second);
      }
    }
    m_database.Close();
    m_bClean = g_advancedSettings.m_bProgramLibraryCleanOnUpdate;

    m_bRunning = true;
    Process();
  }

  void CProgramInfoScanner::Stop()
  {
    if (m_bCanInterrupt)
      m_database.Interupt();

    m_bStop = true;
  }

  static void OnDirectoryScanned(const std::string& strDirectory)
  {
    CGUIMessage msg(GUI_MSG_DIRECTORY_SCANNED, 0, 0, 0);
    msg.SetStringParam(strDirectory);
    g_windowManager.SendThreadMessage(msg);
  }

  bool CProgramInfoScanner::DoScan(const std::string& strDirectory)
  {
    if (m_handle)
    {
      m_handle->SetText(g_localizeStrings.Get(20415));
    }

    /*
     * Remove this path from the list we're processing. This must be done prior to
     * the check for file or folder exclusion to prevent an infinite while loop
     * in Process().
     */
    std::set<std::string>::iterator it = m_pathsToScan.find(strDirectory);
    if (it != m_pathsToScan.end())
      m_pathsToScan.erase(it);

    // load subfolder
    CFileItemList items;
    bool foundDirectly = false;
    bool bSkip = false;

    SScanSettings settings;
    ScraperPtr info = m_database.GetScraperForPath(strDirectory, settings, foundDirectly);
    CONTENT_TYPE content = info ? info->Content() : CONTENT_NONE;

    // exclude folders that match our exclude regexps
    const std::vector<std::string> &regexps = g_advancedSettings.m_gamesExcludeFromScanRegExps;

    if (IsExcluded(strDirectory, regexps))
      return true;

    bool ignoreFolder = !m_scanAll && settings.noupdate;
    if (content == CONTENT_NONE || ignoreFolder)
      return true;

    std::string hash, dbHash;
    if (content == CONTENT_GAMES)
    {
      if (m_handle)
        m_handle->SetTitle(StringUtils::Format(g_localizeStrings.Get(38911).c_str(), info->Name().c_str()));

      std::string fastHash;
      if (g_advancedSettings.m_bProgramLibraryUseFastHash)
        fastHash = GetFastHash(strDirectory, regexps);

      if (m_database.GetPathHash(strDirectory, dbHash) && !fastHash.empty() && fastHash == dbHash)
      { // fast hashes match - no need to process anything
        hash = fastHash;
      }
      else
      { // need to fetch the folder
        CDirectory::GetDirectory(strDirectory, items, g_advancedSettings.m_programExtensions,
                                 DIR_FLAG_DEFAULTS);
        items.Stack();

        // check whether to re-use previously computed fast hash
        if (!CanFastHash(items, regexps) || fastHash.empty())
          GetPathHash(items, hash);
        else
          hash = fastHash;
      }

      if (hash == dbHash)
      { // hash matches - skipping
        CLog::Log(LOGDEBUG, "ProgramInfoScanner: Skipping dir '%s' due to no change%s", CURL::GetRedacted(strDirectory).c_str(), !fastHash.empty() ? " (fasthash)" : "");
        bSkip = true;
      }
      else if (hash.empty())
      { // directory empty or non-existent - add to clean list and skip
        CLog::Log(LOGDEBUG, "ProgramInfoScanner: Skipping dir '%s' as it's empty or doesn't exist - adding to clean list", CURL::GetRedacted(strDirectory).c_str());
        if (m_bClean)
          m_pathsToClean.insert(m_database.GetPathId(strDirectory));
        bSkip = true;
      }
      else if (dbHash.empty())
      { // new folder - scan
        CLog::Log(LOGDEBUG, "ProgramInfoScanner: Scanning dir '%s' as not in the database", CURL::GetRedacted(strDirectory).c_str());
      }
      else
      { // hash changed - rescan
        CLog::Log(LOGDEBUG, "ProgramInfoScanner: Rescanning dir '%s' due to change (%s != %s)", CURL::GetRedacted(strDirectory).c_str(), dbHash.c_str(), hash.c_str());
      }
    }

    if (!bSkip)
    {
      if (RetrieveProgramInfo(items, settings.parent_name_root, content))
      {
        if (!m_bStop)
        {
          m_database.SetPathHash(strDirectory, hash);
          if (m_bClean)
            m_pathsToClean.insert(m_database.GetPathId(strDirectory));
          CLog::Log(LOGDEBUG, "ProgramInfoScanner: Finished adding information from dir %s", CURL::GetRedacted(strDirectory).c_str());
        }
      }
      else
      {
        if (m_bClean)
          m_pathsToClean.insert(m_database.GetPathId(strDirectory));
        CLog::Log(LOGDEBUG, "ProgramInfoScanner: No (new) information was found in dir %s", CURL::GetRedacted(strDirectory).c_str());
      }
    }
    else if (hash != dbHash)
    { // update the hash either way - we may have changed the hash to a fast version
      m_database.SetPathHash(strDirectory, hash);
    }

    if (m_handle)
      OnDirectoryScanned(strDirectory);

    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr pItem = items[i];

      if (m_bStop)
        break;

      // if we have a directory item (non-playlist) we then recurse into that folder
      // do not recurse for tv shows - we have already looked recursively for episodes
      if (pItem->m_bIsFolder && !pItem->IsParentFolder() && !pItem->IsPlayList() && settings.recurse > 0)
      {
        if (!DoScan(pItem->GetPath()))
        {
          m_bStop = true;
        }
      }
    }
    return !m_bStop;
  }

  int CProgramInfoScanner::GetPathHash(const CFileItemList &items, std::string &hash)
  {
    // Create a hash based on the filenames, filesize and filedate.  Also count the number of files
    if (0 == items.Size()) return 0;
    XBMC::XBMC_MD5 md5state;
    int count = 0;
    for (int i = 0; i < items.Size(); ++i)
    {
      const CFileItemPtr pItem = items[i];
      md5state.append(pItem->GetPath());
      md5state.append((unsigned char *)&pItem->m_dwSize, sizeof(pItem->m_dwSize));
      FILETIME time = pItem->m_dateTime;
      md5state.append((unsigned char *)&time, sizeof(FILETIME));
      if (pItem->IsProgram() && !pItem->IsPlayList() && !pItem->IsNFO())
        count++;
    }
    hash = md5state.getDigest();
    return count;
  }

  bool CProgramInfoScanner::RetrieveProgramInfo(CFileItemList& items, bool bDirNames, CONTENT_TYPE content, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress)
  {
    if (pDlgProgress)
    {
      if (items.Size() > 1 || items[0]->m_bIsFolder)
      {
        pDlgProgress->ShowProgressBar(true);
        pDlgProgress->SetPercentage(0);
      }
      else
        pDlgProgress->ShowProgressBar(false);

      pDlgProgress->Progress();
    }

    m_database.Open();

    bool FoundSomeInfo = false;
    std::vector<int> seenPaths;
    for (int i = 0; i < (int)items.Size(); ++i)
    {
      m_nfoReader.Close();
      CFileItemPtr pItem = items[i];

      // we do this since we may have a override per dir
      ScraperPtr info2 = m_database.GetScraperForPath(pItem->m_bIsFolder ? pItem->GetPath() : items.GetPath());
      if (!info2) // skip
        continue;

      // Discard all exclude files defined by regExExclude
      if (IsExcluded(pItem->GetPath(), g_advancedSettings.m_gamesExcludeFromScanRegExps))
        continue;

      if (m_handle)
        m_handle->SetPercentage(i*100.f/items.Size());

      // clear our scraper cache
      info2->ClearCache();

      INFO_RET ret = INFO_CANCELLED;
      if (info2->Content() == CONTENT_GAMES)
        ret = RetrieveInfoForGame(pItem.get(), bDirNames, info2, useLocal, pURL, pDlgProgress);
      else
      {
        CLog::Log(LOGERROR, "ProgramInfoScanner: Unknown content type %d (%s)", info2->Content(), CURL::GetRedacted(pItem->GetPath()).c_str());
        FoundSomeInfo = false;
        break;
      }
      if (ret == INFO_CANCELLED || ret == INFO_ERROR)
      {
        CLog::Log(LOGWARNING,
                  "ProgramInfoScanner: Error %u occurred while retrieving"
                  "information for %s.", ret,
                  CURL::GetRedacted(pItem->GetPath()).c_str());
        FoundSomeInfo = false;
        break;
      }
      if (ret == INFO_ADDED || ret == INFO_HAVE_ALREADY)
        FoundSomeInfo = true;
      else if (ret == INFO_NOT_FOUND)
        CLog::Log(LOGWARNING, "No information found for item '%s', it won't be added to the library.", CURL::GetRedacted(pItem->GetPath()).c_str());

      pURL = NULL;

      // Keep track of directories we've seen
      if (m_bClean && pItem->m_bIsFolder)
        seenPaths.push_back(m_database.GetPathId(pItem->GetPath()));
    }

    if(pDlgProgress)
      pDlgProgress->ShowProgressBar(false);

    m_database.Close();
    return FoundSomeInfo;
  }

  CInfoScanner::INFO_RET CProgramInfoScanner::RetrieveInfoForGame(CFileItem *pItem, bool bDirNames, ScraperPtr &info2, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress)
  {
    // TODO: implement actual scrapping and saving into database

    //! @todo This is not strictly correct as we could fail to download information here or error, or be cancelled
    return INFO_NOT_FOUND;
  }

  bool CProgramInfoScanner::CanFastHash(const CFileItemList &items, const std::vector<std::string> &excludes) const
  {
    if (!g_advancedSettings.m_bProgramLibraryUseFastHash)
      return false;

    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->m_bIsFolder && !CUtil::ExcludeFileOrFolder(items[i]->GetPath(), excludes))
        return false;
    }
    return true;
  }

  std::string CProgramInfoScanner::GetFastHash(const std::string &directory,
      const std::vector<std::string> &excludes) const
  {
    XBMC::XBMC_MD5 md5state;

    if (excludes.size())
      md5state.append(StringUtils::Join(excludes, "|"));

    struct __stat64 buffer;
    if (XFILE::CFile::Stat(directory, &buffer) == 0)
    {
      int64_t time = buffer.st_mtime;
      if (!time)
        time = buffer.st_ctime;
      if (time)
      {
        md5state.append((unsigned char *)&time, sizeof(time));
        return md5state.getDigest();
      }
    }
    return "";
  }
}
