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

#include "ProgramDatabaseDirectory.h"
#include "utils/URIUtils.h"
#include "guilib/TextureManager.h"
#include "File.h"
#include "FileItem.h"
#include "settings/Settings.h"

using namespace XFILE;
using namespace PROGRAMDATABASEDIRECTORY;

CProgramDatabaseDirectory::CProgramDatabaseDirectory(void)
{
}

CProgramDatabaseDirectory::~CProgramDatabaseDirectory(void)
{
}

bool CProgramDatabaseDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  items.SetPath(url.Get());
  items.m_dwSize = -1;  // No size
  boost::movelib::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(url.Get()));

  if (!pNode.get())
    return false;

  bool bResult = pNode->GetChilds(items);
  for (int i=0;i<items.Size();++i)
  {
    CFileItemPtr item = items[i];
    if (item->m_bIsFolder && !item->HasIcon() && !item->HasArt("thumb"))
    {
      std::string strImage = GetIcon(item->GetPath());
      if (!strImage.empty() && g_TextureManager.HasTexture(strImage))
        item->SetIconImage(strImage);
    }
  }
  items.SetLabel(pNode->GetLocalizedName());

  return bResult;
}

NODE_TYPE CProgramDatabaseDirectory::GetDirectoryChildType(const std::string& strPath)
{
  boost::movelib::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return NODE_TYPE_NONE;

  return pNode->GetChildType();
}

NODE_TYPE CProgramDatabaseDirectory::GetDirectoryType(const std::string& strPath)
{
  boost::movelib::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return NODE_TYPE_NONE;

  return pNode->GetType();
}

bool CProgramDatabaseDirectory::GetQueryParams(const std::string& strPath, CQueryParams& params)
{
  boost::movelib::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(strPath));

  if (!pNode.get())
    return false;

  CDirectoryNode::GetDatabaseInfo(strPath,params);
  return true;
}

std::string CProgramDatabaseDirectory::GetIcon(const std::string &strDirectory)
{
  std::string path = strDirectory;
  switch (GetDirectoryChildType(path))
  {
  case NODE_TYPE_TITLE_GAMES:
    if (URIUtils::PathEquals(path, "programdb://games/titles/"))
    {
      if (CSettings::GetInstance().GetBool("myprograms.flatten"))
        return "DefaultGames.png";
      return "DefaultMovieTitle.png";
    }
    return "";
  case NODE_TYPE_GAMES_OVERVIEW: // Games
    return "DefaultGames.png";
  case NODE_TYPE_RECENTLY_ADDED_GAMES: // Recently Added Games
    return "DefaultRecentlyAddedMovies.png";
  default:
    break;
  }

  return "";
}
