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

#include "guhtestbase.h"
#include "guhcore.h"
#include "devicemanager.h"
#include "mocktcpserver.h"

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCoreApplication>
#include <QMetaType>

class TestJSONRPC: public GuhTestBase
{
    Q_OBJECT

private slots:
    void testHandshake();

    void testBasicCall_data();
    void testBasicCall();

    void introspect();

    void enableDisableNotifications_data();
    void enableDisableNotifications();

    void stateChangeEmitsNotifications();

private:
    QStringList extractRefs(const QVariant &variant);

};

QStringList TestJSONRPC::extractRefs(const QVariant &variant)
{
    if (variant.canConvert(QVariant::String)) {
        if (variant.toString().startsWith("$ref")) {
            return QStringList() << variant.toString();
        }
    }
    if (variant.canConvert(QVariant::List)) {
        QStringList refs;
        foreach (const QVariant tmp, variant.toList()) {
            refs << extractRefs(tmp);
        }
        return refs;
    }
    if (variant.canConvert(QVariant::Map)) {
        QStringList refs;
        foreach (const QVariant tmp, variant.toMap()) {
            refs << extractRefs(tmp);
        }
        return refs;
    }
    return QStringList();
}

void TestJSONRPC::testHandshake()
{
    QSignalSpy spy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));

    QUuid newClientId = QUuid::createUuid();
    m_mockTcpServer->clientConnected(newClientId);
    QVERIFY2(spy.count() > 0, "Did not get the handshake message upon connect.");
    QVERIFY2(spy.first().first() == newClientId, "Handshake message addressed at the wrong client.");

    QJsonDocument jsonDoc = QJsonDocument::fromJson(spy.first().at(1).toByteArray());
    QVariantMap handShake = jsonDoc.toVariant().toMap();
    QVERIFY2(handShake.value("version").toString() == GUH_VERSION_STRING, "Handshake version doesn't match Guh version.");

    m_mockTcpServer->clientDisconnected(newClientId);
}

void TestJSONRPC::testBasicCall_data()
{
    QTest::addColumn<QByteArray>("call");
    QTest::addColumn<bool>("idValid");
    QTest::addColumn<bool>("valid");

    QTest::newRow("valid call") << QByteArray("{\"id\":42, \"method\":\"JSONRPC.Introspect\"}") << true << true;
    QTest::newRow("missing id") << QByteArray("{\"method\":\"JSONRPC.Introspect\"}") << false << false;
    QTest::newRow("missing method") << QByteArray("{\"id\":42}") << true << false;
    QTest::newRow("borked") << QByteArray("{\"id\":42, \"method\":\"JSO") << false << false;
    QTest::newRow("invalid function") << QByteArray("{\"id\":42, \"method\":\"JSONRPC.Foobar\"}") << true << false;
    QTest::newRow("invalid namespace") << QByteArray("{\"id\":42, \"method\":\"FOO.Introspect\"}") << true << false;
    QTest::newRow("missing dot") << QByteArray("{\"id\":42, \"method\":\"JSONRPCIntrospect\"}") << true << false;
    QTest::newRow("invalid params") << QByteArray("{\"id\":42, \"method\":\"JSONRPC.Introspect\", \"params\":{\"törööö\":\"chooo-chooo\"}}") << true << false;
}

void TestJSONRPC::testBasicCall()
{
    QFETCH(QByteArray, call);
    QFETCH(bool, idValid);
    QFETCH(bool, valid);

    QSignalSpy spy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));
    QVERIFY(spy.isValid());

    m_mockTcpServer->injectData(m_clientId, call);

    if (spy.count() == 0) {
        spy.wait();
    }

    // Make sure we got exactly one response
    QVERIFY(spy.count() == 1);

    // Make sure the response goes to the correct clientId
    QCOMPARE(spy.first().first().toString(), m_clientId.toString());

    // Make sure the response it a valid JSON string
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(spy.first().last().toByteArray(), &error);
    QCOMPARE(error.error, QJsonParseError::NoError);

    // Make sure the response\"s id is the same as our command
    if (idValid) {
        QCOMPARE(jsonDoc.toVariant().toMap().value("id").toInt(), 42);
    }
    if (valid) {
        QVERIFY2(jsonDoc.toVariant().toMap().value("status").toString() == "success", "Call wasn't parsed correctly by guh.");
    }
}

