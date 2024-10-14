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

#include "ProgramDbUrl.h"
#include "filesystem/ProgramDatabaseDirectory.h"
#include "playlists/SmartPlayList.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace XFILE;

CProgramDbUrl::CProgramDbUrl()
  : CDbUrl()
{ }

CProgramDbUrl::~CProgramDbUrl()
{ }

bool CProgramDbUrl::parse()
{
  // the URL must start with programdb://
  if (!m_url.IsProtocol("programdb") || m_url.GetFileName().empty())
    return false;

  std::string path = m_url.Get();
  PROGRAMDATABASEDIRECTORY::NODE_TYPE dirType = CProgramDatabaseDirectory::GetDirectoryType(path);
  PROGRAMDATABASEDIRECTORY::NODE_TYPE childType = CProgramDatabaseDirectory::GetDirectoryChildType(path);

  switch (dirType)
  {
    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_GAMES_OVERVIEW:
    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_TITLE_GAMES:
      m_type = "games";
      break;

    default:
      break;
  }

  switch (childType)
  {
    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_GAMES_OVERVIEW:
    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_TITLE_GAMES:
      m_type = "games";
      m_itemType = "games";
      break;

    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_ROOT:
    case PROGRAMDATABASEDIRECTORY::NODE_TYPE_OVERVIEW:
    default:
      return false;
  }

  if (m_type.empty() || m_itemType.empty())
    return false;

  // parse query params
  PROGRAMDATABASEDIRECTORY::CQueryParams queryParams;
  if (!CProgramDatabaseDirectory::GetQueryParams(path, queryParams))
    return false;

  // retrieve and parse all options
  AddOptions(m_url.GetOptions());

  // TODO: add options based on the QueryParams

  return true;
}

bool CProgramDbUrl::validateOption(const std::string &key, const CVariant &value)
{
  if (!CDbUrl::validateOption(key, value))
    return false;

  // if the value is empty it will remove the option which is ok
  // otherwise we only care about the "filter" option here
  if (value.empty() || !StringUtils::EqualsNoCase(key, "filter"))
    return true;

  if (!value.isString())
    return false;

  CSmartPlaylist xspFilter;
  if (!xspFilter.LoadFromJson(value.asString()))
    return false;

  // check if the filter playlist matches the item type
  return xspFilter.GetType() == m_itemType;
}
