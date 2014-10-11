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

/*!
    \class DeviceClass
    \brief Describes \l{Device}{Devices}.

    \ingroup devices
    \inmodule libguh

    It holds information general information about devices and their vendors and
    describes what actions, events and states a device supports. As this is
    just a description of device and does not represent actual \l{Device}{Devices},
    the actions, events and states are described in form of \l{EventType},
    \l{StateType} and \l{ActionType}

    \sa Device
*/

#include "deviceclass.h"

/*! Constructs a DeviceClass with the give \a pluginId and \a id.
    When implementing a plugin, create a DeviceClass for each device you support.
    Generate a new uuid (e.g. uuidgen) and hardode it into the plugin. The id
    should never change or it will appear as a new DeviceClass in the system.
 */
DeviceClass::DeviceClass(const PluginId &pluginId, const VendorId &vendorId, const DeviceClassId &id):
    m_id(id),
    m_vendorId(vendorId),
    m_pluginId(pluginId),
    m_createMethods(CreateMethodUser),
    m_setupMethod(SetupMethodJustAdd)
{

}

/*! Returns the id of this DeviceClass. */
DeviceClassId DeviceClass::id() const
{
    return m_id;
}

/*! Returns the VendorId for this DeviceClass */
VendorId DeviceClass::vendorId() const
{
    return m_vendorId;
}

/*! Returns the pluginId this DeviceClass is managed by. */
PluginId DeviceClass::pluginId() const
{
    return m_pluginId;
}

/*! Returns true if this DeviceClass's id, vendorId and pluginId are valid uuids. */
bool DeviceClass::isValid() const
{
    return !m_id.isNull() && !m_vendorId.isNull() && !m_pluginId.isNull();
}

/*! Returns the name of this DeviceClass's. This is visible to the user. */
QString DeviceClass::name() const
{
    return m_name;
}

/*! Set the \a name of this DeviceClass's. This is visible to the user. */
void DeviceClass::setName(const QString &name)
{
    m_name = name;
}

/*! Returns the statesTypes of this DeviceClass. \{Device}{Devices} created
    from this DeviceClass must have their states matching to this template. */
QList<StateType> DeviceClass::stateTypes() const
{
    return m_stateTypes;
}

/*! Set the \a stateTypes of this DeviceClass. \{Device}{Devices} created
    from this DeviceClass must have their states matching to this template. */
void DeviceClass::setStateTypes(const QList<StateType> &stateTypes)
{
    m_stateTypes = stateTypes;

    m_allEventTypes = m_eventTypes;
    foreach (const StateType &stateType, m_stateTypes) {
        EventType eventType(EventTypeId(stateType.id().toString()));
        eventType.setName(QString("%1 changed").arg(stateType.name()));
        ParamType paramType("value", stateType.type());
        eventType.setParamTypes(QList<ParamType>() << paramType);
        m_allEventTypes.append(eventType);
    }
}

/*! Returns the eventTypes of this DeviceClass. \{Device}{Devices} created
    from this DeviceClass must have their events matching to this template. */
QList<EventType> DeviceClass::eventTypes() const
{
    return m_allEventTypes;
}

/*! Set the \a eventTypes of this DeviceClass. \{Device}{Devices} created
    from this DeviceClass must have their events matching to this template. */
void DeviceClass::setEventTypes(const QList<EventType> &eventTypes)
{
    m_eventTypes = eventTypes;

    m_allEventTypes = m_eventTypes;
    foreach (const StateType &stateType, m_stateTypes) {
        EventType eventType(EventTypeId(stateType.id().toString()));
        eventType.setName(QString("%1 changed").arg(stateType.name()));
        ParamType paramType("value", stateType.type());
        eventType.setParamTypes(QList<ParamType>() << paramType);
        m_allEventTypes.append(eventType);
    }
}

/*! Returns the actionTypes of this DeviceClass. \{Device}{Devices} created
    from this DeviceClass must have their actions matching to this template. */
QList<ActionType> DeviceClass::actionTypes() const
{
    return m_actionTypes;
}

/*! Set the \a actionTypes of this DeviceClass. \{Device}{Devices} created
    from this DeviceClass must have their actions matching to this template. */
void DeviceClass::setActionTypes(const QList<ActionType> &actionTypes)
{
    m_actionTypes = actionTypes;
}

/*! Returns the params description of this DeviceClass. \{Device}{Devices} created
    from this DeviceClass must have their params matching to this template. */
QList<ParamType> DeviceClass::paramTypes() const
{
    return m_paramTypes;
}

/*! Set the \a params of this DeviceClass. \{Device}{Devices} created
    from this DeviceClass must have their actions matching to this template. */
void DeviceClass::setParamTypes(const QList<ParamType> &params)
{
    m_paramTypes = params;
}

QList<ParamType> DeviceClass::discoveryParamTypes() const
{
    return m_discoveryParamTypes;
}

void DeviceClass::setDiscoveryParamTypes(const QList<ParamType> &params)
{
    m_discoveryParamTypes = params;
}

DeviceClass::CreateMethods DeviceClass::createMethods() const
{
    return m_createMethods;
}

void DeviceClass::setCreateMethods(DeviceClass::CreateMethods createMethods)
{
    m_createMethods = createMethods;
}

DeviceClass::SetupMethod DeviceClass::setupMethod() const
{
    return m_setupMethod;
}

void DeviceClass::setSetupMethod(DeviceClass::SetupMethod setupMethod)
{
    m_setupMethod = setupMethod;
}

QString DeviceClass::pairingInfo() const
{
    return m_pairingInfo;
}

void DeviceClass::setPairingInfo(const QString &pairingInfo)
{
    m_pairingInfo = pairingInfo;
}

/*! Compare this \a deviceClass to another. This is effectively the same as calling a.id() == b.id(). Returns true if the ids match.*/
bool DeviceClass::operator==(const DeviceClass &deviceClass) const
{
    return m_id == deviceClass.id();
}
