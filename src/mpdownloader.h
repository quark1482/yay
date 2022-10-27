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

#ifndef MPDOWNLOADER_H
#define MPDOWNLOADER_H

#include <QtCore>
#include <QApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

/**
 * @brief The DownloadProgressCB typedef.
 *
 * Declares a callback function which receives the total downloaded bytes, the content
 * length and an optional customized user data, during the time a download is active.
 */
typedef void DownloadProgressCB(quint64,quint64,void *);

/**
 * @brief The MPDownloader class
 *
 * Provides a way of splitting the download of a given resource into multiple parts.
 */
class MPDownloader:public QObject {
private:
    bool                  bDownloading;
    QString               sLastError;
    QNetworkAccessManager *namMPD;
public:
    MPDownloader();
    ~MPDownloader();
    void    cancelDownload();
    bool    download(QString,QByteArray &,DownloadProgressCB=nullptr,void * =nullptr);
    QString getLastError();
    bool    isDownloading();
};

#endif // MPDOWNLOADER_H
