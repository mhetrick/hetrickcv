#include "HetrickCV.hpp"

#include "DSP/Phasors/HCVPhasorCommon.h"
#include "Gamma/scl.h"
#include <cmath>

/// Wavetable Mipmap Anti-Aliasing
///
/// Pre-computed band-limited wavetables at multiple octaves.
/// Each mipmap level contains fewer harmonics, preventing aliasing at higher
/// frequencies. The instantaneous frequency (from phase derivative) determines
/// which mipmap level(s) to use.

// Wavetable configuration
static constexpr int TABLE_SIZE = 2048;
static constexpr int NUM_MIPMAP_LEVELS = 10;

/// Static wavetable storage - shared across all instances
struct MipmapTables {
    float saw[NUM_MIPMAP_LEVELS][TABLE_SIZE];
    float square[NUM_MIPMAP_LEVELS][TABLE_SIZE];
    float triangle[NUM_MIPMAP_LEVELS][TABLE_SIZE];
    bool initialized = false;

    void initialize() {
        if (initialized) return;

        for (int level = 0; level < NUM_MIPMAP_LEVELS; level++) {
            int maxHarmonics = TABLE_SIZE / 2;
            int numHarmonics = maxHarmonics >> level;
            if (numHarmonics < 1) numHarmonics = 1;

            generateSawTable(level, numHarmonics);
            generateSquareTable(level, numHarmonics);
            generateTriangleTable(level, numHarmonics);
        }

        initialized = true;
    }

    void generateSawTable(int level, int numHarmonics) {
        for (int i = 0; i < TABLE_SIZE; i++) {
            float phase = (float)i / TABLE_SIZE;
            float sample = 0.0f;

            for (int h = 1; h <= numHarmonics; h++) {
                float harmPhase = phase * h;
                harmPhase = harmPhase - std::floor(harmPhase);
                sample -= std::sin(harmPhase * M_PI * 2.0f) / h;
            }

            saw[level][i] = sample * (2.0f / M_PI);
        }

        normalizeTable(saw[level]);
    }

    void generateSquareTable(int level, int numHarmonics) {
        for (int i = 0; i < TABLE_SIZE; i++) {
            float phase = (float)i / TABLE_SIZE;
            float sample = 0.0f;

            for (int h = 1; h <= numHarmonics; h += 2) {
                float harmPhase = phase * h;
                harmPhase = harmPhase - std::floor(harmPhase);
                sample += std::sin(harmPhase * M_PI * 2.0f) / h;
            }

            square[level][i] = sample * (4.0f / M_PI);
        }

        normalizeTable(square[level]);
    }

    void generateTriangleTable(int level, int numHarmonics) {
        for (int i = 0; i < TABLE_SIZE; i++) {
            float phase = (float)i / TABLE_SIZE;
            float sample = 0.0f;

            int sign = 1;
            for (int h = 1; h <= numHarmonics; h += 2) {
                float harmPhase = phase * h;
                harmPhase = harmPhase - std::floor(harmPhase);
                sample += sign * std::sin(harmPhase * M_PI * 2.0f) / (h * h);
                sign = -sign;
            }

            triangle[level][i] = sample * (8.0f / (M_PI * M_PI));
        }

        normalizeTable(triangle[level]);
    }

    void normalizeTable(float* table) {
        float maxVal = 0.0f;
        for (int i = 0; i < TABLE_SIZE; i++) {
            maxVal = std::fmax(maxVal, std::fabs(table[i]));
        }
        if (maxVal > 0.0f) {
            float scale = 1.0f / maxVal;
            for (int i = 0; i < TABLE_SIZE; i++) {
                table[i] *= scale;
            }
        }
    }
};

static MipmapTables g_mipmapTables;


struct PhasorToWaveforms : HCVModule
{
	enum ParamIds
	{
        ANTIALIAS_PARAM,
		NUM_PARAMS
	};
	enum InputIds
	{
        PHASOR_INPUT,
		NUM_INPUTS
	};
	enum OutputIds
	{
        SINE_OUTPUT,
        TRIANGLE_OUTPUT,
        SAW_OUTPUT,
        RAMP_OUTPUT,
        SQUARE_OUTPUT,

