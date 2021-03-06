/****************************************************************************
**
** Copyright (C) 2015 basysKom GmbH, opensource@basyskom.com
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtOpcUa module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qopcuaclient.h"
#include "qopcuanode.h"
#include <private/qopcuaclient_p.h>
#include <private/qopcuaclientimpl_p.h>
#include <private/qopcuanode_p.h>
#include <private/qopcuanodeimpl_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QOpcUaNode
    \inmodule QtOpcUa

    \brief QOpcUaNode allows interaction with an OPC UA node.


    The node is the basic building block of the OPC UA address space.
    It has attributes like browse name, value, associated properties and can have
    references to other nodes in the address space.
    Nodes are organized in namespaces and have IDs which can e.g. be numeric,
    a string, a namespace-specific format (opaque) or a globally unique identifier.
    A node is identified by the namespace ID and the node ID.
    This identifier is usually given as a string:
    The identifier of a node residing in namespace 0 and having the numeric
    identifier 42 results in the string \c ns=0;i=42. A node with a string
    identifier can be addressed via \c ns=0;s=myStringIdentifier.

    Objects of this type are owned by the user and must be deleted when they are
    no longer needed. They are valid as long as the \l QOpcUaClient which created them exists.

    \section1 Reading and writing of attributes

    The node attributes are read from the server when \l readAttributes() or \l readAttributeRange()
    is called. The results are cached locally and can be retrieved using \l attribute()
    after the \l attributeRead signal has been received.

    Attributes can be written using \l writeAttribute(), \l writeAttributes() and \l writeAttributeRange()
    if the user has the necessary rights.
    Success of the write operation is reported using the \l attributeWritten signal.

    \l attributeError() contains a status code associated with the last read or write operation
    on the attribute. This is the low level status code returned by the OPC UA service.
    This status code can be simplified by converting it to a \l QOpcUa::ErrorCategory using
    \l QOpcUa::errorCategory().

    \section1 Subscribing to data changes
    Subscriptions are a concept in OPC UA which allows receiving of notifications for changes in data
    or in case of events instead of continuously polling a node for changes.
    Monitored items define how attributes of a node are watched for changes. They are added to a
    subscription and any notifications they generate are forwarded to the user via the subscription.
    The interval of the updates as well as many other options of the monitored items and subscriptions
    can be configured by the user.

    \l QOpcUaNode offers an abstraction to interact with subscriptions and monitored items.
    \l enableMonitoring() enables data change notifications for one or more attributes.
    The \l attributeUpdated signal contains new values and the local cache is updated.
    \l disableMonitoring() disables the data change notifications.
    The \l monitoringStatusChanged signal notifies about changes of the monitoring status, e. g. after
    manual enable and disable or a status change on the server.

    Settings of the subscription and monitored item can be modified at runtime using \l modifyMonitoring().

    \section1 Browsing the address space
    The OPC UA address space consists of nodes connected by references.
    \l browseChildren follows these references in forward direction and returns attributes from all
    nodes connected to the node behind an instance of \l QOpcUaNode in the \l browseFinished signal.

    \section1 Method calls
    OPC UA specifies methods on the server which can be called by the user.
    \l QOpcUaNode supports this via \l callMethod which takes parameters and returns the results of
    the call in the \l methodCallFinished signal.

    \section1 Example
    For connecting the client to a server and getting a \l QOpcUaNode object, see \l QOpcUaClient.

    After the node has been successfully created, the BrowseName of the root node is read from the server:

    \code
    QOpcUaNode *rootNode; // Created before, see QOpcUaClient documentation.
    // Connect to the attributeRead signal. Compatible slots of QObjects can be used instead of a lambda.
    QObject::connect(rootNode, &QOpcUaNode::attributeRead, [rootNode, client](QOpcUa::NodeAttributes attr) {
        qDebug() << "Signal for attributes:" << attr;
        if (rootNode->attributeError(QOpcUa::NodeAttribute::BrowseName) != QOpcUa::UaStatusCode::Good) {
            qDebug() << "Failed to read attribute:" << rootNode->attributeError(QOpcUa::NodeAttribute::BrowseName);
            client->disconnectFromEndpoint();
        }
        qDebug() << "Browse name:" << rootNode->attribute(QOpcUa::NodeAttribute::BrowseName).value<QOpcUa::QQualifiedName>().name;
    });
    rootNode->readAttributes(QOpcUa::NodeAttribute::BrowseName); // Start a read operation for the node's BrowseName attribute.
    \endcode
*/

