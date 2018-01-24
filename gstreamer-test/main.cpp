#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gst/gst.h>

static GIOChannel *io_stdin;
static GMainLoop *loop;
static GstElement *playbin = NULL;
static GstElement *video_sink = NULL;

#define DEBUG
static gchar *prev_pipeline_str = NULL;
static gchar* print_pipeline_elements (GstBin *bin);

/* Process messages from GStreamer */
static gboolean handle_bus_message (GstBus *bus, GstMessage *msg, void *data)
{
    GError *err;

    switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error (msg, &err, NULL);
            g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
            g_clear_error (&err);
            g_main_loop_quit (loop);
            break;
        case GST_MESSAGE_EOS:
            g_print ("End-Of-Stream reached.\n");
            g_main_loop_quit (loop);
            break;
        case GST_MESSAGE_STATE_CHANGED: {
            GstState old_state, new_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, NULL);
            break;
        }
        default:
            //g_print ("Unhandled bus message %d received.\n", GST_MESSAGE_TYPE (msg));
            break;
    }

#ifdef DEBUG
    gchar* curr_pipeline_str = print_pipeline_elements (GST_BIN(playbin));
    if (!prev_pipeline_str || g_strcasecmp (prev_pipeline_str, curr_pipeline_str) != 0) {
        g_print ("current pipeline: %s\n", curr_pipeline_str);
        prev_pipeline_str = g_strdup (curr_pipeline_str);
    }
    g_free (curr_pipeline_str);
#endif
    return TRUE;
}

static void handle_source_setup(void *data)
{
    g_print ("source_setup event processing...\n");
}

#ifdef DEBUG
static gchar* print_pipeline_elements (GstBin *bin)
{
    GValue node = G_VALUE_INIT;
    GstElement *elem;
    gchar *pipeflow = g_strdup_printf("");

    GstIterator *iter = gst_bin_iterate_elements (GST_BIN(bin));
    while (gst_iterator_next(iter, &node) == GST_ITERATOR_OK) {
        elem = (GstElement *)g_value_get_object (&node);

        if (GST_IS_BIN (elem)) {
            gchar *bin_name;
            if (gst_element_get_factory (GST_ELEMENT(elem)))
                bin_name = gst_object_get_name(GST_OBJECT(gst_element_get_factory (GST_ELEMENT(elem))));
            else
                bin_name = gst_element_get_name (GST_ELEMENT(elem));

            pipeflow = g_strconcat (pipeflow, g_strdup_printf("([%s]", bin_name),
                                    print_pipeline_elements (GST_BIN(elem)), ")", NULL);
            g_free (bin_name);
        } else {
            pipeflow = g_strconcat (pipeflow,
                                    g_strdup_printf("%s -> ", gst_object_get_name(GST_OBJECT(gst_element_get_factory(GST_ELEMENT(elem))))), NULL);
        }
        g_value_reset (&node);
    }
    gst_iterator_free (iter);

    return pipeflow;
}
#endif

static void remove_newline_lf(char *line)
{
    int len = 0;

    if(NULL == line)
        return;

    len = strlen(line) -1;
    if ((len >= 0) && (line[len] == '\n')) {
        line[len] = '\0';
    }
}

