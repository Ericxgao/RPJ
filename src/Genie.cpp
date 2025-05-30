#include "RPJ.hpp"
#include "ctrl/RPJButtons.hpp"
#include "ctrl/RPJKnobs.hpp"
#include "ctrl/RPJLights.hpp"
#include "ctrl/RPJPorts.hpp"
#include <random>
#include "Genie.hpp"


Genie::Genie() {
	genieAlgorythm = PENDULUM;
    std::mt19937 generator((std::random_device())());
    std::uniform_real_distribution<> rnd(0, 2 * M_PI);
	nrOfPendulums=3;
	dim.first = 5;
	dim.second = 5;
    len = std::min(dim.first, dim.second);
	uni = false;

	for (int n=0;n<MAXOBJECTS;n++) {
		bumpingBalls[n].setPosition({rnd(generator), rnd(generator)});
	}

	for (int n=0;n<MAXOBJECTS;n++) {
    	st[n] = {{rnd(generator), rnd(generator)}, {0, 0}};
    	ss[n] = {{1, 1}, {len, len}};
	}

	config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	configParam(PARAM_TIMEMULT, 1.f, 10.f,10.f, "Speed");
	configParam(PARAM_TIMEMULTCV, -1.f, 1.f,0.f, "Speed CV Attenuator");
	configParam(PARAM_LENGTHMULT, 0.1f, 1.f,1.f, "Length");
	configParam(PARAM_LENGTHMULTCV, -1.f, 1.f,0.f, "Length CV Attenuator");
	configParam(PARAM_MASS, 0.1f, 10.f,5.f, "Mass");
	configParam(PARAM_MASSCV, -1.f, 1.f,0.f, "Mass CV Attenuator");
	configParam(PARAM_UNI,0.f,1.f,0.f,"Unipolar selection");
	rightExpander.producerMessage = (xpanderPairs*) &xpMsg[0];
	rightExpander.consumerMessage = (xpanderPairs*) &xpMsg[1];
}

void Genie::reset(void) {
	std::mt19937 generator((std::random_device())());
    std::uniform_real_distribution<> rnd(0, 2 * M_PI);

	for (int n=0;n<MAXOBJECTS;n++)
    	st[n] = {{rnd(generator), rnd(generator)}, {0, 0}};
}

json_t *Genie::dataToJson() {
	json_t *rootJ=json_object();
	json_object_set_new(rootJ, "JSON_NROFPENDULUMS", json_integer(static_cast<int>(nrOfPendulums)));
    json_object_set_new(rootJ, "JSON_ALGORYTHM", json_integer(static_cast<int>(genieAlgorythm)));
	return rootJ;
}

void Genie::dataFromJson(json_t *rootJ) {
	json_t *nNrOfPJ = json_object_get(rootJ, "JSON_NROFPENDULUMS");
	json_t *nAlgoJ = json_object_get(rootJ, "JSON_ALGORYTHM");
	if (nNrOfPJ) 
		nrOfPendulums = static_cast<int>(json_integer_value(nNrOfPJ));
	if (nAlgoJ) 
		genieAlgorythm = static_cast<GenieAlgorythms>(json_integer_value(nAlgoJ));
}

void ball::setPosition(std::pair<double,double> position) {
	this->position = position;
}

std::pair<double,double> ball::getPosition(void) {
	return position;
}

void Genie::doBumpingBalls(const ProcessArgs & args) {
	for (int n=0;n<nrOfPendulums+1;n++) {
		std::pair<double,double> x = bumpingBalls[n].getPosition();
	}
}

