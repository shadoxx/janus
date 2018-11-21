#include "multiplayermanager.h"

unsigned int MultiPlayerManager::max_connect_retries = 3;

ServerConnectionListener::ServerConnectionListener()
{
}

QList <QString> & ServerConnectionListener::GetMessages()
{
    return messages;
}

void ServerConnectionListener::ClearMessages()
{
    messages.clear();
}

void ServerConnectionListener::SocketMessageReceived(QString message)
{
    //qDebug() << "ServerConnectionListener::SocketMessageReceived() - " << message;
    messages.push_back(message);
}

MultiPlayerManager::MultiPlayerManager() :
    enabled(true),
    partymode(false),
    reset_player(false),
    cscale_list(1.0f),
    pending_pos(false),
    avatar_update(false),
    recording(false),
    recording_everyone(true)
{
    qsrand(static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()));
    userid_filename = MathUtil::GetAppDataPath() + "userid.txt";
    userid_backup_filename = MathUtil::GetAppDataPath() + "/userid_";
    qDebug() << "MultiPlayerManager::MultiPlayerManager() - SSL enabled? " << QSslSocket::supportsSsl();

    user_ghost = new RoomObject();
}

MultiPlayerManager::~MultiPlayerManager()
{
	Clear();
	
	room_edit_objs.clear();
	connection_list.clear();
	chat_messages_log.clear();
	mic_buffers.clear();
    pending_pos = false;
	ghost_packets.clear();
	ghost_recording_mic_buffers.clear();

    if (user_ghost) {
        delete user_ghost;
    }
}

void MultiPlayerManager::Initialize()
{    
    LoadAvatarData(true);
    update_time.start();
    avatar_update_timer.start();
    avatar_update = true;
}

QList <ServerConnection> & MultiPlayerManager::GetConnectionList()
{
    return connection_list;
}

void MultiPlayerManager::DoSocketConnect()
{
    for (int i=0; i<connection_list.size(); ++i) {

        ServerConnection & s = connection_list[i];

        if (s.tcpserver == "private") {
            continue;
        }

        if (s.tcpsocket->state() == QAbstractSocket::UnconnectedState && s.reconnect_time.elapsed() > 1000 && s.retries < max_connect_retries) {
            qDebug() << "MultiPlayerManager::DoSocketConnect() - tcpsocket trying" << s.tcpserver << s.tcpport << "ssl?" << s.use_ssl << QSslSocket::supportsSsl() << "retry" << s.retries << "...";
            if (s.use_ssl && QSslSocket::supportsSsl() && s.tcpport == 5567) {
                s.tcpsocket->connectToHostEncrypted(s.tcpserver, s.tcpport);
            }
            else {
                s.tcpsocket->connectToHost(s.tcpserver, s.tcpport);
            }
            s.reconnect_time.restart();
            ++s.retries;
        }
        else if (s.tcpsocket->state() == QAbstractSocket::ConnectedState && !s.logging_in && !s.logged_in) {
            qDebug() << "MultiPlayerManager::DoSocketConnect() - tcpsocket connecting" << s.tcpserver << s.tcpport << "ssl?" << s.use_ssl << QSslSocket::supportsSsl() << "retry" << s.retries << "...";
            QString logon_packet;
            if (s.user.length() > 0) {
                logon_packet = GetLogonPacket(s.user, s.password, cur_url_id);
            }
            else {
                logon_packet = GetLogonPacket(user_ghost->GetProperties()->GetID(), s.password, cur_url_id);
            }
            s.logging_in = true;
            s.tcpsocket->write(logon_packet.toUtf8());
        }
    }
}

void MultiPlayerManager::DoSocketDisconnect()
{
    for (int i=0; i<connection_list.size(); ++i) {
        ServerConnection & s = connection_list[i];

        if (s.tcpsocket->state() != QAbstractSocket::UnconnectedState) {
            s.tcpsocket->disconnectFromHost();
        }
        for (int j=0; j<s.rooms.size(); ++j) {
            s.rooms[j].sent_subscribe = false;
        }

        s.logged_in = false;
        s.logging_in = false;
        s.retries = 0;
        s.sent_enterroom_url_id.clear();
    }
}

void MultiPlayerManager::Clear()
{
    DoSocketDisconnect();
    for (int i=0; i<connection_list.size(); ++i) {
        ServerConnection & s = connection_list[i];
        if (s.tcpsocket) {
            if (s.tcpsocket->isOpen()) {
                s.tcpsocket->close();
            }
            delete s.tcpsocket;
            s.tcpsocket = NULL;
        }
    }
    connection_list.clear();
}

void MultiPlayerManager::DoSessionStart()
{
    //send tracking statistics to EASEVR
    //http://api.easevr.com/v1/client/{guid}/session_start
//    api_key (API key; string)
//    sid (session id; string)
//    ts (timestamp; utc time)
//    udid (unique identifier; string)
//    hmd (HMD name; string)
//    ver (experience version; string)
//    os (operating system; string)
//    os_ver (os version; string)
//    mem (memory total; double)
//    proc (processor name; string)

//    qDebug() << "MultiPlayerManager::DoSessionStart() - Sending session_start packet";
//    session_data["api_key"] = "0123456789abcdef";
//    session_data["sid"] = QString::number(QDateTime::currentMSecsSinceEpoch()); //use ts as session id
//    session_data["ts"] = QString::number(QDateTime::currentMSecsSinceEpoch());
//    session_data["udid"] = (player ? player->GetUserID() : "");
//    session_data["hmd"] = (player ? player->GetHMDString() : "");
//    session_data["ver"] = version;
//    session_data["os"] = QSysInfo::kernelType();
//    session_data["os_ver"] = QSysInfo::kernelVersion();
//    session_data["mem"] = "0"; //mem must be a number
//    session_data["proc"] = QSysInfo::currentCpuArchitecture();
//    session_start_webasset.DoHTTPPost(QUrl("http://api.easevr.com/v1/client/janusvr/session_start"), session_data);
}

void MultiPlayerManager::DoSessionEnd()
{
    //send tracking statistics to EASEVR
    //http://api.easevr.com/v1/client/{guid}/session_end
//    api_key (API key; string)
//    sid (session id; string)
//    ts (timestamp; utc time)

//    qDebug() << "MultiPlayerManager::DoSessionEnd() - Sending session_end packet";
//    session_data["ts"] = QString::number(QDateTime::currentMSecsSinceEpoch()); //update timestamp
//    session_data.remove("udid");
//    session_data.remove("hmd");
//    session_data.remove("ver");
//    session_data.remove("os");
//    session_data.remove("os_ver");
//    session_data.remove("mem");
//    session_data.remove("proc");
//    session_end_webasset.DoHTTPPost(QUrl("http://api.easevr.com/v1/client/janusvr/session_end"), session_data);
}

void MultiPlayerManager::SocketConnected()
{
    qDebug() << "MultiPlayerManager::SocketConnected()";
    if (session_tracking_enabled) {
        DoSessionStart();
    }
}

void MultiPlayerManager::SocketDisconnected()
{
    qDebug() << "MultiPlayerManager::SocketDisconnected()";
    if (session_tracking_enabled) {
        DoSessionEnd();
    }

    for (int i=0; i<connection_list.size(); ++i) {
        ServerConnection & s = connection_list[i];
        if (s.tcpsocket) {
            char readline[100000];
            while (s.tcpsocket && s.tcpsocket->canReadLine()) {
                qint64 linelength = s.tcpsocket->readLine(readline, sizeof(readline));
//                qDebug() << "MultiPlayerManager::SocketDisconnected() - Reading:" << readline;

                QJsonDocument doc = QJsonDocument::fromJson(QByteArray(readline, linelength));
                QJsonObject obj = doc.object();

                QString method = obj.value(QString("method")).toString();

                //s.last_error_msg = "";

                if (QString::compare(method, "error") == 0) {
                    DoError(obj.value("data"), s);
                }
            }

//            qDebug() << "GOT HERE";
            if (s.tcpsocket->state() != QAbstractSocket::ConnectedState) {
                s.logged_in = false;
                s.logging_in = false;
                if (s.tcpsocket->isOpen()) {
                    s.tcpsocket->close();
                }
            }
        }
    }
}

void MultiPlayerManager::SocketError()
{
    for (int i=0; i<connection_list.size(); ++i) {
        ServerConnection & s = connection_list[i];

        if (s.tcpsocket) {
            if (s.tcpsocket->error() != QAbstractSocket::UnknownSocketError) {
                const QString err_str = s.tcpsocket->errorString();
                qDebug() << "MultiPlayerManager::SocketError(): " << err_str;

                //Try SSL first!  If we get a socket error, with no reason, we can assume it's lack of support for SSL
                if (s.last_error_msg.length() <= 0) {
                    s.use_ssl = false;
                }

                s.logged_in = false;
                s.logging_in = false;
                if (s.tcpsocket->isOpen()) {
                    s.tcpsocket->close();
                }
            }
        }
    }
}

