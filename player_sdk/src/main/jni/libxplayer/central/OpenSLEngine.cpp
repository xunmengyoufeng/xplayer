//
// Created by chris on 10/24/16.
// Refer to
//      1.http://www.jianshu.com/p/2b8d2de9a47b
//      2.http://ticktick.blog.51cto.com/823160/1771239
//      3.http://www.cnblogs.com/wainiwann/archive/2013/03/27/2984307.html
//      4.https://github.com/KTXSoftware/Kore/blob/5981cf8ce0b982cdfe30a6e1f32260c504bf14c5/Backends/Android/Sources/Kore/Audio.cpp
//

#define TAG "OpenSLEngine_AUDIO_OUT"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "OpenSLEngine.h"

#include "util/XLog.h"

/**
 * audioPlayerCallback
 * play audio buffer.
 * this callback handler is called every time a buffer finishes playing
 */
void audioPlayerCallback(SLAndroidSimpleBufferQueueItf bq,
                                        void* context)
{
    //XLog::e(TAG ,"==>SimpleBufferQueueCallback ===============in callback.>\n");

    OpenSLEngine *openSLEngine = (OpenSLEngine *)context;
    // get decode audio pcm data
    AVFrame *audioFrame;
    audioFrame = av_frame_alloc();
    // TODO
    //openSLEngine->mediaFileHandle->audio_frame_queue->get(audioFrame);
    #if 1
    int rr = openSLEngine->mediaFileHandle->audio_frame_queue->get(audioFrame ,0);
    if(rr == 0){
        XLog::e(TAG ,"==>SimpleBufferQueueCallback ===============in callback. have no audio data> ,audio_packet_q_size=%d ,video_packet_q_size =%d\n" ,openSLEngine->mediaFileHandle->audio_queue->size() ,openSLEngine->mediaFileHandle->video_queue->size());
        openSLEngine->mediaFileHandle->stopRender();  // TODO
        uint8_t silence_buf[1024] = {0};
        (*bq)->Enqueue(bq, (uint8_t *) silence_buf , 1);  // must add this statement .
        // free avframe memory
        if(audioFrame){
            av_frame_free(&audioFrame);
        }
        return;
    }
    #endif
    //
    int needed_buf_size = av_samples_get_buffer_size(NULL,
                                           audioFrame->channels,
                                           audioFrame->nb_samples,
                                           AV_SAMPLE_FMT_S16, 0);

    int outsamples = swr_convert(openSLEngine->swr_ctx,&openSLEngine->resampled_buf,needed_buf_size,(const uint8_t**)audioFrame->data, audioFrame->nb_samples);

    // set audio pts TODO
    openSLEngine->mediaFileHandle->sync_audio_clock_time = audioFrame->pkt_pts * av_q2d(openSLEngine->mediaFileHandle->audio_stream->time_base) * 1000;   // in millisecond , use
    // set current position
    openSLEngine->mediaFileHandle->current_position_ms = audioFrame->pkt_pts * av_q2d(openSLEngine->mediaFileHandle->audio_stream->time_base) * 1000 - openSLEngine->mediaFileHandle->beginning_audio_pts;   // in millisecond , use

    //XLog::d(ANDROID_LOG_WARN ,TAG ,"==>sync_audio_clock_time=%f\n", openSLEngine->mediaFileHandle->sync_audio_clock_time);

    // end
    int resampled_data_size = outsamples * audioFrame->channels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    //XLog::d(ANDROID_LOG_WARN ,TAG ,"==>audio frame queue size :%d ,linesize[0] =%d ,resampled_data_size=%d\n", openSLEngine->mediaFileHandle->audio_frame_queue->size() ,audioFrame->linesize[0] ,resampled_data_size);

    (*bq)->Enqueue(bq, openSLEngine->resampled_buf ,resampled_data_size);
    // en queue.

    // free avframe memory
    if(audioFrame){
        av_frame_free(&audioFrame);
    }


}

OpenSLEngine::OpenSLEngine(MediaFile *mediaFile)
{

    mediaFileHandle = mediaFile;

    resampled_buf = NULL;
    swr_ctx = NULL;

    m_paused = false;
    m_running = true;

    isInitialized = false;
}

OpenSLEngine::~OpenSLEngine()
{
    // audio buffer
    if(resampled_buf)
    {
        free(resampled_buf);
        resampled_buf = NULL;
    }

    if(swr_ctx != NULL){
        swr_free(&swr_ctx);
        swr_ctx = NULL;
    }

}

