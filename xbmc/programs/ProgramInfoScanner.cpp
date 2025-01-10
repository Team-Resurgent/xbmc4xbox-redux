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
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "filesystem/DirectoryCache.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "messaging/helpers/DialogHelper.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "TextureCache.h"
#include "URL.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/md5.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "ProgramThumbLoader.h"
#include "ProgramInfoDownloader.h"

using namespace XFILE;
using namespace ADDON;
using namespace KODI::MESSAGING;

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
    const std::vector<std::string> &regexps = g_advancedSettings.m_programExcludeFromScanRegExps;

    if (IsExcluded(strDirectory, regexps))
      return true;

    bool ignoreFolder = !m_scanAll && settings.noupdate;
    if (content == CONTENT_NONE || ignoreFolder)
      return true;

    std::string hash, dbHash;
    if (content == CONTENT_GAMES || content == CONTENT_APPS)
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

  bool CProgramInfoScanner::GetDetails(CFileItem *pItem, CScraperUrl &url, const ScraperPtr& scraper, CNfoFile *nfoFile, CGUIDialogProgress* pDialog /* = NULL */)
  {
    CProgramInfoTag programDetails;

    if (m_handle && !url.strTitle.empty())
      m_handle->SetText(url.strTitle);

    url.m_url.front().m_url += StringUtils::Format("&programPath=%s", pItem->GetPath().c_str());

    CProgramInfoDownloader igdb(scraper);
    bool ret = igdb.GetDetails(url, programDetails, pDialog);

    if (ret)
    {
      if (nfoFile)
        nfoFile->GetDetails(programDetails,NULL,true);

      if (m_handle && url.strTitle.empty())
        m_handle->SetText(programDetails.m_strTitle);

      if (pDialog)
      {
        pDialog->SetLine(1, programDetails.m_strTitle);
        pDialog->Progress();
      }

      *pItem->GetProgramInfoTag() = programDetails;
      return true;
    }
    return false; // no info found, or cancelled
  }

  void CProgramInfoScanner::ApplyThumbToFolder(const std::string &folder, const std::string &strThumb)
  {
    // copy icon to folder also;
    if (!strThumb.empty())
    {
      CFileItem folderItem(folder, true);
      CThumbLoader loader;
      loader.SetCachedImage(folderItem, "thumb", strThumb);
    }
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

      // skip patched XBEs (any XBE which is not default.xbe)
      if (pItem->IsXBE() && !pItem->IsDefaultXBE() && CSettings::GetInstance().GetBool("myprograms.defaultxbe"))
        continue;

      // we do this since we may have a override per dir
      ScraperPtr info2 = m_database.GetScraperForPath(pItem->m_bIsFolder ? pItem->GetPath() : items.GetPath());
      if (!info2) // skip
        continue;

      // Discard all exclude files defined by regExExclude
      if (IsExcluded(pItem->GetPath(), g_advancedSettings.m_programExcludeFromScanRegExps))
        continue;

      if (m_handle)
        m_handle->SetPercentage(i*100.f/items.Size());

      // clear our scraper cache
      info2->ClearCache();

      INFO_RET ret = INFO_CANCELLED;
      if (info2->Content() == CONTENT_GAMES || info2->Content() == CONTENT_APPS)
        ret = RetrieveInfoForProgram(pItem.get(), bDirNames, info2, useLocal, pURL, pDlgProgress);
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

  CInfoScanner::INFO_RET CProgramInfoScanner::RetrieveInfoForProgram(CFileItem *pItem, bool bDirNames, ScraperPtr &info2, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress)
  {
    if (pItem->m_bIsFolder || !pItem->IsProgram() || pItem->IsNFO())
      return INFO_NOT_NEEDED;

    if (ProgressCancelled(pDlgProgress, 38941, pItem->GetLabel()))
      return INFO_CANCELLED;

    if (m_database.HasProgramInfo(pItem->GetPath()))
      return INFO_HAVE_ALREADY;

    if (m_handle)
      m_handle->SetText(pItem->GetProgramName(bDirNames));

    CNfoFile::NFOResult result=CNfoFile::NO_NFO;
    CScraperUrl scrUrl;
    // handle .nfo files
    if (useLocal)
      result = CheckForNFOFile(pItem, bDirNames, info2, scrUrl);
    if (result == CNfoFile::FULL_NFO)
    {
      pItem->GetProgramInfoTag()->Reset();
      m_nfoReader.GetDetails(*pItem->GetProgramInfoTag());

      if (AddProgram(pItem, info2->Content(), bDirNames, true) < 0)
        return INFO_ERROR;
      return INFO_ADDED;
    }
    if (result == CNfoFile::URL_NFO || result == CNfoFile::COMBINED_NFO)
      pURL = &scrUrl;

    CScraperUrl url;
    int retVal = 0;
    if (pURL && !pURL->m_url.empty())
      url = *pURL;
    else if ((retVal = FindProgram(pItem->GetProgramName(bDirNames), info2, url, pDlgProgress)) <= 0)
      return retVal < 0 ? INFO_CANCELLED : INFO_NOT_FOUND;

    CLog::Log(LOGDEBUG,
              "ProgramInfoScanner: Fetching url '%s' using %s scraper (content: '%s')",
              url.m_url[0].m_url.c_str(), info2->Name().c_str(),
              TranslateContent(info2->Content()).c_str());

    if (GetDetails(pItem, url, info2,
                   (result == CNfoFile::COMBINED_NFO
                    || result == CNfoFile::PARTIAL_NFO) ? &m_nfoReader : NULL,
                   pDlgProgress))
    {
      if (AddProgram(pItem, info2->Content(), bDirNames, useLocal) < 0)
        return INFO_ERROR;
      return INFO_ADDED;
    }

    //! @todo This is not strictly correct as we could fail to download information here or error, or be cancelled
    return INFO_NOT_FOUND;
  }

  long CProgramInfoScanner::AddProgram(CFileItem *pItem, const CONTENT_TYPE &content, bool programFolder /* = false */, bool useLocal /* = true */, bool libraryImport /* = false */)
  {
    // ensure our database is open (this can get called via other classes)
    if (!m_database.Open())
      return -1;

    if (!libraryImport)
      GetArtwork(pItem, content, programFolder, useLocal);

    // ensure the art map isn't completely empty by specifying an empty thumb
    std::map<std::string, std::string> art = pItem->GetArt();
    if (art.empty())
      art["thumb"] = "";

    CProgramInfoTag &programDetails = *pItem->GetProgramInfoTag();
    if (programDetails.m_basePath.empty())
      programDetails.m_basePath = pItem->GetBaseProgramPath(programFolder);
    programDetails.m_parentPathID = m_database.AddPath(URIUtils::GetParentPath(programDetails.m_basePath));

    programDetails.m_strFileNameAndPath = pItem->GetPath();

    if (pItem->m_bIsFolder)
      programDetails.m_strPath = pItem->GetPath();

    std::string redactPath(CURL::GetRedacted(CURL::Decode(pItem->GetPath())));

    CLog::Log(LOGDEBUG, "ProgramInfoScanner: Adding new item to %s:%s", TranslateContent(content).c_str(), redactPath.c_str());
    long lResult = -1;

    if (content == CONTENT_GAMES || content == CONTENT_APPS)
    {
      // find local trailer first
      std::string strTrailer = pItem->FindTrailer();
      if (!strTrailer.empty())
        programDetails.m_strTrailer = strTrailer;

      // set program type (default game)
      programDetails.m_type = MediaTypeGame;
      if (content == CONTENT_APPS)
        programDetails.m_type = MediaTypeApp;

      lResult = m_database.SetDetailsForProgram(pItem->GetPath(), programDetails, art);
      programDetails.m_iDbId = lResult;
    }

    m_database.Close();

    // Do we need to announce here?

    return lResult;
  }

  void CProgramInfoScanner::GetArtwork(CFileItem *pItem, const CONTENT_TYPE &content, bool bApplyToDir, bool useLocal)
  {
    CGUIListItem::ArtMap art = pItem->GetArt();

    // get and cache thumb images
    std::vector<std::string> artTypes = CProgramThumbLoader::GetArtTypes(MediaTypeProgram);

    // find local art
    if (useLocal)
    {
      for (std::vector<std::string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
      {
        if (art.find(*i) == art.end())
        {
          std::string image = CProgramThumbLoader::GetLocalArt(*pItem, *i, bApplyToDir);
          if (!image.empty())
            art.insert(std::make_pair(*i, image));
        }
      }
    }

    // TODO: look for online art

    for (CGUIListItem::ArtMap::const_iterator i = art.begin(); i != art.end(); ++i)
      CTextureCache::Get().BackgroundCacheImage(i->second);

    pItem->SetArt(art);

    // parent folder to apply the thumb to
    std::string parentDir = URIUtils::GetBasePath(pItem->GetPath());
    if (bApplyToDir)
      ApplyThumbToFolder(parentDir, art["thumb"]);
  }

  std::string CProgramInfoScanner::GetnfoFile(CFileItem *item, bool bGrabAny) const
  {
    std::string nfoFile;
    // Find a matching .nfo file
    if (!item->m_bIsFolder)
    {
      // grab the folder path
      std::string strPath = URIUtils::GetDirectory(item->GetPath());

      if (bGrabAny)
      { // looking up by folder name - program.nfo takes priority - but not for stacked items (handled below)
        nfoFile = URIUtils::AddFileToFolder(strPath, "program.nfo");
        if (CFile::Exists(nfoFile))
          return nfoFile;

        nfoFile = URIUtils::AddFileToFolder(strPath, "game.nfo");
        if (CFile::Exists(nfoFile))
          return nfoFile;

        nfoFile = URIUtils::ReplaceExtension(item->GetPath(), ".nfo");
        if (CFile::Exists(nfoFile))
          return nfoFile;

        // look inside X4G resources folder
        nfoFile = URIUtils::AddFileToFolder(strPath, "_resources", "default.xml");
        if (CFile::Exists(nfoFile))
          return nfoFile;
      }

      // already an .nfo file?
      if (URIUtils::HasExtension(item->GetPath(), ".nfo"))
        nfoFile = item->GetPath();
      // no, create .nfo file
      else
        nfoFile = URIUtils::ReplaceExtension(item->GetPath(), ".nfo");

      // test file existence
      if (!nfoFile.empty() && !CFile::Exists(nfoFile))
        nfoFile.clear();
    }
    // folders can take any nfo file if there's a unique one
    if (item->m_bIsFolder || (bGrabAny && nfoFile.empty()))
    {
      // see if there is a unique nfo file in this folder, and if so, use that
      CFileItemList items;
      CDirectory dir;
      std::string strPath;
      if (item->m_bIsFolder)
        strPath = item->GetPath();
      else
        strPath = URIUtils::GetDirectory(item->GetPath());

      if (dir.GetDirectory(strPath, items, ".nfo", DIR_FLAG_DEFAULTS) && items.Size())
      {
        int numNFO = -1;
        for (int i = 0; i < items.Size(); i++)
        {
          if (items[i]->IsNFO())
          {
            if (numNFO == -1)
              numNFO = i;
            else
            {
              numNFO = -1;
              break;
            }
          }
        }
        if (numNFO > -1)
          return items[numNFO]->GetPath();
      }
    }

    return nfoFile;
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

  CNfoFile::NFOResult CProgramInfoScanner::CheckForNFOFile(CFileItem* pItem, bool bGrabAny, ScraperPtr& info, CScraperUrl& scrUrl)
  {
    std::string strNfoFile = GetnfoFile(pItem, bGrabAny);

    CNfoFile::NFOResult result=CNfoFile::NO_NFO;
    if (!strNfoFile.empty() && CFile::Exists(strNfoFile))
    {
      result = m_nfoReader.Create(strNfoFile,info);

      std::string type;
      switch(result)
      {
        case CNfoFile::COMBINED_NFO:
          type = "mixed";
          break;
        case CNfoFile::FULL_NFO:
          type = "full";
          break;
        case CNfoFile::URL_NFO:
          type = "URL";
          break;
        case CNfoFile::NO_NFO:
          type = "";
          break;
        case CNfoFile::PARTIAL_NFO:
          type = "partial";
          break;
        default:
          type = "malformed";
      }
      if (result != CNfoFile::NO_NFO)
        CLog::Log(LOGDEBUG, "ProgramInfoScanner: Found matching %s NFO file: %s", type.c_str(), CURL::GetRedacted(strNfoFile).c_str());
      if (result == CNfoFile::FULL_NFO)
        return result;

      if (result != CNfoFile::NO_NFO && result != CNfoFile::ERROR_NFO)
      {
        if (result != CNfoFile::PARTIAL_NFO)
        {
          scrUrl = m_nfoReader.ScraperUrl();
          StringUtils::RemoveCRLF(scrUrl.m_url[0].m_url);
          info = m_nfoReader.GetScraperInfo();
        }

        if (result != CNfoFile::URL_NFO)
          m_nfoReader.GetDetails(*pItem->GetProgramInfoTag());
      }
    }
    else
      CLog::Log(LOGDEBUG, "ProgramInfoScanner: No NFO file found. Using title search for '%s'", CURL::GetRedacted(pItem->GetPath()).c_str());

    return result;
  }

  bool CProgramInfoScanner::DownloadFailed(CGUIDialogProgress* pDialog)
  {
    if (g_advancedSettings.m_bProgramScannerIgnoreErrors)
      return true;

    if (pDialog)
    {
      CGUIDialogOK::ShowAndGetInput(20448, 20449);
      return false;
    }
    return HELPERS::ShowYesNoDialogText(20448, 20450) == HELPERS::YES;
  }

  bool CProgramInfoScanner::ProgressCancelled(CGUIDialogProgress* progress, int heading, const std::string &line1)
  {
    if (progress)
    {
      progress->SetHeading(heading);
      progress->SetLine(0, line1);
      progress->SetLine(2, "");
      progress->Progress();
      return progress->IsCanceled();
    }
    return m_bStop;
  }

  int CProgramInfoScanner::FindProgram(const std::string &programName, const ScraperPtr &scraper, CScraperUrl &url, CGUIDialogProgress *progress)
  {
    PROGRAMLIST programlist;
    CProgramInfoDownloader igdb(scraper);
    int returncode = igdb.FindProgram(programName, programlist, progress);
    if (returncode < 0 || (returncode == 0 && (m_bStop || !DownloadFailed(progress))))
    { // scraper reported an error, or we had an error and user wants to cancel the scan
      m_bStop = true;
      return -1; // cancelled
    }
    if (returncode > 0 && programlist.size())
    {
      url = programlist[0];
      return 1;  // found a program
    }
    return 0;    // didn't find anything
  }
}