void MultiPlayerManager::SetEnabled(const bool b)
{
    if (enabled != b) {

        enabled = b;

        for (int i=0; i<connection_list.size(); ++i) {
            ServerConnection & s = connection_list[i];
            s.last_error_msg = "";
            s.logged_in = false;
            s.logging_in = false;
            s.retries = 0;
            s.use_ssl = true;
            s.sent_enterroom_url_id.clear(); //58.0 - wherever the user is, clear the last sent_enterroom so that gets resent, for party mode
        }

        enabled ? DoSocketConnect() : DoSocketDisconnect();
    }
}

bool MultiPlayerManager::GetEnabled()
{
    return enabled;
}

QString MultiPlayerManager::GetCurURL() const
{
    return cur_url;
}

void MultiPlayerManager::Update(QPointer <Player> player, const QString & url, const QList <QString> adjacent_urls, const QString & name, const bool room_allows_party_mode, const float delta_time)
{    
    if (user_ghost) {
        user_ghost->GetProperties()->SetAnimID(player->GetProperties()->GetAnimID());
        if (user_ghost->GetGhostAssetObjects().contains(user_ghost->GetProperties()->GetBodyID())) {
            QPointer <AssetObject> body_assetobj = user_ghost->GetGhostAssetObjects()[user_ghost->GetProperties()->GetBodyID()];
            if (body_assetobj && body_assetobj->GetGeom() && user_ghost->GetGhostAssetObjects().contains(user_ghost->GetProperties()->GetAnimID())) {
//                qDebug() << "MultiPlayerManager::Update" << player->GetAnimID();
                body_assetobj->GetGeom()->SetLinkAnimation(user_ghost->GetGhostAssetObjects()[user_ghost->GetProperties()->GetAnimID()]->GetGeom());
                body_assetobj->GetGeom()->SetLoop(true);
            }
        }

        player->GetProperties()->SetEyePos(user_ghost->GetProperties()->GetEyePos()->toQVector3D());
        player->GetProperties()->SetUserID(user_ghost->GetProperties()->GetID());
    }

    cur_url = url;
    cur_url_id = MathUtil::MD5Hash(url);
    cur_name = name;

    if (avatar_update_timer.elapsed() > 5000) { //42.6 NOTE!
        avatar_update_timer.restart();
        avatar_update = true;
    }

    //global network update    
    bool send_updates = false;
    const float update_time_secs = float(update_time.elapsed()) / 1000.0f;
    if (update_time_secs > RoomObject::GetRate()) {
//        qDebug() << "MultiPlayerManager::Update() - sending move packet";
        update_time.restart();
        send_updates = true;        
    }

    //do the assetrecordings (we should refactor this later so it uses same code)
    for (int i=0; i<assetrecording_list.size(); ++i) {
        QPointer <AssetRecording> a = assetrecording_list[i];
        if (a.isNull() || !a->GetProcessed()) {
            continue;
        }
        if (!a->GetPlaying()) {
            continue;
        }

        ServerConnection s;
        QList <AssetRecordingPacket> l = a->GetPackets();
        unsigned int sample_rate = a->GetProperties()->GetSampleRate();

//        qDebug() << "MultiPlayerManager::Update() recordingpackets" << l.size();
        for (int j=0; j<l.size(); ++j) {
//            qDebug() << j << l[j].pPacket;
            QJsonDocument doc = QJsonDocument::fromJson(l[j].pPacket.toUtf8());
            QJsonObject obj = doc.object();

//            qDebug() << "MultiPlayerManager::Update() recordingdoc" << obj;

            QString method = obj.value(QString("method")).toString();
//            qDebug() << "MultiPlayerManager::Update() recordingmethod" << method;

            QJsonObject dataObject = obj.value("data").toObject();
            QVariantMap m = dataObject.toVariantMap();
            m["sample_rate"] = sample_rate;

            if (QString::compare(method, "user_moved") == 0) {
                DoUserMoved(m);
//                qDebug() << "MultiPlayerManager::Update() - user_moved" << obj.value("data").toString();
            }
            else if (QString::compare(method, "user_chat") == 0) {
                DoUserChat(m);
//                qDebug() << "MultiPlayerManager::Update() - user_chat" << obj.value("data").toString();
            }
            else if (QString::compare(method, "user_portal") == 0 || QString::compare(method, "new_portal") == 0) {
                DoUserPortal(m);
            }
        }
    }

//    qDebug() << "MultiPlayerManager::Update" << enabled << url << name;    
    if (enabled) {

        UpdateConnections();

        for (int connectionIndex=0; connectionIndex<connection_list.size(); ++connectionIndex) {
            ServerConnection & s = connection_list[connectionIndex];

            const int readline_len = 1000000;
            char readline[readline_len+1];
            while (s.tcpsocket->canReadLine()) {

                qint64 linelength = s.tcpsocket->readLine(readline, readline_len);

//                qDebug() << "MultiPlayerManager::Update() - Reading:" << readline;
                if (linelength <= 0) {
                    continue;
                }               

                QByteArray b(readline, linelength);
                QJsonDocument doc = QJsonDocument::fromJson(b);
                QJsonObject obj = doc.object();

                QString method = obj.value(QString("method")).toString();
//                qDebug() << "method" << method;

                QJsonObject dataObject = obj.value("data").toObject();
                QVariantMap m = dataObject.toVariantMap();

                if (recording && recording_everyone) {
                    AssetRecordingPacket p;
                    p.SetTimeToEpoch();
                    p.pPacket = QString::fromLatin1(b).trimmed() + QString("\n");
                    p.Write(ghost_ofs);
                }

                if (QString::compare(method, "okay") == 0) {
                    if (!s.logged_in) {
                        s.logging_in = false;
                        s.logged_in = true;
                    }
                }
                else if (QString::compare(method, "user_moved") == 0) {
                    //update position of this player
                    DoUserMoved(m);
//                    qDebug() << "MultiPlayerManager::Update() - user_moved" << obj.value("data").toString();
                }
                else if (QString::compare(method, "user_chat") == 0) {
                    DoUserChat(m);
//                    qDebug() << "MultiPlayerManager::Update() - user_chat" << obj.value("data").toString();
                }
                else if (QString::compare(method, "user_portal") == 0 || QString::compare(method, "new_portal") == 0) {
                    DoUserPortal(m);
                }
                else if (QString::compare(method, "user_disconnected") == 0) {
                    DoUserDisconnected(m);
                }
                else if (QString::compare(method, "error") == 0) {
                    DoError(obj.value("data"), s);
                }
                else if (QString::compare(method, "user_leave") == 0 || QString::compare(method, "user_enter") == 0) {
                    //no-op
                }
                else {
                    qDebug() << "MultiPlayerManager::Update() - Unrecognized method in packet:" << method;
                }
            }           

            if (s.logged_in) {

                //are there any rooms on our subscribed list we didn't send the packet for, now that we're logged in?
                bool send_packet_to_server = false;
                int sendIndex = -1;

                for (int roomIndex=0; roomIndex<s.rooms.size(); ++roomIndex) {
                    if (!s.rooms[roomIndex].sent_subscribe) {
                        s.rooms[roomIndex].sent_subscribe = true;

                        const QString subscribe_packet = GetSubscribePacket(s.rooms[roomIndex].id);
                        s.tcpsocket->write(subscribe_packet.toUtf8());
                    }                   

                    if (QString::compare(s.rooms[roomIndex].id, cur_url_id) == 0) {
                        send_packet_to_server = true;
                        sendIndex = roomIndex;
                    }
                }

                if (send_packet_to_server) {
                    if (sendIndex >= 0 && cur_url_id != s.sent_enterroom_url_id) {
                        //we should broadcast we entered this room, like with reconnects
                        s.last_packet = GetEnterRoomPacket(room_allows_party_mode);
                        s.tcpsocket->write(s.last_packet.toUtf8());
                        s.sent_enterroom_url_id = cur_url_id;
                    }

                    if (chat_message.length() > 0) { //did I write a new text message?
                        s.last_packet = GetChatPacket(chat_message);
                        s.tcpsocket->write(s.last_packet.toUtf8());
                    }

                    if (send_portal.length() > 0) { //new send portal?
                        s.last_packet = GetPortalPacket(player);
                        s.tcpsocket->write(s.last_packet.toUtf8());
                    }

                    //update my position every x ms
                    if (send_updates) { //54.2 - patch to prevent server spamming
//                        qDebug() << s.update_time.elapsed() << s.rooms[sendIndex].rate;
                        s.last_packet = GetMovePacket(player);
                        s.tcpsocket->write(s.last_packet.toUtf8());
                    }
                }
            }
        }
    }

    //did we change rooms?
    if (players.contains(user_ghost->GetProperties()->GetID())) {
        //qDebug() << "user_ghost" << user_ghost->GetID() << players[user_ghost->GetID()];
        const QString last_room = players[user_ghost->GetProperties()->GetID()]->GetURL();

        if (enabled && QString::compare(last_room, cur_url_id) != 0) {
            for (int connectionListIndex=0; connectionListIndex<connection_list.size(); ++connectionListIndex) {
                for (int roomIndex=0; roomIndex<connection_list[connectionListIndex].rooms.size(); ++roomIndex) {
                    if (QString::compare(connection_list[connectionListIndex].rooms[roomIndex].id, cur_url_id) == 0) {
                        connection_list[connectionListIndex].tcpsocket->write(GetEnterRoomPacket(room_allows_party_mode).toUtf8());
                    }
                }
            }
        }

        players[user_ghost->GetProperties()->GetID()]->SetURL(cur_url_id);
    }

    //update player onPlayerEnter onPlayerExit events
    QList <QPointer <RoomObject> > current_in_room = GetPlayersInRoom(cur_url);

    //update players (and remove those who timed out)
    QList <QString> players_to_remove;
    QMap<QString, QPointer <RoomObject> >::iterator playerIndex;
    for (playerIndex = players.begin(); playerIndex != players.end(); ++playerIndex) {

        QPointer <RoomObject> p = playerIndex.value();

        if (p) {
            //release 60.0 - 30+ second timeout delay... if too short, a long avatar load may trigger the timeout, avatar is deleted, then load started, then deleted, on and on...
            if (p->GetTimeElapsed() > RoomObject::GetLogoffRate() + 30.0f) {
//                qDebug() << "MultiplayerManager::Update()  removing";
                players_to_remove.push_back(playerIndex.key());
            }
            else {

                const bool player_in_room = (QString::compare(p->GetURL(), cur_url_id) == 0);
                const bool player_in_adjacent_room = adjacent_urls.contains(p->GetURL());

//                qDebug() << "MultiplayerManager::Update() setting player" << playerIndex.key() << player->GetProperties()->GetID() << p->GetURL() << "cur_url" << cur_url << "cur_url_id" << cur_url_id << QString::number(MathUtil::hash(cur_url), 16) <<  "in room" << player_in_room << player_in_adjacent_room;
                p->SetPlayerInRoom(player_in_room);
                p->SetPlayerInAdjacentRoom(player_in_adjacent_room);

                if (!p->GetPlaying()) {
                    p->Play();
                }

                p->Update(delta_time);
                if (current_in_room.contains(p) && !players_in_room.contains(playerIndex.key())){
    //                Analytics::PostEvent("user", "joined", player->GetID());
//                    qDebug() << "MultiPlayerManager::Update() pushing back userenter" << p << p->GetProperties()->GetID();
                    on_player_enter_events.push_back(p);
                    players_in_room[playerIndex.key()] = p;
                }
            }
        }
    }

    for (QString & s : players_to_remove) {
        players.remove(s);
        if (players_in_room.contains(s)) {
            on_player_exit_events.push_back(players_in_room[s]);
            players_in_room.remove(s);
        }

    }

    //update players_in_room hash
    players_to_remove.clear();
    for (playerIndex = players_in_room.begin(); playerIndex != players_in_room.end(); ++playerIndex) {
        QPointer <RoomObject> p = playerIndex.value();
        if (!current_in_room.contains(p)){
//            Analytics::PostEvent("user","left",player->GetID());
//            qDebug() << "MultiPlayerManager::Update() pushing back userleft" << p << p->GetProperties()->GetID();
            players_to_remove.push_back(playerIndex.key());
        }
    }
    for (QString & s : players_to_remove) {
        if (players_in_room.contains(s)) {
            on_player_exit_events.push_back(players_in_room[s]);
            players_in_room.remove(s);
        }
    }   

    //do ghost recordings for self, regardless of multiplayermanager being enabled or not
    if (recording) {
        AssetRecordingPacket packet;
        packet.SetTimeToEpoch();

        if (chat_message.length() > 0) { //did I write a new text message?
            packet.pPacket = GetUserChatPacket(chat_message);
            ghost_packets.push_back(packet);
        }

        if (send_portal.length() > 0) { //new send portal?
            packet.pPacket = GetUserPortalPacket(player);
            ghost_packets.push_back(packet);
        }

        if (send_updates) {
            mic_buffers = ghost_recording_mic_buffers;
            packet.pPacket = GetUserMovePacket(player);
            ghost_packets.push_back(packet);
            ghost_recording_mic_buffers.clear();
        }

        //write the packets out to the file
        for (int i=0; i<ghost_packets.size(); ++i) {
            ghost_packets[i].Write(ghost_ofs);
        }
        ghost_packets.clear();
    }    

    //49.8 - zero out things that just changed (note these send IMMEDIATELY so 1 pass is fine)
    chat_message.clear();
    send_portal.clear();
    if (send_updates) {
        //everything part of user_moved events that should only be cleared when sent
        pending_pos = false;
        room_edit_assets.clear();
        room_edit_objs.clear();
        MathUtil::room_delete_code.clear();
        mic_buffers.clear();
        avatar_update = false;
    }
}

