#include <stdio.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>

int main(int argc, char *argv[]) {

    // 1. 处理输入参数
    int ret = 0;
    int idx = -1;

    char *src = NULL;
    char *dst = NULL;

    AVFormatContext *pFmtCtx = NULL;
    AVFormatContext *oFmtCtx = NULL;
    const AVOutputFormat *outFmt = NULL;
    AVStream *outStream = NULL;
    AVStream *inStream = NULL;
    AVPacket pkt;

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

    // 3. 从多媒体文件中找到视频流
    idx = av_find_best_stream(pFmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (idx < 0) {
        av_log(pFmtCtx, AV_LOG_ERROR, "Does't include audio stream.\n");
        goto _ERROR;
    }
    // 4. 打开目的文件的上下文
    oFmtCtx = avformat_alloc_context();
    if (!oFmtCtx) {
        av_log(NULL, AV_LOG_ERROR, "No Memory!\n");
        goto _ERROR;
    }
    outFmt = av_guess_format(NULL, dst, NULL);
    oFmtCtx->oformat = outFmt;

    // 5. 为目的文件，创建一个新的视频流
    outStream = avformat_new_stream(oFmtCtx, NULL);

    // 6. 设置输出视频参数
    inStream = pFmtCtx->streams[idx];
    avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
    // 将codec_tag设置为0则会自动匹配编解码器，否则会使用tag指定的编解码器
    outStream->codecpar->codec_tag = 0;

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

    // 8. 从源多媒体文件中读取视频数据写入到目的文件中
    while (av_read_frame(pFmtCtx, &pkt) >= 0) {
        if (pkt.stream_index == idx) {
            // 音频的pts和dts是一样的，而视频可能不同(有B帧的情况)
            pkt.pts = av_rescale_q_rnd(pkt.pts, inStream->time_base, outStream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            pkt.dts = av_rescale_q_rnd(pkt.dts, inStream->time_base, outStream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            pkt.duration = av_rescale_q(pkt.duration, inStream->time_base, outStream->time_base);
            pkt.stream_index = 0;
            pkt.pos = -1;
            av_interleaved_write_frame(oFmtCtx, &pkt);
            av_packet_unref(&pkt);
        }
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

    av_log(NULL, AV_LOG_INFO, "Exit Program\n");

    return 0;
}