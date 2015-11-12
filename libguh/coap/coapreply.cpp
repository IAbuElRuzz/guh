/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2015 Simon Stuerz <simon.stuerz@guh.guru>                *
 *                                                                         *
 *  This file is part of QtCoap.                                           *
 *                                                                         *
 *  QtCoap is free software: you can redistribute it and/or modify         *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, version 3 of the License.                *
 *                                                                         *
 *  QtCoap is distributed in the hope that it will be useful,              *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the           *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with QtCoap. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "coapreply.h"
#include "coappdu.h"

#include <QMetaEnum>

CoapRequest CoapReply::request() const
{
    return m_request;
}

QByteArray CoapReply::payload() const
{
    return m_payload;
}

bool CoapReply::isFinished() const
{
    return m_isFinished;
}

bool CoapReply::isRunning() const
{
    return m_timer->isActive();
}

CoapReply::Error CoapReply::error() const
{
    return m_error;
}

QString CoapReply::errorString() const
{
    QString errorString;
    switch (m_error) {
    case NoError:
        break;
    case HostNotFoundError:
        errorString = "The remote host name was not found (invalid hostname).";
        break;
    case TimeoutError:
        errorString = "The server did not respond after 4 retransmissions.";
        break;
    case InvalidUrlSchemeError:
        errorString = "The given URL does not have a valid scheme.";
        break;
    case InvalidPduError:
        errorString = "The package data unit (PDU) could not be parsed successfully.";
        break;
    default:
        break;
    }
    return errorString;
}

CoapPdu::ContentType CoapReply::contentType() const
{
    return m_contentType;
}

CoapPdu::MessageType CoapReply::messageType() const
{
    return m_messageType;
}

CoapPdu::StatusCode CoapReply::statusCode() const
{
    return m_statusCode;
}

CoapReply::CoapReply(const CoapRequest &request, QObject *parent) :
    QObject(parent),
    m_request(request),
    m_error(NoError),
    m_isFinished(false),
    m_retransmissions(1),
    m_contentType(CoapPdu::TextPlain),
    m_messageType(CoapPdu::Acknowledgement),
    m_statusCode(CoapPdu::Empty),
    m_lockedUp(false)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(false);
    m_timer->setInterval(2000);

    connect(m_timer, &QTimer::timeout, this, &CoapReply::timeout);
}

QByteArray CoapReply::requestData() const
{
    return m_requestData;
}

int CoapReply::messageId() const
{
    return m_messageId;
}

void CoapReply::setMessageId(const int &messageId)
{
    m_messageId = messageId;
}

QByteArray CoapReply::messageToken() const
{
    return m_messageToken;
}

void CoapReply::setMessageToken(const QByteArray &messageToken)
{
    m_messageToken = messageToken;
}

void CoapReply::setFinished()
{
    m_isFinished = true;
    m_timer->stop();
    emit finished();
}

void CoapReply::setError(const CoapReply::Error &code)
{
    m_error = code;
    emit error(m_error);
}

void CoapReply::resend()
{
    m_retransmissions++;
    if (m_retransmissions > 5) {
        setError(CoapReply::TimeoutError);
        setFinished();
    }
}

void CoapReply::setContentType(const CoapPdu::ContentType contentType)
{
    m_contentType = contentType;
}

void CoapReply::setMessageType(const CoapPdu::MessageType &messageType)
{
    m_messageType = messageType;
}

void CoapReply::setStatusCode(const CoapPdu::StatusCode &statusCode)
{
    m_statusCode = statusCode;
}

void CoapReply::setHostAddress(const QHostAddress &address)
{
    m_hostAddress = address;
}

QHostAddress CoapReply::hostAddress() const
{
    return m_hostAddress;
}

void CoapReply::setPort(const int &port)
{
    m_port = port;
}

int CoapReply::port() const
{
    return m_port;
}

void CoapReply::setRequestPayload(const QByteArray &requestPayload)
{
    m_requestPayload = requestPayload;
}

QByteArray CoapReply::requestPayload() const
{
    return m_requestPayload;
}

void CoapReply::setRequestMethod(const CoapPdu::StatusCode &method)
{
    m_requestMethod = method;
}

CoapPdu::StatusCode CoapReply::requestMethod() const
{
    return m_requestMethod;
}

void CoapReply::appendPayloadData(const QByteArray &data)
{
    m_payload.append(data);
    m_timer->start();
    m_retransmissions = 1;
}

void CoapReply::setRequestData(const QByteArray &requestData)
{
    m_requestData = requestData;
}

QDebug operator<<(QDebug debug, CoapReply *reply)
{
    const QMetaObject &metaObject = CoapPdu::staticMetaObject;
    QMetaEnum messageTypeEnum = metaObject.enumerator(metaObject.indexOfEnumerator("MessageType"));
    QMetaEnum contentTypeEnum = metaObject.enumerator(metaObject.indexOfEnumerator("ContentType"));
    debug.nospace() << "CoapReply(" << messageTypeEnum.valueToKey(reply->messageType()) << ")" << endl;
    debug.nospace() << "  Status code: " << CoapPdu::getStatusCodeString(reply->statusCode()) << endl;
    debug.nospace() << "  Content type: " << contentTypeEnum.valueToKey(reply->contentType()) << endl;
    debug.nospace() << "  Payload size: " << reply->payload().size() << endl;

    if (!reply->payload().isEmpty())
        debug.nospace() << endl << reply->payload() << endl;

    return debug.space();
}