/*!
    \typedef QOpcUaNode::AttributeMap

    This type is used by \l writeAttributes() to write more than one attribute at a time.
    QVariant values must be assigned to the attributes to be written.
*/

/*!
    \fn void QOpcUaNode::attributeRead(QOpcUa::NodeAttributes attributes)

    This signal is emitted after a \l readAttributes() or \l readAttributeRange() operation has finished.
    The receiver has to check the status code for the attributes contained in \a attributes.
*/

/*!
    \fn void QOpcUaNode::attributeWritten(QOpcUa::NodeAttribute attribute, QOpcUa::UaStatusCode statusCode)

    This signal is emitted after a \l writeAttribute(), \l writeAttributes() or \l writeAttributeRange()
    operation has finished.

    Before this signal is emitted, the attribute cache is updated in case of a successful write.
    For \l writeAttributes() a signal is emitted for each attribute in the write call.
    \a statusCode contains the success information for the write operation on \a attribute.
*/

/*!
    \fn void QOpcUaNode::attributeUpdated(QOpcUa::NodeAttribute attr, QVariant value)

    This signal is emitted after a data change notification has been received. \a value contains the
    new value for the node attribute \a attr.
*/

/*!
    \fn void QOpcUaNode::enableMonitoringFinished(QOpcUa::NodeAttribute attr, QOpcUa::UaStatusCode statusCode)

    This signal is emitted after an asynchronous call to \l enableMonitoring() has finished.
    After this signal has been emitted, \l monitoringStatus() returns valid information for \a attr.
    \a statusCode contains the status code for the operation.
*/

/*!
    \fn void QOpcUaNode::disableMonitoringFinished(QOpcUa::NodeAttribute attr, QOpcUa::UaStatusCode statusCode)

    This signal is emitted after an asynchronous call to \l disableMonitoring() has finished. \a statusCode contains
    the status code generated by the operation.
    After this signal has been emitted, monitoringStatus returns a default constructed value with
    status code BadMonitoredItemIdIinvalid for \a attr.
*/

/*!
    \fn void QOpcUaNode::monitoringStatusChanged(QOpcUa::NodeAttribute attr, QOpcUaMonitoringParameters::Parameters items, QOpcUa::UaStatusCode statusCode);

    This signal is emitted after an asynchronous call to \l modifyMonitoring() has finished.
    The node attribute for which the operation was requested is returned in \a attr. \a items contains the parameters that have been modified.
    \a statusCode contains the result of the modify operation on the server.
*/

/*!
    \fn void QOpcUaNode::methodCallFinished(QString methodNodeId, QVariant result, QOpcUa::UaStatusCode statusCode)

    This signal is emitted after a method call for \a methodNodeId has finished on the server.
    \a statusCode contains the status code from the method call, \a result contains the output
    arguments of the method. \a result is empty if the method has no output arguments or \a statusCode
    is not \l {QOpcUa::UaStatusCode} {Good}.
*/

/*!
    \fn void QOpcUaNode::browseFinished(QVector<QOpcUaReferenceDescription> children, QOpcUa::UaStatusCode statusCode)

    This signal is emitted after a \l browseChildren() operation has finished.

    \a children contains information about all nodes which matched the criteria in \l browseChildren().
    \a statusCode contains the service result of the browse operation. If \a statusCode is not \l {QOpcUa::UaStatusCode} {Good},
    the passed \a children vector is empty.
    \sa QOpcUaReferenceDescription
*/

/*!
    \fn QOpcUa::NodeAttributes QOpcUaNode::mandatoryBaseAttributes()

    Contains all mandatory attributes of the OPC UA base node class.
*/

/*!
    \fn QOpcUa::NodeAttributes QOpcUaNode::allBaseAttributes()

    Contains all attributes of the OPC UA base node class.
*/

