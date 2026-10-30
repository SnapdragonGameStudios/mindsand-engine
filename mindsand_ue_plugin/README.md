# MindSand

MindSand is an open-source framework for building real-time, context-aware behavior engines for AI-driven game characters.

## Overview

Provides a framework that gathers game and teammate context, sends it through an  LLM pipeline, and turns the response into executable action plans.
<br>
<br>
Enables NPC teammate behavior that responds to goals, world state, authored actions, and gameplay events.


## Features

- LLM-driven, goal-oriented teammate orchestration.
- Data-driven action authoring through Unreal primary data assets.
- Prompt construction from teammate, perception, actor, and plan state.
- Buffered JSON and streaming DSL action-plan parsing.
- Gameplay-task-based action execution with Blueprint support.
- Blueprint and editor plan injection for testing action plans without a live model.

## Dependencies
- Unreal Engine 5.6+
- [`LLMPipelines`](https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/5.6/Plugins/SGAI/SGAILLMPipelines) from [`Snapdragon GameAI SDK`](https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/5.6/Plugins/SGAI).

## Getting Started

### Downloading
Clone the GitHub repository:

   ```bash
   git clone --recurse-submodules https://github.com/SnapdragonGameStudios/mindsand-engine.git
   ```
   ```bash
   git clone --recurse-submodules -b engine/5.6 https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine.git
   ```

Place or link the plugin directory 
>`mindsand-engine/mindsand_ue_plugin/MindSand/` 

and

>`snapdragon-game-plugins-for-unreal-engine/Plugins/SGAI/`

under your Unreal project's `Plugins/` directory. <br>

### Quick Start
Follow steps at [QUICKSTART.md](./QUICKSTART.md)

## How it works

At runtime, MindSand gathers action capabilities, teammate state, perception, actor-registry data, and plan state into an LLM prompt. The response is parsed into one or more plans, each containing an ordered list of actions. The selected teammate's action-plan engine resolves each action through its action description asset, creates the associated gameplay task, and executes the task in order.

```text
World state + teammate capabilities
                │
                ▼
        Prompt context gathering
                │
                ▼
       LLMPipelines conversation
                │
                ▼
      JSON or streaming DSL parser
                │
                ▼
        Teammate action-plan engine
                │
                ▼
       Unreal gameplay task actions
```

Injected responses use the same parser and execution path, making the editor tool useful for development and debugging without implementing a separate simulation path.

## Repository layout

| Path | Description |
| --- | --- |
| `Source/MindSand` | Runtime planner, orchestration, action system, teammate components, and parsers. |
| `Source/ActorRegistry` | Stable runtime actor IDs, descriptions, categories, and transient actor state. |
| `Source/LatencyProfiler` | Named timing statistics and Unreal CSV profiler integration. |
| `Source/MindSandEditor` | Editor menu, dockable Plan Injector tab, and PIE plan dispatch. |
| `Config/DefaultMindSand.ini` | MindSand core redirects and plugin configuration defaults. |
| `Source/ThirdParty/LogViewer` | Optional log viewer. |

## Basic setup

### 1. Define actions

Create `UActionDescriptionAsset` assets for the actions your teammates may perform. Each asset connects the model-facing schema to a `UGameplayTask_Action` or `UGameplayTask_Action_BP` implementation.

Configure, as appropriate:

- Action name and natural-language description.
- Executable gameplay-task class.
- Parameters derived from the task class's `UPROPERTY` fields.

Action names in generated plans must match registered action-description assets.

### 2. Configure teammates

Add the following components to the relevant actors or controllers:

- `UTeammateCharacterAgentDescriptor` for authored name, squad, personality, and initial action names.
- `UTeammateCharacterAgentComponent` for runtime registration, available actions, perception, and plan execution.
- `ULeaderTeammateCharacterAgentComponent` on the player controller to allow the player to issue commands to teammates.

Teammates can register automatically on possession when `bRegisterOnPossess` is enabled, or be registered explicitly through `RegisterTeammate()`.

### 3. Actor Tagging

Add an `UActorDescriptorComponent` to actors that should be available to MindSand. Set its `IDTag` to an `ID.*` gameplay tag, and optionally set `UniqueID` before registration when a predictable runtime ID is needed.

