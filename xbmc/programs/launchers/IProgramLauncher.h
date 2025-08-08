#pragma once
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

class IProgramLauncher
{
public:
  virtual ~IProgramLauncher() {}

  /*!
  \brief Launches the specified program.

  \param bLoadSettings specify if launcher should load custom settings
  \param bAllowRegionSwitching specify if launcher is allowed to force region
  \return Returns true if the program was successfully launched, false otherwise.
  */
  virtual bool Launch(bool bLoadSettings, bool bAllowRegionSwitching) = 0;

protected:
  /*!
  \brief Loads program settings stored in the database.

  Derived classes should implement this method if they support custom settings.
  */
  virtual bool LoadSettings() { return false; };

private:
  /*!
  \brief Checks if the given executable is supported.

  \return Returns true if the given executable is compatible with the launcher, false otherwise.
  */
  virtual bool IsSupported() = 0;
};
