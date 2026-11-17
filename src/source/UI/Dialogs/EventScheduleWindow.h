#pragma once

#include "UI/Core/WindowManager.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <chrono>
#include <string>
#include <vector>

namespace Rml
{
class Element;
}

namespace mu::ui::window
{
// One row of the schedule window.
struct EventScheduleLineEntry
{
    Rml::String text;
    bool heading = false;
    bool halfSpacer = false;
};

// The schedule window's model: the shared window frame's fields (window_shell.rml), the rows and the close button.
struct EventScheduleRmlModel
{
    bool positioned = false;
    float rootX = 0.f;
    float rootY = 0.f;
    bool hasTitle = true;
    Rml::String title;
    Rml::String closeLabel;
    std::vector<EventScheduleLineEntry> lines;
};

// MU Volta: the event schedule on the H key. It asks the server for the next starts of its events (mini games,
// Happy Hour, invasions) and counts them down; below, today's starts in the player's local time. A window in the
// shared frame (window_shell.rml), like the options window: dragged by any part that is not a control.
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

    void OnRmlBuilt();
    void Close();

    void BindRmlModel(Rml::DataModelConstructor& c, EventScheduleRmlModel& model);
    UI::RmlBridge::ThemedView<EventScheduleRmlModel> m_RmlView{"event_schedule",
        [this](Rml::DataModelConstructor& c, EventScheduleRmlModel& model) { BindRmlModel(c, model); },
        {{"Data/Interface/RmlUi/event_schedule.rml"}},
        {.afterBuild = [this] { OnRmlBuilt(); }, .beforeUnload = [this] { m_pPanelEl = nullptr; }}};

    CManager* m_pNewUIMng = nullptr;
    std::chrono::steady_clock::time_point m_LastRequest{};
    std::chrono::system_clock::time_point m_OpenedAt{};
    std::vector<Line> m_BuiltLines;
    bool m_LabelsBuilt = false;
    Rml::Element* m_pPanelEl = nullptr;
};
} // namespace mu::ui::window
