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

#include "ytscraper.h"

/**
 * @brief User-Agent header to be used by the internal QWebEnginePage.
 *
 * @todo Make this a property.
 */
#define YTS_HEADER_USER_AGENT_DEFAULT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/101.0.4951.67 Safari/537.36"

/**
 * @brief YT root domain, to create and validate links.
 */
#define YTS_HOST_MAIN "youtube.com"

/*
 * @brief YT video resource path, to create and validate links.
 */
#define YTS_RES_WATCH_VIDEO "/watch"

/**
 * @brief Barebones JSON validation - don't take it too seriously.
 */
#define REGEX_JSON "[{\\[]{1}(?:[,:{}\\[\\]0-9.\\-+Eaeflnr-u \\n\\r\\t]|\".*?\")+[}\\]]{1}"

/**
 * @brief Locates the JSON config options in the video HTML page
 *        where the URL of the video player script is referenced.
 *
 * The config options definition has this form: ytcfg.set({...});
 *
 * @note For a sample, see http://jsonblob.com/1033985156129767424
 */
#define REGEX_YTCFG \
"ytcfg\\.set\\s*\\(\\s*(?<json>" \
REGEX_JSON \
")\\s*\\)\\s*;"

/**
 * @brief Locates the JSON video details and available media links in the video HTML page.
 *
 * The video details definition has this form: ytInitialPlayerResponse = {...};
 *
 * @note For a sample, see http://jsonblob.com/1033986119666253824
 */
#define REGEX_YTIPR \
"var\\s+ytInitialPlayerResponse\\s*=\\s*(?<json>" \
REGEX_JSON \
")\\s*;"

/**
 * @brief JSON attribute name containing the video player JS code URL.
 *
 * This value is found inside the JSON config options in the video HTML page.
 */
#define YTS_PLAYER_FIELD "PLAYER_JS_URL"

YTScraper::YTScraper() {
    sLastError.clear();
    namYTS=new QNetworkAccessManager();
    webPlayer.profile()->setHttpUserAgent(QStringLiteral(YTS_HEADER_USER_AGENT_DEFAULT));
}

YTScraper::~YTScraper() {
    delete namYTS;
}

/**
 * @brief Checks if the supplied video details are valid enough to work with.
 *
 * @param[in] vdSource  video details
 *
 * @return true if a video id AND at least one valid media entry were found.
 */
bool YTScraper::checkVideoDetails(const VideoDetails vdSource) {
    return !vdSource.sVideoID.isEmpty()&&!vdSource.melMediaEntries.isEmpty();
}

/**
 * @brief Clears the supplied video details structure.
 *
 * @param[out] vdTarget  video details
 */
void YTScraper::clearVideoDetails(VideoDetails &vdTarget) {
    vdTarget.sVideoID.clear();
    vdTarget.sTitle.clear();
    vdTarget.sDescription.clear();
    vdTarget.sThumbnail.clear();
    vdTarget.uiDuration=0;
    vdTarget.melMediaEntries.clear();
}

/**
 * @brief Copies the video details from one structure to another.
 *
 * It just makes the code more readable, but the logic could be extended in the future.
 *
 * @param[out] vdTarget  target video details structure
 * @param[in]  vdSource  source video details structure
 */
void YTScraper::copyVideoDetails(VideoDetails &vdTarget,
                                 const VideoDetails vdSource){
    vdTarget=vdSource;
}

/**
 * @brief Creates a YT video URL from a video id.
 *
 * @param[in] sVideoId  the video id
 *
 * @return the YT video URL
 */
QString YTScraper::createVideoURL(QString sVideoId) {
    QUrl      urlVideo;
    QUrlQuery qryVideo;
    urlVideo.setUrl(
        QStringLiteral("https://%1%2").
        arg(
            QStringLiteral(YTS_HOST_MAIN),
            QStringLiteral(YTS_RES_WATCH_VIDEO)
        )
    );
    qryVideo.addQueryItem(
        QStringLiteral("v"),
        sVideoId
    );
    urlVideo.setQuery(qryVideo.query());
    return urlVideo.toString();
}

/**
 * @brief Gets the last error that has occurred.
 *
 * @return the last error
 */
QString YTScraper::getLastError() {
    return sLastError;
}

/**
 * @brief Gets all the required details and available media links from a given YT video.
 *
 * @param[in]  sVideoId        video id
 * @param[out] vdVideoDetails  extracted video details
 *
 * @return true if the video details were found, with at least one valid media entry
 */
