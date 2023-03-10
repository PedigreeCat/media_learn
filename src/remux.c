#include <stdio.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>

int main(int argc, char *argv[]) {

    // 1. 处理输入参数
    int ret = 0;
    int idx = -1;
    int i = 0;
    int stream_idx = 0;

    char *src = NULL;
    char *dst = NULL;

    AVFormatContext *pFmtCtx = NULL;
    AVFormatContext *oFmtCtx = NULL;
    const AVOutputFormat *outFmt = NULL;
    AVPacket pkt;
    int *stream_map = NULL;

    av_log_set_level(AV_LOG_DEBUG);
    if (argc < 3) {
        av_log(NULL, AV_LOG_ERROR, "arguments must be more than 3.\n");
        exit(-1);
    }
    src = argv[1];
    dst = argv[2];

    // 2. 打开多媒体文件
    if ( (ret = avformat_open_input(&pFmtCtx, src, NULL, NULL)) < 0 ) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        exit(-1);
    }

    // 4. 打开目的文件的上下文
    ret = avformat_alloc_output_context2(&oFmtCtx, NULL, NULL, dst);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        goto _ERROR;
    }

    stream_map = av_calloc(pFmtCtx->nb_streams, sizeof(int));
    if (!stream_map) {
        av_log(NULL, AV_LOG_ERROR, "NO MEMORY!\n");
        goto _ERROR;
    }

    for (i = 0; i < pFmtCtx->nb_streams; i++) {
        AVStream *outStream = NULL;
        AVStream *inStream = pFmtCtx->streams[i];
        AVCodecParameters *inCodecPar = inStream->codecpar;
        if (inCodecPar->codec_type != AVMEDIA_TYPE_AUDIO &&
            inCodecPar->codec_type != AVMEDIA_TYPE_VIDEO &&
            inCodecPar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
                stream_map[i] = -1;
                continue;
            }
        stream_map[i] = stream_idx++;
        // 5. 为目的文件，创建一个新的视频流
        outStream = avformat_new_stream(oFmtCtx, NULL);
        if (!outStream) {
            av_log(NULL, AV_LOG_ERROR, "NO MEMORY!\n");
            goto _ERROR;
        }

        avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
        outStream->codecpar->codec_tag = 0;
    }

    // 绑定
    ret = avio_open2(&oFmtCtx->pb, dst, AVIO_FLAG_WRITE, NULL, NULL);
    if (ret < 0) {
        av_log(oFmtCtx, AV_LOG_ERROR, "%s\n", av_err2str(ret));
        goto _ERROR;
    }

    // 7. 写多媒体文件头
    ret = avformat_write_header(oFmtCtx, NULL);
    if (ret < 0) {
        av_log(oFmtCtx, AV_LOG_ERROR, "%s", av_err2str(ret));
        goto _ERROR;
    }

    // 8. 从源多媒体文件中读取音频/视频/字幕数据写入到目的文件中
    while (av_read_frame(pFmtCtx, &pkt) >= 0) {
        AVStream *outStream, *inStream;
        inStream = pFmtCtx->streams[pkt.stream_index];
        if (stream_map[pkt.stream_index] < 0) {
            av_packet_unref(&pkt);
            continue;
        }
        pkt.stream_index = stream_map[pkt.stream_index];

        outStream = oFmtCtx->streams[pkt.stream_index];
        av_packet_rescale_ts(&pkt, inStream->time_base, outStream->time_base);
        pkt.pos = -1;
        av_interleaved_write_frame(oFmtCtx, &pkt);
        av_packet_unref(&pkt);
    }
    // 9. 写多媒体文件尾
    av_write_trailer(oFmtCtx);

    // 10. 释放资源
_ERROR:
    if (pFmtCtx) {
        avformat_close_input(&pFmtCtx);
        pFmtCtx = NULL;
    }
    if (oFmtCtx->pb) {
        avio_close(oFmtCtx->pb);
    }
    if (oFmtCtx) {
        avformat_free_context(oFmtCtx);
        oFmtCtx = NULL;
    }
    if (stream_map) {
        av_free(stream_map);
    }

    av_log(NULL, AV_LOG_INFO, "Exit Program\n");

    return 0;
}