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

#include "ProgramInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "utils/XMLUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/Archive.h"

void CProgramInfoTag::Reset()
{
  m_developer.clear();
  m_publisher.clear();
  m_genre.clear();
  m_descriptor.clear();
  m_generalFeature.clear();
  m_onlineFeature.clear();
  m_platform.clear();
  m_tags.clear();
  m_strTrailer.clear();
  m_strPlot.clear();
  m_strTitle.clear();
  m_strFile.clear();
  m_strPath.clear();
  m_strXBENumber.clear();
  m_strFileNameAndPath.clear();
  m_strOriginalTitle.clear();
  m_strESRB.clear();
  m_strSystem.clear();
  m_strPictureURL.Clear();
  m_strDefaultRating = "default";
  m_playCount = 0;
  m_iDbId = -1;
  m_iFileId = -1;
  m_iIdRating = -1;
  m_bExclusive = false;
  m_bHasReleaseDate = false;
  m_dateAdded.Reset();
  m_lastPlayed.Reset();
  m_releaseDate.Reset();
  m_ratings.clear();
  m_fanart.m_xml.clear();
  m_type.clear();
}

bool CProgramInfoTag::Save(TiXmlNode *node, const std::string &tag, bool savePathInfo, const TiXmlElement *additionalNode)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement programElement(tag.c_str());
  TiXmlNode *program = node->InsertEndChild(programElement);

  if (!program) return false;

  if (savePathInfo)
  {
    XMLUtils::SetString(program, "file", m_strFile);
    XMLUtils::SetString(program, "path", m_strPath);
    XMLUtils::SetString(program, "filenameandpath", m_strFileNameAndPath);
    XMLUtils::SetString(program, "basepath", m_basePath);
  }

  XMLUtils::SetStringArray(program, "developer", m_developer);
  XMLUtils::SetStringArray(program, "publisher", m_publisher);
  XMLUtils::SetStringArray(program, "genre", m_genre);
  XMLUtils::SetStringArray(program, "descriptor", m_descriptor);
  XMLUtils::SetStringArray(program, "generalfeature", m_generalFeature);
  XMLUtils::SetStringArray(program, "onlinefeature", m_onlineFeature);
  XMLUtils::SetStringArray(program, "platform", m_platform);
  XMLUtils::SetStringArray(program, "tag", m_tags);

  if (!m_type.empty())
    XMLUtils::SetString(program, "type", m_type);
  if (!m_strTrailer.empty())
    XMLUtils::SetString(program, "trailer", m_strTrailer);
  if (!m_strPlot.empty())
    XMLUtils::SetString(program, "plot", m_strPlot);
  XMLUtils::SetString(program, "title", m_strTitle);
  if (!m_strXBENumber.empty())
    XMLUtils::SetString(program, "titleid", m_strXBENumber);
  if (!m_strOriginalTitle.empty())
    XMLUtils::SetString(program, "originaltitle", m_strOriginalTitle);
  if (!m_strESRB.empty())
    XMLUtils::SetString(program, "esrb", m_strESRB);
  if (!m_strSystem.empty())
    XMLUtils::SetString(program, "system", m_strSystem);
  if (!m_strPictureURL.m_xml.empty())
  {
    CXBMCTinyXML doc;
    doc.Parse(m_strPictureURL.m_xml);
    const TiXmlNode* thumb = doc.FirstChild("thumb");
    while (thumb)
    {
      program->InsertEndChild(*thumb);
      thumb = thumb->NextSibling("thumb");
    }
  }

  XMLUtils::SetInt(program, "playcount", m_playCount);
  XMLUtils::SetBoolean(program, "exclusive", m_bExclusive);

  XMLUtils::SetDateTime(program, "dateadded", m_dateAdded);
  XMLUtils::SetDate(program, "lastplayed", m_lastPlayed);
  XMLUtils::SetDate(program, "releasedate", m_releaseDate);

  if (!m_ratings.empty())
  {
    TiXmlElement ratings("ratings");
    for (RatingMap::const_iterator iterator = m_ratings.begin(); iterator != m_ratings.end(); ++iterator)
    {
      const std::pair<const std::string, CRating> &it = *iterator;
      TiXmlElement rating("rating");
      rating.SetAttribute("name", it.first.c_str());
      XMLUtils::SetFloat(&rating, "value", it.second.rating);
      XMLUtils::SetInt(&rating, "votes", it.second.votes);
      rating.SetAttribute("max", 10);
      if (it.first == m_strDefaultRating)
        rating.SetAttribute("default", "true");
      ratings.InsertEndChild(rating);
    }
    program->InsertEndChild(ratings);
  }

  if (m_fanart.m_xml.size())
  {
    CXBMCTinyXML doc;
    doc.Parse(m_fanart.m_xml);
    program->InsertEndChild(*doc.RootElement());
  }

  return true;
}

