#pragma once
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

#include <vector>

#include "video/VideoDatabase.h" // SDbTableOffsets, my_offsetof
#include "addons/Scraper.h"
#include "dbwrappers/Database.h"
#include "ProgramInfoTag.h"

class CGUIDialogProgress;
class CGUIDialogProgressBarHandle;

namespace dbiplus
{
  class field_value;
  typedef std::vector<field_value> sql_record;
}
struct SDbTableOffsets;

typedef std::vector<CProgramInfoTag> VECGAMES;

namespace PROGRAM
{
  struct SScanSettings;
}

enum ProgramDbDetails
{
  ProgramDbDetailsNone     = 0x00,
  ProgramDbDetailsRating   = 0x01,
  ProgramDbDetailsAll      = 0xFF
};

// these defines are based on how many columns we have and which column certain data is going to be in
// when we do GetDetailsForGame()
#define PROGRAMDB_MAX_COLUMNS 24
#define PROGRAMDB_DETAILS_FILEID      1

#define PROGRAMDB_DETAILS_GAME_RELEASEDATE         PROGRAMDB_MAX_COLUMNS + 2
#define PROGRAMDB_DETAILS_GAME_FILE              PROGRAMDB_MAX_COLUMNS + 3
#define PROGRAMDB_DETAILS_GAME_PATH              PROGRAMDB_MAX_COLUMNS + 4
#define PROGRAMDB_DETAILS_GAME_PLAYCOUNT         PROGRAMDB_MAX_COLUMNS + 5
#define PROGRAMDB_DETAILS_GAME_LASTPLAYED        PROGRAMDB_MAX_COLUMNS + 6
#define PROGRAMDB_DETAILS_GAME_DATEADDED         PROGRAMDB_MAX_COLUMNS + 7
#define PROGRAMDB_DETAILS_GAME_RATING            PROGRAMDB_MAX_COLUMNS + 8
#define PROGRAMDB_DETAILS_GAME_VOTES             PROGRAMDB_MAX_COLUMNS + 9
#define PROGRAMDB_DETAILS_GAME_RATING_TYPE       PROGRAMDB_MAX_COLUMNS + 10

#define PROGRAMDB_TYPE_UNUSED 0
#define PROGRAMDB_TYPE_STRING 1
#define PROGRAMDB_TYPE_INT 2
#define PROGRAMDB_TYPE_FLOAT 3
#define PROGRAMDB_TYPE_BOOL 4
#define PROGRAMDB_TYPE_COUNT 5
#define PROGRAMDB_TYPE_STRINGARRAY 6
#define PROGRAMDB_TYPE_DATE 7
#define PROGRAMDB_TYPE_DATETIME 8

typedef enum
{
  PROGRAMDB_CONTENT_GAMES = 1
} PROGRAMDB_CONTENT_TYPE;

typedef enum // this enum MUST match the offset struct further down!! and make sure to keep min and max at -1 and sizeof(offsets)
{
  PROGRAMDB_ID_MIN = -1,
  PROGRAMDB_ID_TITLE = 0,
  PROGRAMDB_ID_PLOT = 1,
  PROGRAMDB_ID_RATING_ID = 5,
  PROGRAMDB_ID_EXCLUSIVE = 6,
  PROGRAMDB_ID_ESRB = 7,
  PROGRAMDB_ID_THUMBURL = 8,
  PROGRAMDB_ID_IDENT_ID = 9, // unused for now
  PROGRAMDB_ID_DEVELOPER = 10,
  PROGRAMDB_ID_PUBLISHER = 11,
  PROGRAMDB_ID_GENRE = 12,
  PROGRAMDB_ID_DESCRIPTOR = 13,
  PROGRAMDB_ID_GENERALFEATURE = 14,
  PROGRAMDB_ID_ONLINEFEATURE = 15,
  PROGRAMDB_ID_PLATFORM = 16,
  PROGRAMDB_ID_ORIGINALTITLE = 17,
  PROGRAMDB_ID_THUMBURL_SPOOF = 18,
  PROGRAMDB_ID_TRAILER = 19,
  PROGRAMDB_ID_FANART = 20,
  PROGRAMDB_ID_SYSTEN = 21,
  PROGRAMDB_ID_BASEPATH = 22,
  PROGRAMDB_ID_PARENTPATHID = 23,
  PROGRAMDB_ID_MAX
} PROGRAMDB_IDS;