/*!
    \internal QOpcUaNodeImpl is an opaque type (as seen from the public API).
    This prevents users of the public API to use this constructor (eventhough
    it is public).
*/
QOpcUaNode::QOpcUaNode(QOpcUaNodeImpl *impl, QOpcUaClient *client, QObject *parent)
    : QObject(*new QOpcUaNodePrivate(impl, client), parent)
{
}

QOpcUaNode::~QOpcUaNode()
{
}

/*!
    Starts an asynchronous read operation for the node attribute \a attribute.
    \a indexRange is a string which can be used to select a part of an array. It is defined in OPC-UA part 4, 7.22.
    The first element in an array is 0, "1" returns the second element, "0:9" returns the first 10 elements,
    "0,1" returns the second element of the first row in a two-dimensional array.

    Returns \c true if the asynchronous call has been successfully dispatched.

    Attribute values only contain valid information after the \l attributeRead signal has been emitted.
*/
bool QOpcUaNode::readAttributeRange(QOpcUa::NodeAttribute attribute, const QString &indexRange)
{
    Q_D(QOpcUaNode);
    if (d->m_client.isNull() || d->m_client->state() != QOpcUaClient::Connected)
        return false;

    return d->m_impl->readAttributes(QOpcUa::NodeAttributes() | attribute, indexRange);
}

/*!
    Starts an asynchronous read operation for the node attributes in \a attributes.

    Returns \c true if the asynchronous call has been successfully dispatched.

    Attribute values only contain valid information after the \l attributeRead signal has been emitted.
*/
bool QOpcUaNode::readAttributes(QOpcUa::NodeAttributes attributes)
{
    Q_D(QOpcUaNode);
    if (d->m_client.isNull() || d->m_client->state() != QOpcUaClient::Connected)
        return false;

    return d->m_impl->readAttributes(attributes, QString());
}

/*!
    Returns the value of the attribute given in \a attribute.

    The value is only valid after the \l attributeRead signal has been emitted.
    An empty QVariant is returned if there is no cached value for the attribute.
*/
QVariant QOpcUaNode::attribute(QOpcUa::NodeAttribute attribute) const
{
    Q_D(const QOpcUaNode);
    auto it = d->m_nodeAttributes.constFind(attribute);
    if (it == d->m_nodeAttributes.constEnd())
        return QVariant();

    return it->value;
}

/*!
    Returns the error code for the attribute given in \a attribute.

    The error code is only valid after the \l attributeRead or \l attributeWritten signal has been emitted.

    \sa QOpcUa::errorCategory
 */
QOpcUa::UaStatusCode QOpcUaNode::attributeError(QOpcUa::NodeAttribute attribute) const
{
    Q_D(const QOpcUaNode);
    auto it = d->m_nodeAttributes.constFind(attribute);
    if (it == d->m_nodeAttributes.constEnd())
        return QOpcUa::UaStatusCode::BadNotFound;

    return it->statusCode;
}

/*!
    Returns the source timestamp from the last read or data change of \a attribute.
    Before at least one \l attributeRead or \l attributeUpdated signal has been emitted,
    a null datetime is returned.

*/
QDateTime QOpcUaNode::sourceTimestamp(QOpcUa::NodeAttribute attribute) const
{
    Q_D(const QOpcUaNode);
    auto it = d->m_nodeAttributes.constFind(attribute);
    if (it == d->m_nodeAttributes.constEnd())
        return QDateTime();

    return it->sourceTimestamp;
}

/*!
    Returns the server timestamp from the last read or data change of \a attribute.
    Before at least one \l attributeRead or \l attributeUpdated signal has been emitted,
    a null datetime is returned.
*/
QDateTime QOpcUaNode::serverTimestamp(QOpcUa::NodeAttribute attribute) const
{
    Q_D(const QOpcUaNode);
    auto it = d->m_nodeAttributes.constFind(attribute);
    if (it == d->m_nodeAttributes.constEnd())
        return QDateTime();

    return it->serverTimestamp;
}