        SINE_BIPOLAR_OUTPUT,
        TRIANGLE_BIPOLAR_OUTPUT,
        SAW_BIPOLAR_OUTPUT,
        RAMP_BIPOLAR_OUTPUT,
        SQUARE_BIPOLAR_OUTPUT,
		NUM_OUTPUTS
    };
    enum LightIds
    {
        SINE_LIGHT,
        TRIANGLE_LIGHT,
        SAW_LIGHT,
        RAMP_LIGHT,
        SQUARE_LIGHT,

        ENUMS(SINE_BIPOLAR_LIGHT, 2),
        ENUMS(TRIANGLE_BIPOLAR_LIGHT, 2),
        ENUMS(SAW_BIPOLAR_LIGHT, 2),
        ENUMS(RAMP_BIPOLAR_LIGHT, 2),
        ENUMS(SQUARE_BIPOLAR_LIGHT, 2),
        NUM_LIGHTS
	};

    // Per-channel state for phase tracking (used in anti-aliased mode)
    float lastPhasor[HCV_MAX_POLYPHONY] = {};

	PhasorToWaveforms()
	{
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configSwitch(ANTIALIAS_PARAM, 0.0f, 1.0f, 0.0f, "Mode", {"LFO", "Oscillator"});
        configInput(PHASOR_INPUT, "Phasor");

        configOutput(SINE_OUTPUT, "Unipolar Sine");
        configOutput(TRIANGLE_OUTPUT, "Unipolar Triangle");
        configOutput(SAW_OUTPUT, "Unipolar Saw");
        configOutput(RAMP_OUTPUT, "Unipolar Ramp");
        configOutput(SQUARE_OUTPUT, "Unipolar Square");

        configOutput(SINE_BIPOLAR_OUTPUT, "Bipolar Sine");
        configOutput(TRIANGLE_BIPOLAR_OUTPUT, "Bipolar Triangle");
        configOutput(SAW_BIPOLAR_OUTPUT, "Bipolar Saw");
        configOutput(RAMP_BIPOLAR_OUTPUT, "Bipolar Ramp");
        configOutput(SQUARE_BIPOLAR_OUTPUT, "Bipolar Square");

        g_mipmapTables.initialize();
	}

    /// Read from wavetable with linear interpolation
    inline float readTable(const float* table, float phase) {
        float indexF = phase * TABLE_SIZE;
        int index0 = (int)indexF;
        int index1 = index0 + 1;
        float frac = indexF - index0;

        index0 &= (TABLE_SIZE - 1);
        index1 &= (TABLE_SIZE - 1);

        return table[index0] + frac * (table[index1] - table[index0]);
    }

    /// Read from mipmap with level interpolation
    inline float readMipmap(const float table[NUM_MIPMAP_LEVELS][TABLE_SIZE], float phase, float level) {
        level = clamp(level, 0.0f, (float)(NUM_MIPMAP_LEVELS - 1));

        int level0 = (int)level;
        int level1 = std::min(level0 + 1, NUM_MIPMAP_LEVELS - 1);
        float levelFrac = level - level0;

        float sample0 = readTable(table[level0], phase);
        float sample1 = readTable(table[level1], phase);

        return sample0 + levelFrac * (sample1 - sample0);
    }

	void process(const ProcessArgs &args) override;

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - reset, randomize: implements special behavior when user clicks these from the context menu
};


