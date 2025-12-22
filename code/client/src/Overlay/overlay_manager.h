#pragma once

#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <imgui.h>
#include "i_widget.h"

namespace HogwartsMP::Overlay {

    struct Notification {
        std::string message;
        float duration; // Secondes restants
        ImVec4 color;
        float timestamp; // Temps de crÃ©ation
    };

    class OverlayManager {
    public:
        static OverlayManager& Get();

        void Init();
        void Render();
        void Update();

        // Gestion des Widgets
        void AddWidget(std::shared_ptr<IWidget> widget);
        void RemoveWidget(const std::string& id);
        std::shared_ptr<IWidget> GetWidget(const std::string& id);

        // SystÃ¨me de Notification
        void Notify(const std::string& message, float duration = 5.0f, ImVec4 color = ImVec4(1, 1, 1, 1));
        void NotifyError(const std::string& message);
        void NotifySuccess(const std::string& message);

        // ContrÃ´le global
        bool IsVisible() const { return _visible; }
        void ToggleVisibility() { _visible = !_visible; }

    private:
        OverlayManager() = default;
        ~OverlayManager() = default;

        void RenderNotifications();

        std::vector<std::shared_ptr<IWidget>> _widgets;
        std::vector<Notification> _notifications;
        
        bool _visible = true;
        bool _initialized = false;
        std::mutex _mutex;
    };

} // namespace HogwartsMP::Overlay
