#pragma once

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

#include "guilib/GUIDialog.h"
#include "FileItem.h"

class CGUIDialogProgramInfo :
      public CGUIDialog
{
public:
  CGUIDialogProgramInfo(void);
  virtual ~CGUIDialogProgramInfo(void);
  bool OnMessage(CGUIMessage& message);
  void SetProgram(const CFileItem *item);
  bool NeedRefresh() const;

  std::string GetThumbnail() const;

  static bool GetItemsForTag(const std::string &strHeading, const std::string &type, CFileItemList &items, int idTag = -1, bool showAll = true);

  static std::string GetLocalizedProgramType(const std::string &strType);

  static void ShowFor(const CFileItem& item);

protected:
  void OnInitWindow();
  void Update();
  void SetLabel(int iControl, const std::string& strLabel);

  // link screenshot to games
  void ClearScreenshotList();

  void PlayTrailer();

  CFileItemPtr m_programItem;
  CFileItemList *m_screenshotList;
  bool m_bRefresh;
};
