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

#include "mainwindow.h"
#include "./ui_mainwindow.h"

/**
 * @brief Application's title
 */
#define APP_NAME "YAY downloader"

/**
 * @brief Default value for the clip size spinbox (in seconds)
 */
#define DEFAULT_CLIP_SIZE 30

/**
 * @brief Callback function receiving the progress from MPDownloader::download().
 *
 * @param[in] ui64Received  amount of bytes received
 * @param[in] ui64Total     total bytes to download
 * @param[in] lpcbData      raw pointer to the user interface
 */
void myProgressCallback(quint64 ui64Received,quint64 ui64Total,void *lpcbData) {
    Ui::MainWindow *myUI=reinterpret_cast<Ui::MainWindow *>(lpcbData);
    if(ui64Total) {
        float flProgress=100.0*ui64Received/ui64Total;
        myUI->stbMain->showMessage(
            QStringLiteral("%1 of %2 (%3%)").
            arg(
                UnitsFormat::bytes(ui64Received),
                UnitsFormat::bytes(ui64Total)
            ).
            arg(flProgress,0,'f',2)
        );
    }
    else
        myUI->stbMain->showMessage(
            QStringLiteral("%1").
            arg(UnitsFormat::bytes(ui64Received))
        );
}

MainWindow::MainWindow(QWidget *wgtParent):QMainWindow(wgtParent),ui(new Ui::MainWindow) {
    bFocusIsInVideoURL=false;
    YTScraper::clearVideoDetails(vdCurrentVideoDetails);
    ui->setupUi(this);
    ui->ledVideoURL->installEventFilter(this);
    ui->ledVideoURL->setFocus();
    ui->ledDestinationFolder->setText(
        QStandardPaths::standardLocations(
            QStandardPaths::StandardLocation::DownloadLocation
        ).at(0)
    );
    ui->chkSplit->setChecked(false);
    ui->spbIgnoreFirst->setEnabled(false);
    ui->spbClipSize->setEnabled(false);
    ui->spbIgnoreLast->setEnabled(false);
    connect(
        ui->btnDestinationFolder,
        &QPushButton::clicked,
        this,
        &MainWindow::slot_btnDestinationFolder_clicked
    );
    connect(
        ui->btnLoad,
        &QPushButton::clicked,
        this,
        &MainWindow::slot_btnLoad_clicked
    );
    connect(
        ui->btnDownload,
        &QPushButton::clicked,
        this,
        &MainWindow::slot_btnDownload_clicked
    );
    connect(
        ui->btnVideoThumbnail,
        &QPushButton::clicked,
        this,
        &MainWindow::slot_btnVideoThumbnail_clicked
    );
    connect(
        ui->chkSplit,
        &QCheckBox::stateChanged,
        this,
        &MainWindow::slot_chkSplit_stateChanged
    );
    this->setWindowTitle(QStringLiteral(APP_NAME));
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *evnE) {
    if(mpdVideoDownloader.isDownloading()) {
        QMessageBox::warning(
            this,
            QStringLiteral(APP_NAME),
            QStringLiteral("Program's busy.\n"
                           "Cannot exit now")
        );
        evnE->ignore();
        return;
    }
    evnE->accept();
}

bool MainWindow::eventFilter(QObject *objO,QEvent *evnE) {
    /**
     * @note Exclusive for the video URL line edit. This keeps the existing text
     *       selected inside the widget in case it gets the focus with a mouse click.
     *       Useful when replacing the URL or switching windows.
     */
    if(QEvent::Type::FocusIn==evnE->type()) {
        if(!bFocusIsInVideoURL) {
            QFocusEvent *evnF=static_cast<QFocusEvent *>(evnE);
            // Behaves like if the focus has been got through hitting tab or shift-tab.
            ui->ledVideoURL->selectAll();
            if(Qt::FocusReason::MouseFocusReason==evnF->reason())
                bFocusIsInVideoURL=true;
        }
    }
    else if(QEvent::Type::FocusOut==evnE->type()) {
        if(bFocusIsInVideoURL)
            bFocusIsInVideoURL=false;
    }
    else if(QEvent::Type::MouseButtonPress==evnE->type()) {
        // Ignores the mouse event right after the widget is focused, to avoid deselecting text.
        if(bFocusIsInVideoURL) {
            bFocusIsInVideoURL=false;
            return true;
        }
    }
    return QMainWindow::eventFilter(objO,evnE);
}

