#include "player.h"

Player::Player() :    
    comfort_spinleft(false),
    comfort_spinright(false),                
    flytransition(false)

{
    props = new DOMNode();
    props->SetS("_type", "player");
    props->SetV("dir", QVector3D(1,0,0));    
    comfort_timer.start();

    props->setProperty("touch", QVariant::fromValue<QObject *>(CreateControllerQObject().data()));
    props->setProperty("vive", QVariant::fromValue<QObject *>(CreateControllerQObject().data()));
    props->setProperty("wmxr", QVariant::fromValue<QObject *>(CreateControllerQObject().data()));
    props->setProperty("xbox", QVariant::fromValue<QObject *>(CreateControllerQObject().data()));
#ifdef __ANDROID__
    props->setProperty("daydream", QVariant::fromValue<QObject *>(CreateControllerQObject().data()));
    props->setProperty("gear", QVariant::fromValue<QObject *>(CreateControllerQObject().data()));
    //props->setProperty("go", QVariant::fromValue<QObject *>(CreateControllerQObject().data())); //60.0 - "go" uses "gear"
#endif
}

void Player::SetProperties(QPointer<DOMNode> props)
{
    this->props = props;
    props->SetS("_type", "player");
}

QPointer<DOMNode> Player::GetProperties()
{
    return props;
}

void Player::SetV(const char * name, const QVector3D p)
{
    if (props) {
        props->SetV(name, p);
    }
}

QVector3D Player::GetV(const char * name) const
{
    if (props) {
        return props->GetV(name);
    }
    return QVector3D();
}

void Player::SetV4(const char * name, const QVector4D p)
{
    if (props) {
        props->SetV4(name, p);
    }
}

QVector4D Player::GetV4(const char * name) const
{
    if (props) {
        return props->GetV4(name);
    }
    return QVector4D();
}

void Player::SetF(const char * name, const float f)
{
    if (props) {
        props->setProperty(name, QString::number(f));
    }
}

float Player::GetF(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return props->property(name).toFloat();
    }
    return 0.0f;
}

void Player::SetI(const char * name, const int i)
{
    if (props) {
        props->setProperty(name, QString::number(i));
    }
}

int Player::GetI(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return props->property(name).toInt();
    }
    return 0;
}

void Player::SetB(const char * name, const bool b)
{
    if (props) {
        props->setProperty(name, b ? "true" : "false");
    }
}

bool Player::GetB(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return props->property(name).toString().toLower() == "true";
    }
    return false;
}

void Player::SetS(const char * name, const QString s)
{
    if (props) {
        props->setProperty(name, s);
    }
}

QString Player::GetS(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return props->property(name).toString();
    }
    return QString();
}

void Player::SetC(const char * name, const QColor c)
{
    if (props) {
        props->setProperty(name, MathUtil::GetColourAsString(c, false));
    }
}

QColor Player::GetC(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return MathUtil::GetStringAsColour(props->property(name).toString());
    }
    return QColor(255,255,255);
}

void Player::UpdateDir()
{
//    qDebug() << "Player::UpdateDir()" << GetV("dir");
    SetV("dir", GetV("dir").normalized());
    float theta, phi;
    MathUtil::NormCartesianToSphere(GetV("dir"), theta, phi);
    SetF("theta", theta);
    SetF("phi", phi);
}

void Player::UpdateEyePoint()
{
    QVector3D eye_point = GetV("pos") + GetV("local_head_pos");
    if (GetS("hmd_type") != "vive" && GetS("hmd_type") != "wmxr" ) {
        eye_point += GetV("eye_pos");
    }
    SetV("eye_point", eye_point);
}

QVector3D Player::GetRightDir() const
{
    return QVector3D::crossProduct(GetV("dir"), GetV("up_dir")).normalized();
}

QMatrix4x4 Player::GetTransform() const
{
    QMatrix4x4 m;
    m.rotate(-GetF("theta"), QVector3D(0,1,0));
    m.setColumn(3, GetV("pos"));
    m.setRow(3, QVector4D(0,0,0,1));
    return m;
}

void Player::SetCursorActive(const bool b, const int index)
{
    if (index == 0) {
        SetB("cursor_active", b);
        SetB("cursor0_active", b);
    }
    else {
        SetB("cursor1_active", b);
    }
}

bool Player::GetCursorActive(const int index) const
{
    return GetB((index == 0) ? "cursor0_active" : "cursor1_active");
}

