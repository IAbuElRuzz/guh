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

#include "coappdu.h"
#include "coapoption.h"

#include <QMetaEnum>
#include <QTime>

CoapPdu::CoapPdu(QObject *parent) :
    QObject(parent),
    m_version(1),
    m_messageType(Confirmable),
    m_statusCode(Empty),
    m_messageId(0),
    m_contentType(TextPlain),
    m_payload(QByteArray()),
    m_error(NoError)
{
    qsrand(QDateTime::currentMSecsSinceEpoch());
}

CoapPdu::CoapPdu(const QByteArray &data, QObject *parent) :
    QObject(parent),
    m_version(1),
    m_messageType(Confirmable),
    m_statusCode(Empty),
    m_messageId(0),
    m_contentType(TextPlain),
    m_payload(QByteArray()),
    m_error(NoError)
{
    qsrand(QDateTime::currentMSecsSinceEpoch());
    unpack(data);
}

QString CoapPdu::getStatusCodeString(const CoapPdu::StatusCode &statusCode)
{
    QString statusCodeString;
    const QMetaObject &metaObject = CoapPdu::staticMetaObject;
    QMetaEnum statusCodeEnum = metaObject.enumerator(metaObject.indexOfEnumerator("StatusCode"));
    int classNumber = (statusCode & 0xE0) >> 5;
    int detailNumber = statusCode & 0x1F;
    statusCodeString.append(QString::number(classNumber) + ".");
    if (detailNumber < 10)
        statusCodeString.append("0");

    statusCodeString.append(QString::number(detailNumber) + " ");
    statusCodeString.append(statusCodeEnum.valueToKey(statusCode));
    return statusCodeString;
}

quint8 CoapPdu::version() const
{
    return m_version;
}

void CoapPdu::setVersion(const quint8 &version)
{
    m_version = version;
}

CoapPdu::MessageType CoapPdu::messageType() const
{
    return m_messageType;
}

void CoapPdu::setMessageType(const CoapPdu::MessageType &messageType)
{
    m_messageType = messageType;
}

CoapPdu::StatusCode CoapPdu::statusCode() const
{
    return m_statusCode;
}

void CoapPdu::setStatusCode(const CoapPdu::StatusCode &statusCode)
{
    m_statusCode = statusCode;
}

quint16 CoapPdu::messageId() const
{
    return m_messageId;
}

void CoapPdu::createMessageId()
{
    setMessageId((quint16)qrand() % 65536);
}

void CoapPdu::setMessageId(const quint16 &messageId)
{
    m_messageId = messageId;
}

CoapPdu::ContentType CoapPdu::contentType() const
{
    return m_contentType;
}

void CoapPdu::setContentType(const CoapPdu::ContentType &contentType)
{
    // TODO: add the contentFormat option

    m_contentType = contentType;
}

QByteArray CoapPdu::token() const
{
    return m_token;
}

void CoapPdu::createToken()
{
    m_token.clear();
    // make sure that the toke has a minimum size of 1
    quint8 length = (quint8)(qrand() % 7) + 1;
    for (int i = 0; i < length; i++) {
        m_token.append((char)qrand() % 256);
    }
}

void CoapPdu::setToken(const QByteArray &token)
{
    m_token = token;
}

QByteArray CoapPdu::payload() const
{
    return m_payload;
}

void CoapPdu::setPayload(const QByteArray &payload)
{
    m_payload = payload;
}

QList<CoapOption> CoapPdu::options() const
{
    return m_options;
}

