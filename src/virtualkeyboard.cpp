#include "virtualkeyboard.h"

VirtualKeyboard::VirtualKeyboard() :    
    shift_modifier(false),
    control_modifier(false),
    alt_modifier(false),
    capslock(false),
    visible(false)
{
    mouse_pressed[0] = false;
    mouse_pressed[1] = false;

    //set up keynames
    keynames[Qt::Key_A] = "a";
    keynames[Qt::Key_B] = "b";
    keynames[Qt::Key_C] = "c";
    keynames[Qt::Key_D] = "d";
    keynames[Qt::Key_E] = "e";
    keynames[Qt::Key_F] = "f";
    keynames[Qt::Key_G] = "g";
    keynames[Qt::Key_H] = "h";
    keynames[Qt::Key_I] = "i";
    keynames[Qt::Key_J] = "j";
    keynames[Qt::Key_K] = "k";
    keynames[Qt::Key_L] = "l";
    keynames[Qt::Key_M] = "m";
    keynames[Qt::Key_N] = "n";
    keynames[Qt::Key_O] = "o";
    keynames[Qt::Key_P] = "p";
    keynames[Qt::Key_Q] = "q";
    keynames[Qt::Key_R] = "r";
    keynames[Qt::Key_S] = "s";
    keynames[Qt::Key_T] = "t";
    keynames[Qt::Key_U] = "u";
    keynames[Qt::Key_V] = "v";
    keynames[Qt::Key_W] = "w";
    keynames[Qt::Key_X] = "x";
    keynames[Qt::Key_Y] = "y";
    keynames[Qt::Key_Z] = "z";

    keynames[Qt::Key_0] = "0";
    keynames[Qt::Key_1] = "1";
    keynames[Qt::Key_2] = "2";
    keynames[Qt::Key_3] = "3";
    keynames[Qt::Key_4] = "4";
    keynames[Qt::Key_5] = "5";
    keynames[Qt::Key_6] = "6";
    keynames[Qt::Key_7] = "7";
    keynames[Qt::Key_8] = "8";
    keynames[Qt::Key_9] = "9";

    keynames[Qt::Key_Alt] = "alt";
    keynames[Qt::Key_Backspace] = "backspace";
    keynames[Qt::Key_CapsLock] = "capslock";
    keynames[Qt::Key_Semicolon] = "braceleft";
    keynames[Qt::Key_Apostrophe] = "braceright";
    keynames[Qt::Key_Comma] = "comma";
    keynames[Qt::Key_Control] = "control";
    keynames[Qt::Key_Minus] = "minus";
    keynames[Qt::Key_Return] = "enter";
    keynames[Qt::Key_Equal] = "equal";
    keynames[Qt::Key_Period] = "period";
    keynames[Qt::Key_Shift] = "shift";
    keynames[Qt::Key_Slash] = "slash";
    keynames[Qt::Key_Space] = "space";
    keynames[Qt::Key_QuoteLeft] = "tilde";

    //load textures for keys
    key_img = new AssetImage();
    key_img->GetProperties()->SetID("keys");
    key_img->SetSrc(MathUtil::GetApplicationURL(), "assets/keyboard/keys.png");
    key_img->Load();

    keypress_img = new AssetImage();
    keypress_img->GetProperties()->SetID("keyspressed");
    keypress_img->SetSrc(MathUtil::GetApplicationURL(), "assets/keyboard/keyspressed.png");
    keypress_img->Load();

    keyhover_img  = new AssetImage();
    keyhover_img->GetProperties()->SetID("keyshovered");
    keyhover_img->SetSrc(MathUtil::GetApplicationURL(), "assets/keyboard/keyshovered.png");
    keyhover_img->Load();

    //load geometries for keys, and set up objects        
    for (QHash <Qt::Key, QString>::const_iterator cit=keynames.begin(); cit!= keynames.end(); ++cit) {
        QPointer <AssetObject> a = new AssetObject();
        a->GetProperties()->SetID(cit.value());
        a->SetSrc(MathUtil::GetApplicationURL(), "assets/keyboard/" + cit.value() + ".obj");
        a->Load();
        assetobjs[cit.key()] = a;

        const QString jsid = "__" + cit.value();
        QPointer <RoomObject> o = new RoomObject();
        o->SetType(TYPE_OBJECT);
        o->SetInterfaceObject(true);
        o->GetProperties()->SetJSID(jsid);
        o->GetProperties()->SetID(jsid);
        o->SetAssetObject(a);
        o->SetCollisionAssetObject(a);
        o->SetAssetImage(key_img);
        o->GetProperties()->SetLighting(false);

        envobjects[jsid] = o;
        jstokeys[jsid] = cit.key();
//        qDebug() << "Loading keyboard... " << cit.value();
    }
}

