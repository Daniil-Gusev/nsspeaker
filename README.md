# Emacspeak macOS Speech Server

This repository contains a native macOS text-to-speech server for Emacspeak. It acts as a bridge between the Emacspeak subsystem and the native macOS speech synthesis frameworks.

The server implements the [Emacspeak TTS-Servers specification](https://tvraman.github.io/emacspeak/manual/TTS-Servers.html) and supports text queuing, rate and scale adjustments, punctuation handling, language switching, tone generation, and inline audio playback, with a few departures from the spec noted in [Protocol Deviations](#protocol-deviations).

## Architecture & API Choice

The server is written in C89 and Objective-C without Automatic Reference Counting (ARC).

It explicitly utilizes the older `NSSpeechSynthesizer` Cocoa API instead of the modern `AVSpeechSynthesizer`, for two reasons:

- **Responsiveness:** `NSSpeechSynthesizer` exhibits lower dispatch latency and faster interrupt times, which is a strict requirement for screen reader software.
- **Backward compatibility:** `AVSpeechSynthesizer` was introduced to the macOS ecosystem in OS X 10.14/10.15, breaking compatibility with older Apple hardware. By relying on `NSSpeechSynthesizer` and manual memory management (no ARC), this server keeps working back to OS X 10.5 Leopard, including PowerPC Macs.

## Protocol Deviations

The server departs from the Emacspeak TTS-Servers spec in two small, deliberate ways:

- **`tts_say`** strips the Dectalk-specific morpheme-boundary marker `[*]` instead of translating it, since `NSSpeechSynthesizer` has no equivalent construct.
- **`tts_sync_state`** parses all four fields (`punct splitcaps caps rate`) but ignores `caps` - that field covers engine-specific capitalization handling that `NSSpeechSynthesizer` doesn't need.

## Compatibility Requirements

Operation has been verified on the following two stacks:

- OS X 10.5 Leopard (PPC VM), Emacs 22.1.1, Emacspeak 29.0.
- macOS 26 (Apple Silicon), Emacs 31.1, Emacspeak 60.0.

Building requires Xcode or the Xcode Command Line Tools.

## Build Instructions

To compile the standard release binary:

```sh
make
```

To compile a debug build with additional logging and diagnostic symbols:

```sh
make DEBUG=1
```

To format the source code (requires `clang-format` in your `$PATH`):

```sh
make fmt
```

## Installation

```sh
cp nsspeaker path_to_your_emacspeak/servers
export DTK_PROGRAM=nsspeaker
```

## Dependencies

This project has no external dynamic dependencies other than the standard Apple macOS system frameworks (`AppKit`, `Foundation`, `AudioToolbox`).

- **[minivorbis](https://github.com/edubart/minivorbis)**: The source tree vendors `minivorbis` to provide `.ogg` audio decoding for Emacspeak auditory icons and alerts. The library source has been slightly patched to ensure strict C89 standard compliance.

## License

GNU General Public License v2.0 — full text in [LICENSE](LICENSE).
