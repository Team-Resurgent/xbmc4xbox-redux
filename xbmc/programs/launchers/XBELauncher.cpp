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

#include "XBELauncher.h"
#include "programs/dialogs/GUIDialogProgramSettings.h"
#include "programs/ProgramDatabase.h"
#include "utils/URIUtils.h"

using namespace LAUNCHERS;

CXBELauncher::CXBELauncher(std::string strExecutable)
{
  m_strExecutable = strExecutable;
  m_database = new CProgramDatabase();
  m_settings = new SProgramSettings();
}

CXBELauncher::~CXBELauncher(void)
{
  delete m_database;
  delete m_settings;
}

bool CXBELauncher::LoadSettings()
{
  return false;
}

bool CXBELauncher::IsSupported()
{
  return URIUtils::HasExtension(m_strExecutable, ".xbe");
}

bool CXBELauncher::Launch()
{
  if (!IsSupported())
    return false;

  // TODO: implement launching of this executable (support for trainers, flicke filter etc.)
  return false;
}
