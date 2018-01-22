/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __WESTEROS_SINK_SOC_H__
#define __WESTEROS_SINK_SOC_H__

#include <stdlib.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include "simplebuffer-client-protocol.h"

#include "OSAL_api.h"
#include "amp_client.h"

#define DEFAULT_PROP_RECTANGLE                          "0,0,0,0"
#define DEFAULT_PROP_FLUSH_REPEAT_FRAME         0
#define DEFAULT_PROP_CURRENT_PTS                        0
#define DEFAULT_PROP_INTER_FRAME_DELAY          0
#define DEFAULT_PROP_SLOW_MODE_RATE             5000
#define DEFAULT_PROP_STEP_FRAME                         3003
#define DEFAULT_PROP_COUNTER_FRAME_RATE         3003
#define DEFAULT_PROP_MUTE                                       1
#define DEFAULT_PROP_ZOOM                   1
#define DEFAULT_PLAY_SPEED                      1

#define WESTEROS_SINK_CAPS "video/x-tbd;"

typedef struct _Gstampvsink      Gstampvsink;

typedef enum
{
        DISP_PLANE_MAIN,
        DISP_PLANE_PIP,
        DISP_PLANE_GFX0,
        DISP_PLANE_GFX1,
        DISP_PLANE_GFX2,
        DISP_PLANE_PG,
        DISP_PLANE_BG,
        DISP_PLANE_AUX,
        DISP_PLANE_MAX
}GstAmpPlane;


typedef enum
{
    DISP_OUT_RES_NTSC_M ,
    DISP_OUT_RES_480P60 ,
    DISP_OUT_RES_PAL_M  ,
    DISP_OUT_RES_PAL_BGH ,
    DISP_OUT_RES_720P60 ,
    DISP_OUT_RES_720P50 ,
    DISP_OUT_RES_1080I60,
    DISP_OUT_RES_1080I50,
    DISP_OUT_RES_1080P60,
    DISP_OUT_RES_1080P50

}GstAmpTvMode;

/* Definition of structure storing data for Gstampvsink element */
struct _Gstampvsink
{
    AMP_FACTORY factory;
    /* disp related */
    AMP_DISP Disp;
    AMP_DISP_ZORDER Zorder;
    AMP_DISP_WIN SrcWin;
    AMP_DISP_WIN DstWin;

    gint plane;
    gint tvmode;
    gboolean repeatframe;
    gulong currentpts;
    guint framedelay;
    gint slowmoderate;
    guint contentframerate;
    guint steprate;
    guint mute;
    guint zoom;
    gboolean is_zoomSet;
    gint64 current_time;
    gint64 stream_duration;
    gint64 tempTime;
    gfloat play_speed;
    gboolean m_isPaused;
    gint64 timeDiff;
    guint resolution;

    GstPad *sinkpad ;
    gboolean pipeline_created;

    GThread *feed_t;
    GCond   feed_cond;
    GMutex  feed_mutex;
    GQueue*  buf_q;
    gboolean stop;

    /* vout related begin */
    AMP_COMPONENT hVout;
    AMP_COMPONENT hClk;
    HANDLE hVListener;
    GstWesterosSink* westeros_sink;
    gboolean is_eos;
    /* vout related end */

    /* vdec related begin */
    GstCaps *cap_accepted;
    AMP_COMPONENT hVdec;
    /* secure pipeline */
    BOOL        secure;

    /* for clear stream*/
    UINT32 shm_v;
    UINT8  *addr_v;
    UINT32 wp_v;
    UINT32 rp_v;

    UINT32 stream_pos;
    UINT32 video_codec;
    AMP_BDCHAIN *video_stream_queue;
    HANDLE hVdecListener;

    /* ES meta data from container */
    UINT8 *codec_data;
    UINT  codec_size;

    /* ES header */
    UINT8  video_priv_data[2048];
    UINT32 video_priv_len;
    gboolean video_priv_done;
    /* H265 specific */
    UINT32 nal_len_size;
    /* vdec related end */
};

struct _GstWesterosSinkSoc
{
   Gstampvsink amp_sink;
   struct wl_sb *sb;
   int activeBuffers;
};

void gst_westeros_sink_soc_load();
void gst_westeros_sink_soc_class_init(GstWesterosSinkClass *klass);
gboolean gst_westeros_sink_soc_init( GstWesterosSink *sink );
void gst_westeros_sink_soc_term( GstWesterosSink *sink );
void gst_westeros_sink_soc_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
void gst_westeros_sink_soc_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
gboolean gst_westeros_sink_soc_query(GstWesterosSink *sink, GstQuery * query);
gboolean gst_westeros_sink_soc_null_to_ready( GstWesterosSink *sink, gboolean *passToDefault );
gboolean gst_westeros_sink_soc_ready_to_paused( GstWesterosSink *sink, gboolean *passToDefault );
gboolean gst_westeros_sink_soc_paused_to_playing( GstWesterosSink *sink, gboolean *passToDefault );
gboolean gst_westeros_sink_soc_playing_to_paused( GstWesterosSink *sink, gboolean *passToDefault );
gboolean gst_westeros_sink_soc_paused_to_ready( GstWesterosSink *sink, gboolean *passToDefault );
gboolean gst_westeros_sink_soc_ready_to_null( GstWesterosSink *sink, gboolean *passToDefault );
void gst_westeros_sink_soc_registryHandleGlobal( GstWesterosSink *sink, 
                                 struct wl_registry *registry, uint32_t id,
		                           const char *interface, uint32_t version);
void gst_westeros_sink_soc_registryHandleGlobalRemove(GstWesterosSink *sink,
                                 struct wl_registry *registry,
			                        uint32_t name);
gboolean gst_westeros_sink_soc_accept_caps( GstWesterosSink *sink, GstCaps *caps );
void gst_westeros_sink_soc_set_startPTS( GstWesterosSink *sink, gint64 pts );
void gst_westeros_sink_soc_render( GstWesterosSink *sink, GstBuffer *buffer );
void gst_westeros_sink_soc_flush( GstWesterosSink *sink );
gboolean gst_westeros_sink_soc_start_video( GstWesterosSink *sink );
void gst_westeros_sink_soc_eos_event( GstWesterosSink *sink );
void gst_westeros_sink_soc_set_video_path( GstWesterosSink *sink, bool useGfxPath );
void gst_westeros_sink_soc_update_video_position( GstWesterosSink *sink );
void gst_westeros_sink_soc_query_caps(GstWesterosSink *sink, GstQuery *query);
void gst_westeros_sink_soc_query_accept_caps(GstWesterosSink *sink, GstQuery *query);

#endif