const struct SDbTableOffsets DbGameOffsets[] =
{
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strTitle) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strPlot) },
  { PROGRAMDB_TYPE_UNUSED, 0 }, // unused
  { PROGRAMDB_TYPE_UNUSED, 0 }, // unused
  { PROGRAMDB_TYPE_UNUSED, 0 }, // unused
  { PROGRAMDB_TYPE_INT, my_offsetof(CProgramInfoTag,m_iIdRating) },
  { PROGRAMDB_TYPE_BOOL, my_offsetof(CProgramInfoTag,m_bExclusive) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strESRB) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strPictureURL.m_xml) },
  { PROGRAMDB_TYPE_UNUSED, 0 }, // unused
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_developer) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_publisher) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_genre) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_descriptor) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_generalFeature) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_onlineFeature) },
  { PROGRAMDB_TYPE_STRINGARRAY, my_offsetof(CProgramInfoTag,m_platform) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strOriginalTitle) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strPictureURL.m_spoof) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strTrailer) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_fanart.m_xml) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_strSystem) },
  { PROGRAMDB_TYPE_STRING, my_offsetof(CProgramInfoTag,m_basePath) },
  { PROGRAMDB_TYPE_INT, my_offsetof(CProgramInfoTag,m_parentPathID) }
};

class CProgramDatabase : public CDatabase
{
public:
  CProgramDatabase(void);
  virtual ~CProgramDatabase(void);

  virtual bool Open();
  virtual bool CommitTransaction();

  int AddGame(const std::string& strFilenameAndPath);

  bool HasGameInfo(const std::string& strFilenameAndPath);

  int GetPathId(const std::string& strPath);

  int SetDetailsForGame(const std::string& strFilenameAndPath, CProgramInfoTag& details, const std::map<std::string, std::string> &artwork, int idGame = -1);

  void DeleteGame(int idMovie, bool bKeepId = false);
  void DeleteGame(const std::string& strFilenameAndPath, bool bKeepId = false);
  void RemoveContentForPath(const std::string& strPath,CGUIDialogProgress *progress = NULL);

  // scraper settings
  void SetScraperForPath(const std::string& filePath, const ADDON::ScraperPtr& info, const PROGRAM::SScanSettings& settings);
  ADDON::ScraperPtr GetScraperForPath(const std::string& strPath);
  ADDON::ScraperPtr GetScraperForPath(const std::string& strPath, PROGRAM::SScanSettings& settings);

  /*! \brief Retrieve the scraper and settings we should use for the specified path
   If the scraper is not set on this particular path, we'll recursively check parent folders.
   \param strPath path to start searching in.
   \param settings [out] scan settings for this folder.
   \param foundDirectly [out] true if a scraper was found directly for strPath, false if it was in a parent path.
   \return A ScraperPtr containing the scraper information. Returns NULL if a trivial (Content == CONTENT_NONE)
           scraper or no scraper is found.
   */
  ADDON::ScraperPtr GetScraperForPath(const std::string& strPath, PROGRAM::SScanSettings& settings, bool& foundDirectly);

  // scanning hashes and paths scanned
  bool SetPathHash(const std::string &path, const std::string &hash);
  bool GetPathHash(const std::string &path, std::string &hash);
  bool GetPaths(std::set<std::string> &paths);

  /*! \brief retrieve subpaths of a given path.  Assumes a heirarchical folder structure
   \param basepath the root path to retrieve subpaths for
   \param subpaths the returned subpaths
   \return true if we successfully retrieve subpaths (may be zero), false on error
   */
  bool GetSubPaths(const std::string& basepath, std::vector< std::pair<int, std::string> >& subpaths);

  // general browsing
  bool GetGamesNav(const std::string& strBaseDir, CFileItemList& items, const SortDescription &sortDescription = SortDescription(), int getDetails = ProgramDbDetailsNone);

  bool HasContent();
  bool HasContent(PROGRAMDB_CONTENT_TYPE type);

  /*! \brief Add a file to the database, if necessary
   If the file is already in the database, we simply return its id.
   \param url - full path of the file to add.
   \return id of the file, -1 if it could not be added.
   */
  int AddFile(const std::string& url);

  /*! \brief Add a path to the database, if necessary
   If the path is already in the database, we simply return its id.
   \param strPath the path to add
   \param parentPath the parent path of the path to add. If empty, URIUtils::GetParentPath() will determine the path.
   \param dateAdded datetime when the path was added to the filesystem/database
   \return id of the file, -1 if it could not be added.
   */
  int AddPath(const std::string& strPath, const std::string &parentPath = "", const CDateTime& dateAdded = CDateTime());

