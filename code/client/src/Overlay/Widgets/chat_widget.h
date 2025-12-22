#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include "../i_widget.h"
#include <imgui.h>

namespace HogwartsMP::Overlay::Widgets {

    class ChatWidget : public IWidget {
    public:
        ChatWidget();
        ~ChatWidget() override = default;

        void Render() override;
        void Update() override;
        std::string GetId() const override { return "Chat"; }

        void AddMessage(const std::string& msg);
        void SetOnMessageSentCallback(std::function<void(const std::string&)> callback) { _onMessageSent = callback; }

    private:
        std::vector<std::string> _messages;
        char _inputBuffer[1024] = {0};
        bool _scrollToBottom = false;
        bool _isFocused = false;
        std::function<void(const std::string&)> _onMessageSent;
        std::mutex _mutex;
    };

} // namespace HogwartsMP::Overlay::Widgets
