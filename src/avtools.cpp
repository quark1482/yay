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

#include "avtools.h"

AVTools::AVTools() {
    sLastError.clear();
}

/**
 * @brief Gets the last error that has occurred.
 *
 * @return the last error
 */
QString AVTools::getLastError() {
    return sLastError;
}

/**
 * @brief Saves a copy of the input video.
 *
 * The output video format is inferred from the output video path extension.
 *
 * @param[in]  sIn   input video path
 * @param[out] sOut  output video path
 *
 * @return true if no errors ocurred during the video processing
 */
bool AVTools::saveAs(QString sIn,QString sOut) {
    return this->saveAs(sIn,sOut,0,0);
}

/**
 * @brief Saves a clip of the input video using the given time interval.
 *
 * The output video format is inferred from the output video path extension.
 *
 * @note Tested with the following containers: MP4, MKV, MOV, WMV and TS.
 *       Other ones may need some code tweaks.
 *       *Streams ARE NOT reencoded*
 *
 * @todo For better cutting, See http://kicherer.org/joomla/index.php/de/blog/
 *       42-avcut-frame-accurate-video-cutting-with-only-small-quality-loss
 *
 * @param[in] sIn         input video path
 * @param[in] sOut        output video path
 * @param[in] fStartTime  start timestamp, in seconds
 * @param[in] fEndTime    end timestamp, in seconds
 *
 * @return true if no errors ocurred during the video processing
 */
bool AVTools::saveAs(QString sIn,QString sOut,float fStartTime,float fEndTime) {
    bool            bResult=false;
    int             iError=0;
    uint            uiK;
    char            szError[AV_ERROR_MAX_STRING_SIZE];
    float           *fLastTime=NULL;
    int64_t         *i64StartDTS=NULL;
    AVFormatContext *fcIn=NULL,
                    *fcOut=NULL;
    AVStream        *stIn=NULL,
                    *stOut=NULL;
    AVPacket        *pkIn=NULL;
    sLastError.clear();
    try {
        if(0>fStartTime||0>fEndTime||0>fEndTime-fStartTime)
            throw std::exception("invalid time range");
        // Opens the first (source) supplied container.
        fcIn=avformat_alloc_context();
        if(NULL==fcIn)
            throw std::exception("avformat_alloc_context(in) failed");
        iError=avformat_open_input(&fcIn,sIn.toUtf8().data(),NULL,NULL);
        if(0>iError)
            throw std::exception("avformat_alloc_output_context2(in) failed");
        iError=avformat_find_stream_info(fcIn,NULL);
        if(0>iError)
            throw std::exception("avformat_find_stream_info(in) failed");
        // Opens (for writing) the second (target) supplied container ...
        // ... and creates the same number of streams as source (same codecs).
        iError=avformat_alloc_output_context2(&fcOut,NULL,NULL,sOut.toUtf8().data());
        if(0>iError)
            throw std::exception("avformat_alloc_output_context2(out) failed");
        i64StartDTS=(int64_t *)av_mallocz(sizeof(int64_t)*fcIn->nb_streams);
        fLastTime=(float *)av_mallocz(sizeof(float)*fcIn->nb_streams);
        for(uiK=0;uiK<fcIn->nb_streams;uiK++) {
            stIn=fcIn->streams[uiK];
            stOut=avformat_new_stream(fcOut,NULL);
            if(NULL==stOut)
                throw std::exception("avformat_new_stream(out) failed");
            iError=avcodec_parameters_copy(stOut->codecpar,stIn->codecpar);
            if(0>iError)
                throw std::exception("avcodec_parameters_copy(out,in) failed");
            stOut->codecpar->codec_tag=0;
            // Holds an initial decompression timestamp per each stream.
            i64StartDTS[uiK]=AV_NOPTS_VALUE;
        }
        if(!(fcOut->flags&AVFMT_NOFILE)) {
            iError=avio_open(&fcOut->pb,sOut.toUtf8().data(),AVIO_FLAG_WRITE);
            if(0>iError)
                throw std::exception("avio_open(out) failed");
        }
        // Moves to the first frame in the selected time range.
        iError=av_seek_frame(fcIn,-1,fStartTime*AV_TIME_BASE,AVSEEK_FLAG_ANY);
        if(0>iError)
            throw std::exception("av_seek_frame(in) failed");
        // Copies the source packets into the target container.
        iError=avformat_write_header(fcOut,NULL);
        if(0>iError)
            throw std::exception("avformat_write_header(out) failed");
        while(true) {
            pkIn=av_packet_alloc();
            if(NULL==pkIn)
                throw std::exception("av_packet_alloc(in) failed");
            iError=av_read_frame(fcIn,pkIn);
            if(0>iError) {
                if(AVERROR_EOF==iError)
                    break;
                else
                    throw std::exception("av_read_frame(in) failed");
            }
            stIn=fcIn->streams[pkIn->stream_index];
            stOut=fcOut->streams[pkIn->stream_index];
            // Ignores the copy when the packet timestamp exceeds the selected time range.
            fLastTime[pkIn->stream_index]=av_q2d(stIn->time_base)*pkIn->pts;
            if(!fEndTime||fLastTime[pkIn->stream_index]<=fEndTime) {
                if(AV_NOPTS_VALUE==i64StartDTS[pkIn->stream_index])
                    i64StartDTS[pkIn->stream_index]=pkIn->dts;
                // Adjusts the timestamps based on the first DTS found in each stream ...
                // ... so the requested clip duration is correctly calculated.
                if(fStartTime||fEndTime) {
                    pkIn->pts-=i64StartDTS[pkIn->stream_index]-(pkIn->pts-pkIn->dts);
                    pkIn->dts-=i64StartDTS[pkIn->stream_index];
                }
                av_packet_rescale_ts(pkIn,stIn->time_base,stOut->time_base);
                pkIn->pos=-1;
                iError=av_interleaved_write_frame(fcOut,pkIn);
                if(0>iError)
                    throw std::exception("av_interleaved_write_frame(out) failed");
            }
            av_packet_free(&pkIn);
            // Finishes the copy when every stream reaches the end of the selected time range.
            for(uiK=0;uiK<fcIn->nb_streams;uiK++)
                if(!fEndTime||fLastTime[uiK]<=fEndTime)
                    break;
            if(uiK==fcIn->nb_streams)
                break;
        }
        iError=av_write_trailer(fcOut);
        if(0>iError)
            throw std::exception("av_write_trailer(out) failed");
        bResult=true;
    }
    catch(const std::exception &exE) {
        sLastError=QStringLiteral("%1").arg(exE.what());
        if(0>iError) {
            av_strerror(iError,szError,sizeof(szError));
            sLastError.append(QStringLiteral(" %1").arg(szError));
        }
    }
    if(NULL!=fcIn)
        avformat_close_input(&fcIn);
    if(NULL!=fcOut) {
        if(!(fcOut->flags&AVFMT_NOFILE))
            avio_closep(&fcOut->pb);
        else
            avformat_close_input(&fcOut);
    }
    if(NULL!=i64StartDTS)
        av_free(i64StartDTS);
    if(NULL!=fLastTime)
        av_free(fLastTime);
    if(NULL!=pkIn)
        av_packet_free(&pkIn);
    return bResult;
}

