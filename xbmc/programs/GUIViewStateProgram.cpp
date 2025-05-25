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
#include "filesystem/Directory.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/Key.h"
#include "view/ViewStateSettings.h"

using namespace XFILE;

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
  // TODO: add sort methods, sort order, views, etc. for each node
  else
  {
    AddSortMethod(SortByLabel, sortAttributes, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // Label, Size | Label, empty
    AddSortMethod(SortBySize, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // Label, Size | Label, Size
    AddSortMethod(SortByDate, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // Label, Date | Label, Date
    AddSortMethod(SortByFile, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Label, Size | Label, empty

    const CViewState *viewState = CViewStateSettings::Get().Get("programfiles");
    SetSortMethod(viewState->m_sortDescription);
    SetViewAsControl(viewState->m_viewMode);
    SetSortOrder(viewState->m_sortDescription.sortOrder);
  }
  LoadViewState(items.GetPath(), WINDOW_PROGRAM_NAV);
}

void CGUIViewStateWindowProgramNav::SaveViewState()
{
  // TODO: save view states for program nodes (genres, year, title, etc.)
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
