#include "virtualmenu.h"

VirtualMenu::VirtualMenu() :
    menu_index(VirtualMenuIndex_MAIN),
    visible(false),
    taking_screenshot(false),
    cur_bookmark(0),
    num_bookmarks(0),
    cur_user(0),
    num_users(0),
    do_back(false),
    do_forward(false),
    do_reload(false),
    do_escape_to_home(false),
    do_exit(false),
    do_create_portal(false),
    do_bookmark_add(false),
    do_bookmark_remove(false)
{
    assetobjs["cube"] = QPointer<AssetObject>(new AssetObject());
    assetobjs["cube"]->SetSrc(MathUtil::GetApplicationURL(), QString("assets/primitives/cube.obj"));
    assetobjs["cube"]->GetProperties()->SetID("cube");
    assetobjs["cube"]->GetProperties()->SetPrimitive(true);
    assetobjs["cube"]->GetProperties()->SetSaveToMarkup(false);
    assetobjs["cube"]->Load();

    connect(&partymode_request_timer, SIGNAL(timeout()), this, SLOT(UpdatePartyModeList()));
    partymode_request_timer.start(5000);
}

VirtualMenu::~VirtualMenu()
{
    Clear();
    disconnect(&partymode_request_timer, 0, 0, 0);
}

void VirtualMenu::SetBookmarkManager(QPointer <BookmarkManager> b)
{
    bookmarkmanager = b;
}

void VirtualMenu::SetMultiPlayerManager(QPointer <MultiPlayerManager> m)
{
    multi_players = m;
}

VirtualMenuButton * VirtualMenu::AddNewButton(const VirtualMenuIndex index, const QString js_id, const QString label, const QMatrix4x4 m)
{
    VirtualMenuButton * b = new VirtualMenuButton(js_id, label, m);

    b->button->SetAssetObject(assetobjs["cube"]);
    b->button->SetCollisionAssetObject(assetobjs["cube"]);
    b->label->SetAssetObject(assetobjs["cube"]);

    envobjects[index][b->button->GetProperties()->GetJSID()] = b->button;
    envobjects_text[index][b->label->GetProperties()->GetJSID()] = b->label;
    return b;
}

VirtualMenuImageButton * VirtualMenu::AddNewImageButton(const VirtualMenuIndex index, const QString js_id, const QString url, const QString thumb_id, const QMatrix4x4 m)
{
    if (!assetimgs.contains(thumb_id)) {
        assetimgs[thumb_id] = QPointer<AssetImage>(new AssetImage());
        assetimgs[thumb_id]->SetSrc(thumb_id, thumb_id);
        assetimgs[thumb_id]->Load();
    }

    VirtualMenuImageButton * b = new VirtualMenuImageButton(js_id, url, thumb_id, m);
    b->button->SetAssetObject(assetobjs["cube"]);
    b->button->SetCollisionAssetObject(assetobjs["cube"]);
    b->button->SetAssetImage(assetimgs[thumb_id]);

    envobjects[index][b->button->GetProperties()->GetJSID()] = b->button;
    envobjects_text[index][b->label->GetProperties()->GetJSID()] = b->label;

    return b;
}

VirtualMenuImageUserButton * VirtualMenu::AddNewImageUserButton(const VirtualMenuIndex index, const QString js_id, const QString user, const QString url, const QString thumb_id, const QMatrix4x4 m)
{
    if (!assetimgs.contains(thumb_id)) {
        assetimgs[thumb_id] = QPointer<AssetImage>(new AssetImage());
        assetimgs[thumb_id]->SetSrc(thumb_id, thumb_id);
        assetimgs[thumb_id]->Load();
    }

    VirtualMenuImageUserButton * b = new VirtualMenuImageUserButton(js_id, user, url, thumb_id, m);
    b->button->SetAssetObject(assetobjs["cube"]);
    b->button->SetCollisionAssetObject(assetobjs["cube"]);
    b->button->SetAssetImage(assetimgs[thumb_id]);

    envobjects[index][b->button->GetProperties()->GetJSID()] = b->button;
    envobjects_text[index][b->labelurl->GetProperties()->GetJSID()] = b->labelurl;
    envobjects_text[index][b->labeluser->GetProperties()->GetJSID()] = b->labeluser;

    return b;
}

void VirtualMenu::SetVisible(const bool b)
{
    visible = b;
}

