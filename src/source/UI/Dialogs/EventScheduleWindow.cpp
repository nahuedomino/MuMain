#include "stdafx.h"
#include "UI/Dialogs/EventScheduleWindow.h"

#include "Audio/DSPlaySound.h"
#include "Core/Utilities/StringUtils.h"
#include "I18N/All.h"
#include "GameLogic/Events/EventSchedule.h"
#include "Network/Server/WSclient.h"
#include "UI/Core/WindowSystem.h"
#include "UI/RmlBridge/RmlDocumentVisibility.h"
#include "UI/RmlBridge/RmlDraggable.h"
#include "UI/RmlBridge/RmlPointer.h"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Event.h>

using namespace mu::ui::window;

namespace
{
// While the window is open, the schedule is asked for again now and then (the countdown runs locally).
constexpr auto RefreshInterval = std::chrono::seconds(60);

// Today's starts per event in the local time list.
constexpr std::size_t MaximumListedStarts = 8;

std::string Text(const wchar_t* text)
{
    return StringUtils::WideToNarrow(text);
}

std::string Format(const wchar_t* format, const std::string& first, const std::string& second)
{
    wchar_t buffer[256] = {};
    mu_swprintf(buffer, format, StringUtils::NarrowToWide(first).c_str(), StringUtils::NarrowToWide(second).c_str());
    return StringUtils::WideToNarrow(buffer);
}
} // namespace

CEventScheduleWindow::~CEventScheduleWindow()
{
    Release();
}

bool CEventScheduleWindow::Create(CManager* pNewUIMng)
{
    if (pNewUIMng == nullptr)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(INTERFACE_EVENT_SCHEDULE, this);
    m_RmlView.Ensure();
    Show(false);
    return true;
}

void CEventScheduleWindow::Release()
{
    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = nullptr;
    }

    m_RmlView.Release();
}

bool CEventScheduleWindow::UpdateMouseEvent()
{
    // Over the window, a click never falls through to the world behind it; the window isn't modal.
    if (!m_pPanelEl && m_RmlView.Document())
        m_pPanelEl = m_RmlView.Document()->GetElementById("panel");

    return !UI::RmlBridge::IsPointerOver(m_pPanelEl);
}

// Dragged by any part that is not a control for the session, like the options window; the position is not saved.
void CEventScheduleWindow::OnRmlBuilt()
{
    Rml::ElementDocument* document = m_RmlView.Document();
    m_pPanelEl = document->GetElementById("panel");
    UI::RmlBridge::MakeDraggable(document, document, nullptr,
                                 [document] { UI::RmlBridge::KeepInsideWindow(document); });
    m_BuiltLines.clear();
    m_LabelsBuilt = false;
}

void CEventScheduleWindow::Close()
{
    if (g_pNewUISystem->IsVisible(INTERFACE_EVENT_SCHEDULE))
    {
        g_pNewUISystem->Hide(INTERFACE_EVENT_SCHEDULE);
        PlayBuffer(SOUND_CLICK01);
    }
}

bool CEventScheduleWindow::UpdateKeyEvent()
{
    if (g_pNewUISystem->IsVisible(INTERFACE_EVENT_SCHEDULE) && (IsPress(VK_ESCAPE) || IsPress('H')))
    {
        Close();
        return false;
    }

    return true;
}

bool CEventScheduleWindow::Update()
{
    if (IsVisible() && std::chrono::steady_clock::now() - m_LastRequest >= RefreshInterval)
    {
        RequestSchedule();
    }

    SyncRmlModel();
    return true;
}

bool CEventScheduleWindow::Render()
{
    return true;
}

float CEventScheduleWindow::GetLayerDepth()
{
    return 8.3f;
}

float CEventScheduleWindow::GetKeyEventOrder()
{
    return 10.f;
}

void CEventScheduleWindow::OpenningProcess()
{
    m_OpenedAt = std::chrono::system_clock::now();
    RequestSchedule();
}

void CEventScheduleWindow::ClosingProcess()
{
}

void CEventScheduleWindow::RequestSchedule()
{
    m_LastRequest = std::chrono::steady_clock::now();
    if (SocketClient != nullptr)
    {
        // Writable: the bridge writes the packet size into it.
        BYTE request[] = {0xC1, 0x04, 0xFA, 0x02};
        SocketClient->Send(request, sizeof(request));
    }
}

