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

#include "ThumbLoader.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "settings/Settings.h"
#include "TextureCache.h"
#include "Shortcut.h"
#include "Util.h"
#include "utils/URIUtils.h"

using namespace XFILE;

CThumbLoader::CThumbLoader() :
  CBackgroundInfoLoader()
{
  m_textureDatabase = new CTextureDatabase();
}

CThumbLoader::~CThumbLoader()
{
  delete m_textureDatabase;
}

void CThumbLoader::OnLoaderStart()
{
  m_textureDatabase->Open();
}

void CThumbLoader::OnLoaderFinish()
{
  m_textureDatabase->Close();
}

std::string CThumbLoader::GetCachedImage(const CFileItem &item, const std::string &type)
{
  if (!item.GetPath().empty() && m_textureDatabase->Open())
  {
    std::string image = m_textureDatabase->GetTextureForPath(item.GetPath(), type);
    m_textureDatabase->Close();
    return image;
  }
  return "";
}

void CThumbLoader::SetCachedImage(const CFileItem &item, const std::string &type, const std::string &image)
{
  if (!item.GetPath().empty() && m_textureDatabase->Open())
  {
    m_textureDatabase->SetTextureForPath(item.GetPath(), type, image);
    m_textureDatabase->Close();
  }
}
