#pragma once
#include "../i_widget.h"
#include <string>

namespace HogwartsMP::Overlay::Widgets {

    class StatsWidget : public IWidget {
    public:
        StatsWidget();
        virtual ~StatsWidget() = default;

        void Render() override;
        void Update() override;

    private:
        int _ping = 0;
        bool _connected = false;
    };

}
