#ifndef VIRTUALKEYBOARD_H
#define VIRTUALKEYBOARD_H

#include "assetobject.h"
#include "roomobject.h"
#include "player.h"

class VirtualKeyboard : public QObject
{

public:

    VirtualKeyboard();

    void SetWebSurface(QPointer <AbstractWebSurface> w);
    QPointer <AbstractWebSurface> GetWebSurface() const;

    void Update();
    void DrawGL(QPointer <AssetShader> shader);

    QHash <QString, QPointer <RoomObject> > & GetEnvObjects();

    void mousePressEvent(const int index);
    void mouseReleaseEvent(const int index);

    void SetShiftModifier(const bool b);
    bool GetShiftModifier();

    bool GetCapsLock();

    Qt::Key GetKeySelected(const QString jsid);

    void SetVisible(const bool b);
    bool GetVisible() const;

    void SetSelected(QPointer <RoomObject> o, const int index);
    QPointer <RoomObject> GetSelected(const int index) const;

    void SetModelMatrix(const QMatrix4x4 m);

private:

    void UpdateAssets();

    QHash <Qt::Key, QPointer <AssetObject> > assetobjs;
    QPointer <AssetImage> key_img;
    QPointer <AssetImage> keypress_img;
    QPointer <AssetImage> keyhover_img;

    QHash <Qt::Key, QString> keynames;    
    QPointer <AbstractWebSurface> websurface;

    QHash <QString, QPointer <RoomObject> > envobjects; //indexes to envobject via js_id   
    QHash <QString, Qt::Key> jstokeys;   

    bool shift_modifier;
    bool control_modifier;
    bool alt_modifier;
    bool capslock;

    bool visible;
    bool mouse_pressed[2];

    QPointer <RoomObject> cur_selected[2];

    QMatrix4x4 modelmatrix;
};

#endif // VIRTUALKEYBOARD_H