Use the component's `SetState` function to expose transient runtime state such as `"injured"` or `"door open"`. The registry provides actor IDs, descriptions, and state to MindSand.

### 4. Configure gameplay-tag descriptions

Create a `UGameplayTagDescriptionAsset` and add descriptions for the `ID.*` tags used by your actors. MindSand uses these descriptions when building actor context for the LLM.

Assign the asset either per world through `AActorRegistryWorldSettings`, or as the project-wide fallback in **Project Settings > Plugins > Object Registry**. A world-specific asset takes precedence over the project setting.

### 5. Configure the LLM pipeline

Configure the provider and prompt/planning settings described in [MindSand Settings](#mindsand-settings), together with the installed `LLMPipelines` integration. MindSand supports buffered JSON plans and streaming DSL plans through its parser strategies; the selected format must match the response produced by the configured pipeline.

## MindSand Settings

MindSand settings are available in 
````ini 
Edit > Project Settings > Plugins > MindSand Settings
````
<br> They are stored in the host project's `Config/DefaultGame.ini` under 
````ini 
[/Script/MindSand.MindSandSettings]
````

| Setting | Description |
| --- | --- |
| **LLM Provider** | Provider name used by the `LLMPipelines` integration. The default is `HttpLLMProvider`. |
| **System Prompt File Path** | Path to the system prompt file supplied to the planner. |
| **Use JSON** | Sends static game information as JSON instead of MindSand's encoded Markdown representation. |
| **Action Plan Format** | Selects the response parser: `DSL` for streaming DSL plans or `JSON` for buffered JSON plans. |
| **Use Unified Perception for Teammates** | Controls whether teammates and the player share unified perception. Enabled by default. |

Keep the **Action Plan Format** setting aligned with the response format produced by the configured LLM provider. For example, choose `JSON` when the provider returns the `Plans`/`Actions` response described below.

## Plan response format

The built-in JSON parser expects a response containing a case-sensitive `Plans` array. Each plan identifies a teammate and contains an `Actions` array.

```json
{
  "Plans": [
    {
      "teammate": "Medic",
      "acknowledgment": "I will treat the injured teammate.",
      "Actions": [
        {
          "Name": "MoveToActor",
          "parameters": {
            "target": "Player"
          }
        },
        {
          "Name": "Heal",
          "parameters": {
            "target": "Player"
          }
        }
      ]
    }
  ]
}
```

## Debugging and editor workflow

When the editor module is enabled:

1. Start a Play-In-Editor session with registered teammates.
2. Open the **MindSand Plan Injector** tab from the editor Tools menu.
3. Enter a response in the supported plan format.
4. Execute it against the active PIE world.

The editor dispatches the response through `UMindSandDebugUtils::ExecutePlan`, which uses the normal runtime planner/parser/orchestrator path. Plans are saved and loaded under `Saved/MindSand/PlanInjector` by the editor panel.

For C++ or Blueprint integrations, `UMindSandDebugUtils` provides the runtime debug boundary for injecting a response and refreshing perception. Injection requires a valid world, game instance, orchestrator, parser configuration, registered teammates, and a valid response.

## Runtime modules

| Module | Role |
| --- | --- |
| `MindSand` | LLM planning, prompt context, parsing, orchestration, action execution, and teammate runtime systems. |
| `ActorRegistry` | Actor IDs, descriptions, categories, and runtime state. |
| `LatencyProfiler` | Game-instance-scoped latency measurements and CSV profiling. |
| `MindSandEditor` | Editor-only Plan Injector UI and PIE integration. |

`MindSand` depends on `ActorRegistry`, `LatencyProfiler`, and the external `LLMPipelines` module. `MindSandEditor` depends on `MindSand` and is not required for runtime builds.

## Log viewer


#### Enable Logs
To dump LLM request and response JSON while the game is running, enter the following in the Unreal console before issuing a request:

```text
MindSand.DumpJson 1
```

The command is disabled by default. When enabled, MindSand writes the request and response files to `Saved/Logs/MindSand/<Date>/`. Disable dumping again with:

```text
MindSand.DumpJson 0
```

#### Viewing Logs
The repository includes a separate log viewer in `Source/ThirdParty/LogViewer`. It has a Python backend and a Vite/TypeScript frontend.

Its README contains the current local setup commands:

```powershell
cd Source/ThirdParty/LogViewer
py -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -r requirements.txt
```

