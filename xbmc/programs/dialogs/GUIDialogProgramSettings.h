#pragma once

/*
 *      Copyright (C) 2005-2014 Team XBMC
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

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CSetting;
class CTrainer;

typedef struct SProgramSettings
{
  SProgramSettings() { Reset(); }
  std::string strExecutable;  /* which executable to launch */
  std::string strEmulator; /* path to emulator */
  int iForceRegion; /* force game region */
  void Reset() { strExecutable.clear(); strEmulator.clear(); iForceRegion = 0; }
} SProgramSettings;

class CGUIDialogProgramSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogProgramSettings();
  virtual ~CGUIDialogProgramSettings();
  virtual bool OnMessage(CGUIMessage &message);

  /*! \brief retrieve settings of a given executable
   \param strExecutable the absolute path to program executable
   \param programSettings the structure in which settings will be loaded
   */
  static void LoadSettings(const std::string& strExecutable, SProgramSettings& programSettings);

  /*! \brief save settings of a given executable
   \param strExecutable the absolute path to program executable
   \param settings settings which needs saving
   */
  static void SaveSettings(const std::string& strExecutable, const SProgramSettings& settings);

  /*! \brief retrieve region of a given executable
   \param strExecutable the absolute path to program executable
   \param forceAllRegions force all regions
   */
  static int GetXBERegion(const std::string& strExecutable, bool forceAllRegions = false);

  static void ShowForTitle(const CFileItemPtr pItem);

protected:
  static void IntegerOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  static void StringOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);

  // implementations of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);

  // specialization of CGUIDialogSettingsManualBase
  virtual void SetupView();

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual void Save();

  // specialization of CGUIDialogSettingsManualBase
  virtual void InitializeSettings();

  void SetExecutable(const std::string strExecutable) { m_strExecutable = strExecutable; }

private:
  void Reset();
  void ResetTrainer(bool bClearTrainers = false);

  void LoadProgramSettings();
  void SaveProgramSettings();

  std::vector<CTrainer*> m_trainers;
  std::vector<std::string> m_trainerOptions;
  std::vector<std::string> m_selectedTrainerOptions;

  CTrainer* m_trainer;
  unsigned int m_iTitleId;
  std::string m_strExecutable;
  SProgramSettings m_settings;
};

