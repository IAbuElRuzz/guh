/****************************************************************************
 *                                                                          *
 *  This file is part of guh.                                               *
 *                                                                          *
 *  Guh is free software: you can redistribute it and/or modify             *
 *  it under the terms of the GNU General Public License as published by    *
 *  the Free Software Foundation, version 2 of the License.                 *
 *                                                                          *
 *  Guh is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *  GNU General Public License for more details.                            *
 *                                                                          *
 *  You should have received a copy of the GNU General Public License       *
 *  along with guh.  If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                          *
 ***************************************************************************/

/*!
    \class DeviceManager
    \brief The main entry point when interacting with \l{Device}{Devices}

    \ingroup devices
    \inmodule libguh

    The DeviceManager holds all information about supported and configured Devices in the system.

    It is also responsible for loading Plugins and managing common hardware resources between
    \l{DevicePlugin}{device plugins}.

*/

/*!
    \enum DeviceManager::HardwareResource

    This enum type specifies hardware resources which can be requested by \l{DevicePlugin}{DevicePlugins}.

    \value HardwareResourceNone
        No Resource required.
    \value HardwareResourceRadio433
        Refers to the 433 MHz radio.
    \value HardwareResourceRadio868
        Refers to the 868 MHz radio.
    \value HardwareResourceTimer
        Refers to the global timer managed by the \l{DeviceManager}. Plugins should not create their own timers, but rather request the global timer using the hardware resources.
*/

/*!
    \enum DeviceManager::DeviceError

    This enum type specifies the errors that can happen when working with \l{Device}{Devices}.

    \value DeviceErrorNoError
        No Error. Everything went fine.
    \value DeviceErrorDeviceNotFound
        Couldn't find a \l{Device} for the given id.
    \value DeviceErrorDeviceClassNotFound
        Couldn't find a \l{DeviceClass} for the given id.
    \value DeviceErrorActionTypeNotFound
        Couldn't find the \l{ActionType} for the given id.
    \value DeviceErrorMissingParameter
        Parameters do not comply to the template.
    \value DeviceErrorPluginNotFound
        Couldn't find the Plugin for the given id.
    \value DeviceErrorSetupFailed
        Error setting up the \{Device}. It will not be functional.
*/

/*! \fn void DeviceManager::emitEvent(const Event &event)
    The DeviceManager will emit a \l{Event} described in \a event whenever a Device
    creates one. Normally only \l{GuhCore} should connect to this and execute actions
    after checking back with the \{RulesEngine}. Exceptions might be monitoring interfaces
    or similar, but you should never directly react to this in a \l{DevicePlugin}.
*/

#include "devicemanager.h"

#include "radio433.h"

#include "device.h"
#include "deviceclass.h"
#include "deviceplugin.h"

#include <QPluginLoader>
#include <QtPlugin>
#include <QDebug>
#include <QSettings>
#include <QStringList>

/*! Constructs the DeviceManager with the given \a parent. There should only be one DeviceManager in the system created by \l{GuhCore}.
    Use \c GuhCore::instance()->deviceManager() instead to access the DeviceManager.
*/
DeviceManager::DeviceManager(QObject *parent) :
    QObject(parent),
    m_radio433(0)
{
    m_pluginTimer.setInterval(15000);
    connect(&m_pluginTimer, &QTimer::timeout, this, &DeviceManager::timerEvent);

    // Give hardware a chance to start up before loading plugins etc.
    QMetaObject::invokeMethod(this, "loadPlugins", Qt::QueuedConnection);
    QMetaObject::invokeMethod(this, "loadConfiguredDevices", Qt::QueuedConnection);
    // Make sure this is always emitted after plugins and devices are loaded
    QMetaObject::invokeMethod(this, "loaded", Qt::QueuedConnection);
}

/*! Returns all the \l{DevicePlugin}{DevicePlugins} loaded in the system. */
QList<DevicePlugin *> DeviceManager::plugins() const
{
    return m_devicePlugins.values();
}

/*! Returns the \{DevicePlugin} with the given \a id. Null if the id couldn't be found. */
DevicePlugin *DeviceManager::plugin(const QUuid &id) const
{
    return m_devicePlugins.value(id);
}