void Player::SetCursorPos(const QVector3D & p, const int index)
{
    if (index == 0) {
        SetV("cursor_pos", p);
        SetV("cursor0_pos", p);
    }
    else {
        SetV("cursor1_pos", p);
    }
}

QVector3D Player::GetCursorPos(const int index) const
{    
    return GetV((index == 0) ? "cursor0_pos" : "cursor1_pos");
}

void Player::SetCursorXDir(const QVector3D & p, const int index)
{
    if (index == 0) {
        SetV("cursor_xdir", p);
        SetV("cursor0_xdir", p);
    }
    else {
        SetV("cursor1_xdir", p);
    }
}

QVector3D Player::GetCursorXDir(const int index) const
{
    return GetV((index == 0) ? "cursor0_xdir" : "cursor1_xdir");
}

void Player::SetCursorYDir(const QVector3D & p, const int index)
{
    if (index == 0) {
        SetV("cursor_ydir", p);
        SetV("cursor0_ydir", p);
    }
    else {
        SetV("cursor1_ydir", p);
    }
}

QVector3D Player::GetCursorYDir(const int index) const
{
    return GetV((index == 0) ? "cursor0_ydir" : "cursor1_ydir");
}

void Player::SetCursorZDir(const QVector3D & p, const int index)
{
    if (index == 0) {
        SetV("cursor_zdir", p);
        SetV("cursor0_zdir", p);
    }
    else {
        SetV("cursor1_zdir", p);
    }
}

QVector3D Player::GetCursorZDir(const int index) const
{
    return GetV((index == 0) ? "cursor0_zdir" : "cursor1_zdir");
}

void Player::SetCursorScale(const float f, const int index)
{
    if (index == 0) {
        SetF("cursor_scale", f);
        SetF("cursor0_scale", f);
    }
    else {
        SetF("cursor1_scale", f);
    }
}

float Player::ComputeCursorScale(const int index) const
{    
    if (index == 0) {
        const QVector3D v1 = GetV("cursor0_pos") - GetV("eye_point");
        return 0.08f * QVector3D::dotProduct(v1, props->GetV("view_dir"));
    }
    else {
        const QVector3D v1 = GetV("cursor1_pos") - GetV("eye_point");
        return 0.08f * QVector3D::dotProduct(v1, props->GetV("view_dir"));
    }
}

float Player::GetCursorScale(const int index) const
{
    return GetF((index == 0) ? "cursor0_scale" : "cursor1_scale");
}

void Player::SetCursorObject(const QString & s, const int index)
{
    if (index == 0) {
        SetS("cursor_object", s);
        SetS("cursor0_object", s);
    }
    else {
        SetS("cursor1_object", s);
    }
}

QString Player::GetCursorObject(const int index) const
{
    return GetS((index == 0) ? "cursor0_object" : "cursor1_object");
}

void Player::SpinView(const float f, const bool scale_rotation_speed)
{    
    const float rotation_speed = SettingsManager::GetRotationSpeed();
    float theta = GetF("theta");
    if (scale_rotation_speed) {
        theta += f * (rotation_speed / 60.0f);
    }
    else {
        theta += f;
    }

    if (theta > 360.0f) {
        theta -= 360.0f;
    }
    if (theta < 0.0f) {
        theta += 360.0f;
    }
    SetF("theta", theta);
}

void Player::TiltView(const float f)
{
    const float rotation_speed = SettingsManager::GetRotationSpeed();
    float phi = GetF("phi");
    phi += f * rotation_speed / 60.0f;
    phi = qMax(phi, 5.0f);
    phi = qMin(phi, 175.0f);
    SetF("phi", phi);
}

QVector3D Player::GetViewToWorldPoint(const QVector3D & p) const
{
    const QVector3D view_dir = GetV("view_dir");
    const QVector3D right_dir = GetRightDir();
    const QVector3D rotated_up_dir = QVector3D::crossProduct(view_dir, right_dir).normalized();
    return GetV("eye_point") + view_dir * p.z() - right_dir * p.x() + rotated_up_dir * (-p.y());
}

QVector3D Player::GetViewToWorldDir(const QVector3D & d) const
{
    const QVector3D view_dir = GetV("view_dir");
    const QVector3D right_dir = GetRightDir();
    const QVector3D rotated_up_dir = QVector3D::crossProduct(view_dir, right_dir);
    return (view_dir * d.z() - right_dir * d.x() + rotated_up_dir * (-d.y()));
}

