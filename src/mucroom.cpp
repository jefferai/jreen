/****************************************************************************
 *  mucroom.cpp
 *
 *  Copyright (c) 2010-2011 by Nigmatullin Ruslan <euroelessar@gmail.com>
 *  Copyright (c) 2011 by Sidorov Aleksey <sauron@citadelspb.com>
 *
 ***************************************************************************
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*****************************************************************************/
#include "mucroom_p.h"

namespace jreen
{
enum MUCRolePrivilege
{
	SendMessage,
	ModifySubject,
	KickParticipantsAndVisitors,
	GrantVoice,
	RevokeVoice,
	RolePrivelegesCount
};

char mucPrivelegesByRole[RolePrivelegesCount][4] = {
	{ 0, 1, 1 },
	{ 0, 1, 1 },
	{ 0, 1, 1 },
	{ 0, 0, 1 },
	{ 0, 0, 1 }
};

bool checkParticipantPrivelege(MUCRolePrivilege priv, MUCRoom::Role role)
{
	return role != MUCRoom::RoleNone && mucPrivelegesByRole[priv][role - 1];
}

//	enum MUCAffiliationPrivilege
//	{
//		SendMessage,
//		ModifySubject,
//		KickParticipantsAndVisitors,
//		GrantVoice,
//		RevokeVoice,
//		RolePrivelegesCount
//	};

//	char mucPrivelegesByAffiliation[RolePrivelegesCount][4] = {
//		{ 0, 1, 1 },
//		{ 0, 1, 1 },
//		{ 0, 1, 1 },
//		{ 0, 0, 1 },
//		{ 0, 0, 1 }
//	};

//	bool checkParticipantPrivelege(MUCAffiliationPrivilege priv, MUCRoom::Affiliation aff)
//	{
//		return aff != MUCRoom::AffiliationOutcast && mucPrivelegesByAffiliation[priv][aff - 1];
//	}

class MUCRoom::ParticipantPrivate
{
public:
	void init(const Presence &pres)
	{
		query = pres.findExtension<MUCRoomUserQuery>();
	}

