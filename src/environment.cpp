#include "environment.h"

bool Environment::launch_url_is_custom = false;
QString Environment::launch_url;

Environment::Environment()
{    
}

Environment::~Environment()
{
}

void Environment::SetLaunchURLIsCustom(const bool b)
{
    launch_url_is_custom = b;
}

bool Environment::GetLaunchURLIsCustom()
{
    return launch_url_is_custom;
}

void Environment::SetLaunchURL(const QString & s)
{
    launch_url = s;
    launch_url.replace("janus://", "http://", Qt::CaseInsensitive);

    //60.0 - get URL representation for local filesystem files
    QFile f(launch_url);
    if (f.exists()) {
        launch_url = QUrl::fromLocalFile(launch_url).toString();
    }
}

QString Environment::GetLaunchURL()
{
    return launch_url;
}

void Environment::SetupPocketspace()
{
    if (rootnode) {
        //delete all children
        rootnode->GetChildren().clear();
        delete rootnode;
    }

    //setup pocketspace
    rootnode = new Room();   

//    qDebug() << "Environment::SetupPocketspace()";
//    QPointer <RoomObject> portal = rootnode->GetPortal0();
    QPointer <RoomObject> parent_portal = new RoomObject();
    parent_portal->SetType("link");
    rootnode->SetParentObject(parent_portal);

    //portal->SetURL("", "http://usagii.net/other/personal/work/pocketspace3/space/index.html");
#ifdef __ANDROID__
    parent_portal->SetURL(MathUtil::GetApplicationURL(), "https://vesta.janusvr.com");
    //parent_portal->SetURL(MathUtil::GetApplicationURL(), "http://janusvr.com");
    //parent_portal->SetURL(MathUtil::GetApplicationURL(), "https://vesta.janusvr.com/aussie/live-stream-abc-tv-australia-in-webvr");
    //parent_portal->SetURL(MathUtil::GetApplicationURL(), "https://vesta.janusvr.com/firefoxg/learn-about-pyramids-in-multiplayer-webvr");
#else
    parent_portal->SetURL(MathUtil::GetApplicationURL(), "assets/3dui/index.html");
#endif
    parent_portal->SetV("pos", QVector3D(0,0,0));
    parent_portal->SetDir(QVector3D(0,0,1));

    QPointer <RoomObject> entrance_portal = rootnode->GetEntranceObject();
    entrance_portal->SetV("pos", QVector3D(0,0,0));
    entrance_portal->SetDir(QVector3D(0,0,1));

    parent_portal->SetB("visible", false);
    parent_portal->SetB("active", false);
    parent_portal->SetSaveToMarkup(false);

    entrance_portal->SetB("visible", false);
    entrance_portal->SetB("active", false);
    entrance_portal->SetSaveToMarkup(false);

//    rootnode->SetUseExperimentalWebsurfaces(false); //Cannot use experimental websurfaces since we need to pass window.janus object to it
    rootnode->SetS("url", parent_portal->GetURL());

    curnode = rootnode;
}

void Environment::Reset()
{    
    qsrand(QDateTime::currentMSecsSinceEpoch() % 1000);

    SetupPocketspace();

#ifdef __ANDROID__
    if (JNIUtil::GetLaunchURL() != ""){
        Environment::SetLaunchURLIsCustom(true);
        Environment::SetLaunchURL(JNIUtil::GetLaunchURL());
    }
    if (launch_url_is_custom) {
        //seed with launch URL
        QPointer <RoomObject> new_portal = new RoomObject();
        new_portal->SetType("link");
        if (launch_url_is_custom) {
            new_portal->SetURL("", launch_url);
        }
        else {
            new_portal->SetURL("", SettingsManager::GetHomeURL());
        }
        new_portal->SetB("visible", false);
        new_portal->SetB("active", false);
        rootnode->AddRoomObject(new_portal);
        curnode = AddRoom(new_portal);
        lastnode = curnode;
    }
    else {
        lastnode.clear();
        curnode = rootnode;
    }
#else
    //seed with launch URL
    QPointer <RoomObject> new_portal = new RoomObject();
    new_portal->SetType("link");
    if (launch_url_is_custom) {
        new_portal->SetURL("", launch_url);
    }
    else {
        new_portal->SetURL("", SettingsManager::GetHomeURL());
    }
    new_portal->SetB("visible", false);
    new_portal->SetB("active", false);

    rootnode->AddRoomObject(new_portal);
    lastnode = AddRoom(new_portal);
    curnode = (launch_url_is_custom ? lastnode : rootnode);
//    qDebug() << launch_url_is_custom << launch_url;

    QPointer <RoomObject> o = lastnode->GetEntranceObject();
    if (o) {
        o->SetB("visible", false);
        o->SetB("active" , false);
    }
#endif

    emit RoomsChanged();
}

QPointer <Room> Environment::AddRoom(QPointer <RoomObject> p)
{
//    qDebug() << "Environment::AddRoom" << p;
    if (p.isNull()) {
        return NULL;
    }

    QPointer <Room> r;
    if (p == curnode->GetEntranceObject()) {
        // Leads to parent (we clicked our room's entranceportal)
        r = curnode->GetConnectedRoom(p);
    }
    else {                
        //create a child
        r = new Room();
        r->SetParentObject(p);
        r->SetParent(curnode);
        r->GetEntranceObject()->SetB("open", true);
        r->SetS("url", p->GetURL());
        curnode->AddChild(r);
//        qDebug() << "Environment::AddNewRoom creating child" << r << curnode;
    }

    emit RoomsChanged();

    return r;
}

bool Environment::ClearRoom(QPointer <RoomObject> p)
{
//    qDebug() << "Environment::ClearRoom" << p;
    QPointer <Room> r = curnode->GetConnectedRoom(p);
    if (r == rootnode) {
        return true;
    }
    else if (r && r != curnode && r != rootnode && r->GetReady()) {
        if (r != curnode->GetLastChild()) {
            curnode->RemoveChild(r);
        }
        r->Clear();
        emit RoomsChanged();
        return true;
    }
    return false;
}

