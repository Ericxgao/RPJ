#pragma once

#include "rack.hpp"
#include "SqUI.h"


#include <random>
#include <functional>

/**
 * UI Widget that does:
 *  functions as a parameter
 *  pops up a menu of discrete choices.
 *  displays the current choice
 */

class PopupMenuParamWidget : public ::rack::app::ParamWidget {
public:
    std::vector<std::string> labels;
    std::string text = {"pop widget default"};

    void setLabels(std::vector<std::string> l) {
        labels = l;
        ::rack::widget::Widget::ChangeEvent e;
        onChange(e);
    }

    

    using NotificationCallback = std::function<void(int index)>;
    void setNotificationCallback(NotificationCallback);

    // input is paramter value (quantized), output is control index/
    using IndexToValueFunction = std::function<float(int index)>;
    using ValueToIndexFunction = std::function<int(float value)>;
    void setIndexToValueFunction(IndexToValueFunction);
    void setValueToIndexFunction(ValueToIndexFunction);


    void drawLayer(const DrawArgs &arg, int layer) override;
    void onButton(const ::rack::widget::Widget::ButtonEvent &e) override;
    void onChange(const ::rack::widget::Widget::ChangeEvent &e) override;
    void onAction(const ::rack::widget::Widget::ActionEvent &e) override;
    void randomize();

    friend class PopupMenuItem;
private:

    NotificationCallback optionalNotificationCallback = {nullptr};
    IndexToValueFunction optionalIndexToValueFunction = {nullptr};
    ValueToIndexFunction optionalValueToIndexFunction = {nullptr};
    int curIndex = 0;
};

inline void PopupMenuParamWidget::randomize() {  
	if (getParamQuantity() && getParamQuantity()->isBounded()) {
		float value = rack::math::rescale(rack::random::uniform(), 0.f, 1.f, getParamQuantity()->getMinValue(), getParamQuantity()->getMaxValue());
		value = std::round(value);
		getParamQuantity()->setValue(value);
	}
}

inline void PopupMenuParamWidget::setNotificationCallback(NotificationCallback callback) {
    optionalNotificationCallback = callback;
}

inline void PopupMenuParamWidget::setIndexToValueFunction(IndexToValueFunction fn) {
    optionalIndexToValueFunction = fn;
}

inline void PopupMenuParamWidget::setValueToIndexFunction(ValueToIndexFunction fn) {
    optionalValueToIndexFunction = fn;
}

inline void PopupMenuParamWidget::onChange(const ::rack::widget::Widget::ChangeEvent &e) {
    if (!this->getParamQuantity()) {
        return;  // no module
    }

    // process our self to update the text label
   
    int index = (int)std::round(this->getParamQuantity()->getValue());
    if (optionalValueToIndexFunction) {
        float value = this->getParamQuantity()->getValue();
        index = optionalValueToIndexFunction(value);
    }

    if (!labels.empty()) {
        if (index < 0 || index >= (int)labels.size()) {
            WARN("onChange: index is outside label ranges is %d", index);
            return;
        }
        this->text = labels[index];
        curIndex = index;               // remember it
    }

    // Delegate to base class to change param value
    ParamWidget::onChange(e);
    if (optionalNotificationCallback) {
        optionalNotificationCallback(index);
    }
}

inline void PopupMenuParamWidget::drawLayer(const DrawArgs &args, int layer) {
    if (layer == 1) {
        BNDwidgetState state = BND_DEFAULT;
        bndChoiceButton(args.vg, 0.0, 0.0, box.size.x, box.size.y, BND_CORNER_NONE, state, -1, text.c_str());
    }
    ParamWidget::drawLayer(args,layer);
}

inline void PopupMenuParamWidget::onButton(const ::rack::widget::Widget::ButtonEvent &e) {
    if ((e.button == GLFW_MOUSE_BUTTON_LEFT) && (e.action == GLFW_PRESS)) {
        ::rack::widget::Widget::ActionEvent ea;
        onAction(ea);
        sq::consumeEvent(&e, this);
    }
}

class PopupMenuItem : public ::rack::ui::MenuItem {
public:
    /**
     * param index is the menu item index, but also the
     *  parameter value.
     */
    PopupMenuItem(int index, PopupMenuParamWidget *inParent) : index(index), parent(inParent) {
        // TODO: just pass text in
        text = parent->labels[index];
    }

    const int index;
    PopupMenuParamWidget *const parent;

    void onAction(const ::rack::widget::Widget::ActionEvent &e) override {
        parent->text = this->text;
        ::rack::widget::Widget::ChangeEvent ce;
        if (parent->getParamQuantity()) {
            float value = index;
            if (parent->optionalIndexToValueFunction) {
                value = parent->optionalIndexToValueFunction(index);
            }
            parent->getParamQuantity()->setValue(value);
        }
        parent->onChange(ce);
    }
};

inline void PopupMenuParamWidget::onAction(const ::rack::widget::Widget::ActionEvent &e) {
    ::rack::ui::Menu *menu = ::rack::createMenu();

    // is choice button the right base class?
    menu->box.pos = getAbsoluteOffset(::rack::math::Vec(0, this->box.size.y)).round();
    menu->box.size.x = box.size.x;
    {
        for (int i = 0; i < (int)labels.size(); ++i) {
            menu->addChild(new PopupMenuItem(i, this));
        }
    }
}
