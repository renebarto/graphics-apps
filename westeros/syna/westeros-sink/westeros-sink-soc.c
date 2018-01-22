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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "syna/westeros-sink/westeros-sink.h"

#define FRAME_POLL_TIME (8000)
#define EOS_DETECT_DELAY (500000)
#define EOS_DETECT_DELAY_AT_START (10000000)

#define MAX_VIDEO_STREAM_SIZE (256)
#define VIDEO_ES_BUFFER_SIZE  (4 * 1024 * 1024)

GST_DEBUG_CATEGORY_EXTERN (gst_westeros_sink_debug);
#define GST_CAT_DEFAULT gst_westeros_sink_debug

#define VERIFY_RESULT(x) \
        do {\
               if ((x) != SUCCESS)\
                   GST_ERROR("error=%x, (%s)@%d\n",\
                                x, __FUNCTION__, __LINE__);\
        } while(0)

typedef enum {
	NO_LINEAR, //No linear space
	LINEAR_END,//Linear space at the end of buffer
	LINEAR_BEG,//Linear space at the beginning of buffer
}LINEAR_POS;

typedef enum
{
  FEED_DONE,
  FEED_RETRY,
  FEED_ERR,
}FEED_RET;

static gboolean amp_handle_event_caps(Gstampvsink* amp_sink, GstCaps* caps);
static HRESULT amp_sink_set_state(Gstampvsink* amp_sink, AMP_STATE state);
static HRESULT amp_disconnect(Gstampvsink *amp_sink);
static void amp_vout_destroy(Gstampvsink* amp_sink);
static HRESULT amp_vdec_destroy(Gstampvsink *amp_sink);
static GstFlowReturn gst_amp_vdec_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);
static void waiting_for_BD_reset_buf(Gstampvsink* amp_sink);
static gpointer vsink_feed_task(gpointer data);

static void feed_thread_init(Gstampvsink *amp_sink);
static void feed_thread_stop(Gstampvsink *amp_sink);
static void feed_thread_push(Gstampvsink *amp_sink, GstBuffer* buf);
static void feed_thread_destroy(Gstampvsink *amp_sink);


/*######################################################################*/

/* amp_sink signals and args */
enum
{
        PROP_TV_MODE = PROP_SOC_BASE,
        PROP_PLANE,
        PROP_RECTANGLE,
        PROP_FLUSH_REPEAT_FRAME,
        PROP_CURRENT_PTS,
        PROP_INTER_FRAME_DELAY,
        PROP_SLOW_MODE_RATE,
        PROP_CONTENT_FRAME_RATE,
        PROP_STEP_FRAME,
        PROP_MUTE,
        PROP_ZOOM,
        PROP_PLAY_SPEED,
        PROP_VOUT
};

static AMP_FACTORY hFactory = NULL;
#define GST_AMP_TV_MODE (gst_tv_mode_get_type())
static GType gst_tv_mode_get_type (void)
{

        static GType ampamp_sink_tvmode_type = 0;

        if (!ampamp_sink_tvmode_type) {
                static const GEnumValue ampamp_sink_tvmode[] = {

            {DISP_OUT_RES_NTSC_M, "480 interlaced at 60 Hz", "0"},
            {DISP_OUT_RES_480P60, "480 progressive at 60 Hz", "1"},
            {DISP_OUT_RES_PAL_M, "576 interlaced at 50 Hz", "2"},
            {DISP_OUT_RES_PAL_BGH, "576 progressive at 50 Hz", "3"},
            {DISP_OUT_RES_720P60, "720 progressive at 60 Hz", "4"},
            {DISP_OUT_RES_720P50, "720 progressive at 50 Hz", "5"},
            {DISP_OUT_RES_1080I60, "1080 interlaced at 60 Hz", "6"},
            {DISP_OUT_RES_1080I50, "1080 interlaced at 50 Hz", "7"},
            {DISP_OUT_RES_1080P60, "1080 progressive at 60 Hz", "8"},
            {DISP_OUT_RES_1080P50, "1080 progressive at 50 Hz", "9"},
            {0, NULL, NULL}

                };

                ampamp_sink_tvmode_type =
                        g_enum_register_static ("GstAmpSinkTvMode",ampamp_sink_tvmode);
        }

        return ampamp_sink_tvmode_type;
}

#define GST_AMP_PLANE (gst_plane_get_type())
static GType gst_plane_get_type (void)
{
        static GType ampamp_sink_plane_type = 0;

        if (!ampamp_sink_plane_type) {
                static const GEnumValue ampamp_sink_planes[] = {
                        {AMP_DISP_PLANE_MAIN, "plane main", "0"},
                        {AMP_DISP_PLANE_PIP, "plane pip", "1"},
                        {AMP_DISP_PLANE_GFX0, "plane gfx0", "2"},
                        {AMP_DISP_PLANE_GFX1, "plane gfx1", "3"},
                        {AMP_DISP_PLANE_GFX2, "palne gfx2", "4"},
                        {AMP_DISP_PLANE_PG, "plane pg", "5"},
                        {AMP_DISP_PLANE_BG, "plane bg", "6"},
                        {AMP_DISP_PLANE_AUX, "plane aux", "7"},
                        {AMP_DISP_PLANE_MAX, "plane max", "8"},
                        {0, NULL, NULL},
                };

                ampamp_sink_plane_type =
                        g_enum_register_static ("GstAmpSinkPlane",ampamp_sink_planes);
        }

        return ampamp_sink_plane_type;

}

/* Description for Element Pads */
static GstStaticPadTemplate sink_amp_sink_pad = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-h264;"
        "video/x-h265;"
        "video/x-vp9;"
        "video/x-vp8;"
       )
    );

/**
 * Sink pad event handler funtion
 */
static gboolean gst_amp_sink_event (GstPad *pad, GstObject * objectParent, GstEvent *event)
{
    GstWesterosSink  *sink = GST_WESTEROS_SINK(objectParent);
    Gstampvsink *amp_sink = &sink->soc.amp_sink;
    GstElement *element = GST_ELEMENT(sink);
    HRESULT result = SUCCESS;
    GstStructure *structureConComp;

    if(GST_EVENT_TYPE (event) != GST_EVENT_TAG)
        GST_DEBUG_OBJECT (sink, "Received %s event: %" GST_PTR_FORMAT,
                GST_EVENT_TYPE_NAME (event), event);

    switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
        GstCaps *caps;
        gst_event_parse_caps (event, &caps);
        GST_LOG_OBJECT (sink, "receive caps %" GST_PTR_FORMAT, caps);
        gboolean handled = amp_handle_event_caps(amp_sink, caps);
        if(handled) {
            if(amp_sink->cap_accepted == NULL)
                amp_sink->cap_accepted = gst_caps_copy(caps);
            else if(!gst_caps_is_equal(amp_sink->cap_accepted, caps)) {
                gst_caps_unref(amp_sink->cap_accepted);
                amp_sink->cap_accepted = gst_caps_copy(caps);
            }
        }
        break;
    }
    case GST_EVENT_EOS:
    {
        gst_westeros_sink_soc_eos_event( sink );
        break;
    }
    case GST_EVENT_FLUSH_START:
    {
        feed_thread_stop(amp_sink);
        result = amp_sink_set_state(amp_sink, AMP_IDLE);
        VERIFY_RESULT(result);
        if(amp_sink->hVdec) {
            AMP_RPC(result, AMP_VDEC_ClearPortBuf, amp_sink->hVdec, AMP_PORT_OUTPUT, 0);
            VERIFY_RESULT(result);
            AMP_RPC(result, AMP_VDEC_ClearPortBuf, amp_sink->hVdec, AMP_PORT_INPUT, 0);
            VERIFY_RESULT(result);
        }
        if(amp_sink->hVout) {
            AMP_RPC(result, AMP_VOUT_ClearPortBuf, amp_sink->hVout, AMP_PORT_INPUT, 0);
            VERIFY_RESULT(result);
        }
        //gst_element_set_base_time(element, GST_CLOCK_TIME_NONE);
        gst_westeros_sink_soc_flush( sink );
        break;
    }
    case GST_EVENT_FLUSH_STOP:
    {
        GST_PAD_STREAM_LOCK(amp_sink->sinkpad);
        feed_thread_destroy(amp_sink);
        feed_thread_init(amp_sink);
        waiting_for_BD_reset_buf(amp_sink);
        result = amp_sink_set_state(amp_sink, AMP_EXECUTING);
        VERIFY_RESULT(result);
        GST_PAD_STREAM_UNLOCK(amp_sink->sinkpad);
        break;
    }
    case GST_EVENT_SEGMENT:
    {
        const GstStructure *event_structure = gst_event_get_structure(event);
        GstSegment *segment;
        const GValue *durVal = gst_structure_get_value(event_structure,"segment");
        segment = (GstSegment *)g_value_peek_pointer(durVal);
        if(segment->stop != -1){
            GST_DEBUG_OBJECT(sink, "###duration = %" GST_TIME_FORMAT,
                    GST_TIME_ARGS(segment->stop - segment->start));
            amp_sink->stream_duration = segment->stop - segment->start;
            amp_sink->current_time = segment->start;
        }else if(segment->duration != -1){
            GST_DEBUG_OBJECT(sink, "###duration = %" GST_TIME_FORMAT,
                    GST_TIME_ARGS(segment->duration));
            amp_sink->stream_duration = segment->duration;
        }else{
            GST_WARNING("###duration is not present in segment event\n");
        }

        gst_westeros_sink_soc_set_startPTS( sink, segment->start);
        break;
    }
    default:
        break;
    }

    return GST_WESTEROS_SINK_GET_CLASS(sink)->sink_cb(pad, objectParent,event);
}

/*######################################################################*/

static gboolean gst_ampvsink_sink_query_function (GstElement *element,GstQuery *query) {
  gboolean ret = FALSE;
  GstFormat format=GST_FORMAT_TIME;
  HRESULT result = SUCCESS;
  UINT32 ptsHigh, ptsLow;

  GstWesterosSink *sink = GST_WESTEROS_SINK(element);
  Gstampvsink *amp_sink = &sink->soc.amp_sink;

  switch(GST_QUERY_TYPE(query)){
    case GST_QUERY_POSITION:
    {
        GstFormat format;
        gst_query_parse_position(query, &format, NULL);
        if(format == GST_FORMAT_TIME) {
            gint64 time = amp_sink->current_time;
            gst_query_set_position (query, format, time);
            ret = TRUE;
        }
        break;
    }
    case GST_QUERY_DURATION:
    {
        GstFormat format;
        gst_query_parse_duration(query,&format, NULL);
        if(format == GST_FORMAT_TIME) {
            if(amp_sink->stream_duration) {
                gst_query_set_duration (query, format, amp_sink->stream_duration);
                ret = TRUE;
            } else {
                if(gst_pad_peer_query_duration(amp_sink->sinkpad, format,
                            &amp_sink->stream_duration)) {
                    gst_query_set_duration (query, format, amp_sink->stream_duration);
                    ret = TRUE;
                }
            }
        }
        break;
    }
    case GST_QUERY_SEEKING:
    {
        ret = gst_pad_peer_query(amp_sink->sinkpad, query);
        break;
    }
    default:
    {
        ret = FALSE;
        break;
    }
  }

  return ret;
}

static HRESULT amp_sink_set_state(Gstampvsink* amp_sink, AMP_STATE state) {
    HRESULT result;
    if(amp_sink->hVdec && amp_sink->pipeline_created){
        AMP_RPC(result, AMP_VDEC_SetState, amp_sink->hVdec, state);
        VERIFY_RESULT(result);
    }
    if(amp_sink->hVout){
        AMP_RPC(result, AMP_VOUT_SetState, amp_sink->hVout, state);
        VERIFY_RESULT(result);
    }
    if(amp_sink->hClk){
        AMP_RPC(result, AMP_CLK_SetState, amp_sink->hClk, state);
        VERIFY_RESULT(result);
    }
    return result;
}

/**
 * To destroy AMP Vout Component
 */

