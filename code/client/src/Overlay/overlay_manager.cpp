#include "overlay_manager.h"
#include <algorithm>
#include <imgui_internal.h> // Pour PushItemFlag si nÃ©cessaire

namespace HogwartsMP::Overlay {

    OverlayManager& OverlayManager::Get() {
        static OverlayManager instance;
        return instance;
    }

    void OverlayManager::Init() {
        if (_initialized) return;

        // Style ImGui par dÃ©faut pour l'overlay
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.0f;
        style.FrameRounding = 3.0f;
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.13f, 0.9f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.2f, 0.25f, 1.0f);
        
        _initialized = true;
    }

    void OverlayManager::Update() {
        if (!_initialized) return;
        std::lock_guard<std::mutex> lock(_mutex);

        // Mettre Ã  jour les widgets
        for (auto& widget : _widgets) {
            widget->Update();
        }

        // Mettre Ã  jour les notifications (supprimer les anciennes)
        float dt = ImGui::GetIO().DeltaTime;
        for (auto it = _notifications.begin(); it != _notifications.end();) {
            it->duration -= dt;
            if (it->duration <= 0.0f) {
                it = _notifications.erase(it);
            } else {
                ++it;
            }
        }
    }

    void OverlayManager::Render() {
        if (!_visible || !_initialized) return;

        std::lock_guard<std::mutex> lock(_mutex);

        // Rendu des widgets
        for (auto& widget : _widgets) {
            if (widget->IsVisible()) {
                widget->Render();
            }
        }

        // Rendu des notifications par dessus tout
        RenderNotifications();
    }

    void OverlayManager::RenderNotifications() {
        if (_notifications.empty()) return;

        ImGuiIO& io = ImGui::GetIO();
        float windowWidth = io.DisplaySize.x;
        float windowHeight = io.DisplaySize.y;
        float pad = 10.0f;

        ImGui::SetNextWindowPos(ImVec2(windowWidth - pad, windowHeight - pad), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::SetNextWindowBgAlpha(0.0f); // Transparent background container

        if (ImGui::Begin("Notifications", nullptr, 
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | 
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | 
            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs)) 
        {
            for (const auto& notif : _notifications) {
                ImGui::PushStyleColor(ImGuiCol_Text, notif.color);
                ImGui::Text("%s", notif.message.c_str());
                ImGui::PopStyleColor();
            }
        }
        ImGui::End();
    }

    void OverlayManager::AddWidget(std::shared_ptr<IWidget> widget) {
        std::lock_guard<std::mutex> lock(_mutex);
        _widgets.push_back(widget);
    }

    void OverlayManager::RemoveWidget(const std::string& id) {
        std::lock_guard<std::mutex> lock(_mutex);
        _widgets.erase(std::remove_if(_widgets.begin(), _widgets.end(),
            [&id](const std::shared_ptr<IWidget>& w) { return w->GetId() == id; }),
            _widgets.end());
    }

    std::shared_ptr<IWidget> OverlayManager::GetWidget(const std::string& id) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& widget : _widgets) {
            if (widget->GetId() == id) return widget;
        }
        return nullptr;
    }

    void OverlayManager::Notify(const std::string& message, float duration, ImVec4 color) {
        std::lock_guard<std::mutex> lock(_mutex);
        _notifications.push_back({message, duration, color, 0.0f});
    }

    void OverlayManager::NotifyError(const std::string& message) {
        Notify(message, 5.0f, ImVec4(1.0f, 0.3f, 0.3f, 1.0f)); // Rouge
    }

    void OverlayManager::NotifySuccess(const std::string& message) {
        Notify(message, 3.0f, ImVec4(0.3f, 1.0f, 0.3f, 1.0f)); // Vert
    }

} // namespace HogwartsMP::Overlay
