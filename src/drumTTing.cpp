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
  VCO_OCT,
  VCO_PITCH,
  VCO_RANGE,
  VCO_PBEND,
  VCO_PENV,
  VCO_DECAY,
  VCO_5TH,
  FM_INTENSITY,
  FM_FREQUENCY,
  FM_PHASE_RESET,
  VCF_TYPE,
  VCF_CUTOFF,
  VCF_RESONANCE,
  VCF_BEND,
  VCF_ENV_TIME,
  CLICK_STYLE,
  CLICK_LEVEL,
  NOISE_COLOR,
  NOISE_DECAY,
  NOISE_LEVEL,
  MIX_VCO,
  MIX_CLICK,
  MIX_NOISE,
  VCA_DECAY,
  DRIVE,
  GAIN,
  VEL_AMOUNT,
  COMP,
  CLIP
};

enum ParamIndex : uint32_t {
  P_VCO_WAVE = 0,
  P_VCO_OCT,
  P_VCO_PITCH,
  P_VCO_RANGE,
  P_VCO_PBEND,
  P_VCO_PENV,
  P_VCO_DECAY,
  P_VCO_5TH,
  P_FM_INTENSITY,
  P_FM_FREQUENCY,
  P_FM_PHASE_RESET,
  P_VCF_TYPE,
  P_VCF_CUTOFF,
  P_VCF_RESONANCE,
  P_VCF_BEND,
  P_VCF_ENV_TIME,
  P_CLICK_STYLE,
  P_CLICK_LEVEL,
  P_NOISE_COLOR,
  P_NOISE_DECAY,
  P_NOISE_LEVEL,
  P_MIX_VCO,
  P_MIX_CLICK,
  P_MIX_NOISE,
  P_VCA_DECAY,
  P_DRIVE,
  P_GAIN,
  P_VEL_AMOUNT,
  P_COMP,
  P_CLIP,
  kParamCount
};

struct ParamDef {
  float def;
  float min;
  float max;
};

static const ParamDef kParamDefs[kParamCount] = {
  {0.25f, 0.0f, 1.0f},       // VCO_Wave
  {0.0f, -3.0f, 3.0f},       // VCO_Oct
  {55.0f, 20.0f, 5000.0f},   // VCO_Pitch
  {0.0f, 0.0f, 1.0f},        // VCO_Range
  {36.0f, -48.0f, 48.0f},    // VCO_PBend
  {45.0f, 0.5f, 3000.0f},    // VCO_PEnv
  {700.0f, 1.0f, 5000.0f},   // VCO_Decay
  {0.0f, 0.0f, 1.0f},        // VCO_5th
  {0.0f, 0.0f, 1.0f},        // FM_Intensity
  {100.0f, 0.1f, 8000.0f},   // FM_Frequency
  {1.0f, 0.0f, 1.0f},        // FM_Phase_Reset
  {0.0f, 0.0f, 1.0f},        // VCF_Type
  {12000.0f, 20.0f, 20000.0f}, // VCF_Cutoff
  {0.1f, 0.0f, 1.0f},        // VCF_Resonance
  {0.0f, -48.0f, 48.0f},     // VCF_Bend
  {100.0f, 0.5f, 5000.0f},   // VCF_Env_Time
  {0.4f, 0.0f, 1.0f},        // CLICK_Style
  {0.6f, 0.0f, 1.0f},        // CLICK_Level
  {0.0f, 0.0f, 1.0f},        // NOISE_Color
  {80.0f, 1.0f, 5000.0f},    // NOISE_Decay
  {0.0f, 0.0f, 1.0f},        // NOISE_Level
  {0.9f, -1.0f, 1.0f},       // MIX_VCO
  {0.25f, -1.0f, 1.0f},      // MIX_Click
  {0.0f, -1.0f, 1.0f},       // MIX_Noise
  {700.0f, 1.0f, 8000.0f},   // VCA_Decay
  {0.25f, 0.0f, 1.0f},       // DRIVE
  {-3.0f, -24.0f, 12.0f},    // GAIN
  {0.7f, 0.0f, 1.0f},        // VEL_AMOUNT
  {0.0f, 0.0f, 1.0f},        // COMPRESSOR
  {0.0f, 0.0f, 1.0f}         // CLIP
};

struct Plugin {
  float sr;
  float* out_l;
  float* out_r;
  const float* controls[kParamCount];
  const LV2_Atom_Sequence* midi_in;