void CoapPdu::addOption(const CoapOption::Option &option, const QByteArray &data)
{
    // set pdu data from the option
    switch (option) {
    case CoapOption::ContentFormat: {
        if (data.isEmpty()) {
            setContentType(TextPlain);
        } else {
            setContentType(static_cast<ContentType>(data.toHex().toInt(0, 16)));
        }
        break;
    }
    case CoapOption::Block1: {
        m_block = CoapPduBlock(data);
        break;
    }
    case CoapOption::Block2: {
        m_block = CoapPduBlock(data);
        break;
    }
    default:
        break;
    }

    // insert option (keep the list sorted to ensure a positiv option delta)
    int index = 0;
    for (int i = 0; i < m_options.length(); i ++) {
        index = i;
        if (m_options.at(i).option() <= option) {
            continue;
        } else {
            break;
        }
    }
    m_options.insert(index + 1, CoapOption(option, data));
}

CoapPduBlock CoapPdu::block() const
{
    return m_block;
}

bool CoapPdu::hasOption(const CoapOption::Option &option) const
{
    foreach (const CoapOption &o, m_options) {
        if (o.option() == option)
            return true;
    }
    return false;
}

void CoapPdu::clear()
{
    m_version = 1;
    m_messageType = Confirmable;
    m_statusCode = Empty;
    m_messageId = 0;
    m_contentType = TextPlain;
    m_token.clear();
    m_payload.clear();
    m_options.clear();
    m_error = NoError;
}

bool CoapPdu::isValid() const
{
    return (m_error == NoError);
}

QByteArray CoapPdu::pack() const
{
    QByteArray pduData;

    // header
    QByteArray header;
    header.resize(4);
    header.fill('0');
    quint8 *rawHeader = (quint8 *)header.data();
    rawHeader[0] = m_version << 6;
    rawHeader[0] |= (quint8)m_messageType << 4;
    rawHeader[0] |= (quint8)m_token.size();
    rawHeader[1] = (quint8)m_statusCode;
    rawHeader[2] = (quint8)(m_messageId >> 8);
    rawHeader[3] = (quint8)(m_messageId & 0xff);
    pduData = QByteArray::fromRawData((char *)rawHeader, 4);

    // token
    pduData.append(m_token);

    // options
    QByteArray optionsData;
    quint8 prevOption = 0;
    foreach (const CoapOption &option, m_options) {
        quint8 optionByte = 0;

        // encode option delta
        quint16 optionDelta = (quint8)option.option() - prevOption;
        prevOption = (quint8)option.option();

        quint8 extendedOptionDeltaByte = 0;
        quint16 bigExtendedOptionDeltaByte = 0;
        if (optionDelta < 13) {
            optionByte = optionDelta << 4;
        } else if (optionDelta < 270) {
            // extended 8 bit option delta
            optionByte = 13 << 4;
            extendedOptionDeltaByte = optionDelta - 13;
        } else {
            // extended 16 bit option delta
            optionByte = 14 << 4;
            bigExtendedOptionDeltaByte = ((optionDelta - 269) >> 8) & 0xff;
            bigExtendedOptionDeltaByte = (optionDelta - 269) & 0xff;
        }

        // encode option length
        int optionLength = option.data().length();
        quint8 extendedOptionLengthByte = 0;
        quint16 bigExtendedOptionLengthByte = 0;
        if (optionLength < 13) {
            optionByte |= optionLength;
        } else if (optionLength < 270) {
            // extended 8 bit option length
            optionByte |= 13;
            extendedOptionLengthByte = optionLength - 13;
        } else {
            // extended 16 bit option length
            optionByte |= 14;
            bigExtendedOptionLengthByte = ((optionLength - 269) >> 8) & 0xff;
            bigExtendedOptionLengthByte = (optionLength - 269) & 0xff;
        }

        // add obligatory option byte
        pduData.append((char)optionByte);

        // check extended option delta bytes
        if (extendedOptionDeltaByte != 0)
            pduData.append((char)extendedOptionDeltaByte);

        if (bigExtendedOptionDeltaByte != 0)
            pduData.append((char)bigExtendedOptionDeltaByte);

        // check extended option length bytes
        if (extendedOptionLengthByte != 0)
            pduData.append((char)extendedOptionLengthByte);

        if (bigExtendedOptionLengthByte != 0)
            pduData.append((char)extendedOptionLengthByte);

        // add the option data
        pduData.append(option.data());
    }
    pduData.append(optionsData);

    if (!m_payload.isEmpty()) {
        pduData.append((char)255);
        pduData.append(m_payload.data());
    }

    return pduData;
}

