> **WIP:** drumTTing is an early S2400-focused LV2 instrument prototype.

# drumTTing for S2400

drumTTing is an original monophonic analog-style percussion voice for the Isla Instruments S2400 DSP Card. It is not an emulation of any commercial instrument and uses no third-party samples, branding, panel artwork, GUI framework, JUCE, VST, or CLAP code.

## S2400 Scope

- LV2 instrument.
- aarch64 / ARM64 Linux.
- MIDI input, stereo audio output.
- MIDI Atom port at index 0, stereo audio outputs at indices 1 and 2.
- No audio input processing.
- No AUX section, no `aux_route`, no `aux_gain`.
- No GUI extension.
- 30 LV2 control parameters, below the S2400 32-parameter limit.
- Continuous controls are internally quantized to two decimal places.

## Build on Ubuntu aarch64

```bash
cd ~/projects/drumTTing
make clean
make
make check
```

The unpacked bundle is:

```text
drumTTing.lv2
```

The binary should report as an aarch64 shared object on the native ARM64 VM:

```bash
file drumTTing.lv2/drumTTing.so
```

Optional validators are used only if installed:

```bash
lv2_validate drumTTing.lv2/manifest.ttl drumTTing.lv2/drumTTing.ttl
lv2lint -Mpack urn:asier:lv2:drumtting
```

## Copy to Unraid / S2400 Plugin Folder

Adjust the destination if your S2400 plugin scan folder changes:

```bash
ssh root@[IP] 'rm -rf "/mnt/user/Musica/Desarrollo LV2/drumTTing.lv2"'
scp -r drumTTing.lv2 root@[IP]:'/mnt/user/Musica/Desarrollo LV2/'
```

## Parameters

VCO:

- `vco_wave` / `VCO_Wave`: sine-like to saturated square-like shaping.
- `vco_oct` / `VCO_Oct`: integer octave offset from -3 to +3.
- `vco_pitch` / `VCO_Pitch`: 20 Hz to 5000 Hz body frequency. MIDI note triggers the voice but does not transpose this frequency.
- `vco_range` / `VCO_Range`: Low or High.
- `vco_pbend` / `VCO_PBend`: pitch envelope amount, -48 to +48 semitones.
- `vco_penv` / `VCO_PEnv`: pitch envelope decay.
- `vco_decay` / `VCO_Decay`: VCO source decay before the routing mixer.
- `vco_5th` / `VCO_5th`: toggles a sine fifth at 3/2 of the VCO frequency, mixed at -9 dB.

FM:

- `fm_intensity` / `FM_Intensity`: bounded pitch FM depth.
- `fm_frequency` / `FM_Frequency`: FM sine frequency.
- `fm_phase_reset` / `FM_Phase_Reset`: reset FM phase on trigger.

VCF:

- `vcf_type` / `VCF_Type`: continuous morph low-pass to band-pass to high-pass.
- `vcf_cutoff` / `VCF_Cutoff`.
- `vcf_resonance` / `VCF_Resonance`.
- `vcf_bend` / `VCF_Bend`.
- `vcf_env_time` / `VCF_Env_Time`.

Routing mixer:

- `mix_vco` / `MIX_VCO`.
- `mix_click` / `MIX_Click`.
- `mix_noise` / `MIX_Noise`.
- Negative values route to the VCF bus.
- Zero mutes the source.
- Positive values route directly to the VCA bus.

Click and noise:

- `click_style` / `CLICK_Style`.
- `click_level` / `CLICK_Level`.
- `noise_color` / `NOISE_Color`: Pink or Blue.
- `noise_decay` / `NOISE_Decay`.
- `noise_level` / `NOISE_Level`.

Output:

- `vca_decay` / `VCA_Decay`.
- `drive` / `DRIVE`.
- `gain` / `GAIN`.
- `vel_amount` / `VEL_AMOUNT`.
- `comp` / `COMPRESSOR`: one-knob SSL-style bus compression after drive and before final clipping.
- `clip` / `CLIP`: one-knob final clipper after the compressor and output gain.

## Starting Recipes

Kick:

- Keep defaults.
- Raise `drive` for more weight.
- Route VCO mostly direct with `mix_vco` around `0.9`.

Dusty Kick:

- `noise_level`: `0.12`
- `mix_noise`: `-0.35`
- `vcf_cutoff`: `2500`
- `vcf_bend`: `-8`
- `drive`: `0.35`

Snare:

- `vco_pitch`: `180`
- `vco_oct`: `0`
- `vco_pbend`: `-12`
- `noise_level`: `0.75`
- `mix_noise`: `-0.8`
- `mix_vco`: `0.2`
- `vcf_type`: `0.45`
- `vca_decay`: `350`

Closed Hat:

- `vco_range`: `High`
- `vco_oct`: `0`
- `vco_wave`: `0.75`
- `noise_color`: `Blue`
- `noise_level`: `0.85`
- `noise_decay`: `60`
- `vco_decay`: `80`
- `vcf_type`: `1.0`
- `vcf_cutoff`: `7000`

Tom:

- `vco_pitch`: `95`
- `vco_oct`: `0`
- `vco_pbend`: `18`
- `vco_penv`: `90`
- `vco_decay`: `950`
- `vca_decay`: `900`
- `click_level`: `0.25`

Metallic Perc:

- `vco_range`: `High`
- `vco_oct`: `0`
- `vco_wave`: `0.55`
- `fm_intensity`: `0.7`
- `fm_frequency`: `1700`
- `vco_5th`: `1`
- `vcf_type`: `0.5`
- `vcf_resonance`: `0.45`
- `vca_decay`: `280`
