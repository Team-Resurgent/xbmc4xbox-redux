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
#include "Shortcut.h"
#include "filesystem/File.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "programs/dialogs/GUIDialogProgramSettings.h"
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
  m_settings = new SProgramSettings();
}

CROMLauncher::~CROMLauncher(void)
{
  delete m_database;
  delete m_settings;
}

bool CROMLauncher::LoadSettings()
{
  CGUIDialogProgramSettings::LoadSettings(m_strExecutable, *m_settings);
  return true;
}

bool CROMLauncher::IsSupported()
{
  if (URIUtils::HasExtension(m_strExecutable, ".xbe|cci|cso"))
    return false;

  return URIUtils::HasExtension(m_strExecutable, g_advancedSettings.m_programExtensions);
}

bool CROMLauncher::GetDefaultEmulator(CFileItemPtr& emulator)
{
  if (m_settings->strEmulator.empty())
    return false;

  if (!XFILE::CFile::Exists(m_settings->strEmulator))
    return false;

  emulator = CFileItemPtr(new CFileItem(m_settings->strEmulator));
  return true;
}

bool CROMLauncher::FindEmulators(const std::string strRomFile, CFileItemList& emulators)
{
  CProgramDatabase database;
  if (!database.Open())
    return false;

  std::vector<std::string> vecSystems;
  for (unsigned int i = 0; i < sizeof(Systems) / sizeof(SystemMapping); ++i)
  {
    if (URIUtils::HasExtension(strRomFile, Systems[i].extension))
      vecSystems.push_back(Systems[i].shortname);
  }

  return database.GetEmulators(vecSystems, emulators);
}

bool CROMLauncher::Launch(bool bLoadSettings, bool bAllowRegionSwitching)
{
  if (!m_database->Open())
    return false;

  if (bLoadSettings)
    LoadSettings();

  if (!IsSupported())
    return false;

  // Get emulator for this ROM
  CFileItemPtr emulator;
  if (!GetDefaultEmulator(emulator))
  { // no default, look for available emulators
    CFileItemList emulators;
    if (!FindEmulators(m_strExecutable, emulators) || emulators.IsEmpty())
    {
      CLog::Log(LOGINFO, "Emulator for %s is not installed", m_strExecutable.c_str());
      return false;
    }

    // if there is more then one, let user to choose one
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

    m_settings->strEmulator = emulator->GetProgramInfoTag()->m_strFileNameAndPath;
    CGUIDialogProgramSettings::SaveSettings(m_strExecutable, *m_settings);
  }

  std::string strExecutable = m_strExecutable;

  // look for default executable
  if (!m_settings->strExecutable.empty())
  {
    std::string strParentPath = URIUtils::GetParentPath(m_strExecutable);
    strExecutable = URIUtils::AddFileToFolder(strParentPath, m_settings->strExecutable);
  }

  m_database->UpdateLastPlayed(m_strExecutable);

  // Launch ROM
  CShortcut shortcut;
  shortcut.m_strPath = m_settings->strEmulator.c_str();
  shortcut.m_strCustomGame = strExecutable.c_str();
  shortcut.Save(CUSTOM_LAUNCH);
  CUtil::RunShortcut(CUSTOM_LAUNCH);
  return true;
}
