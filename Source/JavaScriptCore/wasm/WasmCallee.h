/*
 * Copyright (C) 2016-2017 Apple Inc. All rights reserved.
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

#pragma once

#if ENABLE(WEBASSEMBLY)

#include "JITCompilation.h"
#include "RegisterAtOffsetList.h"
#include "WasmCompilationMode.h"
#include "WasmFormat.h"
#include "WasmFunctionCodeBlock.h"
#include "WasmHandlerInfo.h"
#include "WasmIndexOrName.h"
#include "WasmLLIntTierUpCounter.h"
#include "WasmTierUpCount.h"
#include <wtf/FixedVector.h>
#include <wtf/RefCountedFixedVector.h>
#include <wtf/ThreadSafeRefCounted.h>

namespace JSC {

class LLIntOffsetsExtractor;

namespace Wasm {

#if ENABLE(WEBASSEMBLY_B3JIT)
class OMGForOSREntryCallee;
#endif

class Callee : public ThreadSafeRefCounted<Callee> {
    WTF_MAKE_FAST_ALLOCATED;

public:
    JS_EXPORT_PRIVATE virtual ~Callee();

    IndexOrName indexOrName() const { return m_indexOrName; }
    CompilationMode compilationMode() const { return m_compilationMode; }

    virtual MacroAssemblerCodePtr<WasmEntryPtrTag> entrypoint() const = 0;
    virtual RegisterAtOffsetList* calleeSaveRegisters() = 0;
    virtual std::tuple<void*, void*> range() const = 0;

    const HandlerInfo* handlerForIndex(Instance&, unsigned, const Tag*);

    bool hasExceptionHandlers() const { return !m_exceptionHandlers.isEmpty(); }

#if ENABLE(WEBASSEMBLY_B3JIT)
    virtual void setOSREntryCallee(Ref<OMGForOSREntryCallee>&&, MemoryMode)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }
#endif

    void dump(PrintStream&) const;

    virtual FunctionCodeBlock* llintFunctionCodeBlock() const { return nullptr; }

protected:
    JS_EXPORT_PRIVATE Callee(Wasm::CompilationMode);
    JS_EXPORT_PRIVATE Callee(Wasm::CompilationMode, size_t, std::pair<const Name*, RefPtr<NameSection>>&&);

private:
    CompilationMode m_compilationMode;
    IndexOrName m_indexOrName;

protected:
    FixedVector<HandlerInfo> m_exceptionHandlers;
};

class JITCallee : public Callee {
public:
    MacroAssemblerCodePtr<WasmEntryPtrTag> entrypoint() const override { return m_entrypoint.compilation->code().retagged<WasmEntryPtrTag>(); }
    RegisterAtOffsetList* calleeSaveRegisters() override { return &m_entrypoint.calleeSaveRegisters; }
    FixedVector<UnlinkedWasmToWasmCall>& wasmToWasmCallsites() { return m_wasmToWasmCallsites; }

protected:
    JS_EXPORT_PRIVATE JITCallee(Wasm::CompilationMode, Wasm::Entrypoint&&);
    JS_EXPORT_PRIVATE JITCallee(Wasm::CompilationMode, Wasm::Entrypoint&&, size_t, std::pair<const Name*, RefPtr<NameSection>>&&, Vector<UnlinkedWasmToWasmCall>&&);

    std::tuple<void*, void*> range() const override
    {
        void* start = m_entrypoint.compilation->codeRef().executableMemory()->start().untaggedPtr();
        void* end = m_entrypoint.compilation->codeRef().executableMemory()->end().untaggedPtr();
        return { start, end };
    }

private:
    FixedVector<UnlinkedWasmToWasmCall> m_wasmToWasmCallsites;
    Wasm::Entrypoint m_entrypoint;
};

class EmbedderEntrypointCallee final : public JITCallee {
public:
    static Ref<EmbedderEntrypointCallee> create(Wasm::Entrypoint&& entrypoint)
    {
        return adoptRef(*new EmbedderEntrypointCallee(WTFMove(entrypoint)));
    }

private:
    EmbedderEntrypointCallee(Wasm::Entrypoint&& entrypoint)
        : JITCallee(Wasm::CompilationMode::EmbedderEntrypointMode, WTFMove(entrypoint))
    {
    }
};


#if ENABLE(WEBASSEMBLY_B3JIT)
class OptimizingJITCallee : public JITCallee {
public:
    const StackMap& stackmap(CallSiteIndex) const;

protected:
    OptimizingJITCallee(Wasm::CompilationMode mode, Wasm::Entrypoint&& entrypoint, size_t index, std::pair<const Name*, RefPtr<NameSection>>&& name, Vector<UnlinkedWasmToWasmCall>&& unlinkedCalls, StackMaps&& stackmaps, Vector<UnlinkedHandlerInfo>&& unlinkedExceptionHandlers, Vector<CodeLocationLabel<ExceptionHandlerPtrTag>>&& exceptionHandlerLocations)
        : JITCallee(mode, WTFMove(entrypoint), index, WTFMove(name), WTFMove(unlinkedCalls))
        , m_stackmaps(WTFMove(stackmaps))
    {
        RELEASE_ASSERT(unlinkedExceptionHandlers.size() == exceptionHandlerLocations.size());
        linkExceptionHandlers(WTFMove(unlinkedExceptionHandlers), WTFMove(exceptionHandlerLocations));
    }

private:
    void linkExceptionHandlers(Vector<UnlinkedHandlerInfo>, Vector<CodeLocationLabel<ExceptionHandlerPtrTag>>);

    StackMaps m_stackmaps;
};

class OMGCallee final : public OptimizingJITCallee {
public:
    static Ref<OMGCallee> create(Wasm::Entrypoint&& entrypoint, size_t index, std::pair<const Name*, RefPtr<NameSection>>&& name, Vector<UnlinkedWasmToWasmCall>&& unlinkedCalls, StackMaps&& stackmaps, Vector<UnlinkedHandlerInfo>&& exceptionHandlers, Vector<CodeLocationLabel<ExceptionHandlerPtrTag>>&& exceptionHandlerLocations)
    {
        return adoptRef(*new OMGCallee(WTFMove(entrypoint), index, WTFMove(name), WTFMove(unlinkedCalls), WTFMove(stackmaps), WTFMove(exceptionHandlers), WTFMove(exceptionHandlerLocations)));
    }

private:
    OMGCallee(Wasm::Entrypoint&& entrypoint, size_t index, std::pair<const Name*, RefPtr<NameSection>>&& name, Vector<UnlinkedWasmToWasmCall>&& unlinkedCalls, StackMaps&& stackmaps, Vector<UnlinkedHandlerInfo>&& exceptionHandlers, Vector<CodeLocationLabel<ExceptionHandlerPtrTag>>&& exceptionHandlerLocations)
        : OptimizingJITCallee(Wasm::CompilationMode::OMGMode, WTFMove(entrypoint), index, WTFMove(name), WTFMove(unlinkedCalls), WTFMove(stackmaps), WTFMove(exceptionHandlers), WTFMove(exceptionHandlerLocations))
    {
    }
};

class OMGForOSREntryCallee final : public OptimizingJITCallee {
public:
    static Ref<OMGForOSREntryCallee> create(Wasm::Entrypoint&& entrypoint, size_t index, std::pair<const Name*, RefPtr<NameSection>>&& name, unsigned osrEntryScratchBufferSize, uint32_t loopIndex, Vector<UnlinkedWasmToWasmCall>&& unlinkedCalls, StackMaps&& stackmaps, Vector<UnlinkedHandlerInfo>&& exceptionHandlers, Vector<CodeLocationLabel<ExceptionHandlerPtrTag>>&& exceptionHandlerLocations)
    {
        return adoptRef(*new OMGForOSREntryCallee(WTFMove(entrypoint), index, WTFMove(name), osrEntryScratchBufferSize, loopIndex, WTFMove(unlinkedCalls), WTFMove(stackmaps), WTFMove(exceptionHandlers), WTFMove(exceptionHandlerLocations)));
    }

    unsigned osrEntryScratchBufferSize() const { return m_osrEntryScratchBufferSize; }
    uint32_t loopIndex() const { return m_loopIndex; }

private:
    OMGForOSREntryCallee(Wasm::Entrypoint&& entrypoint, size_t index, std::pair<const Name*, RefPtr<NameSection>>&& name, unsigned osrEntryScratchBufferSize, uint32_t loopIndex, Vector<UnlinkedWasmToWasmCall>&& unlinkedCalls, StackMaps&& stackmaps, Vector<UnlinkedHandlerInfo>&& exceptionHandlers, Vector<CodeLocationLabel<ExceptionHandlerPtrTag>>&& exceptionHandlerLocations)
        : OptimizingJITCallee(Wasm::CompilationMode::OMGForOSREntryMode, WTFMove(entrypoint), index, WTFMove(name), WTFMove(unlinkedCalls), WTFMove(stackmaps), WTFMove(exceptionHandlers), WTFMove(exceptionHandlerLocations))
        , m_osrEntryScratchBufferSize(osrEntryScratchBufferSize)
        , m_loopIndex(loopIndex)
    {
    }

    unsigned m_osrEntryScratchBufferSize;
    uint32_t m_loopIndex;
};

class BBQCallee final : public OptimizingJITCallee {
public:
    static Ref<BBQCallee> create(Wasm::Entrypoint&& entrypoint, size_t index, std::pair<const Name*, RefPtr<NameSection>>&& name, std::unique_ptr<TierUpCount>&& tierUpCount, Vector<UnlinkedWasmToWasmCall>&& unlinkedCalls, StackMaps&& stackmaps, Vector<UnlinkedHandlerInfo>&& exceptionHandlers, Vector<CodeLocationLabel<ExceptionHandlerPtrTag>>&& exceptionHandlerLocations)
    {
        return adoptRef(*new BBQCallee(WTFMove(entrypoint), index, WTFMove(name), WTFMove(tierUpCount), WTFMove(unlinkedCalls), WTFMove(stackmaps), WTFMove(exceptionHandlers), WTFMove(exceptionHandlerLocations)));
    }

    OMGForOSREntryCallee* osrEntryCallee() { return m_osrEntryCallee.get(); }
    void setOSREntryCallee(Ref<OMGForOSREntryCallee>&& osrEntryCallee, MemoryMode) final
    {
        m_osrEntryCallee = WTFMove(osrEntryCallee);
    }

    bool didStartCompilingOSREntryCallee() const { return m_didStartCompilingOSREntryCallee; }
    void setDidStartCompilingOSREntryCallee(bool value) { m_didStartCompilingOSREntryCallee = value; }

    OMGCallee* replacement() { return m_replacement.get(); }
    void setReplacement(Ref<OMGCallee>&& replacement)
    {
        m_replacement = WTFMove(replacement);
    }

    TierUpCount* tierUpCount() { return m_tierUpCount.get(); }

private:
    BBQCallee(Wasm::Entrypoint&& entrypoint, size_t index, std::pair<const Name*, RefPtr<NameSection>>&& name, std::unique_ptr<TierUpCount>&& tierUpCount, Vector<UnlinkedWasmToWasmCall>&& unlinkedCalls, StackMaps&& stackmaps, Vector<UnlinkedHandlerInfo>&& exceptionHandlers, Vector<CodeLocationLabel<ExceptionHandlerPtrTag>>&& exceptionHandlerLocations)
        : OptimizingJITCallee(Wasm::CompilationMode::BBQMode, WTFMove(entrypoint), index, WTFMove(name), WTFMove(unlinkedCalls), WTFMove(stackmaps), WTFMove(exceptionHandlers), WTFMove(exceptionHandlerLocations))
        , m_tierUpCount(WTFMove(tierUpCount))
    {
    }

    RefPtr<OMGForOSREntryCallee> m_osrEntryCallee;
    RefPtr<OMGCallee> m_replacement;
    std::unique_ptr<TierUpCount> m_tierUpCount;
    bool m_didStartCompilingOSREntryCallee { false };
};
#endif

class LLIntCallee final : public Callee {
    friend LLIntOffsetsExtractor;

public:
    static Ref<LLIntCallee> create(std::unique_ptr<FunctionCodeBlock> codeBlock, size_t index, std::pair<const Name*, RefPtr<NameSection>>&& name)
    {
        return adoptRef(*new LLIntCallee(WTFMove(codeBlock), index, WTFMove(name)));
    }

    JS_EXPORT_PRIVATE void setEntrypoint(MacroAssemblerCodePtr<WasmEntryPtrTag>);
    JS_EXPORT_PRIVATE MacroAssemblerCodePtr<WasmEntryPtrTag> entrypoint() const final;
    JS_EXPORT_PRIVATE RegisterAtOffsetList* calleeSaveRegisters() final;
    JS_EXPORT_PRIVATE std::tuple<void*, void*> range() const final;

#if ENABLE(WEBASSEMBLY_B3JIT)
    JITCallee* replacement(MemoryMode mode) { return m_replacements[static_cast<uint8_t>(mode)].get(); }
    void setReplacement(Ref<JITCallee>&& replacement, MemoryMode mode)
    {
        m_replacements[static_cast<uint8_t>(mode)] = WTFMove(replacement);
    }

    OMGForOSREntryCallee* osrEntryCallee(MemoryMode mode) { return m_osrEntryCallees[static_cast<uint8_t>(mode)].get(); }
    void setOSREntryCallee(Ref<OMGForOSREntryCallee>&& osrEntryCallee, MemoryMode mode) final
    {
        m_osrEntryCallees[static_cast<uint8_t>(mode)] = WTFMove(osrEntryCallee);
    }

    LLIntTierUpCounter& tierUpCounter() { return m_codeBlock->tierUpCounter(); }
    FunctionCodeBlock* llintFunctionCodeBlock() const final { return m_codeBlock.get(); }
#endif

private:
    LLIntCallee(std::unique_ptr<FunctionCodeBlock>, size_t index, std::pair<const Name*, RefPtr<NameSection>>&&);

    void linkExceptionHandlers();

#if ENABLE(WEBASSEMBLY_B3JIT)
    RefPtr<JITCallee> m_replacements[Wasm::NumberOfMemoryModes];
    RefPtr<OMGForOSREntryCallee> m_osrEntryCallees[Wasm::NumberOfMemoryModes];
#endif
    std::unique_ptr<FunctionCodeBlock> m_codeBlock;
    MacroAssemblerCodePtr<WasmEntryPtrTag> m_entrypoint;
};

using LLIntCallees = ThreadSafeRefCountedFixedVector<Ref<LLIntCallee>>;

} } // namespace JSC::Wasm

#endif // ENABLE(WEBASSEMBLY)
