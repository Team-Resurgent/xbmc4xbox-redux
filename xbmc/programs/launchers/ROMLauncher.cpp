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

#include "ROMLauncher.h"
#include "programs/ProgramDatabase.h"
#include "settings/AdvancedSettings.h"
#include "utils/URIUtils.h"

using namespace LAUNCHERS;

CROMLauncher::CROMLauncher(std::string strExecutable)
{
  m_strExecutable = strExecutable;
  m_database = new CProgramDatabase();
}

CROMLauncher::~CROMLauncher(void)
{
  delete m_database;
}

bool CROMLauncher::IsSupported()
{
  if (URIUtils::HasExtension(m_strExecutable, ".xbe|cci|cso"))
    return false;

  return URIUtils::HasExtension(m_strExecutable, g_advancedSettings.m_programExtensions);
}

bool CROMLauncher::Launch(bool bLoadSettings, bool bAllowRegionSwitching)
{
  if (!IsSupported())
    return false;

  // TODO: implement ROM launching
  return false;
}
