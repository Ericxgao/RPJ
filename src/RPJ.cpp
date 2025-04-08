#include "RPJ.hpp"
Plugin *pluginInstance;
void init(Plugin *p) {
    pluginInstance = p;
    p->addModel(modelLFO);
    p->addModel(modelLavender);
    p->addModel(modelEaster);
    p->addModel(modelDryLand);
    p->addModel(modelTheWeb);
    p->addModel(modelGazpacho);    
    p->addModel(modelEstonia);
    p->addModel(modelBrave);  
    p->addModel(modelEssence);
    p->addModel(modelLadyNina);
    p->addModel(modelSugarMice);
    p->addModel(modelMontreal);
    p->addModel(modelBlindCurve);
	p->addModel(modelGaza);
	p->addModel(modelCircularRide);
	p->addModel(modelDrillingHoles);
    #ifndef METAMODULE
	p->addModel(modelTuxOn);
    #endif
    p->addModel(modelPigeonPlink);
    p->addModel(modelGenie);
    #ifndef METAMODULE
    p->addModel(modelGenieExpander);
    #endif
}