bool CProgramInfoTag::Load(const TiXmlElement *element, bool append, bool prioritise)
{
  if (!element)
    return false;
  if (!append)
    Reset();
  ParseNative(element, prioritise);
  return true;
}

void CProgramInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_developer;
    ar << m_publisher;
    ar << m_genre;
    ar << m_descriptor;
    ar << m_generalFeature;
    ar << m_onlineFeature;
    ar << m_platform;
    ar << m_tags;
    ar << m_strTrailer;
    ar << m_strPlot;
    ar << m_strTitle;
    ar << m_strFile;
    ar << m_strPath;
    ar << m_strXBENumber;
    ar << m_strFileNameAndPath;
    ar << m_strOriginalTitle;
    ar << m_strESRB;
    ar << m_strSystem;
    ar << m_strPictureURL.m_spoof;
    ar << m_strPictureURL.m_xml;
    ar << m_playCount;
    ar << m_iDbId;
    ar << m_iFileId;
    ar << m_bExclusive;
    ar << m_bHasReleaseDate;
    ar << m_dateAdded.GetAsDBDateTime();
    ar << m_lastPlayed;
    ar << m_releaseDate;
    ar << (int)m_ratings.size();
    for (RatingMap::const_iterator it = m_ratings.begin(); it != m_ratings.end(); ++it)
    {
      const std::pair<const std::string, CRating> &i = *it;
      ar << i.first;
      ar << (i.first == m_strDefaultRating);
      ar << i.second.rating;
      ar << i.second.votes;
    }
    ar << m_fanart.m_xml;
    ar << m_type;
  }
  else
  {
    ar >> m_developer;
    ar >> m_publisher;
    ar >> m_genre;
    ar >> m_descriptor;
    ar >> m_generalFeature;
    ar >> m_onlineFeature;
    ar >> m_platform;
    ar >> m_tags;
    ar >> m_strTrailer;
    ar >> m_strPlot;
    ar >> m_strTitle;
    ar >> m_strFile;
    ar >> m_strPath;
    ar >> m_strXBENumber;
    ar >> m_strFileNameAndPath;
    ar >> m_strOriginalTitle;
    ar >> m_strESRB;
    ar >> m_strSystem;
    ar >> m_strPictureURL.m_spoof;
    ar >> m_strPictureURL.m_xml;
    ar >> m_playCount;
    ar >> m_iDbId;
    ar >> m_iFileId;
    ar >> m_bExclusive;
    ar >> m_bHasReleaseDate;

    std::string dbDateTime;
    ar >> dbDateTime;
    m_dateAdded.SetFromDBDateTime(dbDateTime);

    ar >> m_lastPlayed;
    ar >> m_releaseDate;

    int iRatingSize;
    ar >> iRatingSize;
    for (int i = 0; i < iRatingSize; ++i)
    {
      CRating rating;
      std::string name;
      bool defaultRating;
      ar >> name;
      ar >> defaultRating;
      ar >> rating.rating;
      ar >> rating.votes;
      SetRating(rating, name);
      if (defaultRating)
        m_strDefaultRating = name;
    }

    ar >> m_fanart.m_xml;
    ar >> m_type;
  }
}

