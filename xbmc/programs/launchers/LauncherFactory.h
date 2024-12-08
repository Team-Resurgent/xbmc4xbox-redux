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

#include "IProgramLauncher.h"

class CURL;

namespace LAUNCHERS
{
/*!
 \ingroup launchers
 \brief Get a launcher of given executable.
 \n
 Example:

 \verbatim
 std::string strExecutable="F:\Games\Halo: Combat Evolved\default.xbe";

 IProgramLauncher* pLauncher=CLauncherFactory::Create(strExecutable);
 \endverbatim
 The \e pLauncher pointer can be used to launch a program executable.
 \sa IProgramLauncher
 */
class CLauncherFactory
{
public:
  static IProgramLauncher* Create(const CURL& url);
};
}
