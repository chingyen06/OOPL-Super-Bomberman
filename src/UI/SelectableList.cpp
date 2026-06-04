#include "UI/SelectableList.hpp"

#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

int SelectableList::StepEnabled(int from, int dir) const {
    const int n = static_cast<int>(m_Enabled.size());
    if (n == 0) return 0;
    int idx = from;
    for (int i = 0; i < n; i++) {
        idx = (idx + dir + n) % n;
        if (m_Enabled[idx]) return idx;
    }
    return (from < 0) ? 0 : from;  // 全部停用：維持原樣
}

int SelectableList::ReadNavStep() {
    using K = Util::Keycode;
    const bool prev = Util::Input::IsKeyUp(K::UP)   || Util::Input::IsKeyUp(K::LEFT)  ||
                      Util::Input::IsKeyUp(K::W)    || Util::Input::IsKeyUp(K::A);
    const bool next = Util::Input::IsKeyUp(K::DOWN) || Util::Input::IsKeyUp(K::RIGHT) ||
                      Util::Input::IsKeyUp(K::S)    || Util::Input::IsKeyUp(K::D);
    if (prev) return -1;  // 與原行為一致：同幀同時觸發時「上一個」優先
    if (next) return +1;
    return 0;
}

bool SelectableList::MoveSelection() {
    const int dir = ReadNavStep();
    if (dir == 0) return false;
    m_Selected = StepEnabled(m_Selected, dir);
    UpdateCursor();
    return true;
}