void MultiPlayerManager::SetURLToDraw(const QString & s)
{
    url_to_draw = s;
}

void MultiPlayerManager::DrawCursorsGL(QPointer <AssetShader> shader)
{
    //QString short_url_to_draw = url_to_draw.left(url_maxlen);
    QString url_to_draw_md5 = MathUtil::MD5Hash(url_to_draw);

    //draw players' cursors
    QMap<QString, QPointer <RoomObject> >::iterator i;
    for (i = players.begin(); i != players.end(); ++i) {
        QPointer <RoomObject> player = i.value();
        if (player && QString::compare(player->GetURL(), url_to_draw_md5) == 0) {
            player->DrawCursorGL(shader);
        }
    }
}

void MultiPlayerManager::DrawGL(QPointer <AssetShader> shader, const QVector3D & player_pos, const bool render_left_eye)
{
    //QString short_url_to_draw = url_to_draw.left(url_maxlen);
    QString url_to_draw_md5 = MathUtil::MD5Hash(url_to_draw);

    //draw players
    QMap<QString, QPointer <RoomObject> >::iterator i;
    for (i = players.begin(); i != players.end(); ++i) {
        QPointer <RoomObject> player = i.value();
        if (player && QString::compare(player->GetURL(), url_to_draw_md5) == 0) {
            player->DrawGL(shader, render_left_eye, player_pos);
        }
    }
}

void MultiPlayerManager::SetAvatarFromGhost(QPointer <RoomObject> new_ghost)
{
//    qDebug() << "MultiPlayerManager::SetAvatarFromGhost()";
    if (user_ghost && new_ghost) {
        user_ghost->SetGhostAssetObjects(new_ghost->GetGhostAssetObjects());
        user_ghost->SetChildObjects(new_ghost->GetChildObjects());

        user_ghost->GetProperties()->SetHeadID(new_ghost->GetProperties()->GetHeadID());
        user_ghost->GetProperties()->SetBodyID(new_ghost->GetProperties()->GetBodyID());
        user_ghost->SetHeadAvatarPos(new_ghost->GetHeadAvatarPos());
        user_ghost->GetProperties()->SetCullFace(new_ghost->GetProperties()->GetCullFace());
        user_ghost->GetProperties()->SetColour(new_ghost->GetProperties()->GetColour()->toQVector4D());
        user_ghost->GetProperties()->SetScale(new_ghost->GetProperties()->GetScale()->toQVector3D());
        user_ghost->GetProperties()->SetLighting(new_ghost->GetProperties()->GetLighting());
        user_ghost->GetProperties()->SetEyePos(new_ghost->GetProperties()->GetEyePos()->toQVector3D());

        DoUpdateAvatar();
    }
}

void MultiPlayerManager::AddChatMessage(const QString & s, const QColor col)
{
    chat_messages_log.push_back(QPair<QString, QColor>(s.left(255), col));
    //qDebug() << "MultiPlayerManager::AddChatMessage() - " << chat_messages_pos << chat_messages_log.size() << chat_messages_log;
}

void MultiPlayerManager::SetCursorPosition(const QVector3D & cpos, const QVector3D & cxdir, const QVector3D & cydir, const QVector3D & czdir, const float cscale)
{
    cpos_list = cpos;
    cxdir_list = cxdir;
    cydir_list = cydir;
    czdir_list = czdir;
    cscale_list = cscale;
    pending_pos = true;
}

void MultiPlayerManager::AddMicBuffers(const QList <QByteArray> buffers)
{
    mic_buffers += buffers;
    if (recording) {
        ghost_recording_mic_buffers += buffers;
    }
}

int MultiPlayerManager::GetChatMessagesSize() const
{
    return chat_messages_log.size();
}

void MultiPlayerManager::SetNewUserID(const QString id, const bool append_random_number)
{
    QString userid = id;

    if (append_random_number) {
        //determine if last part of name after underscore is numeric, remove it if so
        const int ind = userid.lastIndexOf("_");
        const QString last_part = userid.right(userid.length()-ind-1);
        bool ok = true;
        last_part.toInt(&ok);
        if (ok) {
            userid = userid.left(ind); //remove previous _XXXX number
        }

        //append a new numeric random ID
        userid += QString("_") + QString::number(qrand() % 10000); //create a new final _XXXX number
    }

    user_ghost->GetProperties()->SetID(userid);
    UpdateAvatarData();
    SaveAvatarData();

    SetEnabled(false);
    SetEnabled(true);
}

