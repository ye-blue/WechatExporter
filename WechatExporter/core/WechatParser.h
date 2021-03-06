//
//  WechatParser.h
//  WechatExporter
//
//  Created by Matthew on 2020/9/30.
//  Copyright © 2020 Matthew. All rights reserved.
//

#ifndef WechatParser_h
#define WechatParser_h

#include <stdio.h>
#include <regex>
#include <string>
#include <vector>
#include <atomic>
#include <map>
#include "Utils.h"
#include "Shell.h"
#include "Downloader.h"
#include "ByteArrayLocater.h"
#include "WechatObjects.h"
#include "ITunesParser.h"

template<class T>
class FilterBase
{
protected:
    std::string m_path;
    std::string m_pattern;

public:
    bool operator() (const ITunesFile* s1, const T& s2) const    // less
    {
        return !startsWith(s1->relativePath, m_path) && s1->relativePath < m_path;
    }
    bool operator() (const T& s2, const ITunesFile* s1) const    // greater
    {
        return !startsWith(s1->relativePath, m_path) && s1->relativePath > m_path;
    }
    bool operator==(const ITunesFile* s) const
    {
        return startsWith(s->relativePath, m_path) && (s->relativePath.find(m_pattern, m_path.size()) != std::string::npos);
    }
    std::string parse(const ITunesFile* s) const
    {
        if (*this == s)
        {
            return s->relativePath.substr(m_path.size());
        }
        return std::string("");
    }
};

template<class T>
class RegexFilterBase
{
protected:
    std::string m_path;
    std::regex m_pattern;

public:
    bool operator() (const ITunesFile* s1, const T& s2) const    // less
    {
        return !startsWith(s1->relativePath, m_path) && s1->relativePath < m_path;
    }
    bool operator() (const T& s2, const ITunesFile* s1) const    // greater
    {
        return !startsWith(s1->relativePath, m_path) && s1->relativePath > m_path;
    }
    bool operator==(const ITunesFile* s) const
    {
        std::smatch sm;
        return startsWith(s->relativePath, m_path) && std::regex_search(s->relativePath.begin() + m_path.size(), s->relativePath.end(), sm, m_pattern);
    }
    std::string parse(const ITunesFile* s) const
    {
        std::smatch sm;
        if (std::regex_search(s->relativePath.begin() + m_path.size(), s->relativePath.end(), sm, m_pattern))
        {
            return sm[1];
        }
        return std::string("");
    }
};

class MessageDbFilter : public RegexFilterBase<MessageDbFilter>
{
public:
    MessageDbFilter(const std::string& basePath) : RegexFilterBase()
    {
        std::string vpath = basePath;
        std::replace(vpath.begin(), vpath.end(), '\\', '/');
        if (!endsWith(vpath, "/"))
        {
            vpath += "/";
        }
        vpath += "DB/";
        
        m_path = vpath;
        m_pattern = "^(message_[0-9]{1,4}\\.sqlite)$";
    }
};

class UserFolderFilter : public RegexFilterBase<UserFolderFilter>
{
public:
    UserFolderFilter() : RegexFilterBase()
    {
        m_path = "Documents/";
        // m_pattern = "^([a-zA-Z0-9]{32})$";
    }
    
    bool operator==(const ITunesFile* s) const
    {
        if ((s->relativePath.size() != (m_path.size() + 32)) || !startsWith(s->relativePath, m_path))
        {
            return false;
        }
        if (s->relativePath.find('/', m_path.size()) != std::string::npos)
        {
            return false;
        }
        
        return true;
    }
    
    std::string parse(const ITunesFile* s) const
    {
        return s->relativePath.size() > 32 ? s->relativePath.substr(m_path.size()) : "";
    }
};

class WechatInfoParser
{
private:
    ITunesDb *m_iTunesDb;
    
public:
    
    WechatInfoParser(ITunesDb *iTunesDb);
    
    bool parse(WechatInfo& wechatInfo);
    
protected:
    bool parsePreferences(WechatInfo& wechatInfo);
};

class SessionCellDataFilter : public FilterBase<SessionCellDataFilter>
{
public:
    SessionCellDataFilter(const std::string& cellDataBasePath, const std::string& cellDataVersion) : FilterBase()
    {
        std::string vpath = cellDataBasePath;
        std::replace(vpath.begin(), vpath.end(), '\\', '/');
        
        m_path = vpath;
        m_pattern = "celldata" + cellDataVersion;    // celldataV7
    }
};

