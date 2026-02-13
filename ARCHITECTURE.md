# UnifiedCASManagement Plugin Architecture

## Overview

The UnifiedCASManagement plugin is a WPEFramework (Thunder) plugin that provides a unified interface for managing Conditional Access System (CAS) sessions in RDK environments. It abstracts CAS operations and provides a JSON-RPC based API for client applications to manage CAS sessions across different media playback scenarios.

## System Architecture

### Component Structure

```
┌─────────────────────────────────────────────────────┐
│          Client Applications (JSON-RPC)             │
└──────────────────┬──────────────────────────────────┘
                   │
                   ▼
┌─────────────────────────────────────────────────────┐
│    UnifiedCASManagement Plugin (Thunder Plugin)     │
│  ┌───────────────────────────────────────────────┐  │
│  │     JSON-RPC Interface Layer                  │  │
│  │  - manage()   - unmanage()   - send()        │  │
│  └───────────────────┬───────────────────────────┘  │
│                      │                               │
│  ┌───────────────────▼───────────────────────────┐  │
│  │     MediaPlayer Abstract Interface            │  │
│  │  - openMediaPlayer()                          │  │
│  │  - closeMediaPlayer()                         │  │
│  │  - requestCASData()                           │  │
│  └───────────────────┬───────────────────────────┘  │
│                      │                               │
│  ┌───────────────────▼───────────────────────────┐  │
│  │   LibMediaPlayerImpl (Implementation)         │  │
│  │  - libmediaplayer integration                 │  │
│  │  - Event/Error callback handling              │  │
│  └───────────────────┬───────────────────────────┘  │
└────────────────────┬─┴───────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────┐
│         libmediaplayer (Native Library)             │
│  - CAS session management                           │
│  - OCDM CAS plugin interaction                      │
└─────────────────────────────────────────────────────┘
```

### Core Components

#### 1. UnifiedCASManagement Plugin Class
- **Role**: Main plugin entry point implementing Thunder's IPlugin interface
- **Responsibilities**:
  - Plugin lifecycle management (Initialize, Deinitialize)
  - JSON-RPC method registration and dispatching
  - Event notification to clients
  - Singleton instance management for callback handling

#### 2. MediaPlayer Abstract Interface
- **Role**: Abstraction layer for media player implementations
- **Purpose**: Decouples CAS management logic from specific player implementations
- **Operations**:
  - `openMediaPlayer()`: Initializes CAS session with specified parameters
  - `closeMediaPlayer()`: Terminates active CAS session
  - `requestCASData()`: Sends CAS-specific commands to OCDM plugin

#### 3. LibMediaPlayerImpl
- **Role**: Concrete implementation using libmediaplayer
- **Responsibilities**:
  - Native libmediaplayer API integration
  - Callback registration for events and errors
  - Data marshalling between plugin and native library
  - Notification forwarding to UnifiedCASManagement service

## Data Flow

### CAS Session Management Flow

1. **Session Initialization (manage)**:
   ```
   Client → JSON-RPC → UnifiedCASManagement::manage()
     ↓
   Validate parameters (mode, manage type, casocdmid)
     ↓
   LibMediaPlayerImpl::openMediaPlayer()
     ↓
   libmediaplayer creates CAS session with OCDM
     ↓
   Response returned to client
   ```

2. **Data Exchange (send)**:
   ```
   Client → JSON-RPC → UnifiedCASManagement::send()
     ↓
   LibMediaPlayerImpl::requestCASData()
     ↓
   Forward data to OCDM CAS plugin via libmediaplayer
     ↓
   Response returned to client
   ```

3. **Session Termination (unmanage)**:
   ```
   Client → JSON-RPC → UnifiedCASManagement::unmanage()
     ↓
   LibMediaPlayerImpl::closeMediaPlayer()
     ↓
   libmediaplayer destroys CAS session
     ↓
   Response returned to client
   ```

4. **Event Notifications**:
   ```
   OCDM/libmediaplayer → eventCallBack() → LibMediaPlayerImpl
     ↓
   UnifiedCASManagement::event_data()
     ↓
   JSON-RPC Event → Client
   ```

## Plugin Framework Integration

### WPEFramework (Thunder) Integration
- Implements `PluginHost::IPlugin` for lifecycle management
- Implements `PluginHost::JSONRPC` for JSON-RPC communication
- Uses Thunder's service registration mechanism: `SERVICE_REGISTRATION(UnifiedCASManagement, 1, 0)`
- Leverages Thunder's interface mapping for polymorphic behavior

### Configuration
- Plugin configuration via `UnifiedCASManagement.config` file
- Build-time configuration through CMake options
- Runtime parameters passed through JSON-RPC API

## Dependencies and Interfaces

### External Dependencies
- **libmediaplayer**: Native media player library for CAS session management
- **IARM Bus**: Inter-process communication (optional, for system integration)
- **WPEFramework**: Thunder plugin framework

### Internal Utilities
- **UtilsJsonRpc**: JSON-RPC helper utilities for parameter validation
- **UtilsCStr**: C-string conversion utilities
- **UtilsIarm**: IARM bus communication helpers

### API Interfaces
- **JSON-RPC Methods**: `manage`, `unmanage`, `send`
- **JSON-RPC Events**: `data` (for asynchronous notifications)
- **Parameters**: Supports mode selection, management levels (FULL, NO_PSI, NO_TUNER), OCDM ID, initialization data

## Technical Implementation Details

### Design Patterns
- **Singleton Pattern**: Single instance management for callback coordination
- **Abstract Factory**: MediaPlayer interface with concrete LibMediaPlayerImpl
- **Observer Pattern**: Event callbacks from libmediaplayer to plugin

### Threading Model
- JSON-RPC calls execute in Thunder framework threads
- Callbacks from libmediaplayer may execute in separate threads
- Event notifications are marshalled through Thunder's event system

### Error Handling
- Parameter validation with detailed error messages
- Graceful handling of missing player implementations
- Error propagation through JSON-RPC response codes

### Memory Management
- Smart pointers (`std::shared_ptr`) for MediaPlayer instance
- RAII principles for resource cleanup
- Proper cleanup in destructor and Deinitialize()

## Build Configuration

### CMake Integration
- Plugin name: `UnifiedCASManagement`
- Required packages: WPEFramework, IARMBus, LMPLAYER
- Test framework integration for L1/L2 testing
- Optional features controlled via CMake options

### Compilation
- C++17 standard required
- Conditional compilation based on LMPLAYER availability
- Platform-specific definitions (RT_PLATFORM_LINUX)