	MUCRoomUserQuery::Ptr query;
};

MUCRoom::Participant::Participant() : d_ptr(new ParticipantPrivate)
{
}

MUCRoom::Participant::~Participant()
{
}

MUCRoom::Affiliation MUCRoom::Participant::affiliation() const
{
	return d_func()->query->item.affiliation;
}

MUCRoom::Role MUCRoom::Participant::role() const
{
	return d_func()->query->item.role;
}

bool MUCRoom::Participant::isSelf() const
{
	return d_func()->query->flags & MUCRoomUserQuery::Self;
}

bool MUCRoom::Participant::isNickChanged() const
{
	return d_func()->query->flags & MUCRoomUserQuery::NickChanged;
}

bool MUCRoom::Participant::isBanned() const
{
	return d_func()->query->flags & MUCRoomUserQuery::Banned;
}

bool MUCRoom::Participant::isKicked() const
{
	return d_func()->query->flags & MUCRoomUserQuery::Kicked;
}

QString MUCRoom::Participant::newNick() const
{
	return d_func()->query->item.nick;
}

QString MUCRoom::Participant::reason() const
{
	return d_func()->query->item.reason;
}

JID MUCRoom::Participant::realJID() const
{
	return d_func()->query->item.jid;
}

void MUCRoomPrivate::handlePresence(const Presence &pres)
{
	Q_Q(MUCRoom);
	qDebug() << "handle presence" << pres.from();
	if (Error::Ptr e = pres.findExtension<Error>()) {
		emit q->error(e);
		return;
	}
	MUCRoom::Participant part;
	part.d_func()->query = pres.findExtension<MUCRoomUserQuery>();
	if (!part.d_func()->query)
		return;
	if (pres.from().resource() == jid.resource()) {
		if(pres.subtype() == Presence::Unavailable) {
			isJoined = false;
			emit q->leaved();
		} else if (!isJoined) {
			realJidHash.clear();
			isJoined = true;
			emit q->joined();
		}
	}
	if (pres.subtype() == Presence::Unavailable)
		realJidHash.remove(pres.from().resource());
	else
		realJidHash.insert(pres.from().resource(), part.realJID());
	if (part.isNickChanged() && pres.from().resource() == jid.resource())
		jid.setResource(part.newNick());
	emit q->presenceReceived(pres, &part);
}

void MUCRoomPrivate::handleMessage(const Message &msg)
{
	Q_Q(MUCRoom);
	bool nice = false;
	if (msg.from() == jid.bare()) {
		qDebug() << "service message" << msg.from() << jid;
		emit q->serviceMessageReceived(msg);
		nice = true;
	}
	if (!msg.subject().isEmpty()) {
		qDebug() << "subject message" << msg.from() << jid;
		subject = msg.subject();
		emit q->subjectChanged(subject, msg.from().resource());
		nice = true;
	}
	if (!nice && !msg.body().isEmpty()) {
		qDebug() << "common message" << msg.from() << jid;
		emit q->messageReceived(msg, msg.subtype() != Message::Groupchat);
	}
}

MUCRoom::MUCRoom(Client *client, const JID &jid) :
	QObject(client),
	d_ptr(new MUCRoomPrivate(this))
{
	Q_D(MUCRoom);
	d->client = client;
	d->jid = jid;
	d->session = new MUCMessageSession(this);
	ClientPrivate::get(d->client)->rooms.insert(d->jid.bare(), d);
	connect(client, SIGNAL(connected()), this, SLOT(onConnected()));
	connect(client, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
}

MUCRoom::~MUCRoom()
{
	Q_D(MUCRoom);
	if (!d->client)
		return;
	ClientPrivate::get(d->client)->rooms.remove(d->jid.bare());
}

QString MUCRoom::id() const
{
	return d_func()->jid.bare();
}

QString MUCRoom::service() const
{
	return d_func()->jid.domain();
}

bool MUCRoom::isJoined() const
{
	return d_func()->isJoined;
}

Presence::Type MUCRoom::presence() const
{
	return d_func()->currentPresence.subtype();
}

void MUCRoom::join(Presence::Type type, const QString &message, int priority)
{
	Q_D(MUCRoom);
	Presence pres(type, d->jid, message, priority);
	qDebug() << Q_FUNC_INFO << type << d->jid;
	MUCRoomQuery *query = new MUCRoomQuery(d->password);
	query->setMaxChars(d->maxChars);
	query->setMaxStanzas(d->maxStanzas);
	query->setSeconds(d->seconds);
	query->setSince(d->since);
	pres.addExtension(query);
	d->currentPresence = pres;
	d->client->send(pres);
}

void MUCRoom::join()
{
	Q_D(MUCRoom);
	Presence pres = d->client->presence();
	join(pres.subtype(), pres.status(), pres.priority());
}

enum MUCRoomRequestContext
{
	MUCRoomRequestConfig = 100,
	MUCRoomSubmitConfig
};

void MUCRoom::requestRoomConfig()
{
	Q_D(MUCRoom);
	IQ iq(IQ::Get, d->jid.bareJID());
	iq.addExtension(new MUCRoomOwnerQuery);
	d->client->send(iq, this, SLOT(handleIQ(jreen::IQ,int)), MUCRoomRequestConfig);
}

void MUCRoom::setRoomConfig(const jreen::DataForm::Ptr &form)
{
	Q_D(MUCRoom);
	IQ iq(IQ::Get, d->jid.bareJID());
	iq.addExtension(new MUCRoomOwnerQuery(form));
	d->client->send(iq, this, SLOT(handleIQ(jreen::IQ,int)), MUCRoomSubmitConfig);
}

void MUCRoom::leave(const QString &message)
{
	Q_D(MUCRoom);
	if (d->currentPresence.subtype() == Presence::Unavailable)
		return;
	d->isJoined = false;
	Presence pres(Presence::Unavailable, d->jid, message);
	d->currentPresence = pres;
	d->client->send(pres);
}

QString MUCRoom::nick() const
{
	return d_func()->jid.resource();
}

JID MUCRoom::realJid(const QString &nick)
{
	return d_func()->realJidHash.value(nick);
}

void MUCRoom::setNick(const QString &nick)
{
	Q_D(MUCRoom);
	if (d->isJoined) {
		JID newJid = d->jid;
		newJid.setResource(nick);
		Presence pres(d->currentPresence.subtype(), newJid,
					  d->currentPresence.status(), d->currentPresence.priority());
		d->client->send(pres);
	} else {
		d->jid.setResource(nick);
	}
}

void MUCRoom::setHistoryMaxChars(int maxChars)
{
	d_func()->maxChars = maxChars;
}

void MUCRoom::setHistoryMaxStanzas(int maxStanzas)
{
	d_func()->maxStanzas = maxStanzas;
}

void MUCRoom::setHistorySeconds(int seconds)
{
	d_func()->seconds = seconds;
}

void MUCRoom::setHistorySince(const QDateTime &since)
{
	d_func()->since = since;
}

void MUCRoom::setRole(const QString &nick, Role role, const QString &reason)
{
	Q_D(MUCRoom);
	IQ iq(IQ::Set, d->jid.bareJID());
	iq.addExtension(new MUCRoomAdminQuery(nick, role, reason));
	d->client->send(iq);
}

void MUCRoom::setAffiliation(const QString &nick, Affiliation affiliation, const QString &reason)
{
	Q_D(MUCRoom);
	IQ iq(IQ::Set, d->jid.bareJID());
	iq.addExtension(new MUCRoomAdminQuery(nick, affiliation, reason));
	d->client->send(iq);
}

void MUCRoom::send(const QString &message)
{
	Q_D(MUCRoom);
	d->session->sendMessage(message);
}

QString MUCRoom::subject() const
{
	return d_func()->subject;
}

void MUCRoom::setSubject(const QString &subject)
{
	Q_D(MUCRoom);
	d->session->setSubject(subject);
}

void MUCRoom::handleIQ(const jreen::IQ &iq, int context)
{
	if (Error::Ptr e = iq.findExtension<Error>()) {
		emit error(e);
		return;
	}
	if (context == MUCRoomRequestConfig) {
		MUCRoomOwnerQuery::Ptr query = iq.findExtension<MUCRoomOwnerQuery>();
		if (!query)
			return;
		emit configurationReceived(query->form);
	}
}

void MUCRoom::onConnected()
{
	Q_D(MUCRoom);
	if (d->currentPresence.subtype() != Presence::Unavailable)
		join(d->currentPresence.subtype(), d->currentPresence.status(), d->currentPresence.priority());
}

void MUCRoom::onDisconnected()
{
	Q_D(MUCRoom);
	if (d->currentPresence.subtype() != Presence::Unavailable) {
		d->isJoined = false;
		emit leaved();
	}
}
}
