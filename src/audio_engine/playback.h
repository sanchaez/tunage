#pragma once

#include "types.h"

namespace audioengine {


class Playback {
    std::shared_ptr<RingBuffer> _playback_buffer;
    PaStream* _stream;

    int _sample_rate;
    int _buffer_size;

protected:
    inline static int audio_callback(const void* /*input_buffer*/,
                                     void* output_buffer,
                                     unsigned long buffer_size,
                                     const PaStreamCallbackTimeInfo* /*stream_time*/,
                                     PaStreamCallbackFlags status,
                                     void* user_data)
    {
        if(!user_data) {
            std::cerr << "[audio_callback] no data given" << std::endl;
            return -1;
        }

        if(status) {
            std::cerr << "[audio_callback] buffer over or underflow" << std::endl;
        }

        // playback
        auto out = static_cast<float*>(output_buffer);
        auto playback_buffer = static_cast<RingBuffer*>(user_data);

        memset(out, 0, buffer_size * stereo * sizeof(float));
        playback_buffer->read(out, buffer_size * stereo);

        return 0;
    }

public:
    Playback() : _stream(NULL), _sample_rate(0), _buffer_size(0)
    {
        auto err = Pa_Initialize();
        if(err != paNoError)
            std::cerr <<  "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    ~Playback()
    {
        stream_stop();
        stream_close();
        auto err = Pa_Terminate();
        if(err != paNoError)
            std::cerr <<  "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    void set_playback_buffer(const std::shared_ptr<RingBuffer>& playback_buffer)
    {
        _playback_buffer = playback_buffer;
    }


    void stream_create(int sample_rate, int buffer_size)
    {
        if(_stream) {
            stream_stop();
            stream_close();
        }

        _sample_rate = sample_rate;
        _buffer_size = buffer_size;

        auto err = Pa_OpenDefaultStream(&_stream,
                                        NULL,
                                        stereo,
                                        paFloat32,
                                        (double) sample_rate,
                                        buffer_size / stereo,
                                        &audio_callback,
                                        (void*) _playback_buffer.get());
        if(err != paNoError)
            std::cerr <<  "PortAudio error: " << Pa_GetErrorText(err) << std::endl;

        stream_start();
    }

    void stream_start()
    {
        if(!Pa_IsStreamActive(_stream)) {
            auto err = Pa_StartStream(_stream);
            if(err != paNoError)
                std::cerr <<  "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        }
    }

    void stream_stop()
    {
        if(_stream && !Pa_IsStreamStopped(_stream)) {
            auto err = Pa_AbortStream(_stream);
            if(err != paNoError)
                std::cerr <<  "PortAudio error: "
                           << Pa_GetErrorText(err) << std::endl;
        }
    }

    void stream_close()
    {
        if(_stream) {
            auto err = Pa_CloseStream(_stream);
            if(err != paNoError)
                std::cerr <<  "PortAudio error: "
                           << Pa_GetErrorText(err) << std::endl;
            _stream = NULL;
        }
    }

    bool running() {
        return _stream && Pa_IsStreamActive(_stream);
    }

    int sample_rate() {return _sample_rate; }
    int buffer_size() {return _buffer_size; }
};

}
