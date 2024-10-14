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

#include "utils/UrlOptions.h"
#include <string>

class CFileItemList;

namespace XFILE
{
  namespace PROGRAMDATABASEDIRECTORY
  {
    typedef enum _NODE_TYPE
    {
      NODE_TYPE_NONE=0,
      NODE_TYPE_GAMES_OVERVIEW,
      NODE_TYPE_ROOT,
      NODE_TYPE_OVERVIEW,
    } NODE_TYPE;

    typedef struct {
      NODE_TYPE   node;
      std::string id;
      int         label;
    } Node;

    class CDirectoryNode
    {
    public:
      static CDirectoryNode* ParseURL(const std::string& strPath);
      virtual ~CDirectoryNode();

      NODE_TYPE GetType() const;

      bool GetChilds(CFileItemList& items);
      virtual NODE_TYPE GetChildType() const;
      virtual std::string GetLocalizedName() const;

      CDirectoryNode* GetParent() const;

      std::string BuildPath() const;

      virtual bool CanCache() const;
    protected:
      CDirectoryNode(NODE_TYPE Type, const std::string& strName, CDirectoryNode* pParent);
      static CDirectoryNode* CreateNode(NODE_TYPE Type, const std::string& strName, CDirectoryNode* pParent);

      const std::string& GetName() const;
      int GetID() const;
      void RemoveParent();

      virtual bool GetContent(CFileItemList& items) const;


    private:
      NODE_TYPE m_Type;
      std::string m_strName;
      CDirectoryNode* m_pParent;
      CUrlOptions m_options;
    };
  }
}



