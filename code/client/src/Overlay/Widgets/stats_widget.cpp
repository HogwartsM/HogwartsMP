#include "stats_widget.h"
#include <imgui.h>
#include <string>
#include "../../Core/application.h"
#include "../../Services/network_client.h"

namespace HogwartsMP::Overlay::Widgets {

    StatsWidget::StatsWidget() {
    }

    void StatsWidget::Update() {
        // Retrieve stats from Application/NetworkClient
        auto app = HogwartsMP::Core::gApplication.get();
        if (app) {
            auto network = app->GetNetworkClient();
            if (network) {
                _connected = network->IsConnected();
                _ping = network->GetPing();
            } else {
                _connected = false;
                _ping = 0;
            }
        }
    }

    void StatsWidget::Render() {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        
        const float PAD = 10.0f;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar
        ImVec2 work_size = viewport->WorkSize;
        ImVec2 window_pos, window_pos_pivot;
        window_pos.x = work_pos.x + work_size.x - PAD;
        window_pos.y = work_pos.y + PAD;
        window_pos_pivot.x = 1.0f;
        window_pos_pivot.y = 0.0f;
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background

        if (ImGui::Begin("OverlayStats", nullptr, window_flags)) {
            ImGui::Text("HogwartsMP Dev Build");
            ImGui::Separator();
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Ping: %d ms", _ping);
            
            if (_connected) {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: Connected");
            } else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: Disconnected");
            }
        }
        ImGui::End();
    }

}
