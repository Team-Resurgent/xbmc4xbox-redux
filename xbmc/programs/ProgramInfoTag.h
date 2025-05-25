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

#include <string>
#include <vector>

#include "video/VideoInfoTag.h" // CRating
#include "XBDateTime.h"
#include "utils/ScraperUrl.h"
#include "utils/Fanart.h"
#include "utils/IArchivable.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"

class CArchive;
class TiXmlNode;
class TiXmlElement;
class CVariant;

class CProgramInfoTag : public IArchivable, public ISerializable, public ISortable
{
public:
  CProgramInfoTag() { Reset(); };
  void Reset();
  /* \brief Load information to a programinfotag from an XML element
   There are three types of tags supported:
    1. Single-value tags, such as <title>.  These are set if available, else are left untouched.
    2. Additive tags, such as <generalfeature> or <genre>.  These are appended to or replaced (if available) based on the value
       of the prioritise parameter.  In addition, a clear attribute is available in the XML to clear the current value prior
       to appending.
    3. Image tags such as <thumb> and <fanart>.  If the prioritise value is specified, any additional values are prepended
       to the existing values.

   \param element    the root XML element to parse.
   \param append     whether information should be added to the existing tag, or whether it should be reset first.
   \param prioritise if appending, whether additive tags should be prioritised (i.e. replace or prepend) over existing values. Defaults to false.

   \sa ParseNative
   */
  bool Load(const TiXmlElement *element, bool append = false, bool prioritise = false);
  bool Save(TiXmlNode *node, const std::string &tag, bool savePathInfo = true, const TiXmlElement *additionalNode = NULL);
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value) const;
  virtual void ToSortable(SortItem& sortable, Field field) const;
  const CRating GetRating(std::string type = "") const;
  const std::string& GetDefaultRating() const;
  const bool HasYear() const;
  const int GetYear() const;
  const bool HasReleaseDate() const;
  const CDateTime& GetReleaseDate() const;
  bool IsEmpty() const;

  const std::string& GetPath() const
  {
    if (m_strFileNameAndPath.empty())
      return m_strPath;
    return m_strFileNameAndPath;
  };

  void SetBasePath(std::string basePath);
  void SetParentPathID(int parentPathID);
  void SetDeveloper(std::vector<std::string> developer);
  void SetPublisher(std::vector<std::string> publisher);
  void SetGenre(std::vector<std::string> genre);
  void SetDescriptor(std::vector<std::string> descriptor);
  void SetGeneralFeature(std::vector<std::string> generalFeature);
  void SetOnlineFeature(std::vector<std::string> onlineFeature);
  void SetPlatform(std::vector<std::string> platform);
  void SetTags(std::vector<std::string> tags);
  void SetTrailer(std::string trailer);
  void SetPlot(std::string plot);
  void SetPictureURL(CScraperUrl& pictureURL);
  void SetTitle(std::string title);
  void SetFile(std::string file);
  void SetPath(std::string path);
  void SetXBENumber(std::string xbeNumber);
  void SetFileNameAndPath(std::string fileNameAndPath);
  void SetOriginalTitle(std::string originalTitle);
  void SetESRB(std::string esrb);
  void SetSystem(std::string system);
  void SetLastPlayed(CDateTime lastPlayed);
  void SetPlayCount(int playCount);
  void SetReleaseDate(CDateTime year);
  void SetReleaseDateFromDBDate(std::string releaseDateString);
  void SetYear(int year);
  void SetDbId(int dbId);
  void SetFileId(int fileId);
  void SetRating(float rating, int votes, const std::string& type = "", bool def = false);
  void SetRating(CRating rating, const std::string& type = "", bool def = false);
  void SetRating(float rating, const std::string& type = "", bool def = false);
  void RemoveRating(const std::string& type);
  void SetRatings(RatingMap ratings);
  void SetExclusive(bool exclusive);

  std::string m_basePath; // the base path of the program, for folder-based lookups
  int m_parentPathID; // the parent path id where the base path of the program lies
  std::vector<std::string> m_developer;
  std::vector<std::string> m_publisher;
  std::vector<std::string> m_genre;
  std::vector<std::string> m_descriptor;
  std::vector<std::string> m_generalFeature;
  std::vector<std::string> m_onlineFeature;
  std::vector<std::string> m_platform;
  std::vector<std::string> m_tags;
  std::string m_strTrailer;
  std::string m_strPlot;
  std::string m_strTitle;
  std::string m_strFile;
  std::string m_strPath;
  std::string m_strXBENumber; // ID of XBE
  std::string m_strFileNameAndPath;
  std::string m_strOriginalTitle;
  std::string m_strESRB;
  std::string m_strSystem;
  CScraperUrl m_strPictureURL;
  int m_playCount;
  int m_iDbId;
  int m_iFileId;
  int m_iIdRating;
  bool m_bExclusive;
  bool m_bHasReleaseDate;
  CDateTime m_dateAdded;
  CDateTime m_lastPlayed;
  CDateTime m_releaseDate;
  RatingMap m_ratings;
  CFanart m_fanart;
  MediaType m_type;

private:
  /* \brief Parse our native XML format for video info.
   See Load for a description of the available tag types.

   \param element    the root XML element to parse.
   \param prioritise whether additive tags should be replaced (or prepended) by the content of the tags, or appended to.
   \sa Load
   */
  void ParseNative(const TiXmlElement* element, bool prioritise);

  std::string m_strDefaultRating;
  std::string Trim(std::string &value);
  std::vector<std::string> Trim(std::vector<std::string> &items);
};

typedef std::vector<CProgramInfoTag> VECPROGRAMS;
