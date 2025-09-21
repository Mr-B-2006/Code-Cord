#include "Code__Cord.h"

std::atomic<bool> running = true;

namespace
{
    PluginRegistrant<Code__Cord> reg(_T("Code__Cord"));
}

Code__Cord::Code__Cord()  {    }
Code__Cord::~Code__Cord() {    }

void Code__Cord::OnAttach()
{
    rate_limit.SetOwner(this); //associates the rate_limit timer with our plugin object

    Manager::Get()->RegisterEventSink(cbEVT_EDITOR_SWITCHED, new cbEventFunctor<Code__Cord, CodeBlocksEvent>(this, &Code__Cord::OnActivityUpdateEvent)); //registering a function that shall be executed when we switch tabs
    Manager::Get()->RegisterEventSink(cbEVT_PROJECT_RENAMED, new cbEventFunctor<Code__Cord, CodeBlocksEvent>(this, &Code__Cord::OnActivityUpdateEvent));

    m_rp_data.timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); //gives us the current time in seconds, Discord uses this for recording our session time
}

void Code__Cord::OnActivityUpdateEvent(CodeBlocksEvent &event)
{
    update_status();
}

int Code__Cord::update_status()
{
    editor = Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor();

    if ( editor && editor->GetProjectFile() && editor->GetProjectFile()->GetParentProject() &&                                                                               //check that neither the editor and nor it's members point to a null pointer
        (m_rp_data.state != editor->GetProjectFile()->GetParentProject()->GetTitle().ToStdString() || m_rp_data.details != editor->GetShortName().ToStdString()) &&         //check that we arent sending out duplicate data when alt-tabbing in and out of the editor (which triggers cbEVT_EDITOR_SWITCHED)
        rich_presence_enabled )                                                                                                                  //check that we are connected to the Discord IPC socket
    {
        wxString activity_error_msg = "Activity changed failed, check Discord is running.\nThese messages will stop appearing if you disable Rich Presence or successfully re-connect to Discord.\nDisconnected from the Discord socket";

        if (!discord.is_connected()) //try re-establish connection
        {
            if (!discord.connect_to_discord(Code__Cord_app_id))
            {
                wxMessageBox(activity_error_msg); //TODO: make a checkbox for this error, asking whether to show this again
                return 0;
            }
        }
        if (!rate_limit.IsRunning())
        {
            m_rp_data.details = editor->GetShortName().ToStdString();
            m_rp_data.state = editor->GetProjectFile()->GetParentProject()->GetTitle().ToStdString();
            rp_update_needed = false;

            Connect(rate_limit.GetId(), wxEVT_TIMER, wxTimerEventHandler(Code__Cord::handle_rate_limit), NULL, this);
            rate_limit.Start(rate_limit_interval, true);

            bool error_flag = discord.set_activity(details_prefix.ToStdString() + m_rp_data.details.ToStdString(), state_prefix.ToStdString() + m_rp_data.state.ToStdString(), m_rp_data.large_image_key.ToStdString(), m_rp_data.large_image_text.ToStdString(), true, m_rp_data.timestamp); //need to re-make this function to allow for certain parameter to be blank, Discord requires for each parameter at least 2 chracters long (i imagine we can just remove the details bit from the JSON payload)
            if (!error_flag)
            {
                wxMessageBox(activity_error_msg); //TODO: make a checkbox for this error, asking whether to show this again
            }
            return error_flag;
        }
        else
        {
            rp_update_needed = true;
            return -1;
        }
    }
    return 0;
}

void Code__Cord::handle_rate_limit(wxTimerEvent &time_event)
{
    if (rp_update_needed)
    {
        update_status();
        rp_update_needed = false;
        rate_limit.Start(rate_limit_interval, true);
    }
    else
    {
        rate_limit.Stop();
    }
}

void Code__Cord::OnRelease(bool appShutDown)
{
    discord.clear_activity();
    discord.discord_disconnect();
}

int Code__Cord::Execute()
{
    rich_presence_enabled = !rich_presence_enabled;

    if (rich_presence_enabled)
    {
        if (!discord.connect_to_discord(Code__Cord_app_id))
        {
            wxMessageBox("Failed to connect to Discord. Make sure Discord is running.");
            return 0;
        }
        else if (!update_status())
        {
            wxMessageBox("You need a project opened to start the plugin, disconnected from Discord.");
            return 0;
        }
        return 1;
    }
    else
    {
        m_rp_data.details = "<No Source File Set>";
        m_rp_data.state = "<No Project Set>";
        discord.clear_activity();
        discord.discord_disconnect();
        return -1;
    }
}