void Genie::doPendulum(const ProcessArgs & args) {

	for (int n=0;n<nrOfPendulums+1;n++) {
		ss[n] = {{mass, mass}, {len, len}};
		edges[n][0] = {
			ss[n].length.first * sin(st[n].theta.first),
			ss[n].length.first * cos(st[n].theta.first)
		};

		edges[n][1] = {
			edges[n][0].first  + ss[n].length.second * sin(st[n].theta.second),
			edges[n][0].second + ss[n].length.second * cos(st[n].theta.second)
		};

		st[n] = dp::advance(st[n], ss[n], args.sampleTime*timeMult);

		outputs[OUTPUT_1_X+2*n].setVoltage((edges[n][0].first)+5*uni);
		outputs[OUTPUT_1_Y+2*n].setVoltage((edges[n][0].second)+5*uni);
		outputs[OUTPUT_1_EDGE+n].setVoltage((st[n].theta.first/18.0f));
		
		#ifndef METAMODULE
		bool expanderPresent = (rightExpander.module && rightExpander.module->model == modelGenieExpander);
		#else
		bool expanderPresent = false;
		#endif
		if (expanderPresent) {
			xpanderPairs* wrMsg = (xpanderPairs*)rightExpander.producerMessage;
			for (int i=0; i < EDGES;i++)
			wrMsg->edges[n][i] = edges[n][i];
			wrMsg->nrOfItems = nrOfPendulums+1;
			rightExpander.messageFlipRequested = true;
		}
	}
}

void Genie::process(const ProcessArgs &args) {

	if (resetTrigger.process(rescale(Genie::params[PARAM_RESET].getValue(), 1.f, 0.1f, 0.f, 1.f))){
		reset();
    }

	uni = params[PARAM_UNI].getValue();

	mass = params[PARAM_MASS].getValue();
	if (inputs[INPUT_MASSCV].isConnected()) {
		double cvm = inputs[INPUT_MASSCV].getVoltage()/10.f;
		mass=clamp((params[PARAM_MASSCV].getValue() * cvm * 10.f) + params[PARAM_MASS].getValue(),0.1f, 10.0f);
	}

    timeMult = params[PARAM_TIMEMULT].getValue();
	if (inputs[INPUT_TIMEMULTCV].isConnected()) {
		double cvt = inputs[INPUT_TIMEMULTCV].getVoltage()/10.f;
		timeMult=clamp((params[PARAM_TIMEMULTCV].getValue() * cvt * 10.f) + params[PARAM_TIMEMULT].getValue(),1.f, 10.0f);
	}

	lengthMult = params[PARAM_LENGTHMULT].getValue();
	if (inputs[INPUT_LENGTHMULTCV].isConnected()) {
		double cvl = inputs[INPUT_LENGTHMULTCV].getVoltage()/10.f;
		lengthMult=clamp((params[PARAM_LENGTHMULTCV].getValue() * cvl) + params[PARAM_LENGTHMULT].getValue(),0.1f, 1.f);
	}

	len = std::min(dim.first, dim.second) * lengthMult;

	switch (static_cast<int>(genieAlgorythm))
	{
		// PENDULUM
		case 0:
			{
				doPendulum(args);
			}
			break;
		case 1:
			{
				doBumpingBalls(args);
			}
			break;
		default:
			break;
	}
}

void GenieModuleWidget::appendContextMenu(Menu *menu) {
	Genie * module = dynamic_cast<Genie*>(this->module);

	menu->addChild(new MenuSeparator());
	menu->addChild(createIndexPtrSubmenuItem("Algorythm", {"Pendulum", "Bumping Balls"}, &module->genieAlgorythm));

	menu->addChild(new MenuSeparator());

	menu->addChild(createIndexPtrSubmenuItem("Number of Pendulums", {"1", "2", "3", "4"}, &module->nrOfPendulums));

}

