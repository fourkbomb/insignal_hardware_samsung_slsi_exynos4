/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file      SecFimc.cpp
 * \brief     source file for Fimc HAL MODULE
 * \author    Hyunkyung, Kim(hk310.kim@samsung.com)
 * \date      2010/10/13
 *
 * <b>Revision History: </b>
 * - 2010/10/13 : Hyunkyung, Kim(hk310.kim@samsung.com) \n
 *   Initial version
 *
 * - 2011/11/15 : Sunmi, Lee(carrotsm.lee@samsung.com) \n
 *   Adjust V4L2 architecture \n
 */

#define LOG_TAG "libfimc"
#include <cutils/log.h>

#include "SecFimc.h"

#define  FIMC2_DEV_NAME  "/dev/video2"

//#define DEBUG_LIB_FIMC

#define V4L2_BUF_TYPE_OUTPUT  V4L2_BUF_TYPE_VIDEO_OUTPUT
#define V4L2_BUF_TYPE_CAPTURE V4L2_BUF_TYPE_VIDEO_CAPTURE
#define V4L2_ROTATE           V4L2_CID_ROTATION

#define V4L2_BUF_TYPE_SRC    V4L2_BUF_TYPE_OUTPUT
#define V4L2_MEMORY_TYPE_SRC V4L2_MEMORY_USERPTR
#define V4L2_BUF_TYPE_DST    V4L2_BUF_TYPE_VIDEO_OVERLAY
#define V4L2_MEMORY_TYPE_DST V4L2_MEMORY_USERPTR

struct yuv_fmt_list yuv_list[] = {
    { "V4L2_PIX_FMT_NV12",      "YUV420/2P/LSB_CBCR",   V4L2_PIX_FMT_NV12,      12, 2 },
    { "V4L2_PIX_FMT_NV12T",     "YUV420/2P/LSB_CBCR",   V4L2_PIX_FMT_NV12T,     12, 2 },
    { "V4L2_PIX_FMT_NV21",      "YUV420/2P/LSB_CRCB",   V4L2_PIX_FMT_NV21,      12, 2 },
    { "V4L2_PIX_FMT_NV21X",     "YUV420/2P/MSB_CBCR",   V4L2_PIX_FMT_NV21X,     12, 2 },
    { "V4L2_PIX_FMT_NV12X",     "YUV420/2P/MSB_CRCB",   V4L2_PIX_FMT_NV12X,     12, 2 },
    { "V4L2_PIX_FMT_YUV420",    "YUV420/3P",            V4L2_PIX_FMT_YUV420,    12, 3 },
    { "V4L2_PIX_FMT_YUYV",      "YUV422/1P/YCBYCR",     V4L2_PIX_FMT_YUYV,      16, 1 },
    { "V4L2_PIX_FMT_YVYU",      "YUV422/1P/YCRYCB",     V4L2_PIX_FMT_YVYU,      16, 1 },
    { "V4L2_PIX_FMT_UYVY",      "YUV422/1P/CBYCRY",     V4L2_PIX_FMT_UYVY,      16, 1 },
    { "V4L2_PIX_FMT_VYUY",      "YUV422/1P/CRYCBY",     V4L2_PIX_FMT_VYUY,      16, 1 },
    { "V4L2_PIX_FMT_UV12",      "YUV422/2P/LSB_CBCR",   V4L2_PIX_FMT_NV16,      16, 2 },
    { "V4L2_PIX_FMT_UV21",      "YUV422/2P/LSB_CRCB",   V4L2_PIX_FMT_NV61,      16, 2 },
    { "V4L2_PIX_FMT_UV12X",     "YUV422/2P/MSB_CBCR",   V4L2_PIX_FMT_NV16X,     16, 2 },
    { "V4L2_PIX_FMT_UV21X",     "YUV422/2P/MSB_CRCB",   V4L2_PIX_FMT_NV61X,     16, 2 },
    { "V4L2_PIX_FMT_YUV422P",   "YUV422/3P",            V4L2_PIX_FMT_YUV422P,   16, 3 },
};

void dump_pixfmt(struct v4l2_pix_format *pix)
{
    LOGI("w: %d", pix->width);
    LOGI("h: %d", pix->height);
    LOGI("color: %x", pix->colorspace);

    switch (pix->pixelformat) {
    case V4L2_PIX_FMT_YUYV:
        LOGI ("YUYV");
        break;
    case V4L2_PIX_FMT_UYVY:
        LOGI ("UYVY");
        break;
    case V4L2_PIX_FMT_RGB565:
        LOGI ("RGB565");
        break;
    case V4L2_PIX_FMT_RGB565X:
        LOGI ("RGB565X");
        break;
    default:
        LOGI("not supported");
    }
}

void dump_crop(struct v4l2_crop *crop)
{
    LOGI("crop l: %d", crop->c.left);
    LOGI("crop t: %d", crop->c.top);
    LOGI("crop w: %d", crop->c.width);
    LOGI("crop h: %d", crop->c.height);
}

void dump_window(struct v4l2_window *win)
{
    LOGI("window l: %d", win->w.left);
    LOGI("window t: %d", win->w.top);
    LOGI("window w: %d", win->w.width);
    LOGI("window h: %d", win->w.height);
}

void v4l2_overlay_dump_state(int fd)
{
    struct v4l2_format format;
    struct v4l2_crop crop;

    format.type = V4L2_BUF_TYPE_OUTPUT;
    if (ioctl(fd, VIDIOC_G_FMT, &format) < 0)
        return;

    LOGI("dumping driver state:");
    dump_pixfmt(&format.fmt.pix);

    crop.type = format.type;
    if (ioctl(fd, VIDIOC_G_CROP, &crop) < 0)
        return;

    LOGI("input window(crop):");
    dump_crop(&crop);

    crop.type = V4L2_BUF_TYPE_CAPTURE;
    if (ioctl(fd, VIDIOC_G_CROP, &crop) < 0)
        return;

    LOGI("output crop:");
    dump_crop(&crop);

}

