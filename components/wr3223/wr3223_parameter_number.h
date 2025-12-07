#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"
#include "wr3223.h"
#include "wr3223_constants.h"

namespace esphome
{
    namespace wr3223
    {

        class WR3223ParameterNumber : public number::Number, public Component
        {
        public:
            WR3223ParameterNumber(WR3223 *parent, const char *command) 
                : parent_(parent), command_(command) {}

            void setup() override;
            float get_setup_priority() const override { return setup_priority::DATA; }

        protected:
            void control(float value) override;

            WR3223 *parent_;
            const char *command_;
        };

    } // namespace wr3223
} // namespace esphome