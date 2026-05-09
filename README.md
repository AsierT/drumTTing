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

## Parameter Order

The S2400 sees the following 30 control parameters after MIDI and stereo audio ports:

1. `vco_wave` / `VCO_Wave`
2. `vco_oct` / `VCO_Oct`
3. `vco_pitch` / `VCO_Pitch`
4. `vco_range` / `VCO_Range`
5. `vco_pbend` / `VCO_PBend`
6. `vco_penv` / `VCO_PEnv`
7. `vco_decay` / `VCO_Decay`
8. `vco_5th` / `VCO_5th`
9. `fm_intensity` / `FM_Intensity`
10. `fm_frequency` / `FM_Frequency`
11. `fm_phase_reset` / `FM_Phase_Reset`
12. `vcf_type` / `VCF_Type`
13. `vcf_cutoff` / `VCF_Cutoff`
14. `vcf_resonance` / `VCF_Resonance`
15. `vcf_bend` / `VCF_Bend`
16. `vcf_env_time` / `VCF_Env_Time`
17. `click_style` / `CLICK_Style`
18. `click_level` / `CLICK_Level`
19. `noise_color` / `NOISE_Color`
20. `noise_decay` / `NOISE_Decay`
21. `noise_level` / `NOISE_Level`
22. `mix_vco` / `MIX_VCO`
23. `mix_click` / `MIX_Click`
24. `mix_noise` / `MIX_Noise`
25. `vca_decay` / `VCA_Decay`
26. `drive` / `DRIVE`
27. `gain` / `GAIN`
28. `vel_amount` / `VEL_AMOUNT`
29. `comp` / `COMPRESSOR`
30. `clip` / `CLIP`

## Parameters

VCO:

- `vco_wave`: sine-like to saturated square-like shaping.
- `vco_oct`: integer octave offset from -3 to +3.
- `vco_pitch`: 20 Hz to 5000 Hz body frequency. MIDI note triggers the voice but does not transpose this frequency.
- `vco_range`: Low or High.
- `vco_pbend`: pitch envelope amount, -48 to +48 semitones.
- `vco_penv`: pitch envelope decay time.
- `vco_decay`: VCO source decay before the routing mixer.
- `vco_5th`: adds a perfect-fifth sine oscillator at 3/2 of the VCO frequency.

FM:

- `fm_intensity`: bounded pitch FM depth, max +/-24 semitones.
- `fm_frequency`: FM sine frequency.
- `fm_phase_reset`: reset FM phase on trigger.

VCF:

- `vcf_type`: continuous morph low-pass to band-pass to high-pass.
- `vcf_cutoff`, `vcf_resonance`.
- `vcf_bend`, `vcf_env_time`.

Click and noise:

- `click_style`, `click_level`.
- `noise_color`: Pink or Blue.
- `noise_decay`, `noise_level`.

Routing mixer:

- `mix_vco`, `mix_click`, `mix_noise`.
- Negative values route to the VCF bus.
- Zero mutes the source.
- Positive values route directly to the VCA bus.

Output:

- `vca_decay`, `drive`, `gain`, `vel_amount`.
- `comp`: one-knob bus compressor after drive.
- `clip`: one-knob final clipper after compressor and gain.

## Starting Recipes

All recipes list the full 30-parameter state in port order.

Kick:

- `vco_wave`: `0.25`
- `vco_oct`: `0`
- `vco_pitch`: `55.00`
- `vco_range`: `Low`
- `vco_pbend`: `36.00`
- `vco_penv`: `45.00`
- `vco_decay`: `700.00`
- `vco_5th`: `Off`
- `fm_intensity`: `0.00`
- `fm_frequency`: `100.00`
- `fm_phase_reset`: `On`
- `vcf_type`: `0.00`
- `vcf_cutoff`: `12000.00`
- `vcf_resonance`: `0.10`
- `vcf_bend`: `0.00`
- `vcf_env_time`: `100.00`
- `click_style`: `0.40`
- `click_level`: `0.60`
- `noise_color`: `Pink`
- `noise_decay`: `80.00`
- `noise_level`: `0.00`
- `mix_vco`: `0.90`
- `mix_click`: `0.25`
- `mix_noise`: `0.00`
- `vca_decay`: `700.00`
- `drive`: `0.30`
- `gain`: `-3.00`
- `vel_amount`: `0.70`
- `comp`: `0.00`
- `clip`: `0.00`