bool YTScraper::getVideoDetails(QString      sVideoId,
                                VideoDetails &vdVideoDetails) {
    bool                    bResult=false;
    QString                 sHTML,sJSON,sPlayerURL;
    QRegularExpression      rxConfig,rxIPR;
    QRegularExpressionMatch rxmConfigMatch,rxmIPRMatch;
    QJsonDocument           jsnDoc;
    QJsonObject             jsnObj;
    QJsonParseError         jsnErr;
    sLastError.clear();
    clearVideoDetails(vdVideoDetails);
    // Loads the video HTML page.
    if(this->getVideoHTML(sVideoId,sHTML,sLastError)) {
        sPlayerURL.clear();
        rxConfig.setPattern(QStringLiteral(REGEX_YTCFG));
        rxmConfigMatch=rxConfig.match(sHTML);
        // Looks for the JSON config options.
        if(rxmConfigMatch.hasMatch()) {
            sJSON=rxmConfigMatch.captured(QStringLiteral("json"));
            jsnDoc=QJsonDocument::fromJson(sJSON.toUtf8(),&jsnErr);
            if(jsnDoc.isNull())
                sLastError=jsnErr.errorString();
            else
                if(jsnDoc.isObject()) {
                    jsnObj=jsnDoc.object();
                    // Looks for the video player JS code URL
                    if(jsnObj.contains(QStringLiteral(YTS_PLAYER_FIELD)))
                        if(jsnObj.value(QStringLiteral(YTS_PLAYER_FIELD)).isString())
                            sPlayerURL=jsnObj.value(
                                QStringLiteral(YTS_PLAYER_FIELD)
                            ).toString();
                }
            if(!sPlayerURL.isEmpty())
                // Configures the signatures dechipering engine.
                bResult=this->setDecipherEngine(sPlayerURL,sLastError);
            else
                sLastError=QStringLiteral("Unexpected JSON content");
        }
        else
            sLastError=QStringLiteral("Unexpected HTML content");
        if(bResult) {
            bResult=false;
            rxIPR.setPattern(QStringLiteral(REGEX_YTIPR));
            rxmIPRMatch=rxIPR.match(sHTML);
            // Looks for the JSON video details and available media links.
            if(rxmIPRMatch.hasMatch()) {
                sJSON=rxmIPRMatch.captured(QStringLiteral("json"));
                // Extracts all details and values.
                if(this->parseQueryVideoResponse(
                    sJSON,
                    vdVideoDetails,
                    sLastError
                )) {
                    quint64    ui64ContentLength;
                    QString    sError,sContentType;
                    // Verifies that each returned link is valid and contains the correct media.
                    for(auto &e:vdVideoDetails.melMediaEntries)
                        if(this->getVideoHeaders(
                            e.sURL,
                            sContentType,
                            ui64ContentLength,
                            sError
                        )) {
                            if(!e.ui64Size)
                                e.ui64Size=ui64ContentLength;
                            // Extra-checks that the collected media entry size ...
                            // ... matches the size of the actual file.
                            if(ui64ContentLength!=e.ui64Size) {
                                qDebug() << "Ignored media"
                                         << "Tag:" << e.uiFormatTag
                                         << "Mismatching content-length"
                                         << "Expected:" << e.ui64Size
                                         << "Found:" << ui64ContentLength;
                                e.ui64Size=0;
                            }
                            // Infers the MIME type of the media entry from the HTTP headers ...
                            // ... in case it was not available in the JSON video details.
                            if(MediaType::MT_INVALID==e.mtMediaType) {
                                e.sMIMEType=sContentType;
                                if(MIMETools::isType(e.sMIMEType,QStringLiteral("video")))
                                    if(e.uiSampleRate)
                                        e.mtMediaType=MediaType::MT_VIDEO_AND_AUDIO;
                                    else
                                        e.mtMediaType=MediaType::MT_VIDEO_ONLY;
                                else
                                    if(MIMETools::isType(e.sMIMEType,QStringLiteral("audio")))
                                        e.mtMediaType=MediaType::MT_AUDIO_ONLY;
                            }
                            // Extra-checks that the collected media entry MIME type ...
                            // ... matches the MIME type of the actual file.
                            if(1>MIMETools::compare(e.sMIMEType,sContentType)) {
                                qDebug() << "Ignored media"
                                         << "Tag:" << e.uiFormatTag
                                         << "Mismatching content-type"
                                         << "Expected:" << e.sMIMEType
                                         << "Found:" << sContentType;
                                e.mtMediaType=MediaType::MT_INVALID;
                            }
                        }
                        else
                            e.ui64Size=0;
                    // Removes the invalid / non-downloadable media entries.
                    vdVideoDetails.melMediaEntries.removeIf(
                        [](const auto &e) {
                            return !e.ui64Size||(MediaType::MT_INVALID==e.mtMediaType);
                        }
                    );
                    bResult=true;
                }
            }
            else
                sLastError=QStringLiteral("Unexpected HTML content");
        }
    }
    return bResult;
}

/**
 * @brief Gets the media type and size from the supplied YT direct-download URL.
 *
 * Performs a quick HTTP HEAD request on a YT direct-download media URL
 * to obtain both "Content-Type" and "Content-Length" headers.
 *
 * @param[in]  sVideoURL          URL of the media file, ready to download
 * @param[out] sContentType       value of the "Content-Type" header (if available)
 * @param[out] ui64ContentLength  value of the "Content-Length" header (if available)
 * @param[out] sError             any communication or parsing error during/after the request
 *
 * @return true if the HEAD request succeeds (type and size can still be unavailable)
 */
