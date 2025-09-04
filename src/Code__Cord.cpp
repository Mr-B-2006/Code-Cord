#include "Code__Cord.h"

/*
FINAL STRETCH TO DOS:
~COMMENT AND REFACTOR CODE WHERE POSSIBLE/NEEDED ALSO DO A LITTLE BIT OF BUG TESTING
~GET GIT REPO READY AND PUT ON GITHUB (MAKE A NEW BRANCH FOR EDITING WITH OUR SILLY AHH COMMENTS IN, AND MAKE ONE JUST FOR UPLOADING TO GITHUB THAT DOESNT HAVE THAT SHIT AND LOOKS NICE AND *PROFESHUNAL*)
~SHOW ERRORS WHEN YOU CANT CONNECT TO DISCORD IN update_status() AND GIVE USER A RETRY PROMPT
~GET CONFIG STUFF WORKING
~UPLOAD CONFIG VERSION TO GITHUB (THIS WILL PROLLY BE THE FINAL FEATURE UPDATE :D )
~PORT TO WINDOWS AND MAYBE MAC/BSD IF POSSIBLE
~UPLOAD THIS FINAL VERSION TO GITHUB THEN THE PROJECT IS COMPLETE :DDDDD
*/

std::atomic<bool> running = true;
// Register the plugin with Code::Blocks.
// We are using an anonymous namespace so we don't litter the global one.

namespace
{
    PluginRegistrant<Code__Cord> reg(_T("Code__Cord"));
}

Code__Cord::Code__Cord()  {    }
Code__Cord::~Code__Cord() {    }

void Code__Cord::OnAttach()
{
    rate_limit.SetOwner(this); //associates the rate_limit timer with our object

    Manager::Get()->RegisterEventSink(cbEVT_EDITOR_SWITCHED, new cbEventFunctor<Code__Cord, CodeBlocksEvent>(this, &Code__Cord::OnEditorSwitch)); //registering a function that shall be executed when we switch tabs
    Manager::Get()->RegisterEventSink(cbEVT_PROJECT_FILE_ADDED, new cbEventFunctor<Code__Cord, CodeBlocksEvent>(this, &Code__Cord::OnProjectFileAddOrRemove));
    Manager::Get()->RegisterEventSink(cbEVT_PROJECT_FILE_REMOVED, new cbEventFunctor<Code__Cord, CodeBlocksEvent>(this, &Code__Cord::OnProjectFileAddOrRemove));
    Manager::Get()->RegisterEventSink(cbEVT_PROJECT_RENAMED, new cbEventFunctor<Code__Cord, CodeBlocksEvent>(this, &Code__Cord::OnProjectRename));

    m_rp_data.large_image_key = "logo_transparent";
    m_rp_data.large_image_text = "Code::Blocks IDE";
    m_rp_data.timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); //gives us the current time in seconds, Discord uses this for recording our session time

    if (!rich_presence_enabled) //for when we get config working
    {
        discord.clear_activity();
    }
}

void Code__Cord::OnProjectRename(CodeBlocksEvent &event)
{
    editor = Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor();
    update_status();
}

void Code__Cord::OnProjectFileAddOrRemove(CodeBlocksEvent &event)
{
    editor = Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor(); //there may be a crash here? INVETIGATE
    update_status();
}

bool Code__Cord::update_status()
{
    if (editor && editor->GetProjectFile() && editor->GetProjectFile() &&                                                                                          //check that neither the editor and nor it's members point to a null pointer
        m_rp_data.details == editor->GetShortName().ToStdString() && m_rp_data.state == editor->GetProjectFile()->GetParentProject()->GetTitle().ToStdString() && //prevents status updates that contain the same details and state data (which may happen due to cbEVT_EDITOR_SWITCHED occurring sometimes we don really need it to (such as focusing the editor after tabbing out))
        rich_presence_enabled && discord.is_connected() )                                                                                                        //check that we are connected to the Discord IPC socket
    {
        m_rp_data.details = editor->GetShortName().ToStdString();
        m_rp_data.state = editor->GetProjectFile()->GetParentProject()->GetTitle().ToStdString();

        if (!rate_limit.IsRunning())
        {
            rp_update_needed = false;
            rate_limit.Start(rate_limit_interval, true);

            Connect(rate_limit.GetId(), wxEVT_TIMER, wxTimerEventHandler(Code__Cord::handle_rate_limit), NULL, this);
            return discord.set_activity(details_prefix.ToStdString() + m_rp_data.details.ToStdString(), state_prefix.ToStdString() + m_rp_data.state.ToStdString(), m_rp_data.large_image_key.ToStdString(), m_rp_data.large_image_text.ToStdString(), true, m_rp_data.timestamp); //need to re-make this function to allow for certain parameter to be blank, Discord requires for each parameter at least 2 chracters long (i imagine we can just remove the details bit from the JSON payload)
        }
        else
        {
            rp_update_needed = true;
            return false;
        }
    }
    else if (rich_presence_enabled && !discord.is_connected())
    {
        //NOT IMPLEMENTED YET, READ BELOW
        //tell user discord is disconnected, they may wanna try restarting Discord or checking their internet connection(?) (unsure what happens with our IPC stuff when we dont have an internet connection (as IPC doesnt need internet)), also ask the user if they wanna retry connecting to discord, also give them a checkbox to ask if they wanna ignore these errors (which they can turn back on in the config menu)
    }
    return false;
}

void Code__Cord::handle_rate_limit(wxTimerEvent &time_event)
{

    if (rp_update_needed)
    {
        editor = Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor();
        update_status();
        rp_update_needed = false;
        rate_limit.Start(rate_limit_interval, true);
    }
    else
    {
        m_rp_data.details = editor->GetShortName().ToStdString();
        m_rp_data.state = editor->GetProjectFile()->GetParentProject()->GetTitle().ToStdString();
        rate_limit.Stop();
    }
}

void Code__Cord::OnEditorSwitch(CodeBlocksEvent &event)
{
    editor = Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor();
    update_status();
}

void Code__Cord::OnRelease(bool appShutDown)
{
    discord.clear_activity();
    discord.discord_disconnect();
}

int Code__Cord::Execute() //TO DO: check if Discord has been disconnected, if so, reconnect (altho really, we might wanna make this a check we do like every 10 seconds but idk ¯\_(ツ)_/¯)
                         //obv also we need to make a config window and save our config
{
    rich_presence_enabled = !rich_presence_enabled;

    if (rich_presence_enabled && !discord.is_connected())
    {
        if (!discord.discord_connect(Code__Cord_app_id))
        {
            wxMessageBox("Failed to connect to Discord. Make sure Discord is running.");
            return -1;
        }
    }
    else if (!rich_presence_enabled)
    {
        discord.clear_activity();
        discord.discord_disconnect();
    }
    else
    {
        editor = Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor();
        if (!update_status())
        {
            wxMessageBox("You need a project opened to start the plugin");
        }
    }
    return 1;
}