int fimc_v4l2_query_buf(int fd, SecBuffer *secBuf, enum v4l2_buf_type type, enum v4l2_memory memory, int buf_index, int num_plane)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(struct v4l2_buffer));

    if (MAX_DST_BUFFERS <= buf_index || MAX_PLANES <= num_plane) {
        LOGE("%s::exceed MAX! : buf_index=%d, num_plane=%d", __func__, buf_index, num_plane);
        return -1;
    }

    buf.type   = type;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index  = buf_index;

    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        LOGE("%s::VIDIOC_QUERYBUF failed, plane_cnt=%d", __func__, buf.length);
        return -1;
    }

    secBuf->size.s = buf.length;

    if ((secBuf->virt.p = (char *)mmap(0, buf.length,
            PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset)) < 0) {
            LOGE("%s::mmap failed", __func__);
        return -1;
    }
    LOGI("%s::buffers[%d] vaddr = 0x%x", __func__, buf_index, (unsigned int)secBuf->virt.p);

    return 0;
}

int fimc_v4l2_req_buf(int fd, unsigned int num_bufs, enum v4l2_buf_type type, enum v4l2_memory memory)
{
    struct v4l2_requestbuffers reqbuf;

    reqbuf.type   = type;
    reqbuf.memory = memory;
    reqbuf.count  = num_bufs;

    if (ioctl(fd, VIDIOC_REQBUFS, &reqbuf) < 0) {
        LOGE("%s::VIDIOC_REQBUFS failed", __func__);
        return -1;
    }

#ifdef DEBUG_LIB_FIMC
    LOGI("%d buffers allocated %d requested", reqbuf.count, 4);
#endif

    if (reqbuf.count < num_bufs) {
        LOGE("%s::VIDIOC_REQBUFS failed ((reqbuf.count(%d) < num_bufs(%d))",
            __func__, reqbuf.count, num_bufs);
        return -1;
    }

    return 0;
}

int fimc_v4l2_s_ctrl(int fd, int id, int value)
{
    struct v4l2_control vc;
    vc.id    = id;
    vc.value = value;

    if (ioctl(fd, VIDIOC_S_CTRL, &vc) < 0) {
        LOGE("%s::VIDIOC_S_CTRL (id=%d,value=%d) failed", __func__, id, value);
        return -1;
    }

    return 0;
}

int fimc_v4l2_set_fmt(int fd, enum v4l2_buf_type type, enum v4l2_field field, s5p_fimc_img_info *img_info, unsigned int addr)
{
    struct v4l2_framebuffer fbuf;
    struct v4l2_format      fmt;
    struct v4l2_crop        crop;
    struct fimc_buf         fimc_dst_buf;
    struct v4l2_control     vc;

    fmt.type = type;
    if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
        LOGE("%s::VIDIOC_G_FMT failed", __func__);
        return -1;
    }

    switch (fmt.type) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
        fmt.fmt.pix.width       = img_info->full_width;
        fmt.fmt.pix.height      = img_info->full_height;
        fmt.fmt.pix.pixelformat = img_info->color_space;
        fmt.fmt.pix.field       = field;
        break;
    case V4L2_BUF_TYPE_VIDEO_OVERLAY:
        if (ioctl(fd, VIDIOC_G_FBUF, &fbuf) < 0) {
            LOGE("%s::VIDIOC_G_FBUF failed", __func__);
            return -1;
        }

        fbuf.base            = (void *)addr;
        fbuf.fmt.width       = img_info->full_width;
        fbuf.fmt.height      = img_info->full_height;
        fbuf.fmt.pixelformat = img_info->color_space;

        if (ioctl(fd, VIDIOC_S_FBUF, &fbuf) < 0) {
            LOGE("%s::VIDIOC_S_FBUF (w=%d, h=%d, color=%d) failed",
                __func__,
                img_info->full_width,
                img_info->full_height,
                img_info->color_space);
            return -1;
        }

        fimc_dst_buf.base[0] = (unsigned int)img_info->buf_addr_phy_rgb_y;
        fimc_dst_buf.base[1] = (unsigned int)img_info->buf_addr_phy_cb;
        fimc_dst_buf.base[2] = (unsigned int)img_info->buf_addr_phy_cr;

        vc.id    = V4L2_CID_DST_INFO;
        vc.value = (unsigned int)&fimc_dst_buf.base[0];

        if (ioctl(fd, VIDIOC_S_CTRL, &vc) < 0) {
            LOGE("%s::VIDIOC_S_CTRL (id=%d,value=%d) failed", __func__, vc.id, vc.value);
            return -1;
        }

        fmt.fmt.win.w.left        = img_info->start_x;
        fmt.fmt.win.w.top         = img_info->start_y;
        fmt.fmt.win.w.width       = img_info->width;
        fmt.fmt.win.w.height      = img_info->height;
        break;
    default:
        LOGE("invalid buffer type");
        return -1;
        break;
    }

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        LOGE("%s::VIDIOC_S_FMT failed", __func__);
        return -1;
    }

    if (fmt.type != V4L2_BUF_TYPE_VIDEO_OVERLAY) {
        crop.type     = type;
        crop.c.left   = img_info->start_x;
        crop.c.top    = img_info->start_y;
        crop.c.width  = img_info->width;
        crop.c.height = img_info->height;

        if (ioctl(fd, VIDIOC_S_CROP, &crop) < 0) {
            LOGE("%s::VIDIOC_S_CROP (x=%d, y=%d, w=%d, h=%d) failed",
                __func__,
                img_info->start_x,
                img_info->start_y,
                img_info->width,
                img_info->height);
            return -1;
        }
    }

    return 0;
}

