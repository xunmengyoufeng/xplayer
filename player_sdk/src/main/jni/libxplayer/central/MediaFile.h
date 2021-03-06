//
// Created by chris on 10/17/16.
// Media File Object
// The inner object , all according to media information can get from here.
//

#ifndef XPLAYER_MEDIAFILE_H
#define XPLAYER_MEDIAFILE_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

} // end of extern C

#include "util/cm_std.h"

#include "MediaFile.h"
#include "PacketQueue.h"
#include "FrameQueue.h"
#include "MediaPlayerListener.h"
#include "util/XMessageQueue.h"

#define x_min(a,b) (((a) < (b)) ? (a):(b))
#define x_max(a,b) (((a) > (b)) ? (a):(b))
#define x_abs(a) (((a) >= 0) ? (a) : -(a))


#define X_S_2_MS(sec) ((sec) * 1000)
#define X_MS_2_US(msec) ((msec) * 1000)

#define X_MAX_PKT_Q_NETWORK_FIRST_BUFFERING_TS X_S_2_MS(0.1)        // 0.5 second in ms unit
#define X_MAX_PKT_Q_NETWORK_BUFFERING_TS X_S_2_MS(2)              // in ms unit

#define X_MAX_PKT_Q_TS         X_S_2_MS(0.6)                  // in ms unit
#define X_MAX_PKT_Q_NETWORK_TS X_S_2_MS(40)                   // in ms unit

#define X_MAX_FRAME_VIDEO_Q_NODE_CNT 25                         // max frm q video frame count
#define X_MIN_FRAME_VIDEO_Q_NODE_CNT 10                         // min frm q video frame count

#define X_MAX_FRAME_Q_TIME_LIMIT (0.7)                          // s

#define X_MAX_FRAME_AUDIO_Q_NODE_CNT 100                         // max frm q audio frame count

#define X_MEGA_SIZE(t) ((t) * 1024 * 1024)
#define X_MAX_FRAME_VIDEO_Q_MEM_SPACE  X_MEGA_SIZE(40)              // 40M space size

//#define X_MAX_PKT_VIDEO_Q_MEM_SPACE  X_MEGA_SIZE(10)                // 10M space size
//#define X_MAX_PKT_AUDIO_Q_MEM_SPACE  X_MEGA_SIZE(5)                 // 5M space size

#define X_MAX_PKT_VIDEO_Q_MEM_SPACE  X_MEGA_SIZE(4)                // 4M space size
#define X_MAX_PKT_AUDIO_Q_MEM_SPACE  X_MEGA_SIZE(2)                 // 2M space size


/**
 * streams type in file media.
 *
 */
typedef enum av_support_type_e_
{
    HAS_NONE = 0,
    HAS_AUDIO,      // has audio
    HAS_VIDEO,    // has video
    HAS_BOTH,     // has all
    HAS_END
} av_support_type_e;

/**
 * Listen Event Type
 *
 */
typedef enum listen_event_type_e_
{
    MEDIA_NOP               = 0, // interface test message
    MEDIA_PREPARED          = 1,        // Prepared
    MEDIA_PLAYBACK_COMPLETE = 2,        // PLAYBACK_COMPLETED

    MEDIA_BUFFERING_UPDATE  = 3,        // BUFFERING

    MEDIA_SEEK_COMPLETE     = 4,
    MEDIA_SET_VIDEO_SIZE    = 5,

    MEDIA_END_OF_FILE       = 6,        // END_OF_FILE


    MEDIA_ERROR             = 100,      // FAILED
    MEDIA_INFO              = 200,

    MEDIA_SET_VIDEO_SAR     = 10001,    // arg1 = sar.num, arg2 = sar.den
} listen_event_type_e;



class MediaFile
{

public:

    MediaFile();
    ~MediaFile();


    int setListener(MediaPlayerListener* listener);

    void notify(int msg, int ext1, int ext2);

    void startRender();

    void stopRender();

    void jNI2BufferState();

    /**
     * create audio mix object
     */
    void createAudioMixObj();

    /**
     * set source url
     */
    void setSourceUrl(const char *source_url);

    /**
     * get source url
     */
    char * getSourceUrl();

    /**
    * OPEN input file
    *
    * if open file success ,and get media info then return CM_TRUE.
    */
    CM_BOOL open();

    /**
    * judge if the packet queue is full by the input parameter ts.
    */
    CM_BOOL is_pkt_q_full(int64_t buffer_ts_duration);

    /**
    * close file stream ,and free resource .
    */
    void close_file();

    void setPausedState(bool is_paused);


private:
    /**
    * OPEN given stream by stream_index
    * refer to function from ffplay.c
    */
    CM_BOOL stream_component_open(int stream_index);

    /**
    * Close given stream by stream_index
    * refer to function from ffplay.c
    */
    void stream_component_close(int stream_index);



private:
    //-----------*******************-------------
    //          private member variable
    //-----------*******************-------------

    /**
     * source url
     */
    char *source_url;



public:
    bool end_of_file;

    bool stop_flag;

    bool file_opened;

    bool isBuffering;
    bool isPaused;
    /**
     * format context for input file
     */
    AVFormatContext *format_context;

    /**
     * audio stream
     */
    AVStream *audio_stream;

    /**
     * video stream
     */
    AVStream *video_stream;

    /**
     * audio codec context
     */
    AVCodecContext  *audio_codec_context ;

    /**
     * video codec context
     */
    AVCodecContext  *video_codec_context ;


    /**
     * duration
     */
    int stream_index[AVMEDIA_TYPE_NB];

    /**
     * audio channels
     */
    int audio_channels_cnt;

    /**
     * media file contain streams type
     */
    av_support_type_e av_support;


    /**
     * audio packet queue
     */
    PacketQueue *audio_queue;

    /**
    * video packet queue
    */
    PacketQueue *video_queue;

    /**
     * audio frame queue
     */
    FrameQueue *audio_frame_queue;

    /**
    * video frame queue
    */
    FrameQueue *video_frame_queue;

    /**
    * buffering percent.
    */
    int buffering_percent;

    MediaPlayerListener*   mListener; //mediaplayer listener for java

    /**
    * first play ,only need to buffer start_playing_buffering_time data.
    */
    int64_t start_playing_buffering_time;

    /**
    * playing_buffering_time data during play.
    */
    int64_t playing_buffering_time;

    /**
    * max buffering time.
    */
    int64_t max_buffering_time;

    /**
    * audio sync clock time (in millisecond)
    */
    int64_t   sync_audio_clock_time;

    /**
     * seek position (in millisecond)
     */
    int64_t     seekpos;

    /**
     * the first video packet in the video stream
     * some video the first packet pts is not start from 0.
     *  (in millisecond)
     */
    int64_t                 beginning_video_pts;

    /**
     * the first audio packet in the video stream
     */
    int64_t                 beginning_audio_pts;

    /**
     * duration in millisecond
     */
    long duration_ms;

    /**
     * current position in millisecond
     */
    long current_position_ms;

    /**
     * seeking mark
     */
    bool seeking_mark;

    /**
     * Whether the player is started up ,and display some frames before.
     */
    bool isPlayedBefore;

    /**
     * message queue for CentralEngineStateMachine
     */
    XMessageQueue *message_queue_central_engine;

    /**
     * message queue for MediaDecodeVideoStateMachine
     */
    XMessageQueue *message_queue_video_decode;

    /**
     * message queue for MediaDecodeAudioStateMachine
     */
    XMessageQueue *message_queue_audio_decode;


};

#endif //XPLAYER_MEDIAFILE_H