void Environment::draw_current_room(MultiPlayerManager*  multi_players, QPointer <Player> player, const bool render_left_eye)
{
    if (curnode.isNull()) {
        return;
    }

    RendererInterface * renderer = RendererInterface::m_pimpl;   

    //59.7 - Note!  Currently no portal culling.

    MathUtil::LoadRoomMatrix(QMatrix4x4());

    // 1. Draw this room's skybox with depth writes disabled
    // Marks the beginning of a named renderer scope for profiling and debug purposes
    renderer->BeginScope(RENDERER::RENDER_SCOPE::CURRENT_ROOM_SKYBOX);
    renderer->SetStencilOp(StencilOp(StencilOpAction::KEEP, StencilOpAction::KEEP, StencilOpAction::KEEP));
    renderer->SetStencilFunc(StencilFunc(StencilTestFuncion::ALWAYS, StencilReferenceValue(0), StencilMask(0xffffffff)));
    renderer->SetDepthFunc(DepthFunc::ALWAYS);
    renderer->SetDepthMask(DepthMask::DEPTH_WRITES_DISABLED);
    renderer->SetColorMask(ColorMask::COLOR_WRITES_ENABLED);

    QMatrix4x4 m;
    m.translate(player->GetV("eye_point"));
    // This draws it near the farclip but avoids the corners being depth failed
    // drawing it far from the user is to keep stereo disparity to near-zero so it's perceived
    // as being infinitely far away.
    m.scale(curnode->GetF("far_dist") * 0.3f);
    curnode->BindShader(Room::GetSkyboxShader());
    curnode->DrawSkyboxGL(Room::GetSkyboxShader(), m);

    renderer->EndCurrentScope();

#ifdef __ANDROID__
    //qDebug() << curnode->GetProgress();
    if (curnode->GetProgress() <= 0.5f){
        return;
    }
#endif

    QPointer <AssetShader> room_shader = Room::GetTransparencyShader();
    if (curnode->GetAssetShader() && curnode->GetAssetShader()->GetCompiled()) {
        room_shader = curnode->GetAssetShader();
    }

    curnode->SetPlayerPosTrans(player->GetV("eye_point"));
    curnode->SetUseClipPlane(false);

    curnode->BindShader(room_shader);

    // 2. Draw each open child portal with it's own unique stencil index
    renderer->BeginScope(RENDERER::RENDER_SCOPE::CURRENT_ROOM_PORTAL_STENCILS);
    renderer->SetStencilOp(StencilOp(StencilOpAction::KEEP, StencilOpAction::KEEP, StencilOpAction::REPLACE));
    renderer->SetDepthFunc(DepthFunc::LEQUAL);
    renderer->SetDepthMask(DepthMask::DEPTH_WRITES_ENABLED);
    renderer->SetColorMask(ColorMask::COLOR_WRITES_ENABLED);

    int i=0;    
    const QHash <QString, QPointer <RoomObject> > & envobjects = curnode->GetRoomObjects();
    for (auto & each_portal : envobjects) {
        // If culled or not open, don't draw
        if (each_portal && each_portal->GetType() == "link") {
            if (each_portal->GetB("open") && each_portal->GetB("visible")) {
                StencilFunc newFunc = StencilFunc(StencilTestFuncion::ALWAYS, StencilReferenceValue(i+1), StencilMask(0xffffffff));
                //qDebug() << "Environement[507] Drawing Portal Stencil for Portal " << each_portal << each_portal->GetURL() << "SetStencilFunc: EQUAL " << (i+1) << " 0xffffffff";
                renderer->SetStencilFunc(newFunc);
                each_portal->DrawStencilGL(room_shader, player->GetV("eye_point"));
            }
            ++i;
        }       
    }

    renderer->EndCurrentScope();

    // 3. Draw current Room
    renderer->BeginScope(RENDERER::RENDER_SCOPE::CURRENT_ROOM_OBJECTS);
    renderer->SetStencilOp(StencilOp(StencilOpAction::KEEP, StencilOpAction::KEEP, StencilOpAction::REPLACE));
    renderer->SetStencilFunc(StencilFunc(StencilTestFuncion::ALWAYS, StencilReferenceValue(0), StencilMask(0xffffffff)));
    renderer->SetDepthMask(DepthMask::DEPTH_WRITES_ENABLED);
    renderer->SetDepthFunc(DepthFunc::LEQUAL);
    renderer->SetColorMask(ColorMask::COLOR_WRITES_ENABLED);

    multi_players->SetURLToDraw(curnode->GetS("url"));

    // Walks the current room queuing all roomobjects to queue themselves for rendering.
    curnode->DrawGL(multi_players, player, render_left_eye, false, false);

    // Marks the end of a named renderer scope for profiling and debug purposes
    renderer->EndCurrentScope();
}

