> **WIP:** DingVoiceLV2 is an early S2400-focused LV2 instrument prototype.

# DingVoiceLV2 for S2400

DingVoiceLV2 is an original monophonic analog-style percussion voice for the Isla Instruments S2400 DSP Card. It is not an emulation of any commercial instrument and uses no third-party samples, branding, panel artwork, GUI framework, JUCE, VST, or CLAP code.

## S2400 Scope

- LV2 instrument.
- aarch64 / ARM64 Linux.
- MIDI input, stereo audio output.
- No audio input processing.
- No AUX section, no `aux_route`, no `aux_gain`.
- No GUI extension.
- 28 LV2 control parameters, below the S2400 32-parameter limit.

## Build on Ubuntu aarch64

```bash
cd ~/projects/drumTTing
make clean
make
make check
```

The unpacked bundle is:

```text
DingVoiceLV2.lv2
```

The binary should report as an aarch64 shared object on the native ARM64 VM:

```bash
file DingVoiceLV2.lv2/DingVoiceLV2.so
```

Optional validators are used only if installed:

```bash
lv2_validate DingVoiceLV2.lv2/manifest.ttl DingVoiceLV2.lv2/DingVoiceLV2.ttl
lv2lint -Mpack urn:asier:lv2:dingvoice
```

## Copy to Unraid / S2400 Plugin Folder

Adjust the destination if your S2400 plugin scan folder changes:

```bash
ssh root@10.10.20.61 'rm -rf "/mnt/user/Musica/Desarrollo LV2/DingVoiceLV2.lv2"'
scp -r DingVoiceLV2.lv2 root@10.10.20.61:'/mnt/user/Musica/Desarrollo LV2/'
```

## Parameters

VCO:

- `vco_wave`: sine-like to saturated square-like shaping.
- `vco_pitch`: 20 Hz to 5000 Hz.
- `vco_range`: Low or High.
- `pitch_bend`: pitch envelope amount, -48 to +48 semitones.
- `pitch_env_time`: pitch envelope decay.
- `vco_decay`: VCO source decay before the routing mixer.

FM:

- `fm_intensity`: bounded pitch FM depth.
- `fm_frequency`: FM sine frequency.
- `fm_phase_reset`: reset FM phase on trigger.

VCF:

- `cutoff`, `resonance`.
- `filter_type`: continuous morph low-pass to band-pass to high-pass.
- `filter_bend`, `filter_env_time`.
- `filter_sh_amount`, `filter_sh_smooth`.

Routing mixer:

- `click_route`, `noise_route`, `vco_route`.
- Negative values route to the VCF bus.
- Zero mutes the source.
- Positive values route directly to the VCA bus.

Click and noise:

- `click_style`, `click_level`.
- `noise_color`: Pink or Blue.
- `noise_decay`, `noise_level`.

Output:

- `vca_decay`, `drive`, `output_gain`, `velocity_amount`.

## Starting Recipes

Kick:

- Keep defaults.
- Raise `drive` for more weight.
- Route VCO mostly direct with `vco_route` around `0.9`.

Dusty Kick:

- `noise_level`: `0.12`
- `noise_route`: `-0.35`
- `cutoff`: `2500`
- `filter_bend`: `-8`
- `drive`: `0.35`

Snare:

- `vco_pitch`: `180`
- `pitch_bend`: `-12`
- `noise_level`: `0.75`
- `noise_route`: `-0.8`
- `vco_route`: `0.2`
- `filter_type`: `0.45`
- `vca_decay`: `350`

Closed Hat:

- `vco_range`: `High`
- `vco_wave`: `0.75`
- `noise_color`: `Blue`
- `noise_level`: `0.85`
- `noise_decay`: `60`
- `vco_decay`: `80`
- `filter_type`: `1.0`
- `cutoff`: `7000`

Tom:

- `vco_pitch`: `95`
- `pitch_bend`: `18`
- `pitch_env_time`: `90`
- `vco_decay`: `950`
- `vca_decay`: `900`
- `click_level`: `0.25`

Metallic Perc:

- `vco_range`: `High`
- `vco_wave`: `0.55`
- `fm_intensity`: `0.7`
- `fm_frequency`: `1700`
- `filter_type`: `0.5`
- `resonance`: `0.45`
- `vca_decay`: `280`
