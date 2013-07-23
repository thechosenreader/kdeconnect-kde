/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "avahiannouncer.h"

#include "devicelinks/udpdevicelink.h"

#include <QHostInfo>

AvahiAnnouncer::AvahiAnnouncer()
{
    QString serviceType = "_kdeconnect._udp";
    quint16 port = 10601;

    //http://api.kde.org/4.x-api/kdelibs-apidocs/dnssd/html/index.html

    service = new DNSSD::PublicService(QHostInfo::localHostName(), serviceType, port);

    mUdpSocket = new QUdpSocket();
    mUdpSocket->bind(port);

    connect(mUdpSocket, SIGNAL(readyRead()), this, SLOT(newConnection()));

    qDebug() << "Listening to udp messages";

}

void AvahiAnnouncer::newConnection()
{

    qDebug() << "AvahiAnnouncer newConnection";
    
    while (mUdpSocket->hasPendingDatagrams()) {

        QByteArray datagram;
        datagram.resize(mUdpSocket->pendingDatagramSize());
        NetAddress client;
        mUdpSocket->readDatagram(datagram.data(), datagram.size(), &(client.ip), &(client.port));

        //log.write(datagram);
        qDebug() << "AvahiAnnouncer incomming udp datagram: " << datagram;

        NetworkPackage np;
        NetworkPackage::unserialize(datagram,&np);

        if (np.type() == "kdeconnect.identity") {

            const QString& id = np.get<QString>("deviceId");
            //const QString& name = np.get<QString>("deviceName");

            qDebug() << "AvahiAnnouncer creating link to device" << id << "(" << client.ip << "," << client.port << ")";

            DeviceLink* dl = new UdpDeviceLink(id, this, client.ip);
            connect(dl,SIGNAL(destroyed(QObject*)),this,SLOT(deviceLinkDestroyed(QObject*)));

            emit onNewDeviceLink(np, dl);

            NetworkPackage::createIdentityPackage(&np);
            dl->sendPackage(np);

            if (links.contains(id)) {
                //Delete old link if we already know it, probably it is down if this happens.
                qDebug() << "Destroying old link";
                delete links[id];
            }

            links[id] = dl;

        } else {
            qDebug() << "Not an identification package (wuh?)";
        }
    }

}

void AvahiAnnouncer::deviceLinkDestroyed(QObject* deviceLink)
{
    const QString& id = ((DeviceLink*)deviceLink)->deviceId();
    if (links.contains(id)) links.remove(id);
}

AvahiAnnouncer::~AvahiAnnouncer()
{
    delete service;
}

void AvahiAnnouncer::setDiscoverable(bool b)
{
    qDebug() << "Avahi announcing";
    if (b) service->publishAsync();
}

