#ifndef MULTIPLAYERMANAGER_H
#define MULTIPLAYERMANAGER_H

#include <QtCore>
#include <QtNetwork>
#include <QtWebSockets/QtWebSockets>

#include "mathutil.h"
#include "htmlpage.h"
#include "assetghost.h"
#include "assetrecording.h"
#include "soundmanager.h"
#include "settingsmanager.h"

class RoomObject;
class Player;

struct RoomConnection
{
    RoomConnection():
        sent_subscribe(false)
    {
    }

    QString id;
    QString url;
    bool sent_subscribe; //this says "we have sent the subscribe packet"
};

class ServerConnectionListener : public QObject
{
    Q_OBJECT

public:

    ServerConnectionListener();

    QList <QString> & GetMessages();
    void ClearMessages();

public slots:

    void SocketMessageReceived(QString message);

private:

    QList <QString> messages;
};

struct ServerConnection {

    ServerConnection() :        
        tcpsocket(new QSslSocket()),
        tcpport(5567),
        logging_in(false),
        logged_in(false),
        retries(0),
        use_ssl(true)
    {
        reconnect_time.start();
        tcpsocket->ignoreSslErrors();
    }   

    QSslSocket * tcpsocket;
    QString tcpserver;
    qint64 tcpport;    
    bool logging_in;
    bool logged_in;

    QList <RoomConnection> rooms;

    QString last_error_msg;
    QTime reconnect_time;
    unsigned int retries;

    bool use_ssl;        

    QString user;
    QString password;

    QString last_packet;
    QString sent_enterroom_url_id;
};

class MultiPlayerManager : public QObject
{

    Q_OBJECT

public:

    MultiPlayerManager();
	~MultiPlayerManager();

    void Initialize();   

    void DoSocketConnect();
    void DoSocketDisconnect();

    void Clear();

    void SetEnabled(const bool b);
    bool GetEnabled();

    QList <ServerConnection> & GetConnectionList();
    void SetConnections(QHash <QString, QHash <int, QSet <QString> > > cs);
    QHash <QString, QHash <int, QSet <QString> > > GetConnections() const;

    void Update(QPointer <Player> player, const QString & url, const QList <QString> adjacent_urls, const QString & name, const bool room_allows_party_mode, const float delta_time);

    void SetURLToDraw(const QString & s);
    void DrawGL(QPointer <AssetShader> shader, const QVector3D & player_pos, const bool render_left_eye);
    void DrawCursorsGL(QPointer <AssetShader> shader);

    void UpdateAvatarData();
    void LoadAvatarData(const bool load_userid);
    void SaveAvatarData();
    void SetAvatarData(const QString s);
    QString GetAvatarData() const;
    void SetAvatarFromGhost(QPointer <RoomObject> new_ghost);

    void DoUpdateAvatar();

    bool SetChatMessage(QPointer <Player> player, const QString & s);
    void SetSendPortal(const QString & s, const QString & js_id);    
    void SetCursorPosition(const QVector3D & cpos, const QVector3D & cxdir, const QVector3D & cydir, const QVector3D & czdir, const float cscale);    
    void AddMicBuffers(const QList <QByteArray> buffers);

    void SetHMD(const QString & s);

    QString GetUserID() const;    
    QPointer <RoomObject> GetPlayer();
    QList <QPointer <RoomObject> > GetPlayersInRoom(const QString & url);
    QMap <QString, DOMNode *> GetPlayersInRoomDOMNodeMap(const QString & url);

    QList <QPair <QString, QColor> > GetNewChatMessages();

    void AddChatMessage(const QString & s, const QColor col = QColor(0,0,0));
    int GetChatMessagesSize() const;
    int GetChatMessagesPos() const;

    void SetRoomAssetEdit(const QString s);
    void SetRoomEdit(const QPointer <RoomObject> o);
    void SetRoomDeleteCode(const QString & s);

    void SetResetPlayer(const bool b);
    bool GetResetPlayer();

    void SetResetPlayerReason(const QString & s);
    QString GetResetPlayerReason();

    void SetSessionTrackingEnabled(const bool b);
    bool GetSessionTrackingEnabled() const;

    void SetPartyMode(const bool b);
    bool GetPartyMode() const;