void VirtualKeyboard::SetWebSurface(QPointer <AbstractWebSurface> w)
{
    websurface = w;
}

QPointer <AbstractWebSurface> VirtualKeyboard::GetWebSurface() const
{
    return websurface;
}

void VirtualKeyboard::UpdateAssets()
{
    for (QPointer <AssetObject> & a : assetobjs) {
        if (a) {
            a->Update();
            a->UpdateGL();
        }
    }

    if (key_img) {
        key_img->UpdateGL();
    }
    if (keypress_img) {
        keypress_img->UpdateGL();
    }
    if (keyhover_img) {
        keyhover_img->UpdateGL();
    }
}

void VirtualKeyboard::Update()
{
    UpdateAssets();

    QHash <QString, QPointer <RoomObject> >::iterator cit;
    for (cit=envobjects.begin(); cit!= envobjects.end(); ++cit) {
        if (cit.value()) {
            const QString js_id = cit.value()->GetProperties()->GetJSID();
            if (shift_modifier && js_id == "__shift") {
                cit.value()->SetAssetImage(keypress_img);
            }
            else if (control_modifier && js_id == "__control") {
                cit.value()->SetAssetImage(keypress_img);
            }
            else if (alt_modifier && js_id == "__alt") {
                cit.value()->SetAssetImage(keypress_img);
            }
            else if (capslock && js_id == "__capslock") {
                cit.value()->SetAssetImage(keypress_img);
            }
            else if (cur_selected[0] == cit.value() && mouse_pressed[0]) {
                cit.value()->SetAssetImage(keypress_img);
            }
            else if (cur_selected[1] == cit.value() && mouse_pressed[1]) {
                cit.value()->SetAssetImage(keypress_img);
            }
            else if (cur_selected[0] == cit.value()) {
                cit.value()->SetAssetImage(keyhover_img);
            }
            else if (cur_selected[1] == cit.value()) {
                cit.value()->SetAssetImage(keyhover_img);
            }
            else {
                cit.value()->SetAssetImage(key_img);
            }
            cit.value()->Update(0.0f);
        }
    }
}

void VirtualKeyboard::DrawGL(QPointer <AssetShader> shader)
{    
    RendererInterface::m_pimpl->SetDepthFunc(DepthFunc::LEQUAL);
    QHash <QString, QPointer <RoomObject> >::iterator cit;
    for (cit=envobjects.begin(); cit!= envobjects.end(); ++cit) {       
        if (cit.value()) {
            cit.value()->DrawGL(shader, true, QVector3D(0,0,0));
//            qDebug() << "DRAWING" << cit.value()->GetJSID();
        }
    }
}

QHash <QString, QPointer <RoomObject> > & VirtualKeyboard::GetEnvObjects()
{
    return envobjects;
}

void VirtualKeyboard::mouseReleaseEvent(const int index)
{
    mouse_pressed[index] = false;
}

