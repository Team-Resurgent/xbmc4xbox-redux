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

#include "ProgramDatabase.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "dbwrappers/dataset.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogProgress.h"
#include "filesystem/MultiPathDirectory.h"
#include "guiinfo/GUIInfoLabels.h"
#include "GUIInfoManager.h"
#include "settings/AdvancedSettings.h"
#include "URL.h"
#include "utils/FileUtils.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "ProgramInfoScanner.h"
#include "XBDateTime.h"

using namespace dbiplus;
using namespace XFILE;
using namespace PROGRAM;
using namespace ADDON;

CProgramDatabase::CProgramDatabase(void)
{
}

CProgramDatabase::~CProgramDatabase(void)
{}

bool CProgramDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseProgram);
}

void CProgramDatabase::CreateTables()
{
  CLog::Log(LOGINFO, "create developer table and link");
  m_pDS->exec("CREATE TABLE developer (developer_id integer primary key, name TEXT)");
  m_pDS->exec("CREATE TABLE developer_link (developer_id integer, media_id integer, media_type TEXT)");

  CLog::Log(LOGINFO, "create publisher table and link");
  m_pDS->exec("CREATE TABLE publisher (publisher_id integer primary key, name TEXT)");
  m_pDS->exec("CREATE TABLE publisher_link (publisher_id integer, media_id integer, media_type TEXT)");

  CLog::Log(LOGINFO, "create genre table and link");
  m_pDS->exec("CREATE TABLE genre (genre_id integer primary key, name TEXT)");
  m_pDS->exec("CREATE TABLE genre_link (genre_id integer, media_id integer, media_type TEXT)");

  CLog::Log(LOGINFO, "create descriptor table and link");
  m_pDS->exec("CREATE TABLE descriptor (descriptor_id integer primary key, name TEXT)");
  m_pDS->exec("CREATE TABLE descriptor_link (descriptor_id integer, media_id integer, media_type TEXT)");

  CLog::Log(LOGINFO, "create generalfeature table and link");
  m_pDS->exec("CREATE TABLE generalfeature (generalfeature_id integer primary key, name TEXT)");
  m_pDS->exec("CREATE TABLE generalfeature_link (generalfeature_id integer, media_id integer, media_type TEXT)");

  CLog::Log(LOGINFO, "create onlinefeature table and link");
  m_pDS->exec("CREATE TABLE onlinefeature (onlinefeature_id integer primary key, name TEXT)");
  m_pDS->exec("CREATE TABLE onlinefeature_link (onlinefeature_id integer, media_id integer, media_type TEXT)");

  CLog::Log(LOGINFO, "create platform table and link");
  m_pDS->exec("CREATE TABLE platform (platform_id integer primary key, name TEXT)");
  m_pDS->exec("CREATE TABLE platform_link (platform_id integer, media_id integer, media_type TEXT)");

  CLog::Log(LOGINFO, "create game table");
  std::string columns = "CREATE TABLE game ( idGame integer primary key, idFile integer";

  for (int i = 0; i < PROGRAMDB_MAX_COLUMNS; i++)
    columns += StringUtils::Format(",c%02d text", i);

  columns += ", released text)";
  m_pDS->exec(columns);

  CLog::Log(LOGINFO, "create path table");
  m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, strContent text, strScraper text, strHash text, scanRecursive integer, useFolderNames bool, strSettings text, noUpdate bool, exclude bool, dateAdded text, idParentPath integer)");

  CLog::Log(LOGINFO, "create files table");
  m_pDS->exec("CREATE TABLE files ( idFile integer primary key, idPath integer, strFilename text, titleId integer, playCount integer, lastPlayed text, dateAdded text)");

  CLog::Log(LOGINFO, "create art table");
  m_pDS->exec("CREATE TABLE art(art_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, type TEXT, url TEXT)");

  CLog::Log(LOGINFO, "create rating table");
  m_pDS->exec("CREATE TABLE rating (rating_id INTEGER PRIMARY KEY, media_id INTEGER, media_type TEXT, rating_type TEXT, rating FLOAT, votes INTEGER)");
}

void CProgramDatabase::CreateLinkIndex(const char *table)
{
  m_pDS->exec(PrepareSQL("CREATE UNIQUE INDEX ix_%s_1 ON %s (name(255))", table, table));
  m_pDS->exec(PrepareSQL("CREATE UNIQUE INDEX ix_%s_link_1 ON %s_link (%s_id, media_type(20), media_id)", table, table, table));
  m_pDS->exec(PrepareSQL("CREATE UNIQUE INDEX ix_%s_link_2 ON %s_link (media_id, media_type(20), %s_id)", table, table, table));
  m_pDS->exec(PrepareSQL("CREATE INDEX ix_%s_link_3 ON %s_link (media_type(20))", table, table));
}

void CProgramDatabase::CreateForeignLinkIndex(const char *table, const char *foreignkey)
{
  m_pDS->exec(PrepareSQL("CREATE UNIQUE INDEX ix_%s_link_1 ON %s_link (%s_id, media_type(20), media_id)", table, table, foreignkey));
  m_pDS->exec(PrepareSQL("CREATE UNIQUE INDEX ix_%s_link_2 ON %s_link (media_id, media_type(20), %s_id)", table, table, foreignkey));
  m_pDS->exec(PrepareSQL("CREATE INDEX ix_%s_link_3 ON %s_link (media_type(20))", table, table));
}

void CProgramDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s - creating indicies", __FUNCTION__);
  m_pDS->exec("CREATE INDEX ix_path ON path ( strPath(255) )");
  m_pDS->exec("CREATE INDEX ix_path2 ON path ( idParentPath )");
  m_pDS->exec("CREATE INDEX ix_files ON files ( idPath, strFilename(255) )");

  m_pDS->exec("CREATE UNIQUE INDEX ix_game_file_1 ON game (idFile, idGame)");
  m_pDS->exec("CREATE UNIQUE INDEX ix_game_file_2 ON game (idGame, idFile)");

  m_pDS->exec("CREATE INDEX ixGameBasePath ON game ( c23(12) )");

  m_pDS->exec("CREATE INDEX ix_art ON art(media_id, media_type(20), type(20))");

  m_pDS->exec("CREATE INDEX ix_rating ON rating(media_id, media_type(20))");

  CreateLinkIndex("developer");
  CreateLinkIndex("publisher");
  CreateLinkIndex("genre");
  CreateLinkIndex("descriptor");
  CreateLinkIndex("generalfeature");
  CreateLinkIndex("onlinefeature");
  CreateLinkIndex("platform");

  CLog::Log(LOGINFO, "%s - creating triggers", __FUNCTION__);
  m_pDS->exec("CREATE TRIGGER delete_game AFTER DELETE ON game FOR EACH ROW BEGIN "
              "DELETE FROM developer_link WHERE media_id=old.idGame AND media_type='game'; "
              "DELETE FROM publisher_link WHERE media_id=old.idGame AND media_type='game'; "
              "DELETE FROM genre_link WHERE media_id=old.idGame AND media_type='game'; "
              "DELETE FROM descriptor_link WHERE media_id=old.idGame AND media_type='game'; "
              "DELETE FROM generalfeature_link WHERE media_id=old.idGame AND media_type='game'; "
              "DELETE FROM onlinefeature_link WHERE media_id=old.idGame AND media_type='game'; "
              "DELETE FROM platform_link WHERE media_id=old.idGame AND media_type='game'; "
              "DELETE FROM art WHERE media_id=old.idGame AND media_type='game'; "
              "DELETE FROM rating WHERE media_id=old.idGame AND media_type='game'; "
              "END");

  CreateViews();
}

void CProgramDatabase::CreateViews()
{
  CLog::Log(LOGINFO, "create game_view");
  std::string gameview = PrepareSQL("CREATE VIEW game_view AS SELECT"
                                      "  game.*,"
                                      "  files.strFileName AS strFileName,"
                                      "  path.strPath AS strPath,"
                                      "  files.playCount AS playCount,"
                                      "  files.lastPlayed AS lastPlayed, "
                                      "  files.dateAdded AS dateAdded, "
                                      "  rating.rating AS rating, "
                                      "  rating.votes AS votes, "
                                      "  rating.rating_type AS rating_type "
                                      "FROM game"
                                      "  JOIN files ON"
                                      "    files.idFile=game.idFile"
                                      "  JOIN path ON"
                                      "    path.idPath=files.idPath"
                                      "  LEFT JOIN rating ON"
                                      "    rating.rating_id=game.c%02d",
                                      PROGRAMDB_ID_RATING_ID);
  m_pDS->exec(gameview);
}