void Environment::draw_child_rooms(MultiPlayerManager*  multi_players, QPointer <Player> player, const bool render_left_eye)
{

#ifdef __ANDROID__
    if (curnode->GetProgress() <= 0.5f){
        return;
    }
#endif

    const QHash <QString, QPointer <RoomObject> > & envobjects = curnode->GetRoomObjects();

    RendererInterface * renderer = RendererInterface::m_pimpl;

    // 2. Draw child portals (and their skyboxes as seen through portal)
    RendererInterface::m_pimpl->BeginScope(RENDERER::RENDER_SCOPE::CHILD_ROOM_OBJECTS);
    renderer->SetDepthFunc(DepthFunc::LEQUAL);
    renderer->SetColorMask(ColorMask::COLOR_WRITES_ENABLED);
    renderer->SetDepthMask(DepthMask::DEPTH_WRITES_ENABLED);
    renderer->SetStencilOp(StencilOp(StencilOpAction::KEEP, StencilOpAction::KEEP, StencilOpAction::KEEP));
    renderer->SetDefaultFaceCullMode(FaceCullMode::BACK);

    int i = 0;
    for (auto & each_portal : envobjects) {
        if (each_portal && each_portal->GetType() == "link") {
            if (each_portal->GetB("open") && each_portal->GetB("visible")) {
                StencilFunc newFunc = StencilFunc(StencilTestFuncion::EQUAL, StencilReferenceValue(i+1), StencilMask(0xffffffff));
                //qDebug() << "Environement[554] Drawing Room within Portal " << each_portal << each_portal->GetURL() << "SetStencilFunc: EQUAL " << (i+1) << " 0xffffffff";
                renderer->SetStencilFunc(newFunc);
    //            qDebug() << "  drawing" << each_portal;
                DrawRoomWithinPortalStencilGL(each_portal, player, multi_players, render_left_eye);
            }
            ++i;
        }
    }
    RendererInterface::m_pimpl->EndCurrentScope();

    // 3. Draw portal stencils again to depth buffer only, not color buffer (this stops portal decorations in current room being drawn "on top" of views into open portals)
    renderer->BeginScope(RENDERER::RENDER_SCOPE::CURRENT_ROOM_PORTAL_DEPTH_REFRESH);
    renderer->SetStencilOp(StencilOp(StencilOpAction::KEEP, StencilOpAction::KEEP, StencilOpAction::KEEP));
    renderer->SetColorMask(ColorMask::COLOR_WRITES_DISABLED);
    renderer->SetDepthMask(DepthMask::DEPTH_WRITES_ENABLED);
    renderer->SetDepthFunc(DepthFunc::LEQUAL);
    renderer->SetDefaultFaceCullMode(FaceCullMode::DISABLED);

    curnode->BindShader(Room::GetTransparencyShader());
    i=0;
    for (auto & each_portal : envobjects) {
        if (each_portal && each_portal->GetType() == "link") {
            // If culled or not open, don't draw
            if (each_portal->GetB("open") && each_portal->GetB("visible"))
            {
                StencilFunc newFunc = StencilFunc(StencilTestFuncion::EQUAL, StencilReferenceValue(i+1), StencilMask(0xffffffff));
                //qDebug() << "Environement[581] Portal Depth Refresh for Portal " << each_portal << each_portal->GetURL() << "SetStencilFunc: EQUAL " << (i+1) << " 0xffffffff";
                renderer->SetStencilFunc(newFunc);
                each_portal->DrawStencilGL(Room::GetTransparencyShader(), player->GetV("eye_point"));
            }
            i++;
        }
    }

    renderer->EndCurrentScope();

    // 4. Draw current Room portal decorations
    // These are drawn after child rooms so that they alpha blend with the child room contents
    renderer->BeginScope(RENDERER::RENDER_SCOPE::CURRENT_ROOM_PORTAL_DECORATIONS);
    renderer->SetStencilOp(StencilOp(StencilOpAction::KEEP, StencilOpAction::KEEP, StencilOpAction::KEEP));
    renderer->SetStencilFunc(StencilFunc(StencilTestFuncion::ALWAYS, StencilReferenceValue(0), StencilMask(0xffffffff)));
    renderer->SetColorMask(ColorMask::COLOR_WRITES_ENABLED);
    renderer->SetDepthMask(DepthMask::DEPTH_WRITES_DISABLED);
    renderer->SetDepthFunc(DepthFunc::LEQUAL);
    renderer->SetDefaultFaceCullMode(FaceCullMode::DISABLED);

    QPointer <RoomObject> player_room_object = multi_players->GetPlayer();
    QHash <QString, QPointer <AssetShader> > player_portal_shader = player_room_object->GetGhostAssetShaders();
    QPointer <AssetShader> user_portal_shader = player_portal_shader["CustomPortalShader"];
    const bool using_shader = user_portal_shader && user_portal_shader->GetCompiled();

    if (!using_shader) {
        user_portal_shader = Room::GetPortalShader();
    }

    curnode->BindShader(user_portal_shader);

    for (auto & each_portal : envobjects) {
        if (each_portal && each_portal->GetType() == "link" && each_portal->GetB("visible")) {
            QPointer <Room> r = curnode->GetConnectedRoom(each_portal);
            const float val = ((r && r->GetStarted() && !r->GetReady()) ? r->GetProgress() : 1.0f);
            each_portal->DrawDecorationsGL(user_portal_shader, val);
        }
    }

    renderer->SetStencilOp(StencilOp(StencilOpAction::KEEP, StencilOpAction::KEEP, StencilOpAction::KEEP));
    renderer->SetStencilFunc(StencilFunc(StencilTestFuncion::ALWAYS, StencilReferenceValue(0), StencilMask(0xffffffff)));
    renderer->SetDepthMask(DepthMask::DEPTH_WRITES_ENABLED);
    renderer->SetDepthFunc(DepthFunc::LEQUAL);
    renderer->SetDefaultFaceCullMode(FaceCullMode::BACK);
    renderer->EndCurrentScope();
}

