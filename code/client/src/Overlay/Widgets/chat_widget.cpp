#include "chat_widget.h"
#include <imgui.h>
#include "../../Core/application.h" // NÃ©cessaire pour gÃ©rer le lock des controls du jeu

namespace HogwartsMP::Overlay::Widgets {

    ChatWidget::ChatWidget() {
        // Messages de bienvenue par dÃ©faut
        _messages.push_back("Welcome to HogwartsMP Chat!");
    }

    void ChatWidget::AddMessage(const std::string& msg) {
        std::lock_guard<std::mutex> lock(_mutex);
        _messages.push_back(msg);
        _scrollToBottom = true;
    }

    void ChatWidget::Update() {
        // Gestion des raccourcis clavier pour ouvrir le chat (Entrée)
        // Utilisation de ImGui pour détecter la touche Entrée globalement
        if (!_isFocused && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
            _isFocused = true;
            auto app = HogwartsMP::Core::gApplication.get();
            if (app) app->LockControls(true); // Bloquer les inputs du jeu
            _scrollToBottom = true;
        }
    }

    void ChatWidget::Render() {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
        
        // FenÃªtre semi-transparente
        ImGui::SetNextWindowBgAlpha(0.7f);

        if (ImGui::Begin("Chat", nullptr, ImGuiWindowFlags_NoCollapse)) {
            
            // Zone de dÃ©filement des messages
            ImGui::BeginChild("##scrolling", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
            
            {
                std::lock_guard<std::mutex> lock(_mutex);
                for (const auto& msg : _messages) {
                    ImGui::TextWrapped("%s", msg.c_str());
                }
            }

            if (_scrollToBottom) {
                ImGui::SetScrollHereY(1.0f);
                _scrollToBottom = false;
            }
            
            ImGui::EndChild();

            // Zone de saisie
            if (_isFocused) {
                ImGui::SetKeyboardFocusHere(0);
            }
            
            ImGui::PushItemWidth(-1);
            if (ImGui::InputText("##chatinput", _inputBuffer, sizeof(_inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                std::string msg = _inputBuffer;
                if (!msg.empty() && _onMessageSent) {
                    _onMessageSent(msg);
                }
                
                // Vider le buffer mais garder le focus si on veut envoyer plusieurs messages ? 
                // Pour l'instant on ferme aprÃ¨s envoi comme les MMO classiques
                memset(_inputBuffer, 0, sizeof(_inputBuffer));
                _isFocused = false;
                
                // DÃ©bloquer le jeu
                auto app = HogwartsMP::Core::gApplication.get();
                if (app) app->LockControls(false);
            }
            ImGui::PopItemWidth();
            
            // Si on clique ailleurs, on perd le focus
            if (_isFocused && !ImGui::IsItemFocused() && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1))) {
                 _isFocused = false;
                 auto app = HogwartsMP::Core::gApplication.get();
                 if (app) app->LockControls(false);
            }
        }
        ImGui::End();
    }

} // namespace HogwartsMP::Overlay::Widgets
