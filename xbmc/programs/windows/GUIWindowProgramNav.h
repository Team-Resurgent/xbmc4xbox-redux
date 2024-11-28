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

#include "GUIWindowProgramBase.h"

class CGUIWindowProgramNav : public CGUIWindowProgramBase
{
public:
  CGUIWindowProgramNav(void);
  virtual ~CGUIWindowProgramNav(void);

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

  virtual void OnItemInfo(const CFileItem& fileItem, ADDON::ScraperPtr &info);

  // override base class methods
  virtual bool GetDirectory(const std::string &strDirectory, CFileItemList &items);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  bool OnAddMediaSource();
  virtual bool OnClick(int iItem, const std::string &player = "");
  virtual std::string GetStartFolder(const std::string &dir);

protected:
  virtual void OnItemLoaded(CFileItem* pItem) {};
};
