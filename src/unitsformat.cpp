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

#include "unitsformat.h"

/**
 * @brief Converts an amount of seconds into a human-readable string.
 *
 * Resulting string unit names are: years, months, days, hours, minutes and seconds.
 * Leading zero-values (e.g., "0 years, 0 months, ...") are not included.
 *
 * @param[in] ui64TotalSecs    the amount of seconds
 * @param[in] bShortUnitNames  use short unit names when possible
 *
 * @return the human-readable string
 */
QString UnitsFormat::seconds(quint64 ui64TotalSecs,bool bShortUnitNames) {
    QString sResult;
    quint64 ui64Years,ui64Months,ui64Days,ui64Hours,ui64Minutes,ui64Seconds,ui64Remainder;
    sResult.clear();
    ui64Remainder=ui64TotalSecs;
    ui64Years=ui64Remainder/31104000;
    ui64Remainder=ui64Remainder%31104000;
    ui64Months=ui64Remainder/2592000;
    ui64Remainder=ui64Remainder%2592000;
    ui64Days=ui64Remainder/86400;
    ui64Remainder=ui64Remainder%86400;
    ui64Hours=ui64Remainder/3600;
    ui64Remainder=ui64Remainder%3600;
    ui64Minutes=ui64Remainder/60;
    ui64Seconds=ui64Remainder%60;
    if(ui64Years||!sResult.isEmpty()) {
        if(!sResult.isEmpty())
            sResult.append(QStringLiteral(", "));
        sResult.append(QStringLiteral("%1 year").arg(ui64Years));
        if(ui64Years!=1)
            sResult.append(QStringLiteral("s"));
    }
    if(ui64Months||!sResult.isEmpty()) {
        if(!sResult.isEmpty())
            sResult.append(QStringLiteral(", "));
        sResult.append(QStringLiteral("%1 month").arg(ui64Months));
        if(ui64Months!=1)
            sResult.append(QStringLiteral("s"));
    }
    if(ui64Days||!sResult.isEmpty()) {
        if(!sResult.isEmpty())
            sResult.append(QStringLiteral(", "));
        sResult.append(QStringLiteral("%1 day").arg(ui64Days));
        if(ui64Days!=1)
            sResult.append(QStringLiteral("s"));
    }
    if(ui64Hours||!sResult.isEmpty()) {
        if(!sResult.isEmpty())
            sResult.append(QStringLiteral(", "));
        sResult.append(QStringLiteral("%1 hour").arg(ui64Hours));
        if(ui64Hours!=1)
            sResult.append(QStringLiteral("s"));
    }
    if(ui64Minutes||!sResult.isEmpty()) {
        if(!sResult.isEmpty())
            sResult.append(QStringLiteral(", "));
        sResult.append(
            QStringLiteral("%1 %2").
            arg(ui64Minutes).
            arg(bShortUnitNames?QStringLiteral("min"):QStringLiteral("minute"))
        );
        if(ui64Minutes!=1)
            sResult.append(QStringLiteral("s"));
    }
    if(!sResult.isEmpty())
        sResult.append(QStringLiteral(", "));
    sResult.append(
        QStringLiteral("%1 %2").
        arg(ui64Seconds).
        arg(bShortUnitNames?QStringLiteral("sec"):QStringLiteral("second"))
    );
    if(ui64Seconds!=1)
        sResult.append(QStringLiteral("s"));
    return(sResult);
}

/**
 * @brief Converts an amount of bytes into a human-readable string.
 *
 * Resulting string unit names are: TB, GB, MB, KB and Bytes.
 *
 * @param[in] ui64TotalBytes  the amount of bytes
 *
 * @return the human-readable string
 */
QString UnitsFormat::bytes(quint64 ui64TotalBytes) {
    QString sResult;
    quint64 ui64TB,ui64GB,ui64MB,ui64KB,ui64B;
    sResult.clear();
    ui64TB=ui64TotalBytes;
    if(ui64TB>=1099511627776)
        sResult=QStringLiteral("%1TB").arg(qreal(ui64TB)/1099511627776,0,'f',2);
    else {
        ui64GB=ui64TB;
        if(ui64GB>=1073741824)
            sResult=QStringLiteral("%1GB").arg(qreal(ui64GB)/1073741824,0,'f',2);
        else {
            ui64MB=ui64GB;
            if(ui64MB>=1048576)
                sResult=QStringLiteral("%1MB").arg(qreal(ui64MB)/1048576,0,'f',2);
            else {
                ui64KB=ui64MB;
                if(ui64KB>=1024)
                    sResult=QStringLiteral("%1KB").arg(qreal(ui64KB)/1024,0,'f',2);
                else {
                    ui64B=ui64KB;
                    sResult=QStringLiteral("%1 Bytes").arg(ui64B);
                }
            }
        }
    }
    return sResult;
}