void Player::SetViewGL(const bool render_left_eye, const float eye_ipd, const QMatrix4x4 xform, const bool no_pitch/* = false*/)
{
    const float half_ipd = (render_left_eye ? -eye_ipd : eye_ipd) * 0.5f;

    QMatrix4x4 rot_mat;
    rot_mat.rotate(-(GetF("theta") + GetF("correction_theta")), 0, 1, 0);
    if (!no_pitch)
    {
        rot_mat.rotate(-(GetF("phi") - 90.0f), 1, 0, 0);
    }

    QMatrix4x4 rotated_xform = rot_mat * xform;

    SetV("up_dir", rotated_xform.column(1).toVector3D());
    SetV("view_dir", -rotated_xform.column(2).toVector3D());
    SetV("local_head_pos", rotated_xform.column(3).toVector3D());

    UpdateEyePoint();

    QVector3D eye_point = GetV("eye_point");
    SetV("head_pos", eye_point);

//    if (view_decoupled || (!walkLeft && !walkRight && !walkForward && !walkBack) || QString::compare("cube", this->GetHMDType()) == 0) {
        //view decoupled, forward dir is fixed with player's theta
        const float theta_rad = (GetF("theta")) * MathUtil::_PI_OVER_180;
        QVector3D dir = MathUtil::GetRotatedAxis(-theta_rad, QVector3D(0,0,-1), QVector3D(0, 1, 0));
        dir.setY(GetV("view_dir").y());
        dir.normalize();
        SetV("dir", dir);
//    }
//    else {
//        props->SetDir(GetViewDir());
//    }

    //get the view matrix (current modelview matrix)
    QVector3D x = rotated_xform.column(0).toVector3D();
    QVector3D y = rotated_xform.column(1).toVector3D();
    QVector3D z = rotated_xform.column(2).toVector3D();
    QVector3D p = eye_point + rotated_xform.column(0).toVector3D() * half_ipd;

    QMatrix4x4 view_matrix;
    view_matrix.setRow(0, QVector4D(x.x(), x.y(), x.z(), -QVector3D::dotProduct(x, p)));
    view_matrix.setRow(1, QVector4D(y.x(), y.y(), y.z(), -QVector3D::dotProduct(y, p)));
    view_matrix.setRow(2, QVector4D(z.x(), z.y(), z.z(), -QVector3D::dotProduct(z, p)));
    view_matrix.setRow(3, QVector4D(0,0,0,1));

    //set view and model matrices
    MathUtil::LoadModelIdentity();
    MathUtil::LoadViewMatrix(view_matrix);
}

bool Player::GetWalking() const
{
    return GetB("walk_forward") || GetB("walk_back") || GetB("walk_left") || GetB("walk_right");
}

void Player::DoSpinLeft(const bool b)
{
    if (b && !GetB("spin_left")) {
        comfort_spinleft = true;
    }
    SetB("spin_left", b);
}

void Player::DoSpinRight(const bool b)
{
    if (b && !GetB("spin_right")) {
        comfort_spinright = true;
    }
    SetB("spin_right", b);
}

void Player::SetFlying(const bool b)
{
    if (!GetB("flying") && b) {
        flytransition = true;
        flyduration = 500.0f;
        time.start();
    }

    SetB("flying", b);
}

void Player::ResetCentre()
{
    //dir theta is the way forward (forward in reality, whatever direction dir_theta in game)
    const float viewdir_theta = atan2f(GetV("view_dir").z(), GetV("view_dir").x()) * MathUtil::_180_OVER_PI + 90.0f;
    const QVector3D dir = GetV("dir");
    const float dir_theta = atan2f(dir.z(), dir.x()) * MathUtil::_180_OVER_PI + 90.0f;

    SetF("theta", GetF("theta") + viewdir_theta - dir_theta);
    SetF("correction_theta", GetF("correction_theta") - viewdir_theta + dir_theta);

    //qDebug() << "Player::ResetCentre() - correction theta" << correction_theta;
}