std::vector<CEventScheduleWindow::Line> CEventScheduleWindow::BuildLines(bool received, std::chrono::system_clock::time_point now)
{
    using namespace GameLogic::Events;
    // The window's title (the frame's header) says "Events".
    std::vector<Line> lines;
    if (!received)
    {
        lines.push_back({Text(I18N::Game::WaitingForTheServer)});
        return lines;
    }

    struct Upcoming
    {
        const ScheduledEvent* event;
        NextRun run;
    };
    std::vector<Upcoming> upcoming;
    for (const auto& event : CurrentSchedule().events)
    {
        if (auto run = GetNextRun(event, now))
            upcoming.push_back({&event, *run});
    }

    if (upcoming.empty())
    {
        lines.push_back({Text(I18N::Game::NoEventsInTheNext24Hours)});
        return lines;
    }

    // Running ones first, then by their next start.
    std::stable_sort(upcoming.begin(), upcoming.end(), [](const Upcoming& a, const Upcoming& b) {
        if (a.run.running != b.run.running)
            return a.run.running;
        return a.run.start < b.run.start;
    });

    for (const auto& item : upcoming)
    {
        if (item.run.running)
        {
            const auto left = std::chrono::duration_cast<std::chrono::seconds>(item.run.start + item.event->duration - now);
            lines.push_back({Format(I18N::Game::SRunningEndsInS, item.event->name, FormatCountdown(left))});
        }
        else
        {
            const auto left = std::chrono::duration_cast<std::chrono::seconds>(item.run.start - now);
            lines.push_back({Format(I18N::Game::SStartsInS, item.event->name, FormatCountdown(left))});
        }
    }

    lines.push_back({"", false, true});
    lines.push_back({Text(I18N::Game::TodayYourLocalTime), true, false});
    for (const auto& item : upcoming)
    {
        std::string times;
        std::size_t listed = 0;
        for (const auto start : item.event->starts)
        {
            if (start + item.event->duration <= now || listed == MaximumListedStarts)
                continue;
            times += (listed++ == 0 ? "" : ", ") + FormatLocalTime(start);
        }

        lines.push_back({item.event->name + ": " + times});
    }

    return lines;
}

void CEventScheduleWindow::BindRmlModel(Rml::DataModelConstructor& c, EventScheduleRmlModel& model)
{
    c.Bind("positioned", &model.positioned);
    c.Bind("root_x", &model.rootX);
    c.Bind("root_y", &model.rootY);
    c.Bind("has_title", &model.hasTitle);
    c.Bind("title", &model.title);
    c.Bind("close_label", &model.closeLabel);

    auto line = c.RegisterStruct<EventScheduleLineEntry>();
    line.RegisterMember("text", &EventScheduleLineEntry::text);
    line.RegisterMember("heading", &EventScheduleLineEntry::heading);
    line.RegisterMember("half_spacer", &EventScheduleLineEntry::halfSpacer);
    c.RegisterArray<std::vector<EventScheduleLineEntry>>();
    c.Bind("lines", &model.lines);

    c.BindEventCallback("event_schedule_close",
                        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Close(); });
}

void CEventScheduleWindow::SyncRmlModel()
{
    m_RmlView.Ensure();
    if (!m_RmlView.Document())
        return;

    const bool visible = IsVisible();
    UI::RmlBridge::SyncDocumentVisibilityInFront(m_RmlView.Document(), visible);
    if (!visible)
        return;

    EventScheduleRmlModel& model = m_RmlView.GetModel();
    if (!m_LabelsBuilt)
    {
        m_LabelsBuilt = true;
        model.title = Text(I18N::Game::Events);
        model.closeLabel = Text(I18N::Game::Close);
        for (const char* field : {"title", "close_label", "has_title"})
            m_RmlView.MarkDirty(field);
    }

    const bool received = GameLogic::Events::CurrentSchedule().receivedAt >= m_OpenedAt - RefreshInterval;
    auto lines = BuildLines(received, std::chrono::system_clock::now());
    const bool same = lines.size() == m_BuiltLines.size() &&
                      std::equal(lines.begin(), lines.end(), m_BuiltLines.begin(), [](const Line& a, const Line& b) {
                          return a.text == b.text && a.heading == b.heading && a.halfSpacer == b.halfSpacer;
                      });
    if (same)
        return;

    m_BuiltLines = lines;
    model.lines.clear();
    for (const Line& line : lines)
        model.lines.push_back({line.text, line.heading, line.halfSpacer});

    m_RmlView.MarkDirty("lines");
}