bool YTScraper::getVideoHeaders(QString sVideoURL,
                                QString &sContentType,
                                quint64 &ui64ContentLength,
                                QString &sError) {
    bool            bResult=false;
    uint            uiResCode;
    QNetworkRequest nrqRequest;
    QNetworkReply   *nrpReply;
    sContentType.clear();
    ui64ContentLength=0;
    sError.clear();
    nrqRequest.setUrl(QUrl(sVideoURL));
    nrqRequest.setHeader(
        QNetworkRequest::KnownHeaders::UserAgentHeader,
        QStringLiteral(YTS_HEADER_USER_AGENT_DEFAULT)
    );
    nrpReply=namYTS->head(nrqRequest);
    while(!nrpReply->isFinished())
        QApplication::processEvents(QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents);
    uiResCode=nrpReply->attribute(
        QNetworkRequest::Attribute::HttpStatusCodeAttribute
    ).toUInt();
    if(QNetworkReply::NetworkError::NoError!=nrpReply->error())
        sError=nrpReply->errorString();
    else
        if(200==uiResCode) {
            sContentType=nrpReply->header(
                QNetworkRequest::KnownHeaders::ContentTypeHeader
            ).toString();
            ui64ContentLength=nrpReply->header(
                QNetworkRequest::KnownHeaders::ContentLengthHeader
            ).toULongLong();
            bResult=true;
        }
        else
            sError=QStringLiteral("Unexpected response code: %1").arg(uiResCode);
    nrpReply->~QNetworkReply();
    return bResult;
}

/**
 * @brief Downloads the source code (HTML) of the YT video page.
 *
 * @param[in]  sVideoId    video id
 * @param[out] sVideoHTML  downloaded video HTML page contents
 * @param[out] sError      any communication or parsing error during/after the download
 *
 * @return true if the HTML code was successfully downloaded
 */
bool YTScraper::getVideoHTML(QString sVideoId,
                             QString &sVideoHTML,
                             QString &sError) {
    bool            bResult=false;
    uint            uiResCode;
    QString         sContentType;
    QNetworkRequest nrqRequest;
    QNetworkReply   *nrpReply;
    sVideoHTML.clear();
    sError.clear();
    nrqRequest.setUrl(QUrl(createVideoURL(sVideoId)));
    nrqRequest.setHeader(
        QNetworkRequest::KnownHeaders::UserAgentHeader,
        QStringLiteral(YTS_HEADER_USER_AGENT_DEFAULT)
    );
    nrpReply=namYTS->get(nrqRequest);
    while(!nrpReply->isFinished())
        QApplication::processEvents(QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents);
    uiResCode=nrpReply->attribute(
        QNetworkRequest::Attribute::HttpStatusCodeAttribute
    ).toUInt();
    sContentType=nrpReply->header(
        QNetworkRequest::KnownHeaders::ContentTypeHeader
    ).toString();
    if(QNetworkReply::NetworkError::NoError!=nrpReply->error())
        sError=nrpReply->errorString();
    else
        if(200==uiResCode)
            if(0==sContentType.indexOf(QStringLiteral("text/html"))) {
                sVideoHTML=nrpReply->readAll();
                bResult=true;
            }
            else
                sError=QStringLiteral("Unexpected content type: %1").arg(sContentType);
        else
            sError=QStringLiteral("Unexpected response code: %1").arg(uiResCode);
    nrpReply->~QNetworkReply();
    return bResult;
}

/**
 * @brief Identifies the signature-decoding function inside the video player JS code.
 *
 * @note See https://github.com/ytdl-org/youtube-dl/blob/master/youtube_dl/extractor/youtube.py
 *
 * @param[in]  sPlayerSource  video player JS code
 * @param[out] sFunctionName  name of the signature-decoding function
 *
 * @return true if the function name was found
 */
bool YTScraper::getVideoPlayerDecipherFunctionName(QString sPlayerSource,
                                                   QString &sFunctionName) {
    const QStringList slRegExes={
        QStringLiteral("\\b[cs]\\s*&&\\s*[adf]\\.set\\([^,]+\\s*,\\s*encodeURIComponent\\s*\\(\\s*(?P<name>[a-zA-Z0-9$]+)\\("),
        QStringLiteral("\\b[a-zA-Z0-9]+\\s*&&\\s*[a-zA-Z0-9]+\\.set\\([^,]+\\s*,\\s*encodeURIComponent\\s*\\(\\s*(?P<name>[a-zA-Z0-9$]+)\\("),
        QStringLiteral("\\bm=(?P<name>[a-zA-Z0-9$]{2,})\\(decodeURIComponent\\(h\\.s\\)\\)"),
        QStringLiteral("\\bc&&\\(c=(?P<name>[a-zA-Z0-9$]{2,})\\(decodeURIComponent\\(c\\)\\)"),
        QStringLiteral("(?:\\b|[^a-zA-Z0-9$])(?P<name>[a-zA-Z0-9$]{2,})\\s*=\\s*function\\(\\s*a\\s*\\)\\s*{\\s*a\\s*=\\s*a\\.split\\(\\s*\"\"\\s*\\);[a-zA-Z0-9$]{2}\\.[a-zA-Z0-9$]{2}\\(a,\\d+\\)"),
        QStringLiteral("(?:\\b|[^a-zA-Z0-9$])(?P<name>[a-zA-Z0-9$]{2,})\\s*=\\s*function\\(\\s*a\\s*\\)\\s*{\\s*a\\s*=\\s*a\\.split\\(\\s*\"\"\\s*\\)"),
        QStringLiteral("(?P<name>[a-zA-Z0-9$]+)\\s*=\\s*function\\(\\s*a\\s*\\)\\s*{\\s*a\\s*=\\s*a\\.split\\(\\s*\"\"\\s*\\)")
    };
    bool                    bResult=false;
    QRegularExpression      rxFunction;
    QRegularExpressionMatch rxmFunctionMatch;
    sFunctionName.clear();
    // Takes each one of the previous regular expressions ...
    for(const auto &s:slRegExes) {
        rxFunction.setPattern(s);
        rxmFunctionMatch=rxFunction.match(sPlayerSource);
        // ... and searches for a match in the whole JS document.
        if(rxmFunctionMatch.hasMatch()) {
            sFunctionName=rxmFunctionMatch.captured(QStringLiteral("name"));
            bResult=true;
            break;
        }
    }
    return bResult;
}