bool OpenSLEngine::isPlaying()
{

    if ((bqPlayerPlay == NULL )|| (isInitialized == false )){
        XLog::e(TAG ,"==>SimpleBufferQueueCallback  ,in isPlaying function ,bqPlayerPlay == NULL===============>\n");
        return false;
    }

    SLuint32 state;
    SLresult result = (*bqPlayerPlay)->GetPlayState(bqPlayerPlay, &state);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }

    return (state == SL_PLAYSTATE_PLAYING);
}


bool OpenSLEngine::isPausing()
{
    if ((bqPlayerPlay == NULL )|| (isInitialized == false )){
        XLog::e(TAG ,"==>SimpleBufferQueueCallback  ,in isPausing function ,bqPlayerPlay == NULL===============>\n");
        return false;
    }

    SLuint32 state;
    SLresult result = (*bqPlayerPlay)->GetPlayState(bqPlayerPlay, &state);
    if(result != SL_RESULT_SUCCESS)
    {
        return false;
    }

    return (state == SL_PLAYSTATE_PAUSED);

}

void OpenSLEngine::play()
{
    if (bqPlayerPlay == NULL){
        XLog::e(TAG ,"==>SimpleBufferQueueCallback  ,in play function ,bqPlayerPlay == NULL===============>\n");
        return;
    }
    XLog::e(TAG ,"==>SimpleBufferQueueCallback  ,in play function start===============>\n");
    SLresult result;
    m_paused = false;
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);   // start play.
    assert(SL_RESULT_SUCCESS == result);
    XLog::e(TAG ,"==>SimpleBufferQueueCallback  ,in play function end===============>\n");
}

void OpenSLEngine::pause()
{
    XLog::e(TAG ,"==>in OpenSLEngine::pause function>\n");
    if ((bqPlayerPlay == NULL )|| (isInitialized == false )){ return;}

    m_paused = true;
    SLresult result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PAUSED);   // set paused.
    assert(SL_RESULT_SUCCESS == result);
    XLog::e(TAG ,"==>in OpenSLEngine::pause function ,ret =%d===============>\n" ,result);
}

void OpenSLEngine::resume()
{
    if ((bqPlayerPlay == NULL )|| (isInitialized == false )){ return;}

    m_paused = false;
    SLresult result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    XLog::e(TAG ,"==>in OpenSLEngine::resume function ,ret =%d===============>\n" ,result);
}

void OpenSLEngine::stop()
{
    if ((bqPlayerPlay == NULL )|| (isInitialized == false )){ return;}
    XLog::e(TAG ,"==>OpenSLEngine::stop 1\n");
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
    XLog::e(TAG ,"==>OpenSLEngine::stop 2\n");
    if (bqPlayerObject != NULL) {
        XLog::e(TAG ,"==>OpenSLEngine::stop 3\n");
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
    }
    XLog::e(TAG ,"==>OpenSLEngine::stop 4\n");
    if (outputMixObject != NULL) {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = NULL;
    }

    if (engineObject != NULL) {
        (*engineObject)->Destroy(engineObject);
        engineObject = NULL;
        engineEngine = NULL;
    }

}

void OpenSLEngine::createEngine()
{
    SLresult result;

    // create audio engine
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    assert(SL_RESULT_SUCCESS == result);

    // realize the engine (initialize the audio engine object)
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

    // get the engine interface, which is needed in order to create other objects
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine);
    assert(SL_RESULT_SUCCESS == result);

}

void OpenSLEngine::createOutputMix()
{
    SLresult result;

    // create output mix
    const SLInterfaceID ids[1] = {SL_IID_VOLUME};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 1, ids, req);
    assert(SL_RESULT_SUCCESS == result);

    // realize the output mix (initialize the output mix)
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    assert(SL_RESULT_SUCCESS == result);

}