class LoginInfo2Parser
{
private:
    ITunesDb *m_iTunesDb;
    
public:
    LoginInfo2Parser(ITunesDb *iTunesDb);
    
    bool parse(std::vector<Friend>& users);
    
    bool parse(const std::string& loginInfo2Path, std::vector<Friend>& users);
    
private:
    int parseUser(const char* data, int length, std::vector<Friend>& users);
    bool parseUserFromFolder(std::vector<Friend>& users);
    bool parseMMSettingsFromMMKV(std::map<std::string, std::pair<std::string, std::string>>& mmsettingFiles);
};

class MMSettingInMMappedKVFilter : public FilterBase<MMSettingInMMappedKVFilter>
{
protected:
    std::string m_suffix;
    
public:
    MMSettingInMMappedKVFilter(const std::string& uid) : FilterBase()
    {
        m_pattern = "Documents/MMappedKV/";
        m_path = m_pattern + "mmsetting.archive." + uid;
        m_suffix = ".crc";
    }
    
    MMSettingInMMappedKVFilter() : FilterBase()
    {
        m_pattern = "Documents/MMappedKV/";
        m_path = m_pattern + "mmsetting.archive.";
        m_suffix = ".crc";
    }
    
    std::string getPrefix() const
    {
        return "mmsetting.archive.";
    }
    
    bool operator==(const ITunesFile* s) const
    {
        return startsWith(s->relativePath, m_path) && !endsWith(s->relativePath, m_suffix);
    }
    std::string parse(const ITunesFile* s) const
    {
        if (*this == s)
        {
            return s->relativePath.substr(m_pattern.size());
        }
        return std::string("");
    }
};

class MMSettings
{
protected:
    std::string m_usrName;
    std::string m_name;
    std::string m_displayName;
    std::string m_portrait;
    std::string m_portraitHD;
public:
    
    std::string getUsrName() const;
    std::string getDisplayName() const;
    std::string getPortrait() const;
    std::string getPortraitHD() const;
    
protected:
    void clear();
};

class MMKVParser : public MMSettings
{
public:
    MMKVParser();
    bool parse(const std::string& path, const std::string& crcPath);
};

class MMSettingParser : public MMSettings
{
private:
    ITunesDb *m_iTunesDb;

public:
    MMSettingParser(ITunesDb *iTunesDb);
    bool parse(const std::string& usrNameHash);
};

class FriendsParser
{
public:
    FriendsParser(bool detailedInfo = true);
    bool parseWcdb(const std::string& mmPath, Friends& friends);
    
private:
    bool parseRemark(const void *data, int length, Friend& f);
    bool parseAvatar(const void *data, int length, Friend& f);
    bool parseChatroom(const void *data, int length, Friend& f);
    
private:
    bool m_detailedInfo;
};

class SessionsParser
{
private:
    ITunesDb *m_iTunesDb;
    ITunesDb *m_iTunesDbShare;
    Shell*      m_shell;
    std::string m_cellDataVersion;
    bool        m_detailedInfo;

public:
    SessionsParser(ITunesDb *iTunesDb, ITunesDb *iTunesDbShare, Shell* shell, const std::string& cellDataVersion, bool detailedInfo = true);
    
    bool parse(const Friend& user, std::vector<Session>& sessions, const Friends& friends);

private:
    bool parseCellData(const std::string& userRoot, Session& session);
    bool parseMessageDbs(const std::string& userRoot, std::vector<Session>& sessions);
    bool parseMessageDb(const std::string& mmPath, std::vector<std::pair<std::string, int>>& sessionIds);
    
    bool parseSessionsInGroupApp(const std::string& userRoot, std::vector<Session>& sessions);
};

struct MsgRecord
{
    int createTime;
    std::string message;
    int des;
    int type;
    int msgId;
};


struct ForwardMsg
{
    int msgid;
    std::string usrName;
    std::string displayName;
    std::string protrait;
    std::string dataType;
    std::string subType;
    std::string dataId;
    std::string dataFormat;
    std::string msgTime;
    std::string srcMsgTime;
    std::string message;
    std::string link;
    std::string nestedMsgs;
};