void Environment::DrawRoomWithinPortalStencilGL(QPointer <RoomObject> portal, QPointer <Player> player, MultiPlayerManager * multi_players, const bool render_left_eye)
{
    QPointer <AssetShader> shader = Room::GetTransparencyShader();
    if (shader == NULL || !shader->GetCompiled()) {
        return;
    }   

    QPointer <Room> room = curnode->GetConnectedRoom(portal);
    if (room.isNull() || room == curnode) {
        return;
    }

    QPointer <RoomObject> p2 = curnode->GetConnectedPortal(portal);
    if (p2.isNull()) {
        return;
    }

#ifdef __ANDROID__
    if (room->GetProgress() <= 0.5f){
        return;
    }
#endif

    RendererInterface * renderer = RendererInterface::m_pimpl;

    QVector3D x1;
    QVector3D y1;
    QVector3D z1;
    QVector3D currentRoomPortalTranslation;

    QVector3D x2;
    QVector3D y2;
    QVector3D z2;
    QVector3D otherRoomPortalTranslation;

    if (portal->GetB("mirror")) {
        p2->SetV("xdir", portal->GetXDir());
        p2->SetV("ydir", portal->GetYDir());
        p2->SetV("zdir", portal->GetZDir());
        p2->SetV("pos", portal->GetV("pos"));
        p2->SetB("mirror", true);
    }

    x1 = portal->GetXDir();
    y1 = portal->GetYDir();
    z1 = portal->GetZDir();
    currentRoomPortalTranslation = portal->GetV("pos") + z1 * RoomObject::GetSpacing();

    x2 = p2->GetXDir();
    y2 = p2->GetYDir();
    z2 = p2->GetZDir();
    otherRoomPortalTranslation = p2->GetV("pos") + z2 * RoomObject::GetSpacing();

    if (portal->GetB("mirror")) {
        x2 *= -1.0f;
    }

    QMatrix4x4 currentRoomPortalRotation;
    currentRoomPortalRotation.setColumn(0, x1);
    currentRoomPortalRotation.setColumn(1, y1);
    currentRoomPortalRotation.setColumn(2, z1);

    QMatrix4x4 otherRoomPortalRotation;
    otherRoomPortalRotation.setColumn(0, -x2);
    otherRoomPortalRotation.setColumn(1, y2);
    otherRoomPortalRotation.setColumn(2, -z2);

    const QMatrix4x4 currentRoomToOtherRoomRotation = currentRoomPortalRotation * otherRoomPortalRotation.transposed(); //R1 x inv(R2) (inverts since orthonormal basis)

    renderer->SetMirrorMode(portal->GetB("mirror"));

    renderer->SetDepthMask(DepthMask::DEPTH_WRITES_ENABLED);
    renderer->SetColorMask(ColorMask::COLOR_WRITES_ENABLED);
    renderer->SetDepthFunc(DepthFunc::ALWAYS);


    QMatrix4x4 m;
    m.translate(player->GetV("eye_point"));
    // This draws it near the farclip but avoids the corners being depth failed
    // drawing it far from the user is to keep stereo disparity to near-zero so it's perceived
    // as being infinitely far away.
#ifndef __ANDROID__
    m.scale(room->GetF("far_dist") * 0.3f);
#else
    m.scale(room->GetF("far_dist") * 0.05f); // Prevents portals appearing black when looking through them
#endif
    renderer->BeginScope(RENDERER::RENDER_SCOPE::CHILD_ROOM_SKYBOX);
    room->BindShader(Room::GetSkyboxShader());
    room->DrawSkyboxGL(Room::GetSkyboxShader(), m * currentRoomToOtherRoomRotation);
    renderer->EndCurrentScope();

    renderer->SetDepthFunc(DepthFunc::LEQUAL);
    renderer->BeginScope(RENDERER::RENDER_SCOPE::CHILD_ROOM_OBJECTS);

    //draw the room itself.  clip plane defined in current room's worldspace    
    if (SettingsManager::GetRenderPortalRooms() || portal->GetB("auto_load")) {
        const QVector3D & child_xdir = p2->GetXDir();
        const QVector3D & child_ydir = p2->GetYDir();
        const QVector3D & child_zdir = p2->GetZDir();
        const QVector3D & child_pos = p2->GetV("pos");

        const QVector3D & parent_xdir = portal->GetXDir();
        const QVector3D & parent_ydir = portal->GetYDir();
        const QVector3D & parent_zdir = portal->GetZDir();
        const QVector3D & parent_pos = portal->GetV("pos");

        QVector4D plane_eqn(-parent_zdir, QVector3D::dotProduct(parent_pos, -parent_zdir) - RoomObject::GetSpacing());
        QVector3D player_pos_trans;

        //to compute we get the player's offset from the portal, in terms of the portal's reference frame.  we then add this to the other end of the portal, given its reference frame
        QVector3D v = player->GetV("eye_point") - (parent_pos + parent_zdir * RoomObject::GetSpacing());
        QVector3D v2(QVector3D::dotProduct(v, parent_xdir),
                     QVector3D::dotProduct(v, parent_ydir),
                     QVector3D::dotProduct(v, parent_zdir));
        QVector3D v3 = -child_xdir * v2.x() + child_ydir * v2.y() -child_zdir * v2.z();

        player_pos_trans = child_pos + v3;

        multi_players->SetURLToDraw(room->GetS("url"));

        bool draw_player = false;
        if (room->GetS("url") == curnode->GetS("url")) {
            draw_player = true;
        }

        MathUtil::PushModelMatrix();
        MathUtil::ModelMatrix().translate(currentRoomPortalTranslation.x(), currentRoomPortalTranslation.y(), currentRoomPortalTranslation.z());
        MathUtil::MultModelMatrix(currentRoomToOtherRoomRotation);
        MathUtil::ModelMatrix().translate(-otherRoomPortalTranslation.x(), -otherRoomPortalTranslation.y(), -otherRoomPortalTranslation.z());

        MathUtil::LoadRoomMatrix(MathUtil::ModelMatrix());
        room->SetPlayerPosTrans(player_pos_trans);
        room->SetUseClipPlane(true, plane_eqn);
        room->DrawGL(multi_players, player, render_left_eye, draw_player, true);
        MathUtil::LoadRoomMatrix(QMatrix4x4());
        MathUtil::PopModelMatrix();
    }

    renderer->SetMirrorMode(false);
}

void Environment::ReloadRoom()
{    
    if (curnode.isNull() || !curnode->GetReady()) {
        return;
    }
    curnode->Clear();
    curnode->SetB("_reloaded", true);
}

void Environment::UpdateRoomCode(const QString & code)
{   
    if (curnode.isNull() || !curnode->GetReady()) {
        return;
    }
    curnode->Clear();
    curnode->UpdateCode(code);
}