void CProgramInfoTag::Serialize(CVariant& value) const
{
  value["developer"] = m_developer;
  value["publisher"] = m_publisher;
  value["genre"] = m_genre;
  value["descriptor"] = m_descriptor;
  value["generalfeature"] = m_generalFeature;
  value["onlinefeature"] = m_onlineFeature;
  value["platform"] = m_platform;
  value["tag"] = m_tags;
  value["trailer"] = m_strTrailer;
  value["plot"] = m_strPlot;
  value["title"] = m_strTitle;
  value["file"] = m_strFile;
  value["path"] = m_strPath;
  value["titleid"] = m_strXBENumber;
  value["filenameandpath"] = m_strFileNameAndPath;
  value["originaltitle"] = m_strOriginalTitle;
  value["esrb"] = m_strESRB;
  value["system"] = m_strSystem;
  value["playcount"] = m_playCount;
  value["dbid"] = m_iDbId;
  value["fileid"] = m_iFileId;
  value["exclusive"] = m_bExclusive;
  value["dateadded"] = m_dateAdded.IsValid() ? m_dateAdded.GetAsDBDateTime() : StringUtils::Empty;
  value["lastplayed"] = m_lastPlayed.IsValid() ? m_lastPlayed.GetAsDBDate() : StringUtils::Empty;
  value["releasedate"] = m_releaseDate.IsValid() ? m_releaseDate.GetAsDBDate() : StringUtils::Empty;
  value["year"] = m_releaseDate.GetYear();
  value["rating"] = GetRating().rating;
  CVariant ratings = CVariant(CVariant::VariantTypeObject);
  for (RatingMap::const_iterator it = m_ratings.begin(); it != m_ratings.end(); ++it)
  {
    const std::pair<const std::string, CRating> &i = *it;
    CVariant rating;
    rating["rating"] = i.second.rating;
    rating["votes"] = i.second.votes;
    rating["default"] = i.first == m_strDefaultRating;

    ratings[i.first] = rating;
  }
  value["ratings"] = ratings;
  value["type"] = m_type;
}

void CProgramInfoTag::ToSortable(SortItem& sortable, Field field) const
{
  switch (field)
  {
  case FieldTitle:
  {
    // make sure not to overwrite an existing title with an empty one
    std::string title = m_strTitle;
    if (!title.empty() || sortable.find(FieldTitle) == sortable.end())
      sortable[FieldTitle] = title;
    break;
  }
  case FieldPlaycount:                sortable[FieldPlaycount] = m_playCount; break;
  case FieldYear:                     sortable[FieldYear] = m_releaseDate.GetYear(); break;
  case FieldRating:                   sortable[FieldRating] = GetRating().rating; break;
  case FieldDateAdded:                sortable[FieldDateAdded] = m_dateAdded.IsValid() ? m_dateAdded.GetAsDBDateTime() : StringUtils::Empty; break;
  default: break;
  }
}

const CRating CProgramInfoTag::GetRating(std::string type) const
{
  if (type.empty())
    type = m_strDefaultRating;

  const RatingMap::const_iterator &rating = m_ratings.find(type);
  if (rating == m_ratings.end())
    return CRating();

  return rating->second;
}

const std::string& CProgramInfoTag::GetDefaultRating() const
{
  return m_strDefaultRating;
}

const bool CProgramInfoTag::HasYear() const
{
  return m_releaseDate.IsValid();
}

const int CProgramInfoTag::GetYear() const
{
  if (m_releaseDate.IsValid())
    return m_releaseDate.GetYear();
  return 0;
}

const bool CProgramInfoTag::HasReleaseDate() const
{
  return m_bHasReleaseDate;
}

