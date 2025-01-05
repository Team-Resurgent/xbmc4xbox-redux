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

#include "ProgramThumbLoader.h"

#include "FileItem.h"
#include "filesystem/DirectoryCache.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "TextureCache.h"
#include "programs/ProgramDatabase.h"
#include "programs/ProgramInfoTag.h"

using namespace XFILE;

CProgramThumbLoader::CProgramThumbLoader()
{
  m_programDatabase = new CProgramDatabase();
}

CProgramThumbLoader::~CProgramThumbLoader()
{
  StopThread();
  delete m_programDatabase;
}

bool CProgramThumbLoader::LoadItem(CFileItem *pItem)
{
  // TODO: check and re-add FillThumb and GetLocalThumb if needed
  bool result  = LoadItemCached(pItem);
       result |= LoadItemLookup(pItem);

  return result;
}

bool CProgramThumbLoader::LoadItemCached(CFileItem *pItem)
{
  if (pItem->IsParentFolder())
    return false;

  m_programDatabase->Open();

  // program db items normally have info in the database
  if (pItem->HasProgramInfoTag() && !pItem->HasArt("thumb"))
  {
    FillLibraryArt(*pItem);

    if (!pItem->GetProgramInfoTag()->m_type.empty()          &&
         pItem->GetProgramInfoTag()->m_type != MediaTypeGame &&
         pItem->GetProgramInfoTag()->m_type != MediaTypeApp)
    {
      m_programDatabase->Close();
      return true; // nothing else to be done
    }
  }

  // if we have no art, look for it all
  std::map<std::string, std::string> artwork = pItem->GetArt();
  if (artwork.empty())
  {
    std::vector<std::string> artTypes = GetArtTypes(pItem->HasProgramInfoTag() ? pItem->GetProgramInfoTag()->m_type : "");
    if (find(artTypes.begin(), artTypes.end(), "thumb") == artTypes.end())
      artTypes.push_back("thumb"); // always look for "thumb" art for files
    for (std::vector<std::string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
    {
      std::string type = *i;
      std::string art = GetCachedImage(*pItem, type);
      if (!art.empty())
        artwork.insert(std::make_pair(type, art));
    }
    SetArt(*pItem, artwork);
  }

  m_programDatabase->Close();

  return true;
}

bool CProgramThumbLoader::LoadItemLookup(CFileItem *pItem)
{
  if (pItem->m_bIsShareOrDrive || pItem->IsParentFolder() || pItem->GetPath() == "add")
    return false;

  if (pItem->HasProgramInfoTag()                          &&
     !pItem->GetProgramInfoTag()->m_type.empty()          &&
      pItem->GetProgramInfoTag()->m_type != MediaTypeGame &&
      pItem->GetProgramInfoTag()->m_type != MediaTypeApp)
    return false; // Nothing to do here

  m_programDatabase->Open();

  std::map<std::string, std::string> artwork = pItem->GetArt();
  std::vector<std::string> artTypes = GetArtTypes(pItem->HasProgramInfoTag() ? pItem->GetProgramInfoTag()->m_type : "");
  if (find(artTypes.begin(), artTypes.end(), "thumb") == artTypes.end())
    artTypes.push_back("thumb"); // always look for "thumb" art for files
  for (std::vector<std::string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
  {
    std::string type = *i;
    if (!pItem->HasArt(type))
    {
      std::string art = GetLocalArt(*pItem, type, type=="fanart");
      if (!art.empty()) // cache it
      {
        SetCachedImage(*pItem, type, art);
        CTextureCache::Get().BackgroundCacheImage(art);
        artwork.insert(std::make_pair(type, art));
      }
    }
  }
  SetArt(*pItem, artwork);

  // TODO: if no thumb, try extracting it from XBE
  // TODO: look for missing data like Title ID, region etc. and pull it from XBE

  m_programDatabase->Close();
  return true;
}

void CProgramThumbLoader::SetArt(CFileItem &item, const std::map<std::string, std::string> &artwork)
{
  item.SetArt(artwork);
  if (artwork.find("thumb") == artwork.end())
  { // set fallback for "thumb"
    if (artwork.find("poster") != artwork.end())
      item.SetArtFallback("thumb", "poster");
    else if (artwork.find("banner") != artwork.end())
      item.SetArtFallback("thumb", "banner");
  }
}

bool CProgramThumbLoader::FillLibraryArt(CFileItem &item)
{
  CProgramInfoTag &tag = *item.GetProgramInfoTag();
  if (tag.m_iDbId > -1 && !tag.m_type.empty())
  {
    std::map<std::string, std::string> artwork;
    m_programDatabase->Open();
    if (m_programDatabase->GetArtForItem(tag.m_iDbId, tag.m_type, artwork))
      SetArt(item, artwork);

    m_programDatabase->Close();
  }
  return !item.GetArt().empty();
}

std::vector<std::string> CProgramThumbLoader::GetArtTypes(const std::string &type)
{
  std::vector<std::string> ret;
  if (type == MediaTypeProgram  ||
      type == MediaTypeGame     ||
      type == MediaTypeApp      ||
      type.empty())
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
