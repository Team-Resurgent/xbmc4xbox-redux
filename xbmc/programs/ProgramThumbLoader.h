#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "ThumbLoader.h"

class CProgramThumbLoader : public CThumbLoader
{
public:
  CProgramThumbLoader();
  virtual ~CProgramThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);
  virtual bool LoadItemCached(CFileItem* pItem);
  virtual bool LoadItemLookup(CFileItem* pItem);

  /*! \brief Fill the thumb of a programs item
   First uses a cached thumb from a previous run, then checks for a local thumb
   and caches it for the next run
   \param item the CFileItem object to fill
   \return true if we fill the thumb, false otherwise
   \sa GetLocalThumb
   */
  virtual bool FillThumb(CFileItem &item);

  /*! \brief Get a local thumb for a programs item
   Shortcuts are checked, then we check for a file or folder thumb
   \param item the CFileItem object to check
   \return the local thumb (if it exists)
   \sa FillThumb
   */
  static std::string GetLocalThumb(const CFileItem &item);

  /*! \brief Find a particular art type for a given item, optionally checking at the folder level
   \param item the CFileItem to search.
   \param type the type of art to look for.
   \param checkFolder whether to also check the folder level for files. Defaults to false.
   \return the art file (if found), else empty.
   */
  static std::string GetLocalArt(const CFileItem &item, const std::string &type, bool checkFolder = false);

  /*! \brief return the available art types for a given media type
   \param type the type of media.
   \return a vector of art types.
   \sa GetLocalArt
   */
  static std::vector<std::string> GetArtTypes(const std::string &type);
};