void MultiPlayerManager::SetCustomPortalShader(const QString shader_src)
{
    QHash <QString, QPointer <AssetShader> > ghost_assetshaders = user_ghost->GetGhostAssetShaders();

    if (shader_src.isEmpty()) {
        ghost_assetshaders.remove("CustomPortalShader");
    }
    else {
        QPointer <AssetShader> s = new AssetShader();
        s->SetSrc(MathUtil::GetApplicationURL(), shader_src, "");
        s->GetProperties()->SetID("CustomPortalShader");
        ghost_assetshaders["CustomPortalShader"] = s;
    }

    user_ghost->SetGhostAssetShaders(ghost_assetshaders);
    UpdateAvatarData();
    SaveAvatarData();
}

QString MultiPlayerManager::GetCustomPortalShader() const
{
    QHash <QString, QPointer <AssetShader> > ghost_assetshaders = user_ghost->GetGhostAssetShaders();
    if (ghost_assetshaders.contains("CustomPortalShader") && ghost_assetshaders["CustomPortalShader"]) {
        return ghost_assetshaders["CustomPortalShader"]->GetProperties()->GetSrcURL();
    }

    return QString();
}

bool MultiPlayerManager::SetChatMessage(QPointer <Player> player, const QString & s)
{   

    //change user id if we detect it    
    QStringList message_list = s.split(" ");

    QString s2;
    if (!message_list.isEmpty()) {
        s2 = message_list[0].toLower();
    }

    //see if userid changed
    if (s2.left(1) == "#") {
        if (s2 == "#userid" && message_list.size() >= 2) {
            SetNewUserID(message_list[1], false);
        }
        else if (s2 == "#server" && message_list.size() >= 2) {
            QString server;
            int port = 0;

            if (message_list[1].contains(":")) {
                server = message_list[1].left(message_list[1].indexOf(":"));
                port = message_list[1].right(message_list[1].length() - message_list[1].indexOf(":")-1).toInt();
            }
            else {
                server = message_list[1];
            }

            SettingsManager::SetServer(server);
            if (port > 0) {
                SettingsManager::SetPort(port);
            }
            for (int i=0; i<connection_list.size(); ++i) {
                connection_list[i].tcpserver = server;
                if (port > 0) {
                    connection_list[i].tcpport = port;
                }
            }

            SetEnabled(false);
            SetEnabled(true);
        }
        else if (s2 == "#port" && message_list.size() >= 2) {
            const int port = message_list[1].toInt();
            SettingsManager::SetPort(port);
            for (int i=0; i<connection_list.size(); ++i) {
                connection_list[i].tcpport = port;
            }

            SetEnabled(false);
            SetEnabled(true);
        }
        else if (s2 == "#clearchat") {
            chat_messages_log.clear();
        }      
        else if (s2 == "#col" || s2 == "#color" || s2 == "#colour") {
            if (message_list.size() == 4) {
                user_ghost->GetProperties()->SetColour(MathUtil::GetColourAsVector4(MathUtil::GetStringAsColour(message_list[1] + " " + message_list[2] + " " + message_list[3])));
                UpdateAvatarData();
                SaveAvatarData();
            }
            else if (message_list.size() == 2) {
                user_ghost->GetProperties()->SetColour(MathUtil::GetColourAsVector4(MathUtil::GetStringAsColour(message_list[1])));
                UpdateAvatarData();
                SaveAvatarData();
            }
        }       
        else if (s2 == "#pos" && message_list.size() >= 4) {
            player->GetProperties()->SetPos(QVector3D(message_list[1].toFloat(), message_list[2].toFloat(), message_list[3].toFloat()));
        }
        else if (s2 == "#head_pos" && message_list.size() >= 4) {
            const QVector3D head_pos(message_list[1].toFloat(), message_list[2].toFloat(), message_list[3].toFloat());
            user_ghost->SetHeadAvatarPos(head_pos);
            //player->SetHeadPos(head_pos);
            UpdateAvatarData();
            SaveAvatarData();
        }    
        else if (s2 == "#scale" && message_list.size() >= 4) {
            user_ghost->GetProperties()->SetScale(QVector3D(message_list[1].toFloat(), message_list[2].toFloat(), message_list[3].toFloat()));
            UpdateAvatarData();
            SaveAvatarData();
        }
        else if (s2 == "#lighting" && message_list.size() >= 2) {
            user_ghost->GetProperties()->SetLighting(message_list[1].toLower() == "true");
            UpdateAvatarData();
            SaveAvatarData();
        }       
        else if (s2 == "#cull_face" && message_list.size() >= 2) {
            user_ghost->GetProperties()->SetCullFace(message_list[1]);
            UpdateAvatarData();
            SaveAvatarData();
        }       
        else if (s2 == "#head_src" && message_list.size() >= 2) {
            SetHeadSrc(message_list[1]);
            DoUpdateAvatar();
        }
        else if (s2 == "#body_src" && message_list.size() >= 2) {
            SetBodySrc(message_list[1]);
            DoUpdateAvatar();
        }       
        else if (s2 == "#head_mtl" && message_list.size() >= 2) {
            SetHeadMtl(message_list[1]);
            DoUpdateAvatar();
        }
        else if (s2 == "#body_mtl" && message_list.size() >= 2) {
            SetBodyMtl(message_list[1]);
            DoUpdateAvatar();
        }

        return false;
    }
    else {
        //qDebug() << "MultiPlayerManager::SetTextMessage() " << s;
//        Analytics::PostEvent("player", "chat");
        chat_message = s.toHtmlEscaped(); //60.0 adds escape characters for double quotes, etc., so they send properly
        AddChatMessage(user_ghost->GetProperties()->GetID() + QString(" ") + s, QColor(64,192,64));
        return true;
    }
}

void MultiPlayerManager::SetSendPortal(const QString & s, const QString & js_id)
{
    send_portal = s;
    send_portal_jsid = js_id;   
}

void MultiPlayerManager::SetHMD(const QString & s)
{
    hmd = s;
}

void MultiPlayerManager::SetRoomAssetEdit(const QString s)
{
    room_edit_assets += MathUtil::EncodeString(s);
}

void MultiPlayerManager::SetRoomEdit(const QPointer <RoomObject> o)
{
    if (!room_edit_objs.contains(o)) {
        room_edit_objs.append(o);
    }
}

void MultiPlayerManager::SetRoomDeleteCode(const QString & s)
{
//    qDebug() << "MultiPlayerManager::SetRoomDeleteCode(const QString & s)" << s;
    MathUtil::room_delete_code += s;
}

QString MultiPlayerManager::GetUserID() const
{
    return user_ghost->GetProperties()->GetID();
}

QList <QPointer <RoomObject> > MultiPlayerManager::GetPlayersInRoom(const QString & url)
{
    const QString url_md5 = MathUtil::MD5Hash(url);
    QList <QPointer <RoomObject> > ps;
    QMap <QString, QPointer <RoomObject> >::iterator it;
    for (it=players.begin(); it!=players.end(); ++it) {
        QPointer <RoomObject> player = it.value();
        if (player && QString::compare(player->GetURL(), url_md5) == 0) {
//            qDebug() << "MultiPlayerManager::GetPlayersInRoom()" << it.value()->GetID() << it.value()->GetTimeElapsed();
            ps.push_back(player);
        }
    }
    return ps;
}

QMap <QString, DOMNode *> MultiPlayerManager::GetPlayersInRoomDOMNodeMap(const QString & url)
{
    const QString url_md5 = MathUtil::MD5Hash(url);
    QMap <QString, DOMNode *> ps;
    QMap <QString, QPointer <RoomObject> >::iterator it;
    for (it=players.begin(); it!=players.end(); ++it) {
        QPointer <RoomObject> player = it.value();
        if (player && player->GetProperties() && QString::compare(player->GetURL(), url_md5) == 0) {
//            qDebug() << "MultiPlayerManager::GetPlayersInRoomDOMNodeMap()" << player->GetProperties()->GetID(); //<< it.value()->GetTimeElapsed();
            ps[player->GetProperties()->GetID()] = player->GetProperties().data();
        }
    }
    return ps;
}

QPointer <RoomObject> MultiPlayerManager::GetPlayer()
{
    return user_ghost;
}

QList <QPair <QString, QColor> > MultiPlayerManager::GetNewChatMessages()
{
    QList <QPair <QString, QColor> > new_msgs = chat_messages_log;
    chat_messages_log.clear();
    return new_msgs;
}

QString MultiPlayerManager::GetLogonPacket(const QString & userId, const QString & password, const QString & url_id)
{
    QString s = "{\"method\":\"logon\",\"data\":{";
    s += "\"userId\":\"" + userId + "\", ";
    if (password.length() > 0) {
        s += "\"password\":\"" + password + "\", ";
    }
    s += "\"version\":\"" + QString(__JANUS_VERSION) + "\", ";
    s += "\"roomId\":\"" + url_id + "\"";    
    s += "}}\n";
//    qDebug() << s;
    return s;
}



