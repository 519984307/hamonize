/*
 * HCenterModel.cpp - item model for browsing HCenter directories
 *
 * Copyright (c) 2019 Tobias Junghans <tobydox@veyon.io>
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

#include "HCenterClient.h"
#include "HCenterConfiguration.h"
#include "HCenterBrowseModel.h"

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
#include <QAbstractItemModelTester>
#endif

// clazy:excludeall=rule-of-three

class HCenterBrowseModelNode {
public:
	enum Type {
		Root,
		DN,
		Attribute
	};

    explicit HCenterBrowseModelNode( Type type, const QString& name, HCenterBrowseModelNode *parent ) :
		m_parent( parent ),
		m_name( name ),
		m_type( type ),
		m_populated( false )
	{
		if( type == Attribute )
		{
			m_populated = true;
		}
	}

    ~HCenterBrowseModelNode()
	{
		qDeleteAll(m_childItems);
	}

    Q_DISABLE_COPY(HCenterBrowseModelNode)
    Q_DISABLE_MOVE(HCenterBrowseModelNode)

	Type type() const
	{
		return m_type;
	}

    HCenterBrowseModelNode* parent() const
	{
		return m_parent;
	}

	int row() const
	{
		if( m_parent )
		{
            return m_parent->children().indexOf( const_cast<HCenterBrowseModelNode *>(this) );
		}

		return 0;
	}

	void setPopulated( bool populated )
	{
		m_populated = populated;
	}

	bool isPopulated() const
	{
		return m_populated;
	}

    void appendChild( HCenterBrowseModelNode* item )
	{
		m_childItems.append( item );
	}

    const QList<HCenterBrowseModelNode *>& children() const
	{
		return m_childItems;
	}

	const QString& name() const
	{
		return m_name;
	}

private:
    HCenterBrowseModelNode* m_parent;
    QList<HCenterBrowseModelNode *> m_childItems;
	QString m_name;
	Type m_type;
	bool m_populated;

};



HCenterBrowseModel::HCenterBrowseModel( Mode mode, const HCenterConfiguration& configuration, QObject* parent ) :
	QAbstractItemModel( parent ),
	m_mode( mode ),
    m_client( new HCenterClient( configuration, QUrl(), this ) ),
	m_root( new Node( Node::Root, QString(), nullptr ) ),
	m_objectIcon( QStringLiteral(":/core/document-open.png") ),
    m_ouIcon( QStringLiteral( ":/HCenter/folder-stash.png") ),
    m_attributeIcon( QStringLiteral(":/HCenter/attribute.png") ),
    m_fullDn ( new QStringList() )
{
	populateRoot();

#if defined(QT_TESTLIB_LIB) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
	new QAbstractItemModelTester( this, QAbstractItemModelTester::FailureReportingMode::Warning, this );
#endif
}



HCenterBrowseModel::~HCenterBrowseModel()
{
	delete m_root;
	delete m_client;
}



QModelIndex HCenterBrowseModel::index( int row, int col, const QModelIndex& parent ) const
{
	auto childNode = toNode( parent )->children().value( row );
	if( childNode )
	{
		return createIndex( row, col, childNode );
	}

	return {};
}



QModelIndex HCenterBrowseModel::parent( const QModelIndex& child ) const
{
	if( child.isValid() == false )
	{
		return {};
	}

	auto childItem = static_cast<Node *>( child.internalPointer() );
	auto parentItem = childItem->parent();

	if( parentItem == m_root )
	{
		return {};
	}

	return createIndex( parentItem->row(), 0, parentItem );
}



QVariant HCenterBrowseModel::data( const QModelIndex& index, int role ) const
{
	if( index.isValid() == false )
	{
		return {};
	}

	const auto node = toNode( index );

	switch( role )
	{
	case Qt::DisplayRole:
        return HCenterClient::toRDNs( node->name() ).value( 0 );

	case Qt::DecorationRole:
		switch( node->type() )
		{
		case Node::Root: break;
		case Node::DN: return node->name().startsWith( QLatin1String("ou="), Qt::CaseInsensitive ) ? m_ouIcon : m_objectIcon;
		case Node::Attribute: return m_attributeIcon;
		}
		break;

	case ItemNameRole:
		return node->name();

	default:
		break;
	}

	return {};
}



Qt::ItemFlags HCenterBrowseModel::flags( const QModelIndex& index ) const
{
	if( index.isValid() == false )
	{
		return QAbstractItemModel::flags(index);
	}

	return QAbstractItemModel::flags(index) | Qt::ItemIsSelectable;
}



int HCenterBrowseModel::columnCount( const QModelIndex& parent ) const
{
	Q_UNUSED(parent);
	return 1;
}



int HCenterBrowseModel::rowCount( const QModelIndex& parent ) const
{
	if( parent.column() >  0 )
	{
		return 0;
	}

	return toNode( parent )->children().count();
}



bool HCenterBrowseModel::hasChildren( const QModelIndex& parent ) const
{
	auto parentNode = toNode( parent );

	if( parent.isValid() && parentNode && parentNode->isPopulated() )
	{
		return parentNode->children().isEmpty() == false;
	}

	return true;
}



bool HCenterBrowseModel::canFetchMore( const QModelIndex& parent ) const
{
	return toNode( parent )->isPopulated() == false;
}



void HCenterBrowseModel::fetchMore( const QModelIndex& parent )
{
	populateNode( parent );
}



QModelIndex HCenterBrowseModel::dnToIndex( const QString& dn )
{
    auto rdns = HCenterClient::toRDNs( dn );

	auto node = m_root;
	QModelIndex index;
	QStringList fullDn;

	while( rdns.isEmpty() == false )
	{
		populateNode( index );

		fullDn.prepend( rdns.takeLast() );
		const auto currentDn = fullDn.join( QLatin1Char(',') ).toLower();

		int row = 0;
		const int count = node->children().count();

		for( auto child : node->children() )
		{
			if( child->name().toLower() == currentDn )
			{
				node = child;
				index = createIndex( row, 0, node );
				break;
			}
			++row;
		}

		if( row >= count )
		{
			return index;
		}
	}

	return index;
}



QString HCenterBrowseModel::baseDn() const
{
	return m_client->baseDn();
}



void HCenterBrowseModel::populateRoot() const
{
	QStringList namingContexts;
	const auto baseDn = m_client->configuration().baseDn();

	if( baseDn.isEmpty() || m_mode == BrowseBaseDN )
	{
		namingContexts.append( m_client->queryNamingContexts() );
		namingContexts.append( m_client->queryNamingContexts( QStringLiteral("namingContexts") ) );
		namingContexts.append( m_client->queryNamingContexts( QStringLiteral("defaultNamingContext") ) );
		namingContexts.removeDuplicates();

		// remove sub naming contexts
		for( const auto& context : namingContexts )
		{
			if( context.isEmpty() == false )
			{
				namingContexts.replaceInStrings( QRegExp( QStringLiteral(".*,%1").arg( context ) ), QString() );
			}
		}
		namingContexts.removeAll( QString() );
		namingContexts.sort();
	}
	else
	{
		namingContexts.append( baseDn );
	}

    // hihoon QStringList dns_except_fullDn;
    //QStringList fullDn;

	for( const auto& namingContext : namingContexts )
	{
		auto parent = m_root;

		const auto dns = namingContext.split( QLatin1Char(',') );
#if QT_VERSION < 0x050600
#warning Building compat code for unsupported version of Qt
		typedef std::reverse_iterator<QStringList::const_iterator> QStringListReverseIterator;
		for( auto it = QStringListReverseIterator(dns.cend()),
			 end = QStringListReverseIterator(dns.cbegin()); it != end; ++it )
#else
		for( auto it = dns.crbegin(), end = dns.crend(); it != end; ++it )
#endif
		{
            m_fullDn->prepend( *it );
            auto node = new Node( Node::DN, m_fullDn->join( QLatin1Char(',') ), parent );
			parent->appendChild( node );
			parent = node;
		}
	}

    // hihoon
    //m_listbaseDn = fullDn;
	m_root->setPopulated( true );
}



void HCenterBrowseModel::populateNode( const QModelIndex& parent )
{
	auto node = toNode( parent );

    if( node->isPopulated() == false )
    {

        if( m_client->baseDn().length() <= node->name().length() ) {

            auto dns = m_client->queryDistinguishedNames( node->name(), QString(), HCenterClient::Scope::One );
            dns.sort();

            QStringList attributes;

            if( m_mode == BrowseAttributes )
            {
                attributes = m_client->queryObjectAttributes( node->name() );
                attributes.sort();
            }

            const auto itemCount = ( dns + attributes ).size();

            if( itemCount > 0 )
            {
                beginInsertRows( parent, 0, itemCount - 1 );

                for( const auto& dn : dns )
                {
                    node->appendChild( new Node( Node::DN, dn, node ) );
                }

                for( const auto& attribute : qAsConst(attributes) )
                {
                    node->appendChild( new Node( Node::Attribute, attribute, node ) );
                }

                endInsertRows();

                emit layoutChanged();
            }
        }
		node->setPopulated( true );
	}
}



HCenterBrowseModel::Node*HCenterBrowseModel::toNode( const QModelIndex& index ) const
{
	return index.isValid() ? static_cast<Node *>( index.internalPointer() ) : m_root;
}
