/*
 * WindowsUserFunctions.cpp - implementation of WindowsUserFunctions class
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

#include <winsock2.h>
#include <windows.h>
#include <lm.h>

#include "VeyonConfiguration.h"
#include "WindowsCoreFunctions.h"
#include "WindowsPlatformConfiguration.h"
#include "WindowsUserFunctions.h"
#include "WtsSessionManager.h"

#include "authSSP.h"


QString WindowsUserFunctions::fullName( const QString& username )
{
	QString fullName;

	QString realUsername = username;
	PBYTE domainController = nullptr;

	const auto nameParts = username.split( QLatin1Char('\\') );
	if( nameParts.size() > 1 )
	{
		realUsername = nameParts[1];
		if( NetGetDCName( nullptr, WindowsCoreFunctions::toConstWCharArray( nameParts[0] ), &domainController ) != NERR_Success )
		{
			domainController = nullptr;
		}
	}

	LPUSER_INFO_2 buf = nullptr;
	NET_API_STATUS nStatus = NetUserGetInfo( reinterpret_cast<LPCWSTR>( domainController ),
											 WindowsCoreFunctions::toConstWCharArray( realUsername ),
											 2, reinterpret_cast<LPBYTE *>( &buf ) );
	if( nStatus == NERR_Success && buf != nullptr )
	{
		fullName = QString::fromWCharArray( buf->usri2_full_name );
	}

	if( buf != nullptr )
	{
		NetApiBufferFree( buf );
	}

	if( domainController != nullptr )
	{
		NetApiBufferFree( domainController );
	}

	return fullName;
}


/* Desker 로그인은 기본 데스크탑 로그인 메니저와 연동하여 로그인/로그아웃시 hamonize-server를 재기동 하는 경우와 달리
 * 해결하기 위해 LinuxUserFunction.cpp 내에서 처리하기로 한다.
 */
QStringList WindowsUserFunctions::guestUserInfo()
{
    // TODO 윈도우 전용으로 개발 필요
    QStringList guestUserInfoList = {QString(), QString()};

//    bool success = true;
/*
    m_guestLoginManager = WindowsCoreFunctions::guestLoginManager();

    윈도우에서 DeskerLoginManager는 QTcpSock 방식으로 바꾸는게 좋겠다
    const auto deskerservice = m_guestLoginManager->service();
    const auto deskerpath = m_guestLoginManager->path();
    const auto deskerinterface = m_guestLoginManager->interface();
*/

//    success &= QDBusConnection::systemBus().connect( deskerservice, deskerpath, deskerinterface, QStringLiteral("newLogon"),
//                                                     this, SLOT(changeDeskerLoginInfo(QString,QString)) );

    return guestUserInfoList;

}


QStringList WindowsUserFunctions::guestUserInfo( const QString& username )
{
    QStringList guestUserInfoList = {QString(), QString()};

    if(username.contains(QStringLiteral("guest-"))) {

        QFile userinfofile( QStringLiteral("/tmp/%1/.huserinfo").arg(username) );

        if( userinfofile.exists() && userinfofile.open(QIODevice::ReadOnly | QIODevice::Text) ) {

            QTextStream in(&userinfofile);
            QString line = in.readLine();

            QStringList resList = line.split( QLatin1Char(':') );

            if( resList.length() >= 2)
            {
                return resList;
            }

        }

    }

    return guestUserInfoList;

}

void WindowsUserFunctions::changeDeskerLoginInfo(const QString& guestId, const QString& guestName)
{
    vDebug() << __PRETTY_FUNCTION__ << "### hihoon ### m_guestId, guestName" << m_guestId << m_guestName;

    m_guestId = guestId;
    m_guestName = guestName;

    vDebug() << __PRETTY_FUNCTION__ << "### hihoon ### m_guestId, guestName" << m_guestId << m_guestName;
}


QStringList WindowsUserFunctions::userGroups( bool queryDomainGroups )
{
	auto groupList = localUserGroups();

	if( queryDomainGroups )
	{
		groupList.append( domainUserGroups() );
	}

	groupList.removeDuplicates();
	groupList.removeAll( QString() );

	return groupList;
}



QStringList WindowsUserFunctions::groupsOfUser( const QString& username, bool queryDomainGroups )
{
	auto groupList = localGroupsOfUser( username );

	if( queryDomainGroups )
	{
		groupList.append( domainGroupsOfUser( username ) );
	}

	groupList.removeDuplicates();
	groupList.removeAll( QString() );

	return groupList;
}



QString WindowsUserFunctions::currentUser()
{
	auto sessionId = WtsSessionManager::activeConsoleSession();

	auto username = WtsSessionManager::querySessionInformation( sessionId, WtsSessionManager::SessionInfoUserName );
	auto domainName = WtsSessionManager::querySessionInformation( sessionId, WtsSessionManager::SessionInfoDomainName );

	// check whether we just got the name of the local computer
	if( !domainName.isEmpty() )
	{
		wchar_t computerName[MAX_PATH]; // Flawfinder: ignore
		DWORD size = MAX_PATH;
		GetComputerName( computerName, &size );

		if( domainName == QString::fromWCharArray( computerName ) )
		{
			// reset domain name as we do not need to store local computer name
			domainName = QString();
		}
	}

	if( domainName.isEmpty() )
	{
		return username;
	}

	return domainName + QLatin1Char('\\') + username;
}



QStringList WindowsUserFunctions::loggedOnUsers()
{
	return WtsSessionManager::loggedOnUsers();
}



void WindowsUserFunctions::logon( const QString& username, const QString& password )
{
	Q_UNUSED(username);
	Q_UNUSED(password);

	// TODO
}