void PhasorToWaveforms::process(const ProcessArgs &args)
{
    int numChannels = setupPolyphonyForAllOutputs();
    const bool antialiasMode = params[ANTIALIAS_PARAM].getValue() > 0.5f;

    // Pre-calculate mipmap constants for anti-aliased mode
    const float sampleRate = args.sampleRate;
    const float baseFreq = 20.0f;
    const float levelOffset = std::log2(sampleRate / baseFreq);

    for (int i = 0; i < numChannels; i++)
    {
        float phasorIn = inputs[PHASOR_INPUT].getPolyVoltage(i);
        const float normalizedPhasor = scaleAndWrapPhasor(phasorIn);

        float sine, saw, ramp, triangle, square;

        if (antialiasMode)
        {
            // Anti-aliased mode using mipmap wavetables
            float phaseDelta = normalizedPhasor - lastPhasor[i];
            lastPhasor[i] = normalizedPhasor;

            // Handle wraparound
            if (phaseDelta > 0.5f) {
                phaseDelta -= 1.0f;
            } else if (phaseDelta < -0.5f) {
                phaseDelta += 1.0f;
            }

            float absDelta = std::fabs(phaseDelta);
            if (absDelta < 1e-7f) absDelta = 1e-7f;

            float mipmapLevel = std::log2(absDelta) + levelOffset;
            mipmapLevel = clamp(mipmapLevel, 0.0f, (float)(NUM_MIPMAP_LEVELS - 1));

            // Read from band-limited wavetables
            saw = readMipmap(g_mipmapTables.saw, normalizedPhasor, mipmapLevel) * 5.0f;
            ramp = readMipmap(g_mipmapTables.saw, 1.0f - normalizedPhasor, mipmapLevel) * 5.0f;
            square = readMipmap(g_mipmapTables.square, normalizedPhasor, mipmapLevel) * 5.0f;
            triangle = readMipmap(g_mipmapTables.triangle, normalizedPhasor, mipmapLevel) * 5.0f;
            sine = gam::scl::sinP9(normalizedPhasor - 0.25f) * 5.0f;
        }
        else
        {
            // LFO mode - original simple waveform generation
            sine = cosf(normalizedPhasor * M_2PI) * -5.0f;
            saw = normalizedPhasor * HCV_PHZ_UPSCALE - 5.0f;
            ramp = (1.0f - normalizedPhasor) * HCV_PHZ_UPSCALE - 5.0f;
            triangle = gam::scl::fold(normalizedPhasor * 2.0f) * HCV_PHZ_UPSCALE - 5.0f;
            square = normalizedPhasor < 0.5f ? 5.0f : -5.0f;
        }

        // Output unipolar (0-10V)
        outputs[SINE_OUTPUT].setVoltage(sine + 5.0f, i);
        outputs[SAW_OUTPUT].setVoltage(saw + 5.0f, i);
        outputs[RAMP_OUTPUT].setVoltage(ramp + 5.0f, i);
        outputs[TRIANGLE_OUTPUT].setVoltage(triangle + 5.0f, i);
        outputs[SQUARE_OUTPUT].setVoltage(square + 5.0f, i);

        // Output bipolar (-5V to +5V)
        outputs[SINE_BIPOLAR_OUTPUT].setVoltage(sine, i);
        outputs[SAW_BIPOLAR_OUTPUT].setVoltage(saw, i);
        outputs[RAMP_BIPOLAR_OUTPUT].setVoltage(ramp, i);
        outputs[TRIANGLE_BIPOLAR_OUTPUT].setVoltage(triangle, i);
        outputs[SQUARE_BIPOLAR_OUTPUT].setVoltage(square, i);
    }

    for (int i = 0; i < 5; i++)
    {
        lights[SINE_LIGHT + i].setBrightness(outputs[i].getVoltage() * 0.1f);
        setBipolarLightBrightness(SINE_BIPOLAR_LIGHT + (i*2), outputs[SINE_BIPOLAR_OUTPUT + i].getVoltage() * 0.2f);
    }
}

struct PhasorToWaveformsWidget : HCVModuleWidget { PhasorToWaveformsWidget(PhasorToWaveforms *module); };

PhasorToWaveformsWidget::PhasorToWaveformsWidget(PhasorToWaveforms *module)
{
    setSkinPath("res/PhasorToWaveforms.svg");
    initializeWidget(module);

    createInputPort(33, 62, PhasorToWaveforms::PHASOR_INPUT);
    createHCVSwitchVert(14, 65, PhasorToWaveforms::ANTIALIAS_PARAM);

    const int initialY = 130;
    const int initialLightY = 138;

    for(int i = 0; i < 5; i++)
    {
        const int yPos = i*42;
        createOutputPort(10, initialY + yPos, PhasorToWaveforms::SINE_OUTPUT + i);
        createOutputPort(56, initialY + yPos, PhasorToWaveforms::SINE_BIPOLAR_OUTPUT + i);

        createHCVRedLight(36, initialLightY + yPos, PhasorToWaveforms::SINE_LIGHT + i);
        createHCVBipolarLight(48, initialLightY + yPos, PhasorToWaveforms::SINE_BIPOLAR_LIGHT + (i*2));
    }
}

Model *modelPhasorToWaveforms = createModel<PhasorToWaveforms, PhasorToWaveformsWidget>("PhasorToWaveforms");
