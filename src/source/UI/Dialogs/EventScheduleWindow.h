#pragma once

#include "UI/Core/WindowManager.h"
#include "UI/Dialogs/HelpWindowRmlModel.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <chrono>
#include <string>
#include <vector>

namespace mu::ui::window
{
// MU Volta: the event schedule on the H key. It asks the server for the next starts of its events (mini games,
// Happy Hour, invasions) and counts them down; below, today's starts in the player's local time. Same look as
// the help window (help_window.rcss).
class CEventScheduleWindow : public CObject
{
public:
    CEventScheduleWindow() = default;
    virtual ~CEventScheduleWindow();

    bool Create(CManager* pNewUIMng);
    void Release();

    bool UpdateMouseEvent();
    bool UpdateKeyEvent();
    bool Update();
    bool Render();

    float GetLayerDepth();
    float GetKeyEventOrder();

    void OpenningProcess();
    void ClosingProcess();

    // The lines of the window for a schedule at `now`: a heading, a countdown per event, then the starts in local time.
    struct Line
    {
        std::string text;
        bool heading = false;
        bool halfSpacer = false;
    };
    [[nodiscard]] static std::vector<Line> BuildLines(bool received, std::chrono::system_clock::time_point now);

private:
    void RequestSchedule();
    void SyncRmlModel();

    void BindRmlModel(Rml::DataModelConstructor& c, HelpWindowRmlModel& model);
    UI::RmlBridge::ThemedView<HelpWindowRmlModel> m_RmlView{"event_schedule",
        [this](Rml::DataModelConstructor& c, HelpWindowRmlModel& model) { BindRmlModel(c, model); },
        {{"Data/Interface/RmlUi/event_schedule.rml"}}};

    CManager* m_pNewUIMng = nullptr;
    std::chrono::steady_clock::time_point m_LastRequest{};
    std::chrono::system_clock::time_point m_OpenedAt{};
    std::vector<Line> m_BuiltLines;
    float m_BuiltTextPx = 0.f;
};
} // namespace mu::ui::window
