/*
 *  RemoteAccessWidget.h - widget containing a VNC view and controls for it
 *
 *  Copyright (c) 2006-2019 Tobias Junghans <tobydox@veyon.io>
 *
 *  This file is part of Veyon - https://veyon.io
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#pragma once

#include "ComputerControlInterface.h"
#include "VncView.h"

#include <QTimeLine>
#include <QWidget>

class VncView;
class VeyonConnection;
class RemoteAccessWidget;
class ToolButton;

// clazy:excludeall=ctor-missing-parent-argument

class RemoteAccessWidgetToolBar : public QWidget
{
	Q_OBJECT
public:
	RemoteAccessWidgetToolBar( RemoteAccessWidget* parent, bool viewOnly );

	~RemoteAccessWidgetToolBar() override = default;

	void appear();
	void disappear();
	void updateControls( bool viewOnly );


protected:
	void leaveEvent( QEvent* event ) override;
	void paintEvent( QPaintEvent* event ) override;

private:
	void updateConnectionAnimation();
	void updatePosition();
	void startConnection();
	void connectionEstablished();

	RemoteAccessWidget * m_parent;
	QTimeLine m_showHideTimeLine;
	QTimeLine m_iconStateTimeLine;

	bool m_connecting;

    ToolButton* m_viewOnlyButton;
	ToolButton* m_sendShortcutButton;
	ToolButton* m_screenshotButton;
	ToolButton* m_fullScreenButton;
	ToolButton* m_exitButton;

    static constexpr int ShowHideAnimationDuration = 500;
    static constexpr int DisappearDelay = 800;

} ;





class RemoteAccessWidget : public QWidget
{
	Q_OBJECT
public:
	// 2019.05.10 hihoon change for RemoteBIOSControl feature //
	// 2019.05.10 hihoon original 
	// hihoon RemoteAccessWidget( const ComputerControlInterface::Pointer& computerControlInterface, bool viewOnly = false );
	RemoteAccessWidget( const ComputerControlInterface::Pointer& computerControlInterface, VncView::Mode remoteMode = VncView::RemoteControlMode );
	~RemoteAccessWidget() override;

	VncView* vncView() const
	{
		return m_vncView;
	}

	void toggleFullScreen( bool );
	void toggleViewOnly( bool viewOnly );
	void takeScreenshot();


protected:
	void enterEvent( QEvent* event ) override;
	void leaveEvent( QEvent* event ) override;
	void resizeEvent( QResizeEvent* event ) override;


private:
	void checkKeyEvent( unsigned int, bool );
	void updateSize();

	ComputerControlInterface::Pointer m_computerControlInterface;
	VncView* m_vncView;
	VeyonConnection* m_connection;
	RemoteAccessWidgetToolBar* m_toolBar;

	static constexpr int AppearDelay = 500;

} ;
