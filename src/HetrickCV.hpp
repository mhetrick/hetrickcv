#pragma once

#include "HetrickUtilities.hpp"

            
/*                                                                
 ╔ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ╗
    M""MMMMM""MM            dP            oo          dP          
 ║  M  MMMMM  MM            88                        88         ║
    M         `M .d8888b. d8888P 88d888b. dP .d8888b. 88  .dP     
 ║  M  MMMMM  MM 88ooood8   88   88'  `88 88 88'  `"" 88888"     ║
    M  MMMMM  MM 88.  ...   88   88       88 88.  ... 88  `8b.    
 ║  M  MMMMM  MM `88888P'   dP   dP       dP `88888P' dP   `YP   ║
    MMMMMMMMMMMM                                                  
 ║                                                               ║
                      MM'""""'YMM M""MMMMM""M                     
 ║                    M' .mmm. `M M  MMMMM  M                    ║
                      M  MMMMMooM M  MMMMP  M                     
 ║                    M  MMMMMMMM M  MMMM' .M                    ║
                      M. `MMM' .M M  MMP' .MM                     
 ║                    MM.     .dM M     .dMMM                    ║
                      MMMMMMMMMMM MMMMMMMMMMM                     
 ╚ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ═ ╝
*/                                                                

using namespace rack;


extern Plugin *pluginInstance;

extern Model *modelTwoToFour;
extern Model *modelAnalogToDigital;
extern Model *modelASR;
extern Model *modelBinaryGate;
extern Model *modelBinaryNoise;
extern Model *modelBitshift;
extern Model *modelBlankPanel;
extern Model *modelBoolean3;
extern Model *modelChaos1Op;
extern Model *modelChaos2Op;
extern Model *modelChaos3Op;
extern Model *modelChaoticAttractors;
extern Model *modelClockedNoise;
extern Model *modelComparator;
extern Model *modelContrast;
extern Model *modelCrackle;
extern Model *modelDataCompander;
extern Model *modelDelta;
extern Model *modelDigitalToAnalog;
extern Model *modelDust;
extern Model *modelExponent;
extern Model *modelFBSineChaos;
extern Model *modelFlipFlop;
extern Model *modelFlipPan;
extern Model *modelGateJunction;
extern Model *modelGingerbread;
extern Model *modelLogicCombine;
extern Model *modelMidSide;
extern Model *modelMinMax;
extern Model *modelRandomGates;
extern Model *modelRotator;
extern Model *modelRungler;
extern Model *modelScanner;
extern Model *modelWaveshape;
extern Model *modelXYToPolar;