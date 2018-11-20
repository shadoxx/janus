#ifndef CONTROLLERMANAGER_H
#define CONTROLLERMANAGER_H

#include <QtCore>

#include "abstracthmdmanager.h"

#if !defined(__APPLE__) && !defined(__ANDROID__)
    #include "gamepad.h"
#elif defined(__ANDROID__)

enum GAMEPAD_DEVICE {
    GAMEPAD_0 = 0,	/**< First gamepad */
    GAMEPAD_1 = 1,	/**< Second gamepad */
    GAMEPAD_2 = 2,	/**< Third gamepad */
    GAMEPAD_3 = 3,	/**< Fourth gamepad */

    GAMEPAD_COUNT	/**< Maximum number of supported gamepads */
};

#endif

#include "assetobject.h"
#include "roomobject.h"

struct ControllerButtonState
{
    ControllerButtonState():
        hover(false),
        value(0.0f),
        pressed(false),
        proc_press(false),
        proc_release(false)
    {

    }

    bool hover;
    float value;
    bool pressed;
    bool proc_press;
    bool proc_release;
};

struct ControllerState
{
    ControllerState():
        active(false),
        x(0.0f),
        y(0.0f),
        x1(0.0f),
        y1(0.0f),
        laser_length(0.0f)
    {

    }

    ControllerButtonState & GetClick()
    {
        return t[0];
    }

    ControllerButtonState & GetGrab()
    {
        return t[1];
    }

    ControllerButtonState & GetTeleport()
    {
        return b[0];
    }

    ControllerButtonState & GetHome()
    {
        return b[1];
    }

    bool active;
    QMatrix4x4 xform;
    QMatrix4x4 xform_prev;
    float x;
    float y;
    float x1; //some controllers (e.g. wmxr) have multiple thumb-driven 2-axis inputs
    float y1;
    ControllerButtonState b[7];
    ControllerButtonState t[2];    
    float laser_length;
#ifdef __ANDROID__
    float laser_angle;
#endif
};

class ControllerManager : public QObject
{

public:

    ControllerManager();
    ~ControllerManager();

    void SetHMDManager(QPointer <AbstractHMDManager> m);
    QPointer <AbstractHMDManager> GetHMDManager();

    void Update(const bool use_gamepad);

    ControllerState * GetStates();   

    bool GetUsingGamepad() const;
    bool GetUsingSpatiallyTrackedControllers() const;       

    static void DrawGhostGL(QPointer <AssetShader> shader, const int i, const QString hmd_type, const QMatrix4x4 c);
    void DrawGL(QPointer <AssetShader> shader, QMatrix4x4 player_xform);

    void SetHapticsEnabled(const bool b);
    void TriggerHapticPulse(const int i, const int val);

    static float GetAxisThreshold();

private:
#if !defined(__APPLE__)
    void UpdateGamepad(GAMEPAD_DEVICE dev = GAMEPAD_0);
#endif
    void UpdateControllers();
    void UpdateAssets();

    bool using_gamepad;

    bool haptics;
    ControllerState s[2];
    QPointer <AbstractHMDManager> hmd_manager;    

    static QPointer <AssetObject> vive_obj;
    static QVector <QPointer <AssetObject> > touch_obj;
    static QPointer <AssetObject> daydream_obj;
    static QPointer <AssetObject> go_obj;
    static QPointer <AssetObject> gear_obj;
    static QVector <QPointer <AssetObject> > wmxr_obj;
    static QPointer <AssetObject> laser_obj;
    static QPointer <AssetObject> sphere_obj;
};

#endif // CONTROLLERMANAGER_H
