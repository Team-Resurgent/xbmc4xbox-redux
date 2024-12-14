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

#include "DirectoryNodeAppsOverview.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "programs/ProgramDatabase.h"
#include "programs/ProgramDbUrl.h"
#include "utils/StringUtils.h"

using namespace XFILE::PROGRAMDATABASEDIRECTORY;

Node AppChildren[] = {
                        { NODE_TYPE_TITLE_APPS,     "titles",          369 },
                        { NODE_TYPE_TAGS,           "tags",            20459 }
                       };

CDirectoryNodeAppsOverview::CDirectoryNodeAppsOverview(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_APPS_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeAppsOverview::GetChildType() const
{
  for (unsigned int i = 0; i < sizeof(AppChildren) / sizeof(Node); ++i)
    if (GetName() == AppChildren[i].id)
      return AppChildren[i].node;

  return NODE_TYPE_NONE;
}

std::string CDirectoryNodeAppsOverview::GetLocalizedName() const
{
  for (unsigned int i = 0; i < sizeof(AppChildren) / sizeof(Node); ++i)
    if (GetName() == AppChildren[i].id)
      return g_localizeStrings.Get(AppChildren[i].label);
  return "";
}

bool CDirectoryNodeAppsOverview::GetContent(CFileItemList& items) const
{
  CProgramDbUrl programUrl;
  if (!programUrl.FromString(BuildPath()))
    return false;

  for (unsigned int i = 0; i < sizeof(AppChildren) / sizeof(Node); ++i)
  {
    CProgramDbUrl itemUrl = programUrl;
    std::string strDir = StringUtils::Format("%s/", AppChildren[i].id.c_str());
    itemUrl.AppendPath(strDir);

    CFileItemPtr pItem(new CFileItem(itemUrl.ToString(), true));
    pItem->SetLabel(g_localizeStrings.Get(AppChildren[i].label));
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