Punchy Kick:

- `vco_wave`: `0.20`
- `vco_oct`: `0`
- `vco_pitch`: `50.00`
- `vco_range`: `Low`
- `vco_pbend`: `40.00`
- `vco_penv`: `30.00`
- `vco_decay`: `500.00`
- `vco_5th`: `Off`
- `fm_intensity`: `0.00`
- `fm_frequency`: `100.00`
- `fm_phase_reset`: `On`
- `vcf_type`: `0.00`
- `vcf_cutoff`: `10000.00`
- `vcf_resonance`: `0.00`
- `vcf_bend`: `-12.00`
- `vcf_env_time`: `60.00`
- `click_style`: `0.30`
- `click_level`: `0.80`
- `noise_color`: `Pink`
- `noise_decay`: `60.00`
- `noise_level`: `0.05`
- `mix_vco`: `0.95`
- `mix_click`: `0.40`
- `mix_noise`: `-0.10`
- `vca_decay`: `500.00`
- `drive`: `0.45`
- `gain`: `-3.00`
- `vel_amount`: `0.80`
- `comp`: `0.30`
- `clip`: `0.10`

Dusty Kick:

- `vco_wave`: `0.20`
- `vco_oct`: `0`
- `vco_pitch`: `52.00`
- `vco_range`: `Low`
- `vco_pbend`: `30.00`
- `vco_penv`: `40.00`
- `vco_decay`: `600.00`
- `vco_5th`: `Off`
- `fm_intensity`: `0.00`
- `fm_frequency`: `100.00`
- `fm_phase_reset`: `On`
- `vcf_type`: `0.00`
- `vcf_cutoff`: `2500.00`
- `vcf_resonance`: `0.10`
- `vcf_bend`: `-8.00`
- `vcf_env_time`: `120.00`
- `click_style`: `0.35`
- `click_level`: `0.50`
- `noise_color`: `Pink`
- `noise_decay`: `120.00`
- `noise_level`: `0.12`
- `mix_vco`: `0.75`
- `mix_click`: `0.25`
- `mix_noise`: `-0.35`
- `vca_decay`: `650.00`
- `drive`: `0.35`
- `gain`: `-3.00`
- `vel_amount`: `0.70`
- `comp`: `0.00`
- `clip`: `0.08`

Snare:

- `vco_wave`: `0.30`
- `vco_oct`: `0`
- `vco_pitch`: `180.00`
- `vco_range`: `Low`
- `vco_pbend`: `-12.00`
- `vco_penv`: `25.00`
- `vco_decay`: `200.00`
- `vco_5th`: `Off`
- `fm_intensity`: `0.00`
- `fm_frequency`: `100.00`
- `fm_phase_reset`: `On`
- `vcf_type`: `0.45`
- `vcf_cutoff`: `4000.00`
- `vcf_resonance`: `0.20`
- `vcf_bend`: `-6.00`
- `vcf_env_time`: `80.00`
- `click_style`: `0.50`
- `click_level`: `0.70`
- `noise_color`: `Pink`
- `noise_decay`: `220.00`
- `noise_level`: `0.75`
- `mix_vco`: `0.20`
- `mix_click`: `0.35`
- `mix_noise`: `-0.80`
- `vca_decay`: `350.00`
- `drive`: `0.25`
- `gain`: `-3.00`
- `vel_amount`: `0.80`
- `comp`: `0.00`
- `clip`: `0.05`

Rimshot:

- `vco_wave`: `0.50`
- `vco_oct`: `0`
- `vco_pitch`: `350.00`
- `vco_range`: `Low`
- `vco_pbend`: `-6.00`
- `vco_penv`: `8.00`
- `vco_decay`: `80.00`
- `vco_5th`: `Off`
- `fm_intensity`: `0.15`
- `fm_frequency`: `700.00`
- `fm_phase_reset`: `On`
- `vcf_type`: `0.50`
- `vcf_cutoff`: `5000.00`
- `vcf_resonance`: `0.35`
- `vcf_bend`: `0.00`
- `vcf_env_time`: `50.00`
- `click_style`: `0.70`
- `click_level`: `0.90`
- `noise_color`: `Blue`
- `noise_decay`: `40.00`
- `noise_level`: `0.30`
- `mix_vco`: `-0.60`
- `mix_click`: `0.70`
- `mix_noise`: `-0.50`
- `vca_decay`: `90.00`
- `drive`: `0.20`
- `gain`: `-3.00`
- `vel_amount`: `0.90`
- `comp`: `0.00`
- `clip`: `0.10`

