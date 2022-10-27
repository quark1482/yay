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

#ifndef MIMETOOLS_H
#define MIMETOOLS_H

#include <QtCore>

/**
 * @brief A map of MIME type parameters (attribute --> value).
 */
typedef QMap<QString,QString> MIMEParams;

/**
 * @brief The MIMETools class.
 *
 * Provides support for parsing and comparing MIME/IANA media type strings.
 */
class MIMETools {
public:
    static int     compare(QString,QString);
    static QString getType(QString);
    static QString getSubtype(QString);
    static QString mediaExtension(QString);
    static bool    isSubtype(QString,QString);
    static bool    isType(QString,QString);
    static bool    parse(QString,QString &,QString &,MIMEParams &);
};

#endif // MIMETOOLS_H