/*! Returns all the supported \l{DeviceClass}{DeviceClasses} by all \l{DevicePlugin}{DevicePlugins} loaded in the system. */
QList<DeviceClass> DeviceManager::supportedDevices() const
{
    return m_supportedDevices.values();
}

/*! Add a new configured device for the given \l{DeviceClass} and the given parameters.
 \a deviceClassId must refer to an existing \{DeviceClass} and \a params must match the parameter description in the \l{DeviceClass}. */
DeviceManager::DeviceError DeviceManager::addConfiguredDevice(const QUuid &deviceClassId, const QVariantMap &params)
{
    DeviceClass deviceClass = findDeviceClass(deviceClassId);
    if (deviceClass.id().isNull()) {
        qWarning() << "cannot find a device class with id" << deviceClassId;
        return DeviceErrorDeviceClassNotFound;
    }
    foreach (const QVariant &param, deviceClass.params()) {
        if (!params.contains(param.toMap().value("name").toString())) {
            qWarning() << "Missing parameter when creating device:" << param.toMap().value("name").toString();
            return DeviceErrorMissingParameter;
        }
        // TODO: Check if parameter type matches
    }

    DevicePlugin *plugin = m_devicePlugins.value(deviceClass.pluginId());
    if (!plugin) {
        qWarning() << "Cannot find a plugin for this device class!";
        return DeviceErrorPluginNotFound;
    }

    Device *device = new Device(plugin->pluginId(), deviceClassId, this);
    device->setName(deviceClass.name());
    device->setParams(params);
    if (setupDevice(device)) {
        m_configuredDevices.append(device);
    } else {
        qWarning() << "Failed to set up device.";
        return DeviceErrorSetupFailed;
    }

    storeConfiguredDevices();

    return DeviceErrorNoError;
}

/*! Returns the \l{Device} with the given \a id. Null if the id couldn't be found. */
Device *DeviceManager::findConfiguredDevice(const QUuid &id) const
{
    foreach (Device *device, m_configuredDevices) {
        if (device->id() == id) {
            return device;
        }
    }
    return 0;
}

/*! Returns all configured \{Device}{Devices} in the system. */
QList<Device *> DeviceManager::configuredDevices() const
{
    return m_configuredDevices;
}

/*! Returns all \l{Device}{Devices} matching the \l{DeviceClass} referred by \a deviceClassId. */
QList<Device *> DeviceManager::findConfiguredDevices(const QUuid &deviceClassId) const
{
    QList<Device*> ret;
    foreach (Device *device, m_configuredDevices) {
        if (device->deviceClassId() == deviceClassId) {
            ret << device;
        }
    }
    return ret;
}

/*! For conveninece, this returns the \{DeviceClass} that describes the \l{EventType} referred by \a eventTypeId. */
DeviceClass DeviceManager::findDeviceClassforEvent(const QUuid &eventTypeId) const
{
    foreach (const DeviceClass &deviceClass, m_supportedDevices) {
        foreach (const EventType &eventType, deviceClass.events()) {
            if (eventType.id() == eventTypeId) {
                return deviceClass;
            }
        }
    }
    return DeviceClass(QUuid(), QUuid());
}

/*! For conveninece, this returns the \{DeviceClass} with the id given by \a deviceClassId. */
DeviceClass DeviceManager::findDeviceClass(const QUuid &deviceClassId) const
{
    foreach (const DeviceClass &deviceClass, m_supportedDevices) {
        if (deviceClass.id() == deviceClassId) {
            return deviceClass;
        }
    }
    return DeviceClass(QUuid(), QUuid());
}

/*! Execute the given \{Action}.
    This will find the \l{Device} \a action refers to in \l{Action::deviceId()} and
    its \l{DevicePlugin}. Then will dispatch the execution to the \l{DevicePlugin}.*/
DeviceManager::DeviceError DeviceManager::executeAction(const Action &action)
{
    foreach (Device *device, m_configuredDevices) {
        if (action.deviceId() == device->id()) {
            // found device

            // Make sure this device has an action type with this id
            DeviceClass deviceClass = findDeviceClass(device->deviceClassId());
            bool found = false;
            foreach (const ActionType &actionType, deviceClass.actions()) {
                if (actionType.id() == action.actionTypeId()) {
                    found = true;
                }
            }
            if (!found) {
                return DeviceErrorActionTypeNotFound;
            }

            m_devicePlugins.value(device->pluginId())->executeAction(device, action);
            return DeviceErrorNoError;
        }
    }
    return DeviceErrorDeviceNotFound;
}

