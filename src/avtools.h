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

#ifndef AVTOOLS_H
#define AVTOOLS_H

#include <QtCore>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
}

/**
 * @brief The AVTools class.
 *
 * Provides basic support for media file operations such as:
 * -Converting from one media container to another
 * -Cutting a media file between two timestamps
 * -Joining video and audio files in a single media file
 */
class AVTools {
private:
    QString sLastError;
public:
    AVTools();
    QString getLastError();
    bool    saveAs(QString,QString);
    bool    saveAs(QString,QString,float,float);
    bool    saveAs(QString,QString,QString);
};

#endif // AVTOOLS_H