int fimc_v4l2_stream_on(int fd, enum v4l2_buf_type type)
{
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        LOGE("%s::VIDIOC_STREAMON failed", __func__);
        return -1;
    }

    return 0;
}

int fimc_v4l2_queue(int fd, SecBuffer *secBuf, enum v4l2_buf_type type, enum v4l2_memory memory, int index, int num_plane)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(struct v4l2_buffer));

    struct fimc_buf fimcbuf;

    buf.type     = type;
    buf.memory   = memory;
    buf.length   = num_plane;
    buf.index    = index;
    for (int i = 0; i < 3 ; i++) {
        fimcbuf.base[i]   = secBuf->phys.extP[i];
        fimcbuf.length[i] = secBuf->size.extS[i];
    }

    buf.m.userptr = (unsigned long)(&fimcbuf);
    //buf.m.userptr = secBuf->phys.p;

    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        LOGE("%s::VIDIOC_QBUF failed", __func__);
        return -1;
    }

    return 0;
}

int fimc_v4l2_dequeue(int fd, enum v4l2_buf_type type, enum v4l2_memory memory, int *index, int num_plane)
{
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(struct v4l2_buffer));

    buf.type     = type;
    buf.memory   = memory;
    buf.length   = num_plane;
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        LOGE("%s::VIDIOC_DQBUF failed", __func__);
        return -1;
    }
    *index = buf.index;

    return 0;
}

int fimc_v4l2_stream_off(int fd, enum v4l2_buf_type type)
{
    if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        LOGE("%s::VIDIOC_STREAMOFF failed", __func__);
        return -1;
    }

    return 0;
}

int fimc_v4l2_clr_buf(int fd, enum v4l2_buf_type type, enum v4l2_memory memory)
{
    struct v4l2_requestbuffers req;

    req.count   = 0;
    req.type    = type;
    req.memory  = memory;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        LOGE("%s::VIDIOC_REQBUFS", __func__);
        return -1;
    }

    return 0;
}

static inline int multipleOfN(int number, int N)
{
    int result = number;
    switch (N) {
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
        result = (number - (number & (N-1)));
        break;
    default:
        result = number - (number % N);
        break;
    }
    return result;
}

extern "C" SecFimc* create_instance()
{
    return new SecFimc;
}

extern "C" void destroy_instance(SecFimc* handle)
{
    if (handle != NULL)
        delete handle;
}

SecFimc::SecFimc()
:   mFlagCreate(false)
{
    memset(&mFimcCap, 0, sizeof(struct v4l2_capability));
    memset(&mS5pFimc, 0, sizeof(s5p_fimc_t));

    mRotVal = 0;
    mRealDev = -1;
    mNumOfBuf = 0;
    mHwVersion = 0;
    mGlobalAlpha = 0x0;
    mFlagStreamOn = false;
    mFlagSetSrcParam = false;
    mFlagSetDstParam = false;
    mFlagGlobalAlpha = false;
    mFlagLocalAlpha = false;
    mFlagColorKey = false;
    mFimcMode = 0;
    mFd = 0;
    mDev = 0;
    mColorKey = 0x0;
}

SecFimc::~SecFimc()
{
    if (mFlagCreate == true) {
        LOGE("%s::this is not Destroyed fail", __func__);
        if (destroy() == false)
            LOGE("%s::destroy failed", __func__);
    }
}

bool SecFimc::create(enum DEV dev, enum MODE mode, int numOfBuf)
{
    if (mFlagCreate == true) {
        LOGE("%s::Already Created fail", __func__);
        return false;
    }

    char node[20];
    struct v4l2_format  fmt;
    struct v4l2_control vc;
    SecBuffer zeroBuf;

    mDev = dev;
    mRealDev = dev;

    switch (mode) {
    case MODE_SINGLE_BUF:
        mFimcMode = FIMC_OVLY_NONE_SINGLE_BUF;
        break;
    case MODE_MULTI_BUF:
        mFimcMode = FIMC_OVLY_NONE_MULTI_BUF;
        break;
    case MODE_DMA_AUTO:
        mFimcMode = FIMC_OVLY_DMA_AUTO;
        break;
    default:
        LOGE("%s::Invalid mode(%d) fail", __func__, mode);
        mFimcMode = FIMC_OVLY_NOT_FIXED;
        goto err;
        break;
    }

    mNumOfBuf = numOfBuf;

    for (int i = 0; i < MAX_DST_BUFFERS; i++)
        mDstBuffer[i] = zeroBuf;

    sprintf(node, "%s%d", PFX_NODE_FIMC, (int)mRealDev);

    mFd = open(node, O_RDWR);
    if (mFd < 0) {
        LOGE("%s::open(%s) failed", __func__, node);
        mFd = 0;
        goto err;
    }

    /* check capability */
    if (ioctl(mFd, VIDIOC_QUERYCAP, &mFimcCap) < 0) {
        LOGE("%s::VIDIOC_QUERYCAP failed", __func__);
        goto err;
    }

    if (!(mFimcCap.capabilities & V4L2_CAP_STREAMING)) {
        LOGE("%s::%s has no streaming support", __func__, node);
        goto err;
    }

    if (!(mFimcCap.capabilities & V4L2_CAP_VIDEO_OUTPUT)) {
        LOGE("%s::%s is no video output", __func__, node);
        goto err;
    }

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    if (ioctl(mFd, VIDIOC_G_FMT, &fmt) < 0) {
        LOGE("%s::VIDIOC_G_FMT failed", __func__);
        goto err;
    }

    vc.id = V4L2_CID_RESERVED_MEM_BASE_ADDR;
    vc.value = 0;
    if (ioctl(mFd, VIDIOC_G_CTRL, &vc) < 0) {
        LOGE("%s::VIDIOC_G_CTRL - V4L2_CID_RESERVED_MEM_BAES_ADDR", __func__);
        goto err;
    }

    mDstBuffer[0].phys.p = (unsigned int)vc.value;

    mS5pFimc.out_buf.phys_addr = (void *)mDstBuffer[0].phys.p;

    vc.id    = V4L2_CID_FIMC_VERSION;
    vc.value = 0;
    if (ioctl(mFd, VIDIOC_G_CTRL, &vc) < 0) {
        LOGE("%s::VIDIOC_G_CTRL - V4L2_CID_FIMC_VERSION failed, FIMC version is set with default", __func__);
        vc.value = 0x43;
    }

    mHwVersion = vc.value;

    vc.id = V4L2_CID_OVLY_MODE;
    vc.value = mFimcMode;
    if (ioctl(mFd, VIDIOC_S_CTRL, &vc) < 0) {
        LOGE("%s::VIDIOC_S_CTRL - V4L2_CID_OVLY_MODE failed", __func__);
        goto err;
    }

    mFlagCreate = true;

    return true;

err :
    if (0 < mFd)
        close(mFd);
    mFd = 0;

    return false;
}