void Environment::MovePlayer(QPointer <RoomObject> portal, QPointer <Player> player, const bool set_player_to_portal)
{        
    QPointer <Room> room = curnode->GetConnectedRoom(portal);
    QPointer <RoomObject> p2 = curnode->GetConnectedPortal(portal);
    if (portal.isNull() || room.isNull() || p2.isNull()) {
        return;
    }   

    if (set_player_to_portal && p2) {
        player->SetV("pos", p2->GetV("pos")+p2->GetV("zdir"));
        //59.0 - orient on teleport through portal, do not do it for rift or vive
        const QString hmd_type = player->GetS("hmd_type");
        if (hmd_type != "rift" && hmd_type != "vive" && hmd_type != "wmxr" && hmd_type != "daydream" && hmd_type != "cardboard" && hmd_type != "gear" && hmd_type != "go") {
            player->SetV("dir", p2->GetZDir());
            player->UpdateDir();
        }
        player->SetV("vel", QVector3D(0,0,0));
    }    
    else {
        //also update player's position, etc., we have to transform them seamlessly to a new world coordinate
        QVector3D z1 = portal->GetZDir();
        QVector3D z2 = p2->GetZDir();

    //    qDebug() << "Environment::MovePlayer" << p0;
        z1.setY(0.0f);
        z1.normalize();
        z2.setY(0.0f);
        z2.normalize();
        const float angle = MathUtil::GetSignedAngleBetweenRadians(-z2, z1); //note: d1 and d2 swapped
        player->SpinView(angle * MathUtil::_180_OVER_PI, false);
//        qDebug() << "Environment::MovePlayer" << p2 << set_player_to_portal;

        QVector3D l = portal->GetLocal(player->GetV("pos"));
        l.setX(-l.x()); //mirror flip going through portal ("left side" of entrance is "right side" locally on exit)
        l.setZ(-l.z()); //flip z as we've already passed entrance portal - we're behind it, we need to be "in front" on the exit side
        const QVector3D p0 = p2->GetGlobal(l);

        player->SetV("pos", p0); // + new_vel * player->GetF("delta_time"));
        player->SetV("vel", QVector3D(0,0,0));
//        player->SetV("impulse_vel", QVector3D(0,0,0));
    }

    if (curnode->GetParent() != room) {
        curnode->SetLastChild(room);
    }

    SetCurRoom(player, room);

#ifdef __ANDROID__
    //Close portal upon crossing
    if (!p2->GetB("auto_load")) {
        p2->SetB("open", false); //Close portal, set to non-visible room
        if (curnode->GetConnectedPortal(p2))
            curnode->GetConnectedPortal(p2)->SetB("open", false);
    }
#endif
}

void Environment::Update_CrossPortals(QPointer <Player> player)
{
    player->UpdateEyePoint();

    //detect if player crosses a portal    
    if (curnode) {
        const QHash <QString, QPointer <RoomObject> > & envobjects = curnode->GetRoomObjects();
        for (auto & each_portal : envobjects) {
            if (each_portal && each_portal->GetType() == "link") {
                //only consider if portal is non null, open, active, visible
                if (each_portal->GetB("open") && each_portal->GetB("active") && each_portal->GetB("visible")) {

                    //only go through if the room is ready, and the portal is open
                    QPointer <Room> r = curnode->GetConnectedRoom(each_portal);
                    QPointer <RoomObject> p2 = curnode->GetConnectedPortal(each_portal);
                    if (r.isNull() || !r->GetProcessed() || p2.isNull()) {
                        continue;
                    }

                    //check for the crossing
                    if (each_portal->GetPlayerCrossed(player->GetV("eye_point"), player_lasteyepoint)) {
                        MovePlayer(each_portal, player, false);
                        break;
                    }
                }
            }
        }        
    }

    player->UpdateEyePoint();
    player_lasteyepoint = player->GetV("eye_point");
}

void Environment::UpdateAssets()
{
    QList <QPointer <Room> > visible_rooms = curnode->GetVisibleRooms();
    for  (int i=0; i<visible_rooms.size(); ++i) {
        visible_rooms[i]->UpdateAssets();
    }
}

void Environment::Update1(QPointer <Player> player)
{        
    if (curnode) {                
        curnode->UpdatePhysics(player);             

        Update_CrossPortals(player);

        //update JS
        if (curnode->GetReady()) {
            curnode->UpdateJS(player);
            curnode->UpdateAutoPlay();
        }
    }   
}

