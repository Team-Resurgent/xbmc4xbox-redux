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

#include "ProgramInfoDownloader.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogOK.h"
#include "messaging/ApplicationMessenger.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"
#include "utils/Variant.h"

using namespace KODI::MESSAGING;

#ifndef __GNUC__
#pragma warning (disable:4018)
#endif

CProgramInfoDownloader::CProgramInfoDownloader(const ADDON::ScraperPtr &scraper) :
  CThread("ProgramInfoDownloader"), m_state(DO_NOTHING), m_found(0), m_info(scraper)
{
}

CProgramInfoDownloader::~CProgramInfoDownloader()
{
}

// return value: 0 = we failed, -1 = we failed and reported an error, 1 = success
int CProgramInfoDownloader::InternalFindProgram(const std::string &strProgram,
                                            PROGRAMLIST& programlist,
                                            bool cleanChars /* = true */)
{
  try
  {
    programlist = m_info->FindProgram(strProgram, cleanChars);
  }
  catch (const ADDON::CScraperError &sce)
  {
    ShowErrorDialog(sce);
    return sce.FAborted() ? 0 : -1;
  }
  return 1;  // success
}

void CProgramInfoDownloader::ShowErrorDialog(const ADDON::CScraperError &sce)
{
  if (!sce.Title().empty())
  {
    CGUIDialogOK *pdlg = static_cast<CGUIDialogOK*>(g_windowManager.GetWindow(WINDOW_DIALOG_OK));
    pdlg->SetHeading(sce.Title());
    pdlg->SetLine(0, sce.Message());
    pdlg->Open();
  }
}

// threaded functions
void CProgramInfoDownloader::Process()
{
  // note here that we're calling our external functions but we're calling them with
  // no progress bar set, so they're effectively calling our internal functions directly.
  m_found = 0;
  if (m_state == FIND_PROGRAM)
  {
    if (!(m_found=FindProgram(m_strProgram, m_programList)))
      CLog::Log(LOGERROR, "%s: Error looking up item %s", __FUNCTION__, m_strProgram.c_str());
    m_state = DO_NOTHING;
    return;
  }

  if (m_url.m_url.empty())
  {
    // empty url when it's not supposed to be..
    // this might happen if the previously scraped item was removed from the site (see ticket #10537)
    CLog::Log(LOGERROR, "%s: Error getting details for %s due to an empty url", __FUNCTION__, m_strProgram.c_str());
  }
  else if (m_state == GET_DETAILS)
  {
    if (!GetDetails(m_url, m_programDetails))
      CLog::Log(LOGERROR, "%s: Error getting details from %s", __FUNCTION__,m_url.m_url[0].m_url.c_str());
  }
  m_found = 1;
  m_state = DO_NOTHING;
}

int CProgramInfoDownloader::FindProgram(const std::string &strProgram,
                                    PROGRAMLIST& programList,
                                    CGUIDialogProgress *pProgress /* = NULL */)
{
  if (pProgress)
  { // threaded version
    m_state = FIND_PROGRAM;
    m_strProgram = strProgram;
    m_found = 0;
    if (IsRunning())
      StopThread();
    Create();
    while (m_state != DO_NOTHING)
    {
      pProgress->Progress();
      if (pProgress->IsCanceled())
      {
        CloseThread();
        return 0;
      }
      Sleep(1);
    }
    // transfer to our programlist
    m_programList.swap(programList);
    int found=m_found;
    CloseThread();
    return found;
  }

  // unthreaded
  int success = InternalFindProgram(strProgram, programList);
  // NOTE: this might be improved by rescraping if the match quality isn't high?
  if (success == 1 && programList.empty())
  { // no results. try without cleaning chars like '.' and '_'
    success = InternalFindProgram(strProgram, programList, false);
  }
  return success;
}

bool CProgramInfoDownloader::GetDetails(const CScraperUrl &url,
                                      CProgramInfoTag &programDetails,
                                      CGUIDialogProgress *pProgress /* = NULL */)
{
  m_url = url;
  m_programDetails = programDetails;

  // fill in the defaults
  programDetails.Reset();
  if (pProgress)
  { // threaded version
    m_state = GET_DETAILS;
    m_found = 0;
    if (IsRunning())
      StopThread();
    Create();
    while (!m_found)
    {
      pProgress->Progress();
      if (pProgress->IsCanceled())
      {
        CloseThread();
        return false;
      }
      Sleep(1);
    }
    programDetails = m_programDetails;
    CloseThread();
    return true;
  }
  else  // unthreaded
    return m_info->GetProgramDetails(url, programDetails);
}

void CProgramInfoDownloader::CloseThread()
{
  StopThread();
  m_state = DO_NOTHING;
  m_found = 0;
}

