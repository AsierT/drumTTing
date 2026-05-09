#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lv2/core/lv2.h"
#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"

static const char* kUri = "urn:asier:lv2:drumtting";

enum PortIndex : uint32_t {
  MIDI_IN = 0,
  OUT_L,
  OUT_R,
  VCO_WAVE,
  VCO_PITCH,
  VCO_RANGE,
  PITCH_BEND,
  PITCH_ENV_TIME,
  VCO_DECAY,
  FM_INTENSITY,
  FM_FREQUENCY,
  FM_PHASE_RESET,
  CUTOFF,
  RESONANCE,
  FILTER_TYPE,
  FILTER_BEND,
  FILTER_ENV_TIME,
  FILTER_SH_AMOUNT,
  FILTER_SH_SMOOTH,
  CLICK_ROUTE,
  NOISE_ROUTE,
  VCO_ROUTE,
  CLICK_STYLE,
  CLICK_LEVEL,
  NOISE_COLOR,
  NOISE_DECAY,
  NOISE_LEVEL,
  VCA_DECAY,
  DRIVE,
  OUTPUT_GAIN,
  VELOCITY_AMOUNT
};

enum ParamIndex : uint32_t {
  P_VCO_WAVE = 0,
  P_VCO_PITCH,
  P_VCO_RANGE,
  P_PITCH_BEND,
  P_PITCH_ENV_TIME,
  P_VCO_DECAY,
  P_FM_INTENSITY,
  P_FM_FREQUENCY,
  P_FM_PHASE_RESET,
  P_CUTOFF,
  P_RESONANCE,
  P_FILTER_TYPE,
  P_FILTER_BEND,
  P_FILTER_ENV_TIME,
  P_FILTER_SH_AMOUNT,
  P_FILTER_SH_SMOOTH,
  P_CLICK_ROUTE,
  P_NOISE_ROUTE,
  P_VCO_ROUTE,
  P_CLICK_STYLE,
  P_CLICK_LEVEL,
  P_NOISE_COLOR,
  P_NOISE_DECAY,
  P_NOISE_LEVEL,
  P_VCA_DECAY,
  P_DRIVE,
  P_OUTPUT_GAIN,
  P_VELOCITY_AMOUNT,
  kParamCount
};

struct ParamDef {
  float def;
  float min;
  float max;
};

static const ParamDef kParamDefs[kParamCount] = {
  {0.25f, 0.0f, 1.0f},       // vco_wave
  {55.0f, 20.0f, 5000.0f},   // vco_pitch
  {0.0f, 0.0f, 1.0f},        // vco_range
  {36.0f, -48.0f, 48.0f},    // pitch_bend
  {45.0f, 0.5f, 3000.0f},    // pitch_env_time
  {700.0f, 1.0f, 5000.0f},   // vco_decay
  {0.0f, 0.0f, 1.0f},        // fm_intensity
  {100.0f, 0.1f, 8000.0f},   // fm_frequency
  {1.0f, 0.0f, 1.0f},        // fm_phase_reset
  {12000.0f, 20.0f, 20000.0f}, // cutoff
  {0.1f, 0.0f, 1.0f},        // resonance
  {0.0f, 0.0f, 1.0f},        // filter_type
  {0.0f, -48.0f, 48.0f},     // filter_bend
  {100.0f, 0.5f, 5000.0f},   // filter_env_time
  {0.0f, 0.0f, 1.0f},        // filter_sh_amount
  {0.2f, 0.0f, 1.0f},        // filter_sh_smooth
  {0.25f, -1.0f, 1.0f},      // click_route
  {0.0f, -1.0f, 1.0f},       // noise_route
  {0.9f, -1.0f, 1.0f},       // vco_route
  {0.4f, 0.0f, 1.0f},        // click_style
  {0.6f, 0.0f, 1.0f},        // click_level
  {0.0f, 0.0f, 1.0f},        // noise_color
  {80.0f, 1.0f, 5000.0f},    // noise_decay
  {0.0f, 0.0f, 1.0f},        // noise_level
  {700.0f, 1.0f, 8000.0f},   // vca_decay
  {0.25f, 0.0f, 1.0f},       // drive
  {-6.0f, -24.0f, 12.0f},    // output_gain
  {0.7f, 0.0f, 1.0f}         // velocity_amount
};

struct Plugin {
  float sr;
  float* out_l;
  float* out_r;
  const float* controls[kParamCount];
  const LV2_Atom_Sequence* midi_in;

  float smooth[kParamCount];

