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

#include "mpdownloader.h"

/**
 * @brief User-Agent header to be used in every HTTP request.
 *
 * @todo Make this a property.
 */
#define MPD_HEADER_USER_AGENT_DEFAULT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/101.0.4951.67 Safari/537.36"

/**
 * @brief Maximum number of parts a download can be splitted into.
 */
#define MPD_MAX_DOWNLOAD_PARTS 16

/**
 * @brief Minimum allowed part size (in bytes) a download can be splitted into.
 */
#define MPD_MIN_DOWNLOAD_PART_SIZE 1048576

MPDownloader::MPDownloader() {
    bDownloading=false;
    sLastError.clear();
    namMPD=new QNetworkAccessManager();
}

MPDownloader::~MPDownloader() {
    delete namMPD;
}

/**
 * @brief Cancels the download forcefully.
 */
void MPDownloader::cancelDownload() {
    bDownloading=false;
}

/**
 * @brief Downloads the given resource with optional progress feedback.
 *
 * Performs a multi-parts download as long as the server supports the "Range" request header.
 *
 * @param[in] sURL          URL containing the resource to be downloaded
 * @param[in] abtTarget     complete downloaded contents
 * @param[in] dpcbProgress  callback function receiving the progress and user data
 * @param[in] lpcbData      customized user data to be passed to the callback
 *
 * @return true if every part was download successfully
 */
