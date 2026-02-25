## Project Domain

PHP web application that converts ZX Spectrum native audio formats to MP3. Acts as an HTTP backend: receives a file upload with metadata, runs the appropriate converter binary, and returns a JSON response with the resulting MP3 file name and track metadata.

---

## Supported Formats

| Extension | Tracker / Tool     | Converter class       | Pipeline                           |
|-----------|--------------------|-----------------------|------------------------------------|
| `*` (default) | ZX Spectrum trackers (PT3, STC, AYC, PSG, NSF, VTX, etc.) | `ZxTuneConverter`   | file → zxtune123.exe → MP3         |
| `.aks` v1  | Arkos Tracker 1    | `Arkos1Converter`     | file → AKSToYM.exe → YM → zxtune123.exe → MP3 |
| `.aks` v2  | Arkos Tracker 2    | `Arkos2Converter`     | file → SongToWav.exe → WAV → ffmpeg → MP3 |
| `.fur`     | Furnace Tracker    | `FurnaceConverter`    | file → furnace.exe → WAV → ffmpeg → MP3 |
| `.chp`     | ChipNSfx           | `ChipNSfxConverter`   | file → CHIPNSFX.EXE → WAV → ffmpeg → MP3 |

**Version detection for `.aks`**: The file is a ZIP or GZIP archive wrapping an XML document.
- **Arkos 1** XML: no namespace, has `<Version>`, `<Name>`, `<Author>`, `<MasterFrequency>` elements.
- **Arkos 2** XML: has `aks:` XML namespace, with `aks:title` and `aks:author` elements.

---

## AY/YM Chip Parameters (ZxTune)

These are passed via `ConversionConfig` and forwarded to `zxtune123.exe`:

| Parameter      | ZxTune option         | Default   | Description                                    |
|----------------|-----------------------|-----------|------------------------------------------------|
| `frequency`    | `aym.clockrate`       | 1770000   | Master clock in Hz (ZX Spectrum: 1.77 MHz)     |
| `chipType`     | `aym.type`            | 0         | Chip model: 0 = AY-3-8910, 1 = YM2149         |
| `channels`     | `aym.layout`          | 0         | Stereo layout (ABC, ACB, BAC, etc.)            |
| `frameDuration`| `--frameduration`     | 20000     | Frame duration in microseconds (50 Hz = 20000) |

For Arkos 1, `frequency` is overridden from the `<MasterFrequency>` field embedded in the `.aks` XML.

---

## Directory Layout (Runtime)

```
uploads/{id}/     — uploaded original file, kept after conversion
result/{id}/      — intermediate files (WAV, YM, MP3); deleted after conversion
public/music/     — final MP3 files, publicly accessible
```

All intermediate files in `result/{id}/` are cleaned up after the converted MP3 is moved to `public/music/`.

---

## ConversionResult Fields

Returned as JSON array to the caller:

| Field         | Description                                              |
|---------------|----------------------------------------------------------|
| `mp3Name`     | Final MP3 filename in `public/music/`                   |
| `title`       | Track title (from tracker metadata or empty)            |
| `author`      | Author name (from tracker metadata or empty)            |
| `time`        | Duration string (from ZxTune output or empty)           |
| `channels`    | Channel count string (from ZxTune or hardcoded `"3"`)   |
| `type`        | Format type identifier (e.g., `"AKS"`, `"FUR"`, `"CHP"`) |
| `container`   | Container type identifier (same as `type` for most)    |
| `program`     | Tracker/program that created the file                   |

ZxTune multi-subtune files produce multiple `ConversionResult` entries (one per subtune).

---

## Binaries

All binaries are Windows `.exe` files located under `binaries/`:

| Path                                | Used by             |
|-------------------------------------|---------------------|
| `binaries/zxtune/zxtune123.exe`     | `ZxTuneConverter`   |
| `binaries/arkos1/Tools/AKSToYM.exe` | `Arkos1Converter`   |
| `binaries/arkos2/tools/SongToWav.exe` | `Arkos2Converter` |
| `binaries/chipnsfx/CHIPNSFX.EXE`   | `ChipNSfxConverter` |
| `binaries/furnace/furnace.exe`      | `FurnaceConverter`  |
| `binaries/ffmpeg/bin/ffmpeg.exe`    | `FfmpegConverter`   |

`FfmpegConverter` is a shared dependency used by Arkos2, Furnace, and ChipNSfx to encode WAV → MP3 at 320 kbps stereo (`-ab 320k -joint_stereo 0 -ac 2`).