Closed Hat:

- `vco_wave`: `0.75`
- `vco_oct`: `0`
- `vco_pitch`: `400.00`
- `vco_range`: `High`
- `vco_pbend`: `0.00`
- `vco_penv`: `5.00`
- `vco_decay`: `80.00`
- `vco_5th`: `Off`
- `fm_intensity`: `0.00`
- `fm_frequency`: `100.00`
- `fm_phase_reset`: `On`
- `vcf_type`: `1.00`
- `vcf_cutoff`: `7000.00`
- `vcf_resonance`: `0.15`
- `vcf_bend`: `0.00`
- `vcf_env_time`: `50.00`
- `click_style`: `0.60`
- `click_level`: `0.40`
- `noise_color`: `Blue`
- `noise_decay`: `60.00`
- `noise_level`: `0.85`
- `mix_vco`: `0.30`
- `mix_click`: `0.20`
- `mix_noise`: `-0.90`
- `vca_decay`: `65.00`
- `drive`: `0.15`
- `gain`: `-3.00`
- `vel_amount`: `0.70`
- `comp`: `0.00`
- `clip`: `0.00`

Open Hat:

- `vco_wave`: `0.75`
- `vco_oct`: `0`
- `vco_pitch`: `400.00`
- `vco_range`: `High`
- `vco_pbend`: `0.00`
- `vco_penv`: `5.00`
- `vco_decay`: `500.00`
- `vco_5th`: `Off`
- `fm_intensity`: `0.00`
- `fm_frequency`: `100.00`
- `fm_phase_reset`: `On`
- `vcf_type`: `1.00`
- `vcf_cutoff`: `5000.00`
- `vcf_resonance`: `0.20`
- `vcf_bend`: `0.00`
- `vcf_env_time`: `100.00`
- `click_style`: `0.60`
- `click_level`: `0.30`
- `noise_color`: `Blue`
- `noise_decay`: `700.00`
- `noise_level`: `0.85`
- `mix_vco`: `0.25`
- `mix_click`: `0.15`
- `mix_noise`: `-0.85`
- `vca_decay`: `700.00`
- `drive`: `0.15`
- `gain`: `-3.00`
- `vel_amount`: `0.70`
- `comp`: `0.00`
- `clip`: `0.00`

Tom:

- `vco_wave`: `0.20`
- `vco_oct`: `0`
- `vco_pitch`: `95.00`
- `vco_range`: `Low`
- `vco_pbend`: `18.00`
- `vco_penv`: `90.00`
- `vco_decay`: `950.00`
- `vco_5th`: `Off`
- `fm_intensity`: `0.00`
- `fm_frequency`: `100.00`
- `fm_phase_reset`: `On`
- `vcf_type`: `0.00`
- `vcf_cutoff`: `8000.00`
- `vcf_resonance`: `0.10`
- `vcf_bend`: `0.00`
- `vcf_env_time`: `100.00`
- `click_style`: `0.30`
- `click_level`: `0.25`
- `noise_color`: `Pink`
- `noise_decay`: `80.00`
- `noise_level`: `0.00`
- `mix_vco`: `0.90`
- `mix_click`: `0.30`
- `mix_noise`: `0.00`
- `vca_decay`: `900.00`
- `drive`: `0.20`
- `gain`: `-3.00`
- `vel_amount`: `0.70`
- `comp`: `0.00`
- `clip`: `0.00`

Floor Tom:

