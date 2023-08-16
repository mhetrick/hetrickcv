#include "HetrickUtilities.hpp"

float bipolarParamToOscillatorFrequencyScalar(float _bipolarParam)
{
    const float scaledRange = _bipolarParam * 54.0f;
    const float semitone = powf(dsp::FREQ_SEMITONE, scaledRange);
    const float frequency = dsp::FREQ_C4 * semitone;

    return frequency;
}

float bipolarParamToLFOFrequencyScalar(float _bipolarParam)
{
    const float scaledRange = _bipolarParam * 9.0f + 1.0f;
    const float power = powf(2.0f, scaledRange);

    return power;
}

float bipolarParamToClockMultScalar(float _bipolarParam)
{
    return bipolarParamToLFOFrequencyScalar(_bipolarParam) * 0.5f;
}

float frequencyToBipolarParamUnscalar(float _frequency)
{
    const float semitone = _frequency/dsp::FREQ_C4;
    const float scaledRange = std::log10(semitone)/std::log10(dsp::FREQ_SEMITONE);
    const float bipolar = scaledRange/54.0f;

    return clamp(bipolar, -1.0f, 1.0f);
}
float lfoFrequencyToBipolarParamUnscalar(float _lfoFrequency)
{
    const float root = std::log2(_lfoFrequency) - 1.0f;
    const float bipolar = root/9.0f;

    return clamp(bipolar, -1.0f, 1.0f);
}
float clockMultToBipolarParamUnscalar(float _clockMult)
{
    return lfoFrequencyToBipolarParamUnscalar(_clockMult * 2.0f);
}

////////
void PanelBaseWidget::draw(const DrawArgs& args) {
	nvgBeginPath(args.vg);

    float contrast = 230.0f;
	NVGcolor baseColor = nvgRGB(contrast, contrast, contrast);

	nvgFillColor(args.vg, baseColor);
	nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
	nvgFill(args.vg);
	TransparentWidget::draw(args);
}

void InverterWidget::refreshForTheme() {
	int newMode = settings::preferDarkPanels ? 1 : 0;
	if (newMode != oldMode) {
        mainPanel->fb->dirty = true;
        oldMode = newMode;
    }
}

void InverterWidget::step() {
	refreshForTheme();
	TransparentWidget::step();
}

void InverterWidget::draw(const DrawArgs& args) {
	TransparentWidget::draw(args);
	if (settings::preferDarkPanels) {
		// nvgSave(args.vg);
		nvgBeginPath(args.vg);
		nvgFillColor(args.vg, SCHEME_WHITE);// this is the source, the current framebuffer is the dest	
		nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
		nvgGlobalCompositeBlendFuncSeparate(args.vg, 
			NVG_ONE_MINUS_DST_COLOR,// srcRGB
			NVG_ZERO,// dstRGB
			NVG_ONE_MINUS_DST_COLOR,// srcAlpha
			NVG_ONE);// dstAlpha
		// blend factor: https://github.com/memononen/nanovg/blob/master/src/nanovg.h#L86
		// OpenGL blend doc: https://www.khronos.org/opengl/wiki/Blending
		nvgFill(args.vg);
		nvgClosePath(args.vg);
		// nvgRestore(args.vg);
	}			
}