void Player::Update(const float move_speed)
{
    const bool flying = GetB("flying");
    const float delta_t = GetF("delta_time");

    const LeapHand & hand0 = hands.first;
    const LeapHand & hand1 = hands.second;

    SetB("hand0_active", hand0.is_active);
    SetB("hand1_active", hand1.is_active);

    const float angle = -MathUtil::_PI_OVER_2 - atan2f(GetV("dir").z(), GetV("dir").x());

    if (hand0.is_active) {                             
        QMatrix4x4 m;
        m.translate(GetV("pos"));
        m.rotate(angle * MathUtil::_180_OVER_PI, QVector3D(0,1,0));
        m = m * hand0.basis;
        SetV("hand0_xdir", m.column(0).toVector3D());
        SetV("hand0_ydir", m.column(1).toVector3D());
        SetV("hand0_zdir", m.column(2).toVector3D());
        SetV("hand0_pos", m.column(3).toVector3D());
    }
    else {
        SetV("hand0_vel", QVector3D());
        SetV("hand0_pos_old", QVector3D());
    }

    if (hand1.is_active) {        
        QMatrix4x4 m;
        m.translate(GetV("pos"));
        m.rotate(angle * MathUtil::_180_OVER_PI, QVector3D(0,1,0));
        m = m * hand1.basis;
        SetV("hand1_xdir", m.column(0).toVector3D());
        SetV("hand1_ydir", m.column(1).toVector3D());
        SetV("hand1_zdir", m.column(2).toVector3D());
        SetV("hand1_pos", m.column(3).toVector3D());
    }
    else {
        SetV("hand1_vel", QVector3D());
        SetV("hand1_pos_old", QVector3D());
    }

    //59.6 - update hand velocity at a minimum interval (30 times per second)
    const double hand_update_time_threshold = 1.0/30.0;
    float hand_update_time = GetF("hand_update_time");
    hand_update_time += delta_t; //integrate time for hand velocity updates
    if (hand_update_time > hand_update_time_threshold) {
        const QVector3D p0 = GetV("hand0_pos");
        const QVector3D p1 = GetV("hand1_pos");
        const QVector3D hand0_pos_old = GetV("hand0_pos_old");
        const QVector3D hand1_pos_old = GetV("hand1_pos_old");
        if (hand0.is_active && hand0_pos_old != QVector3D()) {
            SetV("hand0_vel", (p0 - hand0_pos_old)/hand_update_time);
        }
        if (hand1.is_active && hand1_pos_old != QVector3D()) {
            SetV("hand1_vel", (p1 - hand1_pos_old)/hand_update_time);
        }
        SetV("hand0_pos_old", p0);
        SetV("hand1_pos_old", p1);
        hand_update_time = 0.0;
    }
    SetF("hand_update_time", hand_update_time);

//    qDebug() << "Player::Update() delta_t" << delta_t;
    const float rotation_speed = SettingsManager::GetRotationSpeed();
    if (SettingsManager::GetComfortMode() && GetB("hmd_enabled")) {
        if (comfort_spinleft) {
            SpinView(-rotation_speed*0.5f, true);
            comfort_spinleft = false;
        }
        if (comfort_spinright) {
            SpinView(rotation_speed*0.5f, true);
            comfort_spinright = false;
        }
    }
    else {
        if (GetB("spin_left")) {
            SpinView(-delta_t*rotation_speed, true); //NOTE: was 60.0
        }
        if (GetB("spin_right")) {
            SpinView(delta_t*rotation_speed, true); //NOTE: was 60.0
        }
    }

    //compute walk forward direction walk_dir
    QVector3D walk_dir = GetV("view_dir");
    if (!flying) {
        walk_dir.setY(0.0f);
    }
    walk_dir.normalize();

    //compute side to side direction c
    const QVector3D c = QVector3D::crossProduct(walk_dir, QVector3D(0, 1, 0)).normalized();

    //compute our "impulse" velocity vector to move the player
    QVector3D impulse_vel;

    //59.3 - condition ensures player moves in at least 1 direction
    if (GetB("walk_forward") != GetB("walk_back") || GetB("walk_left") != GetB("walk_right") || GetB("fly_up") != GetB("fly_down")|| flytransition) {
        if (GetB("walk_forward")) {
            impulse_vel += walk_dir;
        }
        if (GetB("walk_back")) {
            impulse_vel -= walk_dir;
        }
        if (GetB("walk_left")) {
            impulse_vel -= c;
        }
        if (GetB("walk_right")) {
            impulse_vel += c;
        }

        impulse_vel.normalize();

        const QVector3D forward = walk_dir * QVector3D::dotProduct(walk_dir, impulse_vel);
        const QVector3D side = c * QVector3D::dotProduct(c, impulse_vel);

        impulse_vel = forward * GetF("scale_velz") + side * GetF("scale_velx");
        impulse_vel *= move_speed;

        if (flying) {
            if (GetB("fly_up")) {
                impulse_vel.setY(move_speed);
            }
            if (GetB("fly_down")) {
                impulse_vel.setY(-move_speed);
            }

            if (flytransition) {
                if (time.elapsed() >= flyduration) {
                    flytransition = false;
                }
                else {
                    impulse_vel.setY(1.0f);
                }
            }
        }
    }

//    qDebug() << "Player::Update() impulse velocity" << scale_velx << scale_vely << impulse_vel << GetB("walk_forward") << GetB("walk_back") << GetB("walk_left") << GetB("walk_right") << GetV("vel");
    SetV("impulse_vel", impulse_vel);

    if (flying) {
        SetS("anim_id", "fly");
    }
    else if (GetB("running")) {
        SetS("anim_id", "run");
    }
    else if (GetB("walk_forward") && !GetB("walk_back")) {
        SetS("anim_id", "walk");
    }
    else if (GetB("walk_left") && !GetB("walk_right")) {
        SetS("anim_id", "walk_left");
    }
    else if (GetB("walk_right") && !GetB("walk_left")) {
        SetS("anim_id", "walk_right");
    }
    else if (GetB("walk_back") && !GetB("walk_forward")) {
        SetS("anim_id", "walk_back");
    }
    else if (GetB("speaking")) {
        SetS("anim_id", "speak");
    }
    else if (GetB("typing")) {
        SetS("anim_id", "type");
    }
    else if (fabsf(GetV("vel").y()) > 0.01f) { //57.1 - too small a threshold, and the player will jitter between idle/jump animations
        SetS("anim_id", "jump");
    }
    else {
        SetS("anim_id", "idle");
    }

    SetB("walking", GetB("walk_back") != GetB("walk_forward") || GetB("walk_left") != GetB("walk_right"));

    UpdateEyePoint();

    if (!SettingsManager::GetMousePitchEnabled()) {
        SetF("phi", 90.0f);
    }   
}