QString MultiPlayerManager::GetSubscribePacket(const QString & url_id)
{
    //When you wish to start receiving events about a room (you are in that room or looking through a portal)
    //{"method":"subscribe", "data": { "roomId": "345678354764987457" }}
//    qDebug() << "MultiPlayerManager::GetSubscribePacket" << url_id;
    QString s = QString("{\"method\":\"subscribe\", \"data\":{\"roomId\":\"" + url_id + "\"");
    s += "}}\n";
//    qDebug() << s;
    return s;
}

QString MultiPlayerManager::GetMovePacket(QPointer <Player> player)
{
    QString packet = "{\"method\":\"move\", \"data\":{";

    packet += GetMovePacket_Helper(player);

    packet += "}}\n";
    return packet;
}

QString MultiPlayerManager::GetMovePacket_Helper(QPointer <Player> player)
{
    QString packet;
    packet += "\"pos\":" + MathUtil::GetVectorAsString(player->GetProperties()->GetPos()->toQVector3D());
    packet += ",\"dir\":" + MathUtil::GetVectorAsString(player->GetProperties()->GetDir()->toQVector3D());
    packet += ",\"view_dir\":" + MathUtil::GetVectorAsString(player->GetProperties()->GetViewDir()->toQVector3D());
    packet += ",\"up_dir\":" + MathUtil::GetVectorAsString(player->GetProperties()->GetUpDir()->toQVector3D());

    if (player->GetHMDType().length() > 0) {
        packet += ",\"hmd_type\":\"" + player->GetHMDType() + "\"";
    }

    packet += ",\"head_pos\":" + MathUtil::GetVectorAsString(player->GetProperties()->GetLocalHeadPos()->toQVector3D());
    packet += ",\"anim_id\":\"" + player->GetProperties()->GetAnimID() + "\"";

    if (pending_pos == true) {
        packet += ",\"cpos\":" + MathUtil::GetVectorAsString(cpos_list);
        packet += ",\"cxdir\":" + MathUtil::GetVectorAsString(cxdir_list);
        packet += ",\"cydir\":" + MathUtil::GetVectorAsString(cydir_list);
        packet += ",\"czdir\":" + MathUtil::GetVectorAsString(czdir_list);
        packet += ",\"cscale\":" + MathUtil::GetFloatAsString(cscale_list);
    }

    if (player->GetSpeaking()) {
        packet += ",\"speaking\":\"true\"";
    }

    if (!mic_buffers.isEmpty()) {

//        QByteArray buffer;
//        for (int i=0; i<mic_buffers.size(); ++i) {
//            buffer += mic_buffers[i];
//        }
//        QByteArray encoded = AudioUtil::encode(buffer);

//        QByteArray encoded;
//        for (int i=0; i<mic_buffers.size(); ++i) {
//            encoded += AudioUtil::encode(mic_buffers[i]);
//        }
//        QByteArray encoded = AudioUtil::encode(buffer);

//        qDebug() << "coding" << buffer.size() << encoded.size();
//        packet += ",\"audio_opus\":\"" + encoded.toBase64() + "\"";
//        packet += ",\"audio\":\"" + buffer.toBase64() + "\""; //Old

        for (int i=0; i<mic_buffers.size(); ++i) {
//            qDebug() << " sending packet" << "audio_opus"+QString::number(i) << "bytes" << mic_buffers[i].size();
            packet += ",\"audio_opus"+QString::number(i)+ "\":\"" + mic_buffers[i] + "\"";
        }
        packet += ",\"sound_level\":" + MathUtil::GetFloatAsString(SoundManager::GetMicLevel());
    }

    //56.0 - for security reasons, do not send hand-tracking data when player is in typing/text entry states
    if (player->GetTyping() || player->GetEnteringText()) {
        packet += ",\"typing\":\"true\"";
    }
    else {
        //add a leap motion packet (if there's activity)
        if (player->GetHand(0).is_active) {
            packet += ",\"hand0\":" + player->GetHand(0).GetJSON();
        }

        //add a leap motion packet (if there's activity)
        if (player->GetHand(1).is_active) {
            packet += ",\"hand1\":" +  player->GetHand(1).GetJSON();
        }
    }

    //add an avatar packet
    if (avatar_update) {
        packet += ",\"avatar\":\"" + avatar_data_encoded + "\"";

    }

    //send room edit packet
    if (!room_edit_objs.isEmpty() || !room_edit_assets.isEmpty()) {
        //room edit assets
        packet += ",\"room_edit\":\"";
        packet += room_edit_assets;

        //room edit objects
        for (int i=0; i<room_edit_objs.size(); ++i) {
            if (room_edit_objs[i]) {
                 //packet += MathUtil::EncodeString(room_edit_objs[i]->GetXMLCode(true));
                packet += MathUtil::EncodeString(room_edit_objs[i]->GetXMLCode());
            }
        }

        //close part of packet
        packet += "\"";
//        qDebug() << "send packet" << packet;
    }

    //send room delete packet
    if (MathUtil::room_delete_code.length() > 0) {
        packet += ",\"room_delete\":\"" + MathUtil::EncodeString(MathUtil::room_delete_code) + "\"";
    }

    return packet;
}

QString MultiPlayerManager::GetUnsubscribePacket(const QString & url_id)
{
    //When you wish to start receiving events about a room (you are in that room or looking through a portal)
    //{"method":"subscribe", "data": { "roomId": "345678354764987457" }}
//    qDebug() << "MultiPlayerManager::GetUnsubscribePacket" << url_id;
    return QString("{\"method\":\"unsubscribe\", \"data\":{\"roomId\":\"" + url_id + "\"}}\n");
}

QString MultiPlayerManager::GetEnterRoomPacket(const bool room_allows_party_mode)
{
    //When you pass through a portal:
    //{"method":"enter_room", "data": { "roomId": "345678354764987457" }}
//    qDebug() << "MultiPlayerManager::GetEnterRoomPacket" << cur_url_id << "party_mode enabled for room?" << room_allows_party_mode;
    QString s = "{\"method\":\"enter_room\", \"data\":{";
    s += "\"roomId\":\"" + cur_url_id + "\"";    
    if (partymode) {
        s += ", \"roomUrl\":\"" + (room_allows_party_mode ? cur_url : QString()) + "\"";
        s += ", \"roomName\":\"" + (room_allows_party_mode ? cur_name : QString()) + "\"";
        s += ", \"partyMode\":\"true\"";
    }
    else {
        s += ", \"partyMode\":\"false\"";
    }
    s += "}}\n";
//    qDebug() << s;
    return s;
}

//emulates a processed chat packet received from the server
QString MultiPlayerManager::GetUserChatPacket(const QString & chat_message)
{
    //{"method":"user_chat","data":{"roomId":"29621ebc54db600dd06fb6d2b0b76055",
    //                              "userId":"Mecha_Cloudy_james",
    //                              "message":{"data":"acknowledge","_userId":"Mecha_Cloudy_james","_userList":[]}}}
    QString packet = "{\"method\":\"user_chat\",\"data\":{\"roomId\":\"";
    packet += cur_url_id;
    packet += "\",\"userId\":\"";
    packet += user_ghost->GetProperties()->GetID();
    packet += "\",\"message\":{\"data\":\"";
    packet += chat_message;
//    packet += "\",\"_userList\":[]}}}\n";
    packet += "\"}}}\n";
    return packet;
}

//emulates a processed show portal packet received from server
QString MultiPlayerManager::GetUserPortalPacket(QPointer <Player> player)
{
    //{"method":"user_portal","data":{"roomId":"29621ebc54db600dd06fb6d2b0b76055",
    //"userId":"Mecha_Cloudy_james",
    //"url":"http://google.com",
    //"pos":"0.17338 0 1.21804",
    //"fwd":"-0.561532 0 0.72741"}}
    const QVector3D d = -QVector3D(player->GetProperties()->GetDir()->toQVector3D().x(),
                                   0.0f,
                                   player->GetProperties()->GetDir()->toQVector3D().z());
    const QVector3D p = player->GetProperties()->GetPos()->toQVector3D()- d * 1.5f;

    QString packet = "{\"method\":\"user_portal\",\"data\":{\"roomId\":\"";
    packet += cur_url_id;
    packet += "\",\"userId\":\"";
    packet += user_ghost->GetProperties()->GetID();
    packet += "\",\"url\":\"";
    packet += send_portal;
    packet += "\",\"pos\":" + MathUtil::GetVectorAsString(p);
    packet += "\",\"fwd\":" + MathUtil::GetVectorAsString(d);
    packet += "\"}}\n";
    return packet;
}

