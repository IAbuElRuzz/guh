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

#include "devicepluginlgsmarttv.h"

#include "plugin/device.h"
#include "devicemanager.h"
#include "plugininfo.h"

#include <QDebug>


DevicePluginLgSmartTv::DevicePluginLgSmartTv()
{
    m_discovery = new TvDiscovery(this);

    connect(m_discovery,SIGNAL(discoveryDone(QList<TvDevice*>)),this,SLOT(discoveryDone(QList<TvDevice*>)));
}

DeviceManager::DeviceError DevicePluginLgSmartTv::discoverDevices(const DeviceClassId &deviceClassId, const ParamList &params)
{
    qDebug() << "should discover devices with params:" << params;

    if(deviceClassId != lgSmartTvDeviceClassId){
        return DeviceManager::DeviceErrorDeviceClassNotFound;
    }

    m_discovery->discover(3000);

    return DeviceManager::DeviceErrorAsync;
}

DeviceManager::DeviceSetupStatus DevicePluginLgSmartTv::setupDevice(Device *device)
{

    device->setName("LG Smart Tv (" + device->paramValue("model").toString() + ")");

    TvDevice *tvDevice = new TvDevice(this);
    tvDevice->setName(device->paramValue("name").toString());
    tvDevice->setUuid(device->paramValue("uuid").toString());
    tvDevice->setModelName(device->paramValue("model").toString());
    tvDevice->setHostAddress(QHostAddress(device->paramValue("host address").toString()));
    tvDevice->setPort(device->paramValue("port").toInt());
    tvDevice->setLocation(QUrl(device->paramValue("location").toString()));
    tvDevice->setUuid(device->paramValue("manufacturer").toString());
    // key if there is one...

    tvDevice->setupEventHandler();

    connect(tvDevice, &TvDevice::pairingFinished, this, &DevicePluginLgSmartTv::pairingFinished);
    connect(tvDevice, &TvDevice::sendCommandFinished, this, &DevicePluginLgSmartTv::sendingCommandFinished);
    connect(tvDevice, &TvDevice::statusChanged, this, &DevicePluginLgSmartTv::statusChanged);

    tvDevice->requestPairing();
    m_tvList.insert(tvDevice,device);

    return DeviceManager::DeviceSetupStatusAsync;
}

DeviceManager::HardwareResources DevicePluginLgSmartTv::requiredHardware() const
{
    return DeviceManager::HardwareResourceTimer;
}

DeviceManager::DeviceError DevicePluginLgSmartTv::executeAction(Device *device, const Action &action)
{
    TvDevice * tvDevice = m_tvList.key(device);

    if(action.actionTypeId() == commandVolumeUpActionTypeId){
        tvDevice->sendCommand(TvDevice::VolUp, action.id());
    } else if(action.actionTypeId() == commandVolumeDownActionTypeId){
        tvDevice->sendCommand(TvDevice::VolDown, action.id());
    } else if(action.actionTypeId() == commandMuteActionTypeId){
        tvDevice->sendCommand(TvDevice::Mute, action.id());
    } else if(action.actionTypeId() == commandChannelUpActionTypeId){
        tvDevice->sendCommand(TvDevice::ChannelUp, action.id());
    } else if(action.actionTypeId() == commandChannelDownActionTypeId){
        tvDevice->sendCommand(TvDevice::ChannelDown, action.id());
    } else if(action.actionTypeId() == commandPowerOffActionTypeId){
        tvDevice->sendCommand(TvDevice::Power, action.id());
    } else if(action.actionTypeId() == commandArrowUpActionTypeId){
        tvDevice->sendCommand(TvDevice::Up, action.id());
    } else if(action.actionTypeId() == commandArrowDownActionTypeId){
        tvDevice->sendCommand(TvDevice::Down, action.id());
    } else if(action.actionTypeId() == commandArrowLeftActionTypeId){
        tvDevice->sendCommand(TvDevice::Left, action.id());
    } else if(action.actionTypeId() == commandArrowRightActionTypeId){
        tvDevice->sendCommand(TvDevice::Right, action.id());
    } else if(action.actionTypeId() == commandOkActionTypeId){
        tvDevice->sendCommand(TvDevice::Ok, action.id());
    } else if(action.actionTypeId() == commandBackActionTypeId){
        tvDevice->sendCommand(TvDevice::Back, action.id());
    } else if(action.actionTypeId() == commandHomeActionTypeId){
        tvDevice->sendCommand(TvDevice::Home, action.id());
    } else if(action.actionTypeId() == commandInputSourceActionTypeId){
        tvDevice->sendCommand(TvDevice::ExternalInput, action.id());
    } else if(action.actionTypeId() == commandExitActionTypeId){
        tvDevice->sendCommand(TvDevice::Exit, action.id());
    } else if(action.actionTypeId() == commandInfoActionTypeId){
        tvDevice->sendCommand(TvDevice::Info, action.id());
    } else if(action.actionTypeId() == commandMyAppsActionTypeId){
        tvDevice->sendCommand(TvDevice::MyApps, action.id());
    } else if(action.actionTypeId() == commandProgramListActionTypeId){
        tvDevice->sendCommand(TvDevice::ProgramList, action.id());
    } else {
        return DeviceManager::DeviceErrorActionTypeNotFound;
    }
    return DeviceManager::DeviceErrorAsync;
}

