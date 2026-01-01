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

#include "LinearPlaybackControlImplementation.h"
#include <inttypes.h>
#include <fstream>
#include <sstream>

namespace WPEFramework
{
    namespace Plugin
    {
        SERVICE_REGISTRATION(LinearPlaybackControlImplementation, 1, 0);

        LinearPlaybackControlImplementation* LinearPlaybackControlImplementation::_instance = nullptr;

        LinearPlaybackControlImplementation::LinearPlaybackControlImplementation()
            : _adminLock()
            , _linearPlaybackControlNotification()
            , _mountPoint("/mnt/streamfs")
            , _isStreamFSEnabled(false)
            , _demuxer(nullptr)
            , _trickPlayFileListener(nullptr)
            , m_callMutex()
        {
            LinearPlaybackControlImplementation::_instance = this;
            
            // Initialize configuration
            LinearConfig::Config config;
            config.MountPoint = Core::JSON::String(_mountPoint);
            config.IsStreamFSEnabled = Core::JSON::Boolean(_isStreamFSEnabled);
            
            if (_isStreamFSEnabled) {
                // For now we only have one Nokia FCC demuxer interface with demux Id = 0
                _demuxer = std::unique_ptr<DemuxerStreamFsFCC>(new DemuxerStreamFsFCC(&config, 0));
                _trickPlayFileListener = std::unique_ptr<FileSelectListener>(
                    new FileSelectListener(_demuxer->getTrickPlayFile(), 16, 
                        [this](const std::string &data) {
                            speedChangedCallback(data);
                        }
                    )
                );
            }
        }

        LinearPlaybackControlImplementation::~LinearPlaybackControlImplementation()
        {
            // Clean up file listener and demuxer
            _trickPlayFileListener.reset();
            _demuxer.reset();
        }

        LinearPlaybackControlImplementation* LinearPlaybackControlImplementation::instance(LinearPlaybackControlImplementation* impl)
        {
            if (impl != nullptr) {
                _instance = impl;
            }
            return _instance;
        }

        /************************************** Registration ****************************************/

        Core::hresult LinearPlaybackControlImplementation::Register(Exchange::ILinearPlaybackControl::INotification* notification)
        {
            Core::hresult status = Core::ERROR_NONE;
            ASSERT(nullptr != notification);
            _adminLock.Lock();

            // Check if the notification is already registered
            if (std::find(_linearPlaybackControlNotification.begin(), _linearPlaybackControlNotification.end(), notification) != _linearPlaybackControlNotification.end())
            {
                syslog(LOG_ERR, "Same notification is registered already");
                status = Core::ERROR_ALREADY_CONNECTED;
            }
            else
            {
                _linearPlaybackControlNotification.push_back(notification);
                notification->AddRef();
            }

            _adminLock.Unlock();
            return status;
        }

        Core::hresult LinearPlaybackControlImplementation::Unregister(Exchange::ILinearPlaybackControl::INotification* notification)
        {
            Core::hresult status = Core::ERROR_GENERAL;
            ASSERT(nullptr != notification);
            _adminLock.Lock();

            // Just unregister one notification once
            auto itr = std::find(_linearPlaybackControlNotification.begin(), _linearPlaybackControlNotification.end(), notification);
            if (itr != _linearPlaybackControlNotification.end())
            {
                (*itr)->Release();
                _linearPlaybackControlNotification.erase(itr);
                status = Core::ERROR_NONE;
            }
            else
            {
                syslog(LOG_ERR, "Notification %p not found in _linearPlaybackControlNotification", notification);
            }

            _adminLock.Unlock();
            return status;
        }

        /************************************** Helper Methods ****************************************/

        uint32_t LinearPlaybackControlImplementation::DmxStatusToCoreStatus(IDemuxer::IO_STATUS status)
        {
            switch (status)
            {
                case DemuxerStreamFsFCC::OK:
                    return Core::ERROR_NONE;
                case DemuxerStreamFsFCC::WRITE_ERROR:
                    syslog(LOG_ERR, "Demuxer failed with error: %d", status);
                    return Core::ERROR_WRITE_ERROR;
                default:
                    syslog(LOG_ERR, "Demuxer failed with error: %d", status);
                    return Core::ERROR_READ_ERROR;
            }
        }