//emulates a move packet received from server
QString MultiPlayerManager::GetUserMovePacket(QPointer <Player> player)
{
    //{"method":"user_moved","data":{"roomId":"29621ebc54db600dd06fb6d2b0b76055","userId":"Mecha_Cloudy_james","position":{"pos":"-0.264996 0 1.56646","dir":"0.728868 -0.259443 -0.633593","view_dir":"0.726968 -0.268641 -0.631941","up_dir":"0.202747 0.96324 -0.176244","hmd_type":"2d","head_pos":"0 0 0","anim_id":"run","cpos":"0.131624 1.453434 1.221689","cxdir":"0.791578 0 0.611068","cydir":"0 1 0","czdir":"-0.611068 0 0.791578","cscale":"0.043646","_userId":"Mecha_Cloudy_james","_userList":[]}}}
    QString packet = "{\"method\":\"user_moved\",\"data\":{\"roomId\":\"";
    packet += cur_url_id;
    packet += "\",\"userId\":\"";
    packet += user_ghost->GetProperties()->GetID();
    packet += "\",\"position\":{";
    packet += GetMovePacket_Helper(player);
    packet += "}}}\n";
    return packet;
}

QString MultiPlayerManager::GetChatPacket(const QString & chat_message)
{
    //Note: for a PM this packs the message and toUserId, treating data as a tuple,
    //      but for public messages data is set to the message string directly,
    //      in order to preserve backward compatibility
    QString packet = "{\"method\":\"chat\", \"data\":";

    if (chat_message.startsWith("@")) {
        QStringList messagelist = chat_message.split(" ");
        QString touserid = messagelist.first();
        touserid.remove(0, 1);
        messagelist.pop_front();
        QString message = messagelist.join(" ");

        packet += "{\"toUserId\": \"" + touserid + "\",";
        packet += "\"message\": \"" + message + "\"}}\n";
    }
    else {
        packet += "\"" + chat_message + "\"}\n";
    }

    return packet;
}

QString MultiPlayerManager::GetPortalPacket(QPointer <Player> player)
{
    if (user_ghost) {
        user_ghost->GetProperties()->SetAnimID("portal");
    }

    //{"method":"portal", "data":{"url":"http://...", "pos":[1,2,4], "fwd":[0,1,0]}}
    const QVector3D d = -QVector3D(player->GetProperties()->GetDir()->toQVector3D().x(),
                                   0.0f,
                                   player->GetProperties()->GetDir()->toQVector3D().z());
    const QVector3D p = player->GetProperties()->GetPos()->toQVector3D() - d * 1.5f;

    QString packet = "{\"method\":\"portal\", \"data\":{\"url\":\"" + send_portal + "\", \"pos\":";
    packet += MathUtil::GetVectorAsString(p);
    packet += ", \"fwd\":";
    packet += MathUtil::GetVectorAsString(d);
    packet += "}}\n";

    //qDebug() << "MultiPlayerManager::GetPortalPacket() - " << packet;
    return packet;
}

void MultiPlayerManager::DoError(const QJsonValue & v, ServerConnection & s)
{
    QJsonObject o = v.toObject();
    if (o != QJsonObject()) {
        s.last_error_msg = o.value("message").toString();
    }
    else {
        s.last_error_msg = v.toString();
    }

    if (s.last_error_msg.left(4).toLower() == "user") {
        SetNewUserID(GetUserID(), true);
    }
    qDebug() << "MultiPlayerManager::DoError() - " << s.last_error_msg << "Packet: " << s.last_packet;
}

void MultiPlayerManager::DoUserMoved(const QVariantMap & m)
{
    //{"method":"user_portal", "data":{"userId":"LL","roomId":"345678354764987457","url":"http://...", "pos":[0,0,0], "fwd":[0,1,0]}}
    const QString userid = m["userId"].toString();
    const QString roomid = m["roomId"].toString();

//    qDebug() << "MultiPlayerManager::DoUserMoved() " << userid;
    QPointer <RoomObject> cur_player;
    if (players.contains(userid) && players[userid]) {
        cur_player = players[userid];
    }
    else {
        cur_player = new RoomObject();
//        qDebug() << "MULTI NEW PLAYER!" << cur_player << userid << roomid;
        cur_player->SetType(TYPE_GHOST);
        cur_player->GetProperties()->SetID(userid);
        cur_player->GetProperties()->SetLoop(false);
        cur_player->SetDoMultiplayerTimeout(true);
        cur_player->GetProperties()->SetVisible(true);
        cur_player->GetProperties()->SetScale(QVector3D(1.5f, 1.5f, 1.5f)); //this sets the "size"
        AddChatMessage(userid + " is nearby.", QColor(32, 32, 32)); //add message to chat log
        players[userid] = cur_player;
    }

    cur_player->DoGhostMoved(m);
}

void MultiPlayerManager::DoUserPortal(const QVariantMap & m)
{
    //{"method":"user_portal", "data":{"userId":"LL","roomId":"345678354764987457","url":"http://...", "pos":[0,0,0], "fwd":[0,1,0]}}
    QString userid = m["userId"].toString();
    QString roomid = m["roomId"].toString();
    QString url = m["url"].toString();
    QString js_id = userid + QString("-") + url;
    QStringList pos = m["pos"].toString().split(" ");
    QStringList fwd = m["fwd"].toString().split(" ");

    //qDebug() << "MultiPlayerManager::DoUserPortal()" << userid << roomid << url << js_id << pos << fwd;
    QVector3D p(0,0,0);
    QVector3D f(0,0,1);

    if (pos.size() == 3) {
        p = QVector3D(pos[0].toFloat(), pos[1].toFloat(), pos[2].toFloat());
    }
    if (fwd.size() == 3) {
        f = QVector3D(fwd[0].toFloat(), fwd[1].toFloat(), fwd[2].toFloat());
    }

    if (QString::compare(cur_url_id, roomid) == 0 && players.contains(userid) && !players[userid].isNull()) { //we are in the same place as the portal spawner
        players[userid]->GetSendPortalURL().push_back(url);
        players[userid]->GetSendPortalJSID().push_back(js_id);
        players[userid]->GetSendPortalPos().push_back(p);
        players[userid]->GetSendPortalFwd().push_back(f);
    }
}

void MultiPlayerManager::DoUserDisconnected(const QVariantMap & )
{
    //no-op... user will just time out after 10 seconds
//    const QString userid = m["userId"].toString();
//    if (players.contains(userid) && players[usreid]) {

//    }
}

void MultiPlayerManager::DoUserChat(const QVariantMap & m)
{
//    qDebug() << "MultiPlayerManager::DoUserChat()" << m;
    //{"method":"user_chat", "data":{"message":"The message", "userId":"LL"}}
    const QString userid = m["userId"].toString();
    const QString touserid = m["toUserId"].toString();
    QString msg;
    if (m["message"].toMap().contains("data")) {
        msg = m["message"].toMap()["data"].toString();
    }
    else {
        msg = m["message"].toString();
    }

    if (players.contains(userid) && !players[userid].isNull()) {
        if (touserid.length() > 0) { //private message (includes toUserId Property)
            AddChatMessage(players[userid]->GetProperties()->GetID() + QString(" ") + msg, QColor(64,64,192));
        }
        else { //public message (does not include "toUserId" Property)
            AddChatMessage(players[userid]->GetProperties()->GetID() + QString(" ") + msg, QColor(128,128,128));
        }

        players[userid]->SetChatMessage(msg);
    }
}

void MultiPlayerManager::LoadAvatarData(const bool load_userid)
{   
    //57.0 - reconstruct the whole ghost, so defaults don't hang around
//    if (user_ghost) {
//        delete user_ghost;
//    }

    user_ghost->SetType(TYPE_GHOST);

    QPointer <AssetGhost> user_ghost_asset = new AssetGhost();    
    user_ghost->SetAssetGhost(user_ghost_asset);
    user_ghost->Play();

    if (SettingsManager::GetDemoModeEnabled() && !SettingsManager::GetDemoModeAvatar()) {
        return;
    }

    QFile file(userid_filename);
    if (!load_userid || !file.open( QIODevice::ReadOnly | QIODevice::Text )) {
        //if we need to reset avatar (!load_userid) or we couldn't load the userid.txt file...
        qsrand(QDateTime::currentMSecsSinceEpoch() % 1000000);

        QList <QString> avatar_names;
        avatar_names.push_back("Beta");

        QList <QString> anim_names;
        anim_names.push_back("fly");
        anim_names.push_back("idle");
        anim_names.push_back("jump");
        anim_names.push_back("portal");
        anim_names.push_back("run");
        anim_names.push_back("speak");
        anim_names.push_back("type");
        anim_names.push_back("walk");
        anim_names.push_back("walk_back");
        anim_names.push_back("walk_left");
        anim_names.push_back("walk_right");

        const int character = qrand() % avatar_names.size();
        const QString base_url = "http://www.janusvr.com/avatars/animated/";

        QPointer <AssetObject> body_obj = new AssetObject();
        body_obj->GetProperties()->SetID("body");
        body_obj->SetSrc(base_url, base_url + avatar_names[character] + "/Beta.fbx.gz");       
//        body_obj->Load();

        //set the body
        QHash <QString, QPointer <AssetObject> > asset_obj_list;
        asset_obj_list["body"] = body_obj;

        //set the animations
        for (int i=0; i<anim_names.size(); ++i) {
            QPointer <AssetObject> anim_obj(new AssetObject());
            anim_obj->GetProperties()->SetID(anim_names[i]);
            anim_obj->SetSrc(base_url, base_url + avatar_names[character] + "/" + anim_names[i] + ".fbx.gz");
//            anim_obj->Load();
            asset_obj_list[anim_names[i]] = anim_obj;
        }

        //set the initial avatar properties
        QString new_id = GenerateUserID();
        user_ghost->SetType(TYPE_GHOST);
        user_ghost->GetProperties()->SetID(new_id);
        user_ghost->GetProperties()->SetScale(QVector3D(0.0095f, 0.0095f, 0.0095f));
        user_ghost->GetProperties()->SetColour(MathUtil::GetColourAsVector4(GenerateColour()));
        user_ghost->GetProperties()->SetBodyID("body");
        user_ghost->SetHeadAvatarPos(QVector3D(0, 0, 0));  
        user_ghost->SetGhostAssetObjects(asset_obj_list);        
        user_ghost->GetChildObjects().clear();
        user_ghost->GetProperties()->SetLighting(true);

        UpdateAvatarData();
        SaveAvatarData();
    }
    else {
        //set base path (for loading resources in same dir)        
        QTextStream ifs(&file);
        SetAvatarData(ifs.readAll());
    }

    if (file.isOpen()) {
        file.close();
    }

    //load the avatar, and set player-specific properties
    //refactor TODO: user_ghost and player object seem redundant
    user_ghost->LoadGhost(avatar_data);
}

