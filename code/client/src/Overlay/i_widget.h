#pragma once

#include <string>

namespace HogwartsMP::Overlay {

    /**
     * @brief Interface base pour tous les widgets de l'overlay (Chat, Console, Stats...)
     */
    class IWidget {
    public:
        virtual ~IWidget() = default;

        // AppelÃ© Ã  chaque frame pour dessiner le widget via ImGui
        virtual void Render() = 0;

        // AppelÃ© pour mettre Ã  jour la logique (peut Ãªtre moins frÃ©quent que Render)
        virtual void Update() {}

        // VisibilitÃ©
        virtual bool IsVisible() const { return _visible; }
        virtual void SetVisible(bool visible) { _visible = visible; }
        virtual void ToggleVisibility() { _visible = !_visible; }

        // Identifiant unique du widget
        virtual std::string GetId() const = 0;

    protected:
        bool _visible = true;
    };

} // namespace HogwartsMP::Overlay