static HRESULT amp_sink_destroy(Gstampvsink *amp_sink) {
    HRESULT result = SUCCESS;

    if(amp_sink->Disp) {
        /* restore Z-orver */
        AMP_RPC(result,
                AMP_DISP_SetPlaneZOrder,
                amp_sink->Disp,
                0,
                &amp_sink->Zorder);
        VERIFY_RESULT(result);
    }

    amp_vout_destroy(amp_sink);
    amp_vdec_destroy(amp_sink);
    return result;
}

/**
 *  Function to handle EOS flag from VOUT
 */
static HRESULT amp_sink_event_handler(HANDLE hListener, AMP_EVENT * pEvent,
    VOID * pUserData)
{
    HRESULT result = SUCCESS;
    AMP_STATE eState;
    Gstampvsink *amp_sink = (Gstampvsink*)pUserData;

    if (pEvent->stEventHead.eEventCode == AMP_EVENT_API_VOUT_CALLBACK) {
        GST_DEBUG("VOUT Received EOS");
        amp_sink->is_eos = true;
        if(gst_element_post_message(GST_ELEMENT(amp_sink->westeros_sink),
                    gst_message_new_eos(GST_OBJECT(amp_sink->westeros_sink))))
            GST_DEBUG(" EOS message posted to application ");
    } else if (pEvent->stEventHead.eEventCode ==
            AMP_EVENT_API_VOUT_CALLBACK_FIRST_FRAME) {
        GST_DEBUG("  Received First Frame Event from  AMP Vout Component ");
    } else if (pEvent->stEventHead.eEventCode ==
            AMP_EVENT_API_VOUT_CALLBACK_FRAME_UPDATE) {
        AMP_VOUT_DISP_FRAME_INFO *pFrame =
            (AMP_VOUT_DISP_FRAME_INFO *) AMP_EVENT_PAYLOAD_PTR(pEvent);
        GST_LOG("amp_sink pts: :%x dis:%d\n", pFrame->pts_l, pFrame->displayed);
        if(!pFrame->displayed)
            GST_LOG("frame dropped pts: %x:%x\n", pFrame->pts_h, pFrame->pts_l);

        amp_sink->current_time = (gint64)((UINT64)AMP_GET_PTS_VAL(
                    AMP_MAKE_PTS64(pFrame->pts_h, pFrame->pts_l)) * 100000 / 9);
    }
    return result;
}

/**
 *  Function to register event callback to VOUT for EOS
 */
static HRESULT amp_vout_event_register(Gstampvsink *amp_sink)
{
  HRESULT result;

  amp_sink->hVListener = AMP_Event_CreateListener(AMP_EVENT_TYPE_MAX, 0);

  result = AMP_Event_RegisterCallback(amp_sink->hVListener,
      AMP_EVENT_API_VOUT_CALLBACK,
      amp_sink_event_handler, amp_sink);
  VERIFY_RESULT(result);
  AMP_RPC(result, AMP_VOUT_RegisterNotify, amp_sink->hVout,
          AMP_Event_GetServiceID(amp_sink->hVListener), AMP_EVENT_API_VOUT_CALLBACK);
  VERIFY_RESULT(result);
  result = AMP_Event_RegisterCallback(amp_sink->hVListener,
      AMP_EVENT_API_VOUT_CALLBACK_FIRST_FRAME,
      amp_sink_event_handler, amp_sink);
  VERIFY_RESULT(result);
  AMP_RPC(result, AMP_VOUT_RegisterNotify, amp_sink->hVout,
          AMP_Event_GetServiceID(amp_sink->hVListener), AMP_EVENT_API_VOUT_CALLBACK_FIRST_FRAME);
  VERIFY_RESULT(result);
  result = AMP_Event_RegisterCallback(amp_sink->hVListener,
      AMP_EVENT_API_VOUT_CALLBACK_FRAME_UPDATE,
      amp_sink_event_handler, amp_sink);
  VERIFY_RESULT(result);
  AMP_RPC(result, AMP_VOUT_RegisterNotify, amp_sink->hVout,
          AMP_Event_GetServiceID(amp_sink->hVListener), AMP_EVENT_API_VOUT_CALLBACK_FRAME_UPDATE);
  VERIFY_RESULT(result);
  return result;
}

/**
 * To unregister VOUT Notification
 */

static HRESULT amp_vout_unregister(Gstampvsink *amp_sink) {
  HRESULT result = S_OK;
  /**
   * Unregister the notification
   */
  AMP_RPC(result, AMP_VOUT_UnregisterNotify,amp_sink->hVout,
      AMP_Event_GetServiceID(amp_sink->hVListener), AMP_EVENT_API_VOUT_CALLBACK);
  VERIFY_RESULT(result);

  result = AMP_Event_UnregisterCallback(amp_sink->hVListener,
      AMP_EVENT_API_VOUT_CALLBACK,
      amp_sink_event_handler);
  VERIFY_RESULT(result);

  AMP_RPC(result, AMP_VOUT_UnregisterNotify,amp_sink->hVout,
      AMP_Event_GetServiceID(amp_sink->hVListener),
      AMP_EVENT_API_VOUT_CALLBACK_FIRST_FRAME);
  VERIFY_RESULT(result);

  result = AMP_Event_UnregisterCallback(amp_sink->hVListener,
      AMP_EVENT_API_VOUT_CALLBACK_FIRST_FRAME,
      amp_sink_event_handler);
  VERIFY_RESULT(result);

  AMP_RPC(result, AMP_VOUT_UnregisterNotify,amp_sink->hVout,
      AMP_Event_GetServiceID(amp_sink->hVListener),
      AMP_EVENT_API_VOUT_CALLBACK_FRAME_UPDATE);
  VERIFY_RESULT(result);

  result = AMP_Event_UnregisterCallback(amp_sink->hVListener,
      AMP_EVENT_API_VOUT_CALLBACK_FRAME_UPDATE,
      amp_sink_event_handler);
  VERIFY_RESULT(result);

  result = AMP_Event_DestroyListener(amp_sink->hVListener);
  amp_sink->hVListener = NULL;
  VERIFY_RESULT(result);
  return result;
}

static HRESULT amp_disp_open(Gstampvsink* amp_sink) {
    HRESULT result = S_OK;
    UINT32 resId = 0;
    /* display service */
    AMP_RPC(result, AMP_FACTORY_CreateDisplayService,
            hFactory, &(amp_sink->Disp));
    VERIFY_RESULT(result);
    /* check if current display is set at 4k */
    AMP_RPC(result,
            AMP_DISP_OUT_GetResolution,
            amp_sink->Disp,
            AMP_DISP_PLANE_MAIN,
            &resId);
    VERIFY_RESULT(result);
    if((resId >= AMP_DISP_OUT_RES_4Kx2K_MIN) &&
            (resId < AMP_DISP_OUT_RES_4Kx2K_MAX)) {
        /* scale the video rectangle window for 4k*/
        amp_sink->DstWin.iX *= 2;
        amp_sink->DstWin.iY *= 2;
        amp_sink->DstWin.iWidth *= 2;
        amp_sink->DstWin.iHeight *= 2;
    }

    /* set scale */
    AMP_RPC(result, AMP_DISP_SetScale, amp_sink->Disp,
            0, &amp_sink->SrcWin, &amp_sink->DstWin);

    /* Z order Main is below GFX plane */
    AMP_RPC(result,
            AMP_DISP_GetPlaneZOrder,
            amp_sink->Disp,
            0,
            &amp_sink->Zorder);
    VERIFY_RESULT(result);

    amp_sink->Zorder.iMain = 2;
    amp_sink->Zorder.iPip  = 6;
    amp_sink->Zorder.iGfx0 = 5;
    amp_sink->Zorder.iGfx1 = 4;
    amp_sink->Zorder.iGfx2 = 3;
    amp_sink->Zorder.iPg   = 1;
    amp_sink->Zorder.iBg   = 0;
    amp_sink->Zorder.iAux  = -1;
    AMP_RPC(result, AMP_DISP_SetPlaneZOrder, amp_sink->Disp, 0, &amp_sink->Zorder);
    VERIFY_RESULT(result);

    if(amp_sink->is_zoomSet) {
        if (1 == amp_sink->zoom) {
            if( amp_sink->resolution == 2160 ) {
                amp_sink->SrcWin.iX=0;
                amp_sink->SrcWin.iY=0;
                amp_sink->SrcWin.iWidth=0;
                amp_sink->SrcWin.iHeight=0;

                amp_sink->DstWin.iX=0;
                amp_sink->DstWin.iY=0;
                amp_sink->DstWin.iWidth=3840;
                amp_sink->DstWin.iHeight=2160;
            } else if( amp_sink->resolution == 1080 ) {
                amp_sink->SrcWin.iX=0;
                amp_sink->SrcWin.iY=0;
                amp_sink->SrcWin.iWidth=0;
                amp_sink->SrcWin.iHeight=0;

                amp_sink->DstWin.iX=0;
                amp_sink->DstWin.iY=0;
                amp_sink->DstWin.iWidth=1920;
                amp_sink->DstWin.iHeight=1080;
            } else if( amp_sink->resolution == 720 ) {
                amp_sink->SrcWin.iX=0;
                amp_sink->SrcWin.iY=0;
                amp_sink->SrcWin.iWidth=0;
                amp_sink->SrcWin.iHeight=0;

                amp_sink->DstWin.iX=0;
                amp_sink->DstWin.iY=0;
                amp_sink->DstWin.iWidth=1280;
                amp_sink->DstWin.iHeight=720;
            }
        } else if(0 == amp_sink->zoom) {
            amp_sink->SrcWin.iX=0;
            amp_sink->SrcWin.iY=0;
            amp_sink->SrcWin.iWidth=0;
            amp_sink->SrcWin.iHeight=0;

            amp_sink->DstWin.iX=0;
            amp_sink->DstWin.iY=0;
            amp_sink->DstWin.iWidth=0;
            amp_sink->DstWin.iHeight=0;

        }
        AMP_RPC(result, AMP_DISP_SetScale, amp_sink->Disp,0, &amp_sink->SrcWin, &amp_sink->DstWin);
        VERIFY_RESULT(result);
        amp_sink->is_zoomSet=false;
    }
    return result;
}

static HRESULT amp_vout_open(Gstampvsink* amp_sink) {
    HRESULT result;
    int cnt = 0;
    UINT32 resId = 0;
    AMP_COMPONENT_CONFIG config;

    AmpMemClear(&config, sizeof (AMP_COMPONENT_CONFIG));
    config._d = AMP_COMPONENT_VOUT;
    config._u.pVOUT.mode = AMP_TUNNEL;
    config._u.pVOUT.uiPlaneID = AMP_DISP_PLANE_MAIN;
    config._u.pVOUT.uiInputPortNum = 2;
    config._u.pVOUT.uiOutputPortNum = 0;

    AMP_RPC(result, AMP_VOUT_Open, amp_sink->hVout, &config);
    VERIFY_RESULT(result);

    result = amp_vout_event_register(amp_sink);
    VERIFY_RESULT(result);
    amp_sink->is_eos = false;

    AMP_RPC(result,
            AMP_VOUT_SetLastFrameMode,
            amp_sink->hVout,
            AMP_VOUT_REPEATLASTFRAME);
    VERIFY_RESULT(result);

    return result;
}

static HRESULT amp_vout_close(Gstampvsink* amp_sink) {
    HRESULT result = SUCCESS;
    if(amp_sink->hVout) {
        amp_vout_unregister(amp_sink);
        AMP_RPC(result, AMP_VOUT_Close,amp_sink->hVout);
        VERIFY_RESULT(result);
    }
    return result;
}

static void amp_vout_destroy(Gstampvsink* amp_sink) {
    if(amp_sink->hVout) {
        HRESULT result;
        AMP_RPC(result, AMP_VOUT_Destroy,amp_sink->hVout);
        VERIFY_RESULT(result);
        amp_sink->hVout = NULL;
    }
}

