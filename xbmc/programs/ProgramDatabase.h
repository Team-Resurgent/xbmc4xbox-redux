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

typedef std::vector<CProgramInfoTag> VECGAMES;

namespace PROGRAM
{
  struct SScanSettings;
}

typedef enum
{
  PROGRAMDB_CONTENT_GAMES = 1
} PROGRAMDB_CONTENT_TYPE;

class CProgramDatabase : public CDatabase
{
public:
  CProgramDatabase(void);
  virtual ~CProgramDatabase(void);

  virtual bool Open();
  virtual bool CommitTransaction();

  int GetPathId(const std::string& strPath);

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

  bool HasContent();
  bool HasContent(PROGRAMDB_CONTENT_TYPE type);

  /*! \brief Add a path to the database, if necessary
   If the path is already in the database, we simply return its id.
   \param strPath the path to add
   \param parentPath the parent path of the path to add. If empty, URIUtils::GetParentPath() will determine the path.
   \param dateAdded datetime when the path was added to the filesystem/database
   \return id of the file, -1 if it could not be added.
   */
  int AddPath(const std::string& strPath, const std::string &parentPath = "", const CDateTime& dateAdded = CDateTime());

private:
  virtual void CreateTables();
  virtual void CreateAnalytics();

  virtual int GetSchemaVersion() const;
  const char *GetBaseDBName() const { return "MyPrograms"; };

  void ConstructPath(std::string& strDest, const std::string& strPath, const std::string& strFileName);
};
