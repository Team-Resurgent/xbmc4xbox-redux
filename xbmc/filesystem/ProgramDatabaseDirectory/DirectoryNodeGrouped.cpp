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

#include "DirectoryNodeGrouped.h"
#include "QueryParams.h"
#include "programs/ProgramDatabase.h"
#include "programs/ProgramDbUrl.h"

using namespace XFILE::PROGRAMDATABASEDIRECTORY;

CDirectoryNodeGrouped::CDirectoryNodeGrouped(NODE_TYPE type, const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(type, strName, pParent)
{ }

NODE_TYPE CDirectoryNodeGrouped::GetChildType() const
{
  CQueryParams params;
  CollectQueryParams(params);

  if (params.GetContentType() == PROGRAMDB_CONTENT_GAMES)
    return NODE_TYPE_TITLE_GAMES;

  return NODE_TYPE_NONE;
}

std::string CDirectoryNodeGrouped::GetLocalizedName() const
{
  CProgramDatabase db;
  if (db.Open())
    return db.GetItemById(GetContentType(), GetID());

  return "";
}

bool CDirectoryNodeGrouped::GetContent(CFileItemList& items) const
{
  CProgramDatabase programdatabase;
  if (!programdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  std::string itemType = GetContentType(params);
  if (itemType.empty())
    return false;

  // make sure to translate all IDs in the path into URL parameters
  CProgramDbUrl programUrl;
  if (!programUrl.FromString(BuildPath()))
    return false;

  return programdatabase.GetItems(programUrl.ToString(), (PROGRAMDB_CONTENT_TYPE)params.GetContentType(), itemType, items);
}

std::string CDirectoryNodeGrouped::GetContentType() const
{
  CQueryParams params;
  CollectQueryParams(params);

  return GetContentType(params);
}

std::string CDirectoryNodeGrouped::GetContentType(const CQueryParams &params) const
{
  switch (GetType())
  {
    case NODE_TYPE_DEVELOPER:
      return "developers";
    case NODE_TYPE_PUBLISHER:
      return "publishers";
    case NODE_TYPE_GENRE:
      return "genres";
    case NODE_TYPE_DESCRIPTOR:
      return "descriptors";
    case NODE_TYPE_GENERALFEATURE:
      return "generalfeatures";
    case NODE_TYPE_ONLINEFEATURE:
      return "onlinefeatures";
    case NODE_TYPE_PLATFORM:
      return "platforms";
    case NODE_TYPE_YEAR:
      return "years";

    case NODE_TYPE_GAMES_OVERVIEW:
    case NODE_TYPE_NONE:
    case NODE_TYPE_OVERVIEW:
    case NODE_TYPE_ROOT:
    case NODE_TYPE_TITLE_GAMES:
    default:
      break;
  }

  return "";
}