bool MPDownloader::download(QString            sURL,
                            QByteArray         &abtTarget,
                            DownloadProgressCB dpcbProgress,
                            void               *lpcbData) {
    bool            bResult=false;
    uint            uiResCode;
    quint64         ui64ContentLength;
    QNetworkRequest nrqMainRequest;
    QNetworkReply   *nrpMainReply;
    sLastError.clear();
    abtTarget.clear();
    nrqMainRequest.setUrl(QUrl(sURL));
    nrqMainRequest.setHeader(
        QNetworkRequest::KnownHeaders::UserAgentHeader,
        QStringLiteral(MPD_HEADER_USER_AGENT_DEFAULT)
    );
    // Sends a HEAD request to check if the server supports the Range header
    nrqMainRequest.setRawHeader(
        QStringLiteral("Range").toUtf8(),
        QStringLiteral("bytes=0-").toUtf8()
    );
    nrpMainReply=namMPD->head(nrqMainRequest);
    while(!nrpMainReply->isFinished())
        QApplication::processEvents(QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents);
    uiResCode=nrpMainReply->attribute(
        QNetworkRequest::Attribute::HttpStatusCodeAttribute
    ).toUInt();
    ui64ContentLength=nrpMainReply->header(
        QNetworkRequest::KnownHeaders::ContentLengthHeader
    ).toULongLong();
    if(QNetworkReply::NetworkError::NoError!=nrpMainReply->error())
        sLastError=nrpMainReply->errorString();
    else
        if(200==uiResCode||206==uiResCode) {
            bool            bRanged;
            quint64         ui64K,
                            ui64Start,ui64End,
                            ui64TotalParts,ui64PartSize,ui64TotalFinished,
                            ui64Progress[MPD_MAX_DOWNLOAD_PARTS];
            QNetworkRequest nrqRequest[MPD_MAX_DOWNLOAD_PARTS];
            QNetworkReply   *nrpReply[MPD_MAX_DOWNLOAD_PARTS];
            // Sometimes the content length is not known in-advance.
            // Without this value, is not possible to calculate the ranges.
            bRanged=ui64ContentLength&&(206==uiResCode);
            for(ui64K=0;ui64K<MPD_MAX_DOWNLOAD_PARTS;ui64K++) {
                ui64Progress[ui64K]=0;
                nrpReply[ui64K]=nullptr;
            }
            // Calculates the number of download parts needed.
            ui64TotalParts=1;
            if(bRanged) {
                ui64TotalParts=ui64ContentLength/MPD_MIN_DOWNLOAD_PART_SIZE;
                if(ui64ContentLength%MPD_MIN_DOWNLOAD_PART_SIZE)
                    ui64TotalParts++;
                if(ui64TotalParts>MPD_MAX_DOWNLOAD_PARTS)
                    ui64TotalParts=MPD_MAX_DOWNLOAD_PARTS;
            }
            // Calculates the size (in bytes) by part.
            ui64PartSize=ui64ContentLength/ui64TotalParts;
            if(ui64ContentLength%ui64TotalParts)
                ui64PartSize++;
            ui64Start=0;
            for(ui64K=0;ui64K<ui64TotalParts;ui64K++) {
                // Calculates the range end for each part.
                ui64End=ui64Start+ui64PartSize-1;
                if(ui64End>ui64ContentLength-1)
                    ui64End=ui64ContentLength-1;
                nrqRequest[ui64K].setUrl(QUrl(sURL));
                nrqRequest[ui64K].setHeader(
                    QNetworkRequest::KnownHeaders::UserAgentHeader,
                    QStringLiteral(MPD_HEADER_USER_AGENT_DEFAULT)
                );
                if(bRanged)
                    // Sets the Range header (if possible) for the associated request.
                    nrqRequest[ui64K].setRawHeader(
                        QStringLiteral("Range").toUtf8(),
                        QStringLiteral("bytes=%1-%2").arg(ui64Start).arg(ui64End).toUtf8()
                    );
                nrpReply[ui64K]=namMPD->get(nrqRequest[ui64K]);
                // Creates a custom property, "index", to identify each response ...
                // ... because it's not expected their signals to be triggered in order.
                nrpReply[ui64K]->setProperty(
                    QStringLiteral("index").toStdString().c_str(),
                    ui64K
                );
                connect(
                    nrpReply[ui64K],
                    &QNetworkReply::downloadProgress,
                    this,
                    [&](qint64 bytesReceived,qint64 bytesTotal) {
                        // Detects which response the emitted signal belongs to.
                        QNetworkReply *nrpSender=qobject_cast<QNetworkReply *>(
                                          QObject::sender()
                                      );
                        // Gets the index for the detected response.
                        quint64       ui64Index=nrpSender->property(
                                          QStringLiteral("index").toStdString().c_str()
                                      ).toULongLong(),
                                      ui64TotalProgress;
                        // Calculates the amount of bytes downloaded so far.
                        ui64Progress[ui64Index]=bytesReceived;
                        ui64TotalProgress=0;
                        for(ui64K=0;ui64K<ui64TotalParts;ui64K++)
                            ui64TotalProgress+=ui64Progress[ui64K];
                        // Invokes the callback with the current progress and user data.
                        if(nullptr!=dpcbProgress)
                            dpcbProgress(ui64TotalProgress,ui64ContentLength,lpcbData);
                    }
                );
                // Calculates the range start for each part.
                ui64Start+=ui64PartSize;
            }
            // Proceeds to wait for the download completion, once all parts have started.
            bDownloading=true;
            while(bDownloading) {
                ui64TotalFinished=0;
                for(ui64K=0;ui64K<ui64TotalParts;ui64K++)
                    // Detects when a part has stopped (for good or bad).
                    if(nrpReply[ui64K]->isFinished()) {
                        uiResCode=nrpReply[ui64K]->attribute(
                            QNetworkRequest::Attribute::HttpStatusCodeAttribute
                        ).toUInt();
                        if(QNetworkReply::NetworkError::NoError!=nrpReply[ui64K]->error()) {
                            sLastError=nrpReply[ui64K]->errorString();
                            break;
                        }
                        else
                            if(200==uiResCode||206==uiResCode) {
                                ui64TotalFinished++;
                                // Once all parts are complete ...
                                if(ui64TotalFinished==ui64TotalParts) {
                                    // ... we have a success.
                                    bResult=true;
                                    break;
                                }
                            }
                            else {
                                sLastError=QStringLiteral("Unexpected response code: %1").arg(uiResCode);
                                break;
                            }
                    }
                if(ui64K<ui64TotalParts)
                    break;
                else
                    QApplication::processEvents(QEventLoop::ProcessEventsFlag::AllEvents);
            }
            bDownloading=false;
            // Proceeds to join the downloaded contents.
            for(ui64K=0;ui64K<ui64TotalParts;ui64K++) {
                if(bResult)
                    abtTarget.append(nrpReply[ui64K]->readAll());
                nrpReply[ui64K]->~QNetworkReply();
            }
        }
        else
            sLastError=QStringLiteral("Unexpected response code: %1").arg(uiResCode);
    nrpMainReply->~QNetworkReply();
    return bResult;
}

/**
 * @brief Gets the last error that has occurred.
 *
 * @return the last error
 */
QString MPDownloader::getLastError() {
    return sLastError;
}

/**
 * @brief Checks if a download is in progress.
 *
 * @return true if there is an active download
 */
bool MPDownloader::isDownloading() {
    return bDownloading;
}
