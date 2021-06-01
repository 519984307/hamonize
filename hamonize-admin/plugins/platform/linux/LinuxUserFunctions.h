/*
 * LinuxUserFunctions.h - declaration of LinuxUserFunctions class
 *
 * Copyright (c) 2017-2019 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include "PlatformUserFunctions.h"
#include "LinuxCoreFunctions.h"

#include <pwd.h>

// clazy:excludeall=copyable-polymorphic

class LinuxUserFunctions : public PlatformUserFunctions
{
public:
	QString fullName( const QString& username ) override;

	QStringList userGroups( bool queryDomainGroups ) override;
	QStringList groupsOfUser( const QString& username, bool queryDomainGroups ) override;

	QString currentUser() override;
	QStringList loggedOnUsers() override;

    QStringList guestUserInfo() override;
    QStringList guestUserInfo( const QString& username ) override;

	void logon( const QString& username, const QString& password ) override;
	void logoff() override;

	bool authenticate( const QString& username, const QString& password ) override;

    void changeDeskerLoginInfo(const QString& guestId, const QString& guestName) override;

	static uid_t userIdFromName( const QString& username );


private:
	enum {
		WhoProcessTimeout = 3000
	};

    typedef struct {
        QString guestId;
        QString guestName;
        QDBusObjectPath path;
    } GuestLoginDBusUserInfo;

    QString m_guestId;
    QString m_guestName;

    LinuxCoreFunctions::DBusInterfacePointer m_guestLoginManager;

};
