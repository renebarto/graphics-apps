/*******************************************************************************/
/* Copyright 2012, MARVELL SEMICONDUCTOR, LTD. */
/* THIS CODE CONTAINS CONFIDENTIAL INFORMATION OF MARVELL. */
/* NO RIGHTS ARE GRANTED HEREIN UNDER ANY PATENT, MASK WORK RIGHT OR COPYRIGHT */
/* OF MARVELL OR ANY THIRD PARTY. MARVELL RESERVES THE RIGHT AT ITS SOLE */
/* DISCRETION TO REQUEST THAT THIS CODE BE IMMEDIATELY RETURNED TO MARVELL. */
/* THIS CODE IS PROVIDED "AS IS". MARVELL MAKES NO WARRANTIES, EXPRESSED,*/
/* IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY, COMPLETENESS OR PERFORMANCE. */
/* MARVELL COMPRISES MARVELL TECHNOLOGY GROUP LTD. (MTGL) AND ITS SUBSIDIARIES, */
/* MARVELL INTERNATIONAL LTD. (MIL), MARVELL TECHNOLOGY, INC. (MTI), MARVELL */
/* SEMICONDUCTOR, INC. (MSI), MARVELL ASIA PTE LTD. (MAPL), MARVELL JAPAN K.K. */
/* (MJKK), MARVELL ISRAEL LTD. (MSIL).*/
/*******************************************************************************/

#ifndef __MPEG_STREAM_INFO_H__
#define __MPEG_STREAM_INFO_H__

#include <gst/gst.h>
#include <config.h>


G_BEGIN_DECLS

/*MPEG-2 TS Media types*/
enum
{
	VIDEO_MPEG1                             = 1,
	VIDEO_MPEG2                             = 2,
	AUDIO_MPEG1                             = 3,
	AUDIO_MPEG2                             = 4,
	AUDIO_AAC                               = 15,
	VIDEO_H264                              = 27,
	VIDEO_MPEG_DCII                         = 128,
	AUDIO_AC3                               = 129,
	VIDEO_HEVC                              = 36,
};
typedef struct PmtStreamInfoClass {
	GObjectClass parent_class;
} PmtStreamInfoClass;

typedef struct PmtStreamInfo {
	GObject parent;

	GValueArray *languages;
	guint16 pid;
	GValueArray *descriptors;
	guint8 stream_type;
} PmtStreamInfo;

PmtStreamInfo *pmt_stream_info_new (guint16 pid, guint8 type);
void pmt_stream_info_add_language(PmtStreamInfo* si,
		gchar* language);
void pmt_stream_info_add_descriptor (PmtStreamInfo *pmt_info,
		const gchar *descriptor, guint length);

GType pmt_stream_info_get_type (void);

#define TYPE_PMT_STREAM_INFO (pmt_stream_info_get_type ())

#define IS_PMT_STREAM_INFO(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_PMT_STREAM_INFO))
#define PMT_STREAM_INFO(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),TYPE_PMT_STREAM_INFO, PmtStreamInfo))

G_END_DECLS

#endif

