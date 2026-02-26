/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2026 RDK Management
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
**/

#include "UnifiedCASManagement.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 2
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework
{

namespace
{
    static Plugin::Metadata<Plugin::UnifiedCASManagement> metadata(
        // Version (Major, Minor, Patch)
        API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
        // Preconditions
        {},
        // Terminations
        {},
        // Controls
        {});
}

namespace Plugin
{

SERVICE_REGISTRATION(UnifiedCASManagement, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

UnifiedCASManagement::UnifiedCASManagement()
    : _service(nullptr)
    , _connectionId(0)
    , _UnifiedCASManagement(nullptr)
    , _UnifiedCASManagementNotification(this)
{
    SYSLOG(Logging::Startup, (_T("UnifiedCASManagement Constructor")));
}

UnifiedCASManagement::~UnifiedCASManagement()
{
    SYSLOG(Logging::Shutdown, (string(_T("UnifiedCASManagement Destructor"))));
}

const string UnifiedCASManagement::Initialize(PluginHost::IShell* service)
{
    string message = "";

    ASSERT(nullptr != service);
    ASSERT(nullptr == _service);
    ASSERT(nullptr == _UnifiedCASManagement);
    ASSERT(0 == _connectionId);

    SYSLOG(Logging::Startup, (_T("UnifiedCASManagement::Initialize: PID=%u"), getpid()));

    _service = service;
    _service->AddRef();
    _service->Register(&_UnifiedCASManagementNotification);
    _UnifiedCASManagement = _service->Root<Exchange::IUnifiedCASManagement>(_connectionId, 5000, _T("UnifiedCASManagementImplementation"));

    if (nullptr != _UnifiedCASManagement)
    {
        // Register for notifications
        _UnifiedCASManagement->Register(&_UnifiedCASManagementNotification);
        // Invoking Plugin API register to wpeframework
        Exchange::JUnifiedCASManagement::Register(*this, _UnifiedCASManagement);
    }
    else
    {
        SYSLOG(Logging::Startup, (_T("UnifiedCASManagement::Initialize: Failed to initialise UnifiedCASManagement plugin")));
        message = _T("UnifiedCASManagement plugin could not be initialised. Failed to instantiate UnifiedCASManagementImplementation");
    }

    return message;
}

void UnifiedCASManagement::Deinitialize(PluginHost::IShell* service)
{
    ASSERT(_service == service);

    SYSLOG(Logging::Shutdown, (string(_T("UnifiedCASManagement::Deinitialize"))));

    if (nullptr != _UnifiedCASManagement)
    {
        _UnifiedCASManagement->Unregister(&_UnifiedCASManagementNotification);
        Exchange::JUnifiedCASManagement::Unregister(*this);

        RPC::IRemoteConnection* connection = service->RemoteConnection(_connectionId);
        VARIABLE_IS_NOT_USED uint32_t result = _UnifiedCASManagement->Release();
        _UnifiedCASManagement = nullptr;

        ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

        if (connection != nullptr)
        {
            try
            {
                connection->Terminate();
                TRACE(Trace::Warning, (_T("Connection terminated successfully")));
            }
            catch (const std::exception& e)
            {
                std::string errorMessage = "Failed to terminate connection: ";
                errorMessage += e.what();
                TRACE(Trace::Warning, (_T("%s"), errorMessage.c_str()));
            }
            connection->Release();
        }
    }

    if (_service != nullptr)
    {
        _service->Unregister(&_UnifiedCASManagementNotification);
        _service->Release();
        _service = nullptr;
    }

    _connectionId = 0;
    SYSLOG(Logging::Shutdown, (string(_T("UnifiedCASManagement de-initialised"))));
}

string UnifiedCASManagement::Information() const
{
    return "Plugin which exposes UnifiedCASManagement related methods and notifications.";
}

void UnifiedCASManagement::event_data(const string& payload, const Exchange::IUnifiedCASManagement::DataSource& source)
{
    // Forward the event to JSON-RPC clients through the notification system
    _UnifiedCASManagementNotification.OnDataReceived(payload, source);
}

void UnifiedCASManagement::Deactivated(RPC::IRemoteConnection* connection)
{
    if (connection->Id() == _connectionId) {
        ASSERT(nullptr != _service);
        Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
    }
}

} // namespace

} // namespace