bool SecFimc::destroy()
{
    s5p_fimc_params_t *params = &(mS5pFimc.params);

    if (mFlagCreate == false) {
        LOGE("%s::Already Destroyed fail", __func__);
        return false;
    }

    if (mFlagStreamOn == true) {
        if (fimc_v4l2_stream_off(mFd, V4L2_BUF_TYPE_SRC) < 0) {
            LOGE("%s::fimc_v4l2_stream_off() failed", __func__);
            return false;
        }
        mFlagStreamOn = false;
    }

    if (fimc_v4l2_clr_buf(mFd, V4L2_BUF_TYPE_SRC, V4L2_MEMORY_TYPE_SRC) < 0) {
        LOGE("%s::fimc_v4l2_clr_buf()[src] failed", __func__);
        return false;
    }

    if (mS5pFimc.out_buf.phys_addr != NULL) {
        mS5pFimc.out_buf.phys_addr = NULL;
        mS5pFimc.out_buf.length = 0;
    }

    if (0 < mFd)
        close(mFd);
    mFd = 0;

    mFlagCreate = false;

    return true;
}

bool SecFimc::flagCreate(void)
{
    return mFlagCreate;
}

int SecFimc::getFd(void)
{
    return mFd;
}

SecBuffer * SecFimc::getMemAddr(int index)
{
    if (mFlagCreate == false) {
        LOGE("%s::Not yet created", __func__);
        return false;
    }

    return &mDstBuffer[index];
}

int SecFimc::getHWVersion(void)
{
    if (mFlagCreate == false) {
        LOGE("%s::Not yet created", __func__);
        return false;
    }

    return mHwVersion;
}

bool SecFimc::setSrcParams(unsigned int width, unsigned int height,
                           unsigned int cropX, unsigned int cropY,
                           unsigned int *cropWidth, unsigned int *cropHeight,
                           int colorFormat,
                           bool forceChange)
{
#ifdef DEBUG_LIB_FIMC
    LOGD("%s", __func__);
#endif

    if (mFlagCreate == false) {
        LOGE("%s::Not yet created", __func__);
        return false;
    }

    int v4l2ColorFormat = HAL_PIXEL_FORMAT_2_V4L2_PIX(colorFormat);
    if (v4l2ColorFormat < 0) {
        LOGE("%s::not supported color format", __func__);
        return false;
    }

    s5p_fimc_params_t *params = &(mS5pFimc.params);

    unsigned int fimcWidth  = *cropWidth;
    unsigned int fimcHeight = *cropHeight;
    int src_planes = m_getYuvPlanes(v4l2ColorFormat);

    m_checkSrcSize(width, height,
                   cropX, cropY,
                   &fimcWidth, &fimcHeight,
                   v4l2ColorFormat,
                   false);

    if (fimcWidth != *cropWidth || fimcHeight != *cropHeight) {
        if (forceChange == true) {
#ifdef DEBUG_LIB_FIMC
            LOGD("size is changed from [w = %d, h= %d] to [w = %d, h = %d]",
                    *cropWidth, *cropHeight, fimcWidth, fimcHeight);
#endif
        } else {
            LOGE("%s::invalid source params", __func__);
            return false;
        }
    }

    if (   (params->src.full_width == width)
        && (params->src.full_height == height)
        && (params->src.start_x == cropX)
        && (params->src.start_y == cropY)
        && (params->src.width == fimcWidth)
        && (params->src.height == fimcHeight)
        && (params->src.color_space == (unsigned int)v4l2ColorFormat))
        return true;

    params->src.full_width  = width;
    params->src.full_height = height;
    params->src.start_x     = cropX;
    params->src.start_y     = cropY;
    params->src.width       = fimcWidth;
    params->src.height      = fimcHeight;
    params->src.color_space = v4l2ColorFormat;
    src_planes = (src_planes == -1) ? 1 : src_planes;

    if (mFlagSetSrcParam == true) {
        if (fimc_v4l2_clr_buf(mFd, V4L2_BUF_TYPE_SRC, V4L2_MEMORY_TYPE_SRC) < 0) {
            LOGE("%s::fimc_v4l2_clr_buf_src() failed", __func__);
            return false;
        }
    }

    if (fimc_v4l2_set_fmt(mFd, V4L2_BUF_TYPE_SRC, V4L2_FIELD_NONE, &(params->src), 0) < 0) {
        LOGE("%s::fimc_v4l2_set_fmt()[src] failed", __func__);
        return false;
    }

    if (fimc_v4l2_req_buf(mFd, 1, V4L2_BUF_TYPE_SRC, V4L2_MEMORY_TYPE_SRC) < 0) {
        LOGE("%s::fimc_v4l2_req_buf()[src] failed", __func__);
        return false;
    }

    *cropWidth  = fimcWidth;
    *cropHeight = fimcHeight;

    mFlagSetSrcParam = true;
    return true;
}

