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

#include <vector>

#include "utils/IArchivable.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"

class CProgramInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
  CProgramInfoTag() { Reset(); };
  void Reset();
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value) const;
  virtual void ToSortable(SortItem& sortable, Field field) const;

  const std::string& GetPath() const
  {
    if (m_strFileNameAndPath.empty())
      return m_strPath;
    return m_strFileNameAndPath;
  };

  std::string m_strPath;
  std::string m_strFileNameAndPath;
  int m_iIdRating;
  MediaType m_type;
};

typedef std::vector<CProgramInfoTag> VECPROGRAMS;
