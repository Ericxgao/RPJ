#pragma once

#include "rack.hpp"

namespace sq {
  
    using EventAction = ::rack::widget::Widget::ActionEvent;
    using EventChange = ::rack::widget::Widget::ChangeEvent;

    inline void consumeEvent(const ::rack::event::Base* evt, ::rack::Widget* widget)
    {
        evt->consume(widget);
    }
}