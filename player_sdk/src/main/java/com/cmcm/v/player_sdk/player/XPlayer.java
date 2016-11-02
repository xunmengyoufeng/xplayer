package com.cmcm.v.player_sdk.player;

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.SurfaceHolder;

import java.lang.ref.WeakReference;

/**
 * Created by chris on 9/26/16.
 * Refer to:
 *  1. https://github.com/commshare/testSparrow/blob/92ec6613f916d4575d4c78f61820108aeec22dcd/ref/FFmpegMediaPlayer/gradle/fmp-library/library/src/main/jni/player/mediaplayer.cpp
 *  2. https://github.com/IllusionLP/platform_frameworks_base/blob/da5223eaf23dda165962fccb1974425a941a82e1/core/jni/android_media_DeviceCallback.cpp
 *  3. https://github.com/commshare/testSparrow/blob/92ec6613f916d4575d4c78f61820108aeec22dcd/ref/FFmpegMediaPlayer/gradle/fmp-library/library/src/main/jni/player/wseemann_media_MediaPlayer.cpp
 *  4. Media 播放器 postEventFromNative  研究明白 再写这块。
 *      http://blog.csdn.net/llping2011/article/details/21239635
 *      postEventFromNative(): 传递来自Native的事件
 *      // MediaPlayer postEventFromNative
 */
public class XPlayer extends SimpleMediaPlayer {

    public static String TAG = "XPlayer";

    // video width
    private int mVideoWidth;

    // video height
    private int mVideoHeight;

    /**
     * sample aspect ratio (sar)
     * That is the width of a pixel divided by the height of the pixel.
     * Numerator(num) and denominator(den) must be relatively prime and
     * smaller than 256 for some video standards. 0 stands for unknown.
     * The numerator and denominator will be obtained during opening the
     * video stream.
     */
    private int mVideoSarNum;
    private int mVideoSarDen;


    /**
     * Event Handle for event from native.
     */
    EventHandler mEventHandler;

    static {
        try {
            System.loadLibrary("ffmpeg");
            System.loadLibrary("cmxplayer");

        } catch(Exception e) {
            e.printStackTrace(System.out);
        }

        //
        _native_init();
    }

