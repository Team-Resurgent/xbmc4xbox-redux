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
  CLog::Log(LOGINFO, "create path table");
  m_pDS->exec("CREATE TABLE path ( idPath integer primary key, strPath text, strContent text, strScraper text, strHash text, scanRecursive integer, useFolderNames bool, strSettings text, noUpdate bool, exclude bool, dateAdded text, idParentPath integer)");
}

void CProgramDatabase::CreateAnalytics()
{
  CLog::Log(LOGINFO, "%s - creating indicies", __FUNCTION__);
  m_pDS->exec("CREATE INDEX ix_path ON path ( strPath(255) )");
  m_pDS->exec("CREATE INDEX ix_path2 ON path ( idParentPath )");
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

      // TODO: delete all games assoicated with this path if user asked
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

bool CProgramDatabase::CommitTransaction()
{
  if (CDatabase::CommitTransaction())
  { // number of items in the db has likely changed, so recalculate
    g_infoManager.SetLibraryBool(LIBRARY_HAS_GAMES, HasContent(PROGRAMDB_CONTENT_GAMES));
    return true;
  }
  return false;
}