static HRESULT amp_vdec_callback(CORBA_Object hCompObj,
        AMP_PORT_IO ePortIo,
        UINT32 uiPortIdx,
        struct AMP_BD_ST *hBD,
        AMP_IN void *pUserData)
{

    HRESULT ret;
    Gstampvsink *amp_sink = (Gstampvsink*)pUserData;
    AMP_BDTAG_MEMINFO *mem_info = NULL;

    if (ePortIo == AMP_PORT_INPUT) {
        ret = AMP_BDTAG_GetWithType(hBD,AMP_BDTAG_ASSOCIATE_MEM_INFO, NULL, (void**)&mem_info);
        VERIFY_RESULT(ret);
        if(!amp_sink->secure) {
            amp_sink->rp_v = (mem_info->uMemOffset + mem_info->uSize);
        } else {
            /* release SHM allocated by decryptor */
            AMP_SHM_Unref(mem_info->uMemHandle);
        }

        GST_LOG("vdec receive BD:%d ro:%x\n", AMP_BD_GET_BDID(hBD), amp_sink->rp_v);
        ret = AMP_BDCHAIN_PushItem(amp_sink->video_stream_queue, hBD);
        VERIFY_RESULT(ret);
    }
    return SUCCESS;
}

static HRESULT amp_vdec_event_callback(HANDLE hListener, AMP_EVENT *pEvent,
        VOID *pUserData)
{
    UINT32 *payload = (UINT32*)AMP_EVENT_PAYLOAD_PTR(pEvent);
    switch (AMP_EVENT_GETCODE(*pEvent)) {
        case AMP_EVENT_API_VDEC_CALLBACK: {
            switch (AMP_EVENT_GETPAR1(*pEvent)) {
            case AMP_VDEC_EVENT_RES_CHANGE:
                GST_INFO("resolution changed width:%d height:%d\n", payload[0], payload[1]);
                break;
            case AMP_VDEC_EVENT_FR_CHANGE:
                GST_DEBUG("frame rate changed numerator:%d denominator:%d\n",
                        payload[0], payload[1]);
                break;
            case AMP_VDEC_EVENT_AR_CHANGE:
                GST_DEBUG("aspect ratio changed width:%d height:%d\n", payload[0], payload[1]);
                break;
            case AMP_VDEC_EVENT_DISP_CROP_CHANGE:
                break;
            default:
                break;
            }
            break;
        }
        default:
            break;
    }
    return SUCCESS;
}

static HRESULT amp_vdec_open(Gstampvsink *amp_sink)
{
    HRESULT ret;
    AMP_COMPONENT_CONFIG config;

    AmpMemClear(&config, sizeof(AMP_COMPONENT_CONFIG));
    config._d = AMP_COMPONENT_VDEC;
    config._u.pVDEC.mode = AMP_SECURE_TUNNEL;
    config._u.pVDEC.uiType = amp_sink->video_codec;
    config._u.pVDEC.uiFlag |= AMP_VDEC_MODE_FRAME_IN;

    AMP_RPC(ret, AMP_VDEC_Open, amp_sink->hVdec, &config);
    VERIFY_RESULT(ret);

    ret = AMP_ConnectApp(amp_sink->hVdec,
            AMP_PORT_INPUT,
            0,
            amp_vdec_callback,
            amp_sink);
    VERIFY_RESULT(ret);

    amp_sink->hVdecListener = AMP_Event_CreateListener(16, 0);

    ret = AMP_Event_RegisterCallback(amp_sink->hVdecListener,
            AMP_EVENT_API_VDEC_CALLBACK,
            amp_vdec_event_callback,
            amp_sink);
    VERIFY_RESULT(ret);
    AMP_RPC(ret, AMP_VDEC_RegisterNotify, amp_sink->hVdec,
            AMP_Event_GetServiceID(amp_sink->hVdecListener),
            AMP_EVENT_API_VDEC_CALLBACK);
    VERIFY_RESULT(ret);

    return ret;
}

static HRESULT amp_vdec_destroy(Gstampvsink *amp_sink)
{
    HRESULT ret = SUCCESS;
    GST_DEBUG("Destroy Vdec");
    if (amp_sink->hVdec) {
        AMP_RPC(ret, AMP_COMPONENT_Destroy,amp_sink->hVdec);
        VERIFY_RESULT(ret);
        amp_sink->hVdec = NULL;
    }
    return ret;
}

static HRESULT amp_vdec_close(Gstampvsink *amp_sink)
{
    HRESULT ret = SUCCESS;

    GST_DEBUG("Close Vdec");
    if (amp_sink->hVdec) {
        AMP_RPC(ret, AMP_VDEC_UnregisterNotify, amp_sink->hVdec,
                AMP_Event_GetServiceID(amp_sink->hVdecListener),
                AMP_EVENT_API_VDEC_CALLBACK);
        VERIFY_RESULT(ret);

        AMP_Event_UnregisterCallback(amp_sink->hVdecListener,
                AMP_EVENT_API_VDEC_CALLBACK, amp_vdec_event_callback);
        AMP_Event_DestroyListener(amp_sink->hVdecListener);
        amp_sink->hVdecListener = NULL;

        ret = AMP_DisconnectApp(amp_sink->hVdec, AMP_PORT_INPUT,
                0, amp_vdec_callback);
        VERIFY_RESULT(ret);

        AMP_RPC(ret, AMP_VDEC_Close,amp_sink->hVdec);
        VERIFY_RESULT(ret);
    }
    return ret;
}

static HRESULT amp_connect(Gstampvsink *amp_sink)
{
    HRESULT ret;
    GST_DEBUG_OBJECT(amp_sink, "connect vdec to vout");
    ret = AMP_ConnectComp(amp_sink->hVdec, 0, amp_sink->hVout, 0);
    VERIFY_RESULT(ret);
    GST_DEBUG_OBJECT(amp_sink, "connect clk to vout");
    ret = AMP_ConnectComp(amp_sink->hClk, 0, amp_sink->hVout, 1);
    VERIFY_RESULT(ret);
    return ret;
}

static HRESULT amp_disconnect(Gstampvsink *amp_sink)
{
    HRESULT ret;
    GST_DEBUG_OBJECT(amp_sink, "disconnect vdec from vout");
    ret = AMP_DisconnectComp(amp_sink->hVdec, 0, amp_sink->hVout, 0);
    VERIFY_RESULT(ret);
    GST_DEBUG_OBJECT(amp_sink, "disconnect clk from vout");
    ret = AMP_DisconnectComp(amp_sink->hClk, 0, amp_sink->hVout, 1);
    VERIFY_RESULT(ret);
    return ret;
}

static int pre_process_h264(Gstampvsink * amp_sink)
{
    UINT8 nal_start_code[4] = {0x00, 0x00, 0x00, 0x01};
    UINT8 *data = amp_sink->codec_data, *data_end = amp_sink->codec_data + amp_sink->codec_size;
    UINT32 copied = 0, sps_nb = 0, pps_nb = 0;
    UINT8 *priv = amp_sink->video_priv_data;

    if (data + 6 <= data_end) {
        sps_nb = data[5] & 0x1f;
        data += 6;
    }

    while (sps_nb--) {
        UINT32 sps_len = 0;

        if (data + 2 <= data_end) {
            sps_len = data[0] << 8 | data[1];
            data += 2;
        }

        if (sps_len && data + sps_len <= data_end) {
            AmpMemcpy(priv + copied, nal_start_code, 4);
            copied += 4;
            AmpMemcpy(priv + copied, data, sps_len);
            data += sps_len;
            copied += sps_len;
        }
    }

    if (copied) {
        AmpMemSet(priv + copied, 0, 16);
        copied += 16;
    }

    if (data + 1 <= data_end) {
        pps_nb = *data;
        data++;
    }

    while (pps_nb--) {
        UINT32 pps_len = 0;

        if (data + 2 <= data_end) {
            pps_len = data[0] << 8 | data[1];
            data += 2;
        }

        if (pps_len && data + pps_len <= data_end) {
            AmpMemcpy(priv + copied, nal_start_code, 4);
            copied += 4;
            AmpMemcpy(priv + copied, data, pps_len);
            data += pps_len;
            copied += pps_len;
        }
    }
    return copied;
}

//return code.
//>0, data being copied
//<0, fail
static int post_process_h264(Gstampvsink *amp_sink, uint8_t *data, uint32_t size, uint8_t* dst, uint32_t dst_size){
    if(size < 4){
        GST_ERROR("data too short\n");
        return -1;
    }

    if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01){
        //NAL starting code can be detected
        if(dst_size >= size) {
            memcpy(dst, data, size);
            return size;
        } else {
            GST_ERROR("dst buffer too small\n");
            return -1;
        }
    }

    //Need to add NAL starting code.
    uint8_t nal_start_code[4] = {0x00, 0x00, 0x00, 0x01};
    uint8_t *src = data;
    uint32_t rp = 0, wp = 0, nal_len_size = 4, nal_len;

    if (amp_sink->codec_size > 3) {
        nal_len_size = (amp_sink->codec_data[4] & 0x3) + 1;
    }

    while (rp < size) {
        uint32_t n;
        switch (nal_len_size) {
            case 1:
                nal_len = *src;
                break;
            case 2:
                nal_len = src[rp] << 8 | src[rp + 1];
                break;
            case 3:
                nal_len = src[rp] << 16 | src[rp + 1] << 8 |
                    src[rp + 2];
                break;
            case 4:
            default:
                nal_len = src[rp] << 24 | src[rp + 1] << 16 |
                    src[rp + 2] << 8 | src[rp + 3];
                break;
        }
        if(wp + 4 < dst_size) {
            memcpy(dst + wp, nal_start_code, 4);
            rp += nal_len_size; wp += 4;
        }else {
            GST_ERROR("dst buffer too small\n");
            return -1;
        }

        n = nal_len;
        if(rp + n > size){
            GST_ERROR("error, nal_len:%d > data left:%d\n", nal_len, size-rp);
            n = size - rp;
        }

        if(wp + n > dst_size) {
            GST_ERROR("dst buffer too small\n");
            return -1;
        }
        memcpy(dst + wp, data + rp, n);
        rp += n;
        wp += n;
    }

    return wp;
}

static int pre_process_h265(Gstampvsink* amp_sink) {
    UINT8 nal_start_code[4] = {0x00, 0x00, 0x00, 0x01};
    UINT32 size = amp_sink->codec_size;
    UINT8 *data = amp_sink->codec_data;
    UINT8 *data_end = amp_sink->codec_data + amp_sink->codec_size;
    UINT32 copied = 0;
    int i, j, num_arrays, nal_len_size = 0;
    UINT8 * priv = amp_sink->video_priv_data;

    //Get code from ffmpeg hevc.c hevc_decode_extradata()
    if(size > 3 && (data[0] || data[1] || data[2] > 1)){
        /* It seems the extradata is encoded as hvcC format.
         * Temporarily, we support configurationVersion==0 until 14496-15 3rd
         * is finalized. When finalized, configurationVersion will be 1 and we
         * can recognize hvcC by checking if avctx->extradata[0]==1 or not. */

        if(data_end - data >= 23){
            data += 21;
            nal_len_size = (data[0]&0x3) + 1;
            num_arrays = data[1];
            data += 2;
        }else{
            return -1;
        }

        /* Decode nal units from hvcC. */
        for (i = 0; i < num_arrays; i++) {
            if ( (int)(data_end - data) < 3) {
                return -1;
            }

            //int type = data[0] & 0x3f;
            data++;
            int cnt  = data[0]<<8 | data[1];
            data += 2;

            for (j = 0; j < cnt; j++) {
                // +2 for the nal size field
                int nalsize = (data[0]<<8 | data[1]);
                data += 2;
                if ( (int)(data_end - data) < nalsize) {
                    return -1;
                }

                AmpMemcpy(priv + copied, nal_start_code, 4);
                copied += 4;
                AmpMemcpy(priv + copied, data, nalsize);
                data += nalsize;
                copied += nalsize;
            }
        }
    }else{
        GST_ERROR("No hvcC format found\n");
    }

    amp_sink->nal_len_size = nal_len_size;

    return copied;
}

