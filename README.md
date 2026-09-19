# STAGE VK

**STAGE VK** is a Vulkan-based game engine and editor being built from scratch in **C++20**. The project focuses on low-level graphics programming, explicit GPU control, clean engine architecture, editor tooling, profiling, and the gradual development of a complete game-development environment.

The project is currently in active development. Its present foundation already covers Vulkan initialization and frame execution, swapchain management and recreation, synchronization, multiple frames in flight, renderer orchestration, Dear ImGui integration, structured logging, CPU/GPU profiling, frame pacing, and automated CI builds.

Current high-level architecture:

```text
Application
├── Window
├── FramePacer
├── Profiler
├── VulkanModule
│   ├── VulkanDevice
│   ├── VulkanSwapchain
│   ├── VulkanCommandContext
│   ├── FrameContext
│   └── VulkanGpuProfiler
├── RenderModule
│   ├── Main VkRenderPass
│   ├── Swapchain Framebuffers
│   └── ImGuiPass
└── EditorModule
    ├── Console
    └── Performance Profiler
```

### Current features

- Vulkan 1.2 instance, surface, physical-device and logical-device setup
- Graphics and presentation queue discovery
- Swapchain creation and resize-safe recreation
- Command pool and primary command-buffer management
- Two frames in flight
- CPU/GPU synchronization with semaphores and fences
- Explicit acquire → record → submit → present frame lifecycle
- Swapchain listener system for dependent renderer resources
- Dedicated `RenderModule` for renderer-level orchestration
- Dear ImGui integration through GLFW + Vulkan backends
- Initial `EditorModule` foundation
- Structured logging with `spdlog`
- In-engine console
- CPU frame profiling with hierarchical scopes
- Vulkan GPU timestamp profiling
- Frame history and individual frame inspection
- Rendering statistics
- Monitor-aware frame pacing
- GitHub Actions CI for Debug and Release x64 builds
- Debug validation layers through `VK_LAYER_KHRONOS_validation`

> STAGE VK is still an early-stage engine project. The current focus is on building a strong low-level foundation before expanding the engine further.

---

## Badges