void MainWindow::slot_btnDestinationFolder_clicked() {
    QString sNewFolder;
    sNewFolder=QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Select destination folder"),
        ui->ledDestinationFolder->text()
    );
    if(!sNewFolder.isEmpty())
        ui->ledDestinationFolder->setText(sNewFolder);
}

void MainWindow::slot_btnLoad_clicked() {
    QString sVideoURL,sVideoId,sListId;
    QWidget *wgtNextFocus=nullptr;
    this->enableControls(false);
    this->setCursor(Qt::CursorShape::WaitCursor);
    ui->btnVideoThumbnail->setIcon(QIcon());
    sVideoURL=ui->ledVideoURL->text().trimmed();
    if(0==sVideoURL.length()) {
        QMessageBox::critical(
            this,
            QStringLiteral(APP_NAME),
            QStringLiteral("Missing URL")
        );
        wgtNextFocus=ui->ledVideoURL;
    }
    else {
        VideoDetails vdLoad;
        if(!ytsVideoScraper.parseURL(sVideoURL,sVideoId,sListId)) {
            QMessageBox::critical(
                this,
                QStringLiteral(APP_NAME),
                QStringLiteral("Unable to load URL:\n%1").arg(ytsVideoScraper.getLastError())
            );
            wgtNextFocus=ui->ledVideoURL;
        }
        else if(!ytsVideoScraper.getVideoDetails(sVideoId,vdLoad)) {
            QMessageBox::critical(
                this,
                QStringLiteral(APP_NAME),
                QStringLiteral("Unable to load URL:\n%1").arg(ytsVideoScraper.getLastError())
            );
            wgtNextFocus=ui->ledVideoURL;
        }
        else {
            // The video details and its list of downloadable media were successfully loaded.
            QString    sType,sSubtype,sSeparator;
            MIMEParams mmpParams;
            MediaType  mtPrevious=MediaType::MT_INVALID;
            YTScraper::copyVideoDetails(vdCurrentVideoDetails,vdLoad);
            ui->txtVideoDetails->clear();
            ui->cboMediaFormats->clear();
            ui->txtVideoDetails->appendPlainText(
                QStringLiteral("Id: %1\n"
                               "Duration: %2\n"
                               "Title: %3\n"
                               "Description: %4\n"
                               "Thumbnail: %5").
                arg(
                    vdLoad.sVideoID,
                    UnitsFormat::seconds(vdLoad.uiDuration/1000),
                    vdLoad.sTitle,
                    vdLoad.sDescription,
                    vdLoad.sThumbnail
                )
            );
            ui->txtLog->clear();
            for(const auto &e:vdLoad.melMediaEntries) {
                ui->txtLog->appendPlainText(
                    QStringLiteral("URL: %1\n"
                                   "Type: %2").
                    arg(
                        e.sURL,
                        e.sMIMEType
                    )
                );
                // Adds a separator/header (to the list of downloadable media formats) ...
                // ... identifying if they are video-only, audio-only or both.
                if(mtPrevious!=e.mtMediaType) {
                    switch(e.mtMediaType) {
                        case MediaType::MT_VIDEO_AND_AUDIO:
                            sSeparator=QStringLiteral("VIDEO AND AUDIO");
                            break;
                        case MediaType::MT_VIDEO_ONLY:
                            sSeparator=QStringLiteral("VIDEO ONLY");
                            break;
                        case MediaType::MT_AUDIO_ONLY:
                            sSeparator=QStringLiteral("AUDIO ONLY");
                            break;
                        default:
                            sSeparator.clear();
                    }
                    if(!sSeparator.isEmpty()) {
                        ui->cboMediaFormats->addItem(sSeparator,MediaType::MT_INVALID);
                        ui->cboMediaFormats->setItemData(
                            ui->cboMediaFormats->count()-1,
                            false,
                            Qt::ItemDataRole::UserRole-1
                        );
                        ui->cboMediaFormats->setItemData(
                            ui->cboMediaFormats->count()-1,
                            Qt::AlignmentFlag::AlignCenter,
                            Qt::ItemDataRole::TextAlignmentRole
                        );
                    }
                    mtPrevious=e.mtMediaType;
                }
                // The MIME type is guaranteed to be valid at this point.
                MIMETools::parse(e.sMIMEType,sType,sSubtype,mmpParams);
                if(MediaType::MT_AUDIO_ONLY==e.mtMediaType) {
                    // The current media entry only contains audio.
                    ui->txtLog->appendPlainText(
                        QStringLiteral("Audio(%1) %2 [%3]\n"
                                       "%4 bps, %5 hz").
                        arg(
                            e.sAudioQuality,
                            UnitsFormat::seconds(e.uiDuration/1000),
                            UnitsFormat::bytes(e.ui64Size)
                        ).
                        arg(e.uiBitrate).
                        arg(e.uiSampleRate)
                    );
                    // Adds the audio details to the list of downloadable formats.
                    ui->cboMediaFormats->addItem(
                        QStringLiteral("%1 (%2 %3 hz) [%4]").
                        arg(
                            e.sAudioQuality.toUpper(),
                            sSubtype
                        ).
                        arg(e.uiSampleRate).
                        arg(UnitsFormat::bytes(e.ui64Size)),
                        e.mtMediaType
                    );
                }
                else {
                    // The current media entry contains video (and maybe audio as well).
                    ui->txtLog->appendPlainText(
                        QStringLiteral("Video(%1)/Audio(%2) %3x%4 (%5 fps) %6 [%7]\n"
                                       "%8 bps, %9 hz").
                        arg(
                            e.sVideoQuality,
                            MediaType::MT_VIDEO_ONLY==e.mtMediaType?
                                QStringLiteral("N/A"):
                                e.sAudioQuality
                        ).
                        arg(e.uiWidth).
                        arg(e.uiHeight).
                        arg(e.uiFPS).
                        arg(
                            UnitsFormat::seconds(e.uiDuration/1000),
                            UnitsFormat::bytes(e.ui64Size)
                        ).
                        arg(e.uiBitrate).
                        arg(e.uiSampleRate)
                    );
                    // Adds the video details to the list of downloadable formats.
                    ui->cboMediaFormats->addItem(
                        QStringLiteral("%1 (%2 %3x%4) [%5]").
                        arg(
                            e.sVideoQuality.toUpper(),
                            sSubtype
                        ).
                        arg(e.uiWidth).
                        arg(e.uiHeight).
                        arg(UnitsFormat::bytes(e.ui64Size)),
                        e.mtMediaType
                    );
                }
                ui->txtLog->appendPlainText("");
            }
            ui->cboMediaFormats->setCurrentIndex(-1);
            this->showThumbnail();
            ui->spbIgnoreFirst->setValue(0);
            ui->spbIgnoreFirst->setMaximum(vdLoad.uiDuration/1000);
            ui->spbClipSize->setValue(DEFAULT_CLIP_SIZE);
            ui->spbClipSize->setMaximum(vdLoad.uiDuration/1000);
            ui->spbIgnoreLast->setValue(0);
            ui->spbIgnoreLast->setMaximum(vdLoad.uiDuration/1000);
            wgtNextFocus=ui->cboMediaFormats;
        }
    }
    this->enableControls();
    this->unsetCursor();
    if(nullptr!=wgtNextFocus)
        wgtNextFocus->setFocus();
}

