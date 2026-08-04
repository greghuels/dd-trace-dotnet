// Unless explicitly stated otherwise all files in this repository are licensed under the Apache 2 License.
// This product includes software developed at Datadog (https://www.datadoghq.com/). Copyright 2022 Datadog, Inc.

#pragma once

// from dotnet coreclr includes
#include "cor.h"
#include "corprof.h"
// end

#include <memory>
#include <unordered_map>

#include "BclEventsParser.h"
#include "ClrEventsParser.h"
#include "DotnetEventsProvider.h"


class IAllocationsListener;
class IContentionListener;
class IGCSuspensionsListener;
class IGarbageCollectionsListener;
class INetworkListener;
class IConfiguration;

class EventPipeEventsManager
{
public:
    EventPipeEventsManager(ICorProfilerInfo12* pCorProfilerInfo,
                           IAllocationsListener* pAllocationListener,
                           IContentionListener* pContentionListener,
                           IGCSuspensionsListener* pGCSuspensionsListener,
                           INetworkListener* pNetworkListener,
                           IConfiguration* pConfiguration,
                           IGCDumpListener* pGCDumpListener);
    void Register(IGarbageCollectionsListener* pGarbageCollectionsListener);
    void ParseEvent(EVENTPIPE_PROVIDER provider,
                    DWORD eventId,
                    DWORD eventVersion,
                    ULONG cbMetadataBlob,
                    LPCBYTE metadataBlob,
                    ULONG cbEventData,
                    LPCBYTE eventData,
                    LPCGUID pActivityId,
                    LPCGUID pRelatedActivityId,
                    ThreadID eventThread,
                    ULONG numStackFrames,
                    UINT_PTR stackFrames[]);

private:
    bool TryGetEventInfo(
        LPCBYTE pMetadata,
        ULONG cbMetadata,
        WCHAR*& name,
        DWORD& id,
        INT64& keywords,
        DWORD& version
        );

    // Resolves (and caches) the provider that emitted an event. EventPipeGetProviderInfo
    // copies the provider name on every call and, combined with the string comparisons,
    // was paid on every event on the EventPipe processing thread. Events are delivered
    // serially on that single thread, so an unsynchronized cache keyed by the (stable)
    // provider pointer is safe and removes that per-event cost.
    DotnetEventsProvider GetProviderType(EVENTPIPE_PROVIDER provider);


private:
    // We only ever subscribe to a handful of providers (<= 6). Providers can in
    // principle be re-created over the process lifetime (e.g. EventPipe session
    // restarts), so cap the cache to keep it bounded; if the cap is ever reached
    // the cache is dropped and entries are re-resolved lazily.
    static constexpr size_t MaxCachedProviders = 64;

    ICorProfilerInfo12* _pCorProfilerInfo;
    std::unique_ptr<ClrEventsParser> _clrParser;
    std::unique_ptr<BclEventsParser> _bclParser;
    std::unordered_map<EVENTPIPE_PROVIDER, DotnetEventsProvider> _providerTypes;
};
