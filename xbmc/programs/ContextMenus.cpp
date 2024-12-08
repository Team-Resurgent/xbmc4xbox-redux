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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ContextMenus.h"
#include "Application.h"
#include "programs/dialogs/GUIDialogProgramInfo.h"
#include "programs/dialogs/GUIDialogProgramSettings.h"
#include "programs/windows/GUIWindowProgramBase.h"


namespace CONTEXTMENU
{

CProgramInfo::CProgramInfo(MediaType mediaType)
    : CStaticContextMenuAction(19033), m_mediaType(mediaType) {}

bool CProgramInfo::IsVisible(const CFileItem& item) const
{
  if (!item.HasProgramInfoTag())
    return false;

  return item.GetProgramInfoTag()->m_type == m_mediaType;
}

bool CProgramInfo::Execute(const CFileItemPtr& item) const
{
  CGUIDialogProgramInfo::ShowFor(*item);
  return true;
}

std::string CLaunchCustom::GetLabel(const CFileItem& item) const
{
  return g_localizeStrings.Get(38996);
}

bool CLaunchCustom::IsVisible(const CFileItem& item) const
{
  if (item.m_bIsFolder)
    return false; //! @todo implement

  return item.IsProgram();
}

bool CLaunchCustom::Execute(const CFileItemPtr& item) const
{
  CGUIDialogProgramSettings::ShowForTitle(item);
  return true;
};
}
