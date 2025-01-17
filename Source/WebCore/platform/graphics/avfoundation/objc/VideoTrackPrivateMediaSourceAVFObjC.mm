/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "VideoTrackPrivateMediaSourceAVFObjC.h"

#if ENABLE(MEDIA_SOURCE)

#import "AVTrackPrivateAVFObjCImpl.h"
#import "SourceBufferPrivateAVFObjC.h"
#import <AVFoundation/AVAssetTrack.h>

namespace WebCore {

VideoTrackPrivateMediaSourceAVFObjC::VideoTrackPrivateMediaSourceAVFObjC(AVAssetTrack* track)
    : m_impl(makeUnique<AVTrackPrivateAVFObjCImpl>(track))
    , m_trackID(-1)
{
    resetPropertiesFromTrack();
}

void VideoTrackPrivateMediaSourceAVFObjC::resetPropertiesFromTrack()
{
    m_trackID = m_impl->trackID();

    setTrackIndex(m_impl->index());
    setKind(m_impl->videoKind());
    setId(m_impl->id());
    setLabel(m_impl->label());
    setLanguage(m_impl->language());
    setCodec(m_impl->codec());
    setWidth(m_impl->width());
    setHeight(m_impl->height());
    setColorSpace(m_impl->colorSpace());
    setFramerate(m_impl->framerate());
    setBitrate(m_impl->bitrate());
}

void VideoTrackPrivateMediaSourceAVFObjC::setAssetTrack(AVAssetTrack *track)
{
    m_impl = makeUnique<AVTrackPrivateAVFObjCImpl>(track);
    resetPropertiesFromTrack();
}

AVAssetTrack* VideoTrackPrivateMediaSourceAVFObjC::assetTrack() const
{
    return m_impl->assetTrack();
}

FloatSize VideoTrackPrivateMediaSourceAVFObjC::naturalSize() const
{
    return FloatSize([assetTrack() naturalSize]);
}

}

#endif
