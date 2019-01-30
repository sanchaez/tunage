#pragma once

#include "types.h"
#include "libnyquist/Decoders.h"

#include <thread>
#include <future>
#include <unordered_map>
#include <functional>

#include <QDebug>

#include "ctpl_stl.h"

namespace audioengine {

// private utility functions
namespace {
    template <class Iterator>
    void fade(Iterator begin, Iterator end, float from, float to) {
        float size = std::distance(begin, end);
        float inc_step = (to - from) / size;
        float coeff = from;

        for(auto it = begin; it != end; it += 2) {
             *it *= coeff;
             *(it + 1) *= coeff;
             coeff += inc_step;
        }
    }

    void fade_in(std::vector<float>& samples, float from = 0.f, float to = 1.f) {
        fade(std::begin(samples), std::end(samples), from, to);
    }

    void fade_out(std::vector<float>& samples, float from = 0.f, float to = 1.f) {
        fade(std::rbegin(samples), std::rend(samples), from, to);
    }
}

// callback function to notify about stuff
using DecoderCallbackFn = std::function<void()>;

struct DecodedFile {
    DecodedFile() : loaded(false) {}

    bool loaded;
    std::string filename;
    std::shared_ptr<nqr::AudioData> data;
};

class Decoder {
    std::shared_ptr<RingBuffer> _sample_buffer, _viz_buffer;
    ctpl::thread_pool _pool;
    DecodedFile _current_file;

    std::thread _decoder_thread;
    std::atomic<bool> _running;
    std::atomic<bool> _pause;
    std::atomic<float> _volume;
    std::atomic<int> _current_frame;

    int _buffer_size;
    int _track_frame_length;
    int _track_length_msec;
    int _fx_length_frames;

    constexpr static int _cpu_load_reduction_wait = 10;

    DecoderCallbackFn _position_callback;
    DecoderCallbackFn _file_ended_callback;

    // full path is the key
    // NOTE: this is simpler to implement, but slower than an integer key
    std::unordered_map<std::string, std::future<DecodedFile>> _future_cached_files;
    std::unordered_map<std::string, DecodedFile> _cached_files;

protected:
// sound manipulation
    void apply_volume(std::vector<float>& samples) {
        float applied_volume = _volume;

        for(auto& sample : samples) {
            sample *= applied_volume;
        }
    }

// decode thread fn
    void notify_position_update() {
        if(_position_callback)
            _position_callback();
    }

    void notify_file_end() {
        if(_file_ended_callback)
            _file_ended_callback();
    }

    int write_to_buffer(const std::vector<float>& buffer) {
        assert(buffer.size() % _buffer_size == 0);

        const int buffer_size_frames = buffer.size() / _buffer_size;

        int retry_counter = 0;
        int frames_written = 0;

        while(_running && frames_written < buffer_size_frames) {
            if(_sample_buffer->write(&buffer[frames_written * _buffer_size],
                                     _buffer_size)) {

                _viz_buffer->write(&buffer[frames_written * _buffer_size],
                                   _viz_buffer->getAvailableWrite());

                ++frames_written;
            }

            // reduce CPU load
            std::this_thread::sleep_for(std::chrono::milliseconds(_cpu_load_reduction_wait));
        }

        return frames_written;
    }

    bool write_and_update_pos(const std::vector<float>& buffer) {
        int last_frame = _current_frame;
        _current_frame += write_to_buffer(buffer);
        if(last_frame != _current_frame)
        {
            notify_position_update();
            return true;
        } else {
            return false;
        }
    }

