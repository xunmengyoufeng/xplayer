//
// Created by chris on 9/26/16.
// native source code entry file.
//
#include <stdlib.h>
#include <unistd.h>

#include <jni.h>
#include "util/XLog.h"
#include "central/PlayerInner.h"
#include "central/YuvGLRender.h"
#include "central/xplayer_android_def.h"

static const char *TAG = "JNI_ONLOAD";

// define the target class name .
static const char *className = "com/cmcm/v/player_sdk/player/XPlayer";

struct fields_t {
    jfieldID    context;
    jmethodID   post_event;

    jmethodID   stopRenderMode;
    jmethodID   startRenderMode;

};
fields_t fields;

static JavaVM *sVm;
//TODO
static PlayerInner * playerInner;
static YuvGLRender *yuvGLRender;

/*
 * Throw an exception with the specified class and an optional message.
 */
int jniThrowException(JNIEnv* env, const char* className, const char* msg) {
    jclass exceptionClass = env->FindClass(className);
    if (exceptionClass == NULL) {
        XLog::d(ANDROID_LOG_ERROR,
            TAG,
            "Unable to find exception class %s",
                    className);
        return -1;
    }

    if (env->ThrowNew(exceptionClass, msg) != JNI_OK) {
        XLog::d(ANDROID_LOG_ERROR,
            TAG,
            "Failed throwing '%s' '%s'",
            className, msg);
    }
    return 0;
}

JavaVM* getJvm()
{
    return sVm;
}

JNIEnv* getJNIEnv() {
    JNIEnv* env = NULL;
    bool isAttached = false;
    int status = 0;

    if (sVm->GetEnv((void**) &env, JNI_VERSION_1_4) < 0) {
        return NULL;
    }
    return env;
}

// ----------------------------------------------------------------------------
// ref-counted object for callbacks
class JNIMediaPlayerListener: public MediaPlayerListener
{
public:
    JNIMediaPlayerListener(JNIEnv* env, jobject thiz, jobject weak_thiz);
    ~JNIMediaPlayerListener();
    void notify(int msg, int ext1, int ext2);
    void JNIStopGlRenderMode();
    void JNIStartGlRenderMode();
    void JNI2BufferState();
    void JNICreateAudioMixObj();

private:
    JNIMediaPlayerListener();
    jclass      mClass;     // Reference to MediaPlayer class
    jobject     mObject;    // Weak ref to MediaPlayer Java object to call on
};


JNIMediaPlayerListener::JNIMediaPlayerListener(JNIEnv* env, jobject thiz, jobject weak_thiz)
{

    // Hold onto the MediaPlayer class for use in calling the static method
    // that posts events to the application thread.

    jclass clazz = env->GetObjectClass(thiz);
    if (clazz == NULL) {
        XLog::e(TAG ,"Can't find android/media/MediaPlayer");
        jniThrowException(env, "java/lang/Exception", NULL);
        return;
    }
    mClass = (jclass)env->NewGlobalRef(clazz);

    // We use a weak reference so the MediaPlayer object can be garbage collected.
    // The reference is only used as a proxy for callbacks.
    mObject  = env->NewGlobalRef(weak_thiz);
}

JNIMediaPlayerListener::~JNIMediaPlayerListener()
{
    // remove global references
    JNIEnv *env = getJNIEnv();
    if(env){
        env->DeleteGlobalRef(mObject);
        env->DeleteGlobalRef(mClass);
    }
}

void JNIMediaPlayerListener::notify(int msg, int ext1, int ext2)
{
    JNIEnv *env = getJNIEnv();
    JavaVM *svm = getJvm();
    bool isAttached = false;
    if(env == NULL)
    {
        svm->AttachCurrentThread(&env, NULL);
        isAttached = true;
    }
    env->CallStaticVoidMethod(mClass, fields.post_event, mObject, msg, ext1, ext2, 0);

    if(isAttached)
    {
        svm->DetachCurrentThread();
    }
}

void JNIMediaPlayerListener::JNIStopGlRenderMode()
{
    JNIEnv *env = getJNIEnv();
    JavaVM *svm = getJvm();
    bool isAttached = false;
    if(env == NULL)
    {
        svm->AttachCurrentThread(&env, NULL);
        isAttached = true;
    }
    env->CallStaticVoidMethod(mClass, fields.stopRenderMode, mObject);

    if(isAttached)
    {
        svm->DetachCurrentThread();
    }

    if(playerInner->audioRender){
        playerInner->audioRender->pause();
    }

    XLog::e(TAG ,"===>stopRender ,OPENSL ES state :isPlaying :%d\n" ,   playerInner->audioRender->isPlaying());
    XLog::e(TAG ,"===>stopRender ,OPENSL ES state :isPausing :%d\n" , playerInner->audioRender->isPausing());
}

