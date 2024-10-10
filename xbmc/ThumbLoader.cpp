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
#ifdef HAS_ADVANCED_PROGRAMS_LIBRARY
#include "filesystem/DirectoryCache.h"
#include "settings/AdvancedSettings.h"
#endif
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

CProgramThumbLoader::CProgramThumbLoader()
{
}

CProgramThumbLoader::~CProgramThumbLoader()
{
}

bool CProgramThumbLoader::LoadItem(CFileItem *pItem)
{
  bool result  = LoadItemCached(pItem);
       result |= LoadItemLookup(pItem);

  return result;
}

bool CProgramThumbLoader::LoadItemCached(CFileItem *pItem)
{
  if (pItem->IsParentFolder())
    return false;

  return FillThumb(*pItem);
}

bool CProgramThumbLoader::LoadItemLookup(CFileItem *pItem)
{
  return false;
}

bool CProgramThumbLoader::FillThumb(CFileItem &item)
{
  // no need to do anything if we already have a thumb set
  std::string thumb = item.GetArt("thumb");

  if (thumb.empty())
  { // see whether we have a cached image for this item
    thumb = GetCachedImage(item, "thumb");
    if (thumb.empty())
    {
      thumb = GetLocalThumb(item);
      if (!thumb.empty())
        SetCachedImage(item, "thumb", thumb);
    }
  }

  if (!thumb.empty())
  {
    CTextureCache::Get().BackgroundCacheImage(thumb);
    item.SetArt("thumb", thumb);
  }
  return true;
}

std::string CProgramThumbLoader::GetLocalThumb(const CFileItem &item)
{
  if (item.IsAddonsPath())
    return "";

  // look for the thumb
  if (item.m_bIsFolder)
  {
    std::string folderThumb = item.GetFolderThumb();
    if (CFile::Exists(folderThumb))
      return folderThumb;
  }
#ifdef _XBOX
  // look for the thumb
  else if (item.IsShortCut())
  {
    CShortcut shortcut;
    if (shortcut.Create(item.GetPath()))
    {
      // use the shortcut's thumb
      if (!shortcut.m_strThumb.empty())
        return shortcut.m_strThumb;
      else
      {
        CFileItem cut(shortcut.m_strPath,false);
        CProgramThumbLoader loader;
        if (loader.LoadItem(&cut))
          return cut.GetArt("thumb");
      }
    }
  }
  else if (item.IsXBE())
  {
    std::string directory = URIUtils::GetDirectory(item.GetPath());
    std::string icon = URIUtils::AddFileToFolder(directory, "avalaunch_icon.jpg");

    // first check for avalaunch_icon.jpg
    if (CFile::Exists(icon) || CUtil::CacheXBEIcon(item.GetPath(), icon))
    {
      CFileItem item(icon,false);
      CProgramThumbLoader loader;
      if (loader.LoadItem(&item))
        return item.GetArt("thumb");
    }
  }
#endif
  else
  {
    std::string fileThumb(item.GetTBNFile());
    if (CFile::Exists(fileThumb))
      return fileThumb;
  }
  return "";
}

#ifdef HAS_ADVANCED_PROGRAMS_LIBRARY
std::vector<std::string> CProgramThumbLoader::GetArtTypes(const std::string &type)
{
  std::vector<std::string> ret;
  if (type == MediaTypeGame || type.empty())
  {
    ret.push_back("alt_synopsis");
    ret.push_back("banner");
    ret.push_back("cd");
    ret.push_back("cd_small");
    ret.push_back("cdposter");
    ret.push_back("dual3d");
    ret.push_back("fanart_blur");
    ret.push_back("fanart");
    ret.push_back("fanart_thumb");
    ret.push_back("fog");
    ret.push_back("icon");
    ret.push_back("opencase");
    ret.push_back("poster");
    ret.push_back("poster_small");
    ret.push_back("poster_small_blurred");
    ret.push_back("synopsis");
    ret.push_back("thumb");
  }
  return ret;
}

std::string CProgramThumbLoader::GetLocalArt(const CFileItem &item, const std::string &type, bool checkFolder)
{
  if (item.SkipLocalArt())
    return "";

  /* Cache directory for (sub) folders on streamed filesystems. We need to do this
     else entering (new) directories from the app thread becomes much slower. This
     is caused by the fact that Curl Stat/Exist() is really slow and that the
     thumbloader thread accesses the streamed filesystem at the same time as the
     App thread and the latter has to wait for it.
   */
  if (item.m_bIsFolder && (item.IsInternetStream(true) || g_advancedSettings.m_cacheBufferMode == CACHE_BUFFER_MODE_ALL))
  {
    CFileItemList items; // Dummy list
    CDirectory::GetDirectory(item.GetPath(), items, "", DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
  }

  std::string art;
  if (!type.empty())
  {
    // Look for XBMC4Gamers artwork
    art = item.FindLocalArt("_resources\\artwork\\" + type + ".jpg", checkFolder);
    if (art.empty())
      art = item.FindLocalArt("_resources\\artwork\\" + type + ".png", checkFolder);
    if (!art.empty())
      return art;

    art = item.FindLocalArt(type + ".jpg", checkFolder);
    if (art.empty())
      art = item.FindLocalArt(type + ".png", checkFolder);
  }
  return art;
}
#endif
