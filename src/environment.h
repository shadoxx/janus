#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "mathutil.h"
#include "assetskybox.h"
#include "assetobject.h"
#include "assetshader.h"
#include "room.h"
#include "roomtemplate.h"
#include "player.h"
#include "soundmanager.h"
#include "multiplayermanager.h"
#include "cookiejar.h"
#include "rendererinterface.h"
#include "renderergl33_loadingthread.h"
#include "assetimage.h"
#include "scriptbuiltins.h"

class Environment : public QObject
{
    Q_OBJECT

public:    

    Environment();
    ~Environment();

    void Reset();

    void draw_current_room(MultiPlayerManager*  multi_players, QPointer <Player> player, const bool render_left_eye);
    void draw_child_rooms(MultiPlayerManager*  multi_players, QPointer <Player> player, const bool render_left_eye);

    static void SetLaunchURLIsCustom(const bool b);
    static bool GetLaunchURLIsCustom();

    static void SetLaunchURL(const QString & s);
    static QString GetLaunchURL();      

    QPointer <Room> AddRoom(QPointer <RoomObject> p);           
    bool ClearRoom(QPointer <RoomObject> p);

    void UpdateAssets();

    void Update1(QPointer <Player> player);
    void Update2(QPointer <Player> player, MultiPlayerManager * multi_players);

    void NavigateToRoom(QPointer <Player>, QPointer <Room> r);

    void SetCurRoom(QPointer <Player>, QPointer <Room> r);
    QPointer <Room> GetCurRoom();
    QPointer <Room> GetLastRoom();
    QPointer <Room> GetRootRoom();

    void MovePlayer(QPointer <RoomObject> portal, QPointer <Player> player, const bool set_player_to_portal);

    void ReloadRoom();
    void UpdateRoomCode(const QString & code);

    void Shutdown();

    void Update_CrossPortals(QPointer <Player> player);

signals:

    void RoomsChanged();

private:   

    void UpdateQueuedFunctions(QPointer <Room> r);

    void SetupPocketspace();
    void DrawRoomWithinPortalStencilGL(QPointer <RoomObject> portal, QPointer <Player> player, MultiPlayerManager*  multi_players, const bool render_left_eye);

    QPointer <Room> rootnode; //the pocketspace and children
    QPointer <Room> curnode; //pointer node player is at
    QPointer <Room> lastnode; //last non-pocketspace/root node player was at

    QVector3D player_lasteyepoint;

    static bool launch_url_is_custom;
    static QString launch_url;

#ifdef __ANDROID__
    QList <QPointer <RoomObject> > rooms_to_clear;
    QMutex clear_room_lock;
#endif
};

#endif // ENVIRONMENT_H