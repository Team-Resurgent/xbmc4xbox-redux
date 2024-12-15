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
#include "FileItem.h"
#include "Shortcut.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "programs/ProgramDatabase.h"
#include "settings/AdvancedSettings.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "Util.h"

using namespace LAUNCHERS;

#define CUSTOM_LAUNCH "special://temp/emu_launch.xbe"

SystemMapping Systems[] = {
                            {"Nintendo Entertainment System",         "nes",                  ".zip|.nes"},
                            {"Sega Master System",                    "mastersystem",         ".zip|.sms"},
                            {"Sega Megadrive",                        "megadrive",            ".zip|.md" },
                            {"Super Nintendo Entertainment System",   "snes",                 ".zip|.sfc"}
                          };

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

bool CROMLauncher::FindEmulators(CFileItemList& emulators)
{
  std::vector<std::string> vecSystems;
  for (unsigned int i = 0; i < sizeof(Systems) / sizeof(SystemMapping); ++i)
  {
    if (URIUtils::HasExtension(m_strExecutable, Systems[i].extension))
      vecSystems.push_back(Systems[i].shortname);
  }

  return m_database->GetEmulators(vecSystems, emulators);
}

bool CROMLauncher::Launch(bool bLoadSettings, bool bAllowRegionSwitching)
{
  if (!m_database->Open())
    return false;

  if (!IsSupported())
    return false;

  // get emualators for this ROM
  CFileItemList emulators;
  if (!FindEmulators(emulators) || emulators.IsEmpty())
  {
    CLog::Log(LOGINFO, "Emulator for %s is not installed", m_strExecutable.c_str());
    return false;
  }

  // if there is more then one, let user to choose one
  CFileItemPtr emulator;
  if (emulators.Size() > 1)
  {
    CGUIDialogSelect *dialog = static_cast<CGUIDialogSelect*>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
    dialog->Reset();
    dialog->SetHeading(22080);
    dialog->SetItems(emulators);
    dialog->Open();
    if (dialog->GetSelectedItem() < 0)
      return false;
    emulator = dialog->GetSelectedFileItem();
  }
  else
    emulator = emulators[0];

  // Launch ROM
  CShortcut shortcut;
  shortcut.m_strPath = emulator->GetProgramInfoTag()->m_strFileNameAndPath.c_str();
  shortcut.m_strCustomGame = m_strExecutable.c_str();
  shortcut.Save(CUSTOM_LAUNCH);
  CUtil::RunShortcut(CUSTOM_LAUNCH);
  return true;
}