/**
 * @brief Combines the input video and audio streams into a single video.
 *
 * The output video format is inferred from the output video path extension.
 *
 * @note Tested with the following containers: MP4, MKV, MOV, WMV and TS.
 *       Other ones may need some code tweaks.
 *       *Streams ARE NOT reencoded*
 *
 * @param[in] sVideoIn  input video stream path
 * @param[in] sAudioIn  input audio stream path
 * @param[in] sOut      output video path
 *
 * @return true if no errors ocurred during the video processing
 */
bool AVTools::saveAs(QString sVideoIn,QString sAudioIn,QString sOut) {
    bool            bResult=false,
                    bVideoInEOF=false,
                    bAudioInEOF=false;
    int             iError=0;
    char            szError[AV_ERROR_MAX_STRING_SIZE];
    AVFormatContext *fcVideoIn=NULL,
                    *fcAudioIn=NULL,
                    *fcOut=NULL;
    AVStream        *stVideoIn=NULL,
                    *stAudioIn=NULL,
                    *stVideoOut=NULL,
                    *stAudioOut=NULL;
    AVPacket        *pkVideoIn=NULL,
                    *pkAudioIn=NULL;
    sLastError.clear();
    try {
        // Finds the first video stream in the first (source) supplied container.
        fcVideoIn=avformat_alloc_context();
        if(NULL==fcVideoIn)
            throw std::exception("avformat_alloc_context(video in) failed");
        iError=avformat_open_input(&fcVideoIn,sVideoIn.toUtf8().data(),NULL,NULL);
        if(0>iError)
            throw std::exception("avformat_alloc_output_context2(video in) failed");
        iError=avformat_find_stream_info(fcVideoIn,NULL);
        if(0>iError)
            throw std::exception("avformat_find_stream_info(video in) failed");
        for(uint uiK=0;uiK<fcVideoIn->nb_streams;uiK++)
            if(AVMEDIA_TYPE_VIDEO==fcVideoIn->streams[uiK]->codecpar->codec_type) {
                stVideoIn=fcVideoIn->streams[uiK];
                break;
            }
        if(NULL==stVideoIn)
            throw std::exception("Could not find any video stream in the video input file");
        // Finds the first audio stream in the second (source) supplied container.
        fcAudioIn=avformat_alloc_context();
        if(NULL==fcAudioIn)
            throw std::exception("avformat_alloc_context(audio in) failed");
        iError=avformat_open_input(&fcAudioIn,sAudioIn.toUtf8().data(),NULL,NULL);
        if(0>iError)
            throw std::exception("avformat_alloc_output_context2(audio in) failed");
        iError=avformat_find_stream_info(fcAudioIn,NULL);
        if(0>iError)
            throw std::exception("avformat_find_stream_info(audio in) failed");
        for(uint uiK=0;uiK<fcAudioIn->nb_streams;uiK++)
            if(AVMEDIA_TYPE_AUDIO==fcAudioIn->streams[uiK]->codecpar->codec_type) {
                stAudioIn=fcAudioIn->streams[uiK];
                break;
            }
        if(NULL==stAudioIn)
            throw std::exception("Could not find any audio stream in the audio input file");
        // Opens (for writing) the third (target) supplied container ...
        // ... and creates both video and audio streams inside (same codecs as sources).
        iError=avformat_alloc_output_context2(&fcOut,NULL,NULL,sOut.toUtf8().data());
        if(0>iError)
            throw std::exception("avformat_alloc_output_context2(out) failed");
        stVideoOut=avformat_new_stream(fcOut,NULL);
        if(NULL==stVideoOut)
            throw std::exception("avformat_new_stream(video out) failed");
        iError=avcodec_parameters_copy(stVideoOut->codecpar,stVideoIn->codecpar);
        if(0>iError)
            throw std::exception("avcodec_parameters_copy(video out,video in) failed");
        stVideoOut->codecpar->codec_tag=0;
        stAudioOut=avformat_new_stream(fcOut,NULL);
        if(NULL==stAudioOut)
            throw std::exception("avformat_new_stream(audio out) failed");
        iError=avcodec_parameters_copy(stAudioOut->codecpar,stAudioIn->codecpar);
        if(0>iError)
            throw std::exception("avcodec_parameters_copy(audio out,audio in) failed");
        stAudioOut->codecpar->codec_tag=0;
        if(!(fcOut->flags&AVFMT_NOFILE)) {
            iError=avio_open(&fcOut->pb,sOut.toUtf8().data(),AVIO_FLAG_WRITE);
            if(0>iError)
                throw std::exception("avio_open(out) failed");
        }
        // Copies the source packets (video/audio, alternately) into the target container.
        iError=avformat_write_header(fcOut,NULL);
        if(0>iError)
            throw std::exception("avformat_write_header(out) failed");
        while(!bVideoInEOF||!bAudioInEOF) {
            if(!bVideoInEOF) {
                pkVideoIn=av_packet_alloc();
                if(NULL==pkVideoIn)
                    throw std::exception("av_packet_alloc(video in) failed");
                iError=av_read_frame(fcVideoIn,pkVideoIn);
                if(0>iError)
                    if(AVERROR_EOF==iError)
                        bVideoInEOF=true;
                    else
                        throw std::exception("av_read_frame(video in) failed");
                else {
                    av_packet_rescale_ts(pkVideoIn,stVideoIn->time_base,stVideoOut->time_base);
                    pkVideoIn->pos=-1;
                    pkVideoIn->stream_index=stVideoOut->index;
                    iError=av_interleaved_write_frame(fcOut,pkVideoIn);
                    if(0>iError)
                        throw std::exception("av_interleaved_write_frame(video out) failed");
                }
                av_packet_free(&pkVideoIn);
            }
            if(!bAudioInEOF) {
                pkAudioIn=av_packet_alloc();
                if(NULL==pkAudioIn)
                    throw std::exception("av_packet_alloc(audio in) failed");
                iError=av_read_frame(fcAudioIn,pkAudioIn);
                if(0>iError)
                    if(AVERROR_EOF==iError)
                        bAudioInEOF=true;
                    else
                        throw std::exception("av_read_frame(audio in) failed");
                else {
                    av_packet_rescale_ts(pkAudioIn,stAudioIn->time_base,stAudioOut->time_base);
                    pkAudioIn->pos=-1;
                    pkAudioIn->stream_index=stAudioOut->index;
                    iError=av_interleaved_write_frame(fcOut,pkAudioIn);
                    if(0>iError)
                        throw std::exception("av_interleaved_write_frame(audio out) failed");
                }
                av_packet_free(&pkAudioIn);
            }
        }
        iError=av_write_trailer(fcOut);
        if(0>iError)
            throw std::exception("av_write_trailer(out) failed");
        bResult=true;
    }
    catch(const std::exception &exE) {
        sLastError=QStringLiteral("%1").arg(exE.what());
        if(0>iError) {
            av_strerror(iError,szError,sizeof(szError));
            sLastError.append(QStringLiteral(" %1").arg(szError));
        }
    }
    if(NULL!=fcVideoIn)
        avformat_close_input(&fcVideoIn);
    if(NULL!=fcAudioIn)
        avformat_close_input(&fcAudioIn);
    if(NULL!=fcOut) {
        if(!(fcOut->flags&AVFMT_NOFILE))
            avio_closep(&fcOut->pb);
        else
            avformat_close_input(&fcOut);
    }
    if(NULL!=pkVideoIn)
        av_packet_free(&pkVideoIn);
    if(NULL!=pkAudioIn)
        av_packet_free(&pkAudioIn);
    return bResult;
}
