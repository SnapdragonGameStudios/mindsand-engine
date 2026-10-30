# MindSand
MindSand is an open-source framework for building real-time, context-aware behavior engines for AI-driven game characters.


| MindSand Projects | Repository |
|---|---|
| **MindSand Unreal Engine Plugin** | <a href="https://github.com/SnapdragonGameStudios/mindsand-engine/tree/main/mindsand-ue-plugin">`mindsand-ue-plugin`</a> |

## Snapdragon Game AI SDK

MindSand Framework uses Snapdragon GameAI SDK for ASR, LLM, and TTS interfaces. Cloud and On-Device provider implementations are included.

<table>
    <tr>
      <th>Interface</th>
      <th>Interface repo</th>
      <th>Implementations repo</th>
    </tr>
    <tr>
      <td rowspan="2">
        <strong>ASR</strong><br>
        <small>Speech Recognizer</small>
      </td>
      <td rowspan="2">
        <a href="https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/5.6/Plugins/SGAI/SGAISpeechRecognizer">
          sgai-speechrecognizer
        </a>
      </td>
      <td>
        <a href="https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/5.6/Plugins/SGAI/SGAIHttpASR">
          sgai-http-provider-asr
        </a><br>
        HTTP-based interfacing with a Cloud Provider.<br>
        </td>
        </tr>
        <tr>
        <td>
        <a href="https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/5.6/Plugins/SGAI/SGAISpeechRecognizer/Source/VoiceAIASR">
          voiceai-asr
        </a><br>
        On-device implementation with Qualcomm&rsquo;s VoiceAI ASR SDK
      </td>
      </tr>
    <tr>
      <td rowspan="2">
        <strong>LLM</strong><br>
        <small>LLM Pipelines</small>
      </td>
      <td rowspan="2">
        <a href="https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/5.6/Plugins/SGAI/SGAILLMPipelines">
          sgai-llmpipelines
        </a>
      </td>
      <td>
        <a href="https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/5.6/Plugins/SGAI/SGAIHttpProviderLLM">
          sgai-http-provider-llmpipelines
        </a><br>
        HTTP-based interfacing with a Cloud Provider.
      </td>
      </tr>
      <tr>
      <td>
        <a href="https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/5.6/Plugins/SGAI/SGAILLMPipelines/Source/GenieLLMProvider">
          genie-llmprovider
        </a>
<br>
        On-device implementation 
        with Qualcomm&rsquo;s Genie SDK<br>
      </td>
    </tr>
    <tr>
      <td rowspan="2">
        <strong>TTS</strong><br>
        <small>Text to Speech</small>
      </td>
      <td rowspan="2">
        <a href="https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/5.6/Plugins/SGAI/SGAITextToSpeech">
          sgai-texttospeech
        </a>
      </td>
      <td>
      <a href="https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/5.6/Plugins/SGAI/SGAIHttpProviderTTS">
          sgai-http-provider-tts
        </a><br>HTTP-based interfacing with a Cloud Provider.
        </td>
        </tr>
        <tr><td>
        <a href="https://github.com/SnapdragonGameStudios/snapdragon-game-plugins-for-unreal-engine/tree/engine/5.6/Plugins/SGAI/SGAITextToSpeech/Source/VoiceAITTS">
          voiceai-tts
        </a><br>
        On-device implementation with Qualcomm&rsquo;s VoiceAI TTS SDK        
      </td>
    </tr>
    <tr></tr>
</table>

# Getting Started with Unreal Engine Plugin

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

Further instructions at [README](./mindsand_ue_plugin/README.md) and [GETTINGSTARTED](./mindsand_ue_plugin/GETTINGSTARTED.md)

## License

mindsand-engine is licensed under the [BSD-3-Clause-Clear](https://spdx.org/licenses/BSD-3-Clause-Clear.html). See [LICENSE](LICENSE) for the full license text.