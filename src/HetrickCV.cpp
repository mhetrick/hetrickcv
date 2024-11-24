#include "HetrickCV.hpp"


// The pluginInstance-wide instance of the Plugin class
Plugin *pluginInstance;

void init(rack::Plugin *p) {
	pluginInstance = p;

	p->addModel(modelTwoToFour);
	p->addModel(modelAnalogToDigital);
	p->addModel(modelASR);
	p->addModel(modelBinaryGate);
	p->addModel(modelBinaryNoise);
	p->addModel(modelBitshift);
	p->addModel(modelBlankPanel);
	p->addModel(modelBoolean3);
	p->addModel(modelChaos1Op);
	p->addModel(modelChaos2Op);
	p->addModel(modelChaos3Op);
	p->addModel(modelChaoticAttractors);
	p->addModel(modelClockedNoise);
	p->addModel(modelComparator);
	p->addModel(modelContrast);
	p->addModel(modelCrackle);
	p->addModel(modelDataCompander);
	p->addModel(modelDelta);
	p->addModel(modelDigitalToAnalog);
	p->addModel(modelDust);
	p->addModel(modelExponent);
	p->addModel(modelFBSineChaos);
	p->addModel(modelFlipFlop);
	p->addModel(modelFlipPan);
	p->addModel(modelGateDelay);
	p->addModel(modelGateJunction);
	p->addModel(modelGateJunctionExp);
	p->addModel(modelGingerbread);
	p->addModel(modelLogicCombine);
	p->addModel(modelMidSide);
	p->addModel(modelMinMax);
	p->addModel(modelPhaseDrivenSequencer);
	p->addModel(modelPhaseDrivenSequencer32);
	p->addModel(modelPhasorAnalyzer);
	p->addModel(modelPhasorBurstGen);
	p->addModel(modelPhasorDivMult);
	p->addModel(modelPhasorEuclidean);
	p->addModel(modelPhasorFreezer);
	p->addModel(modelPhasorGates);
	p->addModel(modelPhasorGates32);
	p->addModel(modelPhasorGates64);
	p->addModel(modelPhasorGen);
	p->addModel(modelPhasorGeometry);
	p->addModel(modelPhasorHumanizer);
	p->addModel(modelPhasorMixer);
	p->addModel(modelPhasorOctature);
	p->addModel(modelPhasorProbability);
	p->addModel(modelPhasorQuadrature);
	p->addModel(modelPhasorRandom);
	p->addModel(modelPhasorRanger);
	p->addModel(modelPhasorReset);
	p->addModel(modelPhasorRhythmGroup);
	p->addModel(modelPhasorShape);
	p->addModel(modelPhasorShift);
	p->addModel(modelPhasorSplitter);
	p->addModel(modelPhasorStutter);
	p->addModel(modelPhasorSubstepShape);
	p->addModel(modelPhasorSwing);
	p->addModel(modelPhasorTimetable);
	p->addModel(modelPhasorToClock);
	p->addModel(modelPhasorToLFO);
	p->addModel(modelPhasorToWaveforms);
	p->addModel(modelProbability);
	p->addModel(modelRandomGates);
	p->addModel(modelRotator);
	p->addModel(modelRungler);
	p->addModel(modelScanner);
	p->addModel(modelTrigShaper);
	p->addModel(modelVectorMix);
	p->addModel(modelWaveshape);
	p->addModel(modelXYToPolar);
	// Any other pluginInstance initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables within this file or the individual module files to reduce startup times of Rack.
}
