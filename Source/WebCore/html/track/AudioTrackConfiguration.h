/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(VIDEO)

#include <wtf/text/WTFString.h>

namespace WebCore {

struct AudioTrackConfigurationInit {
    String codec;
    uint32_t sampleRate { 0 };
    uint32_t numberOfChannels { 0 };
    uint64_t bitrate { 0 };
};

class AudioTrackConfiguration : public RefCounted<AudioTrackConfiguration> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static Ref<AudioTrackConfiguration> create(AudioTrackConfigurationInit&& init) { return adoptRef(*new AudioTrackConfiguration(WTFMove(init))); }
    static Ref<AudioTrackConfiguration> create() { return adoptRef(*new AudioTrackConfiguration()); }

    String codec() const { return m_state.codec; }
    void setCodec(String codec) { m_state.codec = codec; }

    uint32_t sampleRate() const { return m_state.sampleRate; }
    void setSampleRate(uint32_t sampleRate) { m_state.sampleRate = sampleRate; }

    uint32_t numberOfChannels() const { return m_state.numberOfChannels; }
    void setNumberOfChannels(uint32_t numberOfChannels) { m_state.numberOfChannels = numberOfChannels; }

    uint64_t bitrate() const { return m_state.bitrate; }
    void setBitrate(uint64_t bitrate) { m_state.bitrate = bitrate; }

private:
    AudioTrackConfiguration(AudioTrackConfigurationInit&& init)
        : m_state(init)
    {
    }
    AudioTrackConfiguration() = default;

    AudioTrackConfigurationInit m_state;
};

}

#endif