  float vco_phase;
  float fm_phase;
  float click_phase;
  float pitch_env;
  float filter_env;
  float vco_env;
  float noise_env;
  float vca_env;
  float click_env;
  float velocity_gain;
  float note_ratio;
  float sh_target;
  float sh_current;

  float svf_lp;
  float svf_bp;
  float pink;
  float last_white;
  uint32_t rng;
};

static constexpr float kPi = 3.14159265358979323846f;
static constexpr float kTwoPi = 6.28318530717958647692f;
static constexpr float kRefSampleRate = 48000.0f;

static bool finite_float(float x) {
  return __builtin_isfinite(x);
}

static bool finite_double(double x) {
  return __builtin_isfinite(x);
}

static float finite_or(float x, float fallback) {
  return finite_float(x) ? x : fallback;
}

static float clamp(float x, float lo, float hi) {
  x = finite_or(x, lo);
  return x < lo ? lo : (x > hi ? hi : x);
}

static float sample_rate(const Plugin* p) {
  return (p && finite_float(p->sr) && p->sr >= 1000.0f && p->sr <= 384000.0f) ? p->sr : kRefSampleRate;
}

static float zap(float x) {
  x = finite_or(x, 0.0f);
  return fabsf(x) < 1.0e-20f ? 0.0f : x;
}

static float wrap_phase(float x) {
  if (!finite_float(x)) return 0.0f;
  while (x >= kTwoPi) x -= kTwoPi;
  while (x < 0.0f) x += kTwoPi;
  return x;
}

static float semitone_ratio(float semitones) {
  return finite_or(powf(2.0f, semitones / 12.0f), 1.0f);
}

static float db_to_gain(float db) {
  return finite_or(powf(10.0f, db / 20.0f), 1.0f);
}

static float time_coef_ms(const Plugin* p, float ms) {
  const float sr = sample_rate(p);
  ms = clamp(ms, 0.1f, 10000.0f);
  return finite_or(expf(-1.0f / (0.001f * ms * sr)), 0.0f);
}

static uint32_t next_u32(Plugin* p) {
  p->rng = p->rng * 1664525u + 1013904223u;
  return p->rng;
}

static float frand(Plugin* p) {
  return ((next_u32(p) >> 8) & 0xFFFFu) / 32768.0f - 1.0f;
}

static float soft_clip(float x) {
  x = finite_or(x, 0.0f);
  return finite_or(tanhf(x), 0.0f);
}

static int control_switch(float x) {
  return x >= 0.5f ? 1 : 0;
}

static void route_source(float source, float route, float* filter_bus, float* direct_bus) {
  route = clamp(route, -1.0f, 1.0f);
  if (route < 0.0f) {
    *filter_bus += source * -route;
  } else {
    *direct_bus += source * route;
  }
}

static void init_plugin(Plugin* p, float sr) {
  memset(p, 0, sizeof(Plugin));
  p->sr = sr;
  p->rng = 0x44564c32u;
  p->velocity_gain = 1.0f;
  p->note_ratio = 1.0f;
  for (uint32_t i = 0; i < kParamCount; ++i) {
    p->controls[i] = nullptr;
    p->smooth[i] = kParamDefs[i].def;
  }
}

static void reset_dsp_state(Plugin* p) {
  if (!p) return;
  p->vco_phase = 0.0f;
  p->fm_phase = 0.0f;
  p->click_phase = 0.0f;
  p->pitch_env = 0.0f;
  p->filter_env = 0.0f;
  p->vco_env = 0.0f;
  p->noise_env = 0.0f;
  p->vca_env = 0.0f;
  p->click_env = 0.0f;
  p->velocity_gain = 1.0f;
  p->note_ratio = 1.0f;
  p->sh_target = 0.0f;
  p->sh_current = 0.0f;
  p->svf_lp = 0.0f;
  p->svf_bp = 0.0f;
  p->pink = 0.0f;
  p->last_white = 0.0f;
  p->rng = 0x44564c32u;
}

static void trigger(Plugin* p, int note, float velocity) {
  if (!p) return;
  velocity = clamp(velocity, 0.0f, 1.0f);
  const float vel_amt = clamp(p->smooth[P_VELOCITY_AMOUNT], 0.0f, 1.0f);
  p->velocity_gain = clamp((1.0f - vel_amt) + velocity * vel_amt, 0.0f, 1.0f);
  p->note_ratio = semitone_ratio(static_cast<float>(note - 36));
  p->pitch_env = 1.0f;
  p->filter_env = 1.0f;
  p->vco_env = 1.0f;
  p->noise_env = 1.0f;
  p->vca_env = 1.0f;
  p->click_env = 1.0f;
  p->vco_phase = 0.0f;
  p->click_phase = 0.0f;
  if (control_switch(p->smooth[P_FM_PHASE_RESET])) p->fm_phase = 0.0f;
  p->svf_lp = 0.0f;
  p->svf_bp = 0.0f;

  const float sh_amount = clamp(p->smooth[P_FILTER_SH_AMOUNT], 0.0f, 1.0f);
  p->sh_target = frand(p) * sh_amount * 36.0f;
  if (p->smooth[P_FILTER_SH_SMOOTH] <= 0.001f) p->sh_current = p->sh_target;
}