const CDateTime& CProgramInfoTag::GetReleaseDate() const
{
  return m_releaseDate;
}

void CProgramInfoTag::ParseNative(const TiXmlElement* program, bool prioritise)
{
  std::string value;
  float fValue;
  bool bValue;

  std::vector<std::string> developers(m_developer);
  if (XMLUtils::GetStringArray(program, "developer", developers, prioritise, g_advancedSettings.m_programItemSeparator))
    SetDeveloper(developers);

  std::vector<std::string> publishers(m_publisher);
  if (XMLUtils::GetStringArray(program, "publisher", publishers, prioritise, g_advancedSettings.m_programItemSeparator))
    SetPublisher(publishers);

  std::vector<std::string> genres(m_genre);
  if (XMLUtils::GetStringArray(program, "genre", genres, prioritise, g_advancedSettings.m_programItemSeparator))
    SetGenre(genres);

  std::vector<std::string> descriptors(m_descriptor);
  if (XMLUtils::GetStringArray(program, "descriptor", descriptors, prioritise, g_advancedSettings.m_programItemSeparator))
    SetDescriptor(descriptors);

  std::vector<std::string> generalFeatures(m_generalFeature);
  if (XMLUtils::GetStringArray(program, "generalfeature", generalFeatures, prioritise, g_advancedSettings.m_programItemSeparator))
    SetGeneralFeature(generalFeatures);

  std::vector<std::string> onlineFeatures(m_onlineFeature);
  if (XMLUtils::GetStringArray(program, "onlinefeature", onlineFeatures, prioritise, g_advancedSettings.m_programItemSeparator))
    SetOnlineFeature(onlineFeatures);

  std::vector<std::string> platforms(m_platform);
  if (XMLUtils::GetStringArray(program, "platform", platforms, prioritise, g_advancedSettings.m_programItemSeparator))
    SetPlatform(platforms);

  std::vector<std::string> tags(m_tags);
  if (XMLUtils::GetStringArray(program, "tag", tags, prioritise, g_advancedSettings.m_programItemSeparator))
    SetTags(tags);

  if (XMLUtils::GetString(program, "type", value))
    m_type = value;

  if (XMLUtils::GetString(program, "trailer", value))
    m_strTrailer = value;

  if (XMLUtils::GetString(program, "overview", value))
    m_strPlot = value;

  if (XMLUtils::GetString(program, "title", value))
    m_strTitle = value;

  if (XMLUtils::GetString(program, "titleid", value))
    m_strXBENumber = value;

  if (XMLUtils::GetString(program, "originaltitle", value))
    m_strOriginalTitle = value;

  if (XMLUtils::GetString(program, "esrb", value))
    m_strESRB = value;

  if (XMLUtils::GetString(program, "system", value))
    m_strSystem = value;

  size_t iThumbCount = m_strPictureURL.m_url.size();
  std::string xmlAdd = m_strPictureURL.m_xml;

  const TiXmlElement* thumb = program->FirstChildElement("thumb");
  while (thumb)
  {
    m_strPictureURL.ParseElement(thumb);
    if (prioritise)
    {
      std::string temp;
      temp << *thumb;
      xmlAdd = temp+xmlAdd;
    }
    thumb = thumb->NextSiblingElement("thumb");
  }

  XMLUtils::GetInt(program, "playcount", m_playCount);

  if (XMLUtils::GetBoolean(program, "exclusive", bValue))
    m_bExclusive = bValue;

  XMLUtils::GetDateTime(program, "dateadded", m_dateAdded);
  XMLUtils::GetDate(program, "lastplayed", m_lastPlayed);
  if (XMLUtils::GetDate(program, "releasedate", m_releaseDate))
    m_bHasReleaseDate = true;
  else
  {
    int year;
    if (XMLUtils::GetInt(program, "year", year))
      SetYear(year);
  }

  const TiXmlElement* node = program->FirstChildElement("ratings");
  if (node)
  {
    for (const TiXmlElement* child = node->FirstChildElement("rating"); child != nullptr; child = child->NextSiblingElement("rating"))
    {
      CRating r;
      std::string name;
      if (child->QueryStringAttribute("name", &name) != TIXML_SUCCESS)
        name = "default";
      XMLUtils::GetFloat(child, "value", r.rating);
      if (XMLUtils::GetString(child, "votes", value))
        r.votes = StringUtils::ReturnDigits(value);
      int max_value = 10;
      if ((child->QueryIntAttribute("max", &max_value) == TIXML_SUCCESS) && max_value >= 1)
        r.rating = r.rating / max_value * 10; // Normalise the Program Rating to between 1 and 10
      SetRating(r, name);
      bool isDefault = false;
      if ((child->QueryBoolAttribute("default", &isDefault) == TIXML_SUCCESS) && isDefault)
        m_strDefaultRating = name;
    }
  }
  else if (XMLUtils::GetFloat(program, "rating", fValue))
  {
    CRating r(fValue, 0);
    if (XMLUtils::GetString(program, "votes", value))
      r.votes = StringUtils::ReturnDigits(value);
    int max_value = 10;
    const TiXmlElement* rElement = program->FirstChildElement("rating");
    if (rElement && (rElement->QueryIntAttribute("max", &max_value) == TIXML_SUCCESS) && max_value >= 1)
      r.rating = r.rating / max_value * 10; // Normalise the Program Rating to between 1 and 10
    SetRating(r, "default");
    m_strDefaultRating = "default";
  }

  const TiXmlElement *fanart = program->FirstChildElement("fanart");
  if (fanart)
  {
    // we prioritise mixed-mode nfo's with fanart set
    if (prioritise)
    {
      std::string temp;
      temp << *fanart;
      m_fanart.m_xml = temp+m_fanart.m_xml;
    }
    else
      m_fanart.m_xml << *fanart;
    m_fanart.Unpack();
  }
}