void MainWindow::slot_btnDownload_clicked() {
    bool    bSuccess;
    int     iMediaIndex,iAudioIndex;
    QString sSourceVideo,sSourceAudio,sTargetVideo;
    AVTools avtMuxer;
    ui->txtLog->clear();
    iMediaIndex=-1;
    iAudioIndex=-1;
    // Searches for the selected media index, ignoring the separators.
    for(int iK=0;iK<=ui->cboMediaFormats->currentIndex();iK++)
        if(MediaType::MT_INVALID!=ui->cboMediaFormats->itemData(iK,Qt::ItemDataRole::UserRole))
            iMediaIndex++;
    if(-1!=iMediaIndex) {
        MediaEntryList melList=vdCurrentVideoDetails.melMediaEntries;
        if(MediaType::MT_VIDEO_ONLY==melList.at(iMediaIndex).mtMediaType) {
            int     i2ndBestAudioIndex=-1,
                    iWorstAudioIndex=-1;
            QString sVideoSubtype=MIMETools::getSubtype(melList.at(iMediaIndex).sMIMEType),
                    sVideoQuality=melList.at(iMediaIndex).sVideoQuality;
            // Searches for a suitable audio track in the available media.
            for(int iK=0;iK<melList.count();iK++)
                if(MediaType::MT_AUDIO_ONLY==melList.at(iK).mtMediaType) {
                    iWorstAudioIndex=iK; // Just picks any audio track;
                    if(sVideoSubtype==MIMETools::getSubtype(melList.at(iK).sMIMEType)) {
                        i2ndBestAudioIndex=iK; // picks a compatible audio track;
                        if(sVideoQuality==melList.at(iMediaIndex).sAudioQuality) {
                            iAudioIndex=iK; // the perfect audio track was found;
                            break;
                        }
                    }
                }
            if(-1==iAudioIndex) {
                // No audio track with identical media subtype and quality was found.
                if(-1==i2ndBestAudioIndex)
                    // The "worst" option could make the MUX process fail ...
                    // ... but it's the only one we have in the available media.
                    iAudioIndex=iWorstAudioIndex;
                else
                    iAudioIndex=i2ndBestAudioIndex;
            }
        }
    }
    // Performs the download of the selected media.
    if(this->btnDownloadHandler(iMediaIndex,sSourceVideo)) {
        bSuccess=false;
        ui->txtLog->appendPlainText(
            QStringLiteral("Source video: %1").arg(sSourceVideo)
        );
        sTargetVideo=QStringLiteral("%1/%2.%3").
            arg(
                ui->ledDestinationFolder->text(),
                vdCurrentVideoDetails.sVideoID,
                QFileInfo(sSourceVideo).suffix()
            );
        if(-1!=iAudioIndex) {
            ui->txtLog->appendPlainText(QStringLiteral("MUX required"));
            // Performs the download of the audio track (if it's required).
            if(this->btnDownloadHandler(iAudioIndex,sSourceAudio)) {
                ui->txtLog->appendPlainText(
                    QStringLiteral("Source audio: %1").arg(sSourceAudio)
                );
                // Combines the downloaded video and audio files.
                if(avtMuxer.saveAs(sSourceVideo,sSourceAudio,sTargetVideo)) {
                    QDir().remove(sSourceVideo);
                    QDir().remove(sSourceAudio);
                    ui->txtLog->appendPlainText(
                        QStringLiteral("Target video: %1").arg(sTargetVideo)
                    );
                    ui->txtLog->appendPlainText(QStringLiteral("Mux completed"));
                    bSuccess=true;
                }
                else
                    ui->txtLog->appendPlainText(
                        QStringLiteral("MUX process failed: %1").arg(avtMuxer.getLastError())
                    );
            }

        }
        else {
            QDir().remove(sTargetVideo);
            QDir().rename(sSourceVideo,sTargetVideo);
            if(QDir().exists(sTargetVideo)) {
                ui->txtLog->appendPlainText(
                    QStringLiteral("Target video: %1").arg(sTargetVideo)
                );
                ui->txtLog->appendPlainText(QStringLiteral("Download completed"));
                bSuccess=true;
            }
            else
                ui->txtLog->appendPlainText(
                    QStringLiteral("Unable to write to destination folder")
                );
        }
        if(bSuccess) {
            if(ui->chkSplit->isChecked()) {
                ui->txtLog->appendPlainText(QStringLiteral("Split requested"));
                // Cuts the downloaded video according to the user-selected parameters.
                this->createMultipleClips(
                    sTargetVideo,
                    vdCurrentVideoDetails.uiDuration/1000,
                    ui->spbClipSize->value(),
                    ui->spbIgnoreFirst->value(),
                    ui->spbIgnoreLast->value()
                );
            }
            ui->txtLog->appendPlainText(QStringLiteral("** Finished **"));
        }
    }
}