//********************************************************************************************************************************
int CProgramDatabase::GetPathId(const std::string& strPath)
{
  std::string strSQL;
  try
  {
    int idPath=-1;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string strPath1(strPath);
    URIUtils::AddSlashAtEnd(strPath1);

    strSQL=PrepareSQL("select idPath from path where strPath='%s'",strPath1.c_str());
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
      idPath = m_pDS->fv("path.idPath").get_asInt();

    m_pDS->close();
    return idPath;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to getpath (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

bool CProgramDatabase::GetPaths(std::set<std::string> &paths)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    paths.clear();

    // grab all paths with game content set
    if (!m_pDS->query("select strPath,noUpdate from path"
                      " where (strContent = 'games')"
                      " and strPath NOT like 'multipath://%%'"
                      " order by strPath"))
      return false;

    while (!m_pDS->eof())
    {
      if (!m_pDS->fv("noUpdate").get_asBool())
        paths.insert(m_pDS->fv("strPath").get_asString());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CProgramDatabase::GetSubPaths(const std::string &basepath, std::vector<std::pair<int, std::string> >& subpaths)
{
  std::string sql;
  try
  {
    if (!m_pDB.get() || !m_pDS.get())
      return false;

    std::string path(basepath);
    URIUtils::AddSlashAtEnd(path);
    sql = PrepareSQL("SELECT idPath,strPath FROM path WHERE SUBSTR(strPath,1,%i)='%s'"
                     , StringUtils::utf8_strlen(path.c_str()), path.c_str());

    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      subpaths.push_back(std::make_pair(m_pDS->fv(0).get_asInt(), m_pDS->fv(1).get_asString()));
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s error during query: %s",__FUNCTION__, sql.c_str());
  }
  return false;
}

int CProgramDatabase::AddPath(const std::string& strPath, const std::string &parentPath /*= "" */, const CDateTime& dateAdded /* = CDateTime() */)
{
  std::string strSQL;
  try
  {
    int idPath = GetPathId(strPath);
    if (idPath >= 0)
      return idPath; // already have the path

    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string strPath1(strPath);
    URIUtils::AddSlashAtEnd(strPath1);

    int idParentPath = GetPathId(parentPath.empty() ? (std::string)URIUtils::GetParentPath(strPath1) : parentPath);

    // add the path
    if (idParentPath < 0)
    {
      if (dateAdded.IsValid())
        strSQL=PrepareSQL("insert into path (idPath, strPath, dateAdded) values (NULL, '%s', '%s')", strPath1.c_str(), dateAdded.GetAsDBDateTime().c_str());
      else
        strSQL=PrepareSQL("insert into path (idPath, strPath) values (NULL, '%s')", strPath1.c_str());
    }
    else
    {
      if (dateAdded.IsValid())
        strSQL = PrepareSQL("insert into path (idPath, strPath, dateAdded, idParentPath) values (NULL, '%s', '%s', %i)", strPath1.c_str(), dateAdded.GetAsDBDateTime().c_str(), idParentPath);
      else
        strSQL=PrepareSQL("insert into path (idPath, strPath, idParentPath) values (NULL, '%s', %i)", strPath1.c_str(), idParentPath);
    }
    m_pDS->exec(strSQL);
    idPath = (int)m_pDS->lastinsertid();
    return idPath;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to addpath (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

bool CProgramDatabase::GetPathHash(const std::string &path, std::string &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL=PrepareSQL("select strHash from path where strPath='%s'", path.c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() == 0)
      return false;
    hash = m_pDS->fv("strHash").get_asString();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, path.c_str());
  }

  return false;
}

//********************************************************************************************************************************
int CProgramDatabase::AddFile(const std::string& strFileNameAndPath)
{
  std::string strSQL = "";
  try
  {
    int idFile;
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string strFileName, strPath;
    SplitPath(strFileNameAndPath,strPath,strFileName);

    int idPath = AddPath(strPath);
    if (idPath < 0)
      return -1;

    std::string strSQL=PrepareSQL("select idFile from files where strFileName='%s' and idPath=%i", strFileName.c_str(),idPath);

    m_pDS->query(strSQL);
    if (m_pDS->num_rows() > 0)
    {
      idFile = m_pDS->fv("idFile").get_asInt() ;
      m_pDS->close();
      return idFile;
    }
    m_pDS->close();

    strSQL=PrepareSQL("insert into files (idFile, idPath, strFileName) values(NULL, %i, '%s')", idPath, strFileName.c_str());
    m_pDS->exec(strSQL);
    idFile = (int)m_pDS->lastinsertid();
    return idFile;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to addfile (%s)", __FUNCTION__, strSQL.c_str());
  }
  return -1;
}

void CProgramDatabase::UpdateFileDateAdded(int idFile, const std::string& strFileNameAndPath, const CDateTime& dateAdded /* = CDateTime() */)
{
  if (idFile < 0 || strFileNameAndPath.empty())
    return;

  CDateTime finalDateAdded = dateAdded;
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (!finalDateAdded.IsValid())
    {
      // 1 prefering to use the files mtime(if it's valid) and only using the file's ctime if the mtime isn't valid
      if (g_advancedSettings.m_iProgramLibraryDateAdded == 1)
        finalDateAdded = CFileUtils::GetModificationDate(strFileNameAndPath, false);
      //2 using the newer datetime of the file's mtime and ctime
      else if (g_advancedSettings.m_iProgramLibraryDateAdded == 2)
        finalDateAdded = CFileUtils::GetModificationDate(strFileNameAndPath, true);
      //0 using the current datetime if non of the above matches or one returns an invalid datetime
      if (!finalDateAdded.IsValid())
        finalDateAdded = CDateTime::GetCurrentDateTime();
    }

    m_pDS->exec(PrepareSQL("UPDATE files SET dateAdded='%s' WHERE idFile=%d", finalDateAdded.GetAsDBDateTime().c_str(), idFile));
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) failed", __FUNCTION__, CURL::GetRedacted(strFileNameAndPath).c_str(), finalDateAdded.GetAsDBDateTime().c_str());
  }
}

bool CProgramDatabase::SetPathHash(const std::string &path, const std::string &hash)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int idPath = AddPath(path);
    if (idPath < 0) return false;

    std::string strSQL=PrepareSQL("update path set strHash='%s' where idPath=%ld", hash.c_str(), idPath);
    m_pDS->exec(strSQL);

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s, %s) failed", __FUNCTION__, path.c_str(), hash.c_str());
  }

  return false;
}