/**
 * @brief Downloads the source code (JS) of the YT video player.
 *
 * @param[in]  sPlayerURL     video player URL (extracted from the video HTML page)
 * @param[out] sPlayerSource  downloaded video player JS code
 * @param[out] sError         any communication or parsing error during/after the download
 *
 * @return true if JS code was found in the supplied URL
 */
bool YTScraper::getVideoPlayerSource(QString sPlayerURL,
                                     QString &sPlayerSource,
                                     QString &sError) {
    bool            bResult=false;
    uint            uiResCode;
    QString         sContentType;
    QUrl            urlPlayer;
    QNetworkRequest nrqRequest;
    QNetworkReply   *nrpReply;
    sPlayerSource.clear();
    sError.clear();
    urlPlayer.setUrl(sPlayerURL);
    if(urlPlayer.scheme().isEmpty())
        urlPlayer.setScheme(QStringLiteral("https"));
    if(urlPlayer.host().isEmpty())
        urlPlayer.setHost(YTS_HOST_MAIN);
    nrqRequest.setUrl(urlPlayer.toString());
    nrqRequest.setHeader(
        QNetworkRequest::KnownHeaders::UserAgentHeader,
        QStringLiteral(YTS_HEADER_USER_AGENT_DEFAULT)
    );
    nrpReply=namYTS->get(nrqRequest);
    while(!nrpReply->isFinished())
        QApplication::processEvents(QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents);
    uiResCode=nrpReply->attribute(
        QNetworkRequest::Attribute::HttpStatusCodeAttribute
    ).toUInt();
    sContentType=nrpReply->header(
        QNetworkRequest::KnownHeaders::ContentTypeHeader
    ).toString();
    if(QNetworkReply::NetworkError::NoError!=nrpReply->error())
        sError=nrpReply->errorString();
    else
        if(200==uiResCode)
            if(0==sContentType.indexOf(QStringLiteral("text/javascript"))) {
                sPlayerSource=nrpReply->readAll();
                bResult=true;
            }
            else
                sError=QStringLiteral("Unexpected content type: %1").arg(sContentType);
        else
            sError=QStringLiteral("Unexpected response code: %1").arg(uiResCode);
    nrpReply->~QNetworkReply();
    return bResult;
}

/**
 * @brief Decodes a ciphered signature by running a tampered YT video player JS code.
 *
 * @param[in]  sCiphered    ciphered signature
 * @param[out] sDeciphered  deciphered signature
 *
 * @return true if the signature-decoding function was successfully invoked
 */
bool YTScraper::getVideoSignature(QString sCiphered,
                                  QString &sDeciphered) {
    bool    bResult=false;
    int     iJSResult;
    QString sJSEnvelope,sPlayerObj;
    sLastError.clear();
    sDeciphered.clear();
    // Gets the video player global object name from the custom property.
    sPlayerObj=webPlayer.property("objName").toString();
    // Creates a JS function which returns an object with two properties:
    // -"ready" is set to 1 once the ciphered signature is decoded.
    // -"value" is set to the deciphered signature.
    sJSEnvelope=QStringLiteral(
        "(function() {"
            "var jResult={ready:0,value:0};"
            "jResult.value=%1.decipher(\"%2\");"
            "jResult.ready=1;"
            "return jResult;"
        "}());"
    ).arg(sPlayerObj,sCiphered);
    iJSResult=0;
    // Runs the JS function and expects everything's OK.
    webPlayer.runJavaScript(
        sJSEnvelope,
        [&](const QVariant &v) {
            QJsonObject jsonObj=v.toJsonObject();
            if(jsonObj.contains(QStringLiteral("ready"))) {
                iJSResult=jsonObj.value(QStringLiteral("ready")).toInt();
                if(jsonObj.contains(QStringLiteral("value")))
                    sDeciphered=jsonObj.value(QStringLiteral("value")).toString();
                bResult=!sDeciphered.isEmpty();
            }
        }
    );
    while(!iJSResult)
        QApplication::processEvents(QEventLoop::ProcessEventsFlag::EventLoopExec);
    return bResult;
}