enum SessionParsingOption
{
    SPO_IGNORE_AVATAR = 1 << 0,
    SPO_IGNORE_AUDIO = 1 << 1,
    SPO_IGNORE_IMAGE = 1 << 2,
    SPO_IGNORE_VIDEO = 1 << 3,
    SPO_IGNORE_EMOJI = 1 << 4,
    SPO_IGNORE_FILE = 1 << 5,
    SPO_IGNORE_CARD = 1 << 6,
    SPO_IGNORE_SHARING = 1 << 7,
    SPO_IGNORE_HTML_ENC = 1 << 8,
    SPO_TEXT_MODE = 0xFFFF,
    SPO_DESC = 1 << 16,
    SPO_ICON_IN_SESSION = 1 << 17     // Put Head Icon and Emoji files in the folder of session
};

class TemplateValues
{
private:
    std::string m_name;
    std::map<std::string, std::string> m_values;
    
public:
    using const_iterator = std::map<std::string, std::string>::const_iterator;
 
public:
    TemplateValues()
    {
    }
    TemplateValues(const std::string& name) : m_name(name)
    {
    }
    std::string getName() const
    {
        return m_name;
    }
    void setName(const std::string& name)
    {
        m_name = name;
    }
    std::string& operator[](const std::string& k)
    {
        return m_values[k];
    }
    bool hasValue(const std::string& key) const
    {
        return m_values.find(key) != m_values.cend();
    }
    
    const_iterator cbegin() const
    {
        return m_values.cbegin();
    }
    
    const_iterator cend() const
    {
        return m_values.cend();
    }
    
    void clear()
    {
        m_values.clear();
    }
    
    void clearName()
    {
        m_name.clear();
    }
};

class SessionParser
{
private:
    std::function<std::string(const std::string&)> m_localFunction;

    int m_options;
    Friends& m_friends;
    const ITunesDb& m_iTunesDb;
    const Shell&  m_shell;
    Downloader& m_downloader;
    Friend m_myself;
    
    mutable std::vector<unsigned char> m_pcmData;  // buffer
    
public:
    SessionParser(Friend& myself, Friends& friends, const ITunesDb& iTunesDb, const Shell& shell, int options, Downloader& downloader, std::function<std::string(const std::string&)> localeFunc);
    void ignoreAudio(bool ignoreAudio = true)
    {
        if (ignoreAudio)
            m_options |= SPO_IGNORE_AUDIO;
        else
            m_options &= ~SPO_IGNORE_AUDIO;
    }
    void setOrder(bool asc = true)
    {
        if (asc)
            m_options &= ~SPO_DESC;
        else
            m_options |= SPO_DESC;
    }

    int parse(const std::string& userBase, const std::string& outputBase, const Session& session, std::function<bool(const std::vector<TemplateValues>&)> handler);

private:
	std::string getLocaleString(const std::string& key) const
    {
        return m_localFunction(key);
    }
    
    std::string getDisplayTime(int ms) const;
    bool requireFile(const std::string& vpath, const std::string& dest) const;
    bool parseRow(MsgRecord& record, const std::string& userBase, const std::string& path, const Session& session, std::vector<TemplateValues>& tvs);
    bool parseForwardedMsgs(const std::string& userBase, const std::string& outputPath, const Session& session, const MsgRecord& record, const std::string& title, const std::string& message, std::vector<TemplateValues>& tvs);
    std::string buildContentFromTemplateValues(const TemplateValues& values) const;
    void parseImage(const std::string& sessionPath, const std::string& sessionAssertsPath, const std::string& src, const std::string& srcPre, const std::string& dest, const std::string& srcThumb, const std::string& destThumb, TemplateValues& templateValues);
    void parseVideo(const std::string& sessionPath, const std::string& sessionAssertsPath, const std::string& src, const std::string& dest, const std::string& srcThumb, const std::string& destThumb, TemplateValues& templateValues);
    void parseFile(const std::string& sessionPath, const std::string& sessionAssertsPath, const std::string& src, const std::string& dest, const std::string& fileName, TemplateValues& templateValues);
    void parseCard(const std::string& sessionPath, const std::string& portraitDir, const std::string& cardMessage, TemplateValues& templateValues);
    
    void ensureDirectoryExisted(const std::string& path);
    
};


#endif /* WechatParser_h */