void JNIMediaPlayerListener::JNIStartGlRenderMode()
{
    JNIEnv *env = getJNIEnv();
    JavaVM *svm = getJvm();
    bool isAttached = false;
    if(env == NULL)
    {
        svm->AttachCurrentThread(&env, NULL);
        isAttached = true;
    }
    env->CallStaticVoidMethod(mClass, fields.startRenderMode, mObject);

    if(isAttached)
    {
        svm->DetachCurrentThread();
    }

    if(playerInner->audioRender){
        playerInner->audioRender->resume();
    }

    XLog::e(TAG ,"===>startRender ,OPENSL ES state :isPlaying :%d\n" ,   playerInner->audioRender->isPlaying());
    XLog::e(TAG ,"===>startRender ,OPENSL ES state :isPausing :%d\n" , playerInner->audioRender->isPausing());

}

void JNIMediaPlayerListener::JNI2BufferState()
{
    XLog::e(TAG ,"======>in JNI2BufferState  ..");
    playerInner->centralEngineStateMachineHandle->state_machine_change_state(STATE_BUFFERING);
    playerInner->mediaFileHandle->message_queue_video_decode->push(EVT_PAUSE);
    playerInner->mediaFileHandle->message_queue_audio_decode->push(EVT_PAUSE);
    playerInner->mediaFileHandle->message_queue_central_engine->push(EVT_GO_ON);    // push to loop ,must to perform after push msg evt to decode machine.

}

void JNIMediaPlayerListener::JNICreateAudioMixObj()
{
    XLog::e(TAG ,"======>in JNICreateAudioMixObj  ..");
    if(playerInner->audioRender){
        playerInner->audioRender->InitPlayout();
    }

}


// ----------------------------------------------------------------------------

// be called when library be loaded.
static void
jni_native_init(JNIEnv *env , jobject thiz)
{
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find android/media/MediaPlayer");
        return;
    }

    fields.post_event = env->GetStaticMethodID(clazz, "postEventFromNative",
                                                   "(Ljava/lang/Object;IIILjava/lang/Object;)V");
    if (fields.post_event == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find FFMpegMediaPlayer.postEventFromNative");
        return;
    }

    fields.stopRenderMode= env->GetStaticMethodID(clazz, "stopRenderMode","()V");
    if (fields.stopRenderMode == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find FFMpegMediaPlayer.stopRenderMode");
        return;
    }


    fields.startRenderMode= env->GetStaticMethodID(clazz, "startRenderMode","()V");
    if (fields.startRenderMode == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Can't find FFMpegMediaPlayer.startRenderMode");
        return;
    }



}
//
static void jni_native_setup(JNIEnv *env, jobject thiz ,jobject weak_this)
{
    XLog::e(TAG ,"======>native_setup .");

    #if 0
    if(mbPlayer)
    {
         Log::d(ANDROID_LOG_INFO, TAG, "=core=[%s,%s:%d] Player Already exists!",
        __FILE__, __FUNCTION__, __LINE__);
        return -2;
    }
    #endif

    playerInner = new PlayerInner();
    if (playerInner == NULL) {
        jniThrowException(env, "java/lang/RuntimeException", "Out of memory");
        return;
    }

     // create new listener and give it to MediaPlayer
    JNIMediaPlayerListener *listener = new JNIMediaPlayerListener(env, thiz, weak_this);
    playerInner->mediaFileHandle->setListener(listener);

    // Stow our new C++ MediaPlayer in an opaque field in the Java object.
    //setMediaPlayer(env, thiz, playerInner);

}



static void native_init1(JNIEnv *env, jobject thiz)
{
    XLog::e(TAG ,"======>native_init .");
    playerInner->player_engine_init();  // engine init
}

static void native_initEGLCtx(JNIEnv *env, jobject thiz)
{
    XLog::e(TAG ,"======>native_initEGLCtx .");
    yuvGLRender = new YuvGLRender();
    yuvGLRender->init();  // engine init
}

static void native_delEGLCtx(JNIEnv *env, jobject thiz)
{
    XLog::e(TAG ,"======>native_delEGLCtx 1.");
    if(yuvGLRender){
        delete yuvGLRender;
        yuvGLRender = NULL;
    }
}