QPair <LeapHand, LeapHand> Player::GetHands() const
{
    return hands;
}

LeapHand & Player::GetHand(const int i)
{
    return (i == 0) ? hands.first : hands.second;
}

const LeapHand & Player::GetHand(const int i) const
{
    return (i == 0) ? hands.first : hands.second;
}

QTime & Player::GetComfortTimer()
{
    return comfort_timer;
}

QPointer <QObject> Player::CreateControllerQObject()
{
    QPointer <QObject> o = new QObject(props);
    o->setProperty("left_trigger", 0.0f);
    o->setProperty("left_shoulder", 0.0f);
    o->setProperty("left_grip", 0.0f);
    o->setProperty("left_stick_x", 0.0f);
    o->setProperty("left_stick_y", 0.0f);
    o->setProperty("left_stick_click", 0.0f);
    o->setProperty("left_trackpad_x", 0.0f);
    o->setProperty("left_trackpad_y", 0.0f);
    o->setProperty("left_trackpad_click", 0.0f);
    o->setProperty("left_menu", 0.0f);
    o->setProperty("button_a", 0.0f);
    o->setProperty("button_b", 0.0f);
    o->setProperty("right_trigger", 0.0f);
    o->setProperty("right_shoulder", 0.0f);
    o->setProperty("right_grip", 0.0f);
    o->setProperty("right_stick_x", 0.0f);
    o->setProperty("right_stick_y", 0.0f);
    o->setProperty("right_stick_click", 0.0f);
    o->setProperty("right_trackpad_x", 0.0f);
    o->setProperty("right_trackpad_y", 0.0f);
    o->setProperty("right_trackpad_click", 0.0f);
    o->setProperty("right_menu", 0.0f);
    o->setProperty("button_x", 0.0f);
    o->setProperty("button_y", 0.0f);
    o->setProperty("dpad_up", 0.0f);
    o->setProperty("dpad_down", 0.0f);
    o->setProperty("dpad_left", 0.0f);
    o->setProperty("dpad_right", 0.0f);
    o->setProperty("button_select", 0.0f);
    o->setProperty("button_start", 0.0f);
    return o;
}
