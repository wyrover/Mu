/*
 * Copyright (C) Kreogist Dev Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include <QFileInfo>
#include <QTextStream>
#include <QTextCodec>

#include "knglobal.h"

#include "knmusiclrclyricsparser.h"
#include "knmusicnowplayingbase.h"

#include "knmusiclyricsmanager.h"

KNMusicLyricsManager::KNMusicLyricsManager(QObject *parent) :
    QObject(parent)
{
    //Initial the global instance.
    m_global=KNGlobal::instance();
    //Initial the LRC lyrics parser and utf-8 codec.
    m_parser=new KNMusicLRCLyricsParser(this);
    m_utf8Codec=QTextCodec::codecForName("UTF-8");
    //Set the default lyrics directory path.
    setLyricsDir(KNMusicGlobal::musicLibraryPath()+"/Lyrics");
    //Set the default loading policy.
    m_policyList << SameNameInLyricsDir << RelateNameInLyricsDir
                 << SameNameInMusicDir << RelateNameInMusicDir;
    //Set the default relate finding policy list.
    m_relateNamePolicyList << LyricsNamedArtistHyphonTitle
                           << LyricsNamedTitle
                           << LyricsNamedAlbumHyphonTitle;
}

KNMusicLyricsManager::~KNMusicLyricsManager()
{
    //Remove all the downloaders.
    qDeleteAll(m_downloaders);
}

void KNMusicLyricsManager::setNowPlaying(KNMusicNowPlayingBase *nowPlaying)
{
    connect(nowPlaying, &KNMusicNowPlayingBase::nowPlayingChanged,
            this, &KNMusicLyricsManager::loadLyrics);
}

void KNMusicLyricsManager::installLyricsDownloader(KNMusicLyricsDownloader *downloader)
{
    //Move the downloader to manager threads.
    downloader->moveToThread(thread());
    //Add to downloader list.
    m_downloaders.append(downloader);
}

QStringList KNMusicLyricsManager::textList() const
{
    return m_textList;
}

QList<qint64> KNMusicLyricsManager::positionList() const
{
    return m_positionList;
}

void KNMusicLyricsManager::loadLyrics(const KNMusicAnalysisItem &analysisItem)
{
    const KNMusicDetailInfo &detailInfo=analysisItem.detailInfo;
    //Clear the current data of the lyrics.
    clearCurrentData();
    //Find the lyrics at local folder.
    if(findLocalLyricsFile(detailInfo))
    {
        //Ask to update lyrics for datas.
        emit lyricsUpdate();
        m_musicDetailInfo=detailInfo;
        return;
    }
    //Or else we need to download the lyrics.
    if(m_enableOnlineLyrics)
    {
        downloadLyrics(detailInfo);
    }
}

void KNMusicLyricsManager::downloadLyrics(const KNMusicDetailInfo &detailInfo)
{
    //Using all downloaders to download the lyrics.
    QList<KNMusicLyricsDetails> lyricsList;
    for(QLinkedList<KNMusicLyricsDownloader *>::iterator i=m_downloaders.begin();
        i!=m_downloaders.end();
        ++i)
    {
        //Try to download the lyrics from all the remote server.
        (*i)->downloadLyrics(detailInfo, lyricsList);
    }
    //Check the lyrics list.
    if(lyricsList.isEmpty())
    {
        return;
    }
    //Sort the list according to the similarity of the lyrics.
    qSort(lyricsList.begin(), lyricsList.end(), lyricsDetailLessThan);
    //Parse all the data from the top to the bottom, save the first lyrics which
    //can be parsed.
    QList<qint64> positionList;
    QStringList textList;
    for(QList<KNMusicLyricsDetails>::iterator i=lyricsList.begin();
        i!=lyricsList.end();
        ++i)
    {
        if(m_parser->parseData((*i).lyricsData, positionList, textList))
        {
            //Save the current data to a file.
            saveLyrics(detailInfo, (*i).lyricsData);
            //Save the position and text list.
            m_positionList=positionList;
            m_textList=textList;
            //Save the detail info.
            m_musicDetailInfo=detailInfo;
            //Ask to update lyrics for datas.
            emit lyricsUpdate();
            return;
        }
    }
}

inline void KNMusicLyricsManager::clearCurrentData()
{
    //Clear the lyrics data.
    m_textList.clear();
    m_positionList.clear();
    //Reset the lyrics file path.
    m_lyricsFilePath.clear();
    //Reset the music detail info.
    m_musicDetailInfo=KNMusicDetailInfo();
    //Emit the lyrics has been reset.
    emit lyricsReset();
}

bool KNMusicLyricsManager::findLocalLyricsFile(const KNMusicDetailInfo &detailInfo)
{
    //Generate the same file name as the music.
    QFileInfo musicFileInfo(detailInfo.filePath);
    QString sameNameLyricsFileName=musicFileInfo.completeBaseName()+".lrc";
    //Search the lyrics according the lyrics finding policy.
    for(int i=0; i<m_policyList.size(); i++)
    {
        switch(m_policyList.at(i))
        {
        case SameNameInLyricsDir:
            if(triedLyricsFile(m_lyricsDir+"/"+sameNameLyricsFileName))
            {
                return true;
            }
            break;
        case RelateNameInLyricsDir:
            if(triedRelatedNameLyricsFile(m_lyricsDir, detailInfo))
            {
                return true;
            }
            break;
        case SameNameInMusicDir:
            if(triedLyricsFile(musicFileInfo.absolutePath()+"/"+
                               sameNameLyricsFileName))
            {
                return true;
            }
            break;
        case RelateNameInMusicDir:
            if(triedRelatedNameLyricsFile(musicFileInfo.absolutePath(),
                                          detailInfo))
            {
                return true;
            }
            break;
        }
    }
    return false;
}

inline bool KNMusicLyricsManager::triedLyricsFile(const QString &lyricsPath)
{
    //Check if the lyrics is exist.
    QFileInfo lyricsFileInfo(lyricsPath);
    if(!lyricsFileInfo.exists())
    {
        return false;
    }
    //Generate a temporary position and text list.
    QList<qint64> positionList;
    QStringList textList;
    //Tried to parse the lyrics file.
    if(m_parser->parseFile(lyricsPath, positionList, textList))
    {
        //Save the position and text list.
        m_positionList=positionList;
        m_textList=textList;
        return true;
    }
    return false;
}

bool KNMusicLyricsManager::triedRelatedNameLyricsFile(const QString &dirPath,
                                                       const KNMusicDetailInfo &detailInfo)
{
    //Find the lyrics named with the following.
    for(int i=0; i<m_relateNamePolicyList.size(); i++)
    {
        switch(m_relateNamePolicyList.at(i))
        {
        case LyricsNamedArtistHyphonTitle:
            if(triedLyricsFile(dirPath+"/"+m_global->legalFileName(detailInfo.textLists[Artist]+" - "+detailInfo.textLists[Name]+".lrc")))
            {
                return true;
            }
            break;
        case LyricsNamedTitle:
            if(triedLyricsFile(dirPath+"/"+m_global->legalFileName(detailInfo.textLists[Name]+".lrc")))
            {
                return true;
            }
            break;
        case LyricsNamedAlbumHyphonTitle:
            if(triedLyricsFile(dirPath+"/"+m_global->legalFileName(detailInfo.textLists[Album]+" - "+detailInfo.textLists[Name]+".lrc")))
            {
                return true;
            }
            break;
        }
    }
    return false;
}

void KNMusicLyricsManager::saveLyrics(const KNMusicDetailInfo &detailInfo,
                                       const QString &content)
{
    //Generate the lyrics file.
    QFile lyricsFile(m_lyricsDir + "/" +
                     m_global->legalFileName(detailInfo.textLists[Artist]+" - "+detailInfo.textLists[Name]+".lrc"));
    if(lyricsFile.open(QIODevice::WriteOnly))
    {
        //Save the content to the file, encording UTF-8.
        QTextStream lyricsFileStream(&lyricsFile);
        lyricsFileStream.setCodec(m_utf8Codec);
        lyricsFileStream << content << flush;
        //Close the file.
        lyricsFile.close();
    }
}

bool KNMusicLyricsManager::lyricsDetailLessThan(const KNMusicLyricsDetails &lyricsDetailLeft,
                                                 const KNMusicLyricsDetails &lyricsDetailRight)
{
    return (lyricsDetailLeft.titleSimilarity==lyricsDetailRight.titleSimilarity)?
                lyricsDetailLeft.artistSimilarity<lyricsDetailRight.artistSimilarity:
                lyricsDetailLeft.titleSimilarity<lyricsDetailRight.titleSimilarity;
}

KNMusicDetailInfo KNMusicLyricsManager::musicDetailInfo() const
{
    return m_musicDetailInfo;
}

bool KNMusicLyricsManager::enableOnlineLyrics() const
{
    return m_enableOnlineLyrics;
}

void KNMusicLyricsManager::setEnableOnlineLyrics(bool enableOnlineLyrics)
{
    m_enableOnlineLyrics = enableOnlineLyrics;
}

QString KNMusicLyricsManager::lyricsFilePath() const
{
    return m_lyricsFilePath;
}

QString KNMusicLyricsManager::lyricsDir() const
{
    return m_lyricsDir;
}

void KNMusicLyricsManager::setLyricsDir(const QString &lyricsDir)
{
    m_lyricsDir=KNGlobal::ensurePathAvaliable(lyricsDir);
}
