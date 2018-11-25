#include "menu.h"

void MenuOperations::UpdatePageWithJanusObject(QPointer <AbstractWebSurface> webpage)
{
#ifndef __ANDROID__
    QPointer <CEFWebView> w = dynamic_cast<CEFWebView *>(webpage->GetWebView().data());

    if (w.isNull()) {
//        qDebug() << "cefwebview w null";
        return;
    }

//    qDebug() << bookmarks;
//    qDebug() << QJsonDocument::fromVariant(bookmarks).toJson(QJsonDocument::Compact);
//    qDebug() << "Test" << QJsonDocument::fromVariant(getpartymodedata()).toJson(QJsonDocument::Compact);

    QString bc;
//    bc += "window.janus = {};\n";
    bc += "window.janus.updatepartymodedata = function() { };\n";
    bc += "window.janus.updatepopulardata = function() { };\n";
    bc += "window.janus.populardata = " + QJsonDocument::fromVariant(popular_data).toJson(QJsonDocument::Compact) + ";\n";
    bc += "window.janus.partymodedata = " + QJsonDocument::fromVariant(partymode_data).toJson(QJsonDocument::Compact) + ";\n";
    bc += "window.janus.bookmarks = " + QJsonDocument::fromVariant(bookmarks).toJson(QJsonDocument::Compact) + ";\n";
    bc += "window.janus.workspaces = " + QJsonDocument::fromVariant(workspaces).toJson(QJsonDocument::Compact) + ";\n";
    bc += "window.janus.playbackdevices = " + QJsonDocument::fromVariant(playbackdevices).toJson(QJsonDocument::Compact) + ";\n";
    bc += "window.janus.capturedevices = " + QJsonDocument::fromVariant(capturedevices).toJson(QJsonDocument::Compact) + ";\n";
    bc += "window.janus.playerlist = " + QJsonDocument::fromVariant(playerlist).toJson(QJsonDocument::Compact) + ";\n";
    bc += "window.janus.playercount = " + QString::number(playercount) + ";\n";
    bc += "window.janus.networkstatus = " + QString::number(networkstatus) + ";\n";
    bc += "window.janus.networkerror = \"" + networkerror + "\";\n";
    bc += "window.janus.roomserver = \"" + roomserver + "\";\n";
    bc += "window.janus.userid = \"" + userid + "\";\n";

    w->evaluateJavaScript(bc);

//    qDebug() << popular_data;

    getpopulardata();
    getpartymodedata();

    if (timer.elapsed() > 5000) {
        updatepopulardata();
        updatepartymodedata();
        timer.restart();
    }

    const QString l = w->GetLaunchURL();
    if (!l.isEmpty()) {
        do_launchurl = true;
        do_launchurl_url = l;
        do_launchurl_useportal = false;
    }

//    CefRefPtr<CefBrowser> browser = w->GetBrowser();
//    if (browser == NULL) {
//        qDebug() << "browser null";
//        return;
//    }
//    CefRefPtr<CefV8Context> v8 = browser->GetMainFrame()->GetV8Context();
//    if (v8 == NULL) {
//        qDebug() << "v8 null" << CefV8Context::GetCurrentContext();
//        return;
//    }
//    else {
//        qDebug() << "got v8!";
//    }

//    const bool entered = v8->Enter();

//    CefRefPtr<CefV8Value> global = v8->GetGlobal();
//    if (global == NULL) {
//        qDebug() << "global null";
//    }

//    const bool exited = v8->Exit();

//    qDebug() << "MenuOperations::UpdatePageWithJanusObject" << entered << exited;

//    CefRefPtr<CefV8Value> janus_obj = CefV8Value::CreateObject(NULL);
    //obj->SetValue("myval", CefV8Value::CreateString("My String!"));



//    QString version;
//    QString versiononline;
//    QString currentkey;
//    QVariantList chat;
//    int networkstatus;
//    QString networkerror;
//    QString roomserver;
//    int playercount;
//    QVariantList bookmarks;
//    QVariantList workspaces;
//    QVariantList playbackdevices;
//    QVariantList capturedevices;
//    QVariantList playerlist;
////    QVariantMap settings;
//    QString userid;
//    bool hmd;

//    bool do_navback;
//    bool do_navforward;
//    bool do_navhome;
//    bool do_launchurl;
//    QString do_launchurl_url;
//    int do_launchurl_useportal;
//    bool do_chatsend;
//    QString do_chatsend_msg;
//    bool do_sync;
//    bool do_quit;
//    bool do_saveroom;
//    bool do_focus;
//    bool do_unfocus;
//    bool do_saveworkspace;
//    QString do_saveworkspacename;

//    QPointer <Room> player_curroom;
//    QString selected;
//    Environment * env;
//    QPointer <MultiPlayerManager> multi_players;
//    QPointer <Player> player;

//    WebAsset popular_data_request;
//    QVariantList popular_data;

//    WebAsset partymode_data_request;
//    QVariantList partymode_data;

//    float near_dist;

//    webpage->GetWebView()->addToJavaScriptWindowObject("janus", this);
//    webpage->GetWebView()->addToChildFramesJavaScriptWindowObject("janus", this);
#endif
}

Q_INVOKABLE void MenuOperations::updatepopulardata(QString s)
{
//    qDebug() << "updatepopulardata()" << popular_data_request.GetStarted() << popular_data_request.GetProcessed() << s;
    if (!popular_data_request.GetStarted() || popular_data_request.GetProcessed()) {
        popular_data_request.Load(QUrl("http://api.janusvr.com/getPopularRooms"+s));
    }
}

Q_INVOKABLE QVariantList MenuOperations::getpopulardata()
{
//    qDebug() << "MenuOperations::getpopulardata()";
    if (popular_data_request.GetLoaded() && !popular_data_request.GetProcessed()) {
        const QByteArray & ba = popular_data_request.GetData();
        popular_data = QJsonDocument::fromJson(ba).toVariant().toMap()["data"].toList();
        popular_data_request.SetProcessed(true);
    }
    return popular_data;
}

Q_INVOKABLE void MenuOperations::updatepartymodedata()
{
//    qDebug() << "updatepartymodedata()" << partymode_data_request.GetStarted() << partymode_data_request.GetProcessed();
    if (!partymode_data_request.GetStarted() || partymode_data_request.GetProcessed()) {
        partymode_data_request.Load(QUrl("https://vesta.janusvr.com/api/party_mode"));
    }
}

Q_INVOKABLE QVariantList MenuOperations::getpartymodedata()
{
//        qDebug() << "getpartymodedata()" << partymode_data_request.GetLoaded() << partymode_data_request.GetProcessed();
    if (partymode_data_request.GetLoaded() && !partymode_data_request.GetProcessed()) {
        const QByteArray & ba = partymode_data_request.GetData();
//            QJsonParseError e;
//            QJsonDocument::fromJson(ba, &e);
//            qDebug() << e.errorString();
        partymode_data = QJsonDocument::fromJson(ba).toVariant().toMap()["data"].toList();
//            qDebug() << partymode_data;
//            qDebug() << "got here" << partymode_data;
        partymode_data_request.SetProcessed(true);
    }
    return partymode_data;
}
