/*
 *      Copyright (C) 2014 Team XBMC
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

#include "ProgramLibraryQueue.h"

#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "threads/SingleLock.h"
#include "Util.h"
#include "programs/jobs/ProgramLibraryJob.h"
#include "programs/jobs/ProgramLibraryRefreshingJob.h"
#include "programs/jobs/ProgramLibraryScanningJob.h"

CProgramLibraryQueue::CProgramLibraryQueue()
  : CJobQueue(false, 1, CJob::PRIORITY_LOW),
    m_jobs(),
    m_modal(false),
    m_cleaning(false)
{ }

CProgramLibraryQueue::~CProgramLibraryQueue()
{
  CSingleLock lock(m_critical);
  m_jobs.clear();
}

CProgramLibraryQueue& CProgramLibraryQueue::GetInstance()
{
  static CProgramLibraryQueue s_instance;
  return s_instance;
}

void CProgramLibraryQueue::ScanLibrary(const std::string& directory, bool scanAll /* = false */ , bool showProgress /* = true */)
{
  AddJob(new CProgramLibraryScanningJob(directory, scanAll, showProgress));
}

bool CProgramLibraryQueue::IsScanningLibrary() const
{
  // check if the library is being cleaned synchronously
  if (m_cleaning)
    return true;

  // check if the library is being scanned asynchronously
  ProgramLibraryJobMap::const_iterator scanningJobs = m_jobs.find("ProgramLibraryScanningJob");
  if (scanningJobs != m_jobs.end() && !scanningJobs->second.empty())
    return true;

  // check if the library is being cleaned asynchronously
  ProgramLibraryJobMap::const_iterator cleaningJobs = m_jobs.find("ProgramLibraryCleaningJob");
  if (cleaningJobs != m_jobs.end() && !cleaningJobs->second.empty())
    return true;

  return false;
}

bool CProgramLibraryQueue::RefreshItemModal(CFileItemPtr item, bool forceRefresh /* = true */)
{
  // we can't perform a modal library cleaning if other jobs are running
  if (IsRunning())
    return false;

  m_modal = true;
  CProgramLibraryRefreshingJob refreshingJob(item, forceRefresh);

  bool result = refreshingJob.DoModal();
  m_modal = false;

  return result;
}

void CProgramLibraryQueue::AddJob(CProgramLibraryJob *job)
{
  if (job == NULL)
    return;

  CSingleLock lock(m_critical);
  if (!CJobQueue::AddJob(job))
    return;

  // add the job to our list of queued/running jobs
  std::string jobType = job->GetType();
  ProgramLibraryJobMap::iterator jobsIt = m_jobs.find(jobType);
  if (jobsIt == m_jobs.end())
  {
    ProgramLibraryJobs jobs;
    jobs.insert(job);
    m_jobs.insert(std::make_pair(jobType, jobs));
  }
  else
    jobsIt->second.insert(job);
}

bool CProgramLibraryQueue::IsRunning() const
{
  return CJobQueue::IsProcessing() || m_modal;
}

void CProgramLibraryQueue::Refresh()
{
  CUtil::DeleteProgramDatabaseDirectoryCache();
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE);
  g_windowManager.SendThreadMessage(msg);
}

void CProgramLibraryQueue::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (success)
  {
    if (QueueEmpty())
      Refresh();
  }

  {
    CSingleLock lock(m_critical);
    // remove the job from our list of queued/running jobs
    ProgramLibraryJobMap::iterator jobsIt = m_jobs.find(job->GetType());
    if (jobsIt != m_jobs.end())
      jobsIt->second.erase(static_cast<CProgramLibraryJob*>(job));
  }

  return CJobQueue::OnJobComplete(jobID, success, job);
}
