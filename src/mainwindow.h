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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include "avtools.h"
#include "mpdownloader.h"
#include "unitsformat.h"
#include "ytscraper.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow:public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget * =nullptr);
    ~MainWindow();
protected:
    void closeEvent(QCloseEvent *) override;
    bool eventFilter(QObject *,QEvent *) override;
private slots:
    void slot_btnDestinationFolder_clicked();
    void slot_btnLoad_clicked();
    void slot_btnDownload_clicked();
    void slot_btnVideoThumbnail_clicked();
    void slot_chkSplit_stateChanged(int);
private:
    VideoDetails   vdCurrentVideoDetails;
    YTScraper      ytsVideoScraper;
    MPDownloader   mpdVideoDownloader;
    bool           bFocusIsInVideoURL;
    Ui::MainWindow *ui;
    bool btnDownloadHandler(int,QString &);
    void createMultipleClips(QString,uint,uint,uint=0,uint=0);
    void enableControls(bool=true);
    void showThumbnail();
};
#endif // MAINWINDOW_H