void VirtualKeyboard::mousePressEvent(const int index)
{
    mouse_pressed[index] = true;

    if (cur_selected[index] == envobjects["__shift"]) {
        shift_modifier = !shift_modifier;
    }
    else if (cur_selected[index] == envobjects["__control"]) {
        control_modifier = !control_modifier;
    }
    else if (cur_selected[index] == envobjects["__alt"]) {
        alt_modifier = !alt_modifier;
    }
    else if (cur_selected[index] == envobjects["__capslock"]) {
        capslock = !capslock;
    }
    else if (cur_selected[index]) {
        const Qt::Key key = GetKeySelected(cur_selected[index]->GetProperties()->GetJSID());
        int key_code = (int)key;

        if (websurface) {

             QString s = QKeySequence(key).toString();

             if (shift_modifier && capslock) {
                 s = s.toLower();
             }
             else if (shift_modifier) {
                 s = s.toUpper();

                 switch (s.at(0).toLatin1()) {
                 case '0':
                     s = ")";
                     break;
                 case '1':
                     s = "!";
                     break;
                 case '2':
                     s = "@";
                     break;
                 case '3':
                     s = "#";
                     break;
                 case '4':
                     s = "$";
                     break;
                 case '5':
                     s = "%";
                     break;
                 case '6':
                     s = "^";
                     break;
                 case '7':
                     s = "&";
                     break;
                 case '8':
                     s = "*";
                     break;
                 case '9':
                     s = "(";
                     break;
                 case '-':
                     s = "_";
                     break;
                 case '=':
                     s = "+";
                     break;
                 case '`':
                     s = "~";
                     break;
                 case ',':
                     s = "<";
                     break;
                 case '.':
                     s = ">";
                     break;
                 case ';':
                     s = ":";
                     break;
                 case '\'':
                     s = "\"";
                     break;
                 case '[':
                     s = "{";
                     break;
                 case ']':
                     s = "}";
                     break;
                 case '/':
                     s = "?";
                     break;
                 default:
                     break;
                 }
             }

             key_code = key;
             if (key >= Qt::Key_A && key <= Qt::Key_Z) {
                 if (shift_modifier || capslock) {
                     s = s.toUpper();
                 }
                 else {
                     s = s.toLower();
                     key_code += 32;
                 }
             }
             else if (key == Qt::Key_Backspace) {
                 key_code = 65288;
             }

#ifdef __ANDROID__
             unsigned int mod = 0;
             if (shift_modifier || (capslock && key >= Qt::Key_A && key <= Qt::Key_Z)){
                mod |= Qt::ShiftModifier;
                shift_modifier = false;
             }
             else if (control_modifier){
                 mod |= Qt::ControlModifier;
                 control_modifier = false;
             }
             else if (alt_modifier){
                 mod |= Qt::AltModifier;
                 alt_modifier = false;
             }
             QKeyEvent e(QEvent::KeyPress, key, (Qt::KeyboardModifier) mod, 0, key_code, 0, s);
#else
             QKeyEvent e(QEvent::KeyPress, key, Qt::NoModifier, 0, key_code, 0, s);
#endif
             websurface->keyPressEvent(&e);
        }
    }
}

Qt::Key VirtualKeyboard::GetKeySelected(const QString jsid)
{
    if (jstokeys.contains(jsid)) {
        return jstokeys[jsid];
    }
    else {
        return (Qt::Key)0;
    }
}

void VirtualKeyboard::SetShiftModifier(const bool b)
{
    shift_modifier = b;
}

bool VirtualKeyboard::GetShiftModifier()
{
    return shift_modifier;
}

bool VirtualKeyboard::GetCapsLock()
{
    return capslock;
}

void VirtualKeyboard::SetVisible(const bool b)
{
    visible = b;
}

bool VirtualKeyboard::GetVisible() const
{
    return visible;
}

void VirtualKeyboard::SetModelMatrix(const QMatrix4x4 m)
{
    modelmatrix = m;

    QHash <QString, QPointer <RoomObject> >::iterator cit;
    for (cit=envobjects.begin(); cit!= envobjects.end(); ++cit) {
        if (cit.value()) {
//            qDebug() << "before" << cit.value()->GetPos();
            cit.value()->SetAttributeModelMatrix(modelmatrix);
//            qDebug() << "after" << cit.value()->GetPos();
        }
    }
}

void VirtualKeyboard::SetSelected(QPointer <RoomObject> o, const int index)
{    
    cur_selected[index] = o;
}

QPointer <RoomObject> VirtualKeyboard::GetSelected(const int index) const
{
    return cur_selected[index];
}