bool CProgramInfoTag::IsEmpty() const
{
  return (m_strTitle.empty() &&
          m_strFile.empty() &&
          m_strPath.empty());
}

void CProgramInfoTag::SetBasePath(std::string basePath)
{
  m_basePath = basePath;
}

void CProgramInfoTag::SetParentPathID(int parentPathID)
{
  m_parentPathID = parentPathID;
}

void CProgramInfoTag::SetDeveloper(std::vector<std::string> developer)
{
  m_developer = developer;
}

void CProgramInfoTag::SetPublisher(std::vector<std::string> publisher)
{
  m_publisher = publisher;
}

void CProgramInfoTag::SetGenre(std::vector<std::string> genre)
{
  m_genre = genre;
}

void CProgramInfoTag::SetDescriptor(std::vector<std::string> descriptor)
{
  m_descriptor = descriptor;
}

void CProgramInfoTag::SetGeneralFeature(std::vector<std::string> generalFeature)
{
  m_generalFeature = generalFeature;
}

void CProgramInfoTag::SetOnlineFeature(std::vector<std::string> onlineFeature)
{
  m_onlineFeature = onlineFeature;
}

void CProgramInfoTag::SetPlatform(std::vector<std::string> platform)
{
  m_platform = platform;
}

void CProgramInfoTag::SetTags(std::vector<std::string> tags)
{
  m_tags = tags;
}

void CProgramInfoTag::SetTrailer(std::string trailer)
{
  m_strTrailer = trailer;
}

void CProgramInfoTag::SetPlot(std::string plot)
{
  m_strPlot = plot;
}

void CProgramInfoTag::SetPictureURL(CScraperUrl& pictureURL)
{
  m_strPictureURL = pictureURL;
}

void CProgramInfoTag::SetTitle(std::string title)
{
  m_strTitle = title;
}

