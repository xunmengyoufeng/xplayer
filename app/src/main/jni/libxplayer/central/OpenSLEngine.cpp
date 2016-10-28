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

#include "OpenSLEngine.h"

#include "util/XLog.h"

/**
 * SimpleBufferQueueCallback
 * play audio buffer.
 * this callback handler is called every time a buffer finishes playing
 */
void SimpleBufferQueueCallback1(SLAndroidSimpleBufferQueueItf bq,
                                        void* context)
{
    // int bytes = fread(playBuffer, 1, 2048 * sizeof(short), file);    // TODO
    // (*bq)->Enqueue(bq, playBuffer,bytes);                            // TODO

    XLog::e(TAG ,"==>SimpleBufferQueueCallback ===============in callback.>\n");

    OpenSLEngine *openSLEngine = (OpenSLEngine *)context;
    // get decode audio pcm data
    AVFrame *src_frame;
    src_frame = av_frame_alloc();
    // TODO
    openSLEngine->mediaFileHandle->audio_frame_queue->get(src_frame);
    XLog::d(ANDROID_LOG_WARN ,TAG ,"==>audio frame queue size :%d ,linesize[0] =%d\n", openSLEngine->mediaFileHandle->audio_frame_queue->size() ,src_frame->linesize[0]);

    (*bq)->Enqueue(bq, src_frame->data[0] ,src_frame->linesize[0]);
    // en queue.

}

OpenSLEngine::OpenSLEngine(MediaFile *mediaFile)
{

    mediaFileHandle = mediaFile;

    m_paused = false;
    m_running = true;
}

OpenSLEngine::~OpenSLEngine()
{


}

void OpenSLEngine::play()
{
    if (bqPlayerPlay == NULL){
        XLog::e(TAG ,"==>SimpleBufferQueueCallback  ,in play function ,bqPlayerPlay == NULL===============>\n");
        return;
    }
    XLog::e(TAG ,"==>SimpleBufferQueueCallback  ,in play function start===============>\n");
    SLresult result;
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);   // start play.
    assert(SL_RESULT_SUCCESS == result);
    XLog::e(TAG ,"==>SimpleBufferQueueCallback  ,in play function end===============>\n");
}

void OpenSLEngine::pause()
{
    if (bqPlayerPlay == NULL){ return;}

    m_paused = true;
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PAUSED);   // set paused.

}

void OpenSLEngine::resume()
{
    if (bqPlayerPlay == NULL){ return;}

    m_paused = false;
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
}

void OpenSLEngine::stop()
{
    if (bqPlayerPlay == NULL){ return;}
    (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);

    if (bqPlayerObject != NULL) {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = NULL;
        bqPlayerPlay = NULL;
        bqPlayerBufferQueue = NULL;
    }

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

void OpenSLEngine::createAudioPlayer()
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
    const SLInterfaceID ids1[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
    const SLboolean req1[] = { SL_BOOLEAN_TRUE };
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &dataSource, &dataSink,
            1, ids1, req1);
    assert(result == SL_RESULT_SUCCESS);


    result = (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);
    assert(result == SL_RESULT_SUCCESS);

    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
    assert(result == SL_RESULT_SUCCESS);

    //
    result = (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &bqPlayerBufferQueue);
    assert(result == SL_RESULT_SUCCESS);

    // register callback
    //result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, &SimpleBufferQueueCallback, this);
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, SimpleBufferQueueCallback1, this);
    assert(result == SL_RESULT_SUCCESS);

//    // set the player's state to playing
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
    assert(result == SL_RESULT_SUCCESS);
    XLog::e(TAG ,"==>SimpleBufferQueueCallback ,createAudioPlayer===>\n");
}

int OpenSLEngine::InitPlayout()
{
    createEngine();
    createOutputMix();
    createAudioPlayer();
    XLog::e(TAG ,"==>SimpleBufferQueueCallback InitPlayout 4===============>\n");
    return 0;
}