  float smooth[kParamCount];

  float vco_phase;
  float vco_5th_phase;
  float fm_phase;
  float click_phase;
  float pitch_env;
  float filter_env;
  float vco_env;
  float noise_env;
  float vca_env;
  float click_env;
  float velocity_gain;

  float svf_lp;
  float svf_bp;
  float pink_b0;
  float pink_b1;
  float pink_b2;
  float last_white;
  float comp_env;
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

static float quantize_2(float x) {
  x = finite_or(x, 0.0f);
  const float scaled = x * 100.0f;
  const int32_t rounded = static_cast<int32_t>(scaled + (scaled >= 0.0f ? 0.5f : -0.5f));
  return static_cast<float>(rounded) * 0.01f;
}

static int nearest_int(float x) {
  x = finite_or(x, 0.0f);
  return static_cast<int>(x + (x >= 0.0f ? 0.5f : -0.5f));
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
  for (uint32_t i = 0; i < kParamCount; ++i) {
    p->controls[i] = nullptr;
    p->smooth[i] = kParamDefs[i].def;
  }
}

static void reset_dsp_state(Plugin* p) {
  if (!p) return;
  p->vco_phase = 0.0f;
  p->vco_5th_phase = 0.0f;
  p->fm_phase = 0.0f;
  p->click_phase = 0.0f;
  p->pitch_env = 0.0f;
  p->filter_env = 0.0f;
  p->vco_env = 0.0f;
  p->noise_env = 0.0f;
  p->vca_env = 0.0f;
  p->click_env = 0.0f;
  p->velocity_gain = 1.0f;
  p->svf_lp = 0.0f;
  p->svf_bp = 0.0f;
  p->pink_b0 = 0.0f;
  p->pink_b1 = 0.0f;
  p->pink_b2 = 0.0f;
  p->last_white = 0.0f;
  p->comp_env = 0.0f;
  p->rng = 0x44564c32u;
}

static void trigger(Plugin* p, int note, float velocity) {
  if (!p) return;
  static_cast<void>(note);
  velocity = clamp(velocity, 0.0f, 1.0f);
  const float vel_amt = clamp(p->smooth[P_VEL_AMOUNT], 0.0f, 1.0f);
  p->velocity_gain = clamp((1.0f - vel_amt) + velocity * vel_amt, 0.0f, 1.0f);
  p->pitch_env = 1.0f;
  p->filter_env = 1.0f;
  p->vco_env = 1.0f;
  p->noise_env = 1.0f;
  p->vca_env = 1.0f;
  p->click_env = 1.0f;
  p->vco_phase = 0.0f;
  p->vco_5th_phase = 0.0f;
  p->click_phase = 0.0f;
  if (control_switch(p->smooth[P_FM_PHASE_RESET])) p->fm_phase = 0.0f;
  p->svf_lp = 0.0f;
  p->svf_bp = 0.0f;
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
  p->pink_b0 = zap(clamp(0.99765f * p->pink_b0 + white * 0.0990460f, -16.0f, 16.0f));
  p->pink_b1 = zap(clamp(0.96300f * p->pink_b1 + white * 0.2965164f, -16.0f, 16.0f));
  p->pink_b2 = zap(clamp(0.57000f * p->pink_b2 + white * 1.0526913f, -16.0f, 16.0f));
  const float pink = clamp((p->pink_b0 + p->pink_b1 + p->pink_b2 + white * 0.1848f) * 0.11f, -1.0f, 1.0f);
  const float blue = clamp((white - p->last_white) * 0.55f, -1.0f, 1.0f);
  p->last_white = white;
  return color ? blue : pink;
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

static float process_compressor(Plugin* p, float x, float amount, float attack_coef, float release_coef) {
  amount = clamp(amount, 0.0f, 1.0f);
  x = finite_or(x, 0.0f);

  const float peak = clamp(fabsf(x), 0.0f, 8.0f);
  const float coeff = peak > p->comp_env ? attack_coef : release_coef;
  p->comp_env = zap(clamp(peak + coeff * (p->comp_env - peak), 0.0f, 8.0f));

  if (amount <= 0.0001f) return x;

  const float threshold = db_to_gain(-3.0f - amount * 21.0f);
  float gain = 1.0f;
  if (p->comp_env > threshold && p->comp_env > 0.000001f) {
    const float compressed = threshold * finite_or(powf(p->comp_env / threshold, 0.25f), 1.0f);
    gain = clamp(compressed / p->comp_env, 0.0f, 1.0f);
  }

  const float makeup = db_to_gain(amount * 6.0f);
  return finite_or(x * clamp(gain * makeup, 0.0f, 2.0f), 0.0f);
}

static float process_clipper(float x, float amount) {
  amount = clamp(amount, 0.0f, 1.0f);
  x = finite_or(x, 0.0f);
  if (amount <= 0.0001f) return x;

  const float pregain = 1.0f + amount * 8.0f;
  const float ceiling = 1.0f - amount * 0.65f;
  float y = clamp(x * pregain, -ceiling, ceiling);
  y /= ceiling;
  return finite_or(y, 0.0f);
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
    case VCO_OCT: p->controls[P_VCO_OCT] = static_cast<const float*>(data); break;
    case VCO_PITCH: p->controls[P_VCO_PITCH] = static_cast<const float*>(data); break;
    case VCO_RANGE: p->controls[P_VCO_RANGE] = static_cast<const float*>(data); break;
    case VCO_PBEND: p->controls[P_VCO_PBEND] = static_cast<const float*>(data); break;
    case VCO_PENV: p->controls[P_VCO_PENV] = static_cast<const float*>(data); break;
    case VCO_DECAY: p->controls[P_VCO_DECAY] = static_cast<const float*>(data); break;
    case VCO_5TH: p->controls[P_VCO_5TH] = static_cast<const float*>(data); break;
    case FM_INTENSITY: p->controls[P_FM_INTENSITY] = static_cast<const float*>(data); break;
    case FM_FREQUENCY: p->controls[P_FM_FREQUENCY] = static_cast<const float*>(data); break;
    case FM_PHASE_RESET: p->controls[P_FM_PHASE_RESET] = static_cast<const float*>(data); break;
    case VCF_TYPE: p->controls[P_VCF_TYPE] = static_cast<const float*>(data); break;
    case VCF_CUTOFF: p->controls[P_VCF_CUTOFF] = static_cast<const float*>(data); break;
    case VCF_RESONANCE: p->controls[P_VCF_RESONANCE] = static_cast<const float*>(data); break;
    case VCF_BEND: p->controls[P_VCF_BEND] = static_cast<const float*>(data); break;
    case VCF_ENV_TIME: p->controls[P_VCF_ENV_TIME] = static_cast<const float*>(data); break;
    case CLICK_STYLE: p->controls[P_CLICK_STYLE] = static_cast<const float*>(data); break;
    case CLICK_LEVEL: p->controls[P_CLICK_LEVEL] = static_cast<const float*>(data); break;
    case NOISE_COLOR: p->controls[P_NOISE_COLOR] = static_cast<const float*>(data); break;
    case NOISE_DECAY: p->controls[P_NOISE_DECAY] = static_cast<const float*>(data); break;
    case NOISE_LEVEL: p->controls[P_NOISE_LEVEL] = static_cast<const float*>(data); break;
    case MIX_VCO: p->controls[P_MIX_VCO] = static_cast<const float*>(data); break;
    case MIX_CLICK: p->controls[P_MIX_CLICK] = static_cast<const float*>(data); break;
    case MIX_NOISE: p->controls[P_MIX_NOISE] = static_cast<const float*>(data); break;
    case VCA_DECAY: p->controls[P_VCA_DECAY] = static_cast<const float*>(data); break;
    case DRIVE: p->controls[P_DRIVE] = static_cast<const float*>(data); break;
    case GAIN: p->controls[P_GAIN] = static_cast<const float*>(data); break;
    case VEL_AMOUNT: p->controls[P_VEL_AMOUNT] = static_cast<const float*>(data); break;
    case COMP: p->controls[P_COMP] = static_cast<const float*>(data); break;
    case CLIP: p->controls[P_CLIP] = static_cast<const float*>(data); break;
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
  const float pitch_coef = time_coef_ms(p, p->smooth[P_VCO_PENV]);
  const float filter_coef = time_coef_ms(p, p->smooth[P_VCF_ENV_TIME]);
  const float vco_coef = time_coef_ms(p, p->smooth[P_VCO_DECAY]);
  const float noise_coef = time_coef_ms(p, p->smooth[P_NOISE_DECAY]);
  const float vca_coef = time_coef_ms(p, p->smooth[P_VCA_DECAY]);
  const float click_time = 1.5f + p->smooth[P_CLICK_STYLE] * p->smooth[P_CLICK_STYLE] * 12.0f;
  const float click_coef = time_coef_ms(p, click_time);
  const float comp_amount_for_coeff = clamp(p->smooth[P_COMP], 0.0f, 1.0f);
  const float comp_attack_coef = time_coef_ms(p, 10.0f - comp_amount_for_coeff * 9.0f);
  const float comp_release_coef = time_coef_ms(p, 250.0f - comp_amount_for_coeff * 200.0f);

  for (uint32_t i = 0; i < n; ++i) {
    for (uint32_t pidx = 0; pidx < kParamCount; ++pidx) {
      const ParamDef* def = &kParamDefs[pidx];
      const float target = quantize_2(clamp(p->controls[pidx] ? *p->controls[pidx] : def->def, def->min, def->max));
      p->smooth[pidx] += (target - p->smooth[pidx]) * smooth_coeff;
      p->smooth[pidx] = zap(clamp(p->smooth[pidx], def->min, def->max));
    }

    const float fm_hz = clamp(p->smooth[P_FM_FREQUENCY], 0.1f, sr * 0.45f);
    p->fm_phase = wrap_phase(p->fm_phase + kTwoPi * fm_hz / sr);
    const float fm_sine = sinf(p->fm_phase);
    const float fm_depth = p->smooth[P_FM_INTENSITY] * p->smooth[P_FM_INTENSITY] * 24.0f;
    const float range_mul = control_switch(p->smooth[P_VCO_RANGE]) ? 8.0f : 1.0f;
    const float octave_semis = clamp(static_cast<float>(nearest_int(p->smooth[P_VCO_OCT])), -3.0f, 3.0f) * 12.0f;
    const float pitch_semis = octave_semis + p->smooth[P_VCO_PBEND] * p->pitch_env + fm_sine * fm_depth;
    float vco_hz = p->smooth[P_VCO_PITCH] * range_mul * semitone_ratio(pitch_semis);
    vco_hz = clamp(vco_hz, 1.0f, sr * 0.45f);

    const float filter_semis = p->smooth[P_VCF_BEND] * p->filter_env;
    float cutoff = p->smooth[P_VCF_CUTOFF] * semitone_ratio(filter_semis);
    cutoff = clamp(cutoff, 10.0f, sr * 0.45f);

    float vco = render_vco(p, vco_hz, p->smooth[P_VCO_WAVE]);
    if (control_switch(p->smooth[P_VCO_5TH])) {
      p->vco_5th_phase = wrap_phase(p->vco_5th_phase + kTwoPi * (vco_hz * 1.5f) / sr);
      vco += sinf(p->vco_5th_phase) * 0.355f;
    }
    vco *= p->vco_env;
    const int noise_color = control_switch(p->smooth[P_NOISE_COLOR]);
    const float noise = render_noise(p, noise_color) * p->noise_env * p->smooth[P_NOISE_LEVEL] * (0.8f + 0.2f * p->velocity_gain);
    const float click = render_click(p, p->smooth[P_CLICK_STYLE]) * p->smooth[P_CLICK_LEVEL] * (0.75f + 0.25f * p->velocity_gain);

    float filter_bus = 0.0f;
    float direct_bus = 0.0f;
    route_source(vco, p->smooth[P_MIX_VCO], &filter_bus, &direct_bus);
    route_source(click, p->smooth[P_MIX_CLICK], &filter_bus, &direct_bus);
    route_source(noise, p->smooth[P_MIX_NOISE], &filter_bus, &direct_bus);

    const float filtered = process_filter(p, filter_bus, cutoff, p->smooth[P_VCF_RESONANCE], p->smooth[P_VCF_TYPE]);
    float y = (filtered + direct_bus) * p->vca_env * p->velocity_gain;

    const float drive = clamp(p->smooth[P_DRIVE], 0.0f, 1.0f);
    if (drive > 0.0001f) {
      y = soft_clip(y * (1.0f + drive * 8.0f)) / (1.0f + drive * 0.75f);
    }
    y = process_compressor(p, y, p->smooth[P_COMP], comp_attack_coef, comp_release_coef);
    y *= db_to_gain(p->smooth[P_GAIN]);
    y = process_clipper(y, p->smooth[P_CLIP]);
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
