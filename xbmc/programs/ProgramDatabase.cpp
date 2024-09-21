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

#include "dbwrappers/dataset.h"
#include "guiinfo/GUIInfoLabels.h"
#include "GUIInfoManager.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

CProgramDatabase::CProgramDatabase(void)
{
}

CProgramDatabase::~CProgramDatabase(void)
{}

bool CProgramDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseProgram);
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

bool CProgramDatabase::CommitTransaction()
{
  if (CDatabase::CommitTransaction())
  { // number of items in the db has likely changed, so recalculate
    g_infoManager.SetLibraryBool(LIBRARY_HAS_GAMES, HasContent(PROGRAMDB_CONTENT_GAMES));
    return true;
  }
  return false;
}