/*!
    This method creates a monitored item for each of the attributes given in \a attr.
    The settings from \a settings are used in the creation of the monitored items and the subscription.

    Returns \c true if the asynchronous call has been successfully dispatched.

    On completion of the call, the \l enableMonitoringFinished signal is emitted.
    There are multiple error cases in which a bad status code is generated: A subscription with the subscription id specified in \a settings does not exist,
    the node does not exist on the server, the node does not have the requested attribute or the maximum number of monitored items for
    the server is reached.
 */
bool QOpcUaNode::enableMonitoring(QOpcUa::NodeAttributes attr, const QOpcUaMonitoringParameters &settings)
{
    Q_D(QOpcUaNode);
    if (d->m_client.isNull() || d->m_client->state() != QOpcUaClient::Connected)
        return false;

    return d->m_impl->enableMonitoring(attr, settings);
}

/*!
    This method modifies settings of the monitored item or the subscription.
    The parameter \a item of the monitored item or subscription associated with \a attr is attempted to set to \a value.

    Returns \c true if the asynchronous call has been successfully dispatched.

    After the call has finished, the \l monitoringStatusChanged signal is emitted. This signal contains the modified parameters and the status code.
    A bad status code is generated if there is no monitored item associated with the requested attribute, modifying the requested
    parameter is not implemented or if the server has rejected the requested value.
*/
bool QOpcUaNode::modifyMonitoring(QOpcUa::NodeAttribute attr, QOpcUaMonitoringParameters::Parameter item, const QVariant &value)
{
    Q_D(QOpcUaNode);
    if (d->m_client.isNull() || d->m_client->state() != QOpcUaClient::Connected)
        return false;

    return d->m_impl->modifyMonitoring(attr, item, value);
}

/*!
    Returns the monitoring parameters associated with the attribute \a attr. This can be used to check the success of \l enableMonitoring()
    or if parameters have been revised.
    The returned values are only valid after \l enableMonitoringFinished or \l monitoringStatusChanged have been emitted for \a attr.
*/
QOpcUaMonitoringParameters QOpcUaNode::monitoringStatus(QOpcUa::NodeAttribute attr)
{
    Q_D(QOpcUaNode);
    auto it = d->m_monitoringStatus.constFind(attr);
    if (it == d->m_monitoringStatus.constEnd()) {
        QOpcUaMonitoringParameters p;
        p.setStatusCode(QOpcUa::UaStatusCode::BadAttributeIdInvalid);
        return p;
    }

    return *it;
}

/*!
    Modifies an existing data change monitoring to use \a filter as data change filter.

    Returns \c true if the filter modification request has been successfully dispatched to the backend.

    \l monitoringStatusChanged for \a attr is emitted after the operation has finished.
*/
bool QOpcUaNode::modifyDataChangeFilter(QOpcUa::NodeAttribute attr, const QOpcUaMonitoringParameters::DataChangeFilter &filter)
{
    return modifyMonitoring(attr, QOpcUaMonitoringParameters::Parameter::Filter, QVariant::fromValue(filter));
}

/*!
    Writes \a value to the attribute given in \a attribute using the type information from \a type.
    Returns \c true if the asynchronous call has been successfully dispatched.

    If the \a type parameter is omitted, the backend tries to find the correct type. The following default types are assumed:
    \table
        \header
            \li Qt MetaType
            \li OPC UA type
        \row
            \li Bool
            \li Boolean
        \row
            \li UChar
            \li Byte
        \row
            \li Char
            \li SByte
        \row
            \li UShort
            \li UInt16
        \row
            \li Short
            \li Int16
        \row
            \li Int
            \li Int32
        \row
            \li UInt
            \li UInt32
        \row
            \li ULongLong
            \li UInt64
        \row
            \li LongLong
            \li Int64
        \row
            \li Double
            \li Double
        \row
            \li Float
            \li Float
        \row
            \li QString
            \li String
        \row
            \li QDateTime
            \li DateTime
        \row
            \li QByteArray
            \li ByteString
        \row
            \li QUuid
            \li Guid
    \endtable
*/

bool QOpcUaNode::writeAttribute(QOpcUa::NodeAttribute attribute, const QVariant &value, QOpcUa::Types type)
{
    Q_D(QOpcUaNode);
    if (d->m_client.isNull() || d->m_client->state() != QOpcUaClient::Connected)
        return false;

    return d->m_impl->writeAttribute(attribute, value, type, QString());
}

