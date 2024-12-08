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

#include "GUIDialogProgramSettings.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "programs/ProgramDatabase.h"
#include "profiles/ProfilesManager.h"
#include "settings/lib/Setting.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "utils/XMLUtils.h"
#include "Util.h"

using namespace std;

CGUIDialogProgramSettings::CGUIDialogProgramSettings(void)
    : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PROGRAM_SETTINGS, "DialogSettings.xml"),
      m_iTitleId(0)
{
  m_strExecutable.clear();
}

CGUIDialogProgramSettings::~CGUIDialogProgramSettings(void)
{
}

bool CGUIDialogProgramSettings::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      Reset();
      break;
    }

    default:
      break;
  }

  return CGUIDialogSettingsManualBase::OnMessage(message);
}

void CGUIDialogProgramSettings::Reset()
{
  m_iTitleId = 0;
  m_strExecutable.clear();
}

void CGUIDialogProgramSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  // TODO: update changed setting
}

void CGUIDialogProgramSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetHeading(38996);

  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CANCEL_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 518);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CUSTOM_BUTTON, 190);
}

void CGUIDialogProgramSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  // TODO: handle setting action
}

void CGUIDialogProgramSettings::Save()
{
  if (CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE &&
      !g_passwordManager.CheckSettingLevelLock(::SettingLevelExpert))
    return;

  // TODO: save changed settings
}

void CGUIDialogProgramSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("xbelauncher", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogProgramSettings: unable to setup xbelauncher");
    return;
  }

  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogProgramSettings: unable to setup xbelauncher");
    return;
  }

  // TODO: initialize settings
}

void CGUIDialogProgramSettings::ShowForTitle(const CFileItemPtr pItem)
{
  CGUIDialogProgramSettings *dialog = (CGUIDialogProgramSettings *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRAM_SETTINGS);
  if (dialog == nullptr)
    return;

  // initialize and show the dialog
  dialog->Initialize();
  dialog->SetExecutable(pItem->HasProgramInfoTag() ? pItem->GetProgramInfoTag()->m_strFileNameAndPath : pItem->GetPath());
  dialog->Open();
}

