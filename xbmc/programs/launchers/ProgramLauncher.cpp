/*
 *      Copyright (C) 2005-2024 Team XBMC
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

#include "ProgramLauncher.h"
#include "LauncherFactory.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "URL.h"

using namespace LAUNCHERS;

CProgramLauncher::CProgramLauncher()
{}

CProgramLauncher::~CProgramLauncher()
{}

bool CProgramLauncher::LaunchProgram(const std::string& strExecutable, bool bLookForSettings /* = true */, bool bAllowRegionSwitching /* = true */)
{
  const CURL url(strExecutable);
  return LaunchProgram(url);
}

bool CProgramLauncher::LaunchProgram(const CURL& url, bool bLookForSettings /* = true */, bool bAllowRegionSwitching /* = true */)
{
  try
  {
    CURL realURL = URIUtils::SubstitutePath(url);
    boost::shared_ptr<IProgramLauncher> pProgramLauncher(CLauncherFactory::Create(realURL));
    if (!pProgramLauncher.get())
      return false;

    if (pProgramLauncher->Launch(bLookForSettings, bAllowRegionSwitching))
      return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error launching %s", __FUNCTION__, url.GetRedacted().c_str());
  return false;
}