bool SecFimc::getSrcParams(unsigned int *width, unsigned int *height,
                           unsigned int *cropX, unsigned int *cropY,
                           unsigned int *cropWidth, unsigned int *cropHeight,
                           int *colorFormat)
{
    struct v4l2_format fmt;
    struct v4l2_crop crop;

    fmt.type = V4L2_BUF_TYPE_SRC;

    if (ioctl(mFd, VIDIOC_G_FMT, &fmt) < 0) {
        LOGE("%s::VIDIOC_G_FMT(fmt.type : %d) failed", __func__, fmt.type);
        return false;
    }

    switch (fmt.type) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
    case V4L2_BUF_TYPE_VIDEO_OVERLAY:
        *width       = fmt.fmt.pix.width;
        *height      = fmt.fmt.pix.height;
        *colorFormat = fmt.fmt.pix.pixelformat;
        break;
    default:
        LOGE("%s::Invalid buffer type", __func__);
        return false;
        break;
    }

    crop.type = V4L2_BUF_TYPE_SRC;
    if (ioctl(mFd, VIDIOC_G_CROP, &crop) < 0) {
        LOGE("%s::VIDIOC_G_CROP failed", __func__);
        return false;
    }

    *cropX = crop.c.left;
    *cropY = crop.c.top;
    *cropWidth = crop.c.width;
    *cropHeight = crop.c.height;

    return true;
}

bool SecFimc::setSrcAddr(unsigned int physYAddr,
                         unsigned int physCbAddr,
                         unsigned int physCrAddr,
                         int colorFormat)
{
#ifdef DEBUG_LIB_FIMC
    LOGD("%s", __func__);
#endif

    if (mFlagCreate == false) {
        LOGE("%s::Not yet created", __func__);
        return false;
    }

    s5p_fimc_params_t *params = &(mS5pFimc.params);
    int src_planes = m_getYuvPlanes(params->src.color_space);
    int src_bpp = m_getYuvBpp(params->src.color_space);
    unsigned int frame_size = params->src.full_width * params->src.full_height;
    src_planes = (src_planes == -1) ? 1 : src_planes;

    mSrcBuffer.phys.extP[0] = physYAddr;

    if (colorFormat == HAL_PIXEL_FORMAT_YV12) {
        mSrcBuffer.phys.extP[1] = physCrAddr;
        mSrcBuffer.phys.extP[2] = physCbAddr;
    } else {
        mSrcBuffer.phys.extP[1] = physCbAddr;
        mSrcBuffer.phys.extP[2] = physCrAddr;
    }

    if (2 <= src_planes &&  mSrcBuffer.phys.extP[1] == 0)
        mSrcBuffer.phys.extP[1] = mSrcBuffer.phys.extP[0] + frame_size;

    if (3 == src_planes &&  mSrcBuffer.phys.extP[2] == 0) {
        if (colorFormat == HAL_PIXEL_FORMAT_YV12) {
            if (12 == src_bpp)
                mSrcBuffer.phys.extP[1] = mSrcBuffer.phys.extP[2] + (frame_size >> 2);
            else
                mSrcBuffer.phys.extP[1] = mSrcBuffer.phys.extP[2] + (frame_size >> 1);
        } else {
            if (12 == src_bpp)
                mSrcBuffer.phys.extP[2] = mSrcBuffer.phys.extP[1] + (frame_size >> 2);
            else
                mSrcBuffer.phys.extP[2] = mSrcBuffer.phys.extP[1] + (frame_size >> 1);
        }
    }

    return true;
}

bool SecFimc::setDstParams(unsigned int width, unsigned int height,
                           unsigned int cropX, unsigned int cropY,
                           unsigned int *cropWidth, unsigned int *cropHeight,
                           int colorFormat,
                           bool forceChange)
{
#ifdef DEBUG_LIB_FIMC
    LOGD("%s", __func__);
#endif

    if (mFlagCreate == false) {
        LOGE("%s::Not yet created", __func__);
        return false;
    }

    int v4l2ColorFormat = HAL_PIXEL_FORMAT_2_V4L2_PIX(colorFormat);
    if (v4l2ColorFormat < 0) {
        LOGE("%s::not supported color format", __func__);
        return false;
    }

    s5p_fimc_params_t *params = &(mS5pFimc.params);

    unsigned int fimcWidth  = *cropWidth;
    unsigned int fimcHeight = *cropHeight;
    int dst_planes = m_getYuvPlanes(v4l2ColorFormat);

    m_checkDstSize(width, height,
                   cropX, cropY,
                   &fimcWidth, &fimcHeight,
                   v4l2ColorFormat,
                   mRotVal,
                   true);

    if (fimcWidth != *cropWidth || fimcHeight != *cropHeight) {
        if (forceChange == true) {
#ifdef DEBUG_LIB_FIMC
            LOGD("size is changed from [w = %d, h= %d] to [w = %d, h = %d]",
                    *cropWidth, *cropHeight, fimcWidth, fimcHeight);
#endif
        } else {
            LOGE("%s::Invalid destination params", __func__);
            return false;
        }
    }

    if (90 == mRotVal || 270 == mRotVal) {
        params->dst.full_width  = height;
        params->dst.full_height = width;

        if (90 == mRotVal) {
            params->dst.start_x     = cropY;
            params->dst.start_y     = width - (cropX + fimcWidth);
        } else {
            params->dst.start_x = height - (cropY + fimcHeight);
            params->dst.start_y = cropX;
        }

        params->dst.width       = fimcHeight;
        params->dst.height      = fimcWidth;

        if (0x50 != mHwVersion)
            params->dst.start_y     += (fimcWidth - params->dst.height);

    } else {
        params->dst.full_width  = width;
        params->dst.full_height = height;

        if (180 == mRotVal) {
            params->dst.start_x = width - (cropX + fimcWidth);
            params->dst.start_y = height - (cropY + fimcHeight);
        } else {
            params->dst.start_x     = cropX;
            params->dst.start_y     = cropY;
        }

        params->dst.width       = fimcWidth;
        params->dst.height      = fimcHeight;
    }
    params->dst.color_space = v4l2ColorFormat;
    dst_planes = (dst_planes == -1) ? 1 : dst_planes;

    if (fimc_v4l2_s_ctrl(mFd, V4L2_ROTATE, mRotVal) < 0) {
        LOGE("%s::fimc_v4l2_s_ctrl(V4L2_ROTATE)", __func__);
        return false;
    }

    if (fimc_v4l2_set_fmt(mFd, V4L2_BUF_TYPE_DST, V4L2_FIELD_ANY, &(params->dst), (unsigned int)mS5pFimc.out_buf.phys_addr) < 0) {
        LOGE("%s::fimc_v4l2_set_fmt()[dst] failed", __func__);
        return false;
    }

    *cropWidth  = fimcWidth;
    *cropHeight = fimcHeight;

    mFlagSetDstParam = true;
    return true;
}