void MainWindow::slot_btnVideoThumbnail_clicked() {
    if(!vdCurrentVideoDetails.sVideoID.isEmpty())
        // Invokes the default browser and loads the current YT video page.
        QDesktopServices::openUrl(
            QUrl(
                YTScraper::createVideoURL(
                    vdCurrentVideoDetails.sVideoID
                )
            )
        );
}

void MainWindow::slot_chkSplit_stateChanged(int iState) {
    bool bEnabled=Qt::CheckState::Checked==iState;
    ui->spbIgnoreFirst->setEnabled(bEnabled);
    ui->spbClipSize->setEnabled(bEnabled);
    ui->spbIgnoreLast->setEnabled(bEnabled);
}

/**
 * @brief Processes the download of the selected media format.
 *
 * @param[in]  iMediaIndex      index of the selected media format
 * @param[out] sDownloadedFile  downloaded filepath (auto-generated)
 *
 * @return true if the download was successful
 */
bool MainWindow::btnDownloadHandler(int     iMediaIndex,
                                    QString &sDownloadedFile) {
    bool           bResult=false,
                   bIsVideo;
    QString        sExtension,sTempPath;
    QByteArray     abtVideoContent;
    QFile          fFile;
    QWidget        *wgtNextFocus=nullptr;
    MediaEntry     meEntry;
    sDownloadedFile.clear();
    // Cancels the active download if the button is hit before the download finishes.
    if(mpdVideoDownloader.isDownloading()) {
        mpdVideoDownloader.cancelDownload();
        while(mpdVideoDownloader.isDownloading())
            QApplication::processEvents(QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents);
    }
    else
        if(-1!=iMediaIndex) {
            meEntry=vdCurrentVideoDetails.melMediaEntries.at(iMediaIndex);
            bIsVideo=MediaType::MT_AUDIO_ONLY!=meEntry.mtMediaType;
            this->enableControls(false);
            this->setCursor(Qt::CursorShape::BusyCursor);
            ui->btnDownload->setEnabled(true);
            ui->btnDownload->setText(QStringLiteral("Stop"));
            ui->txtLog->appendPlainText(
                QStringLiteral("Downloading %1...").
                arg(bIsVideo?QStringLiteral("video"):QStringLiteral("audio"))
            );
            ui->stbMain->clearMessage();
            // Generates the download filepath based on the system's temporary files folder, ...
            // the associated YT video id and the MIME type of the selected media format.
            sTempPath=QStandardPaths::standardLocations(
                QStandardPaths::StandardLocation::TempLocation
            ).at(0);
            sExtension=MIMETools::mediaExtension(meEntry.sMIMEType);
            if(sExtension.isEmpty())
                sExtension=QStringLiteral("tmp");
            sDownloadedFile=QStringLiteral("%1/%2-%3.%4").
                            arg(
                                sTempPath,
                                vdCurrentVideoDetails.sVideoID,
                                bIsVideo?QStringLiteral("video"):QStringLiteral("audio"),
                                sExtension
                            );
            if(!meEntry.sURL.isEmpty()) {
                // Performs the download of the associated URL, granting the callback function ...
                // ... access to the widgets, by passing a pointer to the user interface.
                if(mpdVideoDownloader.download(
                    meEntry.sURL,
                    abtVideoContent,
                    myProgressCallback,
                    ui
                )) {
                    fFile.setFileName(sDownloadedFile);
                    if(fFile.open(QFile::OpenModeFlag::WriteOnly)) {
                        fFile.write(abtVideoContent);
                        if(QFileDevice::FileError::NoError==fFile.error()) {
                            ui->txtLog->appendPlainText(QStringLiteral("Success"));
                            bResult=true;
                        }
                        else
                            ui->txtLog->appendPlainText(
                                QStringLiteral("Failed: %1").arg(fFile.errorString())
                            );
                        fFile.close();
                    }
                    else
                        ui->txtLog->appendPlainText(
                            QStringLiteral("Failed: %1").arg(fFile.errorString())
                        );
                }
                else
                    if(mpdVideoDownloader.getLastError().isEmpty())
                        ui->txtLog->appendPlainText(QStringLiteral("Canceled"));
                    else
                        ui->txtLog->appendPlainText(
                            QStringLiteral("Failed: %1").arg(mpdVideoDownloader.getLastError())
                        );
            }
            ui->btnDownload->setText(QStringLiteral("Download"));
            ui->stbMain->clearMessage();
            this->enableControls();
            this->unsetCursor();
        }
        else {
            QMessageBox::critical(
                this,
                QStringLiteral(APP_NAME),
                QStringLiteral("Select a format first")
            );
            wgtNextFocus=ui->cboMediaFormats;
        }
    if(nullptr!=wgtNextFocus)
        wgtNextFocus->setFocus();
    return bResult;
}

