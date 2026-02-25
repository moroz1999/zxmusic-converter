# AGENTS.md

This file contains the most CRITICAL rules that ALL agents must follow. For detailed information, see the documentation tree below.

## CRITICAL RULES

### General
- Keep it simple, don't invent unnecessary checks.
- Before each new task, read `AGENTS.md` and review the relevant rules if task scope differs from previous.
- All documentation and code comments MUST be in English, even if the user communicates in Russian or another language.
- After completing a task, re-read the task description and verify every point.
- After finishing the task code, it is necessary to read the rules on this topic from the .md docs once again and double-check your changes.
- Before starting a task, you MUST read the relevant documents from the DOCUMENTATION TREE below.
- Documentation updates must be placed in the appropriate .md file (e.g., PHP rules in php.md).
- Any new knowledge about functionality must be added to separate sub-documents within `domain.md`.
- Documentation additions in `docs` must be concise, clear, and only about the core points.
- ALWAYS add newly created files to GIT immediately after creation.
- When the IDE is in 'Ask' (readonly) mode, it is STRICTLY FORBIDDEN to do anything except answering the user's question. No file modifications or tool calls that change state are allowed.
- ALWAYS use MCP tools (JetBrains IDE) when available for code search, file reading, navigation, and locating files, methods, and classes instead of Grep/Glob/Read/Bash.
- Do not scan the whole project by file extension. Use targeted paths or direct file reads instead.
- Do not run naive recursive searches over the entire repository. Pick the specific directories from the documented project structure that match the task.
- Do not use `em` for icon sizes when fixed pixel size is possible. Use `px` via component/theme CSS variables.

## DOCUMENTATION TREE

Read ONLY the documents relevant to your task.

### Core Documentation
- **[docs/domain.md](docs/domain.md)** - Project domain and entities

### Backend (PHP)
- **[docs/php.md](docs/php.md)** - PHP coding standards, DDD, SOLID, immutability, type safety


## PROJECT STRUCTURE

```
zxmusic-converter/
├── public/
│   ├── index.php               # Entry point: bootstraps DI container, dispatches to MusicController
│   └── music/                  # Final MP3 output (publicly accessible)
├── src/
│   ├── Config/
│   │   └── ContainerSetup.php  # PHP-DI container wiring (all bindings)
│   ├── Controller/
│   │   └── MusicController.php # HTTP layer: validates upload, builds ConversionConfig, calls Converter
│   ├── Dto/
│   │   ├── ApiResponse.php     # JSON envelope: {success, data, error}
│   │   ├── ConversionConfig.php# Input for converters: paths, chip params (frequency, chipType, channels, frameDuration)
│   │   ├── ConversionResult.php# Output of converters: mp3Name, title, author, time, channels, type, container, program
│   │   └── PathConfig.php      # Configured root paths: uploadPath, resultPath, musicPath
│   ├── Factory/
│   │   └── ConverterFactory.php# Resolves ConverterInterface by ConverterType enum via DI container
│   ├── Response/
│   │   └── ResponseHandler.php # Sends gzip-encoded JSON HTTP response
│   └── Service/
│       ├── Directories.php     # Utility: creates directories recursively
│       ├── Arkos/
│       │   ├── AksInformationParser.php  # Extracts XML from .aks (ZIP or GZIP), parses title/author/version/frequency
│       │   ├── Arkos1Converter.php       # .aks v1: AKSToYM.exe → YM → ZxTuneConverter → MP3
│       │   ├── Arkos2Converter.php       # .aks v2: SongToWav.exe → WAV → FfmpegConverter → MP3
│       │   ├── ParsedInformation.php     # DTO: title, author, formatVersion, frequency, trackerVersion
│       │   ├── Version.php               # Enum: VERSION1 | VERSION2
│       │   └── VersionResolver.php       # Delegates to AksInformationParser to detect Arkos version
│       ├── ChipNSfx/
│       │   └── ChipNSfxConverter.php     # .chp: CHIPNSFX.EXE -w → WAV → FfmpegConverter → MP3
│       ├── Converter/
│       │   ├── Converter.php             # Orchestrator: resolve type → get converter → convert → move → cleanup
│       │   ├── ConverterInterface.php    # convert(ConversionConfig): ConversionResult[]
│       │   ├── ConverterType.php         # Enum: ZXTUNE | ARKOS1 | ARKOS2 | FURNACE | CHIPNSFX
│       │   └── ConverterTypeResolver.php # Maps file extension to ConverterType; detects Arkos version for .aks
│       ├── FfmpegConverter/
│       │   └── FfmpegConverter.php       # WAV → MP3 via ffmpeg (320 kbps stereo, shared by Arkos2/Furnace/ChipNSfx)
│       ├── Furnace/
│       │   └── FurnaceConverter.php      # .fur: furnace.exe -output → WAV → FfmpegConverter → MP3
│       └── ZxTune/
│           └── ZxTuneConverter.php       # Default: zxtune123.exe with AY chip params → MP3; parses multi-subtune output
├── binaries/
│   ├── arkos1/Tools/AKSToYM.exe
│   ├── arkos2/tools/SongToWav.exe
│   ├── chipnsfx/CHIPNSFX.EXE
│   ├── ffmpeg/bin/ffmpeg.exe
│   ├── furnace/furnace.exe
│   └── zxtune/zxtune123.exe
├── uploads/                    # Uploaded originals (kept, organized by ID)
├── result/                     # Temporary conversion intermediates (cleaned up per request)
├── docs/
│   ├── domain.md               # Domain knowledge: formats, pipelines, chip params, runtime dirs
│   ├── php.md                  # PHP coding standards
│   └── testing.md              # Testing rules
├── composer.json
└── AGENTS.md
```

