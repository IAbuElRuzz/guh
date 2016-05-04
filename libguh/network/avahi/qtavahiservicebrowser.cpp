/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2016 Simon Stürz <simon.stuerz@guh.guru>                 *
 *                                                                         *
 *  This file is part of guh.                                              *
 *                                                                         *
 *  Guh is free software: you can redistribute it and/or modify            *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 2 of the License.                *
 *                                                                         *
 *  Guh is distributed in the hope that it will be useful,                 *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with guh. If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "qtavahiservicebrowser.h"
#include "qtavahiservicebrowser_p.h"
#include "loggingcategories.h"

#include <avahi-common/error.h>

QtAvahiServiceBrowser::QtAvahiServiceBrowser(QObject *parent) :
    QObject(parent),
    d_ptr(new QtAvahiServiceBrowserPrivate(new QtAvahiClient))
{
    connect(d_ptr->client, &QtAvahiClient::clientStateChanged, this, &QtAvahiServiceBrowser::onClientStateChanged);
}

QtAvahiServiceBrowser::~QtAvahiServiceBrowser()
{
    // Delete each service browser
    foreach (const QString &serviceType, d_ptr->serviceBrowserTable.keys()) {
        AvahiServiceBrowser *browser = d_ptr->serviceBrowserTable.take(serviceType);
        if (browser) {
            avahi_service_browser_free(browser);
        }
    }

    // Delete the service type browser
    if (d_ptr->serviceTypeBrowser)
        avahi_service_type_browser_free(d_ptr->serviceTypeBrowser);

    delete d_ptr;
}

void QtAvahiServiceBrowser::enable()
{
    d_ptr->client->start();
}

QList<AvahiServiceEntry> QtAvahiServiceBrowser::serviceEntries() const
{
    return m_serviceEntries;
}

void QtAvahiServiceBrowser::onClientStateChanged(const QtAvahiClient::QtAvahiClientState &state)
{
    if (state == QtAvahiClient::QtAvahiClientStateRunning) {
        qCDebug(dcAvahi()) << "Service browser client connected.";
        // Return if we already have a service type browser
        if (d_ptr->serviceTypeBrowser)
            return;

        d_ptr->serviceTypeBrowser = avahi_service_type_browser_new(d_ptr->client->client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, 0, (AvahiLookupFlags) 0, QtAvahiServiceBrowserPrivate::callbackServiceTypeBrowser, this);
    } else if (state == QtAvahiClient::QtAvahiClientStateFailure) {
        qCWarning(dcAvahi()) << "Service browser client failure:" << d_ptr->client->errorString();
    }
}

void QtAvahiServiceBrowser::createServiceBrowser(const char *serviceType)
{
    // create a new service browser for the given serviceType
    AvahiServiceBrowser *browser = avahi_service_browser_new(d_ptr->client->client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, serviceType, NULL, (AvahiLookupFlags) 0,  QtAvahiServiceBrowserPrivate::callbackServiceBrowser, this);
    d_ptr->serviceBrowserTable.insert(serviceType, browser);
}

