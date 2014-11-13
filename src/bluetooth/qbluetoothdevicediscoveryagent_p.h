/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtBluetooth module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QBLUETOOTHDEVICEDISCOVERYAGENT_P_H
#define QBLUETOOTHDEVICEDISCOVERYAGENT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qbluetoothdevicediscoveryagent.h"
#ifdef QT_ANDROID_BLUETOOTH
#include <QtAndroidExtras/QAndroidJniObject>
#include "android/devicediscoverybroadcastreceiver_p.h"
#include <QtCore/QTimer>
#endif

#include <QtCore/QVariantMap>

#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothLocalDevice>

#ifdef QT_BLUEZ_BLUETOOTH
#include "bluez/bluez5_helper_p.h"

class OrgBluezManagerInterface;
class OrgBluezAdapterInterface;
class OrgFreedesktopDBusObjectManagerInterface;
class OrgFreedesktopDBusPropertiesInterface;
class OrgBluezAdapter1Interface;
class OrgBluezDevice1Interface;

QT_BEGIN_NAMESPACE
class QDBusVariant;
QT_END_NAMESPACE
#elif defined(QT_QNX_BLUETOOTH)
#include "qnx/ppshelpers_p.h"
#include <QTimer>
#endif

#ifdef Q_OS_WIN32
#include <QtConcurrent>
#include "qbluetoothlocaldevice_p.h"
#include <bluetoothapis.h>
#endif

QT_BEGIN_NAMESPACE

class QBluetoothDeviceDiscoveryAgentPrivate
#if defined(QT_QNX_BLUETOOTH) || defined(QT_ANDROID_BLUETOOTH)
    : public QObject
{
    Q_OBJECT
#elif defined(Q_OS_WIN32)
    : public QBluetoothLocalDevicePrivateData
{
#else
{
#endif
    Q_DECLARE_PUBLIC(QBluetoothDeviceDiscoveryAgent)
public:
    QBluetoothDeviceDiscoveryAgentPrivate(
            const QBluetoothAddress &deviceAdapter,
            QBluetoothDeviceDiscoveryAgent *parent);
    ~QBluetoothDeviceDiscoveryAgentPrivate();

    void start();
    void stop();
    bool isActive() const;

#ifdef QT_BLUEZ_BLUETOOTH
    void _q_deviceFound(const QString &address, const QVariantMap &dict);
    void _q_propertyChanged(const QString &name, const QDBusVariant &value);
    void _q_InterfacesAdded(const QDBusObjectPath &object_path,
                            InterfaceList interfaces_and_properties);
    void _q_discoveryFinished();
    void _q_discoveryInterrupted(const QString &path);
    void _q_PropertiesChanged(const QString &interface,
                              const QVariantMap &changed_properties,
                              const QStringList &invalidated_properties);
    void _q_extendedDeviceDiscoveryTimeout();
#endif

#ifdef Q_OS_WIN32
    void _q_handleFindResult();
#endif

private:
    QList<QBluetoothDeviceInfo> discoveredDevices;
    QBluetoothDeviceDiscoveryAgent::InquiryType inquiryType;

    QBluetoothDeviceDiscoveryAgent::Error lastError;
    QString errorString;

#ifdef QT_ANDROID_BLUETOOTH
private slots:
    void processSdpDiscoveryFinished();
    void processDiscoveredDevices(const QBluetoothDeviceInfo &info, bool isLeResult);
    friend void QtBluetoothLE_leScanResult(JNIEnv *, jobject, jlong, jobject);
    void stopLowEnergyScan();

private:
    void startLowEnergyScan();

    DeviceDiscoveryBroadcastReceiver *receiver;
    QBluetoothAddress m_adapterAddress;
    short m_active;
    QAndroidJniObject adapter;
    QAndroidJniObject leScanner;
    QTimer *leScanTimeout;

    bool pendingCancel, pendingStart;
#elif defined(QT_BLUEZ_BLUETOOTH)
    QBluetoothAddress m_adapterAddress;
    bool pendingCancel;
    bool pendingStart;
    OrgBluezManagerInterface *manager;
    OrgBluezAdapterInterface *adapter;
    OrgFreedesktopDBusObjectManagerInterface *managerBluez5;
    OrgBluezAdapter1Interface *adapterBluez5;
    QTimer *discoveryTimer;
    QList<OrgFreedesktopDBusPropertiesInterface *> propertyMonitors;

    void deviceFoundBluez5(const QString& devicePath);
    void startBluez5();

    bool useExtendedDiscovery;
    QTimer extendedDiscoveryTimer;

#elif defined(QT_QNX_BLUETOOTH)
private slots:
    void finished();
    void remoteDevicesChanged(int);
    void controlReply(ppsResult result);
    void controlEvent(ppsResult result);
    void startDeviceSearch();

private:
    QSocketNotifier *m_rdNotifier;
    QTimer m_finishedTimer;

    int m_rdfd;
    bool m_active;
    enum Ops {
        None,
        Cancel,
        Start
    };
    Ops m_nextOp;
    Ops m_currentOp;
    void processNextOp();
    bool isFinished;
#endif

#ifdef Q_OS_WIN32
    void processDiscoveredDevices(const BLUETOOTH_DEVICE_INFO &info);
    void handleErrors(DWORD errorCode);
    bool isRunning() const;

    static QVariant findFirstDevice(HANDLE radioHandle);
    static QVariant findNextDevice(HBLUETOOTH_DEVICE_FIND findHandle);
    static void findClose(HBLUETOOTH_DEVICE_FIND findHandle);

    QFutureWatcher<QVariant> *findWatcher;
    bool pendingCancel;
    bool pendingStart;
#endif

    QBluetoothDeviceDiscoveryAgent *q_ptr;
};

QT_END_NAMESPACE

#endif