bool SecFimc::getDstParams(unsigned int *width, unsigned int *height,
                           unsigned int *cropX, unsigned int *cropY,
                           unsigned int *cropWidth, unsigned int *cropHeight,
                           int *colorFormat)
{
    struct v4l2_framebuffer fbuf;
    struct v4l2_format      fmt;
    struct v4l2_crop        crop;

    fmt.type = V4L2_BUF_TYPE_DST;
    if (ioctl(mFd, VIDIOC_G_FMT, &fmt) < 0) {
        LOGE("%s::VIDIOC_G_FMT(fmt.type : %d) failed", __func__, fmt.type);
        return false;
    }
    switch (fmt.type) {
    case V4L2_BUF_TYPE_VIDEO_OUTPUT:
    case V4L2_BUF_TYPE_VIDEO_CAPTURE:
        *width       = fmt.fmt.pix.width;
        *height      = fmt.fmt.pix.height;
        *colorFormat = fmt.fmt.pix.pixelformat;
        break;
    case V4L2_BUF_TYPE_VIDEO_OVERLAY:
        *cropX = fmt.fmt.win.w.left;
        *cropY = fmt.fmt.win.w.top;
        *cropWidth = fmt.fmt.win.w.width;
        *cropHeight = fmt.fmt.win.w.height;

        if (ioctl(mFd, VIDIOC_G_FBUF, &fbuf) < 0) {
            LOGE("%s::VIDIOC_G_FBUF failed", __func__);
            return false;
        }

        *width       = fbuf.fmt.width;
        *height      = fbuf.fmt.height;
        *colorFormat = fbuf.fmt.pixelformat;
        break;
    default:
        LOGE("%s::Invalid buffer type", __func__);
        return false;
        break;
    }

    if (fmt.type != V4L2_BUF_TYPE_VIDEO_OVERLAY) {

        crop.type = V4L2_BUF_TYPE_DST;
        if (ioctl(mFd, VIDIOC_G_CROP, &crop) < 0) {
            LOGE("%s::VIDIOC_G_CROP(crop.type : %d) failed", __func__, crop.type);
            return false;
        }

        *cropX = crop.c.left;
        *cropY = crop.c.top;
        *cropWidth = crop.c.width;
        *cropHeight = crop.c.height;
    }

    return true;
}

bool SecFimc::setDstAddr(unsigned int physYAddr, unsigned int physCbAddr, unsigned int physCrAddr, int buf_index)
{
#ifdef DEBUG_LIB_FIMC
    LOGD("%s", __func__);
#endif

    s5p_fimc_params_t *params = &(mS5pFimc.params);

    if (mFlagCreate == false) {
        LOGE("%s::Not yet created", __func__);
        return false;
    }

    mS5pFimc.out_buf.phys_addr = (void *)physYAddr;

    mDstBuffer[buf_index].phys.extP[0] = physYAddr;
    mDstBuffer[buf_index].phys.extP[1] = physCbAddr;
    mDstBuffer[buf_index].phys.extP[2] = physCrAddr;

    params->dst.buf_addr_phy_rgb_y = physYAddr;
    params->dst.buf_addr_phy_cb    = physCbAddr;
    params->dst.buf_addr_phy_cr    = physCrAddr;

    if ((physYAddr != 0)
        && ((unsigned int)mS5pFimc.out_buf.phys_addr != mDstBuffer[0].phys.p))
        mS5pFimc.use_ext_out_mem = 1;

    if (fimc_v4l2_s_ctrl(mFd, V4L2_ROTATE, mRotVal) < 0) {
        LOGE("%s::fimc_v4l2_s_ctrl(V4L2_ROTATE)", __func__);
        return false;
    }

    if (fimc_v4l2_set_fmt(mFd, V4L2_BUF_TYPE_DST, V4L2_FIELD_ANY, &(params->dst), (unsigned int)mS5pFimc.out_buf.phys_addr) < 0) {
        LOGE("%s::fimc_v4l2_set_fmt()[dst] failed", __func__);
        return false;
    }

    return true;
}

bool SecFimc::setRotVal(unsigned int rotVal)
{
    struct v4l2_control vc;

    if (mFlagCreate == false) {
        LOGE("%s::Not yet created", __func__);
        return false;
    }

    if (fimc_v4l2_s_ctrl(mFd, V4L2_ROTATE, rotVal) < 0) {
        LOGE("%s::fimc_v4l2_s_ctrl(V4L2_ROTATE) failed", __func__);
        return false;
    }

    mRotVal = rotVal;
    return true;
}