static void handle_midi(Plugin* p) {
  if (!p || !p->midi_in) return;
  if (p->midi_in->atom.size < 8u) return;

  LV2_ATOM_SEQUENCE_FOREACH(p->midi_in, ev) {
    const uint8_t* msg = reinterpret_cast<const uint8_t*>(ev + 1);
    if (ev->body.size < 3u) continue;
    const uint8_t status = msg[0] & 0xF0u;
    if (status == 0x90u && msg[2] > 0u) {
      trigger(p, static_cast<int>(msg[1]), clamp(static_cast<float>(msg[2]) / 127.0f, 0.0f, 1.0f));
    }
  }
}

static float render_vco(Plugin* p, float hz, float wave) {
  const float sr = sample_rate(p);
  hz = clamp(hz, 1.0f, sr * 0.45f);
  wave = clamp(wave, 0.0f, 1.0f);

  p->vco_phase = wrap_phase(p->vco_phase + kTwoPi * hz / sr);
  const float s1 = sinf(p->vco_phase);
  const float s2 = sinf(2.0f * p->vco_phase);
  const float s3 = sinf(3.0f * p->vco_phase);
  const float harmonic = s1 + s2 * (0.25f * wave) + s3 * (0.15f * wave * wave);
  const float gain = 1.0f + wave * wave * 12.0f;
  const float shaped = soft_clip(harmonic * gain);
  const float blend = wave * (0.35f + 0.65f * wave);
  return finite_or(s1 * (1.0f - blend) + shaped * blend, 0.0f);
}

static float render_noise(Plugin* p, int color) {
  const float white = frand(p);
  p->pink += 0.055f * (white - p->pink);
  p->pink = zap(clamp(p->pink, -2.0f, 2.0f));
  const float blue = clamp((white - p->last_white) * 0.55f, -1.0f, 1.0f);
  p->last_white = white;
  return color ? blue : p->pink * 1.8f;
}

static float render_click(Plugin* p, float style) {
  const float sr = sample_rate(p);
  style = clamp(style, 0.0f, 1.0f);
  const float hz = 70.0f + style * style * 7800.0f;
  p->click_phase = wrap_phase(p->click_phase + kTwoPi * hz / sr);
  const float impulse = sinf(p->click_phase) * (1.0f - style * 0.55f);
  const float tick = frand(p) * (0.15f + style * 0.85f);
  return finite_or((impulse + tick) * p->click_env, 0.0f);
}

static float process_filter(Plugin* p, float x, float cutoff_hz, float resonance, float morph) {
  const float sr = sample_rate(p);
  x = finite_or(x, 0.0f);
  cutoff_hz = clamp(cutoff_hz, 10.0f, sr * 0.45f);
  resonance = clamp(resonance, 0.0f, 1.0f);
  morph = clamp(morph, 0.0f, 1.0f);

  const float f = clamp(2.0f * sinf(kPi * cutoff_hz / sr), 0.00001f, 0.95f);
  const float damping = clamp(1.55f - resonance * 1.45f, 0.10f, 1.55f);

  p->svf_lp += f * p->svf_bp;
  float hp = x - p->svf_lp - damping * p->svf_bp;
  p->svf_bp += f * hp;

  p->svf_lp = zap(clamp(p->svf_lp, -8.0f, 8.0f));
  p->svf_bp = zap(clamp(p->svf_bp, -8.0f, 8.0f));
  hp = zap(clamp(hp, -8.0f, 8.0f));

  if (morph <= 0.5f) {
    const float t = morph * 2.0f;
    return finite_or(p->svf_lp + (p->svf_bp - p->svf_lp) * t, 0.0f);
  }

  const float t = (morph - 0.5f) * 2.0f;
  return finite_or(p->svf_bp + (hp - p->svf_bp) * t, 0.0f);
}