//********************************************************************************************************************************
int CProgramDatabase::GetFileId(const std::string& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    std::string strPath, strFileName;
    SplitPath(strFilenameAndPath,strPath,strFileName);

    int idPath = GetPathId(strPath);
    if (idPath >= 0)
    {
      std::string strSQL;
      strSQL=PrepareSQL("select idFile from files where strFileName='%s' and idPath=%i", strFileName.c_str(),idPath);
      m_pDS->query(strSQL);
      if (m_pDS->num_rows() > 0)
      {
        int idFile = m_pDS->fv("files.idFile").get_asInt();
        m_pDS->close();
        return idFile;
      }
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

int CProgramDatabase::GetFileId(const CFileItem &item)
{
  if (item.IsProgramDb() && item.HasProgramInfoTag())
  {
    if (item.GetProgramInfoTag()->m_iFileId != -1)
      return item.GetProgramInfoTag()->m_iFileId;
    else
      return GetFileId(item.GetProgramInfoTag()->m_strFileNameAndPath);
  }
  return GetFileId(item.GetPath());
}

//********************************************************************************************************************************
int CProgramDatabase::GetGameId(const std::string& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    int idGame = -1;

    // needed for query parameters
    int idFile = GetFileId(strFilenameAndPath);
    int idPath=-1;
    std::string strPath;
    if (idFile < 0)
    {
      std::string strFile;
      SplitPath(strFilenameAndPath,strPath,strFile);

      // have to join gameinfo table for correct results
      idPath = GetPathId(strPath);
      if (idPath < 0 && strPath != strFilenameAndPath)
        return -1;
    }

    if (idFile == -1 && strPath != strFilenameAndPath)
      return -1;

    std::string strSQL;
    if (idFile == -1)
      strSQL=PrepareSQL("select idGame from game join files on files.idFile=game.idFile where files.idPath=%i",idPath);
    else
      strSQL=PrepareSQL("select idGame from game where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, "%s (%s), query = %s", __FUNCTION__, CURL::GetRedacted(strFilenameAndPath).c_str(), strSQL.c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() > 0)
      idGame = m_pDS->fv("idGame").get_asInt();
    m_pDS->close();

    return idGame;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
int CProgramDatabase::AddGame(const std::string& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idGame = GetGameId(strFilenameAndPath);
    if (idGame < 0)
    {
      int idFile = AddFile(strFilenameAndPath);
      if (idFile < 0)
        return -1;
      UpdateFileDateAdded(idFile, strFilenameAndPath);
      std::string strSQL=PrepareSQL("insert into game (idGame, idFile) values (NULL, %i)", idFile);
      m_pDS->exec(strSQL);
      idGame = (int)m_pDS->lastinsertid();
    }

    return idGame;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
int CProgramDatabase::AddToTable(const std::string& table, const std::string& firstField, const std::string& secondField, const std::string& value)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string strSQL = PrepareSQL("select %s from %s where %s like '%s'", firstField.c_str(), table.c_str(), secondField.c_str(), value.substr(0, 255).c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() == 0)
    {
      m_pDS->close();
      // doesnt exists, add it
      strSQL = PrepareSQL("insert into %s (%s, %s) values(NULL, '%s')", table.c_str(), firstField.c_str(), secondField.c_str(), value.substr(0, 255).c_str());
      m_pDS->exec(strSQL);
      int id = (int)m_pDS->lastinsertid();
      return id;
    }
    else
    {
      int id = m_pDS->fv(firstField.c_str()).get_asInt();
      m_pDS->close();
      return id;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, value.c_str() );
  }

  return -1;
}

int CProgramDatabase::AddRatings(int mediaId, const char *mediaType, const RatingMap& values, const std::string& defaultRating)
{
  int ratingid = -1;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    for (RatingMap::const_iterator it = values.begin(); it != values.end(); ++it)
    {
      const std::pair<const std::string, CRating> &i = *it;
      int id;
      std::string strSQL = PrepareSQL("SELECT rating_id FROM rating WHERE media_id=%i AND media_type='%s' AND rating_type = '%s'", mediaId, mediaType, i.first.c_str());
      m_pDS->query(strSQL);
      if (m_pDS->num_rows() == 0)
      {
        m_pDS->close();
        // doesnt exists, add it
        strSQL = PrepareSQL("INSERT INTO rating (media_id, media_type, rating_type, rating, votes) VALUES (%i, '%s', '%s', %f, %i)", mediaId, mediaType, i.first.c_str(), i.second.rating, i.second.votes);
        m_pDS->exec(strSQL);
        id = (int)m_pDS->lastinsertid();
      }
      else
      {
        id = m_pDS->fv(0).get_asInt();
        m_pDS->close();
        strSQL = PrepareSQL("UPDATE rating SET rating = %f, votes = %i WHERE rating_id = %i", i.second.rating, i.second.votes, id);
        m_pDS->exec(strSQL);
      }
      if (i.first == defaultRating)
        ratingid = id;
    }
    return ratingid;

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i - %s) failed", __FUNCTION__, mediaId, mediaType);
  }

  return ratingid;
}

void CProgramDatabase::AddToLinkTable(int mediaId, const std::string& mediaType, const std::string& table, int valueId, const char *foreignKey)
{
  const char *key = foreignKey ? foreignKey : table.c_str();
  std::string sql = PrepareSQL("SELECT 1 FROM %s_link WHERE %s_id=%i AND media_id=%i AND media_type='%s'", table.c_str(), key, valueId, mediaId, mediaType.c_str());

  if (GetSingleValue(sql).empty())
  { // doesnt exists, add it
    sql = PrepareSQL("INSERT INTO %s_link (%s_id,media_id,media_type) VALUES(%i,%i,'%s')", table.c_str(), key, valueId, mediaId, mediaType.c_str());
    ExecuteQuery(sql);
  }
}

void CProgramDatabase::AddLinksToItem(int mediaId, const std::string& mediaType, const std::string& field, const std::vector<std::string>& values)
{
  for (std::vector<std::string>::const_iterator it = values.begin(); it != values.end(); ++it)
  {
    const std::string &i = *it;
    if (!i.empty())
    {
      int idValue = AddToTable(field, field + "_id", "name", i);
      if (idValue > -1)
        AddToLinkTable(mediaId, mediaType, field, idValue);
    }
  }
}

bool CProgramDatabase::HasGameInfo(const std::string& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    int idGame = GetGameId(strFilenameAndPath);
    return (idGame > 0); // index of zero is also invalid
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

std::string CProgramDatabase::GetValueString(const CProgramInfoTag &details, int min, int max, const SDbTableOffsets *offsets) const
{
  std::vector<std::string> conditions;
  for (int i = min + 1; i < max; ++i)
  {
    switch (offsets[i].type)
    {
    case PROGRAMDB_TYPE_STRING:
      conditions.push_back(PrepareSQL("c%02d='%s'", i, ((std::string*)(((char*)&details)+offsets[i].offset))->c_str()));
      break;
    case PROGRAMDB_TYPE_INT:
      conditions.push_back(PrepareSQL("c%02d='%i'", i, *(int*)(((char*)&details)+offsets[i].offset)));
      break;
    case PROGRAMDB_TYPE_COUNT:
      {
        int value = *(int*)(((char*)&details)+offsets[i].offset);
        if (value)
          conditions.push_back(PrepareSQL("c%02d=%i", i, value));
        else
          conditions.push_back(PrepareSQL("c%02d=NULL", i));
      }
      break;
    case PROGRAMDB_TYPE_BOOL:
      conditions.push_back(PrepareSQL("c%02d='%s'", i, *(bool*)(((char*)&details)+offsets[i].offset)?"true":"false"));
      break;
    case PROGRAMDB_TYPE_FLOAT:
      conditions.push_back(PrepareSQL("c%02d='%f'", i, *(float*)(((char*)&details)+offsets[i].offset)));
      break;
    case PROGRAMDB_TYPE_STRINGARRAY:
      conditions.push_back(PrepareSQL("c%02d='%s'", i, StringUtils::Join(*((std::vector<std::string>*)(((char*)&details)+offsets[i].offset)),
                                                                          g_advancedSettings.m_programItemSeparator).c_str()));
      break;
    case PROGRAMDB_TYPE_DATE:
      conditions.push_back(PrepareSQL("c%02d='%s'", i, ((CDateTime*)(((char*)&details)+offsets[i].offset))->GetAsDBDate().c_str()));
      break;
    case PROGRAMDB_TYPE_DATETIME:
      conditions.push_back(PrepareSQL("c%02d='%s'", i, ((CDateTime*)(((char*)&details)+offsets[i].offset))->GetAsDBDateTime().c_str()));
      break;
    case PROGRAMDB_TYPE_UNUSED: // Skip the unused field to avoid populating unused data
      continue;
    }
  }
  return StringUtils::Join(conditions, ",");
}

int CProgramDatabase::SetDetailsForGame(const std::string& strFilenameAndPath, CProgramInfoTag& details,
    const std::map<std::string, std::string> &artwork, int idGame /* = -1 */)
{
  try
  {
    BeginTransaction();

    if (idGame < 0)
      idGame = GetGameId(strFilenameAndPath);

    if (idGame > -1)
      DeleteGame(idGame, true); // true to keep the table entry
    else
    {
      // only add a new game if we don't already have a valid idGame
      // (DeleteGame is called with bKeepId == true so the game won't
      // be removed from the game table)
      idGame = AddGame(strFilenameAndPath);
      if (idGame < 0)
      {
        RollbackTransaction();
        return idGame;
      }
    }

    // update dateadded if it's set
    if (details.m_dateAdded.IsValid())
    {
      if (details.m_iFileId <= 0)
        details.m_iFileId = GetFileId(strFilenameAndPath);

      UpdateFileDateAdded(details.m_iFileId, strFilenameAndPath, details.m_dateAdded);
    }

    AddLinksToItem(idGame, MediaTypeGame, "developer", details.m_developer);
    AddLinksToItem(idGame, MediaTypeGame, "publisher", details.m_publisher);
    AddLinksToItem(idGame, MediaTypeGame, "genre", details.m_genre);
    AddLinksToItem(idGame, MediaTypeGame, "descriptor", details.m_descriptor);
    AddLinksToItem(idGame, MediaTypeGame, "generalfeature", details.m_generalFeature);
    AddLinksToItem(idGame, MediaTypeGame, "onlinefeature", details.m_onlineFeature);
    AddLinksToItem(idGame, MediaTypeGame, "platform", details.m_platform);

    // add ratings
    details.m_iIdRating = AddRatings(idGame, MediaTypeGame, details.m_ratings, details.GetDefaultRating());

    // TODO: add support for unique IDs

    SetArtForItem(idGame, MediaTypeGame, artwork);

    // update our game table (we know it was added already above)
    // and insert the new row
    std::string sql = "UPDATE game SET " + GetValueString(details, PROGRAMDB_ID_MIN, PROGRAMDB_ID_MAX, DbGameOffsets);
    if (details.HasReleaseDate())
      sql += PrepareSQL(", released = '%s'", details.GetReleaseDate().GetAsDBDate().c_str());
    else
      sql += PrepareSQL(", released = '%i'", details.GetYear());
    sql += PrepareSQL(" where idGame=%i", idGame);
    m_pDS->exec(sql);
    CommitTransaction();

    return idGame;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  RollbackTransaction();
  return -1;
}

//********************************************************************************************************************************
void CProgramDatabase::DeleteGame(const std::string& strFilenameAndPath, bool bKeepId /* = false */)
{
  int idGame = GetGameId(strFilenameAndPath);
  if (idGame > -1)
    DeleteGame(idGame, bKeepId);
}

void CProgramDatabase::DeleteGame(int idGame, bool bKeepId /* = false */)
{
  if (idGame < 0)
    return;

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    BeginTransaction();

    // keep the game table entry
    // so we can update the data in place
    // the ancilliary tables are still purged
    if (!bKeepId)
    {
      int idFile = GetDbId(PrepareSQL("SELECT idFile FROM game WHERE idGame=%i", idGame));
      std::string path = GetSingleValue(PrepareSQL("SELECT strPath FROM path JOIN files ON files.idPath=path.idPath WHERE files.idFile=%i", idFile));
      if (!path.empty())
        InvalidatePathHash(path);

      std::string strSQL = PrepareSQL("delete from game where idGame=%i", idGame);
      m_pDS->exec(strSQL);
    }

    // Do we need to announce here?

    CommitTransaction();

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    RollbackTransaction();
  }
}

int CProgramDatabase::GetDbId(const std::string &query)
{
  std::string result = GetSingleValue(query);
  if (!result.empty())
  {
    int idDb = strtol(result.c_str(), NULL, 10);
    if (idDb > 0)
      return idDb;
  }
  return -1;
}

void CProgramDatabase::GetDetailsFromDB(boost::movelib::unique_ptr<Dataset> &pDS, int min, int max, const SDbTableOffsets *offsets, CProgramInfoTag &details, int idxOffset)
{
  GetDetailsFromDB(pDS->get_sql_record(), min, max, offsets, details, idxOffset);
}

void CProgramDatabase::GetDetailsFromDB(const dbiplus::sql_record* const record, int min, int max, const SDbTableOffsets *offsets, CProgramInfoTag &details, int idxOffset)
{
  for (int i = min + 1; i < max; i++)
  {
    switch (offsets[i].type)
    {
    case PROGRAMDB_TYPE_STRING:
      *(std::string*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asString();
      break;
    case PROGRAMDB_TYPE_INT:
    case PROGRAMDB_TYPE_COUNT:
      *(int*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asInt();
      break;
    case PROGRAMDB_TYPE_BOOL:
      *(bool*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asBool();
      break;
    case PROGRAMDB_TYPE_FLOAT:
      *(float*)(((char*)&details)+offsets[i].offset) = record->at(i+idxOffset).get_asFloat();
      break;
    case PROGRAMDB_TYPE_STRINGARRAY:
    {
      std::string value = record->at(i+idxOffset).get_asString();
      if (!value.empty())
        *(std::vector<std::string>*)(((char*)&details)+offsets[i].offset) = StringUtils::Split(value, g_advancedSettings.m_programItemSeparator);
      break;
    }
    case PROGRAMDB_TYPE_DATE:
      ((CDateTime*)(((char*)&details)+offsets[i].offset))->SetFromDBDate(record->at(i+idxOffset).get_asString());
      break;
    case PROGRAMDB_TYPE_DATETIME:
      ((CDateTime*)(((char*)&details)+offsets[i].offset))->SetFromDBDateTime(record->at(i+idxOffset).get_asString());
      break;
    case PROGRAMDB_TYPE_UNUSED: // Skip the unused field to avoid populating unused data
      continue;
    }
  }
}

CProgramInfoTag CProgramDatabase::GetDetailsForGame(boost::movelib::unique_ptr<Dataset> &pDS, int getDetails /* = ProgramDbDetailsNone */)
{
  return GetDetailsForGame(pDS->get_sql_record(), getDetails);
}

DWORD gameTime = 0;

CProgramInfoTag CProgramDatabase::GetDetailsForGame(const dbiplus::sql_record* const record, int getDetails /* = ProgramDbDetailsNone */)
{
  CProgramInfoTag details;

  if (record == NULL)
    return details;

  DWORD time = XbmcThreads::SystemClockMillis();
  int idGame = record->at(0).get_asInt();

  GetDetailsFromDB(record, PROGRAMDB_ID_MIN, PROGRAMDB_ID_MAX, DbGameOffsets, details);

  details.m_iDbId = idGame;
  details.m_type = MediaTypeGame;

  details.m_iFileId = record->at(PROGRAMDB_DETAILS_FILEID).get_asInt();
  details.m_strPath = record->at(PROGRAMDB_DETAILS_GAME_PATH).get_asString();
  std::string strFileName = record->at(PROGRAMDB_DETAILS_GAME_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
  details.m_playCount = record->at(PROGRAMDB_DETAILS_GAME_PLAYCOUNT).get_asInt();
  details.m_lastPlayed.SetFromDBDateTime(record->at(PROGRAMDB_DETAILS_GAME_LASTPLAYED).get_asString());
  details.m_dateAdded.SetFromDBDateTime(record->at(PROGRAMDB_DETAILS_GAME_DATEADDED).get_asString());
  details.SetRating(record->at(PROGRAMDB_DETAILS_GAME_RATING).get_asFloat(),
                    record->at(PROGRAMDB_DETAILS_GAME_VOTES).get_asInt(),
                    record->at(PROGRAMDB_DETAILS_GAME_RATING_TYPE).get_asString(), true);
  std::string releaseDateString = record->at(PROGRAMDB_DETAILS_GAME_RELEASEDATE).get_asString();
  if (releaseDateString.size() == 4)
    details.SetYear(record->at(PROGRAMDB_DETAILS_GAME_RELEASEDATE).get_asInt());
  else
    details.SetReleaseDateFromDBDate(releaseDateString);
  gameTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    if (getDetails & ProgramDbDetailsRating)
      GetRatings(details.m_iDbId, MediaTypeGame, details.m_ratings);

    details.m_strPictureURL.Parse();
  }
  return details;
}

void CProgramDatabase::GetRatings(int media_id, const std::string &media_type, RatingMap &ratings)
{
  try
  {
    if (!m_pDB.get()) return;
    if (!m_pDS2.get()) return;

    std::string sql = PrepareSQL("SELECT rating.rating_type, rating.rating, rating.votes FROM rating WHERE rating.media_id = %i AND rating.media_type = '%s'", media_id, media_type.c_str());
    m_pDS2->query(sql);
    while (!m_pDS2->eof())
    {
      ratings[m_pDS2->fv(0).get_asString()] = CRating(m_pDS2->fv(1).get_asFloat(), m_pDS2->fv(2).get_asInt());
      m_pDS2->next();
    }
    m_pDS2->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i,%s) failed", __FUNCTION__, media_id, media_type.c_str());
  }
}

void CProgramDatabase::SetArtForItem(int mediaId, const MediaType &mediaType, const std::map<std::string, std::string> &art)
{
  for (std::map<std::string, std::string>::const_iterator i = art.begin(); i != art.end(); ++i)
    SetArtForItem(mediaId, mediaType, i->first, i->second);
}

void CProgramDatabase::SetArtForItem(int mediaId, const MediaType &mediaType, const std::string &artType, const std::string &url)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    // don't set <foo>.<bar> art types - these are derivative types from parent items
    if (artType.find('.') != std::string::npos)
      return;

    std::string sql = PrepareSQL("SELECT art_id,url FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
    m_pDS->query(sql);
    if (!m_pDS->eof())
    { // update
      int artId = m_pDS->fv(0).get_asInt();
      std::string oldUrl = m_pDS->fv(1).get_asString();
      m_pDS->close();
      if (oldUrl != url)
      {
        sql = PrepareSQL("UPDATE art SET url='%s' where art_id=%d", url.c_str(), artId);
        m_pDS->exec(sql);
      }
    }
    else
    { // insert
      m_pDS->close();
      sql = PrepareSQL("INSERT INTO art(media_id, media_type, type, url) VALUES (%d, '%s', '%s', '%s')", mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
      m_pDS->exec(sql);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d, '%s', '%s', '%s') failed", __FUNCTION__, mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
  }
}

void CProgramDatabase::RemoveContentForPath(const std::string& strPath, CGUIDialogProgress *progress /* = NULL */)
{
  if(URIUtils::IsMultiPath(strPath))
  {
    std::vector<std::string> paths;
    CMultiPathDirectory::GetPaths(strPath, paths);

    for(unsigned i=0;i<paths.size();i++)
      RemoveContentForPath(paths[i], progress);
  }

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    if (progress)
    {
      progress->SetHeading(700);
      progress->SetLine(0, "");
      progress->SetLine(1, 313);
      progress->SetLine(2, 330);
      progress->SetPercentage(0);
      progress->Open();
      progress->ShowProgressBar(true);
    }
    std::vector<std::pair<int, std::string> > paths;
    GetSubPaths(strPath, paths);
    int iCurr = 0;
    for (std::vector<std::pair<int, std::string> >::const_iterator it = paths.begin(); it != paths.end(); ++it)
    {
      const std::pair<int, std::string> &i = *it;
      if (progress)
      {
        progress->SetPercentage((int)((float)(iCurr++)/paths.size()*100.f));
        progress->Progress();
      }

      std::string strSQL = PrepareSQL("select files.strFilename from files join game on game.idFile=files.idFile where files.idPath=%i", i.first);
      m_pDS2->query(strSQL);
      while (!m_pDS2->eof())
      {
        std::string strGamePath;
        std::string strFileName = m_pDS2->fv("files.strFilename").get_asString();
        ConstructPath(strGamePath, i.second, strFileName);
        if (HasGameInfo(strGamePath))
          DeleteGame(strGamePath);
        m_pDS2->next();
      }
      m_pDS2->close();
      m_pDS2->exec(PrepareSQL("update path set strContent='', strScraper='', strHash='',strSettings='',useFolderNames=0,scanRecursive=0 where idPath=%i", i.first));
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
  if (progress)
    progress->Close();
}

void CProgramDatabase::SetScraperForPath(const std::string& filePath, const ScraperPtr& scraper, const PROGRAM::SScanSettings& settings)
{
  // if we have a multipath, set scraper for all contained paths
  if(URIUtils::IsMultiPath(filePath))
  {
    std::vector<std::string> paths;
    CMultiPathDirectory::GetPaths(filePath, paths);

    for(unsigned i=0;i<paths.size();i++)
      SetScraperForPath(paths[i],scraper,settings);

    return;
  }

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    int idPath = AddPath(filePath);
    if (idPath < 0)
      return;

    // Update
    std::string strSQL;
    if (settings.exclude)
    { //NB See note in ::GetScraperForPath about strContent=='none'
      strSQL=PrepareSQL("update path set strContent='', strScraper='', scanRecursive=0, useFolderNames=0, strSettings='', noUpdate=0 , exclude=1 where idPath=%i", idPath);
    }
    else if(!scraper)
    { // catch clearing content, but not excluding
      strSQL=PrepareSQL("update path set strContent='', strScraper='', scanRecursive=0, useFolderNames=0, strSettings='', noUpdate=0, exclude=0 where idPath=%i", idPath);
    }
    else
    {
      std::string content = TranslateContent(scraper->Content());
      strSQL=PrepareSQL("update path set strContent='%s', strScraper='%s', scanRecursive=%i, useFolderNames=%i, strSettings='%s', noUpdate=%i, exclude=0 where idPath=%i", content.c_str(), scraper->ID().c_str(),settings.recurse,settings.parent_name,scraper->GetPathSettings().c_str(),settings.noupdate, idPath);
    }
    m_pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, filePath.c_str());
  }
}

int CProgramDatabase::GetSchemaVersion() const
{
  return 1;
}

bool CProgramDatabase::HasContent()
{
  return HasContent(PROGRAMDB_CONTENT_GAMES);
}

bool CProgramDatabase::HasContent(PROGRAMDB_CONTENT_TYPE type)
{
  bool result = false;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql;
    if (type == PROGRAMDB_CONTENT_GAMES)
      sql = "select count(1) from game";
    m_pDS->query( sql );

    if (!m_pDS->eof())
      result = (m_pDS->fv(0).get_asInt() > 0);

    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return result;
}

ScraperPtr CProgramDatabase::GetScraperForPath( const std::string& strPath )
{
  SScanSettings settings;
  return GetScraperForPath(strPath, settings);
}

ScraperPtr CProgramDatabase::GetScraperForPath(const std::string& strPath, SScanSettings& settings)
{
  bool dummy;
  return GetScraperForPath(strPath, settings, dummy);
}

ScraperPtr CProgramDatabase::GetScraperForPath(const std::string& strPath, SScanSettings& settings, bool& foundDirectly)
{
  foundDirectly = false;
  try
  {
    if (strPath.empty() || !m_pDB.get() || !m_pDS.get()) return ScraperPtr();

    ScraperPtr scraper;
    std::string strPath2;

    if (URIUtils::IsMultiPath(strPath))
      strPath2 = CMultiPathDirectory::GetFirstPath(strPath);
    else
      strPath2 = strPath;

    std::string strPath1 = URIUtils::GetDirectory(strPath2);
    int idPath = GetPathId(strPath1);

    if (idPath > -1)
    {
      std::string strSQL=PrepareSQL("select path.strContent,path.strScraper,path.scanRecursive,path.useFolderNames,path.strSettings,path.noUpdate,path.exclude from path where path.idPath=%i",idPath);
      m_pDS->query( strSQL );
    }

    int iFound = 1;
    CONTENT_TYPE content = CONTENT_NONE;
    if (!m_pDS->eof())
    { // path is stored in db

      if (m_pDS->fv("path.exclude").get_asBool())
      {
        settings.exclude = true;
        m_pDS->close();
        return ScraperPtr();
      }
      settings.exclude = false;

      // try and ascertain scraper for this path
      std::string strcontent = m_pDS->fv("path.strContent").get_asString();
      StringUtils::ToLower(strcontent);
      content = TranslateContent(strcontent);

      //FIXME paths stored should not have empty strContent
      //assert(content != CONTENT_NONE);
      std::string scraperID = m_pDS->fv("path.strScraper").get_asString();

      AddonPtr addon;
      if (!scraperID.empty() && CServiceBroker::GetAddonMgr().GetAddon(scraperID, addon))
      {
        scraper = boost::dynamic_pointer_cast<CScraper>(addon);
        if (!scraper)
          return ScraperPtr();

        // store this path's content & settings
        scraper->SetPathSettings(content, m_pDS->fv("path.strSettings").get_asString());
        settings.parent_name = m_pDS->fv("path.useFolderNames").get_asBool();
        settings.recurse = m_pDS->fv("path.scanRecursive").get_asInt();
        settings.noupdate = m_pDS->fv("path.noUpdate").get_asBool();
      }
    }

    if (content == CONTENT_NONE)
    { // this path is not saved in db
      // we must drill up until a scraper is configured
      std::string strParent;
      while (URIUtils::GetParentPath(strPath1, strParent))
      {
        iFound++;

        std::string strSQL=PrepareSQL("select path.strContent,path.strScraper,path.scanRecursive,path.useFolderNames,path.strSettings,path.noUpdate, path.exclude from path where strPath='%s'",strParent.c_str());
        m_pDS->query(strSQL);

        CONTENT_TYPE content = CONTENT_NONE;
        if (!m_pDS->eof())
        {

          std::string strcontent = m_pDS->fv("path.strContent").get_asString();
          StringUtils::ToLower(strcontent);
          if (m_pDS->fv("path.exclude").get_asBool())
          {
            settings.exclude = true;
            scraper.reset();
            m_pDS->close();
            break;
          }

          content = TranslateContent(strcontent);

          AddonPtr addon;
          if (content != CONTENT_NONE &&
              CServiceBroker::GetAddonMgr().GetAddon(m_pDS->fv("path.strScraper").get_asString(), addon))
          {
            scraper = boost::dynamic_pointer_cast<CScraper>(addon);
            scraper->SetPathSettings(content, m_pDS->fv("path.strSettings").get_asString());
            settings.parent_name = m_pDS->fv("path.useFolderNames").get_asBool();
            settings.recurse = m_pDS->fv("path.scanRecursive").get_asInt();
            settings.noupdate = m_pDS->fv("path.noUpdate").get_asBool();
            settings.exclude = false;
            break;
          }
        }
        strPath1 = strParent;
      }
    }
    m_pDS->close();

    if (!scraper || scraper->Content() == CONTENT_NONE)
      return ScraperPtr();

    if (scraper->Content() == CONTENT_GAMES)
    {
      settings.recurse = settings.recurse - (iFound-1);
      settings.parent_name_root = settings.parent_name && (!settings.recurse || iFound > 1);
    }
    else
    {
      iFound = 0;
      return ScraperPtr();
    }
    foundDirectly = (iFound == 1);
    return scraper;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return ScraperPtr();
}

void CProgramDatabase::ConstructPath(std::string& strDest, const std::string& strPath, const std::string& strFileName)
{
  if (URIUtils::IsPlugin(strPath))
    strDest = strFileName;
  else
    strDest = URIUtils::AddFileToFolder(strPath, strFileName);
}

void CProgramDatabase::SplitPath(const std::string& strFileNameAndPath, std::string& strPath, std::string& strFileName)
{
  URIUtils::Split(strFileNameAndPath,strPath, strFileName);
}

void CProgramDatabase::InvalidatePathHash(const std::string& strPath)
{
  SScanSettings settings;
  bool foundDirectly;
  ScraperPtr info = GetScraperForPath(strPath,settings,foundDirectly);
  SetPathHash(strPath,"");
  if (!info)
    return;
  if (info->Content() == CONTENT_GAMES && !foundDirectly && settings.parent_name_root) // if we scan by folder name we need to invalidate parent as well
  {
    std::string strParent;
    URIUtils::GetParentPath(strPath,strParent);
    SetPathHash(strParent,"");
  }
}

bool CProgramDatabase::CommitTransaction()
{
  if (CDatabase::CommitTransaction())
  { // number of items in the db has likely changed, so recalculate
    g_infoManager.SetLibraryBool(LIBRARY_HAS_GAMES, HasContent(PROGRAMDB_CONTENT_GAMES));
    return true;
  }
  return false;
}
