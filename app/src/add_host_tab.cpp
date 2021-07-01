//
//  add_host_tab.cpp
//  Moonlight
//
//  Created by XITRIX on 26.05.2021.
//

#include "add_host_tab.hpp"
#include "hosts_tabs_view.hpp"
#include "GameStreamClient.hpp"
#include "helper.hpp"

AddHostTab::AddHostTab()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/tabs/add_host.xml");

    hostIP->setText("Host IP");
    hostIP->setValue("10.0.0.19");
    
    connect->setText("Connect");
    connect->registerClickAction([this](View* view) {
        connectHost(hostIP->getValue());
        return true;
    });
    
    if (GameStreamClient::instance().can_find_host())
        findHost();
    else
    {
        searchHeader->setTitle("Search is unavailable");
        loader->setVisibility(brls::Visibility::GONE);
    }
}

void AddHostTab::findHost()
{
    ASYNC_RETAIN
    GameStreamClient::instance().find_host([ASYNC_TOKEN](GSResult<Host> result) {
        ASYNC_RELEASE
        
        if (result.isSuccess()) {
            Host host = result.value();
            
            brls::DetailCell* hostButton = new brls::DetailCell();
            hostButton->setText(host.hostname);
            hostButton->setDetailText(host.address);
            hostButton->setDetailTextColor(brls::Application::getTheme()["brls/text_disabled"]);
            hostButton->registerClickAction([this, host](View* view) {
                connectHost(host.address);
                return true;
            });
            searchBox->addView(hostButton);
            loader->setVisibility(brls::Visibility::GONE);
        } else {
            showError(result.error(), []{});
        }
    });
}

void AddHostTab::connectHost(std::string address)
{
    ASYNC_RETAIN
    GameStreamClient::instance().connect(address, [ASYNC_TOKEN](GSResult<SERVER_DATA> result) {
        ASYNC_RELEASE
        if (result.isSuccess())
        {
            Host host
            {
                .address = result.value().address,
                .hostname = result.value().hostname,
                .mac = result.value().mac
            };
            
            if (result.value().paired)
            {
                showError("Already paired", [host] {
                    Settings::instance().add_host(host);
                    HostsTabs::getInstanse()->refillTabs();
                });
                return;
            }
            
            char pin[5];
            sprintf(pin, "%d%d%d%d", (int)random() % 10, (int)random() % 10, (int)random() % 10, (int)random() % 10);
            
            brls::Dialog* dialog = new brls::Dialog("Pair up\n\nEnter " + std::string(pin) + " on your host device");
            dialog->setCancelable(false);
            dialog->open();
            
            ASYNC_RETAIN
            GameStreamClient::instance().pair(result.value().address, pin, [ASYNC_TOKEN, host, dialog](GSResult<bool> result) {
                ASYNC_RELEASE
                dialog->dismiss([result, host] {
                    if (result.isSuccess())
                    {
                        Settings::instance().add_host(host);
                        HostsTabs::getInstanse()->refillTabs();
                    }
                    else
                    {
                        showError("Error\n\n" + result.error(), [] {});
                    }
                });
            });
        }
        else
        {
            showError("Error\n\n" + result.error(), [] {});
        }
    });
}

brls::View* AddHostTab::create()
{
    // Called by the XML engine to create a new ComponentsTab
    return new AddHostTab();
}