static LV2_Handle instantiate(const LV2_Descriptor*, double rate, const char*, const LV2_Feature* const*) {
  Plugin* p = static_cast<Plugin*>(malloc(sizeof(Plugin)));
  if (!p) return nullptr;
  const float sr = (finite_double(rate) && rate >= 1000.0 && rate <= 384000.0) ? static_cast<float>(rate) : kRefSampleRate;
  init_plugin(p, sr);
  return p;
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
  Plugin* p = static_cast<Plugin*>(instance);
  if (!p) return;

  switch (port) {
    case OUT_L: p->out_l = static_cast<float*>(data); break;
    case OUT_R: p->out_r = static_cast<float*>(data); break;
    case VCO_WAVE: p->controls[P_VCO_WAVE] = static_cast<const float*>(data); break;
    case VCO_PITCH: p->controls[P_VCO_PITCH] = static_cast<const float*>(data); break;
    case VCO_RANGE: p->controls[P_VCO_RANGE] = static_cast<const float*>(data); break;
    case PITCH_BEND: p->controls[P_PITCH_BEND] = static_cast<const float*>(data); break;
    case PITCH_ENV_TIME: p->controls[P_PITCH_ENV_TIME] = static_cast<const float*>(data); break;
    case VCO_DECAY: p->controls[P_VCO_DECAY] = static_cast<const float*>(data); break;
    case FM_INTENSITY: p->controls[P_FM_INTENSITY] = static_cast<const float*>(data); break;
    case FM_FREQUENCY: p->controls[P_FM_FREQUENCY] = static_cast<const float*>(data); break;
    case FM_PHASE_RESET: p->controls[P_FM_PHASE_RESET] = static_cast<const float*>(data); break;
    case CUTOFF: p->controls[P_CUTOFF] = static_cast<const float*>(data); break;
    case RESONANCE: p->controls[P_RESONANCE] = static_cast<const float*>(data); break;
    case FILTER_TYPE: p->controls[P_FILTER_TYPE] = static_cast<const float*>(data); break;
    case FILTER_BEND: p->controls[P_FILTER_BEND] = static_cast<const float*>(data); break;
    case FILTER_ENV_TIME: p->controls[P_FILTER_ENV_TIME] = static_cast<const float*>(data); break;
    case FILTER_SH_AMOUNT: p->controls[P_FILTER_SH_AMOUNT] = static_cast<const float*>(data); break;
    case FILTER_SH_SMOOTH: p->controls[P_FILTER_SH_SMOOTH] = static_cast<const float*>(data); break;
    case CLICK_ROUTE: p->controls[P_CLICK_ROUTE] = static_cast<const float*>(data); break;
    case NOISE_ROUTE: p->controls[P_NOISE_ROUTE] = static_cast<const float*>(data); break;
    case VCO_ROUTE: p->controls[P_VCO_ROUTE] = static_cast<const float*>(data); break;
    case CLICK_STYLE: p->controls[P_CLICK_STYLE] = static_cast<const float*>(data); break;
    case CLICK_LEVEL: p->controls[P_CLICK_LEVEL] = static_cast<const float*>(data); break;
    case NOISE_COLOR: p->controls[P_NOISE_COLOR] = static_cast<const float*>(data); break;
    case NOISE_DECAY: p->controls[P_NOISE_DECAY] = static_cast<const float*>(data); break;
    case NOISE_LEVEL: p->controls[P_NOISE_LEVEL] = static_cast<const float*>(data); break;
    case VCA_DECAY: p->controls[P_VCA_DECAY] = static_cast<const float*>(data); break;
    case DRIVE: p->controls[P_DRIVE] = static_cast<const float*>(data); break;
    case OUTPUT_GAIN: p->controls[P_OUTPUT_GAIN] = static_cast<const float*>(data); break;
    case VELOCITY_AMOUNT: p->controls[P_VELOCITY_AMOUNT] = static_cast<const float*>(data); break;
    case MIDI_IN: p->midi_in = static_cast<const LV2_Atom_Sequence*>(data); break;
  }
}

static void activate(LV2_Handle instance) {
  reset_dsp_state(static_cast<Plugin*>(instance));
}