    void decoder_thread_fn()
    {
        bool faded_in = true, faded_out = true;

        while(_running) {
            while(_running && (_pause || !_current_file.loaded)) {
                std::this_thread::sleep_for(
                            std::chrono::milliseconds(_cpu_load_reduction_wait));
            }

            if(!_running)
                return;

            auto& data = _current_file.data;
            faded_in = false;

            // playback
            int frame;
            while(_running && _current_frame < _track_frame_length) {
                frame = _current_frame;

                // slow break out
                if(_pause) {
                    faded_out = false;
                }

                if(!faded_out || !faded_in) {
                    std::vector<float> process_buffer(_buffer_size * _fx_length_frames);

                    int left_frames = _track_frame_length - frame;

                    // copy leftover frames but maintain the length
                    if(left_frames < _fx_length_frames) {
                        std::fill(std::begin(process_buffer), std::end(process_buffer), 0);
                        std::copy(std::begin(data->samples) + frame * _buffer_size,
                                  std::end(data->samples),
                                  std::begin(process_buffer));
                    } else {
                        std::copy_n(std::begin(data->samples) + frame * _buffer_size,
                                    process_buffer.size(),
                                    std::begin(process_buffer));
                    }

                    if(!faded_in) {
                        faded_in = true;
                        fade_in(process_buffer);

                        apply_volume(process_buffer);
                        write_and_update_pos(process_buffer);
                    } else if (!faded_out) {
                        // fast break out
                        if(!_pause) {
                            break;
                        }

                        faded_out = true;
                        fade_out(process_buffer);

                        apply_volume(process_buffer);
                        write_and_update_pos(process_buffer);
                        break;
                    }
                } else {
                    std::vector<float> playback_buffer(_buffer_size);

                    std::copy_n(std::begin(data->samples) + frame * _buffer_size,
                                playback_buffer.size(),
                                std::begin(playback_buffer));

                    apply_volume(playback_buffer);

                    write_and_update_pos(playback_buffer);
                }
            }

            if(_current_frame >= _track_frame_length) {
                _pause = true;
                notify_file_end();
            }
        }
    }

protected:
// cache thread fn
    static DecodedFile decode_to_cache_async(int /*thread_id*/, const std::string& filename)
    {
        DecodedFile file;
        nqr::NyquistIO loader;

        file.data = std::make_shared<nqr::AudioData>();
        file.filename = filename;
        file.loaded = true;

        // retreive data from the file
        try {
            loader.Load(file.data.get(), filename);
        } catch (nqr::UnsupportedExtensionEx& e) {
           std::cerr << "File loading failed: " << e.what() << std::endl;
           file.loaded = false;
           file.data.reset();
        }

        file.loaded = file.loaded && !file.data->samples.empty();

        return file;
    }

    void recalculate_lengths() {
        if(_current_file.loaded) {
            _track_frame_length = ((int) _current_file.data->samples.size()) / _buffer_size;
            _track_length_msec = ((int) _current_file.data->lengthSeconds) * 1000;
        } else {
            _track_frame_length = 0;
            _track_length_msec = 0;
        }
    }

public:
    Decoder() :
          _sample_buffer(std::make_shared<RingBuffer>(default_buffer_size)),
          _viz_buffer(std::make_shared<RingBuffer>(default_fft_buffer_size)),
          _pool(4),
          _decoder_thread(),
          _running(true),
          _pause(true),
          _volume(1.0),
          _current_frame(0),
          _buffer_size(default_buffer_size * default_ring_size),
          _track_frame_length(0),
          _track_length_msec(0),
          _fx_length_frames(default_fx_length_frames)
    {
    }

    ~Decoder()
    {
        _running = false;
        _pause = false;

        if(_decoder_thread.joinable())
            _decoder_thread.join();
    }

// controls
public:
    void decode_to_cache(const std::string& filename) {
        if(is_cached(filename) || is_cached_future(filename))
            return;

        std::future<DecodedFile> future_file = _pool.push(Decoder::decode_to_cache_async, filename);
        _future_cached_files[filename] = std::move(future_file);
    }

    std::vector<int> decode_multiple_to_cache(const std::vector<std::string> &file_list) {
        for(auto &&name : file_list) {
            decode_to_cache(name);
        }
    }

    bool decode_load_single(const std::string& filename)
    {
        decode_to_cache(filename);
        return load_from_cache(filename);
    }

    bool decode_load_multiple(const std::vector<std::string> &file_list) {
        assert(!file_list.empty());

        decode_multiple_to_cache(file_list);

        return load_from_cache(file_list.front());
    }