/**
 * @brief Splits a media file in multiple standalone clips of same duration.
 *
 * This method does not uses floating point at all. All the "time" parameters
 * are unsigned integers. Hence, even if the total duration is not a multiple
 * of the clip size, there is no need of calculating the size of the last clip
 * because AVTools::saveAs() stops at the exact last timestamp, no matter if
 * the supplied end timestamp is greater.
 * The clips are saved in the same folder the source media file is in.
 *
 * @note This method is far from perfect. There are cases where the obtained results
 *       are a little different (a bit shorter/longer clips, a few frozen frames at
 *       start, etc) to what should be expected. This is basically because AVTools
 *       DOES NOT reencode frames.
 *
 * @param[in] sSourceMedia    media filepath to split
 * @param[in] uiSourceSize    size of the media file, in seconds
 * @param[in] uiClipSize      amount of seconds each clip should last
 * @param[in] uiLeadingSize   amount of seconds to ignore from the start
 * @param[in] uiTrailingSize  amount of seconds to ignore from the end
 */
void MainWindow::createMultipleClips(QString sSourceMedia,
                                     uint    uiSourceSize,
                                     uint    uiClipSize,
                                     uint    uiLeadingSize,
                                     uint    uiTrailingSize) {
    bool      bSuccess;
    uint      uiK,uiNdx;
    QString   sTargetClip;
    QFileInfo fiTarget(sSourceMedia);
    AVTools   avtClipper;
    if(fiTarget.exists())
        if(uiClipSize) {
            // Calculates the required range, in seconds.
            uiSourceSize-=uiLeadingSize+uiTrailingSize;
            if(uiClipSize>uiSourceSize)
                uiClipSize=uiSourceSize;
            // Creates de clips, starting from uiLeadingSize ...
            // ... and goes one uiClipSize at time.
            for(uiK=0,uiNdx=0;uiK<uiSourceSize;uiK+=uiClipSize) {
                sTargetClip=QStringLiteral("%1/%2.%3.%4").
                            arg(
                                fiTarget.absolutePath(),
                                fiTarget.completeBaseName()
                            ).
                            arg(++uiNdx,3,10,QChar('0')).
                            arg(fiTarget.suffix());
                bSuccess=avtClipper.saveAs(
                    sSourceMedia,
                    sTargetClip,
                    uiK+uiLeadingSize,
                    uiK+uiLeadingSize+uiClipSize
                );
                ui->txtLog->appendPlainText(
                    QStringLiteral("%1 ... %2").
                    arg(
                        sTargetClip,
                        bSuccess?
                            QStringLiteral("OK"):
                            QStringLiteral("failed: %1").arg(avtClipper.getLastError())
                    )
                );
            }
            ui->txtLog->appendPlainText(QStringLiteral("Total clips: %1").arg(uiNdx));
        }
}

