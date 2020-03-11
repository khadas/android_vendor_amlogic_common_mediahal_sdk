#ifndef AM_VIDEO_DEC_BASE_H
#define AM_VIDEO_DEC_BASE_H

#include <stdint.h>

class AmVideoDecCallback {
public:
    virtual ~AmVideoDecCallback() {};
    virtual void onOutputFormatChanged(uint32_t requested_num_of_buffers,
                int32_t width, uint32_t height);
    virtual void onOutputBufferDone(int32_t pictureBufferId, int32_t bitstreamId,
                uint32_t width, uint32_t height);
    virtual void onInputBufferDone(int32_t bitstream_buffer_id);
    virtual void onUpdateDecInfo(const uint8_t* info, uint32_t isize);
    virtual void onFlushDone();
    virtual void onResetDone();
    virtual void onError(int32_t error);
    virtual void onUserdataReady(const uint8_t* userdata, uint32_t usize);
    virtual void onEvent(uint32_t event, void* param, uint32_t paramsize);
};

class AmVideoDecBase {

public:
    AmVideoDecBase(AmVideoDecCallback* callback) { (void)&callback; };
    virtual ~AmVideoDecBase() {};

    virtual int32_t initialize(const char* mime, uint8_t* config, uint32_t configLen,
            bool secureMode, bool useV4l2 = 1);
    virtual int32_t setQueueCount(uint32_t queueCount);
    virtual int32_t queueInputBuffer(int32_t bitstreamId, int ashmemFd, off_t offset,
            uint32_t bytesUsed, uint64_t timestamp);
    virtual int32_t queueInputBuffer(int32_t bitstreamId, uint8_t* pbuf,
            off_t offset, uint32_t bytesUsed, uint64_t timestamp);
    virtual int32_t setupOutputBufferNum(uint32_t numOutputBuffers);
    virtual int32_t createOutputBuffer(uint32_t pictureBufferId,
                    int32_t dmabufFd, bool nv21 = 1, int32_t metaFd = -1);
    virtual int32_t createOutputBuffer(uint32_t pictureBufferId,
                    uint8_t* buf, size_t size, bool nv21 = 1);
    virtual int32_t queueOutputBuffer(int32_t pictureBufferId);
    virtual void flush();
    virtual void reset();
    virtual void destroy();
    virtual int32_t sendCommand(uint32_t index, void* param, uint32_t size);

    /* Ion output for non-bufferqueue */
    virtual int32_t allocIonBuffer(size_t size, void** mapaddr, int* fd = 0);
    virtual int32_t freeIonBuffer(void* mapaddr);
    virtual int32_t freeAllIonBuffer();
};

extern "C" AmVideoDecBase* AmVideoDec_create(AmVideoDecCallback* callback);
extern "C" uint32_t AmVideoDec_getVersion(uint32_t* versionM, uint32_t* verionL);

#endif  // AM_VIDEO_DEC_BASE_H