/*!
    Writes \a value to the attribute given in \a attribute using the type information from \a type.
    For \a indexRange, see \l readAttributeRange().

    Returns \c true if the asynchronous call has been successfully dispatched.
*/
bool QOpcUaNode::writeAttributeRange(QOpcUa::NodeAttribute attribute, const QVariant &value, const QString &indexRange, QOpcUa::Types type)
{
    Q_D(QOpcUaNode);
    if (d->m_client.isNull() || d->m_client->state() != QOpcUaClient::Connected)
        return false;

    return d->m_impl->writeAttribute(attribute, value, type, indexRange);
}

/*!
    Executes a write operation for the attributes and values specified in \a toWrite.

    Returns \c true if the asynchronous call has been successfully dispatched.

    The \a valueAttributeType parameter can be used to supply type information for the value attribute.
    All other attributes have known types.
    \sa writeAttribute()
*/
bool QOpcUaNode::writeAttributes(const AttributeMap &toWrite, QOpcUa::Types valueAttributeType)
{
    Q_D(QOpcUaNode);
    if (d->m_client.isNull() || d->m_client->state() != QOpcUaClient::Connected)
        return false;

    return d->m_impl->writeAttributes(toWrite, valueAttributeType);
}

/*!
    This method disables monitoring for the attributes given in \a attr.

    Returns \c true if the asynchronous call has been successfully dispatched.

    After the call is finished, the \l disableMonitoringFinished signal is emitted and monitoringStatus returns a default constructed value with
    status code BadMonitoredItemIdIinvalid for \a attr.
*/
bool QOpcUaNode::disableMonitoring(QOpcUa::NodeAttributes attr)
{
  Q_D(QOpcUaNode);
  if (d->m_client.isNull() || d->m_client->state() != QOpcUaClient::Connected)
      return false;

  return d->m_impl->disableMonitoring(attr);
}

/*!
    Executes a forward browse call starting from the node this method is called on.
    The browse operation collects information about child nodes connected to the node
    and delivers the results in the \l browseFinished() signal.

    Returns \c true if the asynchronous call has been successfully dispatched.

    To request only children connected to the node by a certain type of reference, \a referenceType must be set to that reference type.
    For example, this can be  used to get all properties of a node by passing \l {QOpcUa::ReferenceTypeId} {HasProperty} in \a referenceType.
    The results can be filtered to contain only nodes with certain node classes by setting them in \a nodeClassMask.
*/
bool QOpcUaNode::browseChildren(QOpcUa::ReferenceTypeId referenceType, QOpcUa::NodeClasses nodeClassMask)
{
    Q_D(QOpcUaNode);
    if (d->m_client.isNull() || d->m_client->state() != QOpcUaClient::Connected)
        return false;

    return d->m_impl->browseChildren(referenceType, nodeClassMask);
}

/*!
    Returns the ID of the OPC UA node.
*/
QString QOpcUaNode::nodeId() const
{
    Q_D(const QOpcUaNode);
    if (d->m_client.isNull() || d->m_client->state() != QOpcUaClient::Connected)
        return QString();

    return d->m_impl->nodeId();
}

/*!
    Calls the OPC UA method \a methodNodeId with the parameters given via \a args. The result is
    returned in the \l methodCallFinished signal.

    Returns \c true if the asynchronous call has been successfully dispatched.
*/
bool QOpcUaNode::callMethod(const QString &methodNodeId, const QVector<QOpcUa::TypedVariant> &args)
{
    Q_D(QOpcUaNode);
    if (d->m_client.isNull() || d->m_client->state() != QOpcUaClient::Connected)
        return false;

    return d->m_impl->callMethod(methodNodeId, args);
}

QDebug operator<<(QDebug dbg, const QOpcUaNode &node)
{
    dbg << "QOpcUaNode {"
        << "DisplayName:" << node.attribute(QOpcUa::NodeAttribute::DisplayName)
        << "Id:" << node.attribute(QOpcUa::NodeAttribute::NodeId)
        << "Class:" << node.attribute(QOpcUa::NodeAttribute::NodeClass)
        << "}";
    return dbg;
}

QT_END_NAMESPACE
