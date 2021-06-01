/*
 * ComputerControlInterface.cpp - interface class for controlling a computer
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

#include "BuiltinFeatures.h"
#include "ComputerControlInterface.h"
#include "Computer.h"
#include "FeatureControl.h"
#include "MonitoringMode.h"
#include "UserSessionControl.h"
#include "VeyonConfiguration.h"
#include "VeyonConnection.h"
#include "VncConnection.h"


ComputerControlInterface::ComputerControlInterface( const Computer& computer,
													QObject* parent ) :
	QObject( parent ),
	m_computer( computer ),
	m_state( State::Disconnected ),
	m_userLoginName(),
	m_userFullName(),
    m_guestLoginName(),
    m_guestFullName(),
	m_scaledScreenSize(),
	m_vncConnection( nullptr ),
	m_connection( nullptr ),
	m_connectionWatchdogTimer( this ),
	m_userUpdateTimer( this ),
	m_activeFeaturesUpdateTimer( this )
{
	m_connectionWatchdogTimer.setInterval( ConnectionWatchdogTimeout );
	m_connectionWatchdogTimer.setSingleShot( true );
	connect( &m_connectionWatchdogTimer, &QTimer::timeout, this, &ComputerControlInterface::restartConnection );

	connect( &m_userUpdateTimer, &QTimer::timeout, this, &ComputerControlInterface::updateUser );
	connect( &m_activeFeaturesUpdateTimer, &QTimer::timeout, this, &ComputerControlInterface::updateActiveFeatures );
}



ComputerControlInterface::~ComputerControlInterface()
{
	stop();
}



void ComputerControlInterface::start( QSize scaledScreenSize )
{
	// make sure we do not leak
	stop();

	m_scaledScreenSize = scaledScreenSize;

	if( m_computer.hostAddress().isEmpty() == false )
	{
		// hihoon
		vWarning() << "### [hihoon] hostAddress ### ComputerControlInterface::start(): " << m_computer.hostAddress();

		m_vncConnection = new VncConnection();
		m_vncConnection->setHost( m_computer.hostAddress() );
		m_vncConnection->setQuality( VncConnection::Quality::Thumbnail );
		m_vncConnection->setScaledSize( m_scaledScreenSize );

		enableUpdates();

		m_connection = new VeyonConnection( m_vncConnection );

		m_vncConnection->start();

		connect( m_vncConnection, &VncConnection::framebufferUpdateComplete, this, &ComputerControlInterface::resetWatchdog );
		connect( m_vncConnection, &VncConnection::framebufferUpdateComplete, this, &ComputerControlInterface::screenUpdated );

		connect( m_vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateState );
		connect( m_vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateUser );
		connect( m_vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::updateActiveFeatures );
		connect( m_vncConnection, &VncConnection::stateChanged, this, &ComputerControlInterface::stateChanged );

		connect( m_connection, &VeyonConnection::featureMessageReceived, this, &ComputerControlInterface::handleFeatureMessage );
		connect( m_connection, &VeyonConnection::featureMessageReceived, this, &ComputerControlInterface::resetWatchdog );
	}
	else
	{
		vWarning() << "computer host address is empty!";
		vWarning() << "### [hihoon] hostAddress ### ComputerControlInterface::start(): " << m_computer.hostAddress();
	}
}



void ComputerControlInterface::stop()
{
	if( m_connection )
	{
		delete m_connection;
		m_connection = nullptr;
	}

	if( m_vncConnection )
	{
		// do not delete VNC connection but let it delete itself after stopping automatically
		m_vncConnection->stopAndDeleteLater();
		m_vncConnection = nullptr;
	}

	m_activeFeaturesUpdateTimer.stop();
	m_userUpdateTimer.stop();
	m_connectionWatchdogTimer.stop();

	m_state = State::Disconnected;
}



void ComputerControlInterface::setScaledScreenSize( QSize scaledScreenSize )
{
	m_scaledScreenSize = scaledScreenSize;

	if( m_vncConnection )
	{
		m_vncConnection->setScaledSize( m_scaledScreenSize );
	}

	emit screenUpdated();
}



QImage ComputerControlInterface::scaledScreen() const
{
	if( m_vncConnection && m_vncConnection->isConnected() )
	{
		return m_vncConnection->scaledScreen();
	}

	return QImage();
}



QImage ComputerControlInterface::screen() const
{
	if( m_vncConnection && m_vncConnection->isConnected() )
	{
		return m_vncConnection->image();
	}

	return QImage();
}



void ComputerControlInterface::setUserLoginName( const QString& userLoginName )
{
	if( userLoginName != m_userLoginName )
	{
		m_userLoginName = userLoginName;

		emit userChanged();
	}
}



void ComputerControlInterface::setUserFullName( const QString& userFullName )
{
	if( userFullName != m_userFullName )
	{
		m_userFullName = userFullName;

		emit userChanged();
	}
}


void ComputerControlInterface::setGuestLoginName( const QString& guestLoginName )
{
    if( guestLoginName != m_guestLoginName )
    {
        m_guestLoginName = guestLoginName;

        emit userChanged();
    }
}



void ComputerControlInterface::setGuestFullName( const QString& guestFullName )
{
    if( guestFullName != m_guestFullName )
    {
        m_guestFullName = guestFullName;

        emit userChanged();
    }
}



void ComputerControlInterface::setActiveFeatures( const FeatureUidList& activeFeatures )
{
	if( activeFeatures != m_activeFeatures )
	{
		m_activeFeatures = activeFeatures;

		emit activeFeaturesChanged();
	}
}



void ComputerControlInterface::setDesignatedModeFeature( Feature::Uid designatedModeFeature )
{
	m_designatedModeFeature = designatedModeFeature;

	updateActiveFeatures();
}



void ComputerControlInterface::sendFeatureMessage( const FeatureMessage& featureMessage, bool wake )
{
	if( m_connection && m_connection->isConnected() )
	{
		m_connection->sendFeatureMessage( featureMessage, wake );
	}
}



bool ComputerControlInterface::isMessageQueueEmpty()
{
	if( m_vncConnection && m_vncConnection->isConnected() )
	{
		return m_vncConnection->isEventQueueEmpty();
	}

	return true;
}



void ComputerControlInterface::enableUpdates()
{
	const auto updateInterval = VeyonCore::config().computerMonitoringUpdateInterval();

	if( m_vncConnection )
	{
		m_vncConnection->setFramebufferUpdateInterval( updateInterval );
	}

	m_userUpdateTimer.start( updateInterval );
	m_activeFeaturesUpdateTimer.start( updateInterval );
}



void ComputerControlInterface::disableUpdates()
{
	if( m_vncConnection )
	{
		m_vncConnection->setFramebufferUpdateInterval( UpdateIntervalWhenDisabled );
	}

	m_userUpdateTimer.stop();
	m_activeFeaturesUpdateTimer.start( UpdateIntervalWhenDisabled );
}



ComputerControlInterface::Pointer ComputerControlInterface::weakPointer()
{
	return Pointer( this, []( ComputerControlInterface* ) { } );
}



void ComputerControlInterface::resetWatchdog()
{
	if( state() == State::Connected )
	{
		m_connectionWatchdogTimer.start();
	}
}



void ComputerControlInterface::restartConnection()
{
	if( m_vncConnection )
	{
		vDebug();
		m_vncConnection->restart();

		m_connectionWatchdogTimer.stop();
	}
}



void ComputerControlInterface::updateState()
{
	if( m_vncConnection )
	{
		switch( m_vncConnection->state() )
		{
		case VncConnection::State::Disconnected: m_state = State::Disconnected; break;
		case VncConnection::State::Connecting: m_state = State::Connecting; break;
		case VncConnection::State::Connected: m_state = State::Connected; break;
		case VncConnection::State::HostOffline: m_state = State::HostOffline; break;
		case VncConnection::State::ServiceUnreachable: m_state = State::ServiceUnreachable; break;
		case VncConnection::State::AuthenticationFailed: m_state = State::AuthenticationFailed; break;
		default: m_state = VncConnection::State::Disconnected; break;
		}
	}
	else
	{
		m_state = State::Disconnected;
	}
}



void ComputerControlInterface::updateUser()
{
	if( m_vncConnection && m_connection && state() == State::Connected )
	{
		if( userLoginName().isEmpty() )
		{
			VeyonCore::builtinFeatures().userSessionControl().getUserSessionInfo( { weakPointer() } );
		}
	}
	else
	{
		setUserLoginName( {} );
		setUserFullName( {} );
        setGuestLoginName( {} );
        setGuestFullName( {} );
    }
}



void ComputerControlInterface::updateActiveFeatures()
{
	if( m_vncConnection && m_connection && state() == State::Connected )
	{
		VeyonCore::builtinFeatures().featureControl().queryActiveFeatures( { weakPointer() } );
	}
	else
	{
		setActiveFeatures( {} );
	}
}



void ComputerControlInterface::handleFeatureMessage( const FeatureMessage& message )
{
	emit featureMessageReceived( message, weakPointer() );
}