//return code.
//>0, data being copied
//<0, fail
static int post_process_h265(Gstampvsink *amp_sink, uint8_t *data,
        uint32_t size, uint8_t* dst, uint32_t dst_size)
{
    if(size < 4){
        GST_ERROR("data too short\n");
        return -1;
    }

    if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01){
        //NAL starting code can be detected
        memcpy(dst, data, size);
        return size;
    }

    //Need to add NAL starting code.
    uint8_t nal_start_code[4] = {0x00, 0x00, 0x00, 0x01};
    uint8_t *src = data;
    uint32_t rp = 0, wp = 0;
    uint32_t nal_len_size = amp_sink->nal_len_size;
    uint32_t nal_len = 0;

    while (rp < size) {
        uint32_t n;
        switch (nal_len_size) {
            case 1:
                nal_len = *src;
                break;
            case 2:
                nal_len = src[rp] << 8 | src[rp + 1];
                break;
            case 3:
                nal_len = src[rp] << 16 | src[rp + 1] << 8 |
                    src[rp + 2];
                break;
            case 4:
            default:
                nal_len = src[rp] << 24 | src[rp + 1] << 16 |
                    src[rp + 2] << 8 | src[rp + 3];
                break;
        }
        if(wp + 4 < dst_size) {
            memcpy(dst + wp, nal_start_code, 4);
            rp += nal_len_size; wp += 4;
        }else {
            GST_ERROR("dst buffer too small\n");
            return -1;
        }

        n = nal_len;
        if(rp + n > size){
            GST_ERROR("error, nal_len:%d > data left:%d\n", nal_len, size-rp);
            n = size - rp;
        }

        if(wp + n > dst_size) {
            GST_ERROR("dst buffer too small\n");
            return -1;
        }
        memcpy(dst + wp, data + rp, n);
        rp += n;
        wp += n;
    }

    return wp;
}

static gboolean vdec_get_private(Gstampvsink * amp_sink)
{
    int priv_len = -1;

    switch(amp_sink->video_codec){
        case MEDIA_VES_AVC:
            priv_len = pre_process_h264(amp_sink);
            break;
        case MEDIA_VES_HEVC:
            priv_len = pre_process_h265(amp_sink);
            break;
        default:
            return TRUE;
    }

    if(priv_len < 0) {
        return FALSE;
    }
    amp_sink->video_priv_len = priv_len;
    amp_sink->video_priv_done = FALSE;
    return TRUE;
}

static HRESULT amp_allocate_buffer(Gstampvsink *amp_sink)
{
    HRESULT ret;
    AMP_BD_HANDLE buf_desc;
    int i = 0;
    AMP_BDTAG_MEMINFO mem_info;
    AMP_BDTAG_UNITSTART unit_start;

    ret = AMP_BDCHAIN_Create(true, &amp_sink->video_stream_queue);
    VERIFY_RESULT(ret);

    ret = AMP_SHM_Allocate(AMP_SHM_DYNAMIC,
            VIDEO_ES_BUFFER_SIZE,
            1024,
            &amp_sink->shm_v);
    VERIFY_RESULT(ret);

    ret = AMP_SHM_GetVirtualAddress(amp_sink->shm_v, 0,
            (void **)&amp_sink->addr_v);
    VERIFY_RESULT(ret);

    amp_sink->wp_v = 0;
    amp_sink->rp_v = 0;

    mem_info.Header.eType = AMP_BDTAG_ASSOCIATE_MEM_INFO;
    mem_info.Header.uLength = sizeof(AMP_BDTAG_MEMINFO);
    mem_info.uMemHandle = amp_sink->shm_v;
    mem_info.uMemOffset = 0;
    mem_info.uSize = 0;

    unit_start.Header.eType = AMP_BDTAG_BS_UNITSTART_CTRL;
    unit_start.Header.uLength = sizeof(AMP_BDTAG_UNITSTART);


    for (i = 0; i < MAX_VIDEO_STREAM_SIZE; i++) {
        ret = AMP_BD_Allocate(&buf_desc);
        VERIFY_RESULT(ret);

        ret = AMP_BDTAG_Append(buf_desc, (UINT8 *)&mem_info, NULL, NULL);
        VERIFY_RESULT(ret);

        ret = AMP_BDTAG_Append(buf_desc, (UINT8 *)&unit_start, NULL, NULL);
        VERIFY_RESULT(ret);

        ret = AMP_BDCHAIN_PushItem(amp_sink->video_stream_queue,
                buf_desc);
        VERIFY_RESULT(ret);
    }
    return ret;
}

static void amp_free_buffer(Gstampvsink *amp_sink)
{
    HRESULT ret;
    AMP_BD_HANDLE buf_desc;
    UINT32 num_bd_allocated;
    UINT32 i;
    ret = AMP_BDCHAIN_GetItemNum(amp_sink->video_stream_queue,
            &num_bd_allocated);
    VERIFY_RESULT(ret);

    for (i = 0; i < num_bd_allocated; i++) {
        ret = AMP_BDCHAIN_PopItem(amp_sink->video_stream_queue,
                &buf_desc);
        if(ret == SUCCESS)
            AMP_BD_Free(buf_desc);
    }

    if(amp_sink->video_stream_queue)
        AMP_BDCHAIN_Destroy(amp_sink->video_stream_queue);

    if(amp_sink->shm_v)
        AMP_SHM_Release(amp_sink->shm_v);
}

static gboolean same_meta_data(UINT8* old_p, UINT32 old_s, UINT8* p, UINT32 s)
{
    if(old_s != s)
        return FALSE;
    if(old_p == NULL && p == NULL)
        return TRUE;
    if(old_p == NULL)
        return FALSE;
    return (memcmp(old_p, p, s) == 0);
}

static gboolean amp_handle_event_caps(Gstampvsink* amp_sink, GstCaps* caps)
{
    GstStructure *caps_structure = gst_caps_get_structure(caps, 0);
    UINT32 codec_int = 0;
    UINT8* codec_data = NULL;
    UINT32 codec_size = 0;

    const GValue *buffer_point = gst_structure_get_value(caps_structure,"codec_data");
    if(buffer_point) {
        GstBuffer *buffer = gst_value_get_buffer(buffer_point);

        codec_size = gst_buffer_get_size(buffer);
        codec_data = (UINT8*)g_malloc(codec_size);
        if(!codec_data){
            GST_ERROR("Memory allocation failed for codec data");
            return false;
        }
        gst_buffer_extract(buffer, 0, codec_data, codec_size);
    } else {
        GST_WARNING("Codec data in not present in caps structure");
    }

    const gchar * stream_format = gst_structure_get_name(caps_structure);
    if(g_strrstr(stream_format,"video/x-h264")){
        GST_INFO("video codec is H.264 / AVC\n");
        codec_int = MEDIA_VES_AVC;
    }else if(g_strrstr(stream_format,"video/x-h265")){
        GST_INFO("video codec is H.265 / HEVC\n");
        codec_int = MEDIA_VES_HEVC;
    }else if(g_strrstr(stream_format,"video/x-vp9")){
        GST_INFO("video codec is VP9\n");
        codec_int = MEDIA_VES_VP9;

    }else if(g_strrstr(stream_format,"video/x-vp8")){
        GST_INFO("video codec is VP8\n");
        codec_int = MEDIA_VES_VP8;
    }else{
        GST_ERROR("Unsuported codec format %s\n", stream_format);
        g_free(codec_data);
        return false;
    }

    if (amp_sink->video_codec == codec_int &&
        same_meta_data(amp_sink->codec_data, amp_sink->codec_size, codec_data, codec_size)) {
        GST_WARNING("Video codec is unchanged\n");
        if(codec_data)
            g_free(codec_data);
        return true;
    }

    if(amp_sink->codec_data != NULL) {
        g_free(amp_sink->codec_data);
        amp_sink->codec_data = NULL;
        amp_sink->codec_size = 0;
    }

    if(codec_size) {
        amp_sink->codec_size = codec_size;
        amp_sink->codec_data = (UINT8*)g_malloc(codec_size);
        memcpy(amp_sink->codec_data, codec_data, codec_size);
        g_free(codec_data);
    }

    amp_sink->video_codec = codec_int;
    if(!vdec_get_private(amp_sink)) {
        GST_ERROR("pasre ES header error\n");
        goto error;
    }
    if(!amp_sink->pipeline_created) {
        if(amp_vdec_open(amp_sink) != SUCCESS){
            GST_ERROR("amp_vdec_open failed");
            goto error;
        }

        if(amp_connect(amp_sink) != SUCCESS){
            GST_ERROR("amp_connect failed");
            goto error;
        }

        amp_sink->pipeline_created = TRUE;
        if(amp_sink_set_state(amp_sink, AMP_EXECUTING) != SUCCESS) {
            GST_ERROR("set state failed");
            goto error;
        }
    }
    return true;
error:
    g_free(amp_sink->codec_data);
    amp_sink->codec_data = NULL;
    amp_sink->codec_size = 0;
    return false;
}

static void amp_reset_meta(Gstampvsink * amp_sink)
{
    amp_sink->secure = FALSE;
    amp_sink->shm_v = 0;
    amp_sink->wp_v = 0;
    amp_sink->rp_v = 0;
    amp_sink->stream_pos = 0;
    amp_sink->addr_v = NULL;
    amp_sink->video_codec = 0;
    amp_sink->video_priv_len = 0;
    amp_sink->video_priv_done = false;
    if(amp_sink->codec_data)
        g_free(amp_sink->codec_data);
    amp_sink->video_priv_len = 0;
    amp_sink->codec_data = NULL;
    amp_sink->codec_size = 0;
    amp_sink->nal_len_size = 0;
    amp_sink->video_stream_queue = NULL;
    amp_sink->hVdec = NULL;
    amp_sink->hVdecListener = NULL;
    if(amp_sink->cap_accepted) {
        gst_caps_unref(amp_sink->cap_accepted);
        amp_sink->cap_accepted = NULL;
    }
}

static gboolean gst_ampvsink_send_event (GstElement * element, GstEvent * event)
{
  HRESULT result = SUCCESS;
  GstWesterosSink *sink = (GstWesterosSink *)element;
  Gstampvsink *amp_sink = &sink->soc.amp_sink;
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
    {
      GstPad *demux_pad = gst_pad_get_peer (amp_sink->sinkpad);
      if (demux_pad != NULL) {
        gst_pad_send_event (demux_pad, event);
        gst_object_unref(demux_pad);
      }
      break;
    }
    default:
      gst_event_unref(event);
      break;
  }
  return true;
}

static GstCaps *
gst_amp_sink_sink_getcaps (GstWesterosSink* sink, GstCaps * filter)
{
    Gstampvsink *amp_sink = &(sink->soc.amp_sink);
    GstCaps *templ_caps = gst_pad_get_pad_template_caps (amp_sink->sinkpad);
    if(filter) {
        GstCaps *caps = gst_caps_intersect (filter, templ_caps);
        gst_caps_unref (templ_caps);
        return caps;
    } else {
        return templ_caps;
    }
}

static gboolean
gst_westeros_sink_sink_query (GstPad * pad, GstObject * parent, GstQuery * query)
{
    gboolean ret = FALSE;
    GstWesterosSink *sink = GST_WESTEROS_SINK(parent);

    GST_DEBUG_OBJECT (sink, "received sink query %d, %s", GST_QUERY_TYPE (query),
            GST_QUERY_TYPE_NAME (query));

    switch (GST_QUERY_TYPE (query)) {
        case GST_QUERY_CAPS:
        {
            GstCaps *filter, *caps;
            gst_query_parse_caps (query, &filter);
            GST_LOG_OBJECT (sink, "filter caps %" GST_PTR_FORMAT, filter);
            caps = gst_amp_sink_sink_getcaps (sink, filter);
            gst_query_set_caps_result (query, caps);
            GST_LOG_OBJECT (sink, "return caps %" GST_PTR_FORMAT, caps);
            gst_caps_unref (caps);
            ret = TRUE;
            break;
        }
        case GST_QUERY_ACCEPT_CAPS:
        {
            gst_westeros_sink_soc_query_accept_caps(sink, query);
            ret = TRUE;
            break;
        }
        default:
            ret = gst_pad_query_default (pad, parent, query);
    }
    return ret;
}

/**
 * Create the pads ,register and add the pad to the plugin
 */
