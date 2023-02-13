#include <stdio.h>
#include "libavutil/avutil.h"

int main(int argc, char *argv[]) {
    printf("hello world\n");
    av_log_set_level(AV_LOG_DEBUG);
    av_log(NULL, AV_LOG_INFO, "hello ffmpeg\n");
    
    return 0;
}