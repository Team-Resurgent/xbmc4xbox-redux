#pragma once
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>

#include "dbwrappers/Database.h"
#include "ProgramInfoTag.h"

namespace dbiplus
{
  class field_value;
  typedef std::vector<field_value> sql_record;
}

typedef std::vector<CProgramInfoTag> VECGAMES;

typedef enum
{
  PROGRAMDB_CONTENT_GAMES = 1
} PROGRAMDB_CONTENT_TYPE;

class CProgramDatabase : public CDatabase
{
public:
  CProgramDatabase(void);
  virtual ~CProgramDatabase(void);

  virtual bool Open();
  virtual bool CommitTransaction();

  bool HasContent();
  bool HasContent(PROGRAMDB_CONTENT_TYPE type);
};