void Environment::Update2(QPointer <Player> player, MultiPlayerManager *multi_players)
{
    //qDebug() << "Environment2::Update" << curnode->GetRoom();
    if (rootnode.isNull() || curnode.isNull()) {
        return;
    }   

    const QString url = curnode->GetS("url");

    //load rooms
    QList <QPointer <Room> > rooms = rootnode->GetAllChildren();
    for (QPointer <Room> & r : rooms) {
        if (r && r->GetLoaded() && r->GetProcessing() && !r->GetProcessed()) {

            //create room            
            //59.6 - Resolve 302 URL with whatever existing URL was
            QUrl u(r->GetS("url"));
            QString s = QUrl::fromPercentEncoding(u.resolved(r->GetPage()->GetURL()).toString().toLatin1());
            r->SetS("url", s);
            r->SetProcessed(true);
            r->Create();

            const bool parent_node = (r->GetAllChildren().contains(curnode));
            bool stitched = false;

            // 1. this bit attempts to link to a portal already in the child room (portal cannot be a mirror)
            QHash <QString, QPointer <RoomObject> > & envobjects = r->GetRoomObjects();
            for (QPointer <RoomObject> & p : envobjects) {
                // p is the stitch portal here
                if (p && p->GetType() == "link" && !p->GetB("mirror") && p->GetS("url") == url) {

                    QPointer <RoomObject> p2; //portal in "my" room
                    QPointer <Room> other_room;
                    if (parent_node) {                        
                        // Parent stitch - r is a parent room to curnode (we modify curnode's things)
                        // We need the room whose parent is r
                        for (QPointer <Room> & r2 : rooms) {
                            // Ensure r2 is r's kid, and curnode is within some tree coming from r2
                            if (r2->GetParent() == r && r2->GetAllChildren().contains(curnode)) {
                                p2 = r2->GetEntranceObject();
                                if (r2->GetParentObject() && r2->GetParentObject() != p) {
                                    delete r2->GetParentObject();
                                }
                                r2->SetParentObject(p);
                                other_room = r2;
                                break;
                            }
                        }
                    }
                    else {
                        // Child stitch - r is a child room to curnode (we modify r's things)
                        p2 = r->GetParentObject();
                        if (r->GetEntranceObject() && r->GetEntranceObject() != p) {
                            delete r->GetEntranceObject();
                        }
                        r->SetEntranceObject(p);
                        other_room = r;
                    }

                    if (p2) {
                        p2->SetB("open", true);

                        p->SetV("scale", p2->GetScale());
                        p->SetB("open", true);
                        p->SetB("_circular", p2->GetB("_circular"));
                        p->SetC("col", p2->GetC("col"));
                        p->SetS("thumb_id", p2->GetS("thumb_id"));

                        QPointer <AssetImage> thumb_a = p2->GetThumbAssetImage();
                        if (other_room && thumb_a) {
                            QPointer <AssetImage> a = new AssetImage();
                            a->SetS("id", thumb_a->GetS("id"));
                            a->SetSrc(thumb_a->GetS("_base_url"), thumb_a->GetS("_src_url"));
                            a->Load();
                            other_room->AddAssetImage(a);
                            p->SetThumbAssetImage(a);
                        }
                    }

                    stitched = true;
                    break;
                }
            }

            // 2. if not stitched, the portal peers into the room based on room's entranceobject (since we didn't already find a match)
            if (!stitched) {
                QPointer <RoomObject> p;
                QPointer <RoomObject> p2;
                QPointer <Room> rc;
                if (parent_node) {
                    // Parent stitch - r is a parent room to curnode (we modify curnode's things)
                    // We need the room whose parent is r
                    for (QPointer <Room> & r2 : rooms) {
                        if (r2->GetParent() == r) {
                            rc = r2;
                            p = r2->GetParentObject();
                            p2 = r2->GetEntranceObject();
                            break;
                        }
                    }
                }
                else {
                    p = r->GetEntranceObject();
                    p2 = r->GetParentObject();
                    rc = r->GetParent();

                    //support anchors for parent rooms
                    if (r->GetPage()->GetURL().hasFragment()) { //anchor
                        const QString anchor = r->GetPage()->GetURL().fragment();
                        QPointer <RoomObject> oa = r->GetRoomObject(anchor);
                        if (oa) {
                            p->SetV("pos", oa->GetV("pos"));
                            p->SetXDirs(oa->GetXDir(), oa->GetYDir(), oa->GetZDir());
                        }
                    }
                }

                // p is in r, p2 is in "the connected room" or rc
                if (p && p2) {
                    p2->SetB("open", true);

                    if (r->GetPage()) {
                        p2->SetTitle(r->GetPage()->GetTitle());
                    }
                    else {
                        p2->SetTitle(r->GetS("title"));
                    }
                    p2->SetURL("", r->GetS("url"));
                    p2->SetB("_url_changed", false);

                    QPointer <AssetImage> thumb_a = p2->GetThumbAssetImage();
                    if (r && thumb_a) {
                        QPointer <AssetImage> a = new AssetImage();
                        a->SetS("id", thumb_a->GetS("id"));
                        a->SetSrc(thumb_a->GetS("_base_url"), thumb_a->GetS("_src_url"));
                        a->Load();
                        r->AddAssetImage(a);
                        p->SetThumbAssetImage(a);
                    }

                    p->SetS("js_id", "__entrance_portal_"+r->GetS("url"));

                    p->SetB("active", p2->GetB("active"));
                    p->SetB("visible", p2->GetB("visible"));

                    p->SetV("scale", p2->GetV("scale"));
                    p->SetB("open", true);
                    p->SetB("_circular", p2->GetB("_circular"));
                    p->SetC("col", p2->GetC("col"));
                    p->SetS("thumb_id", p2->GetS("thumb_id"));

                    if (rc) {
                        if (rc->GetPage()) {
                            p->SetTitle(rc->GetPage()->GetTitle());
                        }
                        else {
                            p->SetTitle(rc->GetS("title"));
                        }
                        p->SetURL("", rc->GetS("url"));
                    }
                    p->SetB("_url_changed", false);

                    //add the entranceportal to the room
                    if (r != rootnode) {
                        r->AddRoomObject(p);
                    }
                }
            }

            //3. for backportals, we only worry in the parent case, and the same thing always happens
            //  note: what if the parent's backportal was also stitched???
            if (parent_node && r != rootnode) {
                QPointer <RoomObject> ep = r->GetEntranceObject();
                QPointer <RoomObject> pp = r->GetParentObject();
                //release 60.0 - rely on parent visibility to show the launch portal
                if (ep && pp) {
                    ep->SetS("js_id", "__entrance_portal_"+r->GetS("url"));
                    ep->SetB("open", false);
                    ep->SetB("visible", pp->GetB("visible"));
                    ep->SetB("active", pp->GetB("active"));
                    r->AddRoomObject(ep);
                }
            }

            //once loaded (except for reloads), if the player is inside we set the pos to the markup
            if (r == curnode) {
                QPointer <RoomObject> o = r->GetEntranceObject();
                if (o && !r->GetB("_reloaded")) {
                    player->SetV("pos", o->GetV("pos") + o->GetV("zdir") * 0.5f);
                    player->SetV("dir", o->GetZDir());
                    player->UpdateDir();
                }
                r->SetB("_reloaded", false);
            }
        }
    }   

    //update objects only for adjacent rooms
    QList <QPointer <Room> > visible_rooms = curnode->GetVisibleRooms();
//    visible_rooms += rootnode;
    for  (int i=0; i<visible_rooms.size(); ++i) {
//        qDebug() << "Environment::Update2 visrooms" << i << visible_rooms[i]->GetURL();
        QPointer <Room> r = visible_rooms[i];
        const bool player_in_room = (r == curnode);
        r->UpdateObjects(player, multi_players, player_in_room);
        UpdateQueuedFunctions(r);

        //59.9 - reload a deallocated/cleared room if it's visible (we returned to some room that has an open portal to this room)
        if (!r->GetStarted()) {
            r->StartURLRequest();
            emit RoomsChanged();
        }
    }   

    //deallocation and desired subscribe/unsubscribe
    //on this server, and that port, this list of URLs
    QHash <QString, QHash <int, QSet <QString> > > connections;
    for (QPointer <Room> & r : rooms) {
        //60.0 - we deallocate very conservatively, only when Room has completed loading ("ready for screenshot")
        //qDebug() << r << r->GetURL() << r->GetReady() << !visible_rooms.contains(r) << (r != rootnode) << (r != curnode) << (r != lastnode);
        if (r && r->GetReady() && !visible_rooms.contains(r) && r != rootnode && r != curnode && r != lastnode) { //deallocate
            r->Clear();            
            emit RoomsChanged();
        }
        else if (r && visible_rooms.contains(r)) { //if not deallocating, increment do-not-unsubscribe counter
            const QString url = r->GetURL();
            const bool is_local_file = url.left(7).toLower() == "file://" || url.left(8).toLower() == "assets:/";
            if (!is_local_file) { //add if non-local
                connections[r->GetS("server")][r->GetS("port").toInt()].insert(url);
            }
        }
    }   
    multi_players->SetConnections(connections);

    //59.9 - update script logs (and optionally print to chat)
    //    qDebug() << "script_print_log size" << ScriptBuiltins::script_print_log.size();
    if (!ScriptBuiltins::script_print_log.empty()) {
        for (int i=0; i<ScriptBuiltins::script_print_log.size(); ++i) {
            if (ScriptBuiltins::script_print_log[i].output_to_chat) {
                multi_players->AddChatMessage(ScriptBuiltins::script_print_log[i].msg, QColor(255,0,255));
            }
            else {
                MathUtil::ErrorLog(ScriptBuiltins::script_print_log[i].msg);
            }
        }
        ScriptBuiltins::script_print_log.clear();
    }
}