static void native_setDataSource(JNIEnv *env, jobject thiz ,jstring dataSource)
{
    const char *c_data_source = env->GetStringUTFChars(dataSource, NULL );

    //
    playerInner->set_data_source(c_data_source);

    XLog::e(TAG ,"======>set_data_source,source_url=%s." ,c_data_source);
    //env->ReleaseStringUTFChars(dataSource, c_data_source);    // not release TODO

}

static void native_prepareAsync(JNIEnv *env, jobject thiz)
{
    playerInner->player_engine_prepare();  // open file ,get stream info
}

static void native_start(JNIEnv *env, jobject thiz)
{
    playerInner->mediaFileHandle->message_queue_central_engine->push(EVT_START);   //TODO here should be performed in upper layer
    playerInner->mediaFileHandle->setPausedState(false);    // upper layer will call start before seek.
    XLog::e(TAG, "====>in native_start paused  set false");
    usleep(50* 1000);   // for seek
}

static void native_play(JNIEnv *env, jobject thiz)
{
    // start audio decoder & video decoder thread.
    playerInner->mediaFileHandle->message_queue_video_decode->push(EVT_START);
    playerInner->mediaFileHandle->message_queue_audio_decode->push(EVT_START);
    playerInner->mediaFileHandle->message_queue_central_engine->push(EVT_PLAY);

    playerInner->mediaFileHandle->startRender();
    XLog::e(TAG ,"======>playerInner->player_start.SimpleBufferQueueCallback");

}

static void native_pause(JNIEnv *env, jobject thiz)
{
    playerInner->mediaFileHandle->stopRender();
    playerInner->mediaFileHandle->setPausedState(true);
    XLog::e(TAG, "====>in native_start paused  set true");

    if(playerInner->mediaFileHandle->isBuffering){

        XLog::e(TAG ,"======>call native_pause, and in buffering state.");
        //
        playerInner->centralEngineStateMachineHandle->state_machine_change_state(STATE_BUFFERING);
        playerInner->mediaFileHandle->message_queue_video_decode->push(EVT_PAUSE);
        playerInner->mediaFileHandle->message_queue_audio_decode->push(EVT_PAUSE);
        return ;
    }
    // process state
    playerInner->mediaFileHandle->message_queue_video_decode->push(EVT_PAUSE);
    playerInner->mediaFileHandle->message_queue_audio_decode->push(EVT_PAUSE);
}

static void native_resume(JNIEnv *env, jobject thiz)
{

    playerInner->mediaFileHandle->setPausedState(false);
    XLog::e(TAG, "====>in native_start paused  set false");

    if(playerInner->centralEngineStateMachineHandle->state == STATE_BUFFERING){
        // notify [fix for buffering then click pause then immediate click start ,but loading is disappear ]
        playerInner->mediaFileHandle->notify(MEDIA_INFO ,MEDIA_INFO_BUFFERING_START ,0);
        XLog::e(TAG ,"======>call native_resume,but in buffering state ,notify buffering start and return.");
        return;
    }

    // process state
    playerInner->mediaFileHandle->message_queue_video_decode->push(EVT_RESUME);
    playerInner->mediaFileHandle->message_queue_audio_decode->push(EVT_RESUME);
    playerInner->mediaFileHandle->message_queue_central_engine->push(EVT_RESUME);
    playerInner->mediaFileHandle->startRender();
}

static void native_stop(JNIEnv *env, jobject thiz)
{
    XLog::e(TAG ,"======>in native_stop 1.");
    if(playerInner->audioRender->isInitialized){
        XLog::e(TAG ,"======>in native_stop 1.1.");
        // audio opensl es
        playerInner->audioRender->stop();
        XLog::e(TAG ,"======>in native_stop 1.2.");
    }
    XLog::e(TAG ,"======>in native_stop 2.");

    // set
    playerInner->mediaFileHandle->stop_flag = true;
    XLog::e(TAG ,"======>in native_stop 2.1");

    // process state
    playerInner->mediaFileHandle->message_queue_video_decode->push_front(EVT_STOP);
    playerInner->mediaFileHandle->message_queue_audio_decode->push_front(EVT_STOP);
    playerInner->mediaFileHandle->message_queue_central_engine->push_front(EVT_STOP);

    // clear packet&frame q
    playerInner->mediaFileHandle->audio_queue->flush();
    playerInner->mediaFileHandle->video_queue->flush();
    playerInner->mediaFileHandle->audio_frame_queue->flush();
    playerInner->mediaFileHandle->video_frame_queue->flush();
    XLog::e(TAG ,"======>in native_stop 3.");

}