  /*! \brief Updates the dateAdded field in the files table for the file
   with the given idFile and the given path based on the files modification date
   \param idFile id of the file in the files table
   \param strFileNameAndPath path to the file
   \param dateAdded datetime when the file was added to the filesystem/database
   */
  void UpdateFileDateAdded(int idFile, const std::string& strFileNameAndPathh, const CDateTime& dateAdded = CDateTime());

  // smart playlists and main retrieval work in these functions
  bool GetGamesByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription = SortDescription(), int getDetails = ProgramDbDetailsNone);

  // retrieve a list of items
  bool GetItems(const std::string &strBaseDir, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
  bool GetItems(const std::string &strBaseDir, const std::string &mediaType, const std::string &itemType, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
  bool GetItems(const std::string &strBaseDir, PROGRAMDB_CONTENT_TYPE mediaType, const std::string &itemType, CFileItemList &items, const Filter &filter = Filter(), const SortDescription &sortDescription = SortDescription());
  std::string GetItemById(const std::string &itemType, int id);

  void SetArtForItem(int mediaId, const MediaType &mediaType, const std::string &artType, const std::string &url);
  void SetArtForItem(int mediaId, const MediaType &mediaType, const std::map<std::string, std::string> &art);

protected:
  int GetGameId(const std::string& strFilenameAndPath);

  /*! \brief Get the id of this fileitem
   Works for both programdb:// items and normal fileitems
   \param item CFileItem to grab the fileid of
   \return id of the file, -1 if it is not in the db.
   */
  int GetFileId(const CFileItem &item);

  /*! \brief Get the id of a file from path
   \param url full path to the file
   \return id of the file, -1 if it is not in the db.
   */
  int GetFileId(const std::string& url);

  int AddToTable(const std::string& table, const std::string& firstField, const std::string& secondField, const std::string& value);
  int AddRatings(int mediaId, const char *mediaType, const RatingMap& values, const std::string& defaultRating);

  // link functions - these two do all the work
  void AddToLinkTable(int mediaId, const std::string& mediaType, const std::string& table, int valueId, const char *foreignKey = NULL);

  void AddLinksToItem(int mediaId, const std::string& mediaType, const std::string& field, const std::vector<std::string>& values);

  CProgramInfoTag GetDetailsForGame(boost::movelib::unique_ptr<dbiplus::Dataset> &pDS, int getDetails = ProgramDbDetailsNone);
  CProgramInfoTag GetDetailsForGame(const dbiplus::sql_record* const record, int getDetails = ProgramDbDetailsNone);
  void GetRatings(int media_id, const std::string &media_type, RatingMap &ratings);

  void GetDetailsFromDB(boost::movelib::unique_ptr<dbiplus::Dataset> &pDS, int min, int max, const SDbTableOffsets *offsets, CProgramInfoTag &details, int idxOffset = 2);
  void GetDetailsFromDB(const dbiplus::sql_record* const record, int min, int max, const SDbTableOffsets *offsets, CProgramInfoTag &details, int idxOffset = 2);
  std::string GetValueString(const CProgramInfoTag &details, int min, int max, const SDbTableOffsets *offsets) const;

private:
  virtual void CreateTables();
  virtual void CreateAnalytics();
  void CreateLinkIndex(const char *table);
  void CreateForeignLinkIndex(const char *table, const char *foreignkey);

  /*! \brief (Re)Create the generic database views for movies, tvshows,
     episodes and music videos
   */
  virtual void CreateViews();

  /*! \brief Helper to get a database id given a query.
   Returns an integer, -1 if not found, and greater than 0 if found.
   \param query the SQL that will retrieve a database id.
   \return -1 if not found, else a valid database id (i.e. > 0)
   */
  int GetDbId(const std::string &query);

  /*! \brief Run a query on the main dataset and return the number of rows
   If no rows are found we close the dataset and return 0.
   \param sql the sql query to run
   \return the number of rows, -1 for an error.
   */
  int RunQuery(const std::string &sql);

  virtual int GetSchemaVersion() const;
  const char *GetBaseDBName() const { return "MyPrograms"; };

  void ConstructPath(std::string& strDest, const std::string& strPath, const std::string& strFileName);
  void SplitPath(const std::string& strFileNameAndPath, std::string& strPath, std::string& strFileName);
  void InvalidatePathHash(const std::string& strPath);
};
