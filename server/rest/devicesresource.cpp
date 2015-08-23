/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stuerz <simon.stuerz@guh.guru>                *
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
    \class guhserver::DevicesResource
    \brief This subclass of \l{RestResource} processes the REST requests for the \tt Devices namespace.

    \ingroup json
    \inmodule core

    This \l{RestResource} will be created in the \l{RestServer} and used to handle REST requests
    for the \tt {Devices} namespace of the API.

    \code
        http://localhost:3333/api/v1/devices
    \endcode

    \sa Device, RestResource, RestServer
*/


#include "devicesresource.h"
#include "httpreply.h"
#include "httprequest.h"
#include "jsontypes.h"
#include "guhcore.h"

#include <QJsonDocument>

namespace guhserver {

/*! Constructs a \l DevicesResource with the given \a parent. */
DevicesResource::DevicesResource(QObject *parent) :
    RestResource(parent)
{
    connect(GuhCore::instance(), &GuhCore::actionExecuted, this, &DevicesResource::actionExecuted);
    connect(GuhCore::instance(), &GuhCore::deviceSetupFinished, this, &DevicesResource::deviceSetupFinished);
    connect(GuhCore::instance(), &GuhCore::deviceEditFinished, this, &DevicesResource::deviceEditFinished);
    connect(GuhCore::instance(), &GuhCore::pairingFinished, this, &DevicesResource::pairingFinished);
}

/*! Returns the name of the \l{RestResource}. In this case \b devices.

    \sa RestResource::name()
*/
QString DevicesResource::name() const
{
    return "devices";
}

/*! This method will be used to process the given \a request and the given \a urlTokens. The request
    has to be in this namespace. Returns the resulting \l HttpReply.

    \sa HttpRequest, HttpReply, RestResource::proccessRequest()
*/
HttpReply *DevicesResource::proccessRequest(const HttpRequest &request, const QStringList &urlTokens)
{
    m_device = 0;

    // get the main resource
    if (urlTokens.count() >= 4) {
        DeviceId deviceId = DeviceId(urlTokens.at(3));
        if (deviceId.isNull()) {
            qCWarning(dcRest) << "Could not parse DeviceId:" << urlTokens.at(3);
            return createErrorReply(HttpReply::BadRequest);
        }
        m_device = GuhCore::instance()->findConfiguredDevice(deviceId);
        if (!m_device) {
            qCWarning(dcRest) << "Could find any device with DeviceId:" << urlTokens.at(3);
            return createErrorReply(HttpReply::NotFound);
        }
    }

    // check method
    HttpReply *reply;
    switch (request.method()) {
    case HttpRequest::Get:
        reply = proccessGetRequest(request, urlTokens);
        break;
    case HttpRequest::Post:
        reply = proccessPostRequest(request, urlTokens);
        break;
    case HttpRequest::Put:
        reply = proccessPutRequest(request, urlTokens);
        break;
    case HttpRequest::Delete:
        reply = proccessDeleteRequest(request, urlTokens);
        break;
    default:
        reply = createErrorReply(HttpReply::BadRequest);
        break;
    }
    return reply;
}

HttpReply *DevicesResource::proccessGetRequest(const HttpRequest &request, const QStringList &urlTokens)
{
    Q_UNUSED(request)

    // GET /api/v1/devices
    if (urlTokens.count() == 3)
        return getConfiguredDevices();

    // GET /api/v1/devices/{deviceId}
    if (urlTokens.count() == 4)
        return getConfiguredDevice(m_device);

    // GET /api/v1/devices/{deviceId}/states
    if (urlTokens.count() == 5 && urlTokens.at(4) == "states")
        return getDeviceStateValues(m_device);


    // /api/v1/devices/{deviceId}/states/{stateTypeId}
    if (urlTokens.count() >= 6 && urlTokens.at(4) == "states") {
        StateTypeId stateTypeId = StateTypeId(urlTokens.at(5));
        if (stateTypeId.isNull()) {
            qCWarning(dcRest) << "Could not parse StateTypeId:" << urlTokens.at(5);
            return createErrorReply(HttpReply::BadRequest);
        }

        if (!m_device->hasState(stateTypeId)){
            qCWarning(dcRest) << "This device has no StateTypeId:" << urlTokens.at(5);
            return createErrorReply(HttpReply::NotFound);
        }
        return getDeviceStateValue(m_device, stateTypeId);
    }
    return createErrorReply(HttpReply::NotImplemented);
}

HttpReply *DevicesResource::proccessDeleteRequest(const HttpRequest &request, const QStringList &urlTokens)
{
    Q_UNUSED(request)

    // DELETE /api/v1/devices
    if (urlTokens.count() == 3)
        return createErrorReply(HttpReply::BadRequest);

    // DELETE /api/v1/devices/{deviceId}
    if (urlTokens.count() == 4)
        return removeDevice(m_device);

    // TODO: /api/v1/devices/{deviceId}?ruleId={ruleId}&removePolicy={RemovePolicy}

    return createErrorReply(HttpReply::NotImplemented);
}

HttpReply *DevicesResource::proccessPutRequest(const HttpRequest &request, const QStringList &urlTokens)
{
    Q_UNUSED(request)
    // PUT /api/v1/devices
    if (urlTokens.count() == 3)
        return createErrorReply(HttpReply::BadRequest);

    // PUT /api/v1/devices/{deviceId}
    if (urlTokens.count() == 4)
        return editDevice(m_device, request.payload());

    return createErrorReply(HttpReply::NotImplemented);
}

HttpReply *DevicesResource::proccessPostRequest(const HttpRequest &request, const QStringList &urlTokens)
{

    // POST /api/v1/devices
    if (urlTokens.count() == 3)
        return addConfiguredDevice(request.payload());

    // POST /api/v1/devices/pair
    if (urlTokens.count() == 4 && urlTokens.at(3) == "pair")
        return pairDevice(request.payload());

    // POST /api/v1/devices/confirmpairing
    if (urlTokens.count() == 4 && urlTokens.at(3) == "confirmpairing")
        return confirmPairDevice(request.payload());

    // POST /api/v1/devices/{deviceId}
    if (urlTokens.count() == 4)
        return createErrorReply(HttpReply::BadRequest);

    // POST /api/v1/devices/{deviceId}/execute/{actionTypeId}
    if (urlTokens.count() >= 6 && urlTokens.at(4) == "execute") {
        ActionTypeId actionTypeId = ActionTypeId(urlTokens.at(5));
        if (actionTypeId.isNull()) {
            qCWarning(dcRest) << "Could not parse ActionTypeId:" << urlTokens.at(5);
            return createErrorReply(HttpReply::BadRequest);
        }
        bool found = false;
        DeviceClass deviceClass = GuhCore::instance()->findDeviceClass(m_device->deviceClassId());
        foreach (const ActionType actionType, deviceClass.actionTypes()) {
            if (actionType.id() == actionTypeId) {
                found = true;
                break;
            }
        }
        if (!found) {
            qCWarning(dcRest) << "Could not find ActionTypeId:" << actionTypeId.toString();
            return createErrorReply(HttpReply::NotFound);
        }

        return executeAction(m_device, actionTypeId, request.payload());
    }

    return createErrorReply(HttpReply::NotImplemented);
}

HttpReply *DevicesResource::getConfiguredDevices() const
{
    qCDebug(dcRest) << "Get all configured devices";
    HttpReply *reply = createSuccessReply();
    QVariantList devices = JsonTypes::packConfiguredDevices();

    QVariantList finalDevices;
    foreach (const QVariant &deviceVariant, devices) {
        QVariantMap deviceMap = deviceVariant.toMap();
        Device* device = GuhCore::instance()->findConfiguredDevice(DeviceId(deviceMap.value("id").toString()));
        QVariantList deviceStates = JsonTypes::packDeviceStates(device);
        deviceMap.insert("states", deviceStates);
        finalDevices.append(deviceMap);
    }

    reply->setPayload(QJsonDocument::fromVariant(finalDevices).toJson());
    return reply;
}

HttpReply *DevicesResource::getConfiguredDevice(Device *device) const
{
    qCDebug(dcRest) << "Get configured device with id:" << device->id().toString();
    HttpReply *reply = createSuccessReply();
    QVariantMap deviceMap = JsonTypes::packDevice(device);
    QVariantList deviceStates = JsonTypes::packDeviceStates(device);
    deviceMap.insert("states", deviceStates);

    qCDebug(dcRest) << deviceMap;

    reply->setPayload(QJsonDocument::fromVariant(deviceMap).toJson());
    return reply;
}

HttpReply *DevicesResource::getDeviceStateValues(Device *device) const
{
    qCDebug(dcRest) << "Get states of device with id:" << device->id().toString();
    HttpReply *reply = createSuccessReply();
    reply->setPayload(QJsonDocument::fromVariant(JsonTypes::packDeviceStates(device)).toJson());
    return reply;
}

HttpReply *DevicesResource::getDeviceStateValue(Device *device, const StateTypeId &stateTypeId) const
{
    qCDebug(dcRest) << "Get device state value of state with id:" << stateTypeId.toString();
    HttpReply *reply = createSuccessReply();
    QVariantMap stateValueMap;
    stateValueMap.insert("value", device->state(stateTypeId).value());
    reply->setPayload(QJsonDocument::fromVariant(stateValueMap).toJson());
    return reply;
}

HttpReply *DevicesResource::removeDevice(Device *device) const
{
    qCDebug(dcRest) << "Remove device with id:" << device->id().toString();
    DeviceManager::DeviceError result = GuhCore::instance()->removeConfiguredDevice(device->id(), QHash<RuleId, RuleEngine::RemovePolicy>());

    // TODO: /api/v1/devices/{deviceId}?ruleId={ruleId}&removePolicy={RemovePolicy}

    if (result == DeviceManager::DeviceErrorNoError) {
        HttpReply *reply = createSuccessReply();
        reply->setCloseConnection(true);
        return reply;
    }
    return createErrorReply(HttpReply::Forbidden);
}

HttpReply *DevicesResource::executeAction(Device *device, const ActionTypeId &actionTypeId, const QByteArray &payload) const
{
    QPair<bool, QVariant> verification = RestResource::verifyPayload(payload);
    if (!verification.first)
        return createErrorReply(HttpReply::BadRequest);

    QVariantMap message = verification.second.toMap();

    if (!message.contains("params"))
        return createErrorReply(HttpReply::BadRequest);

    ParamList actionParams = JsonTypes::unpackParams(message.value("params").toList());

    qCDebug(dcRest) << "Execute action with" << actionParams;

    Action action(actionTypeId, device->id());
    action.setParams(actionParams);

    DeviceManager::DeviceError status = GuhCore::instance()->executeAction(action);
    if (status == DeviceManager::DeviceErrorAsync) {
        HttpReply *reply = createAsyncReply();
        m_asyncActionExecutions.insert(action.id(), reply);
        return reply;
    }

    if (status != DeviceManager::DeviceErrorNoError)
        return createErrorReply(HttpReply::InternalServerError);

    return createSuccessReply();
}

HttpReply *DevicesResource::addConfiguredDevice(const QByteArray &payload) const
{
    QPair<bool, QVariant> verification = RestResource::verifyPayload(payload);
    if (!verification.first)
        return createErrorReply(HttpReply::BadRequest);

    QVariantMap params = verification.second.toMap();

    DeviceClassId deviceClassId(params.value("deviceClassId").toString());
    if (deviceClassId.isNull())
        return createErrorReply(HttpReply::BadRequest);

    DeviceId newDeviceId = DeviceId::createDeviceId();
    ParamList deviceParams = JsonTypes::unpackParams(params.value("deviceParams").toList());
    DeviceDescriptorId deviceDescriptorId(params.value("deviceDescriptorId").toString());

    DeviceManager::DeviceError status;
    if (deviceDescriptorId.isNull()) {
        qCDebug(dcRest) << "Adding device with " << deviceParams;
        status = GuhCore::instance()->addConfiguredDevice(deviceClassId, deviceParams, newDeviceId);
    } else {
        qCDebug(dcRest) << "Adding discovered device with DeviceDescriptorId" << deviceDescriptorId.toString();
        status = GuhCore::instance()->addConfiguredDevice(deviceClassId, deviceDescriptorId, newDeviceId);
    }
    if (status == DeviceManager::DeviceErrorAsync) {
        HttpReply *reply = createAsyncReply();
        qCDebug(dcRest) << "Device setup async reply";
        m_asyncDeviceAdditions.insert(newDeviceId, reply);
        return reply;
    }

    if (status != DeviceManager::DeviceErrorNoError)
        return createErrorReply(HttpReply::InternalServerError);

    QVariantMap result;
    result.insert("id", newDeviceId);
    HttpReply *reply = createSuccessReply();
    reply->setPayload(QJsonDocument::fromVariant(result).toJson());
    return reply;
}

HttpReply *DevicesResource::pairDevice(const QByteArray &payload) const
{
    QPair<bool, QVariant> verification = RestResource::verifyPayload(payload);
    if (!verification.first)
        return createErrorReply(HttpReply::BadRequest);

    QVariantMap params = verification.second.toMap();

    DeviceClassId deviceClassId(params.value("deviceClassId").toString());
    DeviceClass deviceClass = GuhCore::instance()->findDeviceClass(deviceClassId);

    if (deviceClassId.isNull()) {
        qCWarning(dcRest) << "Could not find deviceClassId" << params.value("deviceClassId").toString();
        return createErrorReply(HttpReply::BadRequest);
    }

    qCDebug(dcRest) << "Pair device with deviceClassId" << deviceClassId.toString();

    DeviceManager::DeviceError status;
    PairingTransactionId pairingTransactionId = PairingTransactionId::createPairingTransactionId();
    if (params.contains("deviceDescriptorId")) {
        DeviceDescriptorId deviceDescriptorId(params.value("deviceDescriptorId").toString());
        status = GuhCore::instance()->pairDevice(pairingTransactionId, deviceClassId, deviceDescriptorId);
    } else {
        ParamList deviceParams = JsonTypes::unpackParams(params.value("deviceParams").toList());
        status = GuhCore::instance()->pairDevice(pairingTransactionId, deviceClassId, deviceParams);
    }

    if (status != DeviceManager::DeviceErrorNoError)
        return createErrorReply(HttpReply::BadRequest);

    QVariantMap returns;
    returns.insert("displayMessage", deviceClass.pairingInfo());
    returns.insert("pairingTransactionId", pairingTransactionId.toString());
    returns.insert("setupMethod", JsonTypes::setupMethod().at(deviceClass.setupMethod()));
    HttpReply *reply = createSuccessReply();
    reply->setPayload(QJsonDocument::fromVariant(returns).toJson());
    return reply;
}

HttpReply *DevicesResource::confirmPairDevice(const QByteArray &payload) const
{
    QPair<bool, QVariant> verification = RestResource::verifyPayload(payload);
    if (!verification.first)
        return createErrorReply(HttpReply::BadRequest);

    QVariantMap params = verification.second.toMap();

    PairingTransactionId pairingTransactionId = PairingTransactionId(params.value("pairingTransactionId").toString());
    QString secret = params.value("secret").toString();
    DeviceManager::DeviceError status = GuhCore::instance()->confirmPairing(pairingTransactionId, secret);

    if (status == DeviceManager::DeviceErrorAsync) {
        HttpReply *reply = createAsyncReply();
        qCDebug(dcRest) << "Confirm pairing async reply";
        m_asyncPairingRequests.insert(pairingTransactionId, reply);
        return reply;
    }

    if (status != DeviceManager::DeviceErrorNoError)
        return createErrorReply(HttpReply::InternalServerError);

    return createSuccessReply();
}

HttpReply *DevicesResource::editDevice(Device *device, const QByteArray &payload) const
{
    qCDebug(dcRest) << "Edit device" << device->id();
    QPair<bool, QVariant> verification = RestResource::verifyPayload(payload);
    if (!verification.first)
        return createErrorReply(HttpReply::BadRequest);

    QVariantMap params = verification.second.toMap();
    ParamList deviceParams = JsonTypes::unpackParams(params.value("deviceParams").toList());

    DeviceManager::DeviceError status;
    DeviceDescriptorId deviceDescriptorId(params.value("deviceDescriptorId").toString());
    if (deviceDescriptorId.isNull()) {
        qCDebug(dcRest) << "Edit device with params:" << deviceParams;
        status = GuhCore::instance()->editDevice(device->id(), deviceParams);
    } else {
        qCDebug(dcRest) << "Edit device using the discovered device with descriptorId:" << deviceDescriptorId.toString();
        status = GuhCore::instance()->editDevice(device->id(), deviceDescriptorId);
    }

    if (status == DeviceManager::DeviceErrorAsync) {
        HttpReply *reply = createAsyncReply();
        qCDebug(dcRest) << "Device edit async reply";
        m_asyncEditDevice.insert(device, reply);
        return reply;
    }

    if (status != DeviceManager::DeviceErrorNoError)
        return createErrorReply(HttpReply::InternalServerError);

    return createSuccessReply();
}

void DevicesResource::actionExecuted(const ActionId &actionId, DeviceManager::DeviceError status)
{
    if (!m_asyncActionExecutions.contains(actionId))
        return; // Not the action we are waiting for.

    HttpReply *reply = m_asyncActionExecutions.take(actionId);
    if (status == DeviceManager::DeviceErrorNoError) {
        qCDebug(dcRest) << "Action execution finished successfully";
        reply->setHttpStatusCode(HttpReply::Ok);
    } else {
        qCDebug(dcRest) << "Action execution finished with error" << status;
        reply->setHttpStatusCode(HttpReply::InternalServerError);
    }

    reply->finished();
}

void DevicesResource::deviceSetupFinished(Device *device, DeviceManager::DeviceError status)
{
    if (!m_asyncDeviceAdditions.contains(device->id()))
        return; // Not the device we are waiting for.

    HttpReply *reply = m_asyncDeviceAdditions.take(device->id());
    if (status == DeviceManager::DeviceErrorNoError) {
        qCDebug(dcRest) << "Device setup finished successfully";
        reply->setHttpStatusCode(HttpReply::Ok);
    } else {
        qCDebug(dcRest) << "Device setup finished with error" << status;
        reply->setHttpStatusCode(HttpReply::InternalServerError);
    }

    QVariantMap result;
    result.insert("id", device->id());
    reply->setPayload(QJsonDocument::fromVariant(result).toJson());
    reply->finished();
}

void DevicesResource::deviceEditFinished(Device *device, DeviceManager::DeviceError status)
{
    if (!m_asyncEditDevice.contains(device))
        return; // Not the device we are waiting for.

    HttpReply *reply = m_asyncEditDevice.take(device);
    if (status == DeviceManager::DeviceErrorNoError) {
        qCDebug(dcRest) << "Device edit finished successfully";
        reply->setHttpStatusCode(HttpReply::Ok);
    } else {
        qCDebug(dcRest) << "Device edit finished with error" << status;
        reply->setHttpStatusCode(HttpReply::InternalServerError);
    }

    reply->finished();
}

void DevicesResource::pairingFinished(const PairingTransactionId &pairingTransactionId, DeviceManager::DeviceError status, const DeviceId &deviceId)
{
    if (!m_asyncPairingRequests.contains(pairingTransactionId))
        return; // Not the device pairing we are waiting for.

    HttpReply *reply = m_asyncPairingRequests.take(pairingTransactionId);
    if (status == DeviceManager::DeviceErrorNoError) {
        qCDebug(dcRest) << "Pairing device finished successfully";
        QVariantMap response;
        response.insert("id", deviceId.toString());
        reply->setPayload(QJsonDocument::fromVariant(response).toJson());
        reply->setHttpStatusCode(HttpReply::Ok);
    } else {
        qCDebug(dcRest) << "Pairing device finished with error" << status;
        reply->setHttpStatusCode(HttpReply::InternalServerError);
    }

    reply->finished();
}

}