void CoapPdu::unpack(const QByteArray &data)
{
    // create a CoapPDU
    if (data.length() < 4) {
        m_error = InvalidPduSizeError;
        qWarning() << "pdu to small" << data.length();
    }

    quint8 *rawData = (quint8 *)data.data();
    setVersion((rawData[0] & 0xc0) >> 6);
    setMessageType(static_cast<MessageType>((rawData[0] & 0x30) >> 4));
    quint8 tokenLength = (rawData[0] & 0xf);

    if (tokenLength > 8) {
        m_error = InvalidTokenError;
        qWarning() << "PDU token to long";
    }

    setToken(QByteArray((const char *)rawData + 4, tokenLength));
    setStatusCode(static_cast<StatusCode>(rawData[1]));
    setMessageId((qint16)data.mid(2,2).toHex().toUInt(0,16));

    // parse options
    int index = 4 + tokenLength;
    quint8 optionByte = rawData[index];
    quint16 delta = 0;
    while (QByteArray::number(optionByte, 16) != "ff" && optionByte != 0) {
        quint16 optionNumber = ((optionByte & 0xf0) >> 4);

        // check option delta
        if (optionNumber < 13) {
            delta += optionNumber;
        } else if (optionNumber == 13) {
            // extended 8 bit option delta
            delta += (quint8)(rawData[index + 1] + 13);
            index += 1;
        } else if (optionNumber == 14) {
            // extended 16 bit option delta
            delta += ((rawData[index + 1] << 8) | rawData[index + 2]) + 269;
            index += 2;
        } else if (optionNumber == 15) {
            m_error = InvalidOptionDeltaError;
        }

        // check option length
        quint16 optionLength = (optionByte & 0xf);
        if (optionLength == 13) {
            // extended 8 bit option length
            optionLength = (quint8)(rawData[index + 1] - 13);
            index += 1;
        } else if (optionLength == 14) {
            // extended 16 bit option delta
            optionLength = ((rawData[index + 1] << 8) | rawData[index + 2]) - 269;
            index += 2;
        } else if (optionLength == 15) {
            m_error = InvalidOptionLengthError;
        }

        QByteArray optionData = QByteArray((const char *)rawData + index + 1, optionLength);
        addOption(static_cast<CoapOption::Option>(delta), optionData);

        index += optionLength + 1;
        optionByte = rawData[index];

        if (QByteArray::number(optionByte, 16) == "ff") {
            setPayload(data.right(data.length() - index - 1));
            break;
        }
    }
}

QDebug operator<<(QDebug debug, const CoapPdu &coapPdu)
{
    const QMetaObject &metaObject = CoapPdu::staticMetaObject;
    QMetaEnum messageTypeEnum = metaObject.enumerator(metaObject.indexOfEnumerator("MessageType"));
    debug.nospace() << "CoapPdu(" << messageTypeEnum.valueToKey(coapPdu.messageType()) << ")" << endl;
    debug.nospace() << "  Code: " << CoapPdu::getStatusCodeString(coapPdu.statusCode()) << endl;
    debug.nospace() << "  Ver: " << coapPdu.version() << endl;
    debug.nospace() << "  Token: " << coapPdu.token().length() << " " << "0x"+ coapPdu.token().toHex() << endl;
    debug.nospace() << "  Message ID: " << coapPdu.messageId() << endl;
    debug.nospace() << "  Payload size: " << coapPdu.payload().size() << endl;
    foreach (const CoapOption &option, coapPdu.options()) {
        debug.nospace() << "  " << option;
    }

    if (!coapPdu.payload().isEmpty())
        debug.nospace() << endl << coapPdu.payload() << endl;

    return debug.space();
}