![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![Vulkan](https://img.shields.io/badge/Vulkan-1.2-AC162C?logo=vulkan&logoColor=white)
![Platform](https://img.shields.io/badge/platform-Windows-0078D4?logo=windows&logoColor=white)
![Visual Studio](https://img.shields.io/badge/Visual%20Studio-2022-5C2D91?logo=visualstudio&logoColor=white)
![Status](https://img.shields.io/badge/status-in%20development-orange)

<!-- Replace the placeholders below with your GitHub username and repository name. -->
[![STAGE VK CI](https://github.com/<GITHUB_USERNAME>/<REPOSITORY_NAME>/actions/workflows/build.yml/badge.svg)](https://github.com/<GITHUB_USERNAME>/<REPOSITORY_NAME>/actions/workflows/build.yml)

---

## Visual Demo

<p align="center">
  <img src="Docs/Images/STAGE_VK_Editor.png" alt="STAGE VK Editor" width="900">
</p>

> Add the latest editor screenshot or GIF as `Docs/Images/STAGE_VK_Editor.png`, or replace the path above with the final image used in the repository.

---

## Table of Contents

- [STAGE VK](#stage-vk)
- [Badges](#badges)
- [Visual Demo](#visual-demo)
- [Technologies Used](#technologies-used)
- [Installation and Prerequisites](#installation-and-prerequisites)
- [Usage](#usage)
- [Continuous Integration](#continuous-integration)
- [Contributing](#contributing)
- [License](#license)

---

## Technologies Used

| Technology | Purpose |
|---|---|
| **C++20** | Core engine implementation |
| **Vulkan 1.2** | Low-level graphics API and GPU execution |
| **GLFW** | Window creation, platform integration and Vulkan surface support |
| **Dear ImGui** | Immediate-mode editor interface |
| **ImGui GLFW Backend** | Dear ImGui platform integration |
| **ImGui Vulkan Backend** | Dear ImGui Vulkan rendering integration |
| **GLM** | Mathematics utilities |
| **spdlog** | Structured engine logging |
| **Font Awesome** | Editor icons |
| **Vulkan Validation Layers** | Debug-time API validation and diagnostics |
| **Visual Studio 2022** | Main Windows development environment |
| **MSBuild** | Project build system |
| **Git / GitHub** | Version control and repository hosting |
| **GitHub Actions** | Automated CI build validation |

---

## Installation and Prerequisites

### Prerequisites

STAGE VK currently targets **Windows x64** and is developed with **Visual Studio 2022**.

You will need:

- Windows 10 or Windows 11, 64-bit
- Visual Studio 2022
  - Install the **Desktop development with C++** workload
  - C++20-capable MSVC toolchain
- A recent **Vulkan SDK**
- Vulkan validation layers for Debug builds
- A GPU and graphics driver with **Vulkan 1.2** support
- Git

The project also uses **GLFW**, **GLM**, **Dear ImGui**, **spdlog**, and **Font Awesome**. Project-side dependencies are included according to the Visual Studio project configuration.

### Clone the repository

```bash
git clone <REPOSITORY_URL>
cd STAGE-VK
```

Replace `<REPOSITORY_URL>` with the final GitHub repository URL.

### Vulkan SDK

Install the Vulkan SDK from LunarG:

https://vulkan.lunarg.com/

The Visual Studio project uses the `VULKAN_SDK` environment variable for the Vulkan include and library paths.

### Build with Visual Studio

1. Open `STAGE_VK/STAGE_VK.sln` in Visual Studio 2022.
2. Select an `x64` configuration.
3. Use `Debug` while developing to enable Vulkan validation layers.
4. Set `STAGE_VK` as the startup project if Visual Studio has not already done so.
5. Build the solution with **Build → Build Solution** or `Ctrl + Shift + B`.
6. Run with `F5` or `Ctrl + F5`.

Supported configurations:

```text
Debug | x64
Release | x64
```

If initialization succeeds, STAGE VK creates a resizable Vulkan window and enters the engine frame loop.

---

## Usage

The current executable starts the engine and editor application in a resizable window.

The frame is currently organized approximately as follows:

```text
Application
    ↓
Poll Events
Window Handling
Update
    ↓
VulkanModule::PreRender()
    ↓
Wait for current frame slot
Resolve GPU profiler timestamps
Acquire swapchain image
Reset / begin current command buffer
    ↓
RenderModule::PreRender()
    ↓
Start Dear ImGui frame
    ↓
RenderModule::Render()
    ↓
Begin main Vulkan render pass
Record rendering work
    ↓
EditorModule::Render()
    ↓
Build editor UI
Console
Performance Profiler
    ↓
RenderModule::PostRender()
    ↓
Record ImGui draw data
End render pass
    ↓
VulkanModule::PostRender()
    ↓
End command buffer
Submit to graphics queue
Present swapchain image
Advance frame slot
```

STAGE VK currently uses **two frames in flight**. Each frame slot owns its own primary command buffer, image-available semaphore, and in-flight fence.

The editor currently exposes an in-engine console and performance profiler. The profiler tracks frame time, CPU time, GPU time, frame history, hierarchical CPU scopes, Vulkan timestamp results, and rendering statistics.

The profiler has already been used to diagnose a real frame-pacing issue involving blocking inside `vkAcquireNextImageKHR()`. The investigation led to the introduction of multiple frames in flight and monitor-aware frame pacing.

Window resizing triggers swapchain recreation together with rebuilding the renderer resources that depend on it.

---

## Continuous Integration

STAGE VK uses **GitHub Actions** for automated build validation.

Every push and pull request targeting `main` runs the project through a clean Windows build environment.

The current workflow performs:

```text
Checkout Repository
        ↓
Setup MSBuild
        ↓
Install Vulkan SDK
        ↓
Build Debug x64
        ↓
Build Release x64
        ↓
Pass / Fail
```

This verifies that both supported configurations compile successfully outside the local development machine and helps detect missing dependencies or broken project configuration early.

The workflow is defined in:

```text
.github/workflows/build.yml
```

---

## Feedback, Audits and Suggestions

STAGE VK is currently developed as a personal engine project, so external code contributions and pull requests are not being accepted.

However, technical feedback is very welcome.

I am especially open to:

- Vulkan API reviews
- Rendering architecture audits
- Synchronization feedback
- Performance suggestions
- Code quality reviews
- Engine design suggestions
- Bug reports
- General technical discussion

If you notice something that could be improved, feel free to open an issue with your findings or suggestions.

---

## License

STAGE VK is licensed under the **MIT License**.

Copyright (c) 2026 Daniel Bellido

See the [`LICENSE`](LICENSE) file for the full license text.