/**
 * @brief Takes the JSON video details and available media links and extracts
 *        title, duration, etc., and other values for all available media.
 *
 * @param[in]  sJSON           JSON video details (found inside the video HTML page)
 * @param[out] vdVideoDetails  all details collected
 * @param[out] sError          any parsing error during the JSON processing
 *
 * @return true if a video id AND at least one valid media entry were collected.
 */
bool YTScraper::parseQueryVideoResponse(QString      sJSON,
                                        VideoDetails &vdVideoDetails,
                                        QString      &sError) {
    bool            bResult=false;
    QJsonDocument   jsnDoc;
    QJsonObject     jsnObj,jsnObj2;
    QJsonArray      jsnFormats,jsnFormats2,jsnThumbnails;
    QJsonParseError jsnErr;
    MediaEntry      meEntry;
    clearVideoDetails(vdVideoDetails);
    sError.clear();
    jsnDoc=QJsonDocument::fromJson(sJSON.toUtf8(),&jsnErr);
    if(jsnDoc.isNull())
        sError=jsnErr.errorString();
    else
        if(jsnDoc.isObject()) {
            jsnObj=jsnDoc.object();
            // Makes an unified array of media formats either constant or adaptive.
            // Everything can be downloaded directly, but adaptive formats will ...
            // ... require muxing since they have separated audio and video streams.
            if(jsnObj.contains(QStringLiteral("streamingData")))
                if(jsnObj.value(QStringLiteral("streamingData")).isObject()) {
                    jsnObj2=jsnObj.value(
                        QStringLiteral("streamingData")
                    ).toObject();
                    if(jsnObj2.contains(QStringLiteral("formats")))
                        if(jsnObj2.value(QStringLiteral("formats")).isArray())
                            jsnFormats=jsnObj2.value(
                                QStringLiteral("formats")
                            ).toArray();
                    if(jsnObj2.contains(QStringLiteral("adaptiveFormats")))
                        if(jsnObj2.value(QStringLiteral("adaptiveFormats")).isArray()) {
                            jsnFormats2=jsnObj2.value(
                                QStringLiteral("adaptiveFormats")
                            ).toArray();
                            for(const auto &o:qAsConst(jsnFormats2))
                                jsnFormats.append(o);
                        }
                }
            // Collects the required video details.
            if(jsnObj.contains(QStringLiteral("videoDetails")))
                if(jsnObj.value(QStringLiteral("videoDetails")).isObject()) {
                    jsnObj2=jsnObj.value(
                        QStringLiteral("videoDetails")
                    ).toObject();
                    if(jsnObj2.contains(QStringLiteral("lengthSeconds")))
                        if(jsnObj2.value(QStringLiteral("lengthSeconds")).isString())
                            vdVideoDetails.uiDuration=jsnObj2.value(
                                QStringLiteral("lengthSeconds")
                            ).toString().toUInt()*1000;
                    if(jsnObj2.contains(QStringLiteral("videoId")))
                        if(jsnObj2.value(QStringLiteral("videoId")).isString())
                            vdVideoDetails.sVideoID=jsnObj2.value(
                                QStringLiteral("videoId")
                            ).toString();
                    if(jsnObj2.contains(QStringLiteral("title")))
                        if(jsnObj2.value(QStringLiteral("title")).isString())
                            vdVideoDetails.sTitle=jsnObj2.value(
                                QStringLiteral("title")
                            ).toString();
                    if(jsnObj2.contains(QStringLiteral("shortDescription")))
                        if(jsnObj2.value(QStringLiteral("shortDescription")).isString())
                            vdVideoDetails.sDescription=jsnObj2.value(
                                QStringLiteral("shortDescription")
                            ).toString();
                    if(jsnObj2.contains(QStringLiteral("thumbnail")))
                        if(jsnObj2.value(QStringLiteral("thumbnail")).isObject()) {
                            jsnObj2=jsnObj2.value(
                                QStringLiteral("thumbnail")
                            ).toObject();
                            if(jsnObj2.contains(QStringLiteral("thumbnails")))
                                if(jsnObj2.value(QStringLiteral("thumbnails")).isArray())
                                    jsnThumbnails=jsnObj2.value(
                                        QStringLiteral("thumbnails")
                                    ).toArray();
                        }
                }
        }
    if(!jsnFormats.isEmpty())
        // Uses the previous unified array of media formats ...
        // ... to collect specific values for every entry
        for(const auto &f:qAsConst(jsnFormats))
            if(f.isObject()) {
                jsnObj=f.toObject();
                meEntry.mtMediaType=MediaType::MT_INVALID;
                meEntry.sURL.clear();
                meEntry.sMIMEType.clear();
                meEntry.sVideoQuality.clear();
                meEntry.sAudioQuality.clear();
                meEntry.uiFormatTag=0;
                meEntry.uiBitrate=0;
                meEntry.uiSampleRate=0;
                meEntry.uiWidth=0;
                meEntry.uiHeight=0;
                meEntry.uiFPS=0;
                meEntry.uiDuration=0;
                meEntry.ui64Size=0;
                // An available "url" attribute means that the media is free to download.
                if(jsnObj.contains(QStringLiteral("url")))
                    if(jsnObj.value(QStringLiteral("url")).isString())
                        meEntry.sURL=jsnObj.value(
                            QStringLiteral("url")
                        ).toString();
                // A missing "url" attribute means that the media is "protected" ...
                // ... and the value must be inferred from the signature.
                if(meEntry.sURL.isEmpty())
                    if(jsnObj.contains(QStringLiteral("signatureCipher")))
                        if(jsnObj.value(QStringLiteral("signatureCipher")).isString()) {
                            QString   sSignatureParamKey,sSignatureCiphered,sSignatureDeciphered;
                            QUrl      urlVideo;
                            QUrlQuery qryVideo;
                            // "signatureCipher" is basically the query part of an URL.
                            qryVideo.setQuery(
                                jsnObj.value(QStringLiteral("signatureCipher")).toString()
                            );
                            // Extracts the ciphered signature.
                            sSignatureCiphered=qryVideo.queryItemValue(QStringLiteral("s"));
                            // Extracts the signature "key" that will hold the decoded ...
                            // ... signature in the resulting media download URL.
                            sSignatureParamKey=qryVideo.queryItemValue(QStringLiteral("sp"));
                            // Decodes the ciphered signature.
                            if(this->getVideoSignature(sSignatureCiphered,sSignatureDeciphered)) {
                                // Creates the media download URL.
                                urlVideo.setUrl(
                                    qryVideo.queryItemValue(
                                        QStringLiteral("url"),
                                        QUrl::ComponentFormattingOption::FullyDecoded
                                    )
                                );
                                qryVideo.setQuery(urlVideo.query());
                                // Adds the "key=decoded signature" pair to the URL query.
                                qryVideo.addQueryItem(sSignatureParamKey,sSignatureDeciphered);
                                urlVideo.setQuery(qryVideo);
                                // Adds the now downloadable URL to the media format entry.
                                meEntry.sURL=urlVideo.url();
                            }
                        }
                // Collects the other values, when available.
                if(jsnObj.contains(QStringLiteral("quality")))
                    if(jsnObj.value(QStringLiteral("quality")).isString())
                        meEntry.sVideoQuality=jsnObj.value(
                            QStringLiteral("quality")
                        ).toString();
                if(jsnObj.contains(QStringLiteral("audioQuality")))
                    if(jsnObj.value(QStringLiteral("audioQuality")).isString())
                        meEntry.sAudioQuality=jsnObj.value(
                            QStringLiteral("audioQuality")
                        ).toString();
                if(jsnObj.contains(QStringLiteral("itag")))
                    if(jsnObj.value(QStringLiteral("itag")).isDouble())
                        meEntry.uiFormatTag=jsnObj.value(
                            QStringLiteral("itag")
                        ).toInt();
                if(jsnObj.contains(QStringLiteral("bitrate")))
                    if(jsnObj.value(QStringLiteral("bitrate")).isDouble())
                        meEntry.uiBitrate=jsnObj.value(
                            QStringLiteral("bitrate")
                        ).toInt();
                if(jsnObj.contains(QStringLiteral("audioSampleRate")))
                    if(jsnObj.value(QStringLiteral("audioSampleRate")).isString())
                        meEntry.uiSampleRate=jsnObj.value(
                            QStringLiteral("audioSampleRate")
                        ).toString().toUInt();
                if(jsnObj.contains(QStringLiteral("width")))
                    if(jsnObj.value(QStringLiteral("width")).isDouble())
                        meEntry.uiWidth=jsnObj.value(
                            QStringLiteral("width")
                        ).toInt();
                if(jsnObj.contains(QStringLiteral("height")))
                    if(jsnObj.value(QStringLiteral("height")).isDouble())
                        meEntry.uiHeight=jsnObj.value(
                            QStringLiteral("height")
                        ).toInt();
                if(jsnObj.contains(QStringLiteral("fps")))
                    if(jsnObj.value(QStringLiteral("fps")).isDouble())
                        meEntry.uiFPS=jsnObj.value(
                            QStringLiteral("fps")
                        ).toInt();
                if(jsnObj.contains(QStringLiteral("approxDurationMs")))
                    if(jsnObj.value(QStringLiteral("approxDurationMs")).isString())
                        meEntry.uiDuration=jsnObj.value(
                            QStringLiteral("approxDurationMs")
                        ).toString().toUInt();
                if(jsnObj.contains(QStringLiteral("contentLength")))
                    if(jsnObj.value(QStringLiteral("contentLength")).isString())
                        meEntry.ui64Size=jsnObj.value(
                            QStringLiteral("contentLength")
                        ).toString().toULongLong();
                if(jsnObj.contains(QStringLiteral("mimeType")))
                    if(jsnObj.value(QStringLiteral("mimeType")).isString()) {
                        meEntry.sMIMEType=jsnObj.value(
                            QStringLiteral("mimeType")
                        ).toString();
                        if(MIMETools::isType(meEntry.sMIMEType,QStringLiteral("video")))
                            if(meEntry.uiSampleRate)
                                // If the media entry's MIME type is "video" but the attribute ...
                                // ... "audioSampleRate" was found and it's a non-zero vaue ...
                                // ... then the respective media includes both video and audio.
                                meEntry.mtMediaType=MediaType::MT_VIDEO_AND_AUDIO;
                            else
                                meEntry.mtMediaType=MediaType::MT_VIDEO_ONLY;
                        else
                            if(MIMETools::isType(meEntry.sMIMEType,QStringLiteral("audio")))
                                meEntry.mtMediaType=MediaType::MT_AUDIO_ONLY;
                    }
                if(0==meEntry.sAudioQuality.compare(QStringLiteral("AUDIO_QUALITY_LOW")))
                    meEntry.sAudioQuality=QStringLiteral("low");
                else if(0==meEntry.sAudioQuality.compare(QStringLiteral("AUDIO_QUALITY_MEDIUM")))
                    meEntry.sAudioQuality=QStringLiteral("medium");
                else if(0==meEntry.sAudioQuality.compare(QStringLiteral("AUDIO_QUALITY_HIGH")))
                     meEntry.sAudioQuality=QStringLiteral("high");
                // An existing URL is the only requisite for a media entry to be acceptable.
                if(!meEntry.sURL.isEmpty())
                    vdVideoDetails.melMediaEntries.append(meEntry);
            }
    // For simplicity, extracts the first video thumbnail.
    // The higher the index, the higher the resolution ...
    // ... but that's not important in this case.
    if(!jsnThumbnails.isEmpty())
        if(jsnThumbnails.at(0).isObject()) {
            jsnObj2=jsnThumbnails.at(0).toObject();
            if(jsnObj2.contains(QStringLiteral("url")))
                if(jsnObj2.value(QStringLiteral("url")).isString())
                    vdVideoDetails.sThumbnail=jsnObj2.value(
                        QStringLiteral("url")
                    ).toString();
        }
    bResult=checkVideoDetails(vdVideoDetails);
    if(!bResult)
        if(sError.isEmpty())
            sError=QStringLiteral("Unexpected JSON content");
    return bResult;
}