void DeviceManager::loadPlugins()
{
    foreach (QObject *pluginObject, QPluginLoader::staticInstances()) {
        DevicePlugin *pluginIface = qobject_cast<DevicePlugin*>(pluginObject);
        if (pluginIface) {
            qDebug() << "*** Loaded plugin" << pluginIface->pluginName();
            pluginIface->initPlugin(this);
            foreach (const DeviceClass &deviceClass, pluginIface->supportedDevices()) {
                qDebug() << "* Loaded device class:" << deviceClass.name();
                m_supportedDevices.insert(deviceClass.id(), deviceClass);
            }
            m_devicePlugins.insert(pluginIface->pluginId(), pluginIface);
            connect(pluginIface, &DevicePlugin::emitEvent, this, &DeviceManager::emitEvent);
        }
    }
}

void DeviceManager::loadConfiguredDevices()
{
    QSettings settings;
    qDebug() << "loading devices from" << settings.fileName();
    foreach (const QString &idString, settings.childGroups()) {
        settings.beginGroup(idString);
        Device *device = new Device(settings.value("pluginid").toUuid(), QUuid(idString), settings.value("deviceClassId").toUuid(), this);
        device->setName(settings.value("devicename").toString());
        device->setParams(settings.value("params").toMap());
        settings.endGroup();

        setupDevice(device);

        m_configuredDevices.append(device);
        qDebug() << "found stored device" << device->name() << idString;
    }
}

void DeviceManager::storeConfiguredDevices()
{
    QSettings settings;
    foreach (Device *device, m_configuredDevices) {
        settings.beginGroup(device->id().toString());
        settings.setValue("devicename", device->name());
        settings.setValue("deviceClassId", device->deviceClassId().toString());
        settings.setValue("pluginid", device->pluginId());
        settings.setValue("params", device->params());
        settings.endGroup();
    }
}

void DeviceManager::radio433SignalReceived(QList<int> rawData)
{
    foreach (Device *device, m_configuredDevices) {
        DeviceClass deviceClass = m_supportedDevices.value(device->deviceClassId());
        DevicePlugin *plugin = m_devicePlugins.value(deviceClass.pluginId());
        if (plugin->requiredHardware().testFlag(HardwareResourceRadio433)) {
            plugin->radioData(rawData);
        }
    }
}

void DeviceManager::timerEvent()
{
    foreach (Device *device, m_configuredDevices) {
        DeviceClass deviceClass = m_supportedDevices.value(device->deviceClassId());
        DevicePlugin *plugin = m_devicePlugins.value(deviceClass.pluginId());
        if (plugin->requiredHardware().testFlag(HardwareResourceTimer)) {
            plugin->guhTimer();
        }
    }
}

bool DeviceManager::setupDevice(Device *device)
{
    DeviceClass deviceClass = findDeviceClass(device->deviceClassId());
    DevicePlugin *plugin = m_devicePlugins.value(deviceClass.pluginId());

    QList<State> states;
    foreach (const StateType &stateType, deviceClass.states()) {
        State state(stateType.id(), device->id());
        state.setValue(stateType.defaultValue());
        states.append(state);
    }
    device->setStates(states);

    if (plugin->requiredHardware().testFlag(HardwareResourceRadio433)) {
        if (!m_radio433) {
            m_radio433 = new Radio433();
            connect(m_radio433, &Radio433::dataReceived, this, &DeviceManager::radio433SignalReceived);
        }
    }

    if (plugin->requiredHardware().testFlag(HardwareResourceTimer)) {
        if (!m_pluginTimer.isActive()) {
            m_pluginTimer.start();
            // Additionally fire off one event to initialize stuff
            QTimer::singleShot(0, this, SLOT(timerEvent()));
        }
    }

    if (!plugin->deviceCreated(device)) {
        qWarning() << "Device setup for device" << device->name() << "failed.";
        return false;
    }

    return true;
}
