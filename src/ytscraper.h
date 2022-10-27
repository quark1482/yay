/*
 * Part of the YAY downloader project.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef YTSCRAPER_H
#define YTSCRAPER_H

#include <QtCore>
#include <QtWebEngineCore>
#include <QApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "mimetools.h"
#include "unitsformat.h"

/**
 * @brief Available media types.
 *
 * YT adaptive video formats are either video-only or audio-only.
 * Constant bitrate formats include both video and audio.
 */
typedef enum {
    MT_INVALID=-1,
    MT_VIDEO_AND_AUDIO,
    MT_VIDEO_ONLY,
    MT_AUDIO_ONLY
} MediaType;

/**
 * @brief Media entry details.
 *
 * Holds the details for a single media entry, for a given YT video.
 */
typedef struct {
    MediaType mtMediaType;
    QString   sURL;
    QString   sMIMEType;
    QString   sVideoQuality;
    QString   sAudioQuality;
    uint      uiFormatTag;
    uint      uiBitrate;
    uint      uiSampleRate;
    uint      uiWidth;
    uint      uiHeight;
    uint      uiFPS;
    uint      uiDuration;
    quint64   ui64Size;
} MediaEntry;

/**
 * @brief A list of media entries.
 *
 * Holds the details for all the available media entries, for a given YT video.
 */
typedef QList<MediaEntry> MediaEntryList;

/**
 * @brief YT video details.
 *
 * Holds every required and important detail that can be collected for a given YT video.
 */
typedef struct {
    QString        sVideoID;
    QString        sTitle;
    QString        sDescription;
    QString        sThumbnail;
    uint           uiDuration;
    MediaEntryList melMediaEntries;
} VideoDetails;

/**
 * @brief The MyWebEnginePage class
 *
 * Helper class for displaying JS debug info for QWebEnginePage.
 * The main class, YTScraper, executes JS code from time to time,
 * so this comes handy during development and tests.
 */
class MyWebEnginePage:public QWebEnginePage {
public:
    MyWebEnginePage(QObject *objParent=nullptr):QWebEnginePage(objParent) {}
    virtual void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                          const QString &message,
                                          int lineNumber,
                                          const QString &sourceID) {
        qDebug() << "javaScriptConsoleMessage()";
        qDebug() << "level" << level;
        qDebug() << "message" << message;
        qDebug() << "line number" << lineNumber;
        qDebug() << "source ID" << sourceID;
    }
};

/**
 * @brief The YTScraper class
 *
 * Provides a way of grabbing information and downloadable links for YT videos.
 */
class YTScraper:public QObject {
private:
    QString               sLastError;
    QNetworkAccessManager *namYTS;
    MyWebEnginePage       webPlayer;
    bool getVideoHeaders(QString,QString &,quint64 &,QString &);
    bool getVideoHTML(QString,QString &,QString &);
    bool getVideoPlayerDecipherFunctionName(QString,QString &);
    bool getVideoPlayerSource(QString,QString &,QString &);
    bool getVideoSignature(QString,QString &);
    bool parseQueryVideoResponse(QString,VideoDetails &,QString &);
    bool parseVideoPlayerSource(QString,QString &,QString &,QString &,QString &,QString &,QString &);
    bool setDecipherEngine(QString,QString &);
public:
    YTScraper();
    ~YTScraper();
    static bool    checkVideoDetails(const VideoDetails);
    static void    clearVideoDetails(VideoDetails &);
    static void    copyVideoDetails(VideoDetails &,const VideoDetails);
    static QString createVideoURL(QString);
    QString getLastError();
    bool    getVideoDetails(QString,VideoDetails &);
    bool    parseURL(QString,QString &,QString &);
};

#endif // YTSCRAPER_H