static void run(LV2_Handle instance, uint32_t n) {
  Plugin* p = static_cast<Plugin*>(instance);
  if (!p || !p->out_l || !p->out_r) return;

  handle_midi(p);

  const float sr = sample_rate(p);
  const float smooth_coeff = 1.0f - time_coef_ms(p, 8.0f);
  const float pitch_coef = time_coef_ms(p, p->smooth[P_PITCH_ENV_TIME]);
  const float filter_coef = time_coef_ms(p, p->smooth[P_FILTER_ENV_TIME]);
  const float vco_coef = time_coef_ms(p, p->smooth[P_VCO_DECAY]);
  const float noise_coef = time_coef_ms(p, p->smooth[P_NOISE_DECAY]);
  const float vca_coef = time_coef_ms(p, p->smooth[P_VCA_DECAY]);
  const float click_time = 1.5f + p->smooth[P_CLICK_STYLE] * p->smooth[P_CLICK_STYLE] * 12.0f;
  const float click_coef = time_coef_ms(p, click_time);

  for (uint32_t i = 0; i < n; ++i) {
    for (uint32_t pidx = 0; pidx < kParamCount; ++pidx) {
      const ParamDef* def = &kParamDefs[pidx];
      const float target = clamp(p->controls[pidx] ? *p->controls[pidx] : def->def, def->min, def->max);
      p->smooth[pidx] += (target - p->smooth[pidx]) * smooth_coeff;
      p->smooth[pidx] = zap(clamp(p->smooth[pidx], def->min, def->max));
    }

    const float fm_hz = clamp(p->smooth[P_FM_FREQUENCY], 0.1f, sr * 0.45f);
    p->fm_phase = wrap_phase(p->fm_phase + kTwoPi * fm_hz / sr);
    const float fm_sine = sinf(p->fm_phase);
    const float fm_depth = p->smooth[P_FM_INTENSITY] * p->smooth[P_FM_INTENSITY] * 48.0f;
    const float range_mul = control_switch(p->smooth[P_VCO_RANGE]) ? 8.0f : 1.0f;
    const float pitch_semis = p->smooth[P_PITCH_BEND] * p->pitch_env + fm_sine * fm_depth;
    float vco_hz = p->smooth[P_VCO_PITCH] * range_mul * p->note_ratio * semitone_ratio(pitch_semis);
    vco_hz = clamp(vco_hz, 1.0f, sr * 0.45f);

    const float sh_smooth = clamp(p->smooth[P_FILTER_SH_SMOOTH], 0.0f, 1.0f);
    const float sh_step = sh_smooth <= 0.001f ? 1.0f : (1.0f - time_coef_ms(p, 1.0f + sh_smooth * sh_smooth * 250.0f));
    p->sh_current += (p->sh_target - p->sh_current) * sh_step;
    const float filter_semis = p->smooth[P_FILTER_BEND] * p->filter_env + p->sh_current;
    float cutoff = p->smooth[P_CUTOFF] * semitone_ratio(filter_semis);
    cutoff = clamp(cutoff, 10.0f, sr * 0.45f);

    const float vco = render_vco(p, vco_hz, p->smooth[P_VCO_WAVE]) * p->vco_env;
    const int noise_color = control_switch(p->smooth[P_NOISE_COLOR]);
    const float noise = render_noise(p, noise_color) * p->noise_env * p->smooth[P_NOISE_LEVEL] * (0.8f + 0.2f * p->velocity_gain);
    const float click = render_click(p, p->smooth[P_CLICK_STYLE]) * p->smooth[P_CLICK_LEVEL] * (0.75f + 0.25f * p->velocity_gain);

    float filter_bus = 0.0f;
    float direct_bus = 0.0f;
    route_source(click, p->smooth[P_CLICK_ROUTE], &filter_bus, &direct_bus);
    route_source(noise, p->smooth[P_NOISE_ROUTE], &filter_bus, &direct_bus);
    route_source(vco, p->smooth[P_VCO_ROUTE], &filter_bus, &direct_bus);

    const float filtered = process_filter(p, filter_bus, cutoff, p->smooth[P_RESONANCE], p->smooth[P_FILTER_TYPE]);
    float y = (filtered + direct_bus) * p->vca_env * p->velocity_gain;

    const float drive = clamp(p->smooth[P_DRIVE], 0.0f, 1.0f);
    const float out_gain = db_to_gain(p->smooth[P_OUTPUT_GAIN]);
    y *= out_gain;
    if (drive > 0.0001f) {
      y = soft_clip(y * (1.0f + drive * 8.0f)) / (1.0f + drive * 0.75f);
    }
    y = clamp(finite_or(y, 0.0f), -1.0f, 1.0f);

    p->out_l[i] = y;
    p->out_r[i] = y;

    p->pitch_env = zap(p->pitch_env * pitch_coef);
    p->filter_env = zap(p->filter_env * filter_coef);
    p->vco_env = zap(p->vco_env * vco_coef);
    p->noise_env = zap(p->noise_env * noise_coef);
    p->vca_env = zap(p->vca_env * vca_coef);
    p->click_env = zap(p->click_env * click_coef);
  }
}

static void cleanup(LV2_Handle instance) {
  free(instance);
}

static const LV2_Descriptor descriptor = {
  kUri, instantiate, connect_port, activate, run, nullptr, cleanup, nullptr
};

extern "C" LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
  return index == 0 ? &descriptor : nullptr;
}