/**
 * @brief Extracts the video player source code "logical" sections.
 *
 * A basic YT video player code looks like this:
 * var obj={};(function(param){...})(obj);
 * Where header is exactly "var obj={};(function(param){",
 * body is "..." (LONG text), and footer is "})(obj);".
 * By separating these sections, we may insert our own code right after the body
 * and/or directly call the player functions ourselves.
 *
 * @note See https://regex101.com/r/uQ1tjK/1
 *
 * @param [in]  sPlayerSource  the video player JS code
 * @param [out] sSourceHeader  the header section
 * @param [out] sSourceBody    the body section
 * @param [out] sSourceFooter  the footer section
 * @param [out] sPlayerObj     the video player global object name
 * @param [out] sPlayerParam   the video player associated local parameter name
 * @param [out] sFunctionName  the signature-decoding function name
 *
 * @return true if the supplied video player JS code follows the expected structure
 */
bool YTScraper::parseVideoPlayerSource(QString sPlayerSource,
                                       QString &sSourceHeader,
                                       QString &sSourceBody,
                                       QString &sSourceFooter,
                                       QString &sPlayerObj,
                                       QString &sPlayerParam,
                                       QString &sFunctionName){
    const QString sRegEx=QStringLiteral(
        "(?P<header>var\\s+(?P<player>\\w+)\\s*=\\s*{\\s*}\\s*;\\s*"
        "\\(\\s*function\\s*\\(\\s*(?P<param>\\w+)\\s*\\)\\s*{)"
        "(?P<body>.*?)"
        "(?P<footer>}\\s*\\)\\s*\\(\\s*(?P=player)\\s*\\)\\s*;)"
    );
    bool                    bResult=false;
    QRegularExpression      rxPlayer;
    QRegularExpressionMatch rxmPlayerMatch;
    sSourceHeader.clear();
    sSourceBody.clear();
    sSourceFooter.clear();
    sPlayerObj.clear();
    sPlayerParam.clear();
    sFunctionName.clear();
    // Finds the signature-decoding function name.
    if(this->getVideoPlayerDecipherFunctionName(sPlayerSource,sFunctionName)) {
        rxPlayer.setPattern(sRegEx);
        rxPlayer.setPatternOptions(QRegularExpression::PatternOption::DotMatchesEverythingOption);
        rxmPlayerMatch=rxPlayer.match(sPlayerSource);
        // Finds the other sections and names.
        if(rxmPlayerMatch.hasMatch()) {
            sSourceHeader=rxmPlayerMatch.captured(QStringLiteral("header"));
            sSourceBody=rxmPlayerMatch.captured(QStringLiteral("body"));
            sSourceFooter=rxmPlayerMatch.captured(QStringLiteral("footer"));
            sPlayerObj=rxmPlayerMatch.captured(QStringLiteral("player"));
            sPlayerParam=rxmPlayerMatch.captured(QStringLiteral("param"));
            bResult=true;
        }
    }
    return bResult;
}

