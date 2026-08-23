# Audio architecture

Battalion separates sound generation from operating-system output. Gameplay
submits a semantic sound identifier and, for world sounds, the source position.
The shared C89 mixer then performs distance attenuation, left/right panning,
sample-rate conversion, voice mixing, and clipping. Platform backends only
deliver the resulting 44.1 kHz signed 16-bit stereo PCM stream.

The spatial mixer is currently used by:

- macOS through the system CoreAudio AudioUnit output;
- Linux through OSS `/dev/dsp`;
- IRIX through the SGI Audio Library;
- Solaris and SunOS through `/dev/audio`.

A build with `AUDIO=none` disables output without changing gameplay.

## Building

The native spatial backend is the default on macOS, Linux, IRIX, Solaris, and
SunOS:

```sh
make PLATFORM=macos
make PLATFORM=linux
make PLATFORM=sgi
make PLATFORM=solaris
```

It can also be selected explicitly:

```sh
make PLATFORM=macos AUDIO=coreaudio
make PLATFORM=linux AUDIO=oss
make PLATFORM=sgi AUDIO=sgi
```

## Runtime model

`audioSetListener()` receives the player world position and facing direction.
`doSoundAt()` submits a sound at a world position. The common mixer derives an
equal-power stereo pan from the source direction and reduces volume with
distance. Sources at or beyond the maximum audible distance are silent. Music
is intentionally centered and is not attenuated.

World positions are currently supplied for projectile shots, rockets, energy
beams, explosions, object destruction, and vehicle/monster destruction.

## Assets

PCM mixer assets are declared in `assets/audio/catalog.txt` with backend
`pcm`. Files must be RIFF/WAVE, uncompressed 16-bit PCM, mono or stereo. Sound
effects should normally be mono because spatial panning is applied at runtime.
Music may be stereo. Source sample rates may differ; the mixer converts them to
the backend output rate while playing.

All native backends use the same `pcm` entries, so replacing a sound does not
require platform-specific code or duplicate source assets.

## Adding another output backend

Implement `audioBackendStart()` and `audioBackendStop()` from
`audio_backend.h`. The output callback or worker thread should repeatedly call
`audioMixerRender()` and deliver the returned interleaved stereo frames to the
native API. It must not contain gameplay, coordinate, or renderer logic.

The spatial math and WAV mixer have regression coverage in
`tests/audio_mixer_test.c`; the asset declarations are covered by
`tests/audio_catalog_test.c`.
