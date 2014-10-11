/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
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

#ifndef DEVICEPLUGINOPENWEATHERMAP_H
#define DEVICEPLUGINOPENWEATHERMAP_H

#include "plugin/deviceplugin.h"
#include "openweathermap.h"


class DevicePluginOpenweathermap : public DevicePlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "guru.guh.DevicePlugin" FILE "devicepluginopenweathermap.json")
    Q_INTERFACES(DevicePlugin)

public:
    explicit DevicePluginOpenweathermap();

    OpenWeatherMap *m_openweaher;

    QPair<DeviceManager::DeviceError, QString> discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params) override;
    QPair<DeviceManager::DeviceSetupStatus, QString> setupDevice(Device *device) override;
    DeviceManager::HardwareResources requiredHardware() const override;
    QPair<DeviceManager::DeviceError, QString> executeAction(Device *device, const Action &action) override;

    void guhTimer() override;

private slots:
    void searchResultsReady(const QList<QVariantMap> &cityList);
    void weatherDataReady(const QByteArray &data);

public slots:


};

#endif // DEVICEPLUGINOPENWEATHERMAP_H