/**
 * @brief Parses a YT URL and extracts the video id and list id parameters (when available).
 *
 * @param[in]  sURL      URL to be parsed
 * @param[out] sVideoId  extracted video id parameter
 * @param[out] sListId   extracted list id parameter
 *
 * @return true if at least the video id was found
 */
bool YTScraper::parseURL(QString sURL,
                         QString &sVideoId,
                         QString &sListId) {
    bool bResult=false;
    sLastError.clear();
    QUrl urlVideo(sURL);
    sVideoId.clear();
    sListId.clear();
    urlVideo=urlVideo.adjusted(QUrl::UrlFormattingOption::StripTrailingSlash);
    if(urlVideo.isValid())
        // Verifies that it's an actual YT URL
        if(0==urlVideo.host().compare(
            QStringLiteral(YTS_HOST_MAIN),
            Qt::CaseSensitivity::CaseInsensitive
        )||urlVideo.host().endsWith(
            QStringLiteral(".%1").arg(QStringLiteral(YTS_HOST_MAIN)),
            Qt::CaseSensitivity::CaseInsensitive
        ))
            // Verifies that it's an actual YT video URL
            if(0==urlVideo.path().compare(
                QStringLiteral(YTS_RES_WATCH_VIDEO),
                Qt::CaseSensitivity::CaseInsensitive))
                if(urlVideo.hasQuery()) {
                    QUrlQuery qryVideo(urlVideo.query());
                    if(qryVideo.hasQueryItem(QStringLiteral("v")))
                        sVideoId=qryVideo.queryItemValue(QStringLiteral("v"));
                    if(qryVideo.hasQueryItem(QStringLiteral("list")))
                        sListId=qryVideo.queryItemValue(QStringLiteral("list"));
                    bResult=!sVideoId.isEmpty();
                }
    if(!bResult)
        sLastError=QStringLiteral("Invalid/malformed video URL");
    return bResult;
}

