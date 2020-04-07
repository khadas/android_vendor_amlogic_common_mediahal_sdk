#include <fstream>
#include <unistd.h>
#include <sys/time.h>
#include <cstdlib>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <sys/time.h>
#include <memory>
#include <getopt.h>
#include <chrono>
#include <AmTsPlayer.h>

using namespace std;

const int kRwSize = 188*1024;
const int kRwTimeout = 30000;

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

void video_callback(void *user_data, am_tsplayer_event *event)
{
    UNUSED(user_data);
    printf("video_callback type %d\n", event? event->type : 0);
	switch (event->type) {
        case AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED:
        {
            printf("[evt] AM_TSPLAYER_EVENT_TYPE_VIDEO_CHANGED: %d x %d @%d [%d]\n",
                event->event.video_format.frame_width,
                event->event.video_format.frame_height,
                event->event.video_format.frame_rate,
                event->event.video_format.frame_aspectratio);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_USERDATA_AFD:
        case AM_TSPLAYER_EVENT_TYPE_USERDATA_CC:
        {
            uint8_t* pbuf = event->event.mpeg_user_data.data;
            uint32_t size = event->event.mpeg_user_data.len;
            printf("[evt] USERDATA [%d] : %x-%x-%x-%x %x-%x-%x-%x ,size %d\n",
                event->type, pbuf[0], pbuf[1], pbuf[2], pbuf[3],
                pbuf[4], pbuf[5], pbuf[6], pbuf[7], size);
            UNUSED(pbuf);
            UNUSED(size);
            break;
        }
        case AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME:
        {
            printf("[evt] AM_TSPLAYER_EVENT_TYPE_FIRST_FRAME\n");
            break;
        }
        default:
            break;
	}
}

static int set_osd_blank(int blank)
{
    const char *path1 = "/sys/class/graphics/fb0/blank";
    const char *path2 = "/sys/class/graphics/fb1/blank";
    const char *path3 = "/sys/class/graphics/fb0/osd_display_debug";
    int fd;
	char cmd[128] = {0};

	fd = open(path3,O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd >= 0)
	{
       sprintf(cmd,"%d",1);
	   write (fd,cmd,strlen(cmd));
	   close(fd);
	}
	fd = open(path1,O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd >= 0)
	{
       sprintf(cmd,"%d",blank);
	   write (fd,cmd,strlen(cmd));
	   close(fd);
	}
	fd = open(path2,O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd >= 0)
	{
       sprintf(cmd,"%d",blank);
	   write (fd,cmd,strlen(cmd));
	   close(fd);
	}
    return 0;
}

void signHandler(int iSignNo)
{
    UNUSED(iSignNo);
    set_osd_blank(0);
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

static void usage(char **argv)
{
    printf("Usage: %s\n", argv[0]);
    printf("Version 0.1\n");
    printf("[options]:\n");
    printf("-i | --in           Ts file path\n");
    printf("-t | --tstype       demod:0, memory:1[default]\n");
    printf("-y | --avsync       amaster:0[default], vmaster:1, pcrmaster:2, nosync:3\n");
    printf("-c | --vtrick       none:0[default], pause:1, pause next:2, Ionly:3\n");
    printf("-v | --vcodec       unknown:0, mpeg1:1, mpeg2:2, h264:3[default], h265:4, vp9:5\n");
    printf("-a | --acodec       unknown:0, mp2:1, mp3:2, ac3:3, eac3:4, dts:5, aac:6[default], latm:7, pcm:8\n");
    printf("-V | --vpid         video pid, default 0x100\n");
    printf("-A | --apid         audio pid, default 0x101\n");
    printf("-h | --help         print this usage\n");
}

int main(int argc, char **argv)
{
    int optionChar = 0;
    int optionIndex = 0;
    const char *shortOptions = "i:t:b:y:c:v:a:V:A:h";
    struct option longOptions[] = {
        { "in",             required_argument,  NULL, 'i' },
        { "tstype",         required_argument,  NULL, 't' },
        { "buftype",        required_argument,  NULL, 'b' },
        { "avsync",         required_argument,  NULL, 'y' },
        { "videotrick",     required_argument,  NULL, 'c' },
        { "vcodec",         required_argument,  NULL, 'v' },
        { "acodec",         required_argument,  NULL, 'a' },
        { "vpid",           required_argument,  NULL, 'V' },
        { "apid",           required_argument,  NULL, 'A' },
        { "help",           no_argument,        NULL, 'h' },
        { NULL,             0,                  NULL,  0  },
    };

    std::string inputTsName("/data/1.ts");
    am_tsplayer_input_source_type tsType = TS_MEMORY;
    am_tsplayer_input_buffer_type drmmode = TS_INPUT_BUFFER_TYPE_NORMAL;
    am_tsplayer_avsync_mode avsyncMode= TS_SYNC_AMASTER;
    am_tsplayer_video_trick_mode vTrickMode = AV_VIDEO_TRICK_MODE_NONE;
    am_tsplayer_video_codec vCodec = AV_VIDEO_CODEC_H264;
    am_tsplayer_audio_codec aCodec = AV_AUDIO_CODEC_AAC;
    int32_t vPid = 0x100;
    int32_t aPid = 0x101;

    while ((optionChar = getopt_long(argc, argv, shortOptions,
                                    longOptions, &optionIndex)) != -1) {
        switch (optionChar) {
            case 'i':
                inputTsName.assign((const char*)optarg);
                break;
            case 't':
                tsType = static_cast<am_tsplayer_input_source_type>(atoi(optarg));
                break;
            case 'y':
                avsyncMode = static_cast<am_tsplayer_avsync_mode>(atoi(optarg));
                break;
            case 'c':
                vTrickMode = static_cast<am_tsplayer_video_trick_mode>(atoi(optarg));
                break;
            case 'v':
                vCodec = static_cast<am_tsplayer_video_codec>(atoi(optarg));
                break;
            case 'a':
                aCodec = static_cast<am_tsplayer_audio_codec>(atoi(optarg));
                break;
            case 'V':
                vPid = atoi(optarg);
                break;
            case 'A':
                aPid = atoi(optarg);
                break;
                break;
            case 'h':
                usage(argv);
                exit(-1);
            default:
                break;
        }
    }

    signal(SIGINT, signHandler);
    set_osd_blank(1);
    char* buf = new char[kRwSize];
	uint64_t fsize = 0;
    ifstream file(inputTsName.c_str(), ifstream::binary);
	if (tsType)	{
        file.seekg(0, file.end);
        fsize = file.tellg();
        if (fsize <= 0) {
            printf("file %s size %lld return\n", inputTsName.c_str(), fsize);
            return 0;
        }
        file.seekg(0, file.beg);
	}
    printf("file name = %s, is_open %d, size %lld, tsType %d\n",
                inputTsName.c_str(), file.is_open(), fsize, tsType);

    am_tsplayer_handle session;
    am_tsplayer_init_params parm = {tsType, drmmode, 0, 0};
    AmTsPlayer_create(parm, &session);
    uint32_t versionM, versionL;
    AmTsPlayer_getVersion(&versionM, &versionL);
    uint32_t instanceno;
    AmTsPlayer_getInstansNo(session, &instanceno);
    AmTsPlayer_setWorkMode(session, TS_PLAYER_MODE_NORMAL);
    AmTsPlayer_registerCb(session, video_callback, NULL);

    AmTsPlayer_setSyncMode(session, avsyncMode);

    am_tsplayer_video_params vparm;
    vparm.codectype = vCodec;
    vparm.pid = vPid;
    AmTsPlayer_setVideoParams(session, &vparm);
    AmTsPlayer_startVideoDecoding(session);

    am_tsplayer_audio_params aparm;
    aparm.codectype = aCodec;
    aparm.pid = aPid;
    AmTsPlayer_setAudioParams(session, &aparm);
    AmTsPlayer_startAudioDecoding(session);

    AmTsPlayer_showVideo(session);
    AmTsPlayer_setTrickMode(session, vTrickMode);

    am_tsplayer_input_buffer ibuf = {TS_INPUT_BUFFER_TYPE_NORMAL, (char*)buf, 0};
    long pos = 0;
    while (tsType)
    {
        if (file.eof()) {
            printf("file read eof\n");
            break;
        }
        file.read(buf, (int)kRwSize);
        ibuf.buf_size = kRwSize;
        pos += kRwSize;

        int retry = 100;
        am_tsplayer_result res;
        do {
            res = AmTsPlayer_writeData(session, &ibuf, kRwTimeout);
            if (res == AM_TSPLAYER_ERROR_RETRY) {
                usleep(50000);
            } else
                break;
        } while(res || retry-- > 0);
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));

    delete [](buf);
    if (file.is_open())
        file.close();

    set_osd_blank(0);
    printf("exit\n");
    return 0;
}