GenieModuleWidget::GenieModuleWidget(Genie* module) {

	setModule(module);
	setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Genie.svg")));

	addChild(createWidget<ScrewSilver>(Vec(0, 0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15,0)));
	addChild(createWidget<ScrewSilver>(Vec(box.size.x - 15, 365)));
	addChild(createWidget<ScrewSilver>(Vec(0, 365)));

	// First do the buttons
	const float buttonX1 = 62;
	const float buttonX2 = 107;

	const float buttonY1 = 129;
    addParam(createParam<ButtonBigSwitch>(Vec(buttonX1,buttonY1),module, Genie::PARAM_UNI));
	addParam(createParam<ButtonBig>(Vec(buttonX2,buttonY1),module, Genie::PARAM_RESET));

	// Then do the knobs
	const float knobX1 = 6;
	const float knobX2 = 36;
	const float knobX3 = 54;
	const float knobX4 = 80;
	const float knobX5 = 100;

	const float knobY1 = 31.5;
	const float knobY2 = 70;
	const float knobY3 = 98;

	addParam(createParam<RPJKnob>(Vec(knobX1, knobY1), module, Genie::PARAM_TIMEMULT));
	addParam(createParam<RPJKnobSmall>(Vec(knobX1+3, knobY2), module, Genie::PARAM_TIMEMULTCV));
	addParam(createParam<RPJKnob>(Vec(knobX5, knobY1), module, Genie::PARAM_LENGTHMULT));
	addParam(createParam<RPJKnobSmall>(Vec(knobX5+3, knobY2), module, Genie::PARAM_LENGTHMULTCV));
	addParam(createParam<RPJKnob>(Vec(knobX3, knobY1),module, Genie::PARAM_MASS));
	addParam(createParam<RPJKnobSmall>(Vec(knobX3+3, knobY2), module, Genie::PARAM_MASSCV));

	// Next do the Jacks
	const float jackX1 = 7.5;
	const float jackX2 = 39;		
	const float jackX3 = 56;
	const float jackX4 = 70.5;
	const float jackX5 = 102;

	const float jackY1 = 96;
	const float jackY2 = 260;
	const float jackY3 = 253;
	const float jackY4 = 288;
	const float jackY5 = 323;

	addInput(createInput<RPJPJ301MPort>(Vec(jackX1, jackY1), module, Genie::INPUT_TIMEMULTCV));
	addInput(createInput<RPJPJ301MPort>(Vec(jackX5, jackY1), module, Genie::INPUT_LENGTHMULTCV));
	addInput(createInput<RPJPJ301MPort>(Vec(jackX3, jackY1), module, Genie::INPUT_MASSCV));
	addOutput(createOutput<RPJPJ301MPort>(Vec(jackX1, jackY3), module, Genie::OUTPUT_1_X));
	addOutput(createOutput<RPJPJ301MPort>(Vec(jackX1, jackY4), module, Genie::OUTPUT_1_Y));
	addOutput(createOutput<RPJPJ301MPort>(Vec(jackX1, jackY5), module, Genie::OUTPUT_1_EDGE));
	addOutput(createOutput<RPJPJ301MPort>(Vec(jackX2, jackY3), module, Genie::OUTPUT_2_X));
	addOutput(createOutput<RPJPJ301MPort>(Vec(jackX2, jackY4), module, Genie::OUTPUT_2_Y));
	addOutput(createOutput<RPJPJ301MPort>(Vec(jackX2, jackY5), module, Genie::OUTPUT_2_EDGE));
	addOutput(createOutput<RPJPJ301MPort>(Vec(jackX4, jackY3), module, Genie::OUTPUT_3_X));
	addOutput(createOutput<RPJPJ301MPort>(Vec(jackX4, jackY4), module, Genie::OUTPUT_3_Y));
	addOutput(createOutput<RPJPJ301MPort>(Vec(jackX4, jackY5), module, Genie::OUTPUT_3_EDGE));
	addOutput(createOutput<RPJPJ301MPort>(Vec(jackX5, jackY3), module, Genie::OUTPUT_4_X));
	addOutput(createOutput<RPJPJ301MPort>(Vec(jackX5, jackY4), module, Genie::OUTPUT_4_Y));
	addOutput(createOutput<RPJPJ301MPort>(Vec(jackX5, jackY5), module, Genie::OUTPUT_4_EDGE));
}

Model * modelGenie = createModel<Genie, GenieModuleWidget>("Genie");