/**
 * @brief Configures the internal QWebEnginePage to be used for signatures dechipering.
 *
 * The deciphering engine STAYS ready to be used in the internal QWebEnginePage.
 *
 * @param[in]  sPlayerURL  video player URL (extracted from the video HTML page)
 * @param[out] sError      any communication or parsing error during/after the configuration
 *
 * @return true if the internal QWebEnginePage was correctly configured
 */
bool YTScraper::setDecipherEngine(QString sPlayerURL,
                                  QString &sError) {
    bool    bResult=false,
            bLoadFinished=false,
            bLoadFinishedOK=false;
    QString sSource,sHeader,sBody,sFooter,sObj,sParam,sFunction;
    sError.clear();
    // Downloads the video player source code and extracts its logical sections.
    if(this->getVideoPlayerSource(sPlayerURL,sSource,sError))
        if(this->parseVideoPlayerSource(sSource,sHeader,sBody,sFooter,sObj,sParam,sFunction)) {
            QString sTamperedSource,sBodyAddendum;
            // Adds a new method, "decipher", to the video player object ...
            // ... which invokes the internal signature-decoding function.
            // Additionally, forcefully returns a custom object to identify ...
            // ... a "successfully loaded" condition.
            sBodyAddendum=QStringLiteral(
                "%1.decipher=%2;"
                "return {ready:1};"
            ).arg(sParam,sFunction);
            // Places the new code right after the "body" section.
            sTamperedSource=sHeader+sBody+sBodyAddendum+sFooter;
            connect(
                &webPlayer,
                &QWebEnginePage::loadFinished,
                [&](bool b) {
                     bLoadFinished=true; // Lambda won't be called outside setDecipherEngine().
                     bLoadFinishedOK=b;  // Nothing is going out of scope here. Ignore warnings.
                 }
            );
            // The video player global object name is the only value we need to use our ...
            // ... new "decipher" function once the internal QWebEnginePage is configured.
            // Hence, we set it to a custom property (avoiding the use of a member variable).
            webPlayer.setProperty("objName",sObj);
            // Loads the simplest working HTML code since we only want to run JS code.
            webPlayer.setHtml(QStringLiteral("<html><head></head><body></body></html>"));
            while(!bLoadFinished)
                QApplication::processEvents(QEventLoop::ProcessEventsFlag::EventLoopExec);
            disconnect(&webPlayer); // Previous lambda's not being called beyond this point.
            if(bLoadFinishedOK) {
                int iJSResult=0;
                // Runs our modified video player JS code and expects everything's OK.
                webPlayer.runJavaScript(
                    sTamperedSource,
                    [&](const QVariant &v) {
                        QJsonObject jsonObj=v.toJsonObject();
                        if(jsonObj.contains(QStringLiteral("ready")))
                            iJSResult=jsonObj.value(QStringLiteral("ready")).toInt();
                    }
                );
                while(!iJSResult)
                    QApplication::processEvents(QEventLoop::ProcessEventsFlag::EventLoopExec);
                bResult=true;
            }
        }
        else
            sError=QStringLiteral("Unable to parse the video player source");
    else
        sError=QStringLiteral("Unable to get the video player source - %1").arg(sError);
    if(!bResult)
        if(sError.isEmpty())
            sError=QStringLiteral("Unable to load the decipher engine");
    return bResult;
}