void WindowsUserFunctions::logoff()
{
	ExitWindowsEx( EWX_LOGOFF | (EWX_FORCE | EWX_FORCEIFHUNG), SHTDN_REASON_MAJOR_OTHER );
}



bool WindowsUserFunctions::authenticate( const QString& username, const QString& password )
{
	QString domain;
	QString user;

	const auto userNameParts = username.split( QLatin1Char('\\') );
	if( userNameParts.count() == 2 )
	{
		domain = userNameParts[0];
		user = userNameParts[1];
	}
	else
	{
		user = username;
	}

	auto domainWide = WindowsCoreFunctions::toWCharArray( domain );
	auto userWide = WindowsCoreFunctions::toWCharArray( user );
	auto passwordWide = WindowsCoreFunctions::toWCharArray( password );

	bool result = false;

	WindowsPlatformConfiguration config( &VeyonCore::config() );

	if( config.disableSSPIBasedUserAuthentication() )
	{
		HANDLE token = nullptr;
		result = LogonUserW( userWide, domainWide, passwordWide,
							 LOGON32_LOGON_NETWORK, LOGON32_PROVIDER_DEFAULT, &token );
		vDebug() << "LogonUserW()" << result << GetLastError();
		if( token )
		{
			CloseHandle( token );
		}
	}
	else
	{
		result = SSPLogonUser( domainWide, userWide, passwordWide );
		vDebug() << "SSPLogonUser()" << result << GetLastError();
	}

	delete[] domainWide;
	delete[] userWide;
	delete[] passwordWide;

	return result;
}



QString WindowsUserFunctions::domainController()
{
	QString dcName;
	LPBYTE outBuffer = nullptr;

	if( NetGetDCName( nullptr, nullptr, &outBuffer ) == NERR_Success )
	{
		dcName = QString::fromUtf16( reinterpret_cast<const ushort *>( outBuffer ) );

		NetApiBufferFree( outBuffer );
	}
	else
	{
		vWarning() << "could not query domain controller name!";
	}

	return dcName;
}



QStringList WindowsUserFunctions::domainUserGroups()
{
	const auto dc = domainController();

	QStringList groupList;

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	if( NetGroupEnum( WindowsCoreFunctions::toConstWCharArray( dc ), 0, &outBuffer, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, nullptr ) == NERR_Success )
	{
		const auto* groupInfos = reinterpret_cast<GROUP_INFO_0 *>( outBuffer );

		groupList.reserve( static_cast<int>( entriesRead ) );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16( reinterpret_cast<const ushort *>( groupInfos[i].grpi0_name ) );
		}

		if( entriesRead < totalEntries )
		{
			vWarning() << "not all domain groups fetched";
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		vWarning() << "could not fetch domain groups";
	}

	return groupList;
}



QStringList WindowsUserFunctions::domainGroupsOfUser( const QString& username )
{
	const auto dc = domainController();

	QStringList groupList;

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	const auto usernameWithoutDomain = VeyonCore::stripDomain( username );

	if( NetUserGetGroups( WindowsCoreFunctions::toConstWCharArray( dc ),
						  WindowsCoreFunctions::toConstWCharArray( usernameWithoutDomain ),
						  0, &outBuffer, MAX_PREFERRED_LENGTH,
						  &entriesRead, &totalEntries ) == NERR_Success )
	{
		const auto* groupUsersInfo = reinterpret_cast<GROUP_USERS_INFO_0 *>( outBuffer );

		groupList.reserve( static_cast<int>( entriesRead ) );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16( reinterpret_cast<const ushort *>( groupUsersInfo[i].grui0_name ) );
		}

		if( entriesRead < totalEntries )
		{
			vWarning() << "not all domain groups fetched for user" << username;
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		vWarning() << "could not fetch domain groups for user" << username;
	}

	return groupList;
}



QStringList WindowsUserFunctions::localUserGroups()
{
	QStringList groupList;

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	if( NetLocalGroupEnum( nullptr, 0, &outBuffer, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, nullptr ) == NERR_Success )
	{
		const auto* groupInfos = reinterpret_cast<LOCALGROUP_INFO_0 *>( outBuffer );

		groupList.reserve( static_cast<int>( entriesRead ) );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16( reinterpret_cast<const ushort *>( groupInfos[i].lgrpi0_name ) );
		}

		if( entriesRead < totalEntries )
		{
			vWarning() << "not all local groups fetched";
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		vWarning() << "could not fetch local groups";
	}

	return groupList;
}



QStringList WindowsUserFunctions::localGroupsOfUser( const QString& username )
{
	QStringList groupList;

	LPBYTE outBuffer = nullptr;
	DWORD entriesRead = 0;
	DWORD totalEntries = 0;

	if( NetUserGetLocalGroups( nullptr, WindowsCoreFunctions::toConstWCharArray( username ),
							   0, 0, &outBuffer, MAX_PREFERRED_LENGTH,
							   &entriesRead, &totalEntries ) == NERR_Success )
	{
		const auto* localGroupUsersInfo = reinterpret_cast<LOCALGROUP_USERS_INFO_0 *>( outBuffer );

		groupList.reserve( static_cast<int>( entriesRead ) );

		for( DWORD i = 0; i < entriesRead; ++i )
		{
			groupList += QString::fromUtf16( reinterpret_cast<const ushort *>( localGroupUsersInfo[i].lgrui0_name ) );
		}

		if( entriesRead < totalEntries )
		{
			vWarning() << "not all local groups fetched for user" << username;
		}

		NetApiBufferFree( outBuffer );
	}
	else
	{
		vWarning() << "could not fetch local groups for user" << username;
	}

	return groupList;
}