void TestJSONRPC::introspect()
{
    QVariant response = injectAndWait("JSONRPC.Introspect");
    QVariantMap methods = response.toMap().value("params").toMap().value("methods").toMap();
    QVariantMap notifications = response.toMap().value("params").toMap().value("notifications").toMap();
    QVariantMap types = response.toMap().value("params").toMap().value("types").toMap();

    QVERIFY2(methods.count() > 0, "No methods in Introspect response!");
    QVERIFY2(notifications.count() > 0, "No notifications in Introspect response!");
    QVERIFY2(types.count() > 0, "No types in Introspect response!");

    // Make sure all $ref: pointers have their according type defined
    QVariantMap allItems = methods.unite(notifications).unite(types);
    foreach (const QVariant &item, allItems) {
        foreach (const QString &ref, extractRefs(item)) {
            QString typeId = ref;
            typeId.remove("$ref:");
            QVERIFY2(types.contains(typeId), QString("Undefined ref: %1").arg(ref).toLatin1().data());
        }
    }
}

void TestJSONRPC::enableDisableNotifications_data()
{
    QTest::addColumn<QString>("enabled");

    QTest::newRow("enabled") << "true";
    QTest::newRow("disabled") << "false";
}

void TestJSONRPC::enableDisableNotifications()
{
    QFETCH(QString, enabled);

    QVariantMap params;
    params.insert("enabled", enabled);
    QVariant response = injectAndWait("JSONRPC.SetNotificationStatus", params);

    QCOMPARE(response.toMap().value("params").toMap().value("enabled").toString(), enabled);
}

void TestJSONRPC::stateChangeEmitsNotifications()
{
    QVariantMap params;
    params.insert("enabled", true);
    QVariant response = injectAndWait("JSONRPC.SetNotificationStatus", params);
    QCOMPARE(response.toMap().value("params").toMap().value("enabled").toBool(), true);

    // Setup connection to mock client
    QNetworkAccessManager nam;

    QSignalSpy clientSpy(m_mockTcpServer, SIGNAL(outgoingData(QUuid,QByteArray)));


    // trigger state change in mock device
    int newVal = 1111;
    QUuid stateTypeId("80baec19-54de-4948-ac46-31eabfaceb83");
    QNetworkRequest request(QUrl(QString("http://localhost:%1/setstate?%2=%3").arg(m_mockDevice1Port).arg(stateTypeId.toString()).arg(newVal)));
    QNetworkReply *reply = nam.get(request);
    reply->deleteLater();

    // Lets wait for the notification
    clientSpy.wait();
    QCOMPARE(clientSpy.count(), 2);

    // Make sure the notification contains all the stuff we expect
    QJsonDocument jsonDoc = QJsonDocument::fromJson(clientSpy.at(0).at(1).toByteArray());
    QCOMPARE(jsonDoc.toVariant().toMap().value("notification").toString(), QString("Devices.StateChanged"));
    QCOMPARE(jsonDoc.toVariant().toMap().value("params").toMap().value("stateTypeId").toUuid(), stateTypeId);
    QCOMPARE(jsonDoc.toVariant().toMap().value("params").toMap().value("value").toInt(), newVal);

    // Make sure the notification contains all the stuff we expect
    jsonDoc = QJsonDocument::fromJson(clientSpy.at(1).at(1).toByteArray());
    QCOMPARE(jsonDoc.toVariant().toMap().value("notification").toString(), QString("Events.EventTriggered"));
    QCOMPARE(jsonDoc.toVariant().toMap().value("params").toMap().value("event").toMap().value("eventTypeId").toUuid(), stateTypeId);
    QCOMPARE(jsonDoc.toVariant().toMap().value("params").toMap().value("event").toMap().value("params").toList().first().toMap().value("value").toInt(), newVal);

    // Now turn off notifications
    params.clear();
    params.insert("enabled", false);
    response = injectAndWait("JSONRPC.SetNotificationStatus", params);
    QCOMPARE(response.toMap().value("params").toMap().value("enabled").toBool(), false);

    // Fire the a statechange once again
    clientSpy.clear();
    newVal = 42;
    request.setUrl(QUrl(QString("http://localhost:%1/setstate?%2=%3").arg(m_mockDevice1Port).arg(stateTypeId.toString()).arg(newVal)));
    reply = nam.get(request);
    reply->deleteLater();

    // Lets wait a max of 100ms for the notification
    clientSpy.wait(100);
    // but make sure it doesn't come
    QCOMPARE(clientSpy.count(), 0);

    // Now check that the state has indeed changed even though we didn't get a notification
    params.clear();
    params.insert("deviceId", m_mockDeviceId);
    params.insert("stateTypeId", stateTypeId);
    response = injectAndWait("Devices.GetStateValue", params);

    QCOMPARE(response.toMap().value("params").toMap().value("value").toInt(), newVal);

}

#include "testjsonrpc.moc"

QTEST_MAIN(TestJSONRPC)