    public XPlayer() {
        // new player object ,and get player handle.
        Looper looper;
        if ((looper = Looper.myLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else {
            mEventHandler = null;
        }

        _native_setup(new WeakReference<XPlayer>(this));
    }


    @Override
    public int getVideoWidth() {
        return mVideoWidth;
    }

    @Override
    public int getVideoHeight() {
        return mVideoHeight;
    }

    @Override
    public int getVideoSarNum() {
        return mVideoSarNum;
    }

    @Override
    public int getVideoSarDen() {
        return mVideoSarDen;
    }

    @Override
    public void setDisplay(SurfaceHolder sh) {

    }

    @Override
    public void setDataSource(String path) {
        _setDataSource(path);
    }

    @Override
    public void prepareAsync() throws IllegalStateException {
        _prepareAsync();
    }

    @Override
    public String getDataSource() {
        return null;
    }

    @Override
    public void start() throws IllegalStateException {
        _start();
    }

    public void play() {
        _play();
    }

    @Override
    public void stop() throws IllegalStateException {

    }

    @Override
    public void pause() throws IllegalStateException {
        _pause();
    }

    private native void _pause() throws IllegalStateException;

    private native void _resume();


    @Override
    public void setScreenOnWhilePlaying(boolean screenOn) {

    }

    @Override
    public boolean isPlaying() {
        return false;
    }



    @Override
    public void seekTo(long msec) throws IllegalStateException {

    }

    @Override
    public long getCurrentPosition() {
        return 0;
    }

    @Override
    public long getDuration() {
        return 0;
    }


    @Override
    public void release() {

    }

    @Override
    public void reset() {

    }

    @Override
    public void setVolume(float leftVolume, float rightVolume) {

    }


    @Override
    public boolean isPlayable() {
        return false;
    }



    @Override
    public void resetListeners() {

    }

    @Override
    public void setAudioStreamType(int streamtype) {

    }

    @Override
    public int getIsLiveVideo() throws IllegalStateException {
        return 0;
    }

    @Override
    public String getHttpInfoStr() throws IllegalStateException {
        return null;
    }

    @Override
    public String getLibCompileTime() {
        return null;
    }

    @Override
    public Bitmap getCurrentFrame() {
        return null;
    }

    /**
     * receive native message.
     */
    private static void postEventFromNative(Object weakThiz, int what, int arg1, int arg2, Object obj)
    {

        XPlayer mp = (XPlayer) ((WeakReference<?>) weakThiz).get();
        if (mp == null)
        {
            Log.e(TAG ,"===>in postEventFromNative ,but mp equal null.");
            return;
        }
        if (mp.mEventHandler != null)
        {
            Log.e(TAG ,"===>in postEventFromNative 2.what =" +what + ",arg1="+ arg1);
            Message m = mp.mEventHandler.obtainMessage(what, arg1, arg2, obj);
            mp.mEventHandler.sendMessage(m);
        }
    }


    private static final int MEDIA_NOP = 0; // interface test message
    private static final int MEDIA_PREPARED = 1;
    private static final int MEDIA_PLAYBACK_COMPLETE = 2;
    private static final int MEDIA_BUFFERING_UPDATE = 3;
    private static final int MEDIA_SEEK_COMPLETE = 4;
    private static final int MEDIA_SET_VIDEO_SIZE = 5;
    private static final int MEDIA_ERROR = 100;
    private static final int MEDIA_INFO = 200;

    protected static final int MEDIA_SET_VIDEO_SAR = 10001;

    private class EventHandler extends Handler
    {
        private XPlayer mMediaPlayer;

        public EventHandler(XPlayer ffmpegPlayer, Looper looper)
        {
            super(looper);
            mMediaPlayer = ffmpegPlayer;
        }

        @Override
        public void handleMessage(Message msg)
        {
            //if (mMediaPlayer.mNativeContext == 0)
            {
               // return;
            }
            Log.e(TAG ,"====>on ...===>msg.what" + msg.what);
            switch (msg.what)
            {
                case MEDIA_PREPARED:

                    Log.e(TAG ,"====>on ...MEDIA_PREPARED");
                    if (mOnPreparedListener != null){
                        Log.e(TAG ,"====>on ...MEDIA_PREPARED ,is not null");
                        mOnPreparedListener.onPrepared(mMediaPlayer); // TODO call player start function ,on jni start function will send according to EVT message.
                    }else{
                        Log.e(TAG ,"====>on ...MEDIA_PREPARED ,is null");
                    }
                    return;

                case MEDIA_BUFFERING_UPDATE:
                    Log.e(TAG ,"====>on ...MEDIA_BUFFERING_UPDATE :" + msg.arg1);
                    if (mOnBufferingUpdateListener != null){
                        mOnBufferingUpdateListener.onBufferingUpdate(mMediaPlayer, msg.arg1);
                    }
                    return;

                case MEDIA_PLAYBACK_COMPLETE:
                    Log.e(TAG ,"====>on ...MEDIA_PLAYBACK_COMPLETE");
                    if (mOnCompletionListener != null){
                        mOnCompletionListener.onCompletion(mMediaPlayer);
                    }
                    return;

                case MEDIA_SEEK_COMPLETE:
                    return;

                case MEDIA_SET_VIDEO_SIZE:
                {
                    Log.e(TAG ,"====>on ...MEDIA_SET_VIDEO_SIZE");
                    mMediaPlayer.mVideoWidth = msg.arg1;
                    mMediaPlayer.mVideoHeight = msg.arg2;
                    if (mOnVideoSizeChangedListener != null){
                        mOnVideoSizeChangedListener.onVideoSizeChanged( mMediaPlayer ,mMediaPlayer.mVideoWidth, mMediaPlayer.mVideoHeight, mMediaPlayer.mVideoSarNum, mMediaPlayer.mVideoSarDen);
                    }
                    return;
                }
                case MEDIA_SET_VIDEO_SAR:
                {
                    Log.e(TAG ,"====>on ...MEDIA_SET_VIDEO_SAR");
                    mMediaPlayer.mVideoSarNum = msg.arg1;
                    mMediaPlayer.mVideoSarDen = msg.arg2;
                    if (mOnVideoSizeChangedListener != null){
                        mOnVideoSizeChangedListener.onVideoSizeChanged( mMediaPlayer ,mMediaPlayer.mVideoWidth, mMediaPlayer.mVideoHeight, mMediaPlayer.mVideoSarNum, mMediaPlayer.mVideoSarDen);
                    }
                    return;
                }
                case MEDIA_ERROR:
                    Log.e(TAG, "Error (" + msg.arg1 + "," + msg.arg2 + ")");
                    return;

                case MEDIA_INFO:
                    Log.i(TAG, "Info (" + msg.arg1 + "," + msg.arg2 + ")");
                    return;

                case MEDIA_NOP: // interface test message - ignore
                    break;

                default:
                    Log.e(TAG, "Unknown message type " + msg.what);
                    return;
            }

        }
    }


    @Override
    public void initEGLCtx(){
        _initEGLCtx();
    }

    @Override
    public void renderFrame(){
        _renderFrame();
    }

    @Override
    public void initPlayer(){
        _init();
    }

    @Override
    public void playInterface(){
        _play();
    }

    public static native void _native_init();

    /**
     * native setup function ,create mediaplayer.
     * Native setup requires a weak reference to our object.
     * It's easier to create it here than in C++.
     * @param mediaplayer_this
     */
    public native void _native_setup(Object mediaplayer_this); //(WeakReference<XPlayer> xPlayerWeakReference);

    /**
     * init xplayer .
     * 1. register all codecs and formats
     * 2. create need threads.
     */
    public native void _init();

    /**
     * init opengl context .
     */
    public native void _initEGLCtx();

    /**
     * set DataSource
     */
    public native void _setDataSource(String path);

    /**
     * prepare ,and get mediainfo if prepared success.
     */
    public native void _prepareAsync() throws IllegalStateException;

    public native void _start();

    public native void _play();

    public native void _renderFrame();

}