        uint32_t LinearPlaybackControlImplementation::callDemuxer(const string& demuxerId, const std::function<uint32_t(IDemuxer*)>& func)
        {
            if (!_isStreamFSEnabled) {
                syslog(LOG_ERR, "No demuxer set");
                return Core::ERROR_BAD_REQUEST;
            }
            
            if (!_demuxer) {
                syslog(LOG_ERR, "Demuxer not initialized");
                return Core::ERROR_BAD_REQUEST;
            }
            // In a future implementation, the concrete demuxer instance shall be obtained based
            // on the demuxerId. For now we just use the _demuxer instance, associated with
            // demuxId = 0, when invoking the lambda function for interacting with the instance.
            return func(_demuxer.get());
        }

        /************************************** Notifications ****************************************/

        void LinearPlaybackControlImplementation::dispatchSpeedchanged(const int16_t speed, const uint8_t muxId)
        {
            std::list<Exchange::ILinearPlaybackControl::INotification*>::const_iterator index(_linearPlaybackControlNotification.begin());
            while (index != _linearPlaybackControlNotification.end())
            {
                (*index)->Speedchanged(speed, muxId);
                ++index;
            }
            syslog(LOG_DEBUG, "Dispatched speedchanged event: speed=%d, muxId=%d", speed, muxId);
        }

        void LinearPlaybackControlImplementation::speedChangedCallback(const std::string& data)
        {
            int16_t trickPlaySpeed = 0;
            std::istringstream is(data);
            is >> trickPlaySpeed;

            dispatchSpeedchanged(trickPlaySpeed, 0);
        }

        /************************************** API Methods ****************************************/

        // Set channel URI for a demuxer
        Core::hresult LinearPlaybackControlImplementation::SetChannel(const string& demuxerId, const string& channel, uint32_t& result)
        {
            syslog(LOG_INFO, "Invoked LinearPlaybackControlImplementation::SetChannel");
            std::lock_guard<std::mutex> guard(m_callMutex);

            result = callDemuxer(demuxerId, [&](IDemuxer* dmx) -> uint32_t {
                return DmxStatusToCoreStatus(dmx->setChannel(channel));
            });

            return Core::ERROR_NONE;
        }

        // Get channel URI for a demuxer
        Core::hresult LinearPlaybackControlImplementation::GetChannel(const string& demuxerId, string& channel, uint32_t& result)
        {
            syslog(LOG_INFO, "Invoked LinearPlaybackControlImplementation::GetChannel");
            std::lock_guard<std::mutex> guard(m_callMutex);

            result = callDemuxer(demuxerId, [&](IDemuxer* dmx) -> uint32_t {
                std::string chan;
                auto status = DmxStatusToCoreStatus(dmx->getChannel(chan));
                channel = chan;
                return status;
            });

            return Core::ERROR_NONE;
        }

        // Set seek position
        Core::hresult LinearPlaybackControlImplementation::SetSeek(const string& demuxerId, const uint64_t seekPosInSeconds, uint32_t& result)
        {
            syslog(LOG_INFO, "Invoked LinearPlaybackControlImplementation::SetSeek");
            std::lock_guard<std::mutex> guard(m_callMutex);

            result = callDemuxer(demuxerId, [&](IDemuxer* dmx) -> uint32_t {
                return DmxStatusToCoreStatus(dmx->setSeekPosInSeconds(seekPosInSeconds));
            });

            return Core::ERROR_NONE;
        }

        // Get seek position
        Core::hresult LinearPlaybackControlImplementation::GetSeek(const string& demuxerId, uint64_t& seekPosInSeconds, uint32_t& result)
        {
            syslog(LOG_INFO, "Invoked LinearPlaybackControlImplementation::GetSeek");
            std::lock_guard<std::mutex> guard(m_callMutex);

            result = callDemuxer(demuxerId, [&](IDemuxer* dmx) -> uint32_t {
                IDemuxer::SeekType seek = {};
                auto status = DmxStatusToCoreStatus(dmx->getSeek(seek));
                if (status == Core::ERROR_NONE) {
                    seekPosInSeconds = seek.seekPosInSeconds;
                }
                return status;
            });

            return Core::ERROR_NONE;
        }

