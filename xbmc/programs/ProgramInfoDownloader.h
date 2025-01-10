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

#include <string>
#include <vector>

#include "threads/Thread.h"
#include "ProgramInfoTag.h"
#include "addons/Scraper.h"

// forward declarations
class CGUIDialogProgress;

namespace ADDON
{
class CScraperError;
}

typedef std::vector<CScraperUrl> PROGRAMLIST;

class CProgramInfoDownloader : public CThread
{
public:
  CProgramInfoDownloader(const ADDON::ScraperPtr &scraper);
  virtual ~CProgramInfoDownloader();

  // threaded lookup functions

  /*! \brief Do a search for matching media items (possibly asynchronously) with our scraper
   \param strProgram name of the media item to look for
   \param programlist [out] list of results to fill. May be empty on success.
   \param pProgress progress bar to update as we go. If NULL we run on thread, if non-NULL we run off thread.
   \return 1 on success, -1 on a scraper-specific error, 0 on some other error
   */
  int FindProgram(const std::string& strProgram, PROGRAMLIST& programlist, CGUIDialogProgress *pProgress = NULL);

  bool GetDetails(const CScraperUrl& url, CProgramInfoTag &programDetails, CGUIDialogProgress *pProgress = NULL);

  static void ShowErrorDialog(const ADDON::CScraperError &sce);

protected:
  enum LOOKUP_STATE { DO_NOTHING = 0,
                      FIND_PROGRAM = 1,
                      GET_DETAILS = 2 };

  std::string         m_strProgram;
  PROGRAMLIST         m_programList;
  CProgramInfoTag     m_programDetails;
  CScraperUrl         m_url;
  LOOKUP_STATE        m_state;
  int                 m_found;
  ADDON::ScraperPtr   m_info;

  // threaded stuff
  void Process();
  void CloseThread();

  int InternalFindProgram(const std::string& strProgram, PROGRAMLIST& programlist, bool cleanChars = true);
};