void CProgramInfoTag::SetFile(std::string file)
{
  m_strFile = file;
}

void CProgramInfoTag::SetPath(std::string path)
{
  m_strPath = path;
}

void CProgramInfoTag::SetXBENumber(std::string xbeNumber)
{
  m_strXBENumber = xbeNumber;
}

void CProgramInfoTag::SetFileNameAndPath(std::string fileNameAndPath)
{
  m_strFileNameAndPath = fileNameAndPath;
}

void CProgramInfoTag::SetOriginalTitle(std::string originalTitle)
{
  m_strOriginalTitle = originalTitle;
}

void CProgramInfoTag::SetESRB(std::string esrb)
{
  m_strESRB = esrb;
}

void CProgramInfoTag::SetSystem(std::string system)
{
  m_strSystem = system;
}

void CProgramInfoTag::SetLastPlayed(CDateTime lastPlayed)
{
  m_lastPlayed = lastPlayed;
}

void CProgramInfoTag::SetPlayCount(int playCount)
{
  m_playCount = playCount;
}

void CProgramInfoTag::SetReleaseDate(CDateTime releaseDate)
{
  m_releaseDate = releaseDate;
  m_bHasReleaseDate = m_releaseDate.IsValid();
}

void CProgramInfoTag::SetReleaseDateFromDBDate(std::string releaseDateString)
{
  CDateTime releaseDate;
  releaseDate.SetFromDBDate(releaseDateString);
  SetReleaseDate(releaseDate);
}

void CProgramInfoTag::SetYear(int year)
{
  if (m_bHasReleaseDate)
    m_releaseDate.SetDate(year, m_releaseDate.GetMonth(), m_releaseDate.GetDay());
  else
    m_releaseDate = CDateTime(year, 1, 1, 0, 0, 0);
}

void CProgramInfoTag::SetDbId(int dbId)
{
  m_iDbId = dbId;
}

void CProgramInfoTag::SetFileId(int fileId)
{
  m_iFileId = fileId;
}

void CProgramInfoTag::SetRating(float rating, int votes, const std::string& type /* = "" */, bool def /* = false */)
{
  SetRating(CRating(rating, votes), type, def);
}

void CProgramInfoTag::SetRating(CRating rating, const std::string& type /* = "" */, bool def /* = false */)
{
  if (rating.rating <= 0 || rating.rating > 10)
    return;

  if (type.empty())
    m_ratings[m_strDefaultRating] = rating;
  else
  {
    if (def || m_ratings.empty())
      m_strDefaultRating = type;
    m_ratings[type] = rating;
  }
}

void CProgramInfoTag::SetRating(float rating, const std::string& type /* = "" */, bool def /* = false */)
{
  if (rating <= 0 || rating > 10)
    return;

  if (type.empty())
    m_ratings[m_strDefaultRating].rating = rating;
  else
  {
    if (def || m_ratings.empty())
      m_strDefaultRating = type;
    m_ratings[type].rating = rating;
  }
}

void CProgramInfoTag::RemoveRating(const std::string& type)
{
  if (m_ratings.find(type) != m_ratings.end())
  {
    m_ratings.erase(type);
    if (m_strDefaultRating == type && !m_ratings.empty())
      m_strDefaultRating = m_ratings.begin()->first;
  }
}

void CProgramInfoTag::SetRatings(RatingMap ratings)
{
  m_ratings = boost::move(ratings);
}

void CProgramInfoTag::SetExclusive(bool exclusive)
{
  m_bExclusive = exclusive;
}

std::string CProgramInfoTag::Trim(std::string &value)
{
  return StringUtils::Trim(value);
}

void TrimStr(std::string& str) {
  str = StringUtils::Trim(str);
}

std::vector<std::string> CProgramInfoTag::Trim(std::vector<std::string>& items)
{
  std::for_each(items.begin(), items.end(), TrimStr);
  return items;
}
