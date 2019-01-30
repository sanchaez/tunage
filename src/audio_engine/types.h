#pragma once

#include "ringbuffer.h"
#include "portaudio.h"

#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <numeric>
#include <future>

namespace audioengine {
constexpr static int stereo = 2;

// defaults for audio playback
constexpr static int default_buffer_size = 4096;
constexpr static int default_ring_size = 2;
constexpr static int default_fx_length_frames = 16;

// defaults for fft
constexpr static int default_fft_ring_size = 4;
constexpr static int default_fft_size = 256;
constexpr static int default_fft_read_size = default_fft_size * 4;
constexpr static int default_fft_buffer_size = default_fft_read_size * default_fft_ring_size;

constexpr static int buffer_size_by_sample_rate(int sample_rate) {
    if(sample_rate <= 48000) {
        return default_buffer_size;
    } else if(sample_rate <= 96000 ) {
        return default_buffer_size * 2;
    } else if(sample_rate <= 192000) {
        return default_buffer_size * 4;
    }
}
}