bool VirtualMenu::GetVisible() const
{
    return visible;
}

void VirtualMenu::SetTakingScreenshot(const bool b)
{
    taking_screenshot = b;
}

bool VirtualMenu::GetTakingScreenshot() const
{
    return taking_screenshot;
}

void VirtualMenu::Update()
{
    //hide menu if we moved
    const QString cur_url = multi_players->GetCurURL();
    if (cur_url != last_url) {
        visible = false;
    }
    last_url = cur_url;

    for (QPointer <AssetImage> & a : assetimgs) {
        if (a) {
            a->UpdateGL();
        }
    }

    for (QPointer <AssetObject> & a : assetobjs) {
        if (a) {
            a->Update();
            a->UpdateGL();
        }
    }

    if (partymode_data_request.GetLoaded() && !partymode_data_request.GetProcessed()) {
        const QByteArray & ba = partymode_data_request.GetData();
        MathUtil::GetPartyModeData() = QJsonDocument::fromJson(ba).toVariant().toMap()["data"].toList();
        partymode_data_request.SetProcessed(true);

        ConstructSubmenus();
    }
}

void VirtualMenu::DrawGL(QPointer <AssetShader> shader)
{
    for (QPointer <RoomObject> & o : envobjects[menu_index]) {
        if (o) {
            o->Update(0.0f);
            o->DrawGL(shader, true, QVector3D(0,0,0));
        }
    }

    for (QPointer <RoomObject> & o : envobjects_text[menu_index]) {
        if (o) {
            o->Update(0.0f);
            o->DrawGL(shader, true, QVector3D(0,0,0));
        }
    }

    if (menu_index == VirtualMenuIndex_AVATAR) {
        MathUtil::PushModelMatrix();
        MathUtil::MultModelMatrix(modelmatrix);
        QPointer <RoomObject> player_avatar = multi_players->GetPlayer();
        if (player_avatar) {
            QVector <GhostFrame> frames;
            frames.push_back(GhostFrame());
            frames.push_back(GhostFrame());
            frames[0].time_sec = 0.0f;
            frames[1].time_sec = 1.0f;
            if (player_avatar->GetAssetGhost()) {
                player_avatar->GetAssetGhost()->SetFromFrames(frames, 1000);
            }

            const QVector3D p = player_avatar->GetPos();
            const QString s = player_avatar->GetHMDType();
            player_avatar->SetHMDType("");
            player_avatar->GetProperties()->SetPos(QVector3D(0,0,0));
            player_avatar->Update(0.0f);
            player_avatar->DrawGL(shader, true, QVector3D(0,0,0));
            player_avatar->SetHMDType(s);

            player_avatar->GetProperties()->SetPos(p);
        }

        MathUtil::PopModelMatrix();
    }
}

QHash <QString, QPointer <RoomObject> > & VirtualMenu::GetEnvObjects()
{
    return envobjects[menu_index];
}

void VirtualMenu::mousePressEvent(const QString selected)
{

}