- `vco_wave`: `0.15`
- `vco_oct`: `-1`
- `vco_pitch`: `65.00`
- `vco_range`: `Low`
- `vco_pbend`: `24.00`
- `vco_penv`: `120.00`
- `vco_decay`: `1200.00`
- `vco_5th`: `On`
- `fm_intensity`: `0.00`
- `fm_frequency`: `100.00`
- `fm_phase_reset`: `On`
- `vcf_type`: `0.00`
- `vcf_cutoff`: `6000.00`
- `vcf_resonance`: `0.00`
- `vcf_bend`: `0.00`
- `vcf_env_time`: `150.00`
- `click_style`: `0.25`
- `click_level`: `0.30`
- `noise_color`: `Pink`
- `noise_decay`: `80.00`
- `noise_level`: `0.00`
- `mix_vco`: `0.90`
- `mix_click`: `0.30`
- `mix_noise`: `0.00`
- `vca_decay`: `1100.00`
- `drive`: `0.20`
- `gain`: `-3.00`
- `vel_amount`: `0.60`
- `comp`: `0.00`
- `clip`: `0.00`

Metallic Perc:

- `vco_wave`: `0.55`
- `vco_oct`: `0`
- `vco_pitch`: `200.00`
- `vco_range`: `High`
- `vco_pbend`: `0.00`
- `vco_penv`: `10.00`
- `vco_decay`: `350.00`
- `vco_5th`: `On`
- `fm_intensity`: `0.70`
- `fm_frequency`: `1700.00`
- `fm_phase_reset`: `On`
- `vcf_type`: `0.50`
- `vcf_cutoff`: `6000.00`
- `vcf_resonance`: `0.45`
- `vcf_bend`: `0.00`
- `vcf_env_time`: `100.00`
- `click_style`: `0.50`
- `click_level`: `0.30`
- `noise_color`: `Blue`
- `noise_decay`: `80.00`
- `noise_level`: `0.00`
- `mix_vco`: `-0.80`
- `mix_click`: `0.40`
- `mix_noise`: `0.00`
- `vca_decay`: `280.00`
- `drive`: `0.20`
- `gain`: `-3.00`
- `vel_amount`: `0.70`
- `comp`: `0.00`
- `clip`: `0.05`

Cowbell:

- `vco_wave`: `0.60`
- `vco_oct`: `0`
- `vco_pitch`: `562.00`
- `vco_range`: `Low`
- `vco_pbend`: `0.00`
- `vco_penv`: `5.00`
- `vco_decay`: `400.00`
- `vco_5th`: `On`
- `fm_intensity`: `0.50`
- `fm_frequency`: `845.00`
- `fm_phase_reset`: `On`
- `vcf_type`: `0.50`
- `vcf_cutoff`: `3500.00`
- `vcf_resonance`: `0.50`
- `vcf_bend`: `0.00`
- `vcf_env_time`: `200.00`
- `click_style`: `0.60`
- `click_level`: `0.40`
- `noise_color`: `Blue`
- `noise_decay`: `80.00`
- `noise_level`: `0.00`
- `mix_vco`: `-0.85`
- `mix_click`: `0.50`
- `mix_noise`: `0.00`
- `vca_decay`: `500.00`
- `drive`: `0.15`
- `gain`: `-3.00`
- `vel_amount`: `0.70`
- `comp`: `0.00`
- `clip`: `0.08`

Clap:

- `vco_wave`: `0.00`
- `vco_oct`: `0`
- `vco_pitch`: `200.00`
- `vco_range`: `Low`
- `vco_pbend`: `0.00`
- `vco_penv`: `5.00`
- `vco_decay`: `100.00`
- `vco_5th`: `Off`
- `fm_intensity`: `0.00`
- `fm_frequency`: `100.00`
- `fm_phase_reset`: `On`
- `vcf_type`: `0.50`
- `vcf_cutoff`: `1800.00`
- `vcf_resonance`: `0.35`
- `vcf_bend`: `0.00`
- `vcf_env_time`: `60.00`
- `click_style`: `0.55`
- `click_level`: `0.50`
- `noise_color`: `Pink`
- `noise_decay`: `150.00`
- `noise_level`: `0.90`
- `mix_vco`: `0.00`
- `mix_click`: `0.60`
- `mix_noise`: `-0.90`
- `vca_decay`: `160.00`
- `drive`: `0.30`
- `gain`: `-3.00`
- `vel_amount`: `0.85`
- `comp`: `0.00`
- `clip`: `0.12`
