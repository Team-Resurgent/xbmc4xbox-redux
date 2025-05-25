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

#include "GUIViewStateProgram.h"
#include "filesystem/ProgramDatabaseDirectory.h"
#include "filesystem/Directory.h"
#include "ProgramDatabase.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/Key.h"
#include "view/ViewStateSettings.h"

using namespace XFILE;
using namespace PROGRAMDATABASEDIRECTORY;

CGUIViewStateWindowProgramNav::CGUIViewStateWindowProgramNav(const CFileItemList& items) : CGUIViewStateWindowProgram(items)
{
  SortAttribute sortAttributes = SortAttributeNone;
  if (CSettings::GetInstance().GetBool("filelists.ignorethewhensorting"))
    sortAttributes = SortAttributeIgnoreArticle;

  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SortByNone, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Label, empty
    SetSortMethod(SortByNone);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SortOrderNone);
  }
  else if (items.IsProgramDb())
  {
    NODE_TYPE NodeType=CProgramDatabaseDirectory::GetDirectoryChildType(items.GetPath());
    CQueryParams params;
    CProgramDatabaseDirectory::GetQueryParams(items.GetPath(),params);

    switch (NodeType)
    {
    case NODE_TYPE_YEAR:
      {
        AddSortMethod(SortByLabel, 562, LABEL_MASKS("%T", "%R", "%L", ""));  // Title, Rating | Label, empty
        SetSortMethod(SortByLabel);

        const CViewState *viewState = CViewStateSettings::Get().Get("programnavyears");
        SetViewAsControl(viewState->m_viewMode);
        SetSortOrder(viewState->m_sortDescription.sortOrder);
      }
      break;
    case NODE_TYPE_DEVELOPER:
    case NODE_TYPE_PUBLISHER:
    case NODE_TYPE_GENRE:
    case NODE_TYPE_DESCRIPTOR:
    case NODE_TYPE_GENERALFEATURE:
    case NODE_TYPE_ONLINEFEATURE:
    case NODE_TYPE_PLATFORM:
      {
        AddSortMethod(SortByLabel, 551, LABEL_MASKS("%T", "%R", "%L", ""));  // Title, Rating | Label, empty
        SetSortMethod(SortByLabel);

        const CViewState *viewState = CViewStateSettings::Get().Get("programnavgenres");
        SetViewAsControl(viewState->m_viewMode);
        SetSortOrder(viewState->m_sortDescription.sortOrder);
      }
      break;
    case NODE_TYPE_TAGS:
      {
        AddSortMethod(SortByLabel, sortAttributes, 551, LABEL_MASKS("%T","", "%T",""));  // Title, empty | Title, empty
        SetSortMethod(SortByLabel);

        const CViewState *viewState = CViewStateSettings::Get().Get("programnavgenres");
        SetViewAsControl(viewState->m_viewMode);
        SetSortOrder(viewState->m_sortDescription.sortOrder);
      }
      break;
    case NODE_TYPE_TITLE_GAMES:
    case NODE_TYPE_TITLE_APPS:
      {
        AddSortMethod(SortBySortTitle, sortAttributes, 556, LABEL_MASKS("%T", "%R", "%T", "%R"));  // Title, Rating | Title, Rating
        AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y", "%T", "%Y"));  // Title, Year | Title, Year
        AddSortMethod(SortByRating, 563, LABEL_MASKS("%T", "%R", "%T", "%R"));  // Title, Rating | Title, Rating
        AddSortMethod(SortByDateAdded, 570, LABEL_MASKS("%T", "%a", "%T", "%a"));  // Title, DateAdded | Title, DateAdded

        AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T", "%V", "%T", "%V"));  // Title, Playcount | Title, Playcount

        const CViewState *viewState = CViewStateSettings::Get().Get("programnavtitles");
        SetSortMethod(viewState->m_sortDescription);
        SetSortOrder(viewState->m_sortDescription.sortOrder);

        SetViewAsControl(viewState->m_viewMode);
      }
      break;
    case NODE_TYPE_RECENTLY_ADDED_GAMES:
      {
        AddSortMethod(SortByNone, 552, LABEL_MASKS("%T", "%R"));  // Title, Rating | empty, empty
        SetSortMethod(SortByNone);

        SetViewAsControl(CViewStateSettings::Get().Get("programnavtitles")->m_viewMode);

        SetSortOrder(SortOrderNone);
      }
      break;
    default:
      break;
    }
  }
  else
  {
    AddSortMethod(SortByLabel, sortAttributes, 551, LABEL_MASKS("%L", "", "%L", ""));  // Label, empty | Label, empty
    AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Label, Date | Label, Date

    const CViewState *viewState = CViewStateSettings::Get().Get("programfiles");
    SetSortMethod(viewState->m_sortDescription);
    SetViewAsControl(viewState->m_viewMode);
    SetSortOrder(viewState->m_sortDescription.sortOrder);
  }
  LoadViewState(items.GetPath(), WINDOW_PROGRAM_NAV);
}