void Environment::NavigateToRoom(QPointer <Player> player, QPointer <Room> r)
{    
    if (r.isNull()) {
        return;
    }

    SetCurRoom(player, r);

    QPointer <RoomObject> p = r->GetEntranceObject();
    if (p) {
        player->SetV("pos", p->GetV("pos")+p->GetZDir()*0.5f);
        player->SetV("dir", p->GetZDir());
        player->UpdateDir();
    }

#ifdef __ANDROID__
    // Close all portals
    const QHash <QString, QPointer <RoomObject> > & envobjects = GetCurRoom()->GetRoomObjects();
    for (auto & each_portal : envobjects) {
        if (each_portal && each_portal->GetType() == "link") {
            if (each_portal->GetB("open") && each_portal->GetB("visible") && !each_portal->GetB("auto_load")) {
                    each_portal->SetB("open", false);
                    if (curnode->GetConnectedPortal(each_portal))
                        curnode->GetConnectedPortal(each_portal)->SetB("open", false);
            }
        }
    }
#endif
}

void Environment::SetCurRoom(QPointer <Player> player, QPointer <Room> r)
{
    if (r.isNull()) {
        return;
    }   

    //silence/reset sounds for all rooms
    QList <QPointer <Room> > nodes = rootnode->GetAllChildren();
    for (int i=0; i<nodes.size(); ++i) {
        //59.0 stop all sounds, except for room we are going into
        if (r != nodes[i] && nodes[i] && nodes[i] != rootnode) {
            nodes[i]->StopAll();
            nodes[i]->ResetSoundTriggers();
        }
    }

    QPointer <RoomPhysics> physics = curnode->GetPhysics();
    if (physics) {
        physics->RemovePlayerShape();
        physics->RemoveGroundPlane();
    }
    curnode = r;
    curnode->SetPlayerInRoom(player);    

    if (curnode != rootnode) {
        lastnode = curnode;
    }

    emit RoomsChanged();
}

QPointer <Room> Environment::GetCurRoom()
{
    return curnode;
}

QPointer <Room> Environment::GetLastRoom()
{
    return lastnode;
}

QPointer <Room> Environment::GetRootRoom()
{
    return rootnode;
}

void Environment::Shutdown()
{
    QList <QPointer <Room> > nodes = rootnode->GetAllChildren();
    for (int i=0; i<nodes.size(); ++i) {
        if (nodes[i]) {
            nodes[i]->StopAll();
        }
    }    
}