static gboolean keypress (GIOChannel *src, GIOCondition cond, GstElement *unused){
    gdouble vol;
    char strbuf[100];
    char c;

    fgets(strbuf, sizeof(strbuf), stdin);
    c = strbuf[0];

    switch(c){
        case '?':
        {
            g_print("**********************************\n");
            g_print("? : display help menu\n");
            g_print("+ : volume up\n");
            g_print("- : volume down\n");
            g_print("m : volume mute/unumte\n");
            g_print("f : display time/duration\n");
            g_print("g : seek to time\n");
            g_print("**********************************\n");
            break;
        }
        case '+':
        {
            g_object_get(playbin, "volume", &vol,NULL);
            if (vol >= 0.95) break;
            g_object_set (playbin, "volume", vol + 0.05, NULL);
            g_print("\nvolume: %f", vol);
            break;
        }
        case '-':
        {
            g_object_get(playbin, "volume", &vol, NULL);
            if (vol <= 0.05) break;
            g_object_set (playbin, "volume", vol - 0.05, NULL);
            g_print("\nvolume: %f", vol-0.05);
            break;
        }
        case 'm':
        {
            gboolean mute;
            g_object_get(playbin, "mute", &mute, NULL);
            mute = !mute;
            g_object_set(playbin, "mute", mute, NULL);
            g_print("\nmute: %d", mute);
            break;
        }
        case 'f':
        {
            gint64 pos, len;

            if (gst_element_query_position (video_sink, GST_FORMAT_TIME, &pos)
                && gst_element_query_duration (playbin, GST_FORMAT_TIME, &len)) {
                g_print ("\nTime: %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
                    GST_TIME_ARGS (pos), GST_TIME_ARGS (len));
            }
            break;
        }
        case 'g':
        {
            char seek_to[64] = {0};
            gint64 time;
            printf("seek to (s): ");
            fgets(seek_to, sizeof(seek_to), stdin);
            remove_newline_lf(seek_to);
            time = atoll(seek_to);
            time *= 1000000000;

            if (!gst_element_seek (playbin, 1.0, GST_FORMAT_TIME,
                                   GstSeekFlags(GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_KEY_UNIT),
                                   GST_SEEK_TYPE_SET, time,
                                   GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
                g_print ("Seek failed!\n");
            }

            break;
        }
        default:
            break;
    }
    return TRUE;
}

int main (int argc, char *argv[])
{
    GstBus *bus;
    gchar *uri;
    GstClock * clk = NULL;
    GstElement *audio_sink = NULL;

#if defined(SYNAPTICS_PLATFORM)
    g_print ("TESTING amp_webkit_plugins...\n");
#endif

    if (argc != 2) {
        g_printerr ("Usage: %s uri \n", argv[0]);
        return -1;
    }

    /* Initialize GStreamer */
    gst_init (NULL, NULL);

    /* Create the element */
    playbin = gst_element_factory_make ("playbin", "playbin");

    if (!playbin) {
        g_printerr ("Fail to create playbin.\n");
        return -1;
    }
    /* native audio/video */
    g_object_set (G_OBJECT(playbin), "flags", 0x00000020|0x00000040, NULL);

    /* Check source format */
    if (strstr (argv[1], "://"))
        uri = g_strdup_printf("%s", argv[1]);
    else
        uri = g_strdup_printf("file:///%s", argv[1]);

    /* Set the URI to play */
    g_object_set (G_OBJECT(playbin), "uri", uri, NULL);
    g_free (uri);

    /* Add a bus watch, so we get notified when a message arrives */
    bus = gst_pipeline_get_bus (GST_PIPELINE(playbin));
    gst_bus_add_signal_watch (bus);
    g_signal_connect (bus, "message", G_CALLBACK(handle_bus_message), NULL);

    g_signal_connect_swapped (playbin, "source-setup", G_CALLBACK(handle_source_setup), NULL);
    //g_signal_connect_swapped (playbin, "video-changed", G_CALLBACK(...), NULL);
    //g_signal_connect_swapped (playbin, "audio-changed", G_CALLBACK(...), NULL);

#if defined(SYNAPTICS_PLATFORM)
    /* Set video-sink */
    video_sink = gst_element_factory_make ("westerossink", "westeros_video_sink");
    g_object_set (G_OBJECT(playbin), "video-sink", video_sink, NULL);

    /* Set audio-sink */
    audio_sink = gst_element_factory_make ("amparen", "amp_audio_sink");
    g_object_set (G_OBJECT(playbin), "audio-sink", audio_sink, NULL);
#endif
    /* Set video-sink */
    video_sink = gst_element_factory_make ("westerossink", "vsink");
    g_object_set (G_OBJECT(playbin), "video-sink", video_sink, NULL);

    /* Start playing */
    if (gst_element_set_state (GST_ELEMENT(playbin), GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        g_printerr ("Unable to set the pipeline to playing.\n");
        goto exit;
    }

#ifdef DEBUG
    g_print ("Current playbin pipeline flow: %s (END)\n", print_pipeline_elements (GST_BIN(playbin)));
#endif

    io_stdin = g_io_channel_unix_new (fileno (stdin));
    g_io_add_watch (io_stdin, G_IO_IN, (GIOFunc) keypress, NULL);

    /* Create a GLib Main Loop and set it to run */
    loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (loop);

    /* Free resources */
    gst_element_set_state (GST_ELEMENT(playbin), GST_STATE_NULL);

    g_main_loop_unref (loop);
    gst_object_unref(clk);
    exit:
    gst_object_unref (bus);
    gst_object_unref (playbin);

    if (prev_pipeline_str)
        g_free (prev_pipeline_str);

    return 0;
}