static void native_release(JNIEnv *env, jobject thiz)
{
    // 关闭文件之前，分别向几个队列发CLOSE消息。
    // 这几个队列现在不处理CLOSE消息了，直接退出线程

    playerInner->mediaFileHandle->message_queue_video_decode->push_front(EVT_CLOSE);
    playerInner->mediaFileHandle->message_queue_audio_decode->push_front(EVT_CLOSE);
    playerInner->mediaFileHandle->message_queue_central_engine->push_front(EVT_CLOSE);


    playerInner->mediaFileHandle->message_queue_video_decode->push_front(EVT_EXIT_THREAD);
    playerInner->mediaFileHandle->message_queue_audio_decode->push_front(EVT_EXIT_THREAD);
    playerInner->mediaFileHandle->message_queue_central_engine->push_front(EVT_EXIT_THREAD);

    XLog::e(TAG ,"======>in native_release 1.");
    pthread_join(playerInner->media_demux_tid, NULL);
    XLog::e(TAG ,"======>in native_release 2.");
    pthread_join(playerInner->decode_video_tid, NULL);
    XLog::e(TAG ,"======>in native_release 3.");
    pthread_join(playerInner->decode_audio_tid, NULL);
    XLog::e(TAG ,"======>in native_release 4.");

    playerInner->mediaFileHandle->message_queue_video_decode->flush();
    XLog::e(TAG ,"======>in native_release 5.");
    playerInner->mediaFileHandle->message_queue_audio_decode->flush();
    XLog::e(TAG ,"======>in native_release 6.");
    playerInner->mediaFileHandle->message_queue_central_engine->flush();
    XLog::e(TAG ,"======>in native_release 7.");
    // TODO
    // Release ffmpeg resources
    playerInner->mediaFileHandle->close_file();
    XLog::e(TAG ,"======>in native_release 8.");
    if(playerInner){
        delete playerInner;
        playerInner = NULL;
    }
    XLog::e(TAG ,"======>in native_release 12.");
}


static void native_renderFrame(JNIEnv *env, jobject thiz)
{

    // todo
    AVFrame *src_frame;
    src_frame = av_frame_alloc();
    //XLog::d(ANDROID_LOG_WARN ,TAG ,"==>in render_frame thread .");
    //this->mediaFileHandle->video_frame_queue->get(src_frame);
    int ret1 = playerInner->mediaFileHandle->video_frame_queue->get(src_frame ,0); // no block mode
    if(ret1 != 1){
        XLog::d(ANDROID_LOG_WARN ,TAG ,"==>in render_frame thread .no video frame ,return ,video packet size = %d ,video packet q mem size =%d" ,playerInner->mediaFileHandle->video_queue->size() ,playerInner->mediaFileHandle->video_queue->q_size);
        XLog::d(ANDROID_LOG_WARN ,TAG ,"==>in render_frame thread .no video frame ,return ,audio packet size = %d ,audio packet q mem size =%d" ,playerInner->mediaFileHandle->audio_queue->size() ,playerInner->mediaFileHandle->audio_queue->q_size);
        //mediaFileHandle->stopRender();
        usleep(50 * 1000); //in microseconds
        av_frame_free(&src_frame);  // free frame memory
        return;
    }

    // For synchronization start
    int64_t video_frame_render_pts = src_frame->pts * av_q2d(playerInner->mediaFileHandle->video_stream->time_base) * 1000;   // in millisecond
    int64_t sync_audio_clock_time = playerInner->mediaFileHandle->sync_audio_clock_time;
    int64_t diff_time = video_frame_render_pts - sync_audio_clock_time;
    int64_t sync_sleep_time = 0;

    // TODO filter some error diff_time.
    if( (diff_time > 0) && (diff_time < 800) ){ // 100 millisecond
        sync_sleep_time = diff_time * 1000;

    }else if( (diff_time >= -500) && (diff_time <= 0) ){

        sync_sleep_time = 500;
    }else if(diff_time  < -500){
        XLog::d(ANDROID_LOG_WARN ,TAG ,"==>sync_video_clock_time=%lld ,sync_audio_clock_time =%lld ,diff_time =%lld ,"
                                                                    "video_frame_q size =%d ,audio_frame_q size =%d\n",
                                                                     video_frame_render_pts ,sync_audio_clock_time,diff_time ,
                                                                     playerInner->mediaFileHandle->video_frame_queue->size() ,playerInner->mediaFileHandle->audio_frame_queue->size());

        sync_sleep_time = 1000;    /// video frame need to catch the audio frame clock.
    }else{
        XLog::d(ANDROID_LOG_WARN ,TAG ,"==>sync_video_clock_time=%lld ,sync_audio_clock_time =%lld ,diff_time =%lld ,"
                                                                    "video_frame_q size =%d ,audio_frame_q size =%d\n",
                                                                     video_frame_render_pts ,sync_audio_clock_time,diff_time ,
                                                                     playerInner->mediaFileHandle->video_frame_queue->size() ,playerInner->mediaFileHandle->audio_frame_queue->size());

        sync_sleep_time = 100 * 1000;
    }
    usleep(sync_sleep_time); //in microseconds
    // For synchronization end

    yuvGLRender->render_frame(src_frame);

    if(src_frame != NULL){
        av_frame_free(&src_frame);  // free frame memory
    }


}