void Environment::UpdateQueuedFunctions(QPointer <Room> r)
{
    QList <QScriptValue> & queued_funcs = r->GetQueuedFunctions();
//    qDebug() << "Environment::UpdateQueuedFunctions" << queued_funcs.size();

    //do queued functions
    for (int i=0; i<queued_funcs.size(); ++i) {

        const QString op = queued_funcs[i].property("op").toString();
        const QString name = queued_funcs[i].property("name").toString();

        QHash <QString, QPointer <RoomObject> > & envobjects = r->GetRoomObjects();
        QPointer <RoomObject> obj = r->GetRoomObject(name);                

        bool processed = false;

        if (op == "playSound") { //play
            QPointer <AssetSound> as = r->GetAssetSound(name);
            if (obj && obj->GetType() == "sound") {
                //js_id based case
                QPointer <AssetSound> snd = obj->GetAssetSound();
                if (snd && snd->GetReady(obj->GetMediaContext())) {
                    snd->SetSoundEnabled(obj->GetMediaContext(), SoundManager::GetEnabled());
                    obj->Play();
                    processed = true;
                }
            }
            else if (as) {
                //asset_id based case
                for (QPointer <RoomObject> & o : envobjects) {
                    QPointer <AssetSound> s = o->GetAssetSound();
                    if (o && s == as && s->GetReady(o->GetMediaContext())) {
                        as->Play(o->GetMediaContext());
                        processed = true;
                    }
                }
            }
            else {
                processed = true;
            }
        }
        else if (op == "seekSound") { //seek
            QPointer <AssetSound> as = r->GetAssetSound(name);
            if (obj && obj->GetType() == "sound") {
                //js_id based case
                QPointer <AssetSound> snd = obj->GetAssetSound();
                if (snd && snd->GetReady(obj->GetMediaContext())) {
                    if (queued_funcs[i].property("pos").isNumber()) {
                        obj->Seek(queued_funcs[i].property("pos").toNumber());
                    }
                    processed = true;
                }
            }
            else if (as) {
                //asset_id based case
                for (QPointer <RoomObject> & o : envobjects) {
                    QPointer <AssetSound> s = o->GetAssetSound();
                    if (o && s == as && s->GetReady(o->GetMediaContext())) {
                        if (queued_funcs[i].property("pos").isNumber()) {
                            as->Seek(o->GetMediaContext(), queued_funcs[i].property("pos").toNumber());
                        }
                        processed = true;
                    }
                }
            }
            else {
                processed = true;
            }
        }
        else if (op == "pauseSound") { //pause
            QPointer <AssetSound> as = r->GetAssetSound(name);
            if (obj && obj->GetType() == "sound") {
                //js_id based case
                QPointer <AssetSound> snd = obj->GetAssetSound();
                if (snd && snd->GetReady(obj->GetMediaContext())) {
                    obj->Pause();
                    processed = true;
                }
            }
            else if (as) {
                //asset_id based case
                for (QPointer <RoomObject> & o : envobjects) {
                    QPointer <AssetSound> s = o->GetAssetSound();
                    if (o && s == as && s->GetReady(o->GetMediaContext())) {
                        as->Pause(o->GetMediaContext());
                        processed = true;
                    }
                }
            }
            else {
                processed = true;
            }
        }
        else if (op == "stopSound") { //stop
            QPointer <AssetSound> as = r->GetAssetSound(name);
            if (obj && obj->GetType() == "sound") {
                //js_id based case
                QPointer <AssetSound> snd = obj->GetAssetSound();
                if (snd && snd->GetReady(obj->GetMediaContext())) {
                    obj->Stop();
                    processed = true;
                }
            }
            else if (as) {
                //asset_id based case
                for (QPointer <RoomObject> & o : envobjects) {
                    QPointer <AssetSound> s = o->GetAssetSound();
                    if (o && s == as && s->GetReady(o->GetMediaContext())) {
                        as->Stop(o->GetMediaContext());
                        processed = true;
                    }
                }
            }
            else {
                processed = true;
            }
        }
        else if (op == "playRecording") { //play
            if (r->GetAssetRecording(name)) {
                if (r->GetAssetRecording(name)->GetProcessed()) {
                    const bool loop = queued_funcs[i].property("loop").toBool();
                    r->GetAssetRecording(name)->Play(loop);
                    processed = true;
                }
            }
            else {
                processed = true;
            }
        }
        else if (op == "seekRecording") { //seek
            if (r->GetAssetRecording(name)) {
                if (r->GetAssetRecording(name)->GetProcessed()) {
                    if (queued_funcs[i].property("pos").isNumber()) {
                        const float pos = queued_funcs[i].property("pos").toNumber();
                        r->GetAssetRecording(name)->Seek(pos);
                    }
                    processed = true;
                }
            }
            else {
                processed = true;
            }
        }
        else if (op == "pauseRecording") { //pause
            if (r->GetAssetRecording(name)) {
                if (r->GetAssetRecording(name)->GetProcessed()) {
                    r->GetAssetRecording(name)->Pause();
                    processed = true;
                }
            }
            else {
                processed = true;
            }
        }
        else if (op == "stopRecording") { //stop
            if (r->GetAssetRecording(name)) {
                if (r->GetAssetRecording(name)->GetProcessed()) {
                    r->GetAssetRecording(name)->Stop();
                    processed = true;
                }
            }
            else {
                processed = true;
            }
        }
        else if (op == "playVideo") { //play
            QPointer <AssetVideo> av = r->GetAssetVideo(name);
            if (obj) {
                //js_id based case
                QPointer <AssetVideo> v = obj->GetAssetVideo();
                if (v && v->GetReady(obj->GetMediaContext())) {
                    v->Play(obj->GetMediaContext());
                    processed = true;
                }
            }
            else if (av) {
                //asset_id based case
                for (QPointer <RoomObject> & o : envobjects) {
                    QPointer <AssetVideo> v = o->GetAssetVideo();
                    if (o && v == av && v->GetReady(o->GetMediaContext())) {
                        av->Play(o->GetMediaContext());
                        processed = true;
                    }
                }
            }
            else {
                processed = true;
            }
        }
        else if (op == "seekVideo") { //seek           
            const float pos = queued_funcs[i].property("pos").toNumber();
            QPointer <AssetVideo> av = r->GetAssetVideo(name);
            if (obj) {
                //js_id based case
                QPointer <AssetVideo> v = obj->GetAssetVideo();
                if (v && v->GetReady(obj->GetMediaContext())) {
                    v->Seek(obj->GetMediaContext(), pos);
                    processed = true;
                }
            }
            else if (av) {
                //asset_id based case
                for (QPointer <RoomObject> & o : envobjects) {
                    QPointer <AssetVideo> v = o->GetAssetVideo();
                    if (o && v == av && v->GetReady(o->GetMediaContext())) {
                        av->Seek(o->GetMediaContext(), pos);
                        processed = true;
                    }
                }
            }
            else {
                processed = true;
            }

        }
        else if (op == "pauseVideo") { //pause
            QPointer <AssetVideo> av = r->GetAssetVideo(name);
            if (obj) {
                //js_id based case
                QPointer <AssetVideo> v = obj->GetAssetVideo();
                if (v && v->GetReady(obj->GetMediaContext())) {
                    v->Pause(obj->GetMediaContext());
                    processed = true;
                }
            }
            else if (av) {
                //asset_id based case
                for (QPointer <RoomObject> & o : envobjects) {
                    QPointer <AssetVideo> v = o->GetAssetVideo();
                    if (o && v == av && v->GetReady(o->GetMediaContext())) {
                        av->Pause(o->GetMediaContext());
                        processed = true;
                    }
                }
            }
            else {
                processed = true;
            }
        }
        else if (op == "stopVideo") { //stop
            QPointer <AssetVideo> av = r->GetAssetVideo(name);
            if (obj) {
                //js_id based case
                QPointer <AssetVideo> v = obj->GetAssetVideo();
                if (v && v->GetReady(obj->GetMediaContext())) {
                    v->Stop(obj->GetMediaContext());
                    processed = true;
                }
            }
            else if (av) {
                //asset_id based case
                for (QPointer <RoomObject> & o : envobjects) {
                    QPointer <AssetVideo> v = o->GetAssetVideo();
                    if (o && v == av && v->GetReady(o->GetMediaContext())) {
                        av->Stop(o->GetMediaContext());
                        processed = true;
                    }
                }
            }
            else {
                processed = true;
            }
        }
        else if (op == "openLink") {
            if (obj && !obj->GetB("open")) {
                AddRoom(obj);
                obj->SetB("open", true);
            }
            processed = true;
        }
        else if (op == "closeLink") {
            if (obj) {
                if (ClearRoom(obj)) {
                    obj->SetB("open", false);
                }
                obj->SetB("_auto_load_triggered", false);
            }
            processed = true;
        }
        else if (op == "removeObject") {
            const bool do_sync = queued_funcs[i].property("sync").toBool();
            r->DeleteSelected(name, do_sync, false); //60.0 - delete and do not play sound
            processed = true;
        }

        if (processed || name == "") {
            queued_funcs.removeAt(i);
            --i;
        }
    }
}