/**
 * @brief Enables/disables all the UI controls in a single shot.
 *
 * @param[in] bEnable  the enable/disable condition.
 */
void MainWindow::enableControls(bool bEnable) {
    ui->btnVideoThumbnail->setEnabled(bEnable);
    ui->btnLoad->setEnabled(bEnable);
    ui->btnDownload->setEnabled(bEnable);
    ui->btnDestinationFolder->setEnabled(bEnable);
    ui->ledVideoURL->setEnabled(bEnable);
    ui->ledDestinationFolder->setEnabled(bEnable);
    ui->cboMediaFormats->setEnabled(bEnable);
    ui->chkSplit->setEnabled(bEnable);
    ui->spbIgnoreFirst->setEnabled(bEnable&&ui->chkSplit->isChecked());
    ui->spbClipSize->setEnabled(bEnable&&ui->chkSplit->isChecked());
    ui->spbIgnoreLast->setEnabled(bEnable&&ui->chkSplit->isChecked());
}

/**
 * @brief Downloads and displays the YT video thumbnail in a special button intended for this.
 *
 * @note Using MPDownloader here is kinda overkill, but it's the quickest solution.
 */
void MainWindow::showThumbnail() {
    if(!vdCurrentVideoDetails.sThumbnail.isEmpty()) {
        QByteArray   abtThumbnail;
        MPDownloader mpdThumbnail;
        // Downloads the thumbnail's binary content.
        if(mpdThumbnail.download(vdCurrentVideoDetails.sThumbnail,abtThumbnail)) {
            // Creates a QIcon object with the downloaded content.
            QImage  imgThumbnail=QImage::fromData(abtThumbnail);
            QPixmap pxmThumbnail=QPixmap::fromImage(imgThumbnail);
            QIcon   icoThumbnail(pxmThumbnail);
            if(!imgThumbnail.isNull()) {
                ui->btnVideoThumbnail->setIcon(icoThumbnail);
                // Fills up the whole button size with the thumbnail.
                ui->btnVideoThumbnail->setIconSize(
                    pxmThumbnail.scaled(
                        ui->btnVideoThumbnail->size(),
                        Qt::AspectRatioMode::KeepAspectRatio,
                        Qt::TransformationMode::SmoothTransformation
                    ).rect().size()
                );
            }
        }
    }
}
