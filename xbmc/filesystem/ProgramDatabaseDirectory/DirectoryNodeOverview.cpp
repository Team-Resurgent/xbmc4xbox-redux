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

#include "DirectoryNodeOverview.h"

#include <utility>

#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "programs/ProgramDatabase.h"

using namespace XFILE::PROGRAMDATABASEDIRECTORY;

Node OverviewChildren[] = {
                            { NODE_TYPE_GAMES_OVERVIEW,            "games",                   38928 },
                            { NODE_TYPE_APPS_OVERVIEW,             "apps",                    38932 },
                            { NODE_TYPE_RECENTLY_ADDED_GAMES,      "recentlyaddedgames",      38972 },
                          };

CDirectoryNodeOverview::CDirectoryNodeOverview(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeOverview::GetChildType() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
    if (GetName() == OverviewChildren[i].id)
      return OverviewChildren[i].node;

  return NODE_TYPE_NONE;
}

std::string CDirectoryNodeOverview::GetLocalizedName() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
    if (GetName() == OverviewChildren[i].id)
      return g_localizeStrings.Get(OverviewChildren[i].label);
  return "";
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items) const
{
  CProgramDatabase database;
  database.Open();
  bool hasGames = database.HasContent(PROGRAMDB_CONTENT_GAMES);
  std::vector<std::pair<const char*, int> > vec;
  if (hasGames)
  {
    // TODO: add recently added games node
    if (CSettings::GetInstance().GetBool("myprograms.flatten"))
      vec.push_back(std::make_pair("games/titles", 38928));
    else
      vec.push_back(std::make_pair("games", 38928));   // Games
  }
  std::string path = BuildPath();
  for (unsigned int i = 0; i < vec.size(); ++i)
  {
    CFileItemPtr pItem(new CFileItem(path + vec[i].first + "/", true));
    pItem->SetLabel(g_localizeStrings.Get(vec[i].second));
    pItem->SetLabelPreformated(true);
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
