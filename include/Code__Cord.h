/***************************************************************
 * Name:      Code::Cord
 * Purpose:   Code::Blocks plugin
 * Author:    Mr.B (Benjamin Steward) (ben.steward64@gmail.com)
 * Created:   2025-06-21
 * Copyright: Mr.B (Benjamin Steward)
 * License:   GPL
 **************************************************************/

#ifndef CODE__CORD_H_INCLUDED
#define CODE__CORD_H_INCLUDED

// For compilers that support precompilation, includes <wx/wx.h>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <cbplugin.h> // for "class cbPlugin"
#include <logmanager.h> //I think a good amount of these includes are redundant, please trim the fat :/
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include <csignal>

//WxWidgets Headers

#include <wx/filename.h>
#include <wx/filesys.h>
#include <wx/fs_arc.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/stdpaths.h>
#include <wx/time.h>

//Code::Blocks SDK Headers

#include <sdk.h>
#include <configurationpanel.h>
#include <logmanager.h>
#include <projectmanager.h>
#include <editormanager.h>
#include <cbeditor.h>
#include <projectfile.h>
#include <cbproject.h>
#include <sdk_events.h>

#include "Discord_IPC_Handler.h"

class Code__Cord : public cbToolPlugin
{
    private: //categorise these members properly :>
        std::string Code__Cord_app_id= "1393546362997571634";
        bool rich_presence_enabled = false;
        wxString plugin_dir = wxGetHomeDir() + "/.local/share/codeblocks/";
        wxTimer rate_limit;
        int rate_limit_interval = 12000;
        bool rp_update_needed = false;
        cbEditor *editor;
        wxString details_prefix = "Editing file: "; //a string to store our prefixes like "Working on: "
        wxString state_prefix = "Working on: "; //make this and details_prefix configurable by the user
        struct rp_data //details we use for Rich Presence
        {
            wxString details = "";
            wxString state = "";
            wxString large_image_key = "";
            wxString large_image_text = "";
            int64_t timestamp = 0;
        };
        rp_data m_rp_data;
        Discord_IPC_handler discord;

        bool update_status();
        void handle_rate_limit(wxTimerEvent &time_event);
        void OnProjectRename(CodeBlocksEvent &event);
        void OnEditorSwitch(CodeBlocksEvent &event);
        void OnProjectFileAddOrRemove(CodeBlocksEvent &event);
    public:
        Code__Cord();
        virtual ~Code__Cord();

        /** Return the plugin's configuration priority.
          * This is a number (default is 50) that is used to sort plugins
          * in configuration dialogs. Lower numbers mean the plugin's
          * configuration is put higher in the list.
          */
        virtual int GetConfigurationPriority() const { return 50; }

        /** Return the configuration group for this plugin. Default is cgUnknown.
          * Notice that you can logically OR more than one configuration groups,
          * so you could set it, for example, as "cgCompiler | cgContribPlugin".
          */
        virtual int GetConfigurationGroup() const { return cgUnknown; }

        /** Return plugin's configuration panel.
          * @param parent The parent window.
          * @return A pointer to the plugin's cbConfigurationPanel. It is deleted by the caller.
          */
        virtual cbConfigurationPanel* GetProjectConfigurationPanel(wxWindow* parent, cbProject* project){ return 0; }

        /** @brief Execute the plugin.
          *
          * This is the only function needed by a cbToolPlugin.
          * This will be called when the user selects the plugin from the "Plugins"
          * menu.
          */
        virtual int Execute();
    protected:
        virtual void OnAttach();
        virtual void OnRelease(bool appShutDown);
};

#endif