void VirtualMenu::mouseReleaseEvent(const QString selected)
{
        switch (menu_index) {
        case VirtualMenuIndex_MAIN:
            if (selected == "__url") {
                entered_url = "http://";
                if (envobjects_text[VirtualMenuIndex_URL]["__enteredurl_label"]) {
                    envobjects_text[VirtualMenuIndex_URL]["__enteredurl_label"]->SetText(entered_url);
                }
                menu_index = VirtualMenuIndex_URL;
            }
            else if (selected == "__back") {
                do_back = true;
            }
            else if (selected == "__forward") {
                do_forward = true;
            }
            else if (selected == "__reload") {
                do_reload = true;
            }
            else if (selected == "__home") {
                do_escape_to_home = true;
            }
            else if (selected == "__bookmarks") {
                menu_index = VirtualMenuIndex_BOOKMARKS;
            }
            else if (selected == "__avatar") {
                menu_index = VirtualMenuIndex_AVATAR;
            }
            else if (selected == "__social") {
                menu_index = VirtualMenuIndex_SOCIAL;
            }
            else if (selected == "__exit") {
                do_exit = true;
            }
            break;
        case VirtualMenuIndex_URL:
            if (selected == "__backspace") {
                entered_url = entered_url.left(entered_url.length()-1);
                if (envobjects_text[VirtualMenuIndex_URL]["__enteredurl_label"]) {
                    envobjects_text[VirtualMenuIndex_URL]["__enteredurl_label"]->SetText(entered_url);
                }
            }
            else if (selected == "__enter") {
                menu_index = VirtualMenuIndex_MAIN;
                do_create_portal = true;
                create_portal_url = entered_url;
            }
            else if (selected == "__enteredurl") {

            }
            else {
                entered_url += selected.right(1);
                if (envobjects_text[VirtualMenuIndex_URL]["__enteredurl_label"]) {
                    envobjects_text[VirtualMenuIndex_URL]["__enteredurl_label"]->SetText(entered_url);
                }
            }
        case VirtualMenuIndex_BOOKMARKS:
            if (selected == "__bookmarkadd") {
                do_bookmark_add = true;
            }
            else if (selected == "__bookmarkremove") {
                do_bookmark_remove = true;
            }
            else if (selected == "__bookmarkup") {
                if (cur_bookmark+16 < num_bookmarks) {
                    cur_bookmark += 16;
                    ConstructSubmenus();
                }
            }
            else if (selected == "__bookmarkdown") {
                menu_index = VirtualMenuIndex_BOOKMARKS;
                if (cur_bookmark>0) {
                    cur_bookmark -= 16;
                    ConstructSubmenus();
                }
            }
            else {
                QPointer <RoomObject> o = envobjects[VirtualMenuIndex_BOOKMARKS][selected];
                if (o) {
                    menu_index = VirtualMenuIndex_MAIN;
                    do_create_portal = true;
                    create_portal_url = o->GetProperties()->GetURL();
                    create_portal_thumb = o->GetAssetImage() ? o->GetAssetImage()->GetURL().toString() : "";
                }
            }
            break;
        case VirtualMenuIndex_SOCIAL:
            if (selected == "__socialup") {
                if (cur_user+16 < num_users) {
                    cur_user += 16;
                    ConstructSubmenus();
                }
            }
            else if (selected == "__socialdown") {
                menu_index = VirtualMenuIndex_BOOKMARKS;
                if (cur_user>0) {
                    cur_user -= 16;
                    ConstructSubmenus();
                }
            }
            else {
                QPointer <RoomObject> o = envobjects[VirtualMenuIndex_SOCIAL][selected];
                if (o) {
                    menu_index = VirtualMenuIndex_MAIN;
                    do_create_portal = true;
                    create_portal_url = o->GetProperties()->GetURL();
                    create_portal_thumb = o->GetAssetImage() ? o->GetAssetImage()->GetURL().toString() : "";
                }
            }
            break;
        default:
            break;
        }
}

void VirtualMenu::SetModelMatrix(const QMatrix4x4 m)
{
    modelmatrix = m;
}

QMatrix4x4 VirtualMenu::GetModelMatrix() const
{
    return modelmatrix;
}

bool VirtualMenu::GetDoBack()
{
    if (do_back) {
        do_back = false;
        return true;
    }
    else {
        return false;
    }
}

bool VirtualMenu::GetDoForward()
{
    if (do_forward) {
        do_forward = false;
        return true;
    }
    else {
        return false;
    }
}

bool VirtualMenu::GetDoReload()
{
    if (do_reload) {
        do_reload = false;
        return true;
    }
    else {
        return false;
    }
}

bool VirtualMenu::GetDoEscapeToHome()
{
    if (do_escape_to_home) {
        do_escape_to_home = false;
        return true;
    }
    else {
        return false;
    }
}

bool VirtualMenu::GetDoExit()
{
    if (do_exit) {
        do_exit = false;
        return true;
    }
    else {
        return false;
    }
}

bool VirtualMenu::GetDoCreatePortal()
{
    if (do_create_portal) {
        do_create_portal = false;
        return true;
    }
    else {
        return false;
    }
}

QString VirtualMenu::GetDoCreatePortalURL()
{
    return create_portal_url;
}

QString VirtualMenu::GetDoCreatePortalThumb()
{
    return create_portal_thumb;
}

bool VirtualMenu::GetDoBookmarkAdd()
{
    if (do_bookmark_add) {
        do_bookmark_add = false;
        return true;
    }
    else {
        return false;
    }
}

bool VirtualMenu::GetDoBookmarkRemove()
{
    if (do_bookmark_remove) {
        do_bookmark_remove = false;
        return true;
    }
    else {
        return false;
    }
}

