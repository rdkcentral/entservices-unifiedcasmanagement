/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <mutex>
#include "Module.h"
#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>
#include <interfaces/Ids.h>
#include <interfaces/ILinearPlaybackControl.h>
#include "tracing/Logging.h"
#include "DemuxerStreamFsFCC.h"
#include "FileSelectListener.h"
#include "LinearConfig.h"

namespace WPEFramework
{
    namespace Plugin
    {
        class LinearPlaybackControlImplementation : public Exchange::ILinearPlaybackControl
        {
        public:
            LinearPlaybackControlImplementation();
            ~LinearPlaybackControlImplementation() override;

            static LinearPlaybackControlImplementation* instance(LinearPlaybackControlImplementation* impl = nullptr);

            // We do not allow this plugin to be copied
            LinearPlaybackControlImplementation(const LinearPlaybackControlImplementation&) = delete;
            LinearPlaybackControlImplementation& operator=(const LinearPlaybackControlImplementation&) = delete;

            BEGIN_INTERFACE_MAP(LinearPlaybackControlImplementation)
                INTERFACE_ENTRY(Exchange::ILinearPlaybackControl)
            END_INTERFACE_MAP

        public:
            // ILinearPlaybackControl methods - Registration
            Core::hresult Register(Exchange::ILinearPlaybackControl::INotification* notification) override;
            Core::hresult Unregister(Exchange::ILinearPlaybackControl::INotification* notification) override;

            // ILinearPlaybackControl methods - Channel operations
            Core::hresult SetChannel(const string& demuxerId, const string& channel, uint32_t& result) override;
            Core::hresult GetChannel(const string& demuxerId, string& channel, uint32_t& result) override;

            // ILinearPlaybackControl methods - Seek operations
            Core::hresult SetSeek(const string& demuxerId, const uint64_t seekPosInSeconds, uint32_t& result) override;
            Core::hresult GetSeek(const string& demuxerId, uint64_t& seekPosInSeconds, uint32_t& result) override;

            // ILinearPlaybackControl methods - TrickPlay operations
            Core::hresult SetTrickPlay(const string& demuxerId, const int16_t speed, uint32_t& result) override;
            Core::hresult GetTrickPlay(const string& demuxerId, int16_t& speed, uint32_t& result) override;

            // ILinearPlaybackControl methods - Status operations
            Core::hresult GetStatus(const string& demuxerId,
                                   uint64_t& maxSizeInBytes,
                                   uint64_t& currentSizeInBytes,
                                   uint64_t& currentSizeInSeconds,
                                   uint64_t& seekPosInBytes,
                                   uint64_t& seekPosInSeconds,
                                   int16_t& trickPlaySpeed,
                                   bool& streamSourceLost,
                                   uint64_t& streamSourceLossCount,
                                   uint32_t& result) override;

            // ILinearPlaybackControl methods - Tracing operations
            Core::hresult SetTracing(const bool tracing, uint32_t& result) override;
            Core::hresult GetTracing(bool& tracing, uint32_t& result) override;

            // Public accessor for testing
            std::unique_ptr<DemuxerStreamFsFCC>& getDemuxer() { return _demuxer; }
            bool& getStreamFSEnabled() { return _isStreamFSEnabled; }

            static LinearPlaybackControlImplementation* _instance;

        private:
            // Helper methods
            uint32_t callDemuxer(const string& demuxerId, const std::function<uint32_t(IDemuxer*)>& func);
            uint32_t DmxStatusToCoreStatus(IDemuxer::IO_STATUS status);

            // Notification dispatchers
            void dispatchSpeedchanged(const int16_t speed, const uint8_t muxId);

            // File listener callback
            void speedChangedCallback(const std::string& data);

        private:
            mutable Core::CriticalSection _adminLock;
            std::list<Exchange::ILinearPlaybackControl::INotification*> _linearPlaybackControlNotification;

            std::string _mountPoint;
            bool _isStreamFSEnabled;
            std::unique_ptr<DemuxerStreamFsFCC> _demuxer;
            std::unique_ptr<FileSelectListener> _trickPlayFileListener;
            std::mutex m_callMutex;
        };
    } // namespace Plugin
} // namespace WPEFramework