    bool is_cached(const std::string& filename) {
        return _cached_files.find(filename) != _cached_files.end();
    }

    bool is_cached_future(const std::string& filename) {
        return _future_cached_files.find(filename) != _future_cached_files.end();
    }

    bool load_from_cache(const std::string& filename) {
        // retrieve future file
        stop();

        if(is_cached(filename)) {
            // found saved file
            _current_file = _cached_files.at(filename);
        } else if(is_cached_future(filename)) {
            // get that from a future thread
            auto it = _future_cached_files.find(filename);
            if(it == _future_cached_files.end()) {
                return false;
            }

            auto &future = (*it).second;
            _current_file = future.get();
            _future_cached_files.erase(it);

            if(_current_file.loaded) {
                _cached_files[filename] = _current_file;
            }
        } else {
            return false;
        }

        if(_current_file.data) {
            int new_buffer_size = buffer_size_by_sample_rate(_current_file.data->sampleRate);

            qDebug() << "buffer size" << new_buffer_size << _buffer_size;

            if(new_buffer_size != _buffer_size) {
                set_buffer_size(new_buffer_size);
            }
        }

        recalculate_lengths();

        return _current_file.loaded;
    }

    void remove_from_cache(const std::string& filename) {
        _cached_files.erase(filename);
        _future_cached_files.erase(filename);
    }

    void clear_cache() {
        _cached_files.clear();
    }

    void clear() {
        stop();

        _current_file.data.reset();
        _current_file.loaded = false;

        _track_frame_length = 0;
        _track_length_msec = 0;
    }

    void start_thread() {
        if(!_decoder_thread.joinable()) {
            _running = true;
            _decoder_thread = std::thread(&Decoder::decoder_thread_fn, this);
        }
    }

    void stop_thread() {
        stop();
        _running = false;
        _decoder_thread.join();
    }

    void start() { _pause = false; }

    void pause() { _pause = true; }

    void stop() {
        _pause = true;
        _current_frame = 0;
        notify_position_update();
    }

// getters
public:
    auto sample_buffer() { return _sample_buffer; }
    auto visualizer_buffer() { return _viz_buffer; }

    std::string current_file() const { return _current_file.filename; }

    bool running() const { return _running; }
    bool playing() const { return !_pause; }

    int sample_rate() const {
        return _current_file.data
                ? _current_file.data->sampleRate
                : 0;
    }
    int channels() const {
        return _current_file.data
                ? _current_file.data->channelCount
                : 0;
    }
    int position_frames() const { return _current_frame; }
    double volume() const { return _volume; }

    constexpr int buffer_size() const { return _buffer_size; }
    constexpr int duration_frames() const { return _track_frame_length; }
    constexpr int duration_miliseconds() const { return _track_length_msec; }

    int position_miliseconds() const {
        if(!_current_file.loaded || !_track_frame_length) {
            return 0;
        }
        return _current_frame * (_track_length_msec / _track_frame_length);
    }

// setters
public:
    void set_buffer_size(int buffer_size)
    {
        assert(buffer_size > 0);
        bool was_paused = _pause;

        stop_thread();

        _buffer_size = buffer_size;
        _sample_buffer->clear();
        _viz_buffer->clear();
        _sample_buffer->resize(buffer_size);

        recalculate_lengths();

        start_thread();
    }

    void set_position_frames(int frame) {
        if(frame < 0 || frame > _track_frame_length) {
            return;
        }

        if(_current_file.loaded) {
            _current_frame = frame;
        }
    }

    void set_volume_from_linear(float volume) {
        assert(volume <= 1.0);
        assert(volume >= 0.0);

        _volume = std::pow(volume, 4);
    }

    void set_position_miliseconds(int msec) {
        assert(msec <= _track_length_msec);
        assert(msec >= 0);

        set_position_frames((msec / (double) _track_length_msec) * _track_frame_length );
    }

    void set_position_callback(const DecoderCallbackFn& fn) {
        _position_callback = fn;
    }

    void set_file_end_callback(const DecoderCallbackFn& fn) {
        _file_ended_callback = fn;
    }
};
}
