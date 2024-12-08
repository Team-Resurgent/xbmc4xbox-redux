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

#include "LauncherFactory.h"
#include "utils/log.h"
#include "URL.h"

using namespace LAUNCHERS;

/*!
 \brief Create a IProgramLauncher object of the executable specified in \e strPath .
 \param strPath Specifies the executable to access, can be a share or share with path.
 \return IProgramLauncher object to allow launching of executable.
 \sa IProgramLauncher
 */
IProgramLauncher* CLauncherFactory::Create(const CURL& url)
{
  // We currently only support executables from HDD
  if (!url.IsProtocol(""))
  {
    CLog::Log(LOGWARNING, "%s - unsupported protocol: %s", __FUNCTION__, url.GetProtocol().c_str());
    return false;
  }

  CLog::Log(LOGWARNING, "%s - unsupported executable: %s", __FUNCTION__, url.Get().c_str());
  return NULL;
}