bool SecFimc::setGlobalAlpha(bool enable, int alpha)
{
    struct v4l2_framebuffer fbuf;
    struct v4l2_format fmt;

    if (mFlagCreate == false) {
        LOGE("%s::Not yet created", __func__);
        return false;
    }

    if (mFlagStreamOn == true) {
        LOGE("%s::mFlagStreamOn == true", __func__);
        return false;
    }

    if (mFlagGlobalAlpha == enable && mGlobalAlpha == alpha)
        return true;

    memset(&fbuf, 0, sizeof(fbuf));

    if (ioctl(mFd, VIDIOC_G_FBUF, &fbuf) < 0) {
        LOGE("%s::VIDIOC_G_FBUF failed", __func__);
        return false;
    }

    if (enable)
        fbuf.flags |= V4L2_FBUF_FLAG_GLOBAL_ALPHA;
    else
        fbuf.flags &= ~V4L2_FBUF_FLAG_GLOBAL_ALPHA;

    if (ioctl(mFd, VIDIOC_S_FBUF, &fbuf) < 0) {
        LOGE("%s::VIDIOC_S_FBUF failed", __func__);
        return false;
    }

    if (enable) {
        memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;

        if (ioctl(mFd, VIDIOC_G_FMT, &fmt) < 0) {
            LOGE("%s::VIDIOC_G_FMT failed", __func__);
            return false;
        }

        fmt.fmt.win.global_alpha = alpha & 0xFF;
        if (ioctl(mFd, VIDIOC_S_FMT, &fmt) < 0) {
            LOGE("%s::VIDIOC_S_FMT failed", __func__);
            return false;
        }
    }

    mFlagGlobalAlpha = enable;
    mGlobalAlpha     = alpha;

    return true;

}

bool SecFimc::setLocalAlpha(bool enable)
{
    if (mFlagCreate == false) {
        LOGE("%s::Not yet created", __func__);
        return false;
    }

    if (mFlagStreamOn == true) {
        LOGE("%s::mFlagStreamOn == true", __func__);
        return false;
    }

    if (mFlagLocalAlpha == enable)
        return true;

    return true;
}

bool SecFimc::setColorKey(bool enable, int colorKey)
{
    struct v4l2_framebuffer fbuf;
    struct v4l2_format fmt;

    if (mFlagCreate == false) {
        LOGE("%s::Not yet created", __func__);
        return false;
    }

    if (mFlagStreamOn == true) {
        LOGE("%s::mFlagStreamOn == true", __func__);
        return false;
    }

    if (mFlagColorKey == enable && mColorKey == colorKey)
        return true;

    memset(&fbuf, 0, sizeof(fbuf));

    if (ioctl(mFd, VIDIOC_G_FBUF, &fbuf) < 0) {
        LOGE("%s::VIDIOC_G_FBUF failed", __func__);
        return false;
    }

    if (enable)
        fbuf.flags |= V4L2_FBUF_FLAG_CHROMAKEY;
    else
        fbuf.flags &= ~V4L2_FBUF_FLAG_CHROMAKEY;

    if (ioctl(mFd, VIDIOC_S_FBUF, &fbuf) < 0) {
        LOGE("%s::VIDIOC_S_FBUF failed", __func__);
        return false;
    }

    if (enable) {
        memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;

        if (ioctl(mFd, VIDIOC_G_FMT, &fmt) < 0) {
            LOGE("%s::VIDIOC_G_FMT failed", __func__);
            return false;
        }

        fmt.fmt.win.chromakey = colorKey & 0xFFFFFF;

        if (ioctl(mFd, VIDIOC_S_FMT, &fmt) < 0)
            LOGE("%s::VIDIOC_S_FMT failed", __func__);
    }
    mFlagColorKey = enable;
    mColorKey = colorKey;
    return true;
}

bool SecFimc::draw(int src_index, int dst_index)
{
#ifdef DEBUG_LIB_FIMC
    LOGD("%s", __func__);
#endif

    if (mFlagCreate == false) {
        LOGE("%s::Not yet created", __func__);
        return false;
    }

    if (mFlagSetSrcParam == false) {
        LOGE("%s::mFlagSetSrcParam == false fail", __func__);
        return false;
    }

    if (mFlagSetDstParam == false) {
        LOGE("%s::mFlagSetDstParam == false fail", __func__);
        return false;
    }

    s5p_fimc_params_t *params = &(mS5pFimc.params);
    bool flagStreamOn = false;
    int src_planes = m_getYuvPlanes(params->src.color_space);
    int dst_planes = m_getYuvPlanes(params->dst.color_space);
    src_planes  = (src_planes == -1) ? 1 : src_planes;
    dst_planes  = (dst_planes == -1) ? 1 : dst_planes;

    if (fimc_v4l2_stream_on(mFd, V4L2_BUF_TYPE_SRC) < 0) {
        LOGE("%s::fimc_v4l2_stream_on() failed", __func__);
        goto err;
    }

    flagStreamOn = true;

    if (fimc_v4l2_queue(mFd, &(mSrcBuffer), V4L2_BUF_TYPE_SRC, V4L2_MEMORY_TYPE_SRC, src_index, src_planes) < 0) {
        LOGE("%s::fimc_v4l2_queue(index : %d) (mNumOfBuf : %d) failed", __func__, 0, mNumOfBuf);
        goto err;
    }

    if (fimc_v4l2_dequeue(mFd, V4L2_BUF_TYPE_SRC, V4L2_MEMORY_TYPE_SRC, &src_index, src_planes) < 0) {
        LOGE("%s::fimc_v4l2_dequeue (mNumOfBuf : %d) failed", __func__, mNumOfBuf);
        goto err;
    }

err :
    if (flagStreamOn == true) {
        if (fimc_v4l2_stream_off(mFd, V4L2_BUF_TYPE_SRC) < 0) {
            LOGE("%s::fimc_v4l2_stream_off() failed", __func__);
            return false;
        }
    }

    return true;
}