        // Set trick play speed
        Core::hresult LinearPlaybackControlImplementation::SetTrickPlay(const string& demuxerId, const int16_t speed, uint32_t& result)
        {
            syslog(LOG_INFO, "Invoked LinearPlaybackControlImplementation::SetTrickPlay");
            std::lock_guard<std::mutex> guard(m_callMutex);

            result = callDemuxer(demuxerId, [&](IDemuxer* dmx) -> uint32_t {
                return DmxStatusToCoreStatus(dmx->setTrickPlaySpeed(speed));
            });

            return Core::ERROR_NONE;
        }

        // Get trick play speed
        Core::hresult LinearPlaybackControlImplementation::GetTrickPlay(const string& demuxerId, int16_t& speed, uint32_t& result)
        {
            syslog(LOG_INFO, "Invoked LinearPlaybackControlImplementation::GetTrickPlay");
            std::lock_guard<std::mutex> guard(m_callMutex);

            result = callDemuxer(demuxerId, [&](IDemuxer* dmx) -> uint32_t {
                int16_t sp = 0;
                auto status = DmxStatusToCoreStatus(dmx->getTrickPlaySpeed(sp));
                if (status == Core::ERROR_NONE) {
                    speed = sp;
                }
                return status;
            });

            return Core::ERROR_NONE;
        }

        // Get status
        Core::hresult LinearPlaybackControlImplementation::GetStatus(const string& demuxerId,
                                                                     uint64_t& maxSizeInBytes,
                                                                     uint64_t& currentSizeInBytes,
                                                                     uint64_t& currentSizeInSeconds,
                                                                     uint64_t& seekPosInBytes,
                                                                     uint64_t& seekPosInSeconds,
                                                                     int16_t& trickPlaySpeed,
                                                                     bool& streamSourceLost,
                                                                     uint64_t& streamSourceLossCount,
                                                                     uint32_t& result)
        {
            syslog(LOG_INFO, "Invoked LinearPlaybackControlImplementation::GetStatus");
            std::lock_guard<std::mutex> guard(m_callMutex);

            result = callDemuxer(demuxerId, [&](IDemuxer* dmx) -> uint32_t {
                // Parameter declaration
                int16_t speed = 0;
                IDemuxer::SeekType seek = {};
                IDemuxer::StreamStatusType streamStatus = {};
                
                // Get parameters from selected demuxer.
                // Note: OR operation is used for concatenating the status since possible
                // set of status is ERROR_NONE (0) or ERROR_READ_ERROR (39)
                uint32_t status = DmxStatusToCoreStatus(dmx->getSeek(seek));
                status |= DmxStatusToCoreStatus(dmx->getTrickPlaySpeed(speed));
                status |= DmxStatusToCoreStatus(dmx->getStreamStatus(streamStatus));
                
                // Set parameters in response
                if (status == Core::ERROR_NONE) {
                    seekPosInSeconds = seek.seekPosInSeconds;
                    seekPosInBytes = seek.seekPosInBytes;
                    currentSizeInSeconds = seek.currentSizeInSeconds;
                    currentSizeInBytes = seek.currentSizeInBytes;
                    maxSizeInBytes = seek.maxSizeInBytes;
                    trickPlaySpeed = speed;
                    streamSourceLost = streamStatus.streamSourceLost;
                    streamSourceLossCount = streamStatus.streamSourceLossCount;
                }
                return status;
            });

            return Core::ERROR_NONE;
        }

        // Set tracing
        Core::hresult LinearPlaybackControlImplementation::SetTracing(const bool tracing, uint32_t& result)
        {
            syslog(LOG_INFO, "Invoked LinearPlaybackControlImplementation::SetTracing with tracing = %s", tracing ? "enabled" : "disabled");
            std::lock_guard<std::mutex> guard(m_callMutex);

            // The following stub implementation is added for testing only.
            result = Core::ERROR_NONE;
            return Core::ERROR_NONE;
        }

        // Get tracing
        Core::hresult LinearPlaybackControlImplementation::GetTracing(bool& tracing, uint32_t& result)
        {
            syslog(LOG_INFO, "Invoked LinearPlaybackControlImplementation::GetTracing");
            std::lock_guard<std::mutex> guard(m_callMutex);

            // The following stub implementation is added for testing only.
            tracing = true;
            result = Core::ERROR_NONE;
            return Core::ERROR_NONE;
        }

    } // namespace Plugin
} // namespace WPEFramework