void DevicePluginLgSmartTv::deviceRemoved(Device *device)
{
    if (!m_tvList.values().contains(device)) {
        return;
    }

    TvDevice *tvDevice= m_tvList.key(device);
    qDebug() << "remove LG Smart Tv  " << tvDevice->modelName();
    m_tvList.remove(tvDevice);
    tvDevice->deleteLater();
}

void DevicePluginLgSmartTv::guhTimer()
{
    foreach (TvDevice *tvDevice, m_tvList.keys()) {
        tvDevice->refresh();
    }
}

void DevicePluginLgSmartTv::discoveryDone(QList<TvDevice*> tvList)
{
    QList<DeviceDescriptor> deviceDescriptors;
    foreach (TvDevice *device, tvList) {
        DeviceDescriptor descriptor(lgSmartTvDeviceClassId, "Lg Smart Tv", device->modelName());
        ParamList params;
        params.append(Param("name", device->name()));
        params.append(Param("uuid", device->uuid()));
        params.append(Param("model", device->modelName()));
        params.append(Param("host address", device->hostAddress().toString()));
        params.append(Param("location", device->location().toString()));
        params.append(Param("port", device->port()));
        params.append(Param("manufacturer", device->manufacturer()));
        params.append(Param("key", device->key()));
        descriptor.setParams(params);
        deviceDescriptors.append(descriptor);
    }
    emit devicesDiscovered(lgSmartTvDeviceClassId, deviceDescriptors);
}

void DevicePluginLgSmartTv::pairingFinished(const bool &success)
{
    TvDevice *tvDevice = static_cast<TvDevice*>(sender());
    Device *device = m_tvList.value(tvDevice);

    if(success){
        emit deviceSetupFinished(device,DeviceManager::DeviceSetupStatusSuccess);
        tvDevice->refresh();
    }else{
        emit deviceSetupFinished(device,DeviceManager::DeviceSetupStatusFailure);
    }
}

void DevicePluginLgSmartTv::sendingCommandFinished(const bool &success, const ActionId &actionId)
{
    if(success){
        emit actionExecutionFinished(actionId,DeviceManager::DeviceErrorNoError);
    }else{
        emit actionExecutionFinished(actionId,DeviceManager::DeviceErrorHardwareFailure);
    }
}

void DevicePluginLgSmartTv::statusChanged()
{
    TvDevice *tvDevice = static_cast<TvDevice*>(sender());
    Device *device = m_tvList.value(tvDevice);

    device->setStateValue(tvReachableStateTypeId, tvDevice->reachable());
    device->setStateValue(tv3DModeStateTypeId, tvDevice->is3DMode());
    device->setStateValue(tvVolumeLevelStateTypeId, tvDevice->volumeLevel());
    device->setStateValue(tvMuteStateTypeId, tvDevice->mute());
    device->setStateValue(tvChannelTypeStateTypeId, tvDevice->channelType());
    device->setStateValue(tvChannelNameStateTypeId, tvDevice->channelName());
    device->setStateValue(tvChannelNumberStateTypeId, tvDevice->channelNumber());
    device->setStateValue(tvProgramNameStateTypeId, tvDevice->programName());
    device->setStateValue(tvInputSourceIndexStateTypeId, tvDevice->inputSourceIndex());
    device->setStateValue(tvInputSourceLabelNameStateTypeId, tvDevice->inputSourceLabelName());
}