static jboolean native_isPlaying(JNIEnv *env, jobject thiz)
{

    jboolean retval = JNI_FALSE;
    retval = playerInner->isPlaying();
    return retval;

}

static jlong native_getCurrentPosition(JNIEnv *env, jobject thiz)
{
     jlong retval = 0;
     retval = playerInner->getCurrentPosition();
     return retval;

}

static jlong native_getDuration(JNIEnv *env, jobject thiz)
{
     jlong retval = 0;
     retval = playerInner->getDuration();
     return retval;
}

static void native_seekTo(JNIEnv *env, jobject thiz ,jlong seekedPositionMsec )
{
    long seek_time = seekedPositionMsec;
    // set current_position_ms
    // playerInner->mediaFileHandle->current_position_ms = seek_time;
    //XLog::e(TAG ,"======>native_seekTo :%ld ,playerInner->mediaFileHandle->current_position_ms :%ld" ,seek_time ,playerInner->mediaFileHandle->current_position_ms);
    playerInner->seekTo(seek_time);

}




// define the native method mapping .
static JNINativeMethod methods[] =
{

    {"_native_init",     "()V",                                      (void*)jni_native_init},
    {"_native_setup",    "(Ljava/lang/Object;)V",                    (void*)jni_native_setup},

    {"_init",            "()V",                                      (void*)native_init1},
    {"_initEGLCtx",      "()V",                                      (void*)native_initEGLCtx},
    {"_delEGLCtx",      "()V",                                      (void*)native_delEGLCtx},


    {"_setDataSource",   "(Ljava/lang/String;)V",                    (void*)native_setDataSource},
    {"_prepareAsync",    "()V",                                      (void*)native_prepareAsync},

    {"_start",           "()V",                                      (void*)native_start},
    {"_play",            "()V",                                      (void*)native_play},
    {"_pause",           "()V",                                      (void*)native_pause},
    {"_resume",          "()V",                                      (void*)native_resume},
    {"_stop",            "()V",                                      (void*)native_stop},
    {"_release",         "()V",                                      (void*)native_release},


    {"_isPlaying",                       "()Z",                                      (void*)native_isPlaying},
    {"_getCurrentPosition",              "()J",                                      (void*)native_getCurrentPosition},
    {"_getDuration",                     "()J",                                      (void*)native_getDuration},

    {"_seekTo",                          "(J)V",                                     (void*)native_seekTo},

    {"_renderFrame",     "()V",                                      (void*)native_renderFrame},  // render frame.

};


JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved)
{

    jint result = JNI_ERR;
    JNIEnv* env = NULL;
    jclass clazz;
    int methodsLenght;

    //
    sVm = vm;

    // get java vm .
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        XLog::e(TAG ,"ERROR: GetEnv failed\n");
        return JNI_ERR;
    }

    // get the class name .
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        XLog::e(TAG ,"Native registration unable to find class '%s'", className);
        return JNI_ERR;
    }

    // get methods length ,and then register native methods
    methodsLenght = sizeof(methods) / sizeof(methods[0]);
    if (env->RegisterNatives(clazz, methods, methodsLenght) < 0) {
        XLog::e(TAG ,"RegisterNatives failed for '%s'", className);
        return JNI_ERR;
    }

    result = JNI_VERSION_1_4;
    return result;

}