static gboolean gst_createpads_amp_sink(GstWesterosSink *sink) {
    Gstampvsink *amp_sink = &(sink->soc.amp_sink);
    /* data pad */
    amp_sink->sinkpad = gst_pad_new_from_static_template(&sink_amp_sink_pad, "sink");

    gst_pad_set_event_function (amp_sink->sinkpad,
            GST_DEBUG_FUNCPTR(gst_amp_sink_event));
    gst_pad_set_chain_function (amp_sink->sinkpad,
            GST_DEBUG_FUNCPTR(gst_amp_vdec_chain));
    gst_pad_set_query_function (amp_sink->sinkpad,
            GST_DEBUG_FUNCPTR (gst_westeros_sink_sink_query));
    gst_element_add_pad(GST_ELEMENT(sink), amp_sink->sinkpad);

    return TRUE;
}

/**
 *  Function called when the declared arguments of the plugin are to be set.
 */
static void gst_ampvsink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstWesterosSink *sink = GST_WESTEROS_SINK(object);
  Gstampvsink *amp_sink = &sink->soc.amp_sink;
  GstElement *element = GST_ELEMENT(sink);
  AMP_STATE eState;
  gchar **rectangle;
  HRESULT result;
  gint tvmode;
  FILE *fDsPersistent = fopen("/opt/ds/hostData","r");
  char dsbuffer[200];
  switch (prop_id) {
    case PROP_TV_MODE:
    {
        tvmode = g_value_get_enum (value);
        if( 0 == tvmode)
            amp_sink->tvmode = AMP_DISP_OUT_RES_FIRST;
        else if(1 == tvmode)
            amp_sink->tvmode = AMP_DISP_OUT_RES_480P60;
        else if(2 == tvmode)
            amp_sink->tvmode = AMP_DISP_OUT_RES_PAL_M;
        else if(3 == tvmode)
            amp_sink->tvmode = AMP_DISP_OUT_RES_PAL_BGH;
        else if(4 == tvmode)
            amp_sink->tvmode = AMP_DISP_OUT_RES_720P60;
        else if(5 == tvmode)
            amp_sink->tvmode = AMP_DISP_OUT_RES_720P50;
        else if(6 == tvmode)
            amp_sink->tvmode = AMP_DISP_OUT_RES_1080I60;
        else if(7 == tvmode)
            amp_sink->tvmode = AMP_DISP_OUT_RES_1080I50;
        else if(8 == tvmode)
            amp_sink->tvmode = AMP_DISP_OUT_RES_1080P60;
        else if(9 == tvmode)
            amp_sink->tvmode = AMP_DISP_OUT_RES_1080P50;

        GST_INFO_OBJECT(sink, "tvmode is %d \n",amp_sink->tvmode);
        AMP_RPC(result,
                AMP_DISP_OUT_SetResolution,
                amp_sink->Disp,
                AMP_DISP_PLANE_MAIN,
                amp_sink->tvmode,
                AMP_DISP_OUT_BIT_DPE_8);
        VERIFY_RESULT(result);

        AMP_RPC(result,
                AMP_DISP_OUT_HDMI_SetVidFmt,
                amp_sink->Disp,
                AMP_DISP_OUT_CLR_FMT_RGB888,
                AMP_DISP_OUT_BIT_DPE_8,
                1);
        VERIFY_RESULT(result);
        amp_sink->tvmode = tvmode;
        break;
    }
    case PROP_PLANE:
    {
        amp_sink->plane = g_value_get_enum (value);
        if(amp_sink->hVout) {
            AMP_RPC(result, AMP_VOUT_GetState, amp_sink->hVout,&eState);
            VERIFY_RESULT(result);
            if(eState != AMP_EXECUTING) {
                AMP_RPC(result, AMP_DISP_SetPlaneZOrder, amp_sink->Disp, amp_sink->plane, &(amp_sink->Zorder));
            } else {
                GST_DEBUG(" VOUT is in AMP_EXECUTING so can't set the property");
            }
        } else {
            GST_DEBUG(" Parent Handle is Null ");
        }
        break;
    }
    case PROP_RECTANGLE:
    {
        rectangle = g_strsplit (g_value_get_string (value), ",", -1);

        amp_sink->DstWin.iX=strtol (rectangle[0], NULL, 0);
        amp_sink->DstWin.iY= strtol (rectangle[1], NULL, 0);

        amp_sink->DstWin.iWidth= strtol (rectangle[2], NULL, 0);
        amp_sink->DstWin.iHeight= strtol (rectangle[3], NULL, 0);

        GST_DEBUG_OBJECT(sink, "The rectangle values are %d , %d ,%d, %d ",
                amp_sink->DstWin.iX ,amp_sink->DstWin.iY,amp_sink->DstWin.iWidth,amp_sink->DstWin.iHeight);

        //printf("AMPVOUT Resolution = %d\n", resolution);
        if(fDsPersistent != NULL) {
            fread(dsbuffer, sizeof(dsbuffer), 1, fDsPersistent);
            if(strstr(dsbuffer, "720")){
                amp_sink->resolution = 720;
            }else if(strstr(dsbuffer, "1080")){
                amp_sink->resolution = 1080;
            }else if(strstr(dsbuffer, "2160")){
                amp_sink->resolution = 2160;
            }else{
                g_print("[AMPVOUT] Resolution default case\n");
                amp_sink->resolution = 720;
            }
            fclose(fDsPersistent);
        } else {
            amp_sink->resolution=720;
            //  g_print("[AMPVOUT] Invalid ds persistent file discriptor \n");
        }

        if (1 == amp_sink->zoom) {
            if( amp_sink->resolution == 2160 ) {
                amp_sink->resolution = 2160;
                amp_sink->SrcWin.iX=0;
                amp_sink->SrcWin.iY=0;
                amp_sink->SrcWin.iWidth=0;
                amp_sink->SrcWin.iHeight=0;

                amp_sink->DstWin.iX=0;
                amp_sink->DstWin.iY=0;
                amp_sink->DstWin.iWidth=3840;
                amp_sink->DstWin.iHeight=2160;
            } else if( amp_sink->resolution == 1080 ) {
                amp_sink->resolution = 1080;
                amp_sink->SrcWin.iX=0;
                amp_sink->SrcWin.iY=0;
                amp_sink->SrcWin.iWidth=0;
                amp_sink->SrcWin.iHeight=0;

                amp_sink->DstWin.iX=0;
                amp_sink->DstWin.iY=0;
                amp_sink->DstWin.iWidth=1920;
                amp_sink->DstWin.iHeight=1080;
            } else if( amp_sink->resolution == 720 ) {
                amp_sink->resolution = 720;
                amp_sink->SrcWin.iX=0;
                amp_sink->SrcWin.iY=0;
                amp_sink->SrcWin.iWidth=0;
                amp_sink->SrcWin.iHeight=0;

                amp_sink->DstWin.iX=0;
                amp_sink->DstWin.iY=0;
                amp_sink->DstWin.iWidth=1280;
                amp_sink->DstWin.iHeight=720;
            }
        } else if(0 == amp_sink->zoom) {
            amp_sink->SrcWin.iX=0;
            amp_sink->SrcWin.iY=0;
            amp_sink->SrcWin.iWidth=0;
            amp_sink->SrcWin.iHeight=0;
            /* modified for video scalling issue :
               use rectangle passed from Mediaplayersink if window is lesser than 1080 */
            if((amp_sink->DstWin.iWidth >= 1920) ||(amp_sink->DstWin.iHeight >=1080))
            {
                amp_sink->DstWin.iX=0;
                amp_sink->DstWin.iY=0;
                amp_sink->DstWin.iWidth=0;
                amp_sink->DstWin.iHeight=0;

            }
        }

        if(amp_sink->Disp) {
            UINT32 resId = 0;
            /* query if disp out is set to 4k, scale video rectangle for 4k */
            AMP_RPC(result,
                    AMP_DISP_OUT_GetResolution,
                    amp_sink->Disp,
                    AMP_DISP_PLANE_MAIN,
                    &resId);
            VERIFY_RESULT(result);
            if((resId >= AMP_DISP_OUT_RES_4Kx2K_MIN) && (resId < AMP_DISP_OUT_RES_4Kx2K_MAX))
            {
                /* scale the video rectangle window for 4k*/
                amp_sink->DstWin.iX *= 2;
                amp_sink->DstWin.iY *= 2;
                amp_sink->DstWin.iWidth *= 2;
                amp_sink->DstWin.iHeight *= 2;
            }
            AMP_RPC(result, AMP_DISP_SetScale, amp_sink->Disp, amp_sink->plane, &amp_sink->SrcWin, &amp_sink->DstWin);
            VERIFY_RESULT(result);
        } else {
            GST_DEBUG(" Parent Handle is Null ");
        }
        g_strfreev (rectangle);
        break;
    }
    case PROP_FLUSH_REPEAT_FRAME:
    {
        amp_sink->repeatframe = g_value_get_boolean (value);
        if(amp_sink->hVout) {
            if(amp_sink->repeatframe) {
                AMP_RPC(result,AMP_VOUT_SetLastFrameMode, amp_sink->hVout, AMP_VOUT_REPEATLASTFRAME);
                VERIFY_RESULT(result);
            } else {
                AMP_RPC(result,AMP_VOUT_SetLastFrameMode, amp_sink->hVout, AMP_VOUT_SHOWBLACKSCREEN);
                VERIFY_RESULT(result);
            }
        }
        break;
    }
    case PROP_INTER_FRAME_DELAY:
      amp_sink->framedelay = g_value_get_uint (value);
      break;
    case PROP_SLOW_MODE_RATE:
      amp_sink->slowmoderate = g_value_get_int  (value);
      break;
    case PROP_STEP_FRAME:
      amp_sink->steprate = g_value_get_uint  (value);
      break;
    case PROP_MUTE:
    {
        amp_sink->mute = g_value_get_uint (value);
        if(amp_sink->Disp) {
            AMP_RPC(result, AMP_DISP_SetPlaneMute, amp_sink->Disp, amp_sink->plane, amp_sink->mute);
            VERIFY_RESULT(result);
        }
        break;
    }
    case PROP_ZOOM:
      amp_sink->zoom = g_value_get_uint(value);
      amp_sink->is_zoomSet=true;
      break;
    case PROP_PLAY_SPEED:
    {
        guint rate;
        amp_sink->play_speed = g_value_get_float (value);
        rate = 1000* amp_sink->play_speed;
        GstClock* clk = gst_element_get_clock(element);
        if(clk) {
            g_object_set(G_OBJECT(clk), "clk-rate", rate, NULL);
            gst_object_unref(clk);
        }
        break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


/**
 *   Function called when the declared arguments of the plugin is to be fetched.
 */
static void gst_ampvsink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstWesterosSink *sink = GST_WESTEROS_SINK(object);
  Gstampvsink *amp_sink = &sink->soc.amp_sink;
  HRESULT result;
  GString *rectangle;
  UINT32 left;
  UINT32 right;
  switch (prop_id) {
    case PROP_TV_MODE:
      g_value_set_enum (value, amp_sink->tvmode);
      break;
    case PROP_PLANE:
      g_value_set_enum (value, amp_sink->plane);
      break;
    case PROP_RECTANGLE:
      rectangle  = g_string_sized_new (32);
      g_string_append_printf (rectangle, "%d", amp_sink->DstWin.iX);
      g_string_append_printf (rectangle, ",%d", amp_sink->DstWin.iY);
      g_string_append_printf (rectangle, ",%d", amp_sink->DstWin.iWidth);
      g_string_append_printf (rectangle, ",%d", amp_sink->DstWin.iHeight);
      g_value_take_string (value, g_string_free (rectangle, FALSE));

      break;
    case PROP_FLUSH_REPEAT_FRAME:
      g_value_set_boolean (value, amp_sink->repeatframe);
      break;
    case PROP_INTER_FRAME_DELAY:
      g_value_set_uint (value, amp_sink->framedelay);
      break;
    case PROP_SLOW_MODE_RATE:
      g_value_set_int (value, amp_sink->slowmoderate);
      break;
    case PROP_CURRENT_PTS:
      if(amp_sink->hVout){
        AMP_RPC(result, AMP_VOUT_GetCurrentPTS, amp_sink->hVout,&left,&right);
        amp_sink->currentpts = (long)(uint)right;
      } else {
        GST_DEBUG(" Parent Handle is Null ");
      }
      g_value_set_ulong (value, amp_sink->currentpts);
      break;
    case PROP_MUTE:
      if(amp_sink->Disp){
        AMP_RPC(result, AMP_DISP_GetPlaneMute, amp_sink->Disp, amp_sink->plane,&amp_sink->mute);
      } else {
        GST_DEBUG(" Parent Handle is Null ");
      }
      g_value_set_uint (value, amp_sink->mute);
      break;
    case PROP_CONTENT_FRAME_RATE:
      g_value_set_uint (value, amp_sink->contentframerate);
      break;
    case PROP_ZOOM:
       g_value_set_uint (value, amp_sink->zoom);
       break;
    case PROP_VOUT:
       g_value_set_pointer (value, amp_sink->hVout);
       break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void waiting_for_BD_reset_buf(Gstampvsink* amp_sink)
{
    HRESULT ret;
    UINT32 num_bd_remained;
    UINT32 timeout = 100;
    do {
        ret = AMP_BDCHAIN_GetItemNum(amp_sink->video_stream_queue,
                &num_bd_remained);
        VERIFY_RESULT(ret);
        if(num_bd_remained == MAX_VIDEO_STREAM_SIZE) {
            break;
        } else {
            usleep(10000);
            timeout--;
            if(timeout == 0)
                break;
        }
    }while(1);

    if(timeout == 0)
        GST_ERROR("BD recycling timeout");
    else
        GST_DEBUG("all Video BD returned\n");

    amp_sink->wp_v = 0;
    amp_sink->rp_v = 0;
    amp_sink->stream_pos = 0;
    amp_sink->video_priv_done = FALSE;
}

static void feed_thread_init(Gstampvsink *amp_sink)
{
    g_cond_init(&amp_sink->feed_cond);
    g_mutex_init(&amp_sink->feed_mutex);
    amp_sink->buf_q = g_queue_new();
    amp_sink->stop = FALSE;
    amp_sink->feed_t = g_thread_new("video feed thread", vsink_feed_task, amp_sink);
}

static void feed_thread_stop(Gstampvsink *amp_sink)
{
    g_mutex_lock(&amp_sink->feed_mutex);
    amp_sink->stop = TRUE;
    g_cond_signal(&amp_sink->feed_cond);
    g_mutex_unlock(&amp_sink->feed_mutex);
}

static void feed_thread_push(Gstampvsink *amp_sink, GstBuffer* buf)
{
    g_mutex_lock(&amp_sink->feed_mutex);
    g_queue_push_tail (amp_sink->buf_q, buf);
    g_cond_signal(&amp_sink->feed_cond);
    g_mutex_unlock(&amp_sink->feed_mutex);
}


static void buf_destroy(gpointer data)
{
    GstBuffer * buf = (GstBuffer *)data;
    gst_buffer_unref(buf);
}

static void feed_thread_destroy(Gstampvsink *amp_sink)
{
    g_thread_join(amp_sink->feed_t);
    g_cond_clear(&amp_sink->feed_cond);
    g_mutex_clear(&amp_sink->feed_mutex);
    g_queue_free_full (amp_sink->buf_q, buf_destroy);
}


/**
 * To handle the change state of the plugin
 */
static GstStateChangeReturn gst_ampvsink_change_state (GstElement * element, GstStateChange transition)
{
  HRESULT result = SUCCESS;
  AMP_STATE eState;
  g_return_val_if_fail (GST_IS_WESTEROS_SINK (element), GST_STATE_CHANGE_FAILURE);

  GstWesterosSink *sink = GST_WESTEROS_SINK(element);
  Gstampvsink *amp_sink = &sink->soc.amp_sink;

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
    {
      GstClock *clk;
      VERIFY_RESULT(result);
      AMP_RPC(result, AMP_FACTORY_CreateComponent,
              hFactory, AMP_COMPONENT_VDEC, 0, &amp_sink->hVdec);
      VERIFY_RESULT(result);
      AMP_RPC(result, AMP_FACTORY_CreateComponent,
              hFactory, AMP_COMPONENT_VOUT, 0, &amp_sink->hVout);
      VERIFY_RESULT(result);

      if(amp_disp_open(amp_sink) != SUCCESS)
          return GST_STATE_CHANGE_FAILURE;

      if(amp_vout_open(amp_sink) != SUCCESS)
          return GST_STATE_CHANGE_FAILURE;

      clk = gst_element_get_clock(element);
      g_object_get(G_OBJECT(clk), "clk-handle", &amp_sink->hClk, NULL);
      GST_DEBUG(" GST_STATE_CHANGE_NULL_TO_READY ");
      gst_object_unref(clk);
      break;
    }
    case GST_STATE_CHANGE_READY_TO_PAUSED:
    {
      feed_thread_init(amp_sink);
      if(amp_allocate_buffer(amp_sink) != SUCCESS){
          GST_ERROR("amp_allocate_buffer failed");
          return GST_STATE_CHANGE_FAILURE;
      }
      GST_DEBUG(" GST_STATE_CHANGE_READY_TO_PAUSED ");
      break;
    }
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
    {
      if(amp_sink->m_isPaused)
      {
        amp_sink->m_isPaused = false;
        if(amp_sink->cap_accepted)
          amp_sink_set_state(amp_sink, AMP_EXECUTING);
      }
      VERIFY_RESULT(result);
      GST_DEBUG(" GST_STATE_CHANGE_PAUSED_TO_PLAYING ");
      break;
    }
    default:
      break;
  }

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
    {
      GST_DEBUG(" GST_STATE_CHANGE_PLAYING_TO_PAUSED ");
      break;
    }
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    {
      feed_thread_stop(amp_sink);
      result = amp_sink_set_state(amp_sink, AMP_IDLE);
      if(amp_sink->pipeline_created) {
          waiting_for_BD_reset_buf(amp_sink);
          amp_disconnect(amp_sink);
          amp_vdec_close(amp_sink);
          amp_sink->pipeline_created = FALSE;
          amp_sink->video_codec = 0;
      }
      amp_free_buffer(amp_sink);
      feed_thread_destroy(amp_sink);
      GST_DEBUG(" GST_STATE_CHANGE_PAUSED_TO_READY ");
      break;
    }
    case GST_STATE_CHANGE_READY_TO_NULL:
    {
      amp_vout_close(amp_sink);
      amp_sink_destroy(amp_sink);
      amp_reset_meta(amp_sink);

      GST_DEBUG(" GST_STATE_CHANGE_READY_TO_NULL ");
      break;
    }
    default:
      break;
  }

  return GST_STATE_CHANGE_SUCCESS;
}

void append_unit_start(AMP_BDTAG_UNITSTART* p, GstClockTime pts, UINT32 pos)
{
    if(p) {
        p->uPtsHigh = (UINT32)(pts >> 32);
        p->uPtsLow = (UINT32)(pts & 0x0FFFFFFFF);
        p->uStrmPos = pos;
    }
}

static FEED_RET feed_protection_data(Gstampvsink *amp_sink, GstBuffer *buf, GstProtectionMeta* meta,
        GstClockTime pts, gboolean eos, UINT8 *private_data, UINT32 private_len) {
    HRESULT ret = -1;
    AMP_BD_HANDLE bd;
    guint hShm;
    gint offset, size;
    UINT32 num_bd_remained;
    AMP_SHM_HANDLE headerShm;
    AMP_BDTAG_MEMINFO* mem_info;
    AMP_BDTAG_UNITSTART* unit_start;

    if(!gst_structure_get_uint(meta->info, "handle", &hShm) ||
            !gst_structure_get_int(meta->info, "offset", &offset) ||
            !gst_structure_get_int(meta->info, "size", &size)) {
        GST_ERROR("corrupted meta\n");
        return FEED_ERR;
    }

    int bd_needed = private_len>0?2:1;
    ret = AMP_BDCHAIN_GetItemNum(amp_sink->video_stream_queue,&num_bd_remained);
    VERIFY_RESULT(ret);
    if(num_bd_remained < bd_needed) {
        return FEED_RETRY;
    }

    if(private_len > 0 && !eos){
        void *p;
        /*This SHM will be released in amp_sink_callback */
        ret = AMP_SHM_Allocate(AMP_SHM_DYNAMIC, private_len, 32, &headerShm);
        VERIFY_RESULT(ret);
        AMP_SHM_GetVirtualAddress(headerShm, 0, &p);
        memcpy(p, private_data, private_len);

        ret = AMP_BDCHAIN_PopItem(amp_sink->video_stream_queue, &bd);
        VERIFY_RESULT(ret);

        ret = AMP_BDTAG_GetWithType(bd, AMP_BDTAG_ASSOCIATE_MEM_INFO, NULL, (void**)&mem_info);
        VERIFY_RESULT(ret);
        mem_info->Header.eType = AMP_BDTAG_ASSOCIATE_MEM_INFO;
        mem_info->Header.uLength = sizeof(AMP_BDTAG_MEMINFO);
        mem_info->uMemHandle = headerShm;
        mem_info->uMemOffset = 0;
        mem_info->uSize = private_len;

        amp_sink->stream_pos += private_len;

        ret = AMP_SHM_CleanCache(headerShm, 0, private_len);
        VERIFY_RESULT(ret);
        AMP_RPC(ret, AMP_VDEC_PushBD, amp_sink->hVdec, AMP_PORT_INPUT, 0, bd);
        VERIFY_RESULT(ret);
        amp_sink->video_priv_done = true;
    }

    ret = AMP_BDCHAIN_PopItem(amp_sink->video_stream_queue, &bd);
    VERIFY_RESULT(ret);

    ret = AMP_BDTAG_GetWithType(bd, AMP_BDTAG_ASSOCIATE_MEM_INFO, NULL, (void**)&mem_info);
    VERIFY_RESULT(ret);
    /* mem info tag*/
    mem_info->Header.eType = AMP_BDTAG_ASSOCIATE_MEM_INFO;
    mem_info->Header.uLength = sizeof(AMP_BDTAG_MEMINFO);
    mem_info->uMemHandle = hShm;
    mem_info->uMemOffset = offset;
    mem_info->uSize = size;

    if (!(eos /*&& !amp_sink->codec_size*/)) {
        mem_info->uFlag &= ~AMP_MEMINFO_FLAG_EOS_MASK;
    } else {
        mem_info->uFlag |= AMP_MEMINFO_FLAG_EOS_MASK;
        GST_INFO("send video eos\n");
    }

    /* pts tag */
    ret = AMP_BDTAG_GetWithType(bd, AMP_BDTAG_BS_UNITSTART_CTRL, NULL, (void**)&unit_start);
    VERIFY_RESULT(ret);
    append_unit_start(unit_start, pts , amp_sink->stream_pos);

    amp_sink->stream_pos += size;

    AMP_RPC(ret, AMP_VDEC_PushBD, amp_sink->hVdec, AMP_PORT_INPUT, 0, bd);
    VERIFY_RESULT(ret);
    return FEED_DONE;
}

static LINEAR_POS enough_video_linear_space(Gstampvsink *amp_sink, UINT32 length)
{
	UINT32 left_end, left_beg;
	if(amp_sink->wp_v > VIDEO_ES_BUFFER_SIZE) {
		GST_ERROR("should not happen\n");
		return NO_LINEAR;
	}
	if (amp_sink->rp_v > amp_sink->wp_v) {
		left_end = amp_sink->rp_v - amp_sink->wp_v;
		left_beg = 0;
	} else {
		left_end = (VIDEO_ES_BUFFER_SIZE - amp_sink->wp_v);
		left_beg = amp_sink->rp_v;
	}
	if(left_end >= length)
		return LINEAR_END;
	else if(left_beg >= length)
		return LINEAR_BEG;
	else
		return NO_LINEAR;
}

static FEED_RET amp_feed_amp_sink(Gstampvsink* amp_sink, GstBuffer *buf)
{
    FEED_RET retval;
    AMP_COMPONENT handle = amp_sink->hVdec;
    HRESULT ret = -1;
    UINT32 num_bd_remained = 0;
    AMP_BD_HANDLE buf_desc;
    AMP_BDTAG_MEMINFO* mem_info;
    AMP_BDTAG_UNITSTART* unit_start;
    UINT8 *buf_mapped = NULL;
    GstMapInfo info;
    gsize orig_length = gst_buffer_get_size(buf);
    UINT32 length = 0;
    GstClockTime pts = GST_BUFFER_PTS(buf);
    //GstClockTime duration = GST_BUFFER_DURATION(buf);
    guint64 eos = GST_BUFFER_OFFSET_END(buf);
    UINT8 *private_data = NULL;
    UINT32 private_len = 0;
    AVRational new_base;
    AVRational time_base;
    LINEAR_POS buf_pos;
    UINT32 buf_max;
    /* rescaling the PTS value with time_base of nano-second to the
       default value of MPEG-TS time_base(i.e.,new_base) for AVSync */
    new_base.num = 1;
    new_base.den = 90000;
    time_base.num=1;
    time_base.den=1000000000;
    if (pts != GST_CLOCK_TIME_NONE && pts >= 0) {
        pts = av_rescale_q(pts,time_base,new_base);
    }
    if (pts == GST_CLOCK_TIME_NONE || pts < 0) {
        pts &= ~(AMP_PTS_VALID_MASK);
    } else {
        pts |= AMP_PTS_VALID_MASK;
    }

    if(amp_sink->video_codec == MEDIA_VES_HEVC ||
            amp_sink->video_codec == MEDIA_VES_AVC) {
        if(!amp_sink->video_priv_done){
            //Always start from I-frame
            if(GST_BUFFER_FLAG_IS_SET(buf, GST_BUFFER_FLAG_DELTA_UNIT)) {
                GST_DEBUG("drop non-I frame\n");
                return FEED_DONE;
            }
            private_len = amp_sink->video_priv_len;
            private_data = amp_sink->video_priv_data;
            if(private_len == 0) {
                GST_WARNING("no ES header in container\n");
                amp_sink->video_priv_done = TRUE;
            }
        }
    } else {
        private_len = amp_sink->video_priv_len;
        private_data = amp_sink->video_priv_data;
    }

    GstProtectionMeta* meta = gst_buffer_get_protection_meta(buf);
    if(meta != NULL){
        amp_sink->secure = TRUE;
        if(amp_sink->shm_v) {
            AMP_SHM_Release(amp_sink->shm_v);
            amp_sink->shm_v = 0;
            amp_sink->addr_v = NULL;
        }
        //non-CC buffer frame-in feeding
        return feed_protection_data(amp_sink, buf, meta, pts, eos!=0, private_data, private_len);
    }

    ret = AMP_BDCHAIN_GetItemNum(amp_sink->video_stream_queue,&num_bd_remained);
    VERIFY_RESULT(ret);
    if(num_bd_remained == 0) {
        return FEED_RETRY;
    }

    //Add 0.5M for NAL padding
    UINT32 needed = orig_length + private_len + 512*1024;
    buf_pos = enough_video_linear_space(amp_sink, needed);
    if(buf_pos == NO_LINEAR) {
        return FEED_RETRY;
    }

    if(buf_pos == LINEAR_END) {
        buf_mapped = amp_sink->addr_v + amp_sink->wp_v;
        buf_max = VIDEO_ES_BUFFER_SIZE - amp_sink->wp_v;
    } else {
        buf_mapped = amp_sink->addr_v;
        buf_max = amp_sink->rp_v;
    }

    if (private_len && !eos) {
        AmpMemcpy(buf_mapped, private_data, private_len);
        amp_sink->video_priv_done = TRUE;
        buf_mapped += private_len;
        length += private_len;
    }

    if(orig_length) {
        gst_buffer_map(buf, &info, GST_MAP_READ);
        if (info.data && info.size > 0) {
            int32_t copied = 0;

            switch(amp_sink->video_codec) {
                case MEDIA_VES_AVC:
                    copied = post_process_h264(amp_sink, info.data, info.size, buf_mapped, buf_max);
                    break;
                case MEDIA_VES_HEVC:
                    copied = post_process_h265(amp_sink, info.data, info.size, buf_mapped, buf_max);
                    break;
                default:
                    memcpy(buf_mapped, info.data, info.size);
                    copied = info.size;
                    break;
            }
            if(copied < 0){
                GST_ERROR("post_process  fail\n");
                goto ERROR;
            }
            length += copied;
        }
    }

    ret = AMP_BDCHAIN_PopItem(amp_sink->video_stream_queue, &buf_desc);
    VERIFY_RESULT(ret);

    /* mem info tag*/
    ret = AMP_BDTAG_GetWithType(buf_desc, AMP_BDTAG_ASSOCIATE_MEM_INFO, NULL, (void**)&mem_info);
    VERIFY_RESULT(ret);
    mem_info->Header.eType = AMP_BDTAG_ASSOCIATE_MEM_INFO;
    mem_info->Header.uLength = sizeof(AMP_BDTAG_MEMINFO);
    mem_info->uMemHandle = amp_sink->shm_v;

    if(buf_pos == LINEAR_END)
        mem_info->uMemOffset = amp_sink->wp_v;
    else
        mem_info->uMemOffset = 0;
    mem_info->uSize = length;
    mem_info->uFlag = AMP_MEMINFO_FLAG_DATA_IN_CACHE_MASK;
    if (!eos) {
        mem_info->uFlag &= ~AMP_MEMINFO_FLAG_EOS_MASK;
    } else {
        mem_info->uFlag |= AMP_MEMINFO_FLAG_EOS_MASK;
        GST_INFO("send video eos\n");
    }

    /* pts tag */
    ret = AMP_BDTAG_GetWithType(buf_desc,AMP_BDTAG_BS_UNITSTART_CTRL, NULL, (void**)&unit_start);
    VERIFY_RESULT(ret);
    append_unit_start(unit_start, pts , amp_sink->stream_pos);

    if(length != 0) {
        if(buf_pos == LINEAR_END)
            amp_sink->wp_v += length;
        else
            amp_sink->wp_v = length;

        amp_sink->stream_pos += length;
    }

    AMP_RPC(ret,
            AMP_VDEC_PushBD,
            handle,
            AMP_PORT_INPUT,
            0,
            buf_desc);
    VERIFY_RESULT(ret);
    GST_LOG("VDEC push BD:%d pts:%x spos: %x wo:%x buf_pos:%d len:%x\n",
            AMP_BD_GET_BDID(buf_desc), unit_start->uPtsLow,
            amp_sink->stream_pos, amp_sink->wp_v,
            buf_pos, length);
    retval = FEED_DONE;

ERROR:
    if(orig_length)
        gst_buffer_unmap(buf,&info);
    return retval;
}

static GstFlowReturn
gst_amp_vdec_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    AMP_STATE state;
    HRESULT ret;

    GstWesterosSink* sink = GST_WESTEROS_SINK(parent);
    Gstampvsink *amp_sink = &sink->soc.amp_sink;

    GST_BUFFER_OFFSET_END(buf) = 0;
    GST_BUFFER_OFFSET(buf) = 0;

    AMP_RPC(ret, AMP_COMPONENT_GetState, amp_sink->hVdec, &state);
    VERIFY_RESULT(ret);
    if(state != AMP_EXECUTING && state != AMP_PAUSED) {
        GST_DEBUG("VDEC drop buf in state: %d\n",state);
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    feed_thread_push(amp_sink, buf);

    return GST_FLOW_OK;
}

typedef struct {
    UINT32 signature;
    UINT16 version;
    UINT16 hdr_len;
    UINT32 fcc;
    UINT16 width;
    UINT16 height;
    UINT32 frame_rate;
    UINT32 time_scale;
    UINT32 num_frames;
    UINT32 reserved;
    UINT32 frame_size;
    signed long long int  pts;
}__attribute__ ((packed)) VP9_IVF;

typedef struct {
    UINT32 frame_size;
    signed long long int  pts;
}__attribute__ ((packed)) VP9_FHDR;

static int process_vp9(Gstampvsink* amp_sink, GstBuffer *buf) {
    gsize length = gst_buffer_get_size(buf);
    GstClockTime pts = GST_BUFFER_PTS(buf);

    if(amp_sink->stream_pos == 0) {
        VP9_IVF *IVFheader;
        IVFheader = (VP9_IVF *)amp_sink->video_priv_data;
        amp_sink->video_priv_len = sizeof(VP9_IVF);

        IVFheader->signature  = 0x46494b44;
        IVFheader->version = 0x0;
        IVFheader->hdr_len  = 0x20;
        IVFheader->fcc   = 0x30395056;
        IVFheader->width      = 1920;
        IVFheader->height = 1080;
        IVFheader->frame_rate = 0;
        IVFheader->time_scale = 0;

        IVFheader->num_frames = 1000;
        IVFheader->reserved = 0x0;
        IVFheader->frame_size = length;
        IVFheader->pts = pts;
    } else {
        VP9_FHDR *VP9header = (VP9_FHDR *)amp_sink->video_priv_data;
        amp_sink->video_priv_len = sizeof(VP9_FHDR);
        VP9header->frame_size = length;
        VP9header->pts = pts;
    }
}

static int process_vp8(Gstampvsink* amp_sink, GstBuffer *buf) {
    gsize length = gst_buffer_get_size(buf);
    GstClockTime pts = GST_BUFFER_PTS(buf);

    if(amp_sink->stream_pos == 0) {
        VP9_IVF *IVFheader;
        IVFheader = (VP9_IVF *)amp_sink->video_priv_data;
        amp_sink->video_priv_len = sizeof(VP9_IVF);

        IVFheader->signature  = 0x46494b44;
        IVFheader->version = 0x0;
        IVFheader->hdr_len  = 0x20;
        IVFheader->fcc   = 0x30385056;
        IVFheader->width      = 1920;
        IVFheader->height = 1080;
        IVFheader->frame_rate = 0;
        IVFheader->time_scale = 0;

        IVFheader->num_frames = 1000;
        IVFheader->reserved = 0x0;
        IVFheader->frame_size = length;
        IVFheader->pts = pts;
    } else {
        VP9_FHDR *VP9header = (VP9_FHDR *)amp_sink->video_priv_data;
        amp_sink->video_priv_len = sizeof(VP9_FHDR);
        VP9header->frame_size = length;
        VP9header->pts = pts;
    }
}


static gpointer vsink_feed_task(gpointer data)
{
   Gstampvsink *amp_sink = (Gstampvsink *)data;
    while(1){
        guint num;
        g_mutex_lock(&amp_sink->feed_mutex);
        while(g_queue_get_length (amp_sink->buf_q) == 0 &&
                !amp_sink->stop){
            g_cond_wait(&amp_sink->feed_cond, &amp_sink->feed_mutex);
        }
        g_mutex_unlock(&amp_sink->feed_mutex);

        if(amp_sink->stop)
            return NULL;

        FEED_RET ret;
        g_mutex_lock(&amp_sink->feed_mutex);
        num = g_queue_get_length (amp_sink->buf_q);
        for(guint i=0; i< num; i++){
            GstBuffer *buf = (GstBuffer*)g_queue_peek_head(amp_sink->buf_q);

            if(amp_sink->video_codec == MEDIA_VES_VP9) {
                process_vp9(amp_sink, buf);
            }else if(amp_sink->video_codec == MEDIA_VES_VP8) {
                process_vp8(amp_sink, buf);
            }

            ret = amp_feed_amp_sink(amp_sink, buf);
            if(ret == FEED_RETRY){
                usleep(10000);
                break;
            }else if(ret == FEED_DONE){
                gst_buffer_unref(buf);
                g_queue_pop_head (amp_sink->buf_q);
            }else if( ret == FEED_ERR){
                gst_buffer_unref(buf);
                GST_ERROR("feed error");
                break;
            }
        }
        g_mutex_unlock(&amp_sink->feed_mutex);

        if(ret == FEED_ERR)
            break;
    }
    return NULL;
}
/**
 * Used to initialize the class only once ,specifying what signals
 * arguments and virtual functions the class has.
 * */
static void gst_ampvsink_class_init (GstWesterosSinkClass *klass)
{
    GObjectClass *gobject_class = (GObjectClass *) klass;
    GstElementClass *gstelement_class = (GstElementClass *) klass;

    gstelement_class->query = GST_DEBUG_FUNCPTR(gst_ampvsink_sink_query_function);
    gstelement_class->send_event = GST_DEBUG_FUNCPTR (gst_ampvsink_send_event);

#if 0
    gst_element_class_set_metadata (gstelement_class,
            "AmpVout",
            "Video Render",
            "AMP GST plugin for WPEWebkit",
            "http://www.synaptics.com/");
#endif
    gst_element_class_add_pad_template (gstelement_class,
            gst_static_pad_template_get (&sink_amp_sink_pad));

    g_object_class_install_property (gobject_class, PROP_TV_MODE,
            g_param_spec_enum("tv-mode","tv-mode",
                " Define the television mode",
                GST_AMP_TV_MODE, 0,
                (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_PLANE,
            g_param_spec_enum ("gdl-plane","gdl-plane",
                "define the Universal Pixel Plane used in the GDL layer",
                GST_AMP_PLANE, 0,
                (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_RECTANGLE,
            g_param_spec_string ("rectangle", "rectangle",
                "The destination rectangle, (0,0,0,0) full screen"
                " eg 0,0,0,0 ", DEFAULT_PROP_RECTANGLE,
                (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_FLUSH_REPEAT_FRAME,
            g_param_spec_boolean ("flush-repeate-frame", "flush-repate-frame",
                " Keep displaying the last frame rather than a black one whilst flushing",DEFAULT_PROP_FLUSH_REPEAT_FRAME,
                (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class,  PROP_CURRENT_PTS,
            g_param_spec_ulong ("currentpts", "currentPTS" ,"Value in seconds  (0 - 4294967295)",0 , 0,
                DEFAULT_PROP_CURRENT_PTS, (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS )));

    g_object_class_install_property (gobject_class, PROP_INTER_FRAME_DELAY,
            g_param_spec_uint ("inter-frame-delay","inter-frame-delay", "Enables fixed frame rate mode  (0 - 4294967295)",0 , 0,
                DEFAULT_PROP_INTER_FRAME_DELAY, (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_SLOW_MODE_RATE,
            g_param_spec_int ("slow-mode-rate", "slow-mode-rate",
                "slow mode rate for video sink (-2000 to 2000)", 0 , G_MAXINT,
                DEFAULT_PROP_SLOW_MODE_RATE,
                (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_CONTENT_FRAME_RATE,
            g_param_spec_uint ("contentframerate", " get content frame rate","contentframerate",
                0 ,4505, DEFAULT_PROP_COUNTER_FRAME_RATE,
                (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_STEP_FRAME,
            g_param_spec_uint ("step-frame", "step-frame", "step-frame" ,
                0 ,4505, DEFAULT_PROP_STEP_FRAME,
                (GParamFlags)(G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property (gobject_class, PROP_MUTE,
            g_param_spec_uint ("mute", "mute","mute",0 , 1, DEFAULT_PROP_MUTE,
                (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS )));

    g_object_class_install_property (gobject_class, PROP_ZOOM,
            g_param_spec_uint ("zoom","zoom","zoom",0 , 1, DEFAULT_PROP_ZOOM,
                (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS )));

    g_object_class_install_property (gobject_class, PROP_PLAY_SPEED,
            g_param_spec_float("play-speed", "play speed",
                "Play Speed to be set or current play speed",
                -G_MAXFLOAT, G_MAXFLOAT, DEFAULT_PLAY_SPEED,
                G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_VOUT,
            g_param_spec_pointer ("vout", "vout amp handle",
                "amp handle of vout instance",
                (GParamFlags)(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

}

/**
 * initialize the new element
 * instantiate pads and add them to element
 */
static void gst_ampvsink_init (GstWesterosSink * sink) {
    Gstampvsink * amp_sink = &(sink->soc.amp_sink);

    memset(amp_sink, 0, sizeof(Gstampvsink));

    amp_sink->westeros_sink = sink;
    GST_OBJECT_FLAG_SET (GST_OBJECT (sink), GST_ELEMENT_FLAG_SINK | GST_ELEMENT_FLAG_REQUIRE_CLOCK);
    gst_createpads_amp_sink(sink);
}

/*######################################################################*/

static void sbFormat(void *data, struct wl_sb *wl_sb, uint32_t format)
{
   WESTEROS_UNUSED(wl_sb);
   GstWesterosSink *sink= (GstWesterosSink*)data;
   WESTEROS_UNUSED(sink);
   printf("westeros-sink-soc: registry: sbFormat: %X\n", format);
}

static const struct wl_sb_listener sbListener = {
	sbFormat
};

void gst_westeros_sink_soc_class_init(GstWesterosSinkClass *klass)
{
    gst_ampvsink_class_init(klass);
}

gboolean gst_westeros_sink_soc_init( GstWesterosSink *sink )
{
    gst_ampvsink_init(sink);
    return true;
}

void gst_westeros_sink_soc_term( GstWesterosSink *sink )
{
    if ( sink->soc.sb ) {
        wl_sb_destroy( sink->soc.sb );
        sink->soc.sb= 0;
    }

    GST_DEBUG("gst_westeros_sink_soc_term");
}

void gst_westeros_sink_soc_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    gst_ampvsink_set_property(object, prop_id, value, pspec);
}

void gst_westeros_sink_soc_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    gst_ampvsink_get_property(object, prop_id, value, pspec);
}

void gst_westeros_sink_soc_registryHandleGlobal( GstWesterosSink *sink,
                                 struct wl_registry *registry, uint32_t id,
		                           const char *interface, uint32_t version)
{
    WESTEROS_UNUSED(version);
    int len;

    len= strlen(interface);

    if ((len==5) && (strncmp(interface, "wl_sb", len) == 0))
    {
        sink->soc.sb= (struct wl_sb*)wl_registry_bind(registry, id, &wl_sb_interface, 1);
        printf("westeros-sink-soc: registry: sb %p\n", (void*)sink->soc.sb);
        wl_proxy_set_queue((struct wl_proxy*)sink->soc.sb, sink->queue);
        wl_sb_add_listener(sink->soc.sb, &sbListener, sink);
        printf("westeros-sink-soc: registry: done add sb listener\n");
    }
}

void gst_westeros_sink_soc_registryHandleGlobalRemove( GstWesterosSink *sink,
        struct wl_registry *registry,
        uint32_t name)
{
    WESTEROS_UNUSED(sink);
    WESTEROS_UNUSED(registry);
    WESTEROS_UNUSED(name);
}

gboolean gst_westeros_sink_soc_null_to_ready( GstWesterosSink *sink, gboolean *passToDefault )
{
    GstElement *element = GST_ELEMENT(sink);
    gst_ampvsink_change_state(element, GST_STATE_CHANGE_NULL_TO_READY);
    return TRUE;
}

gboolean gst_westeros_sink_soc_ready_to_paused( GstWesterosSink *sink, gboolean *passToDefault )
{
    GstElement *element = GST_ELEMENT(sink);
    gst_ampvsink_change_state(element, GST_STATE_CHANGE_READY_TO_PAUSED);
    return TRUE;
}

gboolean gst_westeros_sink_soc_paused_to_playing( GstWesterosSink *sink, gboolean *passToDefault )
{
    GstElement *element = GST_ELEMENT(sink);
    gst_ampvsink_change_state(element, GST_STATE_CHANGE_PAUSED_TO_PLAYING);
    return TRUE;
}

gboolean gst_westeros_sink_soc_playing_to_paused( GstWesterosSink *sink, gboolean *passToDefault )
{
    LOCK( sink );
    sink->videoStarted= FALSE;
    UNLOCK( sink );

    GstElement *element = GST_ELEMENT(sink);
    gst_ampvsink_change_state(element, GST_STATE_CHANGE_PLAYING_TO_PAUSED);

    *passToDefault= false;

    return TRUE;
}

gboolean gst_westeros_sink_soc_paused_to_ready( GstWesterosSink *sink, gboolean *passToDefault )
{
    LOCK( sink );
    sink->videoStarted= FALSE;
    UNLOCK( sink );

    GstElement *element = GST_ELEMENT(sink);
    gst_ampvsink_change_state(element, GST_STATE_CHANGE_PAUSED_TO_READY);

    *passToDefault= false;

    return TRUE;
}

gboolean gst_westeros_sink_soc_ready_to_null( GstWesterosSink *sink, gboolean *passToDefault )
{
    WESTEROS_UNUSED(sink);

    GstElement *element = GST_ELEMENT(sink);
    gst_ampvsink_change_state(element, GST_STATE_CHANGE_READY_TO_NULL);

    *passToDefault= false;

    return TRUE;
}

void gst_westeros_sink_soc_query_accept_caps(GstWesterosSink *sink, GstQuery *query)
{
    GstCaps *caps;
    Gstampvsink * amp_sink = &(sink->soc.amp_sink);
    gboolean accepted = false;

    gst_query_parse_accept_caps (query, &caps);
    GST_DEBUG_OBJECT(sink, "accept caps %" GST_PTR_FORMAT, caps);
    GstStructure *caps_structure = gst_caps_get_structure(caps, 0);

    const gchar * stream_format = gst_structure_get_name(caps_structure);
    if(g_strrstr(stream_format,"video/x-h264")){
        accepted = true;
    }else if(g_strrstr(stream_format,"video/x-h265")){
        accepted = true;
    }else if(g_strrstr(stream_format,"video/x-vp9")){
        accepted = true;
    }else if(g_strrstr(stream_format,"video/x-vp8")){
        accepted = true;
    }
    gst_query_set_accept_caps_result(query, accepted);
}

void gst_westeros_sink_soc_set_startPTS( GstWesterosSink *sink, gint64 pts )
{
    HRESULT ret;
    Gstampvsink * amp_sink = &(sink->soc.amp_sink);
    AVRational new_base;
    AVRational time_base;
    AMP_PTS pts_t;

    new_base.num = 1;
    new_base.den = 90000;
    time_base.num=1;
    time_base.den=1000000000;
    if (pts != AV_NOPTS_VALUE && pts >= 0) {
        pts = av_rescale_q(pts,time_base,new_base);
    }
    if (pts == AV_NOPTS_VALUE || pts < 0) {
        pts &= ~(AMP_PTS_VALID_MASK);
    } else {
        pts |= AMP_PTS_VALID_MASK;
    }


    pts_t = pts;
    AMP_RPC(ret, AMP_CLK_SetStartPTS, amp_sink->hClk, AMP_CLK_PORT_IRRELEVANT, pts_t);
    VERIFY_RESULT(ret);
    GST_DEBUG_OBJECT(sink, "set start pts: %llx", pts_t);
}

void gst_westeros_sink_soc_render( GstWesterosSink *sink, GstBuffer *buffer )
{
    //TBD
}

void gst_westeros_sink_soc_flush( GstWesterosSink *sink )
{
}

gboolean gst_westeros_sink_soc_start_video( GstWesterosSink *sink )
{
    gboolean result= FALSE;
    return result;
}

void gst_westeros_sink_soc_eos_event( GstWesterosSink *sink )
{
    Gstampvsink * amp_sink = &(sink->soc.amp_sink);
    GstBuffer *endbuf;
    endbuf = gst_buffer_new();
    GST_BUFFER_OFFSET_END(endbuf)  = 1;
    GST_BUFFER_OFFSET(endbuf) = 0;

    //Need stream lock to sync with chain
    GST_PAD_STREAM_LOCK(amp_sink->sinkpad);
    feed_thread_push(amp_sink, endbuf);
    GST_PAD_STREAM_UNLOCK(amp_sink->sinkpad);
}

void gst_westeros_sink_soc_set_video_path( GstWesterosSink *sink, bool useGfxPath )
{
    WESTEROS_UNUSED(sink);
    WESTEROS_UNUSED(useGfxPath);
}

void gst_westeros_sink_soc_update_video_position( GstWesterosSink *sink )
{
    WESTEROS_UNUSED(sink);
}

void gst_westeros_sink_soc_load()
{
    MV_OSAL_Init();
    AMP_Initialize(0, NULL, &hFactory);
}