void VirtualMenu::MenuButtonPressed()
{
    if (!visible) {
        visible = true;
        ConstructSubmenus();
    }
    else {
        switch (menu_index) {
        case VirtualMenuIndex_MAIN:
            visible = false;
            break;
        default:
            menu_index = VirtualMenuIndex_MAIN;
            break;
        }
    }
}

void VirtualMenu::Clear()
{
    envobjects.clear();
    envobjects_text.clear();
}

void VirtualMenu::ConstructSubmenus()
{
    Clear();

    //construct submenus
    ConstructSubmenuMain();
    ConstructSubmenuURL();
    ConstructSubmenuBookmarks();
    ConstructSubmenuAvatar();
    ConstructSubmenuSocial();
}

void VirtualMenu::ConstructSubmenuMain()
{
    QMatrix4x4 m = modelmatrix;
    m.translate(0,2.25f,0);

    AddNewButton(VirtualMenuIndex_MAIN, "__url", multi_players->GetCurURL(), m);
    m.translate(0,-0.25f,0);

    AddNewButton(VirtualMenuIndex_MAIN, "__back", "Back", m);
    m.translate(0,-0.25f,0);

    AddNewButton(VirtualMenuIndex_MAIN, "__forward", "Forward", m);
    m.translate(0,-0.25f,0);

    AddNewButton(VirtualMenuIndex_MAIN, "__reload", "Reload", m);
    m.translate(0,-0.25f,0);

    AddNewButton(VirtualMenuIndex_MAIN, "__home", "Home", m);
    m.translate(0,-0.25f,0);

    AddNewButton(VirtualMenuIndex_MAIN, "__bookmarks", "Bookmarks", m);
    m.translate(0,-0.25f,0);

    AddNewButton(VirtualMenuIndex_MAIN, "__avatar", "Avatar", m);
    m.translate(0,-0.25f,0);

    AddNewButton(VirtualMenuIndex_MAIN, "__social", "Social", m);
    m.translate(0,-0.25f,0);

    AddNewButton(VirtualMenuIndex_MAIN, "__exit", "Exit", m);
    m.translate(0,-0.25f,0);
}

void VirtualMenu::ConstructSubmenuURL()
{
    QMatrix4x4 m = modelmatrix;
    m.translate(0,1.75f,0);
    m.scale(4,1,1);
    VirtualMenuButton * b = AddNewButton(VirtualMenuIndex_URL, "__enteredurl", entered_url, m);
    b->label->GetProperties()->SetJSID("__enteredurl_label");

    QList <QString> rows;
    rows.push_back("~1234567890-_+");
    rows.push_back("qwertyuiop");
    rows.push_back("asdfghjkl:");
    rows.push_back("zxcvbnm,./");

    for (int i=0; i<rows.size(); ++i) {
        QMatrix4x4 m = modelmatrix;
        m.translate(-1.75f, 1.5f - i*0.25f,0);
        m.scale(0.25f, 1, 1);

        if (i == 1) {
            m.translate(1.5f,0,0);
        }
        else if (i == 2) {
            m.translate(2.0f,0,0);
        }
        else if (i == 3) {
            m.translate(2.5f,0,0);
        }

        for (int j=0; j<rows[i].length(); ++j) {
            AddNewButton(VirtualMenuIndex_URL, "__" + rows[i].mid(j,1), rows[i].mid(j,1), m);
            m.translate(1.05f,0,0);
        }

        if (i == 0) {
            m.translate(-0.5f,0,0);
            m.scale(2,1,1);
            m.translate(0.5f,0,0);
            AddNewButton(VirtualMenuIndex_URL, "__backspace", "Backspace", m);
        }
        else if (i == 2) {
            m.translate(-0.5f,0,0);
            m.scale(2,1,1);
            m.translate(0.5f,0,0);
            VirtualMenuButton * b = AddNewButton(VirtualMenuIndex_URL, "__enter", "Enter", m);
            b->button->GetProperties()->SetColour(QVector4D(0.5f,1.0f,0.5f,1.0f));
        }
    }
}