void MultiPlayerManager::SetHeadSrc(const QString & s)
{
    if (s.length() > 0) {
        QPointer <AssetObject> new_asset_obj(new AssetObject());
        new_asset_obj->SetSrc("", s);
        new_asset_obj->GetProperties()->SetID("head_id");

        user_ghost->GetGhostAssetObjects()["head_id"] = new_asset_obj;
        user_ghost->GetProperties()->SetHeadID("head_id");
    }
    else {
        user_ghost->GetGhostAssetObjects()["head_id"] = 0;
        user_ghost->GetProperties()->SetHeadID("head_id");
    }
}

void MultiPlayerManager::SetHeadMtl(const QString & s)
{
    QPointer <AssetObject> a = user_ghost->GetGhostAssetObjects()[user_ghost->GetProperties()->GetHeadID()];
    if (a) {
        a->SetMTLFile(s);
    }
}

void MultiPlayerManager::SetBodySrc(const QString & s)
{
    if (s.length() > 0) {
        QPointer <AssetObject> new_asset_obj(new AssetObject());
        new_asset_obj->SetSrc("", s);
        new_asset_obj->GetProperties()->SetID("body_id");

        user_ghost->GetGhostAssetObjects()["body_id"] = new_asset_obj;
        user_ghost->GetProperties()->SetBodyID("body_id");
    }
    else {
        user_ghost->GetGhostAssetObjects()["body_id"].clear();
        user_ghost->GetProperties()->SetBodyID("body_id");
    }
}

void MultiPlayerManager::SetBodyMtl(const QString & s)
{    
    const QPointer <AssetObject> a = user_ghost->GetGhostAssetObjects()[user_ghost->GetProperties()->GetBodyID()];
    if (a) {
        a->SetMTLFile(s);
    }
}

void MultiPlayerManager::DoUpdateAvatar()
{
    UpdateAvatarData();
    SaveAvatarData();
    user_ghost->LoadGhost(avatar_data);    
}

void MultiPlayerManager::SetAvatarData(const QString s)
{
    avatar_data = s;
    avatar_data_encoded = MathUtil::EncodeString(avatar_data);
}

QString MultiPlayerManager::GetAvatarData() const
{
    return avatar_data;
}

void MultiPlayerManager::UpdateAvatarData()
{
    QHash <QString, unsigned int>::iterator i;
    QString s;    
    QTextStream ofs(&s);

    ofs << "<FireBoxRoom>\n";
    ofs << "<assets>\n";
    //write ghost's assetobjects    
    for (QPointer <AssetShader> & s : user_ghost->GetGhostAssetShaders()) {
        if (s) {
            ofs << s->GetXMLCode() << "\n";
        }
    }
    //write ghost's assetobjects    
    for (QPointer <AssetObject> & o : user_ghost->GetGhostAssetObjects()) {
        if (o) {
            ofs << o->GetXMLCode() << "\n";
        }
    }
    ofs << "</assets>\n";

    //write ghost
    ofs << "<room>\n";
    ofs << user_ghost->GetXMLCode(false) << "\n";
    ofs << "</room>\n";
    ofs << "</FireBoxRoom>\n";
    ofs.flush();

    SetAvatarData(s);
}

void MultiPlayerManager::SaveAvatarData()
{   
    QFile::rename(userid_filename, userid_backup_filename + MathUtil::GetCurrentDateTimeAsString() + ".txt");   

    QFile file(userid_filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "MultiPlayerManager::SaveUserID() - saving";
        QTextStream ofs(&file);
        ofs << avatar_data;
        file.close();
    }
    else {
        qDebug() << "MultiPlayerManager::SaveUserID() - could not save " << userid_filename;
    }
}

QString MultiPlayerManager::GenerateUserID()
{

    QString adjs[108] = {
        "Big",
        "Wild",
        "Rough",
        "Happy",
        "Sad",
        "Ludicrous",
        "Engaged",
        "Thoughtful",
        "Sincere",
        "Crazy",
        "Fun",
        "Mild",
        "Hot",
        "Lazy",
        "Excited",
        "Stern",
        "Agreeable",
        "Brave",
        "Calm",
        "Delightful",
        "Eager",
        "Faithful",
        "Gentle",
        "Jolly",
        "Kind",
        "Lively",
        "Nice",
        "Obedient",
        "Proud",
        "Relieved",
        "Silly",
        "Thankful",
        "Victorious",
        "Witty",
        "Zealous",
        "Broad",
        "Chubby",
        "Crooked",
        "Curved",
        "Deep",
        "Flat",
        "High",
        "Hollow",
        "Low",
        "Narrow",
        "Round",
        "Shallow",
        "Skinny",
        "Square",
        "Steep",
        "Straight",
        "Wide",
        "Ancient",
        "Brief",
        "Early",
        "Fast",
        "Late",
        "Long",
        "Modern",
        "Old",
        "Quick",
        "Rapid",
        "Short",
        "Slow",
        "Swift",
        "Young",
        "Bitter",
        "Delicious",
        "Fresh",
        "Greasy",
        "Juicy",
        "Icy",
        "Loose",
        "Melted",
        "Nutritious",
        "Prickly",
        "Rainy",
        "Rotten",
        "Salty",
        "Sticky",
        "Strong",
        "Sweet",
        "Tart",
        "Tasteless",
        "Uneven",
        "Weak",
        "Wet",
        "Wooden",
        "Yummy",
        "Angry",
        "Bewildered",
        "Clumsy",
        "Defeated",
        "Embarrassed",
        "Fierce",
        "Grumpy",
        "Helpless",
        "Itchy",
        "Jealous",
        "Mysterious",
        "Nervous",
        "Obnoxious",
        "Panicky",
        "Repulsive",
        "Scary",
        "Thoughtless",
        "Uptight",
        "Worried"
    };

    QString names[100] = {
        "Rachelle",
        "Emogene",
        "Chere",
        "Stephanie",
        "Marquetta",
        "Genia",
        "Keva",
        "Alesia",
        "Catherin",
        "Janetta",
        "Georgina",
        "Rozella",
        "Maurita",
        "Lonnie",
        "Laureen",
        "Novella",
        "Millard",
        "Cherry",
        "Louie",
        "Joane",
        "Ula",
        "Sanav",
        "Eufemia",
        "Leonia",
        "Theresev",
        "Tiesha",
        "Mao",
        "Blaine",
        "Wynell",
        "Frankie",
        "Moira",
        "Rene",
        "Craig",
        "Dennise",
        "Frederica",
        "Cheryle",
        "Bret",
        "Monique",
        "Shawanda",
        "Bertram",
        "Lisbeth",
        "Vannessa",
        "Maisie",
        "Dee",
        "Edda",
        "Lucie",
        "Kaley",
        "Jani",
        "Malorie",
        "Minna",
        "Donette",
        "Becki",
        "Lorena",
        "Genny",
        "Iesha",
        "Raeann",
        "Alysa",
        "Neida",
        "Earlean",
        "Fausto",
        "Bart",
        "Adrianna",
        "Louetta",
        "Kiara",
        "Tari",
        "Mari",
        "Denae",
        "Karyn",
        "Alyse",
        "Penny",
        "Misti",
        "Rolland",
        "Betsy",
        "Holley",
        "Shaneka",
        "Max",
        "Gena",
        "Nelida",
        "Joel",
        "Lela",
        "Dagny",
        "Simon",
        "Jasmin",
        "Isaiah",
        "Bernadette",
        "Basil",
        "Alexandra",
        "Inell",
        "Laquita",
        "Hoa",
        "Olivia",
        "Georgia",
        "Jennefer",
        "Carolyn",
        "Catherine",
        "Addie",
        "Dennis",
        "Quintin",
        "Niesha",
        "Laura"
    };

    return adjs[qrand()%100]+names[qrand()%100] + QString::number(qrand()%1000);
}

