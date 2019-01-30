#pragma once

#include "types.h"
#include "ringbuffer.h"
#include "kiss_fft.h"

#include <vector>
#include <memory>
#include <thread>
#include <algorithm>
#include <mutex>
#ifndef _MSC_VER
#include <cmath>
#else
#define _USE_MATH_DEFINES
#include <math.h>
#endif

namespace audioengine {

/*
 * SpectrumAnalyzer applies a FFT to a buffer frame and outputs in into a ringbuffer.
 */
class SpectrumAnalyzer
{
    std::thread _thread;
    std::shared_ptr<RingBuffer> _source;
    std::atomic<bool> _running;
    std::function<void()> _update_callback;

    const int _fft_size;
    const int _audio_read_size;

    constexpr static int wait_msec = 5;
    constexpr static double smoothing_fft = 0.8;
    constexpr static double smoothing_wave = 0.6;
    constexpr static double high_fft_bound = 40;
    constexpr static double low_fft_bound = -64;
    constexpr static int wait_for_silence_iterations = 60;

    template <typename T>
    void hann(std::vector<T>& v) {
        int size = static_cast<int>(v.size());
        for (int i = 0; i < size; i++)
        {
            v[i] *= 0.5 * (1.0 - std::cos(2.0 * M_PI * i / (float) size));
        }
    }

    template <typename T>
    void normalize(std::vector<T>& v, T min_val, T max_val) {
        // normalize
        float scale_factor = max_val - min_val + 0.00001f;

        for(auto &x : v) {
            x = (x - min_val) / scale_factor;
        }
    }

    template <typename T>
    void smooth(std::vector<T>& v, const std::vector<T>& prev, T alpha) {
        std::transform(std::begin(v), std::end(v),
                       std::begin(prev),
                       std::begin(v),
                       [alpha](T x , T prev) -> T {
            return alpha * prev + ((1 - alpha) * x);
        });

    }

    template <typename T>
    void dropoff(std::vector<T>& v, T alpha) {
        for(auto && x : v) {
            x *= alpha;
        }
    }

    template <typename T>
    std::vector<T> calculate_frequency_spectrum(std::vector<T> samples)
    {
        auto samples_size = samples.size();
        std::vector<T> output;

        // apply windows function
        hann(samples);

        // convert values for fft
        std::vector<kiss_fft_cpx> fft_in(samples_size), fft_out(samples_size);

        for(auto i = 0; i < samples_size; i++) {
            fft_in[i].r = samples[i];
            fft_in[i].i = 0;
        }

        // calculate fft
        kiss_fft_cfg mycfg = kiss_fft_alloc(samples_size, 0, NULL, NULL);
        kiss_fft(mycfg, fft_in.data(), fft_out.data());

        // trim fft result in half
        fft_out.resize(samples_size / 2);

        // calculate magnitude in a dB scale and output
        output.reserve(samples_size);

        for(auto &&x : fft_out) {
            T magnitude_db = 10 * std::log10(x.r * x.r + x.i * x.i);
            magnitude_db = magnitude_db >= high_fft_bound
                    ? high_fft_bound
                    : magnitude_db <= low_fft_bound
                      ? low_fft_bound
                      : magnitude_db;

            output.push_back(magnitude_db);
        }

        free(mycfg);

        return output;
    }

    void thread_fn()
    {
        std::vector<float> wave_interleaved_stereo(_audio_read_size);
        std::vector<double> fft_avg(_fft_size), fft_avg_previous;
        std::vector<double> wave_avg(_fft_size), wave_avg_prev;

        bool wave_silenced = true, spectrum_silenced = true;

        // maximum values ever displayed
        auto write_all_data = [&]() {
            fft_avg_out->write(fft_avg.data(), _fft_size);
            waveform_avg_out->write(wave_avg.data(), _fft_size);

            if(_update_callback)
                _update_callback();
        };

        // initialize
        std::fill(fft_avg.begin(), fft_avg.end(), 0);
        std::fill(wave_avg.begin(), wave_avg.end(), 0.5);

        write_all_data();

        fft_avg_previous = fft_avg;
        wave_avg_prev = wave_avg;

        int silence_count = wait_for_silence_iterations;
        while(_running) {
            // read just a bit
            bool read_status = _source->read(wave_interleaved_stereo.data(),
                                             _audio_read_size);

            // process values
            if(read_status) {
                silence_count = 0;
                wave_silenced = false;
                spectrum_silenced = false;
                _source->clear();

                // convert to mono
                const int fft_calc_size = _fft_size * 2;
                std::vector<double> wave_avg_full(fft_calc_size);
                for(int i = 0, j = 0; j < fft_calc_size; ++j, i += stereo) {
                    wave_avg_full[j] = (wave_interleaved_stereo[i] + wave_interleaved_stereo[i + 1]) / double(2);
                }

                std::copy_n(std::begin(wave_avg_full), _fft_size, std::begin(wave_avg));

                // get frequency info
                fft_avg = calculate_frequency_spectrum(wave_avg_full);

                // smooth
                smooth(fft_avg, fft_avg_previous, smoothing_fft);
                smooth(wave_avg, wave_avg_prev, smoothing_wave);

                fft_avg_previous = fft_avg;
                wave_avg_prev = wave_avg;

                normalize(wave_avg, -1., 1.);
                normalize(fft_avg, low_fft_bound, high_fft_bound);

                // write to ringbuffer
                write_all_data();
            } else if(!wave_silenced || !spectrum_silenced) {
                if(silence_count < wait_for_silence_iterations) {
                    ++silence_count;
                } else {
                    if(!wave_silenced) {
                        wave_silenced = true;
                        std::fill(wave_avg.begin(), wave_avg.end(), 0.5);
                    }

                    if(!spectrum_silenced) {
                        // drop off slowly
                        double max_value = low_fft_bound;

                        dropoff(fft_avg, 0.94);

                        // find out if silenced
                        for(auto &x : fft_avg) {
                            if(max_value < x) {
                                max_value = x;
                            }
                        }

                        if(max_value <= low_fft_bound) {
                            spectrum_silenced = true;

                            // silence completely
                            std::fill(fft_avg.begin(), fft_avg.end(), low_fft_bound);
                        }
                    }
                    write_all_data();
                }
            }

            // FIXME: replace this with 60 Hz polling
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }


public:
    std::shared_ptr<RingBufferT<double>> fft_avg_out, waveform_avg_out;

    SpectrumAnalyzer(std::shared_ptr<RingBuffer> source) :
        _source(source),
        _running(true),
        _fft_size(default_fft_size),
        _audio_read_size(default_fft_read_size),
        fft_avg_out(std::make_shared<RingBufferT<double>>(_fft_size)),
        waveform_avg_out(std::make_shared<RingBufferT<double>>(_fft_size))
    {
    }

    void set_update_callback(const std::function<void()>& callback) {
        _update_callback = callback;
    }

    ~SpectrumAnalyzer()  {
        _running = false;

        if(_thread.joinable())
            _thread.join();
    }

    void start_thread() {
        if(!_thread.joinable()) {
            _running = true;
            _thread = std::thread(&SpectrumAnalyzer::thread_fn, this);
        }
    }

    void stop_thread() {
        _running = false;
        _thread.join();
    }

    std::shared_ptr<RingBuffer> source() const;
    void setSource(const std::shared_ptr<RingBuffer> &source);
};

} // audioengine