void VirtualMenu::ConstructSubmenuBookmarks()
{
    if (bookmarkmanager && multi_players) {
        const QString cur_url = multi_players->GetCurURL();
        const bool bookmarked = bookmarkmanager->GetBookmarked(cur_url);

        QMatrix4x4 m = modelmatrix;
        m.translate(0,2.55f,0);
        if (bookmarked) {
            AddNewButton(VirtualMenuIndex_BOOKMARKS, "__bookmarkremove", "Remove current URL", m);
        }
        else {
            AddNewButton(VirtualMenuIndex_BOOKMARKS, "__bookmarkadd", "Add current URL", m);
        }

        QVariantList list = bookmarkmanager->GetBookmarks() + bookmarkmanager->GetWorkspaces();
        num_bookmarks = list.length();

//        qDebug() << "VirtualMenu::ConstructSubmenuBookmarks()" << cur_bookmark << num_bookmarks;
        if (cur_bookmark > 0) {
            QMatrix4x4 m_down = modelmatrix;
            m_down.translate(-1.35f, 1.25f, 0.0f);
            m_down.scale(0.5f, 1.0f, 1.0f);
            AddNewButton(VirtualMenuIndex_BOOKMARKS, "__bookmarkdown", "<-", m_down);
        }

        if (cur_bookmark + 16 < num_bookmarks) {
            QMatrix4x4 m_up = modelmatrix;
            m_up.translate(1.35f, 1.25f, 0.0f);
            m_up.scale(0.5f, 1.0f, 1.0f);
            AddNewButton(VirtualMenuIndex_BOOKMARKS, "__bookmarkup", "->", m_up);
        }

        for (int i=cur_bookmark; i<qMin(num_bookmarks, cur_bookmark+16); ++i) {
            int x = i % 4;
            int y = (i/4) % 4;
            QMatrix4x4 m = modelmatrix;
            m.translate(x*0.55f-0.85f, 2.1f-y*0.55f, 0);
            m.scale(0.5f, 0.5f, 1.0f);

            QMap <QString, QVariant> o = list[i].toMap();
            const QString url = o["url"].toString();
            const QString thumbnail = o["thumbnail"].toString();
            VirtualMenuImageButton * v = AddNewImageButton(VirtualMenuIndex_BOOKMARKS, "__bookmark" + QString::number(i), url, thumbnail, m);
        }
    }
}

void VirtualMenu::ConstructSubmenuAvatar()
{

}

void VirtualMenu::ConstructSubmenuSocial()
{
//    qDebug() << "VirtualMenu::ConstructSubmenuSocial()";
    QVariantList & d = MathUtil::GetPartyModeData();
    num_users = d.size();

    if (cur_user > 0) {
        QMatrix4x4 m_down = modelmatrix;
        m_down.translate(-1.35f, 1.25f, 0.0f);
        m_down.scale(0.5f, 1.0f, 1.0f);
        AddNewButton(VirtualMenuIndex_SOCIAL, "__socialdown", "<-", m_down);
    }

    if (cur_user + 9 < num_users) {
        QMatrix4x4 m_up = modelmatrix;
        m_up.translate(1.35f, 1.25f, 0.0f);
        m_up.scale(0.5f, 1.0f, 1.0f);
        AddNewButton(VirtualMenuIndex_SOCIAL, "__socialup", "->", m_up);
    }

    for (int i=cur_user; i<qMin(num_users, cur_user+9); ++i) {
        int x = i % 3;
        int y = (i/3) % 3;
        QMatrix4x4 m = modelmatrix;
        m.translate(x*0.65f-0.65f, 2.0f-y*0.65f, 0);
        m.scale(0.6f, 0.6f, 1.0f);

        QMap<QString, QVariant> map = d[i].toMap();
        const QString userid = map["userId"].toString();
        const QString url = map["url"].toString();

        const QString thumb_id = "https://thumbnails.janusvr.com/" + MathUtil::MD5Hash(url) + "/thumb.jpg";
        AddNewImageUserButton(VirtualMenuIndex_SOCIAL, "__user"+QString::number(i), userid, url, thumb_id, m);
    }
}

void VirtualMenu::UpdatePartyModeList()
{
//    qDebug() << "VirtualMenu::UpdatePartyModeList()";
    //if not visible
    if (!MathUtil::GetPartyModeData().isEmpty()) {
        if (!visible || menu_index != VirtualMenuIndex_SOCIAL) {
            return;
        }
    }

    if (!partymode_data_request.GetStarted() || partymode_data_request.GetProcessed()) {
        partymode_data_request.Load(QUrl("http://api.janusvr.com/partymodeAPI"));
    }

}
