/***************************************************************
 * Name:      Code::Cord                                       *
 * Purpose:   Code::Blocks plugin                              *
 * Author:    Mr.B (Benjamin Steward) (ben.steward64@gmail.com)*
 * Created:   21/06/2025                                       *
 * Copyright: Mr.B (Benjamin Steward)                          *
 * License:   GPL                                              *
 **************************************************************/

#ifndef CODE__CORD_H_INCLUDED
#define CODE__CORD_H_INCLUDED

//For compilers that support precompilation, includes <wx/wx.h>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <cbplugin.h>
#include <iostream>
#include <atomic>

#include <sdk.h>
#include <editormanager.h>
#include <cbeditor.h>
#include <cbproject.h>

#include "Discord_IPC_Handler.h"

class Code__Cord : public cbToolPlugin
{
    private: //categorise these members properly :>
        cbEditor *editor;
        const std::string Code__Cord_app_id = "1393546362997571634";
        wxTimer rate_limit;
        const int rate_limit_interval = 12000;
        wxString details_prefix = "Editing file: "; //a string to store our prefixes like "Working on: "
        wxString state_prefix = "Working on: "; //make this and details_prefix configurable by the user

        struct rp_data //details we use for Rich Presence
        {
            wxString details = "<No Source File Set>"; //will hold source file name, in "details" rich presence parameter, set with a default value so it doesnt get detected as duplicate and to show if some kind of error occured
            wxString state = "<No Project Set>"; //will hold project name, in "details" rich presence parameter, set with a default value so it doesnt get detected as duplicate and to show if some kind of error occured
            wxString large_image_key = "logo_transparent";
            wxString large_image_text = "Code::Blocks IDE";
            int64_t timestamp = 0;
        };
        rp_data m_rp_data;
        bool rich_presence_enabled = false;
        bool rp_update_needed = false;
        Discord_IPC_handler discord;
        int update_status();
        void handle_rate_limit(wxTimerEvent &time_event);
        void OnActivityUpdateEvent(CodeBlocksEvent &event);

    public:
        Code__Cord();
        virtual ~Code__Cord();

        virtual int GetConfigurationPriority() const { return 50; }
        virtual int GetConfigurationGroup() const { return cgUnknown; }
        virtual cbConfigurationPanel* GetProjectConfigurationPanel(wxWindow* parent, cbProject* project){ return 0; }
        virtual int Execute();

    protected:
        virtual void OnAttach();
        virtual void OnRelease(bool appShutDown);
};

#endif