    void SetHeadSrc(const QString & s);
    void SetHeadMtl(const QString & s);
    void SetBodySrc(const QString & s);
    void SetBodyMtl(const QString & s);

    void SetRecording(const bool b, const bool record_everyone = true);
    bool GetRecording() const;

    int GetNumberUsersURL(const QString & url);

    void AddAssetRecording(QPointer <AssetRecording> a);

    void SetCustomPortalShader(const QString shader_src);
    QString GetCustomPortalShader() const;

    QList <QPointer <RoomObject> > & GetOnPlayerEnterEvents();
    QList <QPointer <RoomObject> > & GetOnPlayerExitEvents();

public slots:

    void SocketConnected();
    void SocketDisconnected();    
    void SocketError();

private:      

    void AddSubscribeURL(const QString & server, const int port, const QString & url);
    void RemoveSubscribeURL(const QString & url);
    void UpdateConnections();

    void SetNewUserID(const QString id, const bool append_random_number);    

    QString GenerateUserID();
    QColor GenerateColour();   

    void DoSessionStart();
    void DoSessionEnd();

    QString GetLogonPacket(const QString & userId, const QString & password, const QString & url_id);
    QString GetSubscribePacket(const QString & url_id);
    QString GetUnsubscribePacket(const QString & url_id);
    QString GetEnterRoomPacket(const bool room_allows_party_mode);

    QString GetChatPacket(const QString & chat_message);
    QString GetPortalPacket(QPointer <Player> player);
    QString GetMovePacket(QPointer <Player> player);
    QString GetMovePacket_Helper(QPointer <Player> player);

    QString GetUserChatPacket(const QString & chat_message);
    QString GetUserPortalPacket(QPointer <Player> player);
    QString GetUserMovePacket(QPointer <Player> player);

    void DoError(const QJsonValue & v, ServerConnection & s);
    void DoUserMoved(const QVariantMap & m);
    void DoUserChat(const QVariantMap & m);
    void DoUserPortal(const QVariantMap & m);
    void DoUserDisconnected(const QVariantMap & m);

    QHash <QString, QHash <int, QSet <QString> > > connections;

    bool enabled;    
    bool partymode;

    QMap <QString, QPointer <RoomObject> > players;
    QMap <QString, QPointer <RoomObject> > players_in_room;

    QString cur_url;
    QString cur_url_id;
    QString cur_name;

    QString avatar_data;
    QString avatar_data_encoded;
    QPointer <RoomObject> user_ghost;
    QString chat_message;
    QString send_portal;
    QString send_portal_jsid;

    QString hmd;

    QString room_edit_assets;
    QList <QPointer <RoomObject> > room_edit_objs;

    bool reset_player;

    QList <ServerConnection> connection_list;

    QString userid_filename;
    QString userid_backup_filename;

    QString url_to_draw;
    QString url_to_draw_md5;

    QList <QPair <QString, QColor> > chat_messages_log;

    QList <QByteArray> mic_buffers;

    QVector3D cpos_list;
    QVector3D cxdir_list;
    QVector3D cydir_list;
    QVector3D czdir_list;
    float cscale_list;
    bool pending_pos;

    bool avatar_update;
    QTime avatar_update_timer;

    QTime update_time;
    DOMNode last_player_properties;

    bool recording;
    bool recording_everyone;
    QFile ghost_file;
    QTextStream ghost_ofs;
    QList <AssetRecordingPacket> ghost_packets;
    QTime ghost_recording_start_time;
    QList <QByteArray> ghost_recording_mic_buffers;

    //tracking/analytics with EASEVR
    bool session_tracking_enabled;
    QHash <QString, QString> session_data;
    WebAsset session_start_webasset;
    WebAsset session_end_webasset;

    //assetrecordings (emulate server activity)
    QList <QPointer <AssetRecording> > assetrecording_list;

    QList <QPointer <RoomObject> > on_player_enter_events; //for room.onPlayerEnter room.onPlayerExit events
    QList <QPointer <RoomObject> > on_player_exit_events; //for room.onPlayerEnter room.onPlayerExit events

    static unsigned int max_connect_retries;
};

#endif // MULTIPLAYERMANAGER_H