bool SecFimc::m_streamOn()
{
#ifdef DEBUG_LIB_FIMC
    LOGD("%s", __func__);
#endif

    if (fimc_v4l2_stream_on(mFd, V4L2_BUF_TYPE_SRC) < 0) {
        LOGE("%s::fimc_v4l2_stream_on() failed", __func__);
        return false;
    }

    return true;
}

bool SecFimc::m_checkSrcSize(unsigned int width, unsigned int height,
                             unsigned int cropX, unsigned int cropY,
                             unsigned int *cropWidth, unsigned int *cropHeight,
                             int colorFormat,
                             bool forceChange)
{
    bool ret = true;

    if (8 <= height && *cropHeight < 8) {
        if (forceChange)
            *cropHeight = 8;
        ret = false;
    }

    if (16 <= width && *cropWidth < 16) {
        if (forceChange)
            *cropWidth = 16;
        ret = false;
    }

    if (0x50 == mHwVersion) {
        if (colorFormat == V4L2_PIX_FMT_YUV422P) {
            if (*cropHeight % 2 != 0) {
                if (forceChange)
                    *cropHeight = multipleOfN(*cropHeight, 2);
                ret = false;
            }
            if (*cropWidth % 2 != 0) {
                if (forceChange)
                    *cropWidth = multipleOfN(*cropWidth, 2);
                ret = false;
            }
        }
    } else {
        if (height < 8)
            return false;

        if (width % 16 != 0)
            return false;

        if (*cropWidth % 16 != 0) {
            if (forceChange)
                *cropWidth = multipleOfN(*cropWidth, 16);
            ret = false;
        }
    }

    return ret;
}

bool SecFimc::m_checkDstSize(unsigned int width, unsigned int height,
                             unsigned int cropX, unsigned int cropY,
                             unsigned int *cropWidth, unsigned int *cropHeight,
                             int colorFormat, int rotVal,  bool forceChange)
{
    bool ret = true;
    unsigned int rotWidth;
    unsigned int rotHeight;
    unsigned int *rotCropWidth;
    unsigned int *rotCropHeight;

    if (rotVal == 90 || rotVal == 270) {
        rotWidth = height;
        rotHeight = width;
        rotCropWidth = cropHeight;
        rotCropHeight = cropWidth;
    } else {
        rotWidth = width;
        rotHeight = height;
        rotCropWidth = cropWidth;
        rotCropHeight = cropHeight;
    }

    if (rotHeight < 8)
        return false;

    if (rotWidth % 8 != 0)
        return false;

    switch (colorFormat) {
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV12T:
    case V4L2_PIX_FMT_YUV420:
        if (*rotCropHeight % 2 != 0) {
            if (forceChange)
                *rotCropHeight = multipleOfN(*rotCropHeight, 2);
            ret = false;
        }
    }
    return ret;
}

int SecFimc::m_widthOfFimc(int v4l2ColorFormat, int width)
{
    int newWidth = width;

    if (0x50 == mHwVersion) {
        switch (v4l2ColorFormat) {
        /* 422 1/2/3 plane */
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
        case V4L2_PIX_FMT_NV61:
        case V4L2_PIX_FMT_NV16:
        case V4L2_PIX_FMT_YUV422P:
        /* 420 2/3 plane */
        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV12T:
        case V4L2_PIX_FMT_YUV420:

            newWidth = multipleOfN(width, 2);
            break;
        default :
            break;
        }
    } else {
        switch (v4l2ColorFormat) {
        case V4L2_PIX_FMT_RGB565:
            newWidth = multipleOfN(width, 8);
            break;
        case V4L2_PIX_FMT_RGB32:
            newWidth = multipleOfN(width, 4);
            break;
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
            newWidth = multipleOfN(width, 4);
            break;
        case V4L2_PIX_FMT_NV61:
        case V4L2_PIX_FMT_NV16:
            newWidth = multipleOfN(width, 8);
            break;
        case V4L2_PIX_FMT_YUV422P:
            newWidth = multipleOfN(width, 16);
            break;
        case V4L2_PIX_FMT_NV21:
        case V4L2_PIX_FMT_NV12:
        case V4L2_PIX_FMT_NV12T:
            newWidth = multipleOfN(width, 8);
            break;
        case V4L2_PIX_FMT_YUV420:
            newWidth = multipleOfN(width, 16);
            break;
        default :
            break;
        }
    }
    return newWidth;
}

int SecFimc::m_heightOfFimc(int v4l2ColorFormat, int height)
{
    int newHeight = height;

    switch (v4l2ColorFormat) {
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV12T:
    case V4L2_PIX_FMT_YUV420:
        newHeight = multipleOfN(height, 2);
        break;
    default :
        break;
    }
    return newHeight;
}

int SecFimc::m_getYuvBpp(unsigned int fmt)
{
    int i, sel = -1;

    for (i = 0; i < (int)(sizeof(yuv_list) / sizeof(struct yuv_fmt_list)); i++) {
        if (yuv_list[i].fmt == fmt) {
            sel = i;
            break;
        }
    }

    if (sel == -1)
        return sel;
    else
        return yuv_list[sel].bpp;
}

int SecFimc::m_getYuvPlanes(unsigned int fmt)
{
    int i, sel = -1;

    for (i = 0; i < (int)(sizeof(yuv_list) / sizeof(struct yuv_fmt_list)); i++) {
        if (yuv_list[i].fmt == fmt) {
            sel = i;
            break;
        }
    }

    if (sel == -1)
        return sel;
    else
        return yuv_list[sel].planes;
}