QColor MultiPlayerManager::GenerateColour()
{        
    QColor color;
    color.setHsl(int(10.0f * (qrand()%255)), 128, 192); //generates pastel colours
    return color;
}

void MultiPlayerManager::SetResetPlayer(const bool b)
{
    reset_player = b;
}

bool MultiPlayerManager::GetResetPlayer()
{
    return reset_player;
}

void MultiPlayerManager::SetSessionTrackingEnabled(const bool b)
{
    if (session_tracking_enabled != b) {
        b ? DoSessionStart() : DoSessionEnd();
    }
    session_tracking_enabled = b;
}

bool MultiPlayerManager::GetSessionTrackingEnabled() const
{
    return session_tracking_enabled;
}

void MultiPlayerManager::SetPartyMode(const bool b)
{
    partymode = b;
}

bool MultiPlayerManager::GetPartyMode() const
{
    return partymode;
}

void MultiPlayerManager::SetRecording(const bool b, const bool record_everyone)
{
    if (b) {
        //open file for writing
        QString filename = MathUtil::GetRecordingPath() + "ghost-" + MathUtil::GetCurrentDateTimeAsString() + ".rec";
        ghost_file.setFileName(filename);
        if (!ghost_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "MultiPlayerManager::StartRecording(): File " << filename << " can't be saved";
            return;
        }

        //save out the data
        ghost_ofs.setDevice(&ghost_file);
        ghost_ofs.setRealNumberNotation(QTextStream::FixedNotation);
        ghost_ofs.setRealNumberPrecision(3);

        ghost_recording_start_time.start();
        recording = true;
        recording_everyone = record_everyone;
        ghost_packets.clear();
    }
    else {
        recording = false;
        if (ghost_file.isOpen()) {
            ghost_file.close();
        }
    }
}

bool MultiPlayerManager::GetRecording() const
{
    return recording;
}

int MultiPlayerManager::GetNumberUsersURL(const QString & url)
{
    int num = 1; //include the player

    QString url_to_draw_md5 = MathUtil::MD5Hash(url);

    //count players
    QMap<QString, QPointer <RoomObject> >::iterator i;
    for (i = players.begin(); i != players.end(); ++i) {
        QPointer <RoomObject> player = i.value();
        if (player && QString::compare(player->GetURL(), url_to_draw_md5) == 0 && player->GetTimeElapsed() < 5.0f) { //47.23 - make players who disconnect disappear
            ++num;
        }
    }

    return num;
}

void MultiPlayerManager::AddAssetRecording(QPointer <AssetRecording> a)
{
    if (!assetrecording_list.contains(a)) {
        assetrecording_list.push_back(a);
    }
}

void MultiPlayerManager::SetConnections(QHash <QString, QHash <int, QSet <QString> > > cs)
{
    connections = cs;
}

QHash <QString, QHash <int, QSet <QString> > > MultiPlayerManager::GetConnections() const
{
    QHash <QString, QHash <int, QSet <QString> > > cs;
    for (const ServerConnection & c : connection_list) {
        for (const RoomConnection & rc : c.rooms) {
            cs[c.tcpserver][c.tcpport].insert(rc.url);
        }
    }
    return cs;
}

void MultiPlayerManager::UpdateConnections()
{
    //we have two lists:
    // 1 - connections, the desired connection list
    // 2 - cs, the list of actual current connections

    //on each iteration, we account for the difference between these two, involving two things
    // 1 - connect to new rooms (and servers) if in connections and not cs
    // 2 - disconnect from deallocated rooms (and potentially end connection with server) if in cs and not connections

    const QHash <QString, QHash <int, QSet <QString> > > cs = GetConnections();

//    qDebug() << "MultiPlayerManager::UpdateConnections() connections" << connections << "cs" << cs;

    //handle add subscribe
    QHash <QString, QHash <int, QSet <QString> > >::const_iterator it;
    QHash <int, QSet <QString> >::const_iterator it2;
    QSet <QString>::const_iterator it3;
    for (it=connections.begin(); it!=connections.end(); ++it) { //iterate over servers
        for (it2=it.value().begin(); it2!=it.value().end(); ++it2) { //iterate over ports
            for (it3=it2.value().begin(); it3!=it2.value().end(); ++it3) { //iterate over URLs
//                qDebug() << "MultiPlayerManager::UpdateConnections() conections" << it.key() << it2.key() << *it3;
                if (!cs.contains(it.key()) || !cs[it.key()].contains(it2.key()) || !cs[it.key()][it2.key()].contains(*it3)) {
                    AddSubscribeURL(it.key(), it2.key(), *it3);
                }
            }
        }
    }

    //handle remove subscribe
    for (it=cs.begin(); it!=cs.end(); ++it) { //iterate over servers
        for (it2=it.value().begin(); it2!=it.value().end(); ++it2) { //iterate over ports
            for (it3=it2.value().begin(); it3!=it2.value().end(); ++it3) { //iterate over URLs
                if (!connections.contains(it.key()) || !connections[it.key()].contains(it2.key()) || !connections[it.key()][it2.key()].contains(*it3)) {
                    RemoveSubscribeURL(*it3);
                }
            }
        }
    }

    DoSocketConnect();
}

void MultiPlayerManager::AddSubscribeURL(const QString & server, const int port, const QString & url)
{
//    qDebug() << "MultiPlayerManager::AddSubscribeURL()" << server << port << url;
    const QString url_id = MathUtil::MD5Hash(url);

    int conn_index = -1;
    for (int i=0; i<connection_list.size(); ++i) {
        ServerConnection & c = connection_list[i];

        if (QString::compare(c.tcpserver, server) == 0 && c.tcpport == port) {
            conn_index = i;
            break;
        }
    }

    if (conn_index >= 0) {
        ServerConnection & c = connection_list[conn_index];

        // 60.0 - we need subconditions to know for this server connection, if it is actually connected or not
        // (as a server connection with no visible rooms, will exist but not be connected)
        if (c.tcpsocket && !c.tcpsocket->isOpen()) {
            DoSocketConnect();
        }

        int room_index = -1;
        for (int i=0; i<c.rooms.size(); ++i) {
            if (QString::compare(c.rooms[i].url, url) == 0) {
                room_index = i;
                break;
            }
        }

        if (room_index == -1) {
//            qDebug() << "  MultiPlayerManager::AddSubscribeURL() - adding new room on existing connection" << server << port << url;
            RoomConnection r;
            r.url = url;
            r.id = url_id;
            r.sent_subscribe = false;
            c.rooms.push_back(r);
        }
    }
    else {
//        qDebug() << "  MultiPlayerManager::AddSubscribeURL() - New server" << server << port << url;
        ServerConnection c;

        c.tcpserver = server;
        c.tcpport = port;

        const QString host = QUrl(c.tcpserver).host();
        if (WebAsset::auth_list.contains(host)) {
            c.user = WebAsset::auth_list[host].user;
            c.password = WebAsset::auth_list[host].password;
        }

        RoomConnection r;
        r.url = url;
        r.id = url_id;
        r.sent_subscribe = false;

        c.rooms.push_back(r);

        connect(c.tcpsocket, SIGNAL(connected()), this, SLOT(SocketConnected()));
        connect(c.tcpsocket, SIGNAL(disconnected()), this, SLOT(SocketDisconnected()));
        connect(c.tcpsocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(SocketError()));

        connection_list.push_back(c);
    }
}

void MultiPlayerManager::RemoveSubscribeURL(const QString & url)
{
//    qDebug() << "MultiPlayerManager::RemoveSubscribeURL" << url;
    const QString url_id = MathUtil::MD5Hash(url);

    for (int i=0; i<connection_list.size(); ++i) {

        ServerConnection & c = connection_list[i];

        for (int j=0; j<c.rooms.size(); ++j) {
            if (QString::compare(c.rooms[j].url, url) == 0) {
                const QString unsubscribe_packet = GetUnsubscribePacket(url_id);
                c.tcpsocket->write(unsubscribe_packet.toUtf8());
                c.rooms[j].sent_subscribe = false;
                c.rooms.removeAt(j);
                break;
            }
        }

        //59.0 - bugfix, prevent disconnecting from server once connected
        if (c.rooms.empty()) {
            if (c.tcpsocket->state() != QAbstractSocket::UnconnectedState) {
//                qDebug() << "  MultiPlayerManager::RemoveSubscribeURL disconnecting from" << c.tcpserver << c.tcpport << url;
                c.tcpsocket->disconnectFromHost();
            }
            c.logged_in = false;
            c.logging_in = false;

//            qDebug() << "  MultiPlayerManager::RemoveSubscribeURL removing" << c.tcpserver << c.tcpport;
            connection_list.removeAt(i);
            --i;

        }
    }
}

QList <QPointer <RoomObject> > & MultiPlayerManager::GetOnPlayerEnterEvents()
{
    return on_player_enter_events;
}

QList <QPointer <RoomObject> > & MultiPlayerManager::GetOnPlayerExitEvents()
{
    return on_player_exit_events;
}
