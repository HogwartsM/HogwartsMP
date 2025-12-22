#include "ui_base.h"

#include "Core/application.h"

namespace HogwartsMP::Core::UI {
    bool UIBase::AreControlsLocked() const {
        if (gApplication->AreControlsLockedBypassed()) {
            return false;
        }

        return gApplication->AreControlsLocked();
    }

    void UIBase::LockControls(bool lock) const {
        gApplication->LockControls(lock);
    }
} // namespace HogwartsMP::Core::UI
