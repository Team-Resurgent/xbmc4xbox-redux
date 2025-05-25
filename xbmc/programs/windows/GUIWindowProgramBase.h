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

#include "windows/GUIMediaWindow.h"
#include "programs/ProgramDatabase.h"

class CGUIWindowProgramBase : public CGUIMediaWindow
{
public:
  CGUIWindowProgramBase(int id, const std::string &xmlFile);
  virtual ~CGUIWindowProgramBase(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  /*! \brief Prompt the user for assigning content to a path.
   Based on changes, we then call OnUnassignContent, update or refresh scraper information in the database
   and optionally start a scan
   \param path the path to assign content for
   */
  static void OnAssignContent(const std::string &path);

protected:
  void OnScan(const std::string& strPath, bool scanAll = false);
  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);

  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);

  int GetScraperForItem(CFileItem *item, ADDON::ScraperPtr &info, PROGRAM::SScanSettings& settings);

  static bool OnUnAssignContent(const std::string &path, int header, int text);

  CProgramDatabase m_database;
};