int OpenSLEngine::createAudioPlayer()
{
    // get audio stream codec parameters
    AVCodecParameters *audioCodecParameters = mediaFileHandle->audio_stream->codecpar;

    SLresult result = SL_RESULT_SUCCESS;

    SLDataLocator_AndroidSimpleBufferQueue bufferQueueLocator = {};
    bufferQueueLocator.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
    bufferQueueLocator.numBuffers  = BUFFER_COUNT;

    // pcm format setting.
    SLDataFormat_PCM dataFormat = {};
    dataFormat.formatType    = SL_DATAFORMAT_PCM;
    dataFormat.numChannels   = audioCodecParameters->channels;
    dataFormat.samplesPerSec = audioCodecParameters->sample_rate * 1000;    // pAudioSrc: samplesPerSec=44100  SL_RESULT_CONTENT_UNSUPPORTED
    dataFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16; // Notice Here PCM  16
    dataFormat.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
    dataFormat.channelMask   = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    if(dataFormat.numChannels == 1){dataFormat.channelMask   = SL_SPEAKER_FRONT_CENTER;}
    dataFormat.endianness    = SL_BYTEORDER_LITTLEENDIAN;

    // 1. configure data source
    SLDataSource dataSource = {};
    dataSource.pLocator = &bufferQueueLocator;  // TODO buffer queue locator
    dataSource.pFormat  = &dataFormat;  // TODO data format

    // 2. configure audio sink
    SLDataLocator_OutputMix outputMixLocator = {};
    outputMixLocator.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    outputMixLocator.outputMix   = outputMixObject;     //

    SLDataSink dataSink = {};
    dataSink.pLocator = &outputMixLocator;

    // 3. create audio player object
    //const SLInterfaceID ids1[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
    //const SLboolean req1[] = { SL_BOOLEAN_TRUE };
    const SLInterfaceID ids1[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND,
            SL_IID_VOLUME};
    const SLboolean req1[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE,
            SL_BOOLEAN_TRUE};
    XLog::e(TAG ,"SimpleBufferQueueCallback ,CreateAudioPlayer \nformatType[%d]\nnumChannels[%d]\nsamplesPerSec[%d]\nbitsPerSample[%d]\ncontainerSize[%d]\nchannelMask[%d]\nendianness[%d]",
        dataFormat.formatType,dataFormat.numChannels,dataFormat.samplesPerSec,
        dataFormat.bitsPerSample,dataFormat.containerSize,dataFormat.channelMask,
        dataFormat.endianness);

    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &dataSource, &dataSink,
            3, ids1, req1);
    assert(result == SL_RESULT_SUCCESS);


    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(result == SL_RESULT_SUCCESS);

    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(result == SL_RESULT_SUCCESS);

    //
    //result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bqPlayerBufferQueue);
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);
    assert(result == SL_RESULT_SUCCESS);

    // register callback
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, audioPlayerCallback, this);
    assert(result == SL_RESULT_SUCCESS);

    // TODO
    uint8_t silence_buf[1024];
    (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, (uint8_t *) silence_buf , 1);  // must add this statement .

    // set the player's state to playing
    //result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    //assert(result == SL_RESULT_SUCCESS);

    if(audioCodecParameters->format != AV_SAMPLE_FMT_S16){
        // init context
        #define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

        int out_size = MAX_AUDIO_FRAME_SIZE*10; // *100
        resampled_buf = (uint8_t *)malloc(out_size);
        if(resampled_buf == NULL){
            XLog::e(TAG ,"===resampled_buf malloc failed. ");
            return -1;
        }

        swr_ctx = swr_alloc();
        int64_t in_out_channel_layout = audioCodecParameters->channel_layout;
        if(in_out_channel_layout == 0){  // fix bug for mono channel file play audio dirty.
            in_out_channel_layout = (audioCodecParameters->channels == 2)?AV_CH_LAYOUT_STEREO:AV_CH_LAYOUT_MONO;
        }

        swr_ctx = swr_alloc_set_opts(swr_ctx,
                               in_out_channel_layout,
                               AV_SAMPLE_FMT_S16, audioCodecParameters->sample_rate,

                               in_out_channel_layout,
                               (AVSampleFormat)audioCodecParameters->format, audioCodecParameters->sample_rate,
                               0, NULL);

        XLog::e(TAG ,"=====>audioCodecParameters->channel_layout = %d\n" ,audioCodecParameters->channel_layout);
        XLog::e(TAG ,"=====>audioCodecParameters->sample_rate = %d\n" ,audioCodecParameters->sample_rate);
        XLog::e(TAG ,"=====>audioCodecParameters->format = %d\n" ,audioCodecParameters->format);

        if (!swr_ctx){
            XLog::e(TAG ,"Failed to initialize the swr_ctx\n");
            return -1;
        }

        int ret = 0;
        if ((ret = swr_init(swr_ctx)) < 0) {

            XLog::e(TAG ,"Failed to initialize the resampling context ,ret = %d\n" ,ret);
            return -1;
        }
    }

    return 0;
}

int OpenSLEngine::InitPlayout()
{
    createEngine();
    createOutputMix();
    createAudioPlayer();
    isInitialized = true;
    return 0;
}
