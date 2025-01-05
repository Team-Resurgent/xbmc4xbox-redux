/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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

#include "ContextMenus.h"
#include "Application.h"
#include "addons/Addon.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/AddonsDirectory.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "guilib/GUIWindowManager.h"
#include "programs/dialogs/GUIDialogProgramInfo.h"
#include "programs/dialogs/GUIDialogProgramSettings.h"
#include "programs/windows/GUIWindowProgramBase.h"
#include "utils/URIUtils.h"


namespace CONTEXTMENU
{

CProgramInfo::CProgramInfo()
    : CStaticContextMenuAction(19033) {}

bool CProgramInfo::IsVisible(const CFileItem& item) const
{
  if (!item.HasProgramInfoTag())
    return false;

  return item.GetProgramInfoTag()->m_type == MediaTypeProgram ||
         item.GetProgramInfoTag()->m_type == MediaTypeGame ||
         item.GetProgramInfoTag()->m_type == MediaTypeApp;
}

bool CProgramInfo::Execute(const CFileItemPtr& item) const
{
  CGUIDialogProgramInfo::ShowFor(*item);
  return true;
}

std::string CLaunchCustom::GetLabel(const CFileItem& item) const
{
  return g_localizeStrings.Get(38996);
}

bool CLaunchCustom::IsVisible(const CFileItem& item) const
{
  if (item.m_bIsFolder)
    return false; //! @todo implement

  return item.IsProgram();
}

bool CLaunchCustom::Execute(const CFileItemPtr& item) const
{
  CGUIDialogProgramSettings::ShowForTitle(item);
  return true;
};

std::string CScriptLaunch::GetLabel(const CFileItem& item) const
{
  return g_localizeStrings.Get(247);
}

bool CScriptLaunch::IsVisible(const CFileItem& item) const
{
  if (item.m_bIsFolder)
    return URIUtils::IsDOSPath(item.GetPath());

  return item.IsProgram();
}

bool CScriptLaunch::Execute(const CFileItemPtr& item) const
{
  ADDON::VECADDONS addons;
  if (XFILE::CAddonsDirectory::GetScriptsAndPlugins("executable", addons) && addons.size())
  {
    CGUIDialogSelect *dialog = static_cast<CGUIDialogSelect*>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
    if (dialog)
    {
      dialog->SetHeading(247);
      dialog->Reset();
      for (ADDON::VECADDONS::const_iterator it = addons.begin(); it != addons.end(); ++it)
      {
        std::string strOption = StringUtils::Format("%s (%s)", (*it)->Name().c_str(), (*it)->Author().c_str());
        dialog->Add(strOption);
      }
      dialog->Open();

      int iSelected = dialog->GetSelectedItem();
      if (!dialog->IsConfirmed() || iSelected < 0)
        return true;

      std::string strPath = item->IsProgramDb() && item->HasProgramInfoTag() ? item->GetProgramInfoTag()->m_strFileNameAndPath : item->GetPath();
      std::string strParentPath = item->m_bIsFolder ? item->GetPath() : URIUtils::GetParentPath(strPath);

      std::vector<std::string> argv;
      argv.push_back(strPath);
      argv.push_back(strParentPath);

      ADDON::AddonPtr addon = addons[iSelected];
      CScriptInvocationManager::GetInstance().ExecuteAsync(addon->LibPath(), addon, argv);

      return true;
    }
  }

  CGUIDialogKaiToast::QueueNotification(StringUtils::Format(g_localizeStrings.Get(13328).c_str(), g_localizeStrings.Get(247).c_str()), g_localizeStrings.Get(161));
  return false;
};
}