void CGUIViewStateWindowProgramNav::SaveViewState()
{
  if (m_items.IsProgramDb())
  {
    NODE_TYPE NodeType = CProgramDatabaseDirectory::GetDirectoryChildType(m_items.GetPath());
    CQueryParams params;
    CProgramDatabaseDirectory::GetQueryParams(m_items.GetPath(),params);
    switch (NodeType)
    {
    case NODE_TYPE_YEAR:
      SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAM_NAV, CViewStateSettings::Get().Get("programnavyears"));
      break;
    case NODE_TYPE_GENRE:
      SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAM_NAV, CViewStateSettings::Get().Get("programnavgenres"));
      break;
    case NODE_TYPE_TITLE_GAMES:
    case NODE_TYPE_TITLE_APPS:
      SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAM_NAV, CViewStateSettings::Get().Get("programnavtitles"));
      break;
    default:
      SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAM_NAV);
      break;
    }
  }
  else
  {
    SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAM_NAV, CViewStateSettings::Get().Get("programfiles"));
  }
}

VECSOURCES& CGUIViewStateWindowProgramNav::GetSources()
{
  //  Setup shares we want to have
  m_sources.clear();
  CFileItemList items;
  if (CSettings::GetInstance().GetBool("myprograms.flatten"))
    CDirectory::GetDirectory("library://program_flat/", items, "", DIR_FLAG_DEFAULTS);
  else
    CDirectory::GetDirectory("library://program/", items, "", DIR_FLAG_DEFAULTS);
  for (int i=0; i<items.Size(); ++i)
  {
    CFileItemPtr item=items[i];
    CMediaSource share;
    share.strName=item->GetLabel();
    share.strPath = item->GetPath();
    share.m_strThumbnailImage= item->GetIconImage();
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    m_sources.push_back(share);
  }
  return CGUIViewStateWindowProgram::GetSources();
}

CGUIViewStateProgramGames::CGUIViewStateProgramGames(const CFileItemList& items) : CGUIViewStateWindowProgram(items)
{
  AddSortMethod(SortBySortTitle, 556, LABEL_MASKS("%T", "%R", "%T", "%R"),  // Title, Rating | Title, Rating
    CSettings::GetInstance().GetBool("filelists.ignorethewhensorting") ? SortAttributeIgnoreArticle : SortAttributeNone);
  AddSortMethod(SortByYear, 562, LABEL_MASKS("%T", "%Y", "%T", "%Y"));  // Title, Year | Title, Year
  AddSortMethod(SortByRating, 563, LABEL_MASKS("%T", "%R", "%T", "%R"));  // Title, Rating | Title, Rating
  AddSortMethod(SortByDateAdded, 570, LABEL_MASKS("%T", "%a", "%T", "%a"));  // Title, DateAdded | Title, DateAdded

  AddSortMethod(SortByPlaycount, 567, LABEL_MASKS("%T", "%V", "%T", "%V"));  // Title, Playcount | Title, Playcount

  const CViewState *viewState = CViewStateSettings::Get().Get("programnavtitles");
  if (items.IsSmartPlayList() || items.IsLibraryFolder())
    AddPlaylistOrder(items, LABEL_MASKS("%T", "%R", "%T", "%R"));  // Title, Rating | Title, Rating
  else
  {
    SetSortMethod(viewState->m_sortDescription);
    SetSortOrder(viewState->m_sortDescription.sortOrder);
  }

  SetViewAsControl(viewState->m_viewMode);

  LoadViewState(items.GetPath(), WINDOW_PROGRAM_NAV);
}

void CGUIViewStateProgramGames::SaveViewState()
{
  SaveViewToDb(m_items.GetPath(), WINDOW_PROGRAM_NAV, CViewStateSettings::Get().Get("programnavtitles"));
}