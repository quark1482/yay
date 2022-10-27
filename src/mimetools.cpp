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

#include "mimetools.h"

/**
 * @brief Compares two media type strings.
 *
 * @param[in] sMIME1  the first MIME/IANA media type string
 * @param[in] sMIME2  the second MIME/IANA media type string
 *
 * @return -1 --> Different (or parsing error)
 *          0 --> Same Type
 *          1 --> Same Type/Subtype
 *          2 --> Identical (including parameters)
 */
int MIMETools::compare(QString sMIME1,
                       QString sMIME2) {
    int        iResult=-1;
    QString    sType1,sType2,sSubtype1,sSubtype2;
    MIMEParams mmpParams1,mmpParams2;
    if(MIMETools::parse(sMIME1,sType1,sSubtype1,mmpParams1)&&
       MIMETools::parse(sMIME2,sType2,sSubtype2,mmpParams2))
        if(sType1==sType2) {
            if(sSubtype1==sSubtype2)
                if(mmpParams1==mmpParams2)
                    iResult=2;
                else
                    iResult=1;
            else
                iResult=0;
        }
    return iResult;
}

/**
 * @brief Extracts the main type from a given media type string.
 *
 * @param sMIME  the MIME/IANA media type string
 *
 * @return the main media type (or an empty string if the parsing failed).
 */
QString MIMETools::getType(QString sMIME) {
    QString    sResult,sSubtype;
    MIMEParams mmpParams;
    sResult.clear();
    MIMETools::parse(sMIME,sResult,sSubtype,mmpParams);
    return sResult;
}

/**
 * @brief Extracts the subtype from a given media type string.
 *
 * @param sMIME  the MIME/IANA media type string
 *
 * @return the media subtype (or an empty string if the parsing failed).
 */
QString MIMETools::getSubtype(QString sMIME) {
    QString    sResult,sType;
    MIMEParams mmpParams;
    sResult.clear();
    MIMETools::parse(sMIME,sType,sResult,mmpParams);
    return sResult;
}

/**
 * @brief Picks a proper media file extension for a given media type string.
 *
 * The list of MIME types is far from complete and should include any type,
 * but it's more than enough for the usual video and audio types.
 *
 * @todo The implementation should be JSON BLOB-based (downloadable from a remote location).
 *
 * @note See https://mimetype.io/all-types#audio
 *       See https://mimetype.io/all-types#video
 *       See https://www.iana.org/assignments/media-types/media-types.xhtml#audio
 *       See https://www.iana.org/assignments/media-types/media-types.xhtml#video
 *
 * @param[in] sMIME  the MIME/IANA media type string
 *
 * @return the selected file extension (not including a leading ".")
 *         or an empty string if no suitable extension is found (rare)
 */
QString MIMETools::mediaExtension(QString sMIME) {
    QString    sResult,sType,sSubtype;
    MIMEParams mmpParams;
    sResult.clear();
    if(MIMETools::parse(sMIME,sType,sSubtype,mmpParams)) {
        if(0==sType.compare(QStringLiteral("audio"))) {
            if(0==sSubtype.compare(QStringLiteral("adpcm")))
                sResult=QStringLiteral("adp");
            else if(0==sSubtype.compare(QStringLiteral("aiff")))
                sResult=QStringLiteral("aif");
            else if(0==sSubtype.compare(QStringLiteral("basic")))
                sResult=QStringLiteral("au");
            else if(0==sSubtype.compare(QStringLiteral("midi")))
                sResult=QStringLiteral("mid");
            else if(0==sSubtype.compare(QStringLiteral("mp3")))
                sResult=QStringLiteral("mp3");
            else if(0==sSubtype.compare(QStringLiteral("mp4")))
                sResult=QStringLiteral("m4a");
            else if(0==sSubtype.compare(QStringLiteral("mpa")))
                sResult=QStringLiteral("mpa");
            else if(0==sSubtype.compare(QStringLiteral("mpeg")))
                sResult=QStringLiteral("mp2");
            else if(0==sSubtype.compare(QStringLiteral("mpeg3")))
                sResult=QStringLiteral("mp3");
            else if(0==sSubtype.compare(QStringLiteral("ogg")))
                sResult=QStringLiteral("ogg");
            else if(0==sSubtype.compare(QStringLiteral("opus")))
                sResult=QStringLiteral("ogg");
            else if(0==sSubtype.compare(QStringLiteral("wav")))
                sResult=QStringLiteral("wav");
            else if(0==sSubtype.compare(QStringLiteral("webm")))
                sResult=QStringLiteral("weba");
            else if(0==sSubtype.compare(QStringLiteral("x-aac")))
                sResult=QStringLiteral("aac");
            else if(0==sSubtype.compare(QStringLiteral("x-aiff")))
                sResult=QStringLiteral("aif");
            else if(0==sSubtype.compare(QStringLiteral("x-matroska")))
                sResult=QStringLiteral("mka");
            else if(0==sSubtype.compare(QStringLiteral("x-mpeg-3")))
                sResult=QStringLiteral("mp3");
            else if(0==sSubtype.compare(QStringLiteral("x-ms-wax")))
                sResult=QStringLiteral("wax");
            else if(0==sSubtype.compare(QStringLiteral("x-ms-wma")))
                sResult=QStringLiteral("wma");
            else if(0==sSubtype.compare(QStringLiteral("x-pn-realaudio")))
                sResult=QStringLiteral("ra");
            else if(0==sSubtype.compare(QStringLiteral("x-wav")))
                sResult=QStringLiteral("wav");
        }
        else if(0==sType.compare(QStringLiteral("video"))) {
            if(0==sSubtype.compare(QStringLiteral("3gpp")))
                sResult=QStringLiteral("3gp");
            else if(0==sSubtype.compare(QStringLiteral("3gpp2")))
                sResult=QStringLiteral("3g2");
            else if(0==sSubtype.compare(QStringLiteral("h261")))
                sResult=QStringLiteral("h261");
            else if(0==sSubtype.compare(QStringLiteral("h263")))
                sResult=QStringLiteral("h263");
            else if(0==sSubtype.compare(QStringLiteral("h264")))
                sResult=QStringLiteral("h264");
            else if(0==sSubtype.compare(QStringLiteral("h265")))
                sResult=QStringLiteral("h265");
            else if(0==sSubtype.compare(QStringLiteral("jpeg")))
                sResult=QStringLiteral("jpgv");
            else if(0==sSubtype.compare(QStringLiteral("jpm")))
                sResult=QStringLiteral("jpm");
            else if(0==sSubtype.compare(QStringLiteral("mj2")))
                sResult=QStringLiteral("mj2");
            else if(0==sSubtype.compare(QStringLiteral("mp2t")))
                sResult=QStringLiteral("ts");
            else if(0==sSubtype.compare(QStringLiteral("mp4")))
                sResult=QStringLiteral("mp4");
            else if(0==sSubtype.compare(QStringLiteral("mpeg")))
                sResult=QStringLiteral("mpg");
            else if(0==sSubtype.compare(QStringLiteral("ogg")))
                sResult=QStringLiteral("ogv");
            else if(0==sSubtype.compare(QStringLiteral("quicktime")))
                sResult=QStringLiteral("mov");
            else if(0==sSubtype.compare(QStringLiteral("webm")))
                sResult=QStringLiteral("webm");
            else if(0==sSubtype.compare(QStringLiteral("x-f4v")))
                sResult=QStringLiteral("f4v");
            else if(0==sSubtype.compare(QStringLiteral("x-fli")))
                sResult=QStringLiteral("fli");
            else if(0==sSubtype.compare(QStringLiteral("x-flv")))
                sResult=QStringLiteral("flv");
            else if(0==sSubtype.compare(QStringLiteral("x-m4v")))
                sResult=QStringLiteral("m4v");
            else if(0==sSubtype.compare(QStringLiteral("x-matroska")))
                sResult=QStringLiteral("mkv");
            else if(0==sSubtype.compare(QStringLiteral("x-ms-asf")))
                sResult=QStringLiteral("asf");
            else if(0==sSubtype.compare(QStringLiteral("x-ms-wm")))
                sResult=QStringLiteral("wm");
            else if(0==sSubtype.compare(QStringLiteral("x-ms-wmv")))
                sResult=QStringLiteral("wmv");
            else if(0==sSubtype.compare(QStringLiteral("x-msvideo")))
                sResult=QStringLiteral("avi");
            else if(0==sSubtype.compare(QStringLiteral("x-pn-realvideo")))
                sResult=QStringLiteral("rm");
        }
    }
    return sResult;
}

