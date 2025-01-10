#pragma once
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

#include "InfoScanner.h"
#include "NfoFile.h"
#include "ProgramDatabase.h"
#include "addons/Scraper.h"

class CFileItem;
class CFileItemList;

namespace PROGRAM
{
  typedef struct SScanSettings
  {
    SScanSettings() { parent_name = parent_name_root = noupdate = exclude = false; recurse = 1;}
    bool parent_name;       /* use the parent dirname as name of lookup */
    bool parent_name_root;  /* use the name of directory where scan started as name for files in that dir */
    int  recurse;           /* recurse into sub folders (indicate levels) */
    bool noupdate;          /* exclude from update library function */
    bool exclude;           /* exclude this path from scraping */
    // TODO: extend and implement preferable format for Xbox games (XBE, XISO, CSO, CCI)
  } SScanSettings;
  class CProgramInfoScanner : public CInfoScanner
  {
  public:
    CProgramInfoScanner();
    virtual ~CProgramInfoScanner();

    /*! \brief Scan a folder using the background scanner
     \param strDirectory path to scan
     \param scanAll whether to scan everything not already scanned (regardless of whether the user normally doesn't want a folder scanned.) Defaults to false.
     */
    void Start(const std::string& strDirectory, bool scanAll = false);
    void Stop();

    /*! \brief Add an item to the database.
     \param pItem item to add to the database.
     \param content content type of the item.
     \param programFolder whether the program is represented by a folder (single game per folder). Defaults to false.
     \param useLocal whether to use local information for artwork etc.
     \param libraryImport Whether this call belongs to a full library import or not. Defaults to false.
     \return database id of the added item, or -1 on failure.
     */
    long AddProgram(CFileItem *pItem, const CONTENT_TYPE &content, bool programFolder = false, bool useLocal = true, bool libraryImport = false);


    /*! \brief Retrieve information for a list of items and add them to the database.
     \param items list of items to retrieve info for.
     \param bDirNames whether we should use folder or file names for lookups.
     \param content type of content to retrieve.
     \param useLocal should local data (.nfo and art) be used. Defaults to true.
     \param pURL an optional URL to use to retrieve online info.  Defaults to NULL.
     \param fetchEpisodes whether we are fetching episodes with shows. Defaults to true.
     \param pDlgProgress progress dialog to update and check for cancellation during processing.  Defaults to NULL.
     \return true if we successfully found information for some items, false otherwise
     */
    bool RetrieveProgramInfo(CFileItemList& items, bool bDirNames, CONTENT_TYPE content, bool useLocal = true, CScraperUrl *pURL = NULL, CGUIDialogProgress* pDlgProgress = NULL);

    static void ApplyThumbToFolder(const std::string &folder, const std::string &imdbThumb);
    static bool DownloadFailed(CGUIDialogProgress* pDlgProgress);
    CNfoFile::NFOResult CheckForNFOFile(CFileItem* pItem, bool bGrabAny, ADDON::ScraperPtr& scraper, CScraperUrl& scrUrl);

    /*! \brief Retrieve any artwork associated with an item
     \param pItem item to find artwork for.
     \param content content type of the item.
     \param bApplyToDir whether we should apply any thumbs to a folder.  Defaults to false.
     \param useLocal whether we should use local thumbs. Defaults to true.
     */
    void GetArtwork(CFileItem *pItem, const CONTENT_TYPE &content, bool bApplyToDir=false, bool useLocal=true);

  protected:
    virtual void Process();
    bool DoScan(const std::string& strDirectory);

    INFO_RET RetrieveInfoForProgram(CFileItem *pItem, bool bDirNames, ADDON::ScraperPtr &scraper, bool useLocal, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress);

    /*! \brief Update the progress bar with the heading and line and check for cancellation
     \param progress CGUIDialogProgress bar
     \param heading string id of heading
     \param line1   string to set for the first line
     \return true if the user has cancelled the scanner, false otherwise
     */
    bool ProgressCancelled(CGUIDialogProgress* progress, int heading, const std::string &line1);

    /*! \brief Find a url for the given program using the given scraper
     \param programName name of the program to lookup
     \param scraper scraper to use for the lookup
     \param url [out] returned url from the scraper
     \param progress CGUIDialogProgress bar
     \return >0 on success, <0 on failure (cancellation), and 0 on no info found
     */
    int FindProgram(const std::string &programName, const ADDON::ScraperPtr &scraper, CScraperUrl &url, CGUIDialogProgress *progress);

    /*! \brief Retrieve detailed information for an item from an online source, optionally supplemented with local data
     @todo sort out some better return codes.
     \param pItem item to retrieve online details for.
     \param url URL to use to retrieve online details.
     \param scraper Scraper that handles parsing the online data.
     \param nfoFile if set, we override the online data with the locally supplied data. Defaults to NULL.
     \param pDialog progress dialog to update and check for cancellation during processing. Defaults to NULL.
     \return true if information is found, false if an error occurred, the lookup was cancelled, or no information was found.
     */
    bool GetDetails(CFileItem *pItem, CScraperUrl &url, const ADDON::ScraperPtr &scraper, CNfoFile *nfoFile=NULL, CGUIDialogProgress* pDialog=NULL);

    static int GetPathHash(const CFileItemList &items, std::string &hash);

    /*! \brief Retrieve a "fast" hash of the given directory (if available)
     Performs a stat() on the directory, and uses modified time to create a "fast"
     hash of the folder. If no modified time is available, the create time is used,
     and if neither are available, an empty hash is returned.
     In case exclude from scan expressions are present, the string array will be appended
     to the md5 hash to ensure we're doing a re-scan whenever the user modifies those.
     \param directory folder to hash
     \param excludes string array of exclude expressions
     \return the md5 hash of the folder"
     */
    std::string GetFastHash(const std::string &directory, const std::vector<std::string> &excludes) const;

    /*! \brief Decide whether a folder listing could use the "fast" hash
     Fast hashing can be done whenever the folder contains no scannable subfolders, as the
     fast hash technique uses modified time to determine when folder content changes, which
     is generally not propogated up the directory tree.
     \param items the directory listing
     \param excludes string array of exclude expressions
     \return true if this directory listing can be fast hashed, false otherwise
     */
    bool CanFastHash(const CFileItemList &items, const std::vector<std::string> &excludes) const;

    std::string GetnfoFile(CFileItem *item, bool bGrabAny=false) const;

    bool m_bStop;
    bool m_scanAll;
    std::string m_strStartDir;
    CProgramDatabase m_database;
    std::set<int> m_pathsToClean;
    CNfoFile m_nfoReader;
  };
}