/**
 * @brief Checks if a media type string has a given media subtype.
 *
 * E.g., "video/mp4" and "audio/mp4" have the "mp4" subtype.
 *
 * @param[in] sMIME          the MIME/IANA media type string
 * @param[in] sSubtypeCheck  the media subtype to check against
 *
 * @return true if the media subtypes match
 */
bool MIMETools::isSubtype(QString sMIME,
                          QString sSubtypeCheck) {
    bool       bResult=false;
    QString    sType,sSubtype;
    MIMEParams mmpParams;
    if(MIMETools::parse(sMIME,sType,sSubtype,mmpParams))
        bResult=sSubtype==sSubtypeCheck.toLower();
    return bResult;
}

/**
 * @brief Checks if a media type string belongs to a given main media type.
 *
 * E.g., "video/H264" belongs to "video", "audio/ogg" belongs to "audio", etc.
 *
 * @param[in] sMIME       the MIME/IANA media type string
 * @param[in] sTypeCheck  the main media type to check against
 *
 * @return true if the main media types match
 */
bool MIMETools::isType(QString sMIME,
                       QString sTypeCheck) {
    bool       bResult=false;
    QString    sType,sSubtype;
    MIMEParams mmpParams;
    if(MIMETools::parse(sMIME,sType,sSubtype,mmpParams))
        bResult=sType==sTypeCheck.toLower();
    return bResult;
}

/**
 * @brief Parses a media type string into its components.
 *
 * @note See https://regex101.com/r/jHeglA/1 (expanded, used here)
 *       See https://regex101.com/r/vW49Dj/1 (same, but simplified for one-shot matching)
 *
 * @param[in]  sMIME      the MIME/IANA media type string
 * @param[out] sType      the main media type
 * @param[out] sSubtype   the secondary media type
 * @param[out] mmpParams  a map (attribute, value) of the recognized parameters
 *
 * @return true if the supplied string is a media type string
 */
bool MIMETools::parse(QString    sMIME,
                      QString    &sType,
                      QString    &sSubtype,
                      MIMEParams &mmpParams) {
    const QString sRegExMIMEToken=QStringLiteral(
        "(?:\\w+(?:[\\.\\-\\+]?\\w)*|\\*)"
    );
    const QString sRegExMIMEParam=QStringLiteral(
        "(?<param>(?<attribute>%1) *= *"
        "(?<value>%1|\"(?:(?<valid>[^\"\\\\\\p{Cc}]*)(?:\\\\\\P{Cc}(?&valid))*)\"))"
    ).arg(sRegExMIMEToken);
    const QString sRegExMIMEParamList=QStringLiteral(
        "(?<paramlist>(?: *; *%1)+)"
    ).arg(sRegExMIMEParam);
    const QString sRegExMIMEType=QStringLiteral(
        "(?<type>%1)\\/(?<subtype>%1)%2?"
    ).arg(sRegExMIMEToken,sRegExMIMEParamList);
    bool                            bResult=false;
    QString                         sParamList,sAttribute,sValue;
    QRegularExpression              rxMIME;
    QRegularExpressionMatch         rxmMIMEMatch;
    QRegularExpressionMatchIterator rxmiMIMEMatchIterator;
    sType.clear();
    sSubtype.clear();
    mmpParams.clear();
    rxMIME.setPattern(sRegExMIMEType);
    rxmMIMEMatch=rxMIME.match(sMIME);
    if(rxmMIMEMatch.hasMatch()) {
        // Types and Subtypes are always case-insensitive - See rfc2045
        sType=rxmMIMEMatch.captured(QStringLiteral("type")).toLower();
        sSubtype=rxmMIMEMatch.captured(QStringLiteral("subtype")).toLower();
        sParamList=rxmMIMEMatch.captured(QStringLiteral("paramlist"));
        rxMIME.setPattern(sRegExMIMEParam);
        rxmiMIMEMatchIterator=rxMIME.globalMatch(sParamList);
        while(rxmiMIMEMatchIterator.hasNext()) {
            rxmMIMEMatch=rxmiMIMEMatchIterator.next();
            // Attribute names are always case-insensitive - See rfc2045
            sAttribute=rxmMIMEMatch.captured(QStringLiteral("attribute")).toLower();
            sValue=rxmMIMEMatch.captured(QStringLiteral("value"));
            mmpParams.insert(sAttribute,sValue);
        }
        bResult=true;
    }
    return bResult;
}
