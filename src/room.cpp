#include "room.h"

QPointer <AssetShader> Room::transparency_shader;
QPointer <AssetShader> Room::portal_shader;
QPointer <AssetShader> Room::cubemap_shader;
QPointer <AssetShader> Room::cubemap_shader2;
QPointer <AssetShader> Room::skybox_shader;

QList <QPointer <AssetSkybox> > Room::skyboxes;
QHash <QString, QPointer <RoomTemplate> > Room::room_templates;

QHash <QString, QPointer <AssetObject> > Room::object_primitives;

Room::Room() :            
    assetshader(0),
    scripts_ready(false)
{        
    if (transparency_shader == NULL) {
        transparency_shader = QPointer<AssetShader>(new AssetShader());
        transparency_shader->SetProgramHandle(RendererInterface::m_pimpl->GetDefaultObjectShaderProgram());
        transparency_shader->SetS("id", "transparency_shader");
        transparency_shader->Load();
    }

    if (portal_shader == NULL) {
        portal_shader = QPointer<AssetShader>(new AssetShader());
        portal_shader->SetProgramHandle(RendererInterface::m_pimpl->GetDefaultPortalShaderProgram());
        portal_shader->SetS("id", "portal_shader");
        portal_shader->Load();
    }

    if (cubemap_shader == NULL) {
        cubemap_shader = QPointer<AssetShader>(new AssetShader());
        cubemap_shader->SetSrc(MathUtil::GetApplicationURL(), "assets/shaders/cubemap_to_equi_frag.txt", "");
        cubemap_shader->SetS("id", "cubemap_shader");
        cubemap_shader->Load();
    }

    if (cubemap_shader2 == NULL) {
        cubemap_shader2 = QPointer<AssetShader>(new AssetShader());
        cubemap_shader2->SetSrc(MathUtil::GetApplicationURL(), "assets/shaders/cubemap_to_equi_frag2.txt", "");
        cubemap_shader2->SetS("id", "cubemap_shader2");
        cubemap_shader2->Load();
    }

    if (skybox_shader == NULL) {
        skybox_shader = QPointer<AssetShader>(new AssetShader());
        skybox_shader->SetProgramHandle(RendererInterface::m_pimpl->GetDefaultSkyboxShaderProgram());
        skybox_shader->SetS("id", "skybox_shader");
        skybox_shader->Load();
    }   

    page = new HTMLPage();    

    Clear();

    LoadTemplates();
    LoadSkyboxes();

    SetS("use_local_asset", "");   

    physics = new RoomPhysics();
}

Room::~Room()
{
    Clear();

    if (entrance_object) {
        delete entrance_object;
    }
    if (page) {
        delete page;
    }
    if (physics) {
        delete physics;
    }
    if (script_engine) {
        delete script_engine;
    }
}

void Room::Clear()
{
    StopAll();

    // We do not delete
    QSet <RoomObject *> saved_portals;
    saved_portals.insert(entrance_object);
    for (QPointer <Room> & r : children) {
        if (r && r->GetParentObject()) {
            saved_portals.insert(r->GetParentObject());
        }
    }

    if (cubemap) {
        delete cubemap;
    }

    if (cubemap_radiance) {
        delete cubemap_radiance;
    }

    if (cubemap_irradiance) {
        delete cubemap_irradiance;
    }

    for (QPointer <AssetGhost> & a : assetghosts) {
        if (a) {
            delete a;
        }
    }
    assetghosts.clear();

    for (QPointer <AssetImage> & a : assetimages) {
        if (a) {
            delete a;
        }
    }
    assetimages.clear();

    for (QPointer <AssetObject> & a : assetobjects) {
        if (a && !a->GetB("_primitive")) {
            delete a;
        }
    }
    assetobjects.clear();

    for (QPointer <AssetRecording> & a : assetrecordings) {
        if (a) {
            delete a;
        }
    }
    assetrecordings.clear();

    for (QPointer <AssetScript> & a : assetscripts) {
        if (a) {
            delete a;
        }
    }
    assetscripts.clear();

    for (QPointer <AssetShader> & a : assetshaders) {
        if (a) {
            delete a;
        }
    }
    assetshaders.clear();

    for (QPointer <AssetSound> & a : assetsounds) {
        if (a) {
            delete a;
        }
    }
    assetsounds.clear();

    for (QPointer <AssetVideo> & a : assetvideos) {
        if (a) {
            delete a;
        }
    }
    assetvideos.clear();

    for (QPointer <AbstractWebSurface> & a : assetwebsurfaces) {
        if (a) {
            delete a;
        }
    }
    assetwebsurfaces.clear();

    for (QPointer<RoomObject>& obj : envobjects) {
        if (obj && !saved_portals.contains(obj)) {
            delete obj;
        }
    }
    envobjects.clear();

    //clear/reset cubemaps related room properties
    qint64 room_url_md5 = MathUtil::hash(GetS("url"));
    FilteredCubemapManager::GetSingleton()->RemoveFromProcessing(room_url_md5, false);

    if (page) {
        page->Clear();
    }

    assetshader.clear();

    if (entrance_object.isNull()) {
        entrance_object = new RoomObject();
        entrance_object->SetType("link");
        entrance_object->SetSaveToMarkup(false);
    }

    //initialize DOM tree
    QString url;
    if (props) {
        url = props->GetS("url");

        //60.0 - prevent deletion of DOMNodes via deleting props below
        for (const QPointer <RoomObject> & o : saved_portals) {
            if (o && o->GetProperties()) {
                props->RemoveChild(o->GetProperties());
            }
        }

        delete props;
    }
    props = new DOMNode();
    SetS("_type", "room");
    SetS("js_id", "__room");
    SetS("url", url);

    //59.13 - we need to initialize script engine after creation of dom_tree_root
    if (script_engine) {
        delete script_engine;
    }
    script_engine = AssetScript::GetNewScriptEngine(this);
    scripts_ready = false;
    custom_elements_processed = false;

    AddPrimitiveAssetObjects();
}

void Room::SetV(const char * name, const QVector3D p)
{
    if (props) {
        props->SetV(name, p);
    }
}

QVector3D Room::GetV(const char * name) const
{
    if (props) {
        return props->GetV(name);
    }
    return QVector3D();
}

void Room::SetV4(const char * name, const QVector4D p)
{
    if (props) {
        props->SetV4(name, p);
    }

}

QVector4D Room::GetV4(const char * name) const
{
    if (props) {
        return props->GetV4(name);
    }
    return QVector4D();
}

void Room::SetF(const char * name, const float f)
{
    if (props) {
        props->setProperty(name, QString::number(f));
    }
}

float Room::GetF(const char * name) const
{
    if (props) {
        return props->property(name).toFloat();
    }
    return 0.0f;
}

void Room::SetI(const char * name, const int i)
{
    if (props) {
        props->setProperty(name, QString::number(i));
    }
}

int Room::GetI(const char * name) const
{
    if (props) {
        return props->property(name).toInt();
    }
    return 0;
}

void Room::SetB(const char * name, const bool b)
{
    if (props) {
        props->setProperty(name, b ? "true" : "false");
    }
}

bool Room::GetB(const char * name) const
{
    if (props) {
        return props->property(name).toString().toLower() == "true";
    }
    return false;
}

void Room::SetS(const char * name, const QString s)
{
    if (props) {
        props->setProperty(name, s);
    }
}

QString Room::GetS(const char * name) const
{
    if (props) {
        return props->property(name).toString();
    }
    return QString();
}

void Room::SetC(const char * name, const QColor c)
{
    if (props) {
        props->setProperty(name, MathUtil::GetColourAsString(c, false));
    }
}

QColor Room::GetC(const char * name) const
{
    if (props) {
        return MathUtil::GetStringAsColour(props->property(name).toString());
    }
    return QColor(255,255,255);
}

void Room::SetProperties(const QVariantMap & d)
{
//    qDebug() << "Room::SetProperties" << this;
    QVariantMap::const_iterator it;
    for (it=d.begin(); it!=d.end(); ++it) {
//        qDebug() << " " << it.key() << it.value().toString();
        if (!it.key().isEmpty()) {
            props->setProperty(it.key().trimmed().toLower().toLatin1().data(), it.value());
        }
    }

    if (d.contains("skybox_down_id") ||
            d.contains("skybox_front_id") ||
            d.contains("skybox_left_id") ||
            d.contains("skybox_back_id") ||
            d.contains("skybox_right_id") ||
            d.contains("skybox_up_id")) {

        QVector <QString> skybox_id(6);
        skybox_id[0] = d["skybox_right_id"].toString();
        skybox_id[1] = d["skybox_left_id"].toString();
        skybox_id[2] = d["skybox_up_id"].toString();
        skybox_id[3] = d["skybox_down_id"].toString();
        skybox_id[4] = d["skybox_front_id"].toString();
        skybox_id[5] = d["skybox_back_id"].toString();
        SetCubemap(skybox_id, CUBEMAP_TYPE::DEFAULT);        
    }
    if (d.contains("cubemap_id")) {
        QVector <QString> skybox_id(1);
        skybox_id[0] = d["cubemap_id"].toString();
        SetCubemap(skybox_id, CUBEMAP_TYPE::DEFAULT);        
    }
    if (d.contains("cubemap_radiance_id")) {
        QVector <QString> skybox_id(1);
        skybox_id[0] = d["cubemap_radiance_id"].toString();
        SetCubemap(skybox_id, CUBEMAP_TYPE::RADIANCE);
    }
    if (d.contains("cubemap_irradiance_id")) {
        QVector <QString> skybox_id(1);
        skybox_id[0] = d["cubemap_irradiance_id"].toString();
        SetCubemap(skybox_id, CUBEMAP_TYPE::IRRADIANCE);
    }

    if (d.contains("pos")) {
        entrance_object->SetV("pos", MathUtil::GetStringAsVector(d["pos"].toString()));
    }
    if (d.contains("fwd")) {
        entrance_object->SetDir(MathUtil::GetStringAsVector(d["fwd"].toString()));
    }
    else {
        if (d.contains("xdir")) {
            entrance_object->SetV("xdir", MathUtil::GetStringAsVector(d["xdir"].toString()));
        }
        if (d.contains("ydir")) {
            entrance_object->SetV("ydir", MathUtil::GetStringAsVector(d["ydir"].toString()));
        }
        if (d.contains("zdir")) {
            entrance_object->SetV("zdir", MathUtil::GetStringAsVector(d["zdir"].toString()));
        }
    }
//    qDebug() << "Room::SetProperties PROCESSING ENTRANCEOBJECT" << d["pos"].toString() << entrance_object->GetV("pos");
}

void Room::save_cubemap_faces_to_cache(QString p_room_url_md5_string, QVector<QString>& p_file_names)
{
#ifndef __ANDROID__
    auto gl_tex_id = cubemap->GetTextureHandle();

    if (gl_tex_id != 0 && gl_tex_id != AssetImage::null_cubemap_tex_handle.get())
    {
        const QString cache_path = MathUtil::GetCachePath();

        int gl_tex_width;
        int gl_tex_height;
        int gl_tex_red_size;
        int gl_tex_internal_format;
        int gl_tex_swizzle_mask[4];
        int gl_max_level;
        int target_mip_level;
        gl_tex_swizzle_mask[0] = GL_RED;
        gl_tex_swizzle_mask[1] = GL_GREEN;
        gl_tex_swizzle_mask[2] = GL_BLUE;
        gl_tex_swizzle_mask[3] = GL_ALPHA;

        RendererInterface::m_pimpl->BindTextureHandle(10, gl_tex_id);
        MathUtil::glFuncs->glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, &gl_max_level);
        MathUtil::glFuncs->glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_R, &(gl_tex_swizzle_mask[0]));
        MathUtil::glFuncs->glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_G, &(gl_tex_swizzle_mask[1]));
        MathUtil::glFuncs->glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_B, &(gl_tex_swizzle_mask[2]));
        MathUtil::glFuncs->glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_A, &(gl_tex_swizzle_mask[3]));

        // Chose the texture level that nearest to but at least 256
        target_mip_level = (gl_max_level < 9) ? 0 : gl_max_level - 8;

        MathUtil::glFuncs->glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, target_mip_level, GL_TEXTURE_WIDTH, &gl_tex_width);
        MathUtil::glFuncs->glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, target_mip_level, GL_TEXTURE_HEIGHT, &gl_tex_height);
        MathUtil::glFuncs->glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, target_mip_level, GL_TEXTURE_RED_SIZE, &gl_tex_red_size);
        MathUtil::glFuncs->glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, target_mip_level, GL_TEXTURE_INTERNAL_FORMAT, &gl_tex_internal_format);

        QByteArray pixel_data = QByteArray();
        pixel_data.resize(gl_tex_width * gl_tex_height * 3);
        p_file_names.resize(6);        
        for(uint i = 0; i < 6; ++i)
        {
            MathUtil::glFuncs->glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, target_mip_level, GL_RGB, GL_UNSIGNED_BYTE, pixel_data.data());

            QImage image = QImage((uchar*)pixel_data.data(), gl_tex_width, gl_tex_height, gl_tex_width * 3, QImage::Format::Format_RGBA8888);

            p_file_names[i] = cache_path + QString("%1_cubemap_face%2.dds").arg(p_room_url_md5_string).arg(QString::number(i));

            QDir path_dir(cache_path);
            if (!path_dir.exists()) {
                path_dir.mkpath(".");
            }

            if(gl_tex_swizzle_mask[0] == GL_BLUE && gl_tex_swizzle_mask[1] == GL_GREEN && gl_tex_swizzle_mask[2] == GL_RED)
            {
                QImage rgbSwapped = image.rgbSwapped();
                rgbSwapped.save(p_file_names[i], "dds");
            }
            else
            {
                image.save(p_file_names[i],"dds");
            }
        }
    }
#endif
}

QString Room::GetSaveFilename() const
{
//    qDebug() << "Room::GetSaveFilename" << this << save_filename;
    //first ensure that the file is either online (not on local filesystem)
    //or it ends in .htm or .html (and can thus be overwritten)
    QString out_filename = GetS("url");
    QFileInfo file_info(out_filename);
    const QString s = file_info.suffix().toLower();

    if (!file_info.exists() || s == "htm" || s == "html") {
        //Behaviour:
        //1.  file is local.  copy existing file to timestamped one, then overwrite.
        //2.  file not local.  create new timestamp file.
        const QUrl url(out_filename);
        return (url.isLocalFile() ? url.toLocalFile() : MathUtil::GetSaveTimestampFilename());
    }

    return QString();
}

void Room::SetAssetShader(const QPointer <AssetShader> a)
{
    assetshader = a;
}

QPointer <AssetShader> Room::GetAssetShader()
{
    return assetshader;
}

void Room::SetCubemap(const QVector <QString> & skybox_image_ids, CUBEMAP_TYPE p_skybox_type)
{
//    qDebug() << "Room::SetCubemap()" << skybox_image_ids;
    int const imageCount = skybox_image_ids.size();
    QVector <QPointer <AssetImage> > imgs = QVector<QPointer <AssetImage> >(imageCount);

    for (int imageIndex = 0; imageIndex < imageCount; ++imageIndex) {
        if (assetimages.contains(skybox_image_ids[imageIndex]) &&
                assetimages[skybox_image_ids[imageIndex]]) {
            imgs[imageIndex] = dynamic_cast<AssetImage *>(assetimages[skybox_image_ids[imageIndex]].data());
            if (imgs[imageIndex]) {
                imgs[imageIndex]->SetB("tex_clamp", true);
            }
        }
    }

    switch(p_skybox_type)
    {
    case CUBEMAP_TYPE::RADIANCE:
        if (cubemap_radiance == nullptr)
        {
            cubemap_radiance = new AssetSkybox();
        }
        cubemap_radiance->SetAssetImages(imgs);
        break;
    case CUBEMAP_TYPE::IRRADIANCE:
        if (cubemap_irradiance == nullptr)
        {
            cubemap_irradiance = new AssetSkybox();
        }
        cubemap_irradiance->SetAssetImages(imgs);
        break;
    case CUBEMAP_TYPE::DEFAULT:
    default:
        if (cubemap == nullptr)
        {
            cubemap = new AssetSkybox();
        }
        cubemap->SetAssetImages(imgs);
        break;
    }
}

void Room::LinkToAssets(QPointer <RoomObject> o)
{
    //custom linking where "id" meaning differs based on tag name/type
    const QString t = o->GetType();

//    qDebug() << "Room::LinkToAssets" << o->GetS("type") << o->GetS("id") << o->GetS("collision_id") << GetAssetObject(o->GetS("collision_id"));
    o->SetCollisionAssetObject(GetAssetObject(o->GetS("collision_id")));
    o->SetEmitterAssetObject(GetAssetObject(o->GetS("emitter_id")));
    o->SetBlendAssetObject(0, GetAssetObject(o->GetS("blend0_id")));
    o->SetBlendAssetObject(1, GetAssetObject(o->GetS("blend1_id")));
    o->SetBlendAssetObject(2, GetAssetObject(o->GetS("blend2_id")));
    o->SetBlendAssetObject(3, GetAssetObject(o->GetS("blend3_id")));
    o->SetAnimAssetObject(GetAssetObject(o->GetS("anim_id")));
    o->SetCubemapRadiance(GetAssetImage(o->GetS("cubemap_radiance_id")));
    o->SetCubemapIrradiance(GetAssetImage(o->GetS("cubemap_irradiance_id")));
    //59.9 - bugfix, without this conditional, causes video type not to work with id="blah" set
    if (t != "video") {
        o->SetAssetVideo(GetAssetVideo(o->GetS("video_id")));
    }
    o->SetAssetShader(GetAssetShader(o->GetS("shader_id")));
    if (t == "image" && !o->GetS("id").isEmpty()) { //60.0 - super finicky!  Do not remove null check
        const QString id = o->GetS("id");
        QPointer <AssetImage> a = GetAssetImage(id);
        if (a.isNull() && !id.isEmpty()) {
            //lazy load it
            a = new AssetImage();
            a->SetS("id", id);
            a->SetSrc(GetS("url"), id);
            a->Load();
            MathUtil::ErrorLog(QString("Warning: id ") + id + QString(" not found, trying image lazy loading"));
            AddAssetImage(a);
        }
        o->SetAssetImage(a);
    }
    else if (!o->GetS("thumb_id").isEmpty()) {
        o->SetAssetImage(GetAssetImage(o->GetS("thumb_id")));
    }
    else if (!o->GetS("image_id").isEmpty()) {
        const QString id = o->GetS("image_id");
        QPointer <AssetImage> a = GetAssetImage(id);
        if (a.isNull() && !id.isEmpty()) {
            //lazy load it
            a = new AssetImage();
            a->SetS("id", id);
            a->SetSrc(GetS("url"), id);
            a->Load();
            MathUtil::ErrorLog(QString("Warning: image_id ") + id + QString(" not found, trying image lazy loading"));
            AddAssetImage(a);
        }
        o->SetAssetImage(a);
    }
    else {
        o->SetAssetImage(QPointer<AssetImage>());
    }

    o->SetTeleportAssetObject(GetAssetObject(o->GetS("teleport_id")));
    o->SetAssetLightmap(GetAssetImage(o->GetS("lightmap_id")));
    o->SetAssetWebSurface(GetAssetWebSurface(o->GetS("websurface_id")));

    if (t == "ghost") {
        o->SetAssetGhost(GetAssetGhost(o->GetS("id")));
    }
    else if (t == "particle") {
        o->SetAssetObject(GetAssetObject(o->GetS("id")));
    }    
    else if (t == "sound") {
        o->SetAssetSound(GetAssetSound(o->GetS("id")));
    }
    else if (t == "object") {
        const QString id = o->GetS("id");
        QPointer <AssetObject> a = GetAssetObject(id);
        if (a.isNull() && !id.isEmpty()) {
            //lazy load it
            a = new AssetObject();
            a->SetS("id", id);
            a->SetSrc(GetS("url"), id);
            a->Load();
            MathUtil::ErrorLog(QString("Warning: id ") + id + QString(" not found, trying object lazy loading"));
            AddAssetObject(a);
        }
        o->SetAssetObject(a);
    }
    else if (t == "video") {
        o->SetAssetVideo(GetAssetVideo(o->GetS("id")));
    }    
}

QString Room::AddRoomObject(QPointer <RoomObject> o)
{
    if (o.isNull()) {
        return QString();
    }

    //If js_id is empty, or in use *by another roomobject*, assign it a unique number
    QString js_id = o->GetS("js_id");
    int room_object_uuid = GetI("_room_object_uuid");
    while (js_id.isEmpty() || (!GetRoomObject(js_id).isNull() && GetRoomObject(js_id) != o)) {
        js_id = QString::number(room_object_uuid);
        room_object_uuid++;
    }
    SetI("_room_object_uuid", room_object_uuid);

    o->SetS("js_id", js_id);
    envobjects[js_id] = o;

    QPointer <RoomObject> po;
    if (o->GetParentObject()) {
        po = GetRoomObject(o->GetParentObject()->GetS("js_id"));
    }

    if (po && o->GetParentObject()->GetS("js_id") != QString("__room")) {
        po->AppendChild(o);
    }
    else {
        props->AppendChild(o->GetProperties());
    }

    //recursively add child objects as well
    if (!o->GetChildObjects().isEmpty()) {
        AddRoomObjects(o->GetChildObjects());
    }

    return js_id;
}

void Room::AddRoomObjects(QList<QPointer <RoomObject> > & objects)
{
    for (int i=0; i<objects.size(); ++i) {
        AddRoomObject(objects[i]);
    }
}

void Room::DrawCollisionModelGL(QPointer <AssetShader> shader)
{
    //for debugging
    const QString s = GetS("use_local_asset");
    if (room_templates.contains(s) && room_templates[s] && room_templates[s]->GetEnvObject()) {
        room_templates[s]->GetEnvObject()->DrawCollisionModelGL(shader);
    }

    for (QPointer <RoomObject> & o : envobjects) {
        if (o && o->GetType() == "object") {
            o->DrawCollisionModelGL(shader);
        }
    }
}

void Room::SetPlayerPosTrans(const QVector3D p)
{
    player_pos_trans = p;
}

void Room::SetUseClipPlane(const bool b, const QVector4D p)
{
    use_clip_plane = b;
    plane_eqn = p;
}

void Room::BindShader(QPointer <AssetShader> shader)
{
    if (shader.isNull()) {
        return;
    }

    shader->SetUseClipPlane(use_clip_plane);
    shader->SetClipPlane(plane_eqn);    
    shader->SetFogEnabled(GetB("fog"));
    int fog_mode = 0;
    const QString s = GetS("fog_mode").toLower();
    if (s == "linear") {
        fog_mode = 0;
    }
    else if (s == "exp2") {
        fog_mode = 2;
    }
    else {
        fog_mode = 1;
    }
    shader->SetFogMode(fog_mode);
    shader->SetFogDensity(GetF("fog_density"));
    shader->SetFogStart(GetF("fog_start"));
    shader->SetFogEnd(GetF("fog_end"));
    shader->SetFogColour(GetC("fog_col"));
    shader->SetUseLighting(false);
    shader->SetPlayerPosition(player_pos_trans);    

    shader->UpdateFrameUniforms();
    shader->UpdateObjectUniforms();

    BindCubemaps(shader);
}

void Room::UnbindShader(QPointer <AssetShader> shader)
{
    if (shader.isNull()) {
        return;
    }

    shader->SetUseClipPlane(false);    
}

void Room::remove_intermediate_cubemap_files()
{
    const QString cache_path = MathUtil::GetCachePath();
    qint64 room_url_md5 = MathUtil::hash(GetS("url"));
    QString room_url_md5_string = QString::number(room_url_md5);
    QString radiance_file_path = cache_path  + QString("/%1_cubemap_radiance256.dds").arg(room_url_md5_string);
    QString irradiance_file_path =  cache_path  + QString("/%1_cubemap_irradiance64.dds").arg(room_url_md5_string);

    QFile radiance_file(radiance_file_path);
    bool radiance_remove_result = radiance_file.remove();
    QFile irradiance_file(irradiance_file_path);
    bool irradiance_remove_result = irradiance_file.remove();

    QString file_names[6];
    bool file_remove_results[6];
    for(uint i = 0; i < 6; ++i)
    {
        file_names[i] = cache_path + QString("/%1_cubemap_face%2.dds").arg(room_url_md5_string).arg(QString::number(i));
        QFile face_file(file_names[i]);
        file_remove_results[i] = face_file.remove();
    }

    if (radiance_remove_result == false
        || irradiance_remove_result == false
        || file_remove_results[0] == false
        || file_remove_results[1] == false
        || file_remove_results[2] == false
        || file_remove_results[3] == false
        || file_remove_results[4] == false
        || file_remove_results[5] == false)
    {
        qDebug() << "WARNING: Failed to delete a temporary file used for cubemap filtering, please report this issue.";
    }
}

void Room::BindCubemaps(QPointer <AssetShader> shader)
{
    // Bind room default cubemaps, which may be overriden below
    if (!skyboxes.isEmpty())
    {
        auto radiance_texture_id = skyboxes[1]->GetTextureHandle();
        if (radiance_texture_id != 0
            && radiance_texture_id != AssetImage::null_image_tex_handle.get()
            && radiance_texture_id != AssetImage::null_cubemap_tex_handle.get())
        {
            RendererInterface::m_pimpl->BindTextureHandle(11, radiance_texture_id);
            shader->SetUseCubeTexture1(true);
        }
        else
        {
            shader->SetUseCubeTexture1(false);
        }

        auto irradiance_texture_id = skyboxes[2]->GetTextureHandle();
        if (irradiance_texture_id != 0
            && irradiance_texture_id != AssetImage::null_image_tex_handle.get()
            && irradiance_texture_id != AssetImage::null_cubemap_tex_handle.get())
        {
            RendererInterface::m_pimpl->BindTextureHandle(12, irradiance_texture_id);
            shader->SetUseCubeTexture2(true);
        }
        else
        {
            shader->SetUseCubeTexture2(false);
        }
    }
    else
    {
        shader->SetUseCubeTexture1(false);
        shader->SetUseCubeTexture2(false);
    }

    bool const has_cubemap = !cubemap.isNull();
    bool const has_radiance_cubemap = !cubemap_radiance.isNull();
    bool const has_irradiance_cubemap = !cubemap_irradiance.isNull();

    // If we have a valid loaded cubemap and either of our radiance and irradiance maps are null
    // and we haven't already requested cubemap processing then save out the faces of the cubemap and
    // register them for processing.
    TextureHandle* gl_tex_id = nullptr;
    if (has_cubemap)
    {
        gl_tex_id = cubemap->GetTextureHandle();
        qint64 room_url_md5 = MathUtil::hash(GetS("url"));
        FilteredCubemapManager* cubemap_manager = FilteredCubemapManager::GetSingleton();
        PROCESSING_STATE current_processing_state = cubemap_manager->GetProcessingState(room_url_md5);
        bool const has_valid_cubemap = (
                gl_tex_id != 0
                && gl_tex_id != AssetImage::null_cubemap_tex_handle.get()
                && gl_tex_id != AssetImage::null_image_tex_handle.get());


        if (has_valid_cubemap
            && (!has_radiance_cubemap || !has_irradiance_cubemap)
            && current_processing_state == PROCESSING_STATE::INVALID)
        {
            QString room_url_md5_string = QString::number(room_url_md5);
            QString room_save_filename = GetSaveFilename();
            bool is_room_local = QFileInfo(room_save_filename).exists();
            QString room_base_absolute_path = QFileInfo(GetSaveFilename()).absoluteDir().absolutePath();
            QString const cubemap_base_path = (is_room_local == false) ? MathUtil::GetCachePath(): room_base_absolute_path  + "/" ;
            QString cubemap_output_path_irradiance = cubemap_base_path + QString("%1_cubemap_irradiance64").arg(room_url_md5_string);
            QString cubemap_output_path_radiance = cubemap_base_path + QString("%1_cubemap_radiance256").arg(room_url_md5_string);

            QVector<QString> file_names;
            file_names.reserve(2);

            //save_cubemap_faces_to_cache(room_url_md5_string, file_names);

            file_names.push_back(cubemap_output_path_irradiance);
            file_names.push_back(cubemap_output_path_radiance);
            Cubemaps cubemaps(file_names, gl_tex_id);
            RendererInterface::m_pimpl->GenerateEnvMapsFromCubemapTextureHandle(cubemaps);
            cubemap_manager->RegisterForProcessing(room_url_md5, cubemaps);
        }
        else if (current_processing_state == PROCESSING_STATE::READY)
        {
            const QString cache_path = MathUtil::GetCachePath();
            qint64 room_url_md5 = MathUtil::hash(GetS("url"));
            QString room_url_md5_string = QString::number(room_url_md5);
            QVector <QPointer <AssetImage> > imgs = QVector <QPointer <AssetImage> > (1);
            QString room_save_filename = GetSaveFilename();
            bool is_room_local = QFileInfo(room_save_filename).exists();
            QString room_base_absolute_path = QFileInfo(GetSaveFilename()).absoluteDir().absolutePath();
            QString cubemap_base_path = (is_room_local == false) ? cache_path : room_base_absolute_path + '/';

            cubemap_base_path = QUrl::fromLocalFile(cubemap_base_path).toString();

            const QString rad_src = QString("./%1_cubemap_radiance256.dds").arg(room_url_md5_string);
            const QString irrad_src = QString("./%1_cubemap_irradiance64.dds").arg(room_url_md5_string);
//            qDebug() << "Room::BindCubemaps cubemap_base_path" << cubemap_base_path << rad_src << irrad_src;

            QPointer<AssetImage> radiance_image = new AssetImage();
            radiance_image->SetSrc(cubemap_base_path, rad_src);
            radiance_image->SetB("tex_clamp", true);
            radiance_image->SetB("tex_mipmap", true);
            radiance_image->SetS("id", "__CUBEMAP_RADIANCE");
            radiance_image->SetB("_save_to_markup", false); //60.0 - always add Asset to Room (so it links the texture), but do mark it for not-write-to-markup

            AddAssetImage(radiance_image);
            imgs[0] = GetAssetImage(radiance_image->GetS("id"));

            cubemap_radiance = new AssetSkybox();
            cubemap_radiance->SetAssetImages(imgs);

            QPointer<AssetImage> irradiance_image = new AssetImage();
            irradiance_image->SetSrc(cubemap_base_path, irrad_src);
            irradiance_image->SetB("tex_clamp", true);
            irradiance_image->SetB("tex_mipmap", true);
            irradiance_image->SetS("id", "__CUBEMAP_IRRADIANCE");
            irradiance_image->SetB("_save_to_markup", false); //60.0 - always add Asset to Room (so it links the texture), but do mark it for not-write-to-markup

            AddAssetImage(irradiance_image);
            imgs[0] = GetAssetImage(irradiance_image->GetS("id"));

            cubemap_irradiance = new AssetSkybox();
            cubemap_irradiance->SetAssetImages(imgs);

            // If room is not local this call will queue the filtered cubemap files for deletion.
            // We keep the files for local rooms as we've just added them as AssetImages in the room markup.
            cubemap_manager->RemoveFromProcessing(room_url_md5, !is_room_local);
        }
    }

    if (has_radiance_cubemap)
    {
        auto texID = cubemap_radiance->GetTextureHandle();
        if (texID != 0
            && texID != AssetImage::null_image_tex_handle.get()
            && texID != AssetImage::null_cubemap_tex_handle.get())
        {
            RendererInterface::m_pimpl->BindTextureHandle(11, texID);
            shader->SetUseCubeTexture1(true);
        }
        else
        {
            shader->SetUseCubeTexture1(false);
        }
    }

    if (has_irradiance_cubemap)
    {
        auto texID = cubemap_irradiance->GetTextureHandle();
        if (texID != 0
            && texID != AssetImage::null_image_tex_handle.get()
            && texID != AssetImage::null_cubemap_tex_handle.get())
        {
            RendererInterface::m_pimpl->BindTextureHandle(12, texID);
            shader->SetUseCubeTexture2(true);
        }
        else
        {
            shader->SetUseCubeTexture2(false);
        }
    }
}

void Room::GetLights(LightContainer* p_container)
{
    QMatrix4x4 model_matrix;

    for (QPointer <RoomObject> & o : envobjects) {
        if (o && o->GetParentObject().isNull()) {
            o->GetLights(p_container, &model_matrix);
        }
    }
}

void Room::DrawGL(MultiPlayerManager *multi_players, QPointer <Player> player, const bool render_left_eye, const bool draw_player, const bool draw_portal_decorations)
{
//    qDebug() << "Room::DrawGL - " << GetURL() << "Drawing objects:" << envobjects.size() << "Portals:" << portals.size();
    // If our default shader is not valid, early out
    if (transparency_shader.isNull()) {
        return;
    }

    //general updates
    for (QPointer <AssetObject> a : assetobjects) {
        if (a) {
            a->UpdateGL();
        }
    }

    for (QPointer <AssetImage> a : assetimages) {
        if (a) {
            a->UpdateGL();
        }
    }

    for (QPointer <AbstractWebSurface> a : assetwebsurfaces) {
        if (a) {
            a->UpdateGL();
        }
    }

    const bool allow_glsl_shaders = SettingsManager::GetShadersEnabled();

    RendererInterface * renderer = RendererInterface::m_pimpl;

    //59.3 - iterate through all of this room's shaders, updating room-specific shader uniforms (and applying cubemap textures)    
    for (QPointer <AssetShader> & s : assetshaders) {
        if (s) {
            BindShader(s);
        }
    }

    // Check if we have a valid room_shader to override the default transparency shader
    QPointer <AssetShader> room_shader = transparency_shader;
    if (assetshader && assetshader->GetCompiled() && allow_glsl_shaders) {
        room_shader = assetshader;
    }

    // Build list of Lights that exist in this room
    LightContainer light_container;
    GetLights(&light_container);
    light_container.m_lights.emplace_back(Light());
    renderer->PushLightContainer(&light_container, renderer->GetStencilFunc().GetStencilReferenceValue());

    BindShader(room_shader);
    room_shader->UpdateObjectUniforms();

    // Draw template room (if specified and visible)
    const QString s = GetS("use_local_asset");
    if (room_templates.contains(s) && room_templates[s] && room_templates[s]->GetEnvObject() && GetB("visible")) {
        room_templates[s]->GetEnvObject()->SetC("col", GetC("col"));
        room_templates[s]->GetEnvObject()->DrawGL(room_shader, false, player->GetV("pos"));
    }

    //DEBUG HELP - show where BULLET thinks everything is
//    for (int i=0; i<envobjects_ordering.size(); ++i) {
//        const QString jsid = envobjects_ordering[i];

//        if (envobjects.contains(jsid) && !envobjects[jsid].isNull() && envobjects[jsid]->GetParentObject().isNull())
//        {
//            QPointer <RoomObject> obj = envobjects[jsid];

//            if (obj->GetType() == TYPE_OBJECT && obj->GetCollisionAssetObject()) {

//                obj->SetInterpolate(false);
//                if (physics->GetRigidBody(obj)) {

//                    btTransform & t = physics->GetRigidBody(obj)->getWorldTransform();
//                    btScalar m[16];
//                    t.getOpenGLMatrix(&m[0]);

//                    QMatrix4x4 m2;
//                    memcpy(m2.data(), m, 15 * sizeof(float));

//                    m2.scale(obj->GetScale());

//                    MathUtil::PushModelMatrix();
//                    MathUtil::MultModelMatrix(m2);

//                    room_shader->SetConstColour(QVector4D(1,0,0,1));
//                    obj->GetCollisionAssetObject()->DrawGL(room_shader);

//                    MathUtil::PopModelMatrix();
//                }

//            }
//        }
//    }

    // Draw this player through a portal
    if (draw_player)
    {
        QPointer <RoomObject> player_avatar = multi_players->GetPlayer();

        GhostFrame frame0;
        GhostFrame frame1;

        frame1.time_sec = 1.0f;
        frame1.pos = player->GetV("pos");
        frame1.dir = player->GetV("dir");

        frame1.head_xform.setColumn(0, QVector3D::crossProduct(player->GetV("up_dir"), player->GetV("view_dir")).normalized());
        frame1.head_xform.setColumn(1, player->GetV("up_dir"));
        frame1.head_xform.setColumn(2, player->GetV("view_dir"));
        frame1.head_xform.setColumn(3, QVector4D(player->GetV("local_head_pos"), 1));

        frame1.speaking = player->GetB("speaking");
        frame1.typing = player->GetB("typing");
        frame1.hands = player->GetHands();
        frame1.cursor_active = player->GetCursorActive(0);

        frame1.cursor_xform.setColumn(0, player->GetCursorXDir(0));
        frame1.cursor_xform.setColumn(1, player->GetCursorYDir(0));
        frame1.cursor_xform.setColumn(2, player->GetCursorZDir(0));
        frame1.cursor_xform.setColumn(3, QVector4D(player->GetCursorPos(0), 1));
        frame1.cscale = player->GetCursorScale(0);

        frame1.hmd_type = player->GetS("hmd_type");

        // Set first and last frames to be the same (current packet)
        frame0 = frame1;
        frame0.time_sec = 0.0f;

        if (player_avatar->GetAssetGhost()) {
            QVector <GhostFrame> frames;
            frames.push_back(frame0);
            frames.push_back(frame1);
            player_avatar->GetAssetGhost()->SetFromFrames(frames, 1000);
        }

        //set stuff so player looks right through mirrors
        player_avatar->SetHMDType(player->GetS("hmd_type"));
        player_avatar->DrawGL(room_shader, render_left_eye, player->GetV("eye_point"));
    }

    // Draw other players
    multi_players->DrawGL(room_shader, player_pos_trans, render_left_eye);
    if (GetB("cursor_visible")) {
        multi_players->DrawCursorsGL(room_shader);
    }

    // Draw Roomobjects
    for (QPointer <RoomObject> & obj : envobjects) {
        if (obj && obj->GetParentObject().isNull() && obj->GetType() != "link") {

            obj->SetPlaySounds(!use_clip_plane);

            // If this object has a custom shader, bind it for its draw then unbind and return to the room_shader
            QPointer <AssetShader> obj_shader = obj->GetAssetShader();
            bool const using_shader = obj_shader && obj_shader->GetCompiled() && allow_glsl_shaders;
            QPointer <AssetShader> current_shader = (using_shader) ? obj_shader : room_shader;
            if (using_shader) {
                UnbindShader(room_shader);
                BindShader(obj_shader);
            }
            else {
                //59.6 - Attention Devlin - shader needs to be rebound for each object, as it calls BindCubemaps, which resets the texture indexes for rad/irrad maps
                // to prevent those indexes from being overwritten by the last drawn object, see:
                // https://trello.com/c/i513wMO6/998-121017-if-you-have-object-specific-rad-irrad-map-any-new-spawned-object-inherits-the-object-specific-instead-of-global-room-prob
                //
                //If you find a more efficient way to preserve the previous textures bound to texture channels 11 and 12 (rad and irrad), please switch this to that as this
                //fix is probably not that efficient.
                BindShader(room_shader);
            }

            obj->DrawGL(current_shader, render_left_eye, player_pos_trans);

            if (using_shader) {
                UnbindShader(obj_shader);
                BindShader(room_shader);
            }
        }
    }

    // Draw portals that don't have a valid thumb_id and also draw portal decorations if enabled
    UnbindShader(room_shader);
    renderer->BeginScope((!draw_portal_decorations)
                         ? RENDERER::RENDER_SCOPE::CURRENT_ROOM_PORTAL_DECORATIONS
                         : RENDERER::RENDER_SCOPE::CHILD_ROOM_PORTAL_DECORATIONS);
    renderer->SetDepthMask(DepthMask::DEPTH_WRITES_DISABLED);

    // If this object has a custom shader, bind it for its draw then unbind and return to the room_shader
    QPointer <RoomObject> player_room_object = multi_players->GetPlayer();
    QHash <QString, QPointer <AssetShader> > player_portal_shader = player_room_object->GetGhostAssetShaders();
    QPointer <AssetShader> user_portal_shader = player_portal_shader["CustomPortalShader"];
    bool const using_shader = user_portal_shader && user_portal_shader->GetCompiled();

    if (!using_shader) {
        user_portal_shader = portal_shader;
    }
    BindShader(user_portal_shader);

    for (auto & o : envobjects) {
        if (o && o->GetType() == "link" && o->GetB("visible")) {
            //59.0 bugfix - draw back if not in child room, and we are distant, and it's not a mirror
            o->SetB("_draw_back", !use_clip_plane && !o->GetPlayerAtSigned(player->GetV("eye_point")) && !o->GetB("mirror"));
            o->DrawGL(user_portal_shader);

            QPointer <Room> r = GetConnectedRoom(o);
            if (draw_portal_decorations) {
                const float val = ((r && r->GetStarted() && !r->GetReady()) ? r->GetProgress() : 1.0f);
                renderer->SetDefaultFaceCullMode(FaceCullMode::DISABLED);
                o->DrawDecorationsGL(user_portal_shader, val);
                renderer->SetDefaultFaceCullMode(FaceCullMode::BACK);
            }
        }
    }

    UnbindShader(user_portal_shader);

    renderer->EndCurrentScope();
}

bool Room::GetMountPoint(QVector3D & pos, QVector3D & dir)
{
//    qDebug() << "Room::GetMountPoint() mount free" << cur_mount << room_template;
    const QString s = GetS("use_local_asset");
    int cur_mount = GetI("_cur_mount");
    if (room_templates.contains(s) && room_templates[s] && (room_templates[s]->GetNumMounts() - cur_mount) > 0) {
        room_templates[s]->GetMount(cur_mount, pos, dir);
        ++cur_mount;
        SetI("_cur_mount", cur_mount);
        return true;
    }
    return false;
}

int Room::GetNumMountPointsFree() const
{
    const QString s = GetS("use_local_asset");
    const int cur_mount = GetI("_cur_mount");
    if (room_templates.contains(s) && room_templates[s]) {
        return (room_templates[s]->GetNumMounts() - cur_mount);
    }
    else {
        return 0;
    }
}

QPointer <RoomObject> Room::GetRoomObject(const QString js_id)
{
    if (!js_id.isEmpty() && envobjects.contains(js_id) && envobjects[js_id]) {
        return envobjects[js_id];
    }
    return QPointer <RoomObject> ();
}

QHash <QString, QPointer <RoomObject> > & Room::GetRoomObjects()
{
    return envobjects;
}

QPointer <AssetSkybox> Room::GetCubemap(CUBEMAP_TYPE p_skybox_type)
{
    QPointer <AssetSkybox> skybox = nullptr;
    switch(p_skybox_type)
    {
    case CUBEMAP_TYPE::RADIANCE:
        skybox = cubemap_radiance;
        break;
    case CUBEMAP_TYPE::IRRADIANCE:
        skybox = cubemap_irradiance;
        break;
    case CUBEMAP_TYPE::DEFAULT:
    default:
        skybox = cubemap;
        break;
    }

    return skybox;
}

void Room::DrawSkyboxGL(QPointer <AssetShader> shader, const QMatrix4x4 & model_matrix)
{    
    //59.0 - prevent seeing glitches/random memory as room is loading (cubemap may be != null but texture handle doesn't seem ready)
    if (cubemap && cubemap->GetTextureHandle()) {
        cubemap->DrawGL(shader, model_matrix);
    }
    else if (!skyboxes.isEmpty()) {
        skyboxes[0]->DrawGL(shader, model_matrix);
    }
}

void Room::DoEditsDeletes(QPointer <RoomObject> obj)
{    
    if (obj.isNull()) {
        return;
    }

    //any room edits or deletes to make?
    QList <RoomObjectEdit> & room_edits = obj->GetRoomEditsIncoming();
    QList <RoomObjectEdit> & room_deletes = obj->GetRoomDeletesIncoming();

    if (!GetB("locked")) {
        for (int i=0; i<room_edits.size(); ++i) {
            for (int j=0; j<room_edits[i].data.size(); ++j) {
                DoEdit(room_edits[i].data[j]);
            }
        }
        for (int i=0; i<room_deletes.size(); ++i) {
            for (int j=0; j<room_deletes[i].data.size(); ++j) {
                DoDelete(room_deletes[i].data[j]);
            }
        }
    }

    room_edits.clear();
    room_deletes.clear();

    //any portals to create?
    QList <QString> & send_portal_url = obj->GetSendPortalURL();
    QList <QString> & send_portal_jsid = obj->GetSendPortalJSID();
    QList <QVector3D> & send_portal_pos = obj->GetSendPortalPos();
    QList <QVector3D> & send_portal_fwd = obj->GetSendPortalFwd();
    for (int i=0; i<send_portal_url.size(); ++i) {
        QColor child_col;
        child_col.setHsl(int(30.0f * envobjects.size()), 128, 128);

        QPointer <RoomObject> new_portal = new RoomObject();
        new_portal->SetType("link");
        new_portal->SetURL(send_portal_url[i], send_portal_url[i]);
        new_portal->SetTitle("");
        new_portal->SetV("pos", send_portal_pos[i]);
        new_portal->SetDir(send_portal_fwd[i]);
        new_portal->SetC("col", child_col);
        new_portal->SetV("scale", QVector3D(1.8f, 2.5f, 1.0f));
        new_portal->SetB("_circular", true);
        new_portal->SetS("js_id", send_portal_jsid[i]);
        AddRoomObject(new_portal);
    }
    send_portal_url.clear();
    send_portal_jsid.clear();
    send_portal_pos.clear();
    send_portal_fwd.clear();
}

void Room::UpdateAssets()
{
    for (QPointer <AssetSkybox> & a : skyboxes) {
        if (a) {
            a->UpdateAssets();
        }
    }

    for (QPointer <RoomTemplate> & r : room_templates) {
        if (r) {
            r->UpdateAssets();
        }
    }

    //general updates
    for (QPointer <AssetGhost> a : assetghosts) {
        if (a) {
            if (!a->GetStarted()) {
                a->Load();
            }
            a->Update();
        }
    }

    for (QPointer <AssetRecording> a : assetrecordings) {
        if (a) {
            if (!a->GetStarted()) {
                a->Load();
            }
            a->Update();
        }
    }
    for (QPointer <AssetObject> a : assetobjects) {
        if (a) {
            if (!a->GetStarted()) {
                a->Load();
            }
            a->Update();
            a->UpdateGL();
        }
    }

    for (QPointer <AssetImage> a : assetimages) {
        if (a) {
            if (!a->GetStarted()) {
                a->Load();
            }
            a->UpdateGL();
        }
    }

    for (QPointer <AbstractWebSurface> a : assetwebsurfaces) {
        if (a) {
            if (!a->GetStarted()) {
                a->SetStarted(true);
                a->Load();
            }
            a->GetWebView()->update();
            a->UpdateGL();
        }
    }
}

void Room::UpdateObjects(QPointer <Player> player, MultiPlayerManager *multi_players, const bool player_in_room)
{
    page->Update();

    //update player entrance position if uninitialized/unchanged
    if (entrance_object && player_lastxform.isIdentity()) {
        player_lastxform = entrance_object->GetModelMatrixLocal();
    }

    QPointF pos_pt(player->GetV("pos").x(), player->GetV("pos").z());

    //any edits to room assets?
    for (QPointer <AssetImage> & a : assetimages) {
        if (a && a->GetB("sync")) {
            a->SetB("sync", false);
            multi_players->SetRoomAssetEdit(a->GetXMLCode());

            //59.5 - report room asset errors
            if (a->GetLoaded() && a->GetError() && !a->GetErrorLogged()) {
                MathUtil::ErrorLog(a->GetErrorString());
                a->SetErrorLogged(true);
            }
        }
    }
    for (QPointer <AssetObject> & a : assetobjects) {
        if (a && a->GetB("sync")) {
            a->SetB("sync", false);
            multi_players->SetRoomAssetEdit(a->GetXMLCode());

            //59.5 - report room asset errors
            if (a->GetLoaded() && a->GetError() && !a->GetErrorLogged()) {
                MathUtil::ErrorLog(a->GetErrorString());
                a->SetErrorLogged(true);
            }
        }
    }

    //any edits to room objects?
    QHash <QString, QPointer <RoomObject> >::iterator it;
    for (it=envobjects.begin(); it!=envobjects.end(); ++it) {
        QPointer <RoomObject> obj = it.value();

        if (obj) {           

            LinkToAssets(obj); //update object links

            obj->SetPlayerInRoom(player_in_room);
            obj->Update(player->GetF("delta_time")); //object's internal update stuff

            if (obj->GetTeleportAssetObject() != 0) {
                SetB("_teleport_override", true);
            }

            if (player_in_room) {

                //update synchronization with other players (if boolean set)
                if (obj->GetB("sync")) {
                    obj->SetB("sync", false);
                    multi_players->SetRoomEdit(obj);
                    //qDebug() << "Room::UpdateObjects" << obj.GetFireBoxCode(true);                    

                    //54.10 - any syncing triggers update to physics engine
                    if (!obj->GetSelected()) {
                        if (physics) {
                            physics->UpdateToRigidBody(obj);
                        }
                    }
                }

                QPointer <AssetSound> snd = obj->GetAssetSound();
                //55.9 - note the ordering: confirm the sound is ready before calling TestPlayerPosition
                if (obj->GetType() == "sound" && snd && snd->GetReady(obj->GetMediaContext()) && obj->TestPlayerPosition(pos_pt)) {
                    obj->Play();
                }
            }

            DoEditsDeletes(obj);

            //collision updates
            //do all pairwise collision intersections (calls for room.onCollisionEnter and room.onCollisionExit)
            //check room readiness to prevent early/false intersections at first load
//            qDebug() << "Room::UpdateObjects" << ready << obj->GetJSID() << obj->GetCollisionAssetObject() << obj->GetCollisionTrigger();
            if (physics && GetB("_ready") && obj->GetCollisionAssetObject() && obj->GetB("collision_trigger")) {
                //iterate over those intersected with
                QSet <QString> s0 = physics->GetRigidBodyCollisions(obj);
                QSet <QString> s1 = obj->GetCollisionSet();

//                qDebug() << "testing" << obj->GetJSID() << s0 << s1;                
                //process onenter
                for (const QString & s : s0) {
                    QPointer <RoomObject> o = GetRoomObject(s);
                    if (o && !s1.contains(s)) {
                        QList <QPointer <DOMNode> > args;
                        args.push_back(envobjects[obj->GetS("js_id")]->GetProperties());
                        args.push_back(envobjects[o->GetS("js_id")]->GetProperties());
                        CallJSFunction("room.onCollisionEnter", player, args);
                    }
                }
                //process onexit
                for (const QString & s : s1) {
                    QPointer <RoomObject> o = GetRoomObject(s);
                    if (o && !s0.contains(s)) {
                        QList <QPointer <DOMNode> > args;
                        args.push_back(envobjects[obj->GetS("js_id")]->GetProperties());
                        args.push_back(envobjects[o->GetS("js_id")]->GetProperties());
                        CallJSFunction("room.onCollisionExit", player, args);
                    }
                }

                obj->SetCollisionSet(s0);
            }
        }                
    }

    //do room/edits delets for multi_players
    QList <QPointer <RoomObject> > ps = multi_players->GetPlayersInRoom(GetS("url"));
    for (int i=0; i<ps.size(); ++i) {
        DoEditsDeletes(ps[i]);
    }

    //check for sync for portals
    bool player_near_portal = false;
    float player_near_portal_height = 0.0f;

    for (QPointer <RoomObject> & o : envobjects) {
        if (o && o->GetType() == "link") {
            //update portal highlight
            const bool portal_selected = ((player->GetCursorObject(0) == o->GetS("js_id")) ||
                                          (player->GetCursorObject(1) == o->GetS("js_id")));
            o->SetB("_highlighted", portal_selected);
            o->Update(player->GetF("delta_time")); //object's internal update stuff

            if (o->GetB("sync")) {
                o->SetB("sync", false);
                multi_players->SetRoomEdit(o);                
            }

            //allow dynamic updating of thumb_id so portal assetimage thumbnail changes
            o->SetThumbAssetImage(GetAssetImage(o->GetS("thumb_id")));

            //update player collision sets based on portal being: not mirror, open, having a room that has been processed, active, and proximity
            if (o->GetB("open") && o->GetB("active") && o->GetPlayerAtSigned(player->GetV("eye_point"))) {
                QPointer <Room> r = GetConnectedRoom(o);
                if (r && r->GetLoaded()) {
                    player_near_portal = true;
                    player_near_portal_height = o->GetV("pos").y();
                }
            }
        }
    }

    if (physics) {
        physics->SetPlayerNearPortal(player_near_portal, player_near_portal_height);
    }

    const QString s = GetS("use_local_asset");
    float progress = GetF("_progress");
    if ((!GetB("_ready_for_screenshot") || !GetB("_ready") || progress < 1.0f) && GetProcessed() && (envobjects.size() > 0 || (room_templates.contains(s) && room_templates[s])) && !GetB("_translator_busy")) {

        bool is_room_ready = true;
        bool is_room_ready_for_screenshot = true;

        int nObjects = 0;
        int nImages = 0;

        progress = 0.0f;

        for (QPointer <AssetObject> & a: assetobjects) {
            if (a) {
                ++nObjects;
                progress += a->GetProgress();
                if (!a->GetFinished()) {
                    is_room_ready = false;
                    is_room_ready_for_screenshot = false;
                }
                if (!a->GetTexturesFinished()) {
                    is_room_ready_for_screenshot = false;
                }
//                qDebug() << "object" << a->GetProperty("src") << a->GetFinished() << a->GetTexturesFinished();
            }
        }        

        for (QPointer <AssetImage> & a: assetimages) {
            if (a) {
                ++nImages;
                progress += a->GetProgress();
                if (!a->GetFinished()) {
                    is_room_ready_for_screenshot = false;
                }
//                qDebug() << "image" << a->GetProperty("src") << a->GetFinished();
            }
        }

        if (nObjects != 0 || nImages != 0){
            progress /= float(nObjects + nImages);
        }
        else{
            progress = 0.0f;
        }

        SetF("_progress", progress);

        //qDebug() << "progress" << progress;
        if (is_room_ready) {
            SetB("_ready", true);
        }
        if (is_room_ready_for_screenshot) {
            SetB("_ready_for_screenshot", true);
        }
    }
}

QList <QScriptValue> & Room::GetQueuedFunctions()
{
    return queued_functions;
}

void Room::SetPlayerInRoom(QPointer <Player> player)
{
    player->SetS("url", GetS("url")); //55.1 - update player URL immediately so room triggers in UpdateObjects dont accidentally fire
    if (physics) {
        physics->UpdateToRigidBody(player);
    }
}

QPointer <RoomPhysics> Room::GetPhysics()
{
    return physics;
}

QPointer <QScriptEngine> Room::GetScriptEngine()
{
    return script_engine;
}

void Room::UpdatePhysics(QPointer <Player> player)
{
    if (physics) {
        //update gravity
        const double dt = player->GetF("delta_time");
        const float gravity = GetF("gravity");
        const float player_gravity = (player->GetB("flying") ? 0.0f : gravity);
        physics->SetGravity(GetB("_ready") ? gravity : 0.0f);
        physics->SetPlayerGravity(GetB("_ready") ? player_gravity : 0.0f);

        //update jump velocity
        physics->SetJumpVelocity(GetF("jump_velocity"));

        //update room template
        const QString s = GetS("use_local_asset");
        if (room_templates.contains(s) && room_templates[s]) {
            physics->AddRoomTemplate(room_templates[s]->GetEnvObject());
        }

        //update player
        physics->AddPlayerShape(player);

        //update player standing on ground test
        physics->DoPlayerGroundTest(player);

        //iterate over all objects (first pass to allow changes in JS to object position pre-simulation step)
        QHash <QString, QPointer <RoomObject> >::iterator it;
        for (it=envobjects.begin(); it!=envobjects.end(); ++it) {
            if (it.value().isNull()) {
                continue;
            }

            if (it.value()->GetType() == "object" || it.value()->GetType() == "ghost") {
                if (it.value()->GetS("collision_id").length() > 0) {
                    if (physics->GetRigidBody(it.value()) == NULL) {
                        //add any not already in the simulation, and have dynamics set to true
                        physics->AddRigidBody(it.value(), COL_WALL, COL_PLAYER | COL_WALL);
                    }

                    //Handles JS changes to position, scale, and collision_static (when true mass should be = 0, when false mass should be != 0)
                    const QString jsid = it.value()->GetS("js_id");
                    if (it.value()->GetPos() != physics->GetRigidBodyPos(jsid) ||
                               it.value()->GetScale() != physics->GetRigidBodyScale(jsid) ||
                                it.value()->GetXDir() != physics->GetRigidBodyXDir(jsid) ||
                                it.value()->GetVel() != physics->GetRigidBodyVel(jsid)) {
                        physics->UpdateToRigidBody(it.value());
                    }
                }
                else {
                    //58.0 - remove objects as a result of js_id being set to NULL via JS
                    if (physics->GetRigidBody(it.value()) != NULL) {
                        physics->RemoveRigidBody(it.value());
                    }
                }
            }
        }

        //54.8 - if player position is changed via JS, update simulation
        if (player->GetV("pos") != physics->GetRigidBodyPos("__player")) {
            physics->UpdateToRigidBody(player);
        }

        //update the simulation
        physics->UpdateSimulation(dt); //59.0 - provide consistent speed even when FPS is low

        //update player
        physics->UpdateFromRigidBody(player);

        //iterate over all objects (this time writing to objects after the simulation step)
        for (it=envobjects.begin(); it!=envobjects.end(); ++it) {
            if (it.value().isNull() || it.value()->GetType() != "object" || it.value()->GetS("collision_id").length() <= 0) {
                continue;
            }

            if (!it.value()->GetB("collision_static") && !it.value()->GetSelected()) {
                physics->UpdateFromRigidBody(it.value());
            }
        }
    }
}

void Room::UpdateJS(QPointer <Player> player)
{    
    bool all_scripts_ready = (!assetscripts.isEmpty() && GetProcessed());
    const QVector3D d = player->GetV("dir");
    for (QPointer <AssetScript> & script : assetscripts) {
        if (script) {
            script->Update();
            LogErrorOnException();
//            qDebug() << "Room::UpdateJS evaluating" << script->GetS("src");

            if (script->GetFinished()) {

                //update objects stuff
                if (!script->GetOnLoadInvoked()) {
//                    qDebug() << "scriptonload" << script << script->HasRoomFunction("onLoad") << script->GetOnLoadInvoked();
                    script->SetOnLoadInvoked(true);
                    QList<QPointer <RoomObject> > objectsAdded = script->DoRoomLoad(envobjects, player);
                    LogErrorOnException();
                    AddRoomObjects(objectsAdded);
                }
                else {
//                    qDebug() << " calling update";
                    QList<QPointer <RoomObject> > objectsAdded = script->DoRoomUpdate(envobjects, player, QScriptValueList() << (int)(player->GetF("delta_time") * 1000));
                    LogErrorOnException();
                    AddRoomObjects(objectsAdded);
                }

                //process all deferred functions
                QScriptValueIterator itr(script->GetGlobalProperty("room").property(ScriptBuiltins::janus_queued_functions));
                while (itr.hasNext()) {                    
                    itr.next();
                    if (itr.flags() & QScriptValue::SkipInEnumeration) {
                        continue;
                    }
                    queued_functions.push_back(itr.value());
                }
                script->GetGlobalProperty("room").property(ScriptBuiltins::janus_queued_functions).setProperty("length", 0);               
            }
            else {
                all_scripts_ready = false;
            }
        }
    }

    // 59.13 - once all scripts are loaded, invoke any/all custom elements creation, since all constructors should be defined
    if (all_scripts_ready) {
        scripts_ready = all_scripts_ready;

        //Iterate through all registered element types, looking for tag matches with what is in markup?
        if (!custom_elements_processed) {
//            qDebug() << "Room::UpdateJS() - Processing custom elements";
            custom_elements_processed = true;

            // 59.13 - sync JS DOM data structure with DOMNode's in C++ world (otherwise getObjectByID to obtain QScriptValue doesn't work)
            for (QPointer <AssetScript> & script : assetscripts) {
                if (script) {
                    script->UpdateInternalDataStructures(player);
                }
            }
        }
    }

    if (scripts_ready && custom_elements_processed) {

        QScriptValue globalObject = script_engine->globalObject();
        QScriptValue roomObject = globalObject.property("room");
        QScriptValue s = globalObject.property("__custom_elements");
        const int num = s.property("length").toInteger();
        //        qDebug() << "__custom_elements" << s.toString() << s.property("length").toInteger();

        QList <QPointer <DOMNode> > d;
        d.push_back(props);

        for (int k=0; k<d.size(); ++k) {

            QPointer <DOMNode> o = d[k];
            if (o) {
                QList <DOMNode *> c = o->GetChildren();
                for (int j=0;j<c.size(); ++j) {
                    if (!d.contains(c[j])) {
                        d.push_back(c[j]);
                    }
                }

                if (o->GetIsCustomElement() && !o->GetB("_custom_element_processed")) {
                    o->SetB("_custom_element_processed", true);

                    for (int i=0; i<num; ++i) {
                        const QString t = s.property(i).property("_type").toString();
    //                    qDebug() << " " << i << t;
                        if (o->GetS("_type") == t) {
    //                        qDebug() << k << o->GetS("js_id") << o->GetS("id") << o->GetS("_type") << o->GetIsCustomElement() << o->GetB("_custom_element_processed");
                            //we need to run the create function on this, as well as set any other properties (custom or not)
                            //we now assign all properties to o that are in this registerelement object/hash
                            QScriptValue n = script_engine->globalObject().property("__dom").property(o->GetS("js_id"));
                            DOMNode * m = qscriptvalue_cast<DOMNode* >(n);
                            if (!n.isUndefined() && n.isValid() && m) {
                                QList <QByteArray> keys = m->dynamicPropertyNames();
                                // do property assignments to object via what's defined when element was registered
                                QScriptValue o2 = s.property(i).toObject();
                                QScriptValueIterator itr(o2);
                                while (itr.hasNext()) {
                                    itr.next();
                                    if( itr.flags() & QScriptValue::SkipInEnumeration ) {
                                        continue;
                                    }
                                    //60.0 - set key from registered element but ONLY IF it is not already defined in markup
                                    if (!keys.contains(itr.name().toLatin1())) {
                                        n.setProperty(itr.name().toStdString().c_str(), itr.value());
                                    }
                                }

                                // call create/createChildren using the QScriptValue in the JS environment
                                QScriptValue returnVal;
                                if (n.property("create").isFunction()) {
                                    returnVal = n.property("create").call(n);
                                }
                                else if (n.property("createChildren").isFunction()) {
    //                                qDebug() << "Room::UpdateJS() CreateChildren" << js_id << t;
                                    returnVal = n.property("createChildren").call(n);
                                }
                                // 60.0 - how to handle intermediate creation of assets, calls to LoadNewAsset WITHIN Create/CreateChildren?
                            }
                        }
                    }
                }
            }
        }
    }

    //player dir may have updated
    if (player->GetV("dir") != d) {
        player->UpdateDir();
    }
}

bool Room::HasJSFunctionContains(const QString & s, const QString & code)
{
    for (QPointer <AssetScript> & script : assetscripts) {
        if (script && script->HasRoomFunctionContains(s, code)) {
            return true;
        }
    }
    return false;
}

void Room::LogErrorOnException()
{
    if (script_engine->hasUncaughtException()) {
//        const int script_error_line = script_engine->uncaughtExceptionLineNumber();
        const QString script_error_str = script_engine->uncaughtException().toString();
        const QStringList trace = script_engine->uncaughtExceptionBacktrace();

        QString err = QString("JS exception: ") + script_error_str;
        err += "\n Backtrace:";
        for (int i=0; i<trace.size(); ++i) {
            err += "\n  " + trace[i];
        }

        MathUtil::ErrorLog(err);
        script_engine->clearExceptions();
    }
}

void Room::CallJSFunction(const QString & s, QPointer <Player> player, QList <QPointer <DOMNode> > nodes)
{
    const QVector3D d = player->GetV("dir");

    for (QPointer <AssetScript> & script : assetscripts) {
        if (script) {            
            QList<QPointer <RoomObject> > objectsAdded;
            if (script->HasFunction(s)) {
                QScriptValueList args;
                for (QPointer <DOMNode> & n : nodes) {
                    args << (n ? script_engine->toScriptValue(n) : QScriptValue());
                }
                objectsAdded = script->RunFunctionOnObjects(s, envobjects, player, args);
                LogErrorOnException();
            }
            else {
                objectsAdded = script->RunScriptCodeOnObjects(s, envobjects, player);
                LogErrorOnException();
            }
            AddRoomObjects(objectsAdded);
        }
    }

    //player dir may have updated
    if (player->GetV("dir") != d) {
        player->UpdateDir();
    }
}

void Room::UpdateAutoPlay()
{
    if (GetB("_started_auto_play") || !GetB("_ready")) {
        return;
    }

//    qDebug() << "Room::UpdateAutoPlay()" << this->GetURL();
    SetB("_started_auto_play", true);

    for (QPointer <RoomObject> & obj : envobjects) {
        if (obj) {
            const QString t = obj->GetType();
            if (t == "video" || t == "object") {
                //so if object has a websurface, and either no image placeholder, or it's selected, then do updates
                QPointer <AbstractWebSurface> web = obj->GetAssetWebSurface();
//                if (web) {
//                    qDebug() << "Room::UpdateAutoPlay()" << web << web->GetS("src") << web->GetOriginalURL();
//                }
                if (web && obj->GetS("image_id").length() == 0 && obj->GetS("thumb_id").length() == 0) {
                    if (QString::compare(web->GetURL(), "about:blank") == 0) {
//                        qDebug() << "SETTING!" << a->GetProperty("src").toString() << a->GetOriginalURL();
                        //a->SetURL(a->GetProperty("src").toString());
                        web->SetURL(web->GetOriginalURL());
                    }
                    if (QString::compare(obj->GetS("url"), "about:blank") == 0) {
                        obj->SetS("url", web->GetOriginalURL());
                    }
                }

                //auto-play videos
                QPointer <AssetVideo> vid = obj->GetAssetVideo();
                if (vid && vid->GetB("auto_play")) {
                    vid->SetSoundEnabled(obj->GetMediaContext(), SoundManager::GetEnabled());
                    vid->Play(obj->GetMediaContext());
                }
            }
            else if (t == "ghost") {
                if (obj->GetB("auto_play")) {
                    obj->Play();
                }                
            }
        }
    }
}

void Room::ResetSoundTriggers()
{
    QHash <QString, QPointer <RoomObject> >::iterator it;
    for (it=envobjects.begin(); it!=envobjects.end(); ++it) {
        QPointer <RoomObject> obj = it.value();
        if (obj && obj->GetType() == "sound") {
            obj->Stop();
            if (!obj->GetB("play_once")) {
                obj->SetB("_triggered", false);
            }
        }
    }

    SetB("_started_auto_play", false);
//    envobjects_queued_functions.clear();
}

void Room::StopAll()
{
    for (QPointer <RoomObject> & o : envobjects) {
        if (o) {
            QPointer <AssetSound> s = o->GetAssetSound();
            if (s && s->GetReady(o->GetMediaContext())) {
                s->Stop(o->GetMediaContext());
            }

            QPointer <AssetVideo> v = o->GetAssetVideo();
            if (v && v->GetReady(o->GetMediaContext())) {
                v->Stop(o->GetMediaContext());
            }
        }

        /*if (o && o->GetType() == TYPE_SOUND) {
            o->Stop();
        }
        if (o && o->GetType() == TYPE_VIDEO) {
            o->Stop();
        }*/
    }

    queued_functions.clear();

    for (QPointer <AbstractWebSurface> & a : assetwebsurfaces) {
//        qDebug() << "Room::StopAll()" << a << a->GetB("_save_to_markup");
        if (a && a->GetB("_save_to_markup")) {
            a->SetURL("about:blank");
        }
    }

    /*for (i=assets[ASSETTYPE_SOUND].begin(); i!=assets[ASSETTYPE_SOUND].end(); ++i) {
        const QPointer <AssetSound> a = i.value();
        if (a) {
            a->Stop(0);
        }
    }*/

    /*for (i=assets[ASSETTYPE_VIDEO].begin(); i!=assets[ASSETTYPE_VIDEO].end(); ++i) {
        const QPointer <AssetVideo> a = i.value();
        if (a) {
            a->Stop();
        }
    }*/
}

void Room::SetSoundsEnabled(const bool b)
{
    /*QHash<QString, unsigned int>::iterator i;
    for (i=assets[ASSETTYPE_SOUND].begin(); i!=assets[ASSETTYPE_SOUND].end(); ++i) {
        QPointer <AssetSound> a = i.value();
        if (a) {
            a->SetSoundEnabled(b);
        }
    }*/

    for (QPointer <RoomObject> & o : envobjects) {
        QPointer <AssetSound> s = o->GetAssetSound();
        if (s && s->GetReady(o->GetMediaContext())) {
            s->SetSoundEnabled(o->GetMediaContext(), b);
        }

        QPointer <AssetVideo> v = o->GetAssetVideo();
        if (v && v->GetReady(o->GetMediaContext())) {
            v->SetSoundEnabled(o->GetMediaContext(), b);
        }
        /*if (o && o->GetType() == TYPE_VIDEO){
            if (o->GetAssetVideo() && o->GetAssetVideo()){
                o->GetAssetVideo()->SetSoundEnabled(o->GetMediaContext(), b);
            }
        }
        if (o && o->GetType() == TYPE_SOUND){
            if (o->GetAssetSound() && o->GetAssetSound()){
                o->GetAssetSound()->SetSoundEnabled(o->GetMediaContext(), b);
            }
        }*/
    }

    /*for (i=assets[ASSETTYPE_VIDEO].begin(); i!=assets[ASSETTYPE_VIDEO].end(); ++i) {
        QPointer <AssetVideo> a = i.value();
        if (a) {
            a->SetSoundEnabled(b);
        }
    }*/
}


#ifdef __ANDROID__
void Room::Pause()
{
    for (QPointer <RoomObject> & o : envobjects) {
        if (o) {
            QPointer <AssetSound> s = o->GetAssetSound();
            if (s && s->GetReady(o->GetMediaContext()) && s->GetPlaying(o->GetMediaContext())) {
                if (s->CanPause(o->GetMediaContext())){
                    s->Pause(o->GetMediaContext());
                }
                else{
                    s->Stop(o->GetMediaContext());
                    //o->GetMediaContext()->setup = false;
                }
                o->GetMediaContext()->app_paused = true;
            }

            QPointer <AssetVideo> v = o->GetAssetVideo();
            if (v && v->GetReady(o->GetMediaContext()) && s->GetPlaying(o->GetMediaContext())) {
                if (v->CanPause(o->GetMediaContext())){
                    v->Pause(o->GetMediaContext());
                }
                else{
                    v->Stop(o->GetMediaContext());
                    //o->GetMediaContext()->setup = false;
                }
                o->GetMediaContext()->app_paused = true;
            }
        }
    }
}

void Room::Resume()
{
    for (QPointer <RoomObject> & o : envobjects) {
        if (o) {
            QPointer <AssetSound> s = o->GetAssetSound();
            if (s && s->GetReady(o->GetMediaContext()) && o->GetMediaContext()->app_paused) {
                if (!o->GetMediaContext()->setup){
                    s->SetupOutput(o->GetMediaContext(), o->GetB("loop"));
                }
                else{
                    s->Play(o->GetMediaContext());
                }
            }

            QPointer <AssetVideo> v = o->GetAssetVideo();
            if (v && v->GetReady(o->GetMediaContext()) && o->GetMediaContext()->app_paused) {
                if (!o->GetMediaContext()->setup){
                    v->SetupOutput(o->GetMediaContext());
                }
                else{
                    v->Play(o->GetMediaContext());
                }
            }
        }
    }
}
#endif

void Room::SyncAll()
{
    for (QPointer <RoomObject> & o : envobjects) {
        if (o && !o->GetB("locked")) {
            o->SetB("sync", true);
        }
    }
}

bool Room::SetSelected(const QString & selected, const bool b)
{
    QPointer <RoomObject> o = GetRoomObject(selected);
    if (o && !o->GetB("locked")) {
//            qDebug() << "setting envobject selected" << b << selected << envobjects[selected]->GetLocked();
        o->SetSelected(b);
        return true;
    }
    return false;
}

QString Room::GetSelectedCode(const QString & selected) const
{
    if (envobjects.contains(selected)) {
        return envobjects[selected]->GetXMLCode();
    }    
    return QString("");
}

QString Room::PasteSelected(const QString & selected, const QVector3D & p, const QVector3D & x, const QVector3D & y, const QVector3D & z, const QString & js_id)
{
    if (GetB("locked")) {
        MathUtil::ErrorLog("Warning: cannot do paste, room.locked=true");
        return "";
    }

    if (envobjects.contains(selected)) {
        QPointer <RoomObject> newobj = new RoomObject();
        newobj->Copy(envobjects[selected]); //note: newobj gets its own unique JSID, that is not overwritten by the copy
        newobj->SetS("js_id", js_id);
        newobj->SetV("pos", p);
        newobj->SetXDirs(x, y, z);
        return AddRoomObject(newobj);
    }
    else {
        return "";
    }

}

void Room::EditText(const QString & selected, const QString & c, const bool backspace)
{
    QPointer <RoomObject> o = GetRoomObject(selected);
    if (o) {
        const QString cur_text = o->GetText();
        if (backspace) {
            o->SetText(cur_text.left(cur_text.length()-1));
        }
        else {
            o->SetText(cur_text + c);
        }
    }
}

void Room::SelectAssetForObject(const QString & selected, const QString & new_id)
{
    QPointer <RoomObject> o = GetRoomObject(selected);
    if (o) {
        o->SetAssetObject(GetAssetObject(new_id));
    }
}

void Room::SelectCollisionAssetForObject(const QString & selected, const QString & new_coll_id)
{
    //qDebug() << "Room::SelectCollisionAssetForObject setting object's collisionid" << new_coll_id;
    QPointer <RoomObject> o = GetRoomObject(selected);
    if (o) {
        if (new_coll_id.length() > 0) {
            o->SetCollisionAssetObject(GetAssetObject(new_coll_id));
            o->SetS("collision_id", new_coll_id);

            //update physics object
            if (physics) {
                physics->RemoveRigidBody(envobjects[selected]);
                physics->AddRigidBody(envobjects[selected], COL_WALL, COL_PLAYER | COL_WALL);
            }
        }
        else {
            o->SetCollisionAssetObject(QPointer<AssetObject>());
            o->SetS("collision_id", "");

            //remove object from physics engine
            if (physics) {
                physics->RemoveRigidBody(o);
            }
        }
    }
}

void Room::SelectAssetForObject(const QString & selected, const int offset)
{
    QPointer <RoomObject> o = GetRoomObject(selected);
    if (o) {
        const QString t = o->GetType();

        if (t == "image") {
            QList <QPointer <AssetImage> > img_list = assetimages.values();
            if (!img_list.isEmpty()) {
                const int index = qMax(0, img_list.indexOf(assetimages[o->GetS("id")]));
                const int new_index = (img_list.size() + index + offset) % img_list.size();

                QPointer <AssetImage> new_img = img_list[new_index];
                if (new_img) {
                    o->SetAssetImage(new_img);
                    o->SetS("id", new_img->GetS("id"));
                }
            }
        }
        else if (t == "object") {
            QList <QPointer <AssetObject> > obj_list = assetobjects.values();
            if (!obj_list.isEmpty()) {
                const int index = qMax(0, obj_list.indexOf(assetobjects[o->GetS("id")]));
                const int new_index = (obj_list.size() + index + offset) % obj_list.size();

                QPointer <AssetObject> new_obj = obj_list[new_index];
                if (new_obj) {
                    o->SetAssetObject(new_obj);
                    o->SetS("id", new_obj->GetS("id"));
                    o->SetV("scale", QVector3D(1,1,1));
                    o->SetS("collision_id", new_obj->GetS("id"));
                    o->SetCollisionAssetObject(envobjects[selected]->GetAssetObject());
                }
            }
        }        
    }
}

bool Room::DeleteSelected(const QString & selected, const bool do_sync, const bool play_delete_sound)
{        
    if (GetB("locked")) {
        MathUtil::ErrorLog("Warning: cannot do delete, room.locked=true");
        return false;
    }

    bool did_delete = false;
    QPointer <RoomObject> o = GetRoomObject(selected);
    if (o && !o->GetB("locked"))
    {
        //59.7 - sync delete to other users
        if (do_sync) {
            MathUtil::room_delete_code += o->GetXMLCode();
        }

        if (!(o->GetType() == "link" && o->GetB("open")))
        {
            //remove this object from physics sim
            if (physics) {
                physics->RemoveRigidBody(o);
            }

            //remove this object from it's parent's child list
            if (o->GetParentObject()) {
                o->GetParentObject()->GetChildObjects().removeAll(o);
            }

            o->SetSelected(false);
            o->Stop();
            if (play_delete_sound) {
                o->PlayDeleteObject();
            }

            if (envobjects.contains(selected) && envobjects[selected] && envobjects[selected]->GetParentObject()) {
                envobjects[selected]->GetParentObject()->GetProperties()->removeChild(envobjects[selected]->GetProperties());
            }

            envobjects.remove(selected);
            did_delete = true;
        }
    }

    return did_delete;
}

bool Room::SaveJSON(const QString & filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Room::SaveJSON(): File " << filename << " can't be saved";
        return false;
    }

    QTextStream ofs(&file);

    ofs.setRealNumberNotation(QTextStream::FixedNotation);
    ofs << QJsonDocument::fromVariant(GetJSONCode(false)).toJson();

    file.close();

    qDebug() << "Room::SaveJSON() - File" << filename << "saved.";
    return true;
}

QVariantMap Room::GetJSONCode(const bool show_defaults) const
{
    QVariantMap root;

    QVariantMap fireboxroom;
    QVariantMap assetsmap;
    QVariantList assetobjectlist;
    QVariantList assetimagelist;
    QVariantList assetghostlist;
    QVariantList assetrecordinglist;
    QVariantList assetshaderlist;
    QVariantList assetscriptlist;
    QVariantList assetsoundlist;
    QVariantList assetvideolist;    
    QVariantList assetwebsurfacelist;

    QVariantMap room;
    QMap <QString, QVariantList> elementlistmap;

    for (const QPointer <AssetObject> & a : assetobjects) {
        if (a && !a->GetB("_primitive") && a->GetB("_save_to_markup")) {
            assetobjectlist.push_back(a->GetJSONCode());
        }
    }

    for (const QPointer <AssetImage> & a : assetimages) {
        if (a && a->GetB("_save_to_markup")) {
            assetimagelist.push_back(a->GetJSONCode());
        }
    }

    for (const QPointer <AssetGhost> & a : assetghosts) {
        if (a && a->GetB("_save_to_markup")) {
            assetghostlist.push_back(a->GetJSONCode());
        }
    }

    for (const QPointer <AssetRecording> & a : assetrecordings) {
        if (a && a->GetB("_save_to_markup")) {
            assetrecordinglist.push_back(a->GetJSONCode());
        }
    }

    for (const QPointer <AssetShader> & a : assetshaders) {
        if (a && a->GetB("_save_to_markup")) {
            assetshaderlist.push_back(a->GetJSONCode());
        }
    }

    for (const QPointer <AssetScript> & a : assetscripts) {
        if (a && a->GetB("_save_to_markup")) {
            assetscriptlist.push_back(a->GetJSONCode());
        }
    }

    for (const QPointer <AssetSound> & a : assetsounds) {
        if (a && a->GetB("_save_to_markup")) {
            assetsoundlist.push_back(a->GetJSONCode());
        }
    }

    for (const QPointer <AssetVideo> & a : assetvideos) {
        if (a && a->GetB("_save_to_markup")) {
            assetvideolist.push_back(a->GetJSONCode());
        }
    }   

    for (const QPointer <AbstractWebSurface> & a : assetwebsurfaces) {
        if (a && a->GetB("_save_to_markup")) {
            assetwebsurfacelist.push_back(a->GetJSONCode());
        }
    }

    if (!assetobjectlist.empty()) {
        assetsmap.insert("assetobject", assetobjectlist);
    }
    if (!assetimagelist.empty()) {
        assetsmap.insert("assetimage", assetimagelist);
    }
    if (!assetghostlist.empty()) {
        assetsmap.insert("assetghost", assetghostlist);
    }
    if (!assetrecordinglist.empty()) {
        assetsmap.insert("assetrecording", assetrecordinglist);
    }
    if (!assetshaderlist.empty()) {
        assetsmap.insert("assetshader", assetshaderlist);
    }
    if (!assetscriptlist.empty()) {
        assetsmap.insert("assetscript", assetscriptlist);
    }
    if (!assetsoundlist.empty()) {
        assetsmap.insert("assetsound", assetsoundlist);
    }
    if (!assetvideolist.empty()) {
        assetsmap.insert("assetvideo", assetvideolist);
    }    
    if (!assetwebsurfacelist.empty()) {
        assetsmap.insert("assetwebsurface", assetwebsurfacelist);
    }

    //just do all of them and don't worry about defaults
    QList <QByteArray> b = props->dynamicPropertyNames();
    for (int i=0; i<b.size(); ++i) {
        if (props->GetSaveAttribute(b[i].data(), false)) {
            room[b[i]] = props->property(b[i]);
        }
    }

    if (entrance_object) {
        room["pos"] = MathUtil::GetVectorAsString(entrance_object->GetPos(), false);
        room["xdir"] = MathUtil::GetVectorAsString(entrance_object->GetXDir(), false);
        room["ydir"] = MathUtil::GetVectorAsString(entrance_object->GetYDir(), false);
        room["zdir"] = MathUtil::GetVectorAsString(entrance_object->GetZDir(), false);
    }
    if (cubemap) {
        if (cubemap->GetAssetImages().size() == 6) {
            QVector <QPointer <AssetImage> > & skybox_imgs = cubemap->GetAssetImages();
            //faces 0 right 1 left 2 up 3 down 4 front 5 back
            if (skybox_imgs[0]) {
                room["skybox_right_id"] = skybox_imgs[0]->GetS("id");
            }
            if (skybox_imgs[1]) {
                room["skybox_left_id"] = skybox_imgs[1]->GetS("id");
            }
            if (skybox_imgs[2]) {
                room["skybox_up_id"] = skybox_imgs[2]->GetS("id");
            }
            if (skybox_imgs[3]) {
                room["skybox_down_id"] = skybox_imgs[3]->GetS("id");
            }
            if (skybox_imgs[4]) {
                room["skybox_front_id"] = skybox_imgs[4]->GetS("id");
            }
            if (skybox_imgs[5]) {
                room["skybox_back_id"] = skybox_imgs[5]->GetS("id");
            }
        }
        else if (cubemap->GetAssetImages().size() == 1) {
            if (cubemap->GetAssetImages().first()) {
                room["cubemap_id"] = cubemap->GetAssetImages().first()->GetS("id");
            }
        }
    }
    if (cubemap_radiance &&
            !cubemap_radiance->GetAssetImages().empty() && cubemap_radiance->GetAssetImages().first()) {
        room["cubemap_radiance_id"] = cubemap_radiance->GetAssetImages().first()->GetS("id");
    }
    if (cubemap_irradiance &&
            !cubemap_irradiance->GetAssetImages().empty() &&
            cubemap_irradiance->GetAssetImages().first()) {
        room["cubemap_irradiance_id"] = cubemap_irradiance->GetAssetImages().first()->GetS("id");
    }

    //write the environment objects out, easy
    for (const QPointer <RoomObject> & obj : envobjects) {
        if (obj && (obj->GetType() != "link" || (obj->GetType() == "link" && obj != GetEntranceObject() && obj->GetSaveToMarkup()))) {
            elementlistmap[obj->GetType()].push_back(obj->GetJSONCode(show_defaults));
        }
    }

    //add all qvariantlists to room
    QMap <QString, QVariantList>::const_iterator ele_cit;
    for (ele_cit=elementlistmap.begin(); ele_cit!=elementlistmap.end(); ++ele_cit) {
        room.insert(ele_cit.key(), ele_cit.value());
    }

    //assemble fireboxroom
    fireboxroom.insert("assets", assetsmap);
    fireboxroom.insert("room", room);

    root.insert("FireBoxRoom", fireboxroom);

    return root;
}

void Room::AddNewAssetScript()
{    
    const QString save_filename = GetSaveFilename();
    QFileInfo d(save_filename);
    if (d.exists()) {
//        qDebug() << "Room::AddNewAssetScript()" << d.absoluteDir().path() + "/" + script_filename;
        QString script_filename;
        QString script_path;

        unsigned int num_script = assetscripts.size();

        do {
            script_filename = QString("script") + QString::number(num_script) + QString(".js");
            script_path = d.absoluteDir().path() + QString("/") + script_filename;
            ++num_script;
        } while (QFileInfo(script_path).exists());

        const QString js_code = "room.onLoad = function() {\n\n}\n\nroom.update = function(dt) {\n\n}\n\n";
        const QUrl script_url = QUrl::fromLocalFile(script_path);

        QFile file(script_path);
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream ofs(&file);
            ofs << js_code;
            file.close();
        }
        else {
            MathUtil::ErrorLog("Room::AddNewAssetScript() - Error writing: " + script_path);
        }

        //add new assetscript to room's JML data structure
        QPointer <AssetScript> s = new AssetScript(this);
        s->SetJSCode(js_code);
        s->SetSrc(script_url.toString(), script_filename);
        s->SetLoaded(true);
        AddAssetScript(s);
        s->Update();
    }
}

void Room::SaveXML(QTextStream & ofs)
{
    ofs.setRealNumberNotation(QTextStream::FixedNotation);
    //ofs.setRealNumberPrecision(3); //Note: this causes problems for pos values e.g. > 1000

    ofs << "<!-- Written with Janus VR.  URL: " << GetS("url") << " -->\n";
    ofs << "<html>\n";
    ofs << "<head>\n";
    ofs << "<title>" << GetS("title") << "</title>\n";
    ofs << "<meta charset=\"utf-8\">\n";
    ofs << "</head>\n";
    ofs << "<body>\n";
    ofs << "<!--\n";
    ofs << "<FireBoxRoom>\n";
    ofs << "<assets>\n";

    //iterate over all asset types
    QList <QPointer <Asset> > assets = GetAllAssets();
    for (QPointer <Asset> & a : assets) {
        if (a && !a->GetB("_primitive") && a->GetB("_save_to_markup")) {
            ofs << a->GetXMLCode() << "\n";
        }
    }

    ofs << "</assets>\n";
    ofs << "<room";

    //write out all room attributes
    QVariantMap rd;

    QList <QByteArray> b = props->dynamicPropertyNames();
    for (int i=0; i<b.size(); ++i) {
        if (props->GetSaveAttribute(b[i].data(), false)) {
            rd[b[i]] = props->property(b[i]);
        }
    }

    if (entrance_object) {
//        qDebug() << "Room::SaveFireBoxRoom() - saving" << entrance_object << entrance_object->GetPos();
        if (entrance_object->GetPos() != QVector3D(0,0,0)) {
            rd["pos"] = MathUtil::GetVectorAsString(entrance_object->GetPos(), false);
        }
        if (entrance_object->GetXDir() != QVector3D(1,0,0)) {
            rd["xdir"] = MathUtil::GetVectorAsString(entrance_object->GetXDir(), false);
        }
        if (entrance_object->GetYDir() != QVector3D(0,1,0)) {
            rd["ydir"] = MathUtil::GetVectorAsString(entrance_object->GetYDir(), false);
        }
        if (entrance_object->GetZDir() != QVector3D(0,0,1)) {
            rd["zdir"] = MathUtil::GetVectorAsString(entrance_object->GetZDir(), false);
        }
    }

    if (cubemap) {
        if (cubemap->GetAssetImages().size() == 6) {
            QVector <QPointer <AssetImage> > & skybox_imgs = cubemap->GetAssetImages();
            //faces 0 right 1 left 2 up 3 down 4 front 5 back
            if (skybox_imgs[0]) {
                rd["skybox_right_id"] = skybox_imgs[0]->GetS("id");
            }
            if (skybox_imgs[1]) {
                rd["skybox_left_id"] = skybox_imgs[1]->GetS("id");
            }
            if (skybox_imgs[2]) {
                rd["skybox_up_id"] = skybox_imgs[2]->GetS("id");
            }
            if (skybox_imgs[3]) {
                rd["skybox_down_id"] = skybox_imgs[3]->GetS("id");
            }
            if (skybox_imgs[4]) {
                rd["skybox_front_id"] = skybox_imgs[4]->GetS("id");
            }
            if (skybox_imgs[5]) {
                rd["skybox_back_id"] = skybox_imgs[5]->GetS("id");
            }
        }
        else if (cubemap->GetAssetImages().size() == 1) {
            if (cubemap->GetAssetImages().first()) {
                rd["cubemap_id"] = cubemap->GetAssetImages().first()->GetS("id");
            }
        }
    }    
    if (cubemap_radiance &&
            !cubemap_radiance->GetAssetImages().empty() &&
            cubemap_radiance->GetAssetImages().first()) {
        rd["cubemap_radiance_id"] =  cubemap_radiance->GetAssetImages().first()->GetS("id");
    }
    if (cubemap_irradiance &&
            !cubemap_irradiance->GetAssetImages().empty() &&
            cubemap_irradiance->GetAssetImages().first()) {
        rd["cubemap_irradiance_id"] = cubemap_irradiance->GetAssetImages().first()->GetS("id");
    }

    QVariantMap::const_iterator it;
    for (it=rd.begin(); it!=rd.end(); ++it) {
        ofs << " " << it.key() << "=\"" << it.value().toString() << "\"";
    }
    ofs << ">\n";

    //write the environment objects out, easy
    for (QPointer <RoomObject> & obj : envobjects) {
        if (obj && obj->GetSaveToMarkup() &&
                (obj->GetType() != "link" ||
                 (obj->GetType() == "link" && obj != GetEntranceObject() && obj->GetSaveToMarkup()))) {
            ofs << obj->GetXMLCode(false) << "\n";
        }
    }

    ofs << "</room>\n";
    ofs << "</FireBoxRoom>\n";
    ofs << "-->\n";
    ofs << "<script src=\"https://web.janusvr.com/janusweb.js\"></script>\n";
    ofs << "<script>elation.janusweb.init({url: document.location.href})</script>\n";
    ofs << "</body>\n";
    ofs << "</html>";
    ofs.flush();
}

bool Room::SaveXML(const QString & filename)
{
    //open file for writing
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Room::SaveXML(): File " << filename << " can't be saved";
        return false;
    }

    //save out the data
    QTextStream ofs(&file);

    SaveXML(ofs);

    //close file, report saving ok
    file.close();

    qDebug() << "Room::SaveXML() - File" << filename << "saved.";
    return true;
}

bool Room::RunKeyPressEvent(QKeyEvent * e, QPointer <Player> player)
{
    bool defaultPrevented = false;

    const QVector3D d = player->GetV("dir");

    for (QPointer <AssetScript> & a : assetscripts) {
        if (a) {
            bool eachDefaultPrevented = false;
            QList<QPointer <RoomObject> > objectsAdded = a->OnKeyEvent("onKeyDown", e, envobjects, player, &eachDefaultPrevented);
            AddRoomObjects(objectsAdded);

            if (eachDefaultPrevented) {
                defaultPrevented = true;
            }
        }
    }

    //player dir may have updated
    if (player->GetV("dir") != d) {
        player->UpdateDir();
    }

    return defaultPrevented;
}

bool Room::RunKeyReleaseEvent(QKeyEvent * e, QPointer <Player> player)
{
    const QVector3D d = player->GetV("dir");
    bool defaultPrevented = false;

    for (QPointer <AssetScript> & a : assetscripts) {
        if (a) {
            bool eachDefaultPrevented = false;
            QList<QPointer <RoomObject> > objectsAdded = a->OnKeyEvent("onKeyUp", e, envobjects, player, &eachDefaultPrevented);
            AddRoomObjects(objectsAdded);

            if (eachDefaultPrevented) {
                defaultPrevented = true;
            }
        }
    }

    //player dir may have updated
    if (player->GetV("dir") != d) {
        player->UpdateDir();
    }

    return defaultPrevented;
}

unsigned int Room::GetRoomNumTris() const
{
    unsigned int num_tris = 0;

    const QString s = GetS("use_local_asset");
    if (room_templates.contains(s) && room_templates[s] && room_templates[s]->GetEnvObject()) {
        QPointer <AssetObject> obj = room_templates[s]->GetEnvObject()->GetCollisionAssetObject();
        if (obj) {
            num_tris += obj->GetNumTris();
        }
    }   

    for (const QPointer <RoomObject> & o : envobjects) {
        if (o) {
            num_tris += o->GetNumTris();
        }
    }

    return num_tris;
}

void Room::SetProperties(QPointer <DOMNode> props)
{
    this->props = props;
    props->setProperty("_type", "room");
    props->setProperty("js_id", "__room");
}

QPointer <DOMNode> Room::GetProperties()
{
    return props;
}

void Room::DoDelete(const QString & s)
{
    RoomObject delete_object;
    delete_object.ReadXMLCode(s);
    DeleteSelected(delete_object.GetS("js_id"));
}

void Room::DoEdit(const QString & s)
{   
//    qDebug() << "Room::DoEdit" << s;
    HTMLPage page;
    page.ReadXMLContent("<FireBoxRoom>" + s + "</FireBoxRoom>");

    QVariantMap map = page.GetData()["FireBoxRoom"].toMap();
    QPointer <Room> temp_room = new Room(); //59.9 - Note!  We need a temp_room because create_default_helper calls AddRoomObject, which modifies js_ids!
    QPointer <RoomObject> root_object(new RoomObject());
    root_object->SetType("room");
    root_object->SetS("js_id", "__room");

    Create_Default_Assets_Helper(map);
    temp_room->Create_Default_Helper(map, root_object);

//    qDebug() << "Room::DoEdit rootobject" << root_object;
    for (int j=0; j<root_object->GetChildObjects().size(); ++j) {

        QPointer <RoomObject> obj = root_object->GetChildObjects()[j];
//        qDebug() << "Room::DoEdit" << j << obj << obj->GetS("js_id") << obj->GetS("_type");
        if (obj->GetS("js_id").length() <= 0) { //41.5 fix - some objs to be sync'ed may not have an id
            continue;
        }

        obj->SetInterpolate(true);
        obj->SetParentObject(QPointer <RoomObject> ());
        obj->SetB("sync", false);

        QPointer <RoomObject> o2 = GetRoomObject(obj->GetS("js_id"));
        if (o2) {            
            //object already exists
            o2->SetInterpolation();
            o2->SetInterpolate(true);
            //release 60.0 copy properties only and not assetobject pointers, or this will flicker
            o2->GetProperties()->Copy(obj->GetProperties());

            //delete obj - we copied it's edit info, it will no longer be parented, we can delete it
            delete obj;
//            qDebug() << "object existed" << o2->GetS("js_id") << o2->GetV("pos") << o2->GetPos();
        }
        else {
            //create an object copy, as the original will be deleted by the owned room
            o2 = new RoomObject();
            o2->Copy(obj);            
            o2->SetInterpolate(true);

            //make new objects scale in
            o2->SetV("scale", QVector3D(0,0,0));
            o2->SetInterpolation();
            o2->SetV("scale", obj->GetV("scale"));

            AddRoomObject(o2);
            o2->PlayCreateObject(); //play sound for new object
//            qDebug() << "adding room object" << obj << obj->GetProperties(); //envobjects[obj->GetS("js_id")]->GetXMLCode();
        }
    }

    delete root_object;
    delete temp_room;
}

float Room::GetProgress() const
{
    return GetF("_progress");
}

bool Room::GetReady() const
{
    return GetB("_ready");
}

bool Room::GetReadyForScreenshot() const
{
    return GetB("_ready_for_screenshot");
}

void Room::SetLoaded(const bool b)
{    
    page->GetWebAsset().SetLoaded(b);
}

bool Room::GetLoaded() const
{    
    return page->GetWebAsset().GetLoaded();
}

void Room::SetStarted(const bool b)
{
    page->GetWebAsset().SetStarted(b);
}

bool Room::GetStarted() const
{
    return page->GetWebAsset().GetStarted();
}

void Room::SetProcessing(bool b)
{
    page->GetWebAsset().SetProcessing(b);
}

bool Room::GetProcessing() const
{
    return page->GetWebAsset().GetProcessing();
}

void Room::SetProcessed(bool b)
{
    page->GetWebAsset().SetProcessed(b);
}

bool Room::GetProcessed() const
{
    return page->GetWebAsset().GetProcessed();
}

QPointer <HTMLPage> Room::GetPage() const
{
    return page;
}

void Room::StartURLRequest()
{    
    page->Clear();
    page->SetURL(GetS("url"));
    page->Request(GetS("url"));
//    qDebug() << "Room::StartURLRequest()" << props->GetURL() << page->GetWebAsset().GetStarted();
}

void Room::ImportCode(const QString code, const QString src_url)
{
    page->SetCode(code);

    if (page->FoundFireBoxContent() && !page->FoundError()) {
        //temporarily assign room's URL so relative paths resolve
        const QString original_url = GetS("url");
        SetS("url", src_url);

        //construct the room with this parsed XML code
        const QVariantMap fireboxroom = page->GetData()["FireBoxRoom"].toMap();
        Create_Default(fireboxroom);

        //restore original URL
        SetS("url", original_url);
    }
}

void Room::UpdateCode(const QString code)
{
    page->Clear();
    page->SetURL(GetS("url"));
    page->SetCode(code);
}

//player_lastxform stores the player's last transform while in this room (used for putting the player back when teleporting to/from this Room)
void Room::SetPlayerLastTransform(const QMatrix4x4 & m)
{
    player_lastxform = m;
}

QMatrix4x4 Room::GetPlayerLastTransform() const
{
    return player_lastxform;
}

void Room::SetEntranceObject(QPointer <RoomObject> o)
{    
    entrance_object = o;
}

QPointer <RoomObject> Room::GetEntranceObject() const
{
    return entrance_object;
}

void Room::SetParentObject(QPointer <RoomObject> o)
{
    parent_object = o;
}

QPointer <RoomObject> Room::GetParentObject() const
{
    return parent_object;
}

QPointer <RoomTemplate> Room::GetRoomTemplate() const
{    
    const QString s = GetS("use_local_asset");
//    qDebug() << "Room::GetRoomTemplate()" << s;
    if (room_templates.contains(s)) {
        return room_templates[s];
    }
    return QPointer<RoomTemplate>();
}

//"Default" handles FireBoxRooms as well as standard HTML pages
void Room::Create_Default_Assets_Helper(const QVariantMap & assets)
{ 
    QVariantMap::const_iterator it;
    for (it=assets.begin(); it!=assets.end(); ++it) {
//        qDebug() << "Room::Create_Default_Assets_Helper processing key" << it.key();
        const QString asset_type = it.key(); //e.g. "AssetObject"
        const QVariantList vl = assets[asset_type].toList();
        for (int j=0; j<vl.size(); ++j) {            
            const QVariantMap d = vl[j].toMap();
            AddAsset(asset_type, d, false);
        }
    }
}

void Room::Create_Default(const QVariantMap fireboxroom)
{
//    qDebug() << page->GetData()["FireBoxRoom"].toMap()["Assets"].toMap()["AssetObject"].toList();
//    QVariantMap fireboxroom = page->GetData()["FireBoxRoom"].toMap();

    QVariantMap assets = fireboxroom["assets"].toMap();
    QVariantMap room;
    if (fireboxroom.contains("room")) {
        room = fireboxroom["room"].toMap();
    }    

    Create_Default_Assets_Helper(assets);

//    qDebug() << "room" << fireboxroom["room"].toMap();
    SetProperties(room);

    QPointer <RoomObject> root_object(new RoomObject());
    root_object->SetType("room");
    root_object->SetS("js_id", "__room");
    Create_Default_Helper(room, root_object);

    //link the room to its shader asset (if one is set)
    SetAssetShader(GetAssetShader(GetS("shader_id")));

    root_object->GetProperties()->GetChildren().clear();
    root_object->GetChildObjects().clear();
    delete root_object;
}

void Room::Create()
{
//    qDebug() << "Room::Create()" << this->GetURL();
    //update portal with page title and set this portal as the room's parent
    const QString url = GetS("url");

    SetS("title", page->GetTitle());

    envobjects.clear();

    if (QString::compare(url, "workspaces") == 0) {
        SetS("url", "workspaces");
    }
    else if (QString::compare(url, "bookmarks") == 0) {
        SetS("url", "bookmarks");
    }

    QVariantMap d_room = page->GetRoomData();

    //determine if we should use a custom translator (check if translator dir contains filename that matches domain)
    QString use_translator_name;
    QDir d(QDir::currentPath()+"/assets/translators/");
    d.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
    QStringList l = d.entryList();
    for (int i=0; i<l.size(); ++i) {
        const QString domain_name = l[i].left(l[i].length()-3); //strip .js filename extension
        const QString first_part = url.left(domain_name.length() + 12); //12 accounts for leading chars up to: "https://www."
        if (first_part.contains(domain_name)) {
            use_translator_name = l[i];
            break;
        }
    }

    //update the data for the room
    if (page->FoundError()) {        
        entrance_object->SetV("pos", QVector3D(0.8f, -0.2f, 0.0f));
    }
    else if (page->FoundFireBoxContent()) {           
        SetProperties(d_room);        
        entrance_object->SetProperties(d_room); //sets initial position/dir for entrance
    }
    else if (!use_translator_name.isEmpty()) {
        Create_Custom_Translator(use_translator_name);
    }
    else if (page->FoundSingleImageContent()) {
        SetS("use_local_asset", "room_plane");
        SetB("visible", false); //room template isn't visible

        QPointer <AssetImage> new_asset_image(new AssetImage());
        new_asset_image->SetSrc(url, url);
        new_asset_image->SetS("id", "image");
        AddAssetImage(new_asset_image);

        QPointer <RoomObject> new_img = RoomObject::CreateImage("", "image", QColor(255,255,255), false);
        new_img->SetV("pos", QVector3D(0,0,10));
        new_img->SetDir(QVector3D(0,0,-1));
        new_img->SetV("scale", QVector3D(10,10,10));
        AddRoomObject(new_img);

        entrance_object->SetV("pos", QVector3D(0,0,0));
        entrance_object->SetDir(QVector3D(0,0,1));
    }
    else if (page->FoundGeometryContent()) {
        SetS("use_local_asset", "room_plane");
        SetB("visible", false); //room template isn't visible

        //QPointer <AssetObject> new_asset_obj(new AssetObject(new_room->GetURL(), new_room->GetURL()));
        QString url_fixed = QUrl::fromPercentEncoding(url.toUtf8());
        QPointer <AssetObject> new_asset_obj(new AssetObject());
        new_asset_obj->SetSrc(url_fixed, url_fixed);
        new_asset_obj->SetS("id", "geometry");
        if (url.right(4).toLower() == ".obj") {
            const QString obj_mtl = url.left(url.length()-4) + ".mtl";
            new_asset_obj->SetMTLFile(obj_mtl);
        }
        else if (url.right(7).toLower() == ".obj.gz") {
            const QString obj_mtl = url.left(url.length()-7) + ".mtl";
            new_asset_obj->SetMTLFile(obj_mtl);
        }
        AddAssetObject(new_asset_obj);

        QPointer <RoomObject> new_obj(new RoomObject);
        new_obj->SetType("object");
        new_obj->SetS("id", "geometry");
        new_obj->SetV("pos", QVector3D(0,0,10));
        new_obj->SetDir(QVector3D(0,0,-1));
        new_obj->SetB("loop", true);
        AddRoomObject(new_obj);

        entrance_object->SetV("pos", QVector3D(0,0,0));
        entrance_object->SetDir(QVector3D(0,0,1));
    }
    else if (page->FoundVideoContent()) {
        SetS("use_local_asset", "room_plane");
        SetB("visible", false); //room template isn't visible

        QPointer <AssetVideo> new_asset_vid = new AssetVideo();
        new_asset_vid->SetSrc(url, url);
        new_asset_vid->SetS("id", "video");
        new_asset_vid->SetB("auto_play", true);
        AddAssetVideo(new_asset_vid);

        QPointer <RoomObject> new_vid(new RoomObject());
        new_vid->SetType("video");
        new_vid->SetS("id", "video");
        new_vid->SetV("pos", QVector3D(0,1,10));
        new_vid->SetDir(QVector3D(0,0,-1));
        new_vid->SetV("scale", QVector3D(10,10,10));
        new_vid->SetB("lighting", false);
        AddRoomObject(new_vid);

        entrance_object->SetV("pos", QVector3D(0,0,0));
        entrance_object->SetDir(QVector3D(0,0,1));
    }    
    else if (page->FoundRedditCommentContent()) {
        SetS("use_local_asset", "room2");
        SetB("visible", true);

        QVector3D pos, dir;
        if (GetMountPoint(pos, dir)) {
            entrance_object->SetV("pos", pos);
            entrance_object->SetDir(dir);
        }        
    }
    else if (page->FoundRedditContent()) {
        SetS("use_local_asset", "room_box_large");
        SetB("visible", false);

        QVector3D pos, dir;
        if (GetMountPoint(pos, dir)) {
            entrance_object->SetV("pos", QVector3D(20.0f, 0.0f, 2.0f));
            entrance_object->SetDir(dir);
        }
        else {
            entrance_object->SetV("pos", QVector3D(20.0f, 0.0f, 2.0f));
            entrance_object->SetDir(QVector3D(-1,0,0));
        }       
    }
    else if (page->FoundImgurContent() || page->FoundFlickrContent()) {
        SetS("use_local_asset", "room_plane");
        SetB("visible", true);
        SetC("col", QColor(100,100,100));

        entrance_object->SetV("pos", QVector3D(0,0,0));
        entrance_object->SetDir(QVector3D(0,0,1));
    }
    else if (page->FoundDirectoryListing()) {
        SetS("use_local_asset", "room_plane");
        SetB("visible", true);

        QColor c;
        c.setHsl(url.length(), 128, 192);
        SetC("col", c);

        entrance_object->SetV("pos", QVector3D(0,0,0)); //default pos/dir for websurface room
        entrance_object->SetDir(QVector3D(0,0,1));
    }
    else {        
        entrance_object->SetV("pos", QVector3D(8,0,-3)); //default pos/dir for websurface room
        entrance_object->SetDir(QVector3D(-1,0,0));
    }

    if (page->FoundError()) {
        Create_Error(page->GetErrorCode());
    }
    else if (!use_translator_name.isEmpty()) {

    }
    else if (page->FoundFireBoxContent()) {
        const QVariantMap fireboxroom = page->GetData()["FireBoxRoom"].toMap();
        Create_Default(fireboxroom);
    }
    else if (page->FoundDirectoryListing()) {
        Create_DirectoryListing();
    }
    else if (page->FoundRedditCommentContent()) {
        Create_RedditComments();
    }    
    else if (page->FoundRedditContent()) {
        Create_Reddit();
    }
    else if (page->FoundImgurContent()) {
        Create_Imgur();
    }
    else if (page->FoundVimeoContent()) {
        Create_Vimeo();
    }
    else if (page->FoundYoutubeContent()) {        
        Create_Youtube();
    }
    else if (page->FoundFlickrContent()) {
        Create_Flickr();
    }
    else if (page->FoundSingleImageContent()) {

    }
    else if (page->FoundGeometryContent()) {

    }
    else if (page->FoundVideoContent()) {

    }
    else {
        Create_WebSurface();
    }      
}

void Room::Create_Flickr()
{
    const QString translator_path = MathUtil::GetTranslatorPath();
    const QList <ImgurData> & things = page->ImgurThings();

    QPointer <AssetObject> new_asset_obj(new AssetObject());
    new_asset_obj->SetSrc(translator_path, "flickr/statue.obj");
    new_asset_obj->SetTextureFile("flickr/statue.png", 0);
    new_asset_obj->SetS("id", "statue");
    AddAssetObject(new_asset_obj);

    //C = 2 pi r,
    //r = C / 2pi
    const float circ_radius = qMax(2.0f, ((things.size() + 1)* 4.0f) / (2.0f * MathUtil::_2_PI));
    const QVector3D circ_centre(0,0,circ_radius);

    QPointer <RoomObject> new_obj = RoomObject::CreateObject("", "statue", QColor(255,255,255), false);
    new_obj->SetV("pos", circ_centre + QVector3D(0,2,0));
    new_obj->SetDir(QVector3D(0, 0, -1));
    new_obj->SetV("scale", QVector3D(5, 5, 5));
    new_obj->SetV("rotate_axis", QVector3D(0,1,0));
    new_obj->SetF("rotate_deg_per_sec", -3.0f);
    AddRoomObject(new_obj);

    for (int i=0; i<things.size(); ++i) {

        const float theta = float(i+1)/float(things.size()+1) * MathUtil::_2_PI;
        const QVector3D img_dir = QVector3D(sinf(theta),0,-cosf(theta));
        const QVector3D img_pos = circ_centre + QVector3D(0,1.5f,0) + img_dir * circ_radius;

        QString img_id = QString("image") + QString::number(i);

        QPointer <AssetImage> new_asset_image(new AssetImage());
        new_asset_image->SetSrc(GetS("url"), things[i].img_url);
        new_asset_image->SetS("id", img_id);
        AddAssetImage(new_asset_image);

        QPointer <RoomObject> new_img = RoomObject::CreateImage("", img_id, QColor(255,255,255), false);
        new_img->SetV("pos", img_pos);
        new_img->SetDir(-img_dir);
        AddRoomObject(new_img);
    }
}

void Room::Create_Youtube()
{
    const QString translator_path = MathUtil::GetTranslatorPath();

    QString replace_url = GetS("url");
    replace_url.replace("/watch?v=", "/embed/");
    if (!replace_url.contains("?autoplay=true")) {
        replace_url = replace_url + "?autoplay=true";
    }

    QPointer <AssetImage> assetrad(new AssetImage());
    assetrad->SetS("id", "Annotated_skybox_radiance");
    assetrad->SetSrc(translator_path, "youtube/YoutubeRadience.dds");
    assetrad->SetB("tex_clamp", false);
    assetrad->SetB("tex_linear", true);
    AddAssetImage(assetrad);

    QPointer <AssetImage> assetirrad(new AssetImage());
    assetirrad->SetS("id", "Annotated_skybox_Irradiance");
    assetirrad->SetSrc(translator_path, "youtube/YoutubeIrRadience.dds");
    assetirrad->SetB("tex_clamp", false);
    assetirrad->SetB("tex_linear", true);
    AddAssetImage(assetirrad);

    QVector <QString> r0;
    QVector <QString> r1;
    r0.push_back(assetrad->GetS("id"));
    r1.push_back(assetirrad->GetS("id"));
    SetCubemap(r0, CUBEMAP_TYPE::RADIANCE);
    SetCubemap(r1, CUBEMAP_TYPE::IRRADIANCE);

    QPointer <AssetObject> new_asset_obj(new AssetObject());
    new_asset_obj->SetSrc(translator_path, "youtube/youtube.dae.gz");
    new_asset_obj->SetS("id", "main");
    AddAssetObject(new_asset_obj);

    new_asset_obj = new AssetObject();
    new_asset_obj->SetSrc(translator_path, "youtube/youtubecol.obj");
    new_asset_obj->SetS("id", "maincol");
    AddAssetObject(new_asset_obj);

    new_asset_obj = new AssetObject();
    new_asset_obj->SetSrc(translator_path, "youtube/youtubeNolight.dae.gz");
    new_asset_obj->SetS("id", "mainL");
    AddAssetObject(new_asset_obj);

    new_asset_obj = new AssetObject();
    new_asset_obj->SetSrc(translator_path, "youtube/youtubemainscreen.obj");
    new_asset_obj->SetS("id", "screen");
    AddAssetObject(new_asset_obj);

    QPointer <AbstractWebSurface> new_ws = (AbstractWebSurface*)new AssetWebSurface();
    new_ws->SetS("id", "webscreen");
    new_ws->SetSrc(replace_url, replace_url);
    new_ws->SetWidth(1280);
    new_ws->SetHeight(700);
    AddAssetWebSurface(new_ws);   

    QPointer <RoomObject> o(new RoomObject);    
    o->SetV("pos", QVector3D(0,0.2f,0.0f)); //56.0- fix falling through floor
    o->SetDir(QVector3D(0,0,1));
    SetEntranceObject(o);

    QPointer <RoomObject> new_obj = RoomObject::CreateObject("", "main", QColor(255,255,255), true);
    new_obj->SetV("pos", QVector3D(0,0,0));
    new_obj->SetXDirs(QVector3D(0.707107f, 0, -0.707106f), QVector3D(0,1,0), QVector3D(0.707106f, 0, 0.707107f));
    new_obj->SetV("scale", QVector3D(1, 1.1f, 1));
    new_obj->SetS("collision_id", "maincol");
    AddRoomObject(new_obj);

    new_obj = RoomObject::CreateObject("", "mainL", QColor(255,255,255), false);
    new_obj->SetV("pos", QVector3D(0,0,0));
    new_obj->SetXDirs(QVector3D(0.707107f, 0, -0.707106f), QVector3D(0,1,0), QVector3D(0.707106f, 0, 0.707107f));
    new_obj->SetV("scale", QVector3D(1, 1.1f, 1));
    AddRoomObject(new_obj);

    new_obj = RoomObject::CreateObject("", "screen", QColor(255,255,255), false);
    new_obj->SetV("pos", QVector3D(0,0,0));
    new_obj->SetXDirs(QVector3D(0.707107f, 0, -0.707106f), QVector3D(0,1,0), QVector3D(0.707106f, 0, 0.707107f));
    new_obj->SetV("scale", QVector3D(1, 1.1f, 1));
    new_obj->SetS("websurface_id", "webscreen");
    AddRoomObject(new_obj);

    SetAllObjects("locked", true);
}

void Room::Create_Imgur()
{
    const QString translator_path = MathUtil::GetTranslatorPath();
    const QList <ImgurData> & things = page->ImgurThings();

    QPointer <AssetObject> new_asset_obj = new AssetObject();
    new_asset_obj->SetSrc(translator_path, "imgur/statue.obj");
    new_asset_obj->SetTextureFile("imgur/statue.png", 0);
    new_asset_obj->SetS("id", "statue");
    AddAssetObject(new_asset_obj);

    //C = 2 pi r,
    //r = C / 2pi
    const float circ_radius = qMax(2.0f, ((things.size() + 1)* 4.0f) / (2.0f * MathUtil::_2_PI));
    const QVector3D circ_centre(0,0,circ_radius);

    QPointer <RoomObject> new_obj = RoomObject::CreateObject("", "statue", QColor(255,255,255), false);
    new_obj->SetV("pos", circ_centre);
    new_obj->SetDir(QVector3D(0, 0, -1));
    new_obj->SetV("scale", QVector3D(5, 5, 5));
    new_obj->SetV("rotate_axis", QVector3D(0,1,0));
    new_obj->SetF("rotate_deg_per_sec", -3.0f);
    AddRoomObject(new_obj);

    for (int i=0; i<things.size(); ++i) {
        const float theta = float(i+1)/float(things.size()+1) * MathUtil::_2_PI;
        const QVector3D img_dir = QVector3D(sinf(theta),0,-cosf(theta));
        const QVector3D img_pos = circ_centre + QVector3D(0,1.5f,0) + img_dir * circ_radius;

        QString img_id = QString("image") + QString::number(i);

        QPointer <AssetImage> new_asset_image(new AssetImage());
        new_asset_image->SetSrc(GetS("url"), things[i].img_url);
        new_asset_image->SetS("id", img_id);
        AddAssetImage(new_asset_image);

        QPointer <RoomObject> new_img = RoomObject::CreateImage("", img_id, QColor(255,255,255), false);
        new_img->SetV("pos", img_pos);
        new_img->SetDir(-img_dir);
        AddRoomObject(new_img);
    }

}

void Room::Create_RedditComments()
{
    const QString translator_path = MathUtil::GetTranslatorPath();
    const QList <RedditData> & things = page->RedditThings();    

    QPointer <AssetImage> down_image(new AssetImage());
    down_image->SetSrc(translator_path, "reddit/down.png");
    down_image->SetS("id", "img_down_arrow");
    AddAssetImage(down_image);

    QPointer <AssetImage> up_image(new AssetImage());
    up_image->SetSrc(translator_path, "reddit/up.png");
    up_image->SetS("id", "img_up_arrow");
    AddAssetImage(up_image);

    for (int i=0; i<things.size(); ++i) {
        QVector3D pos, dir;
        if (!GetMountPoint(pos, dir)) {
            return;
        }

        QPointer <RoomObject> new_paragraph = RoomObject::CreateParagraph("", "", things[i].comment_str, QColor(255,255,255), false);
        new_paragraph->SetV("pos", pos + QVector3D(0.0f, 1.75f, 0.0f));
        new_paragraph->SetDir(dir);
        new_paragraph->SetV("scale", QVector3D(0.8f, 0.8f, 0.8f));
        AddRoomObject(new_paragraph);

        if (i == 0) {

            QPointer <RoomObject> new_img1 = RoomObject::CreateImage("", "img_down_arrow", QColor(255,255,255), false);
            new_img1->SetV("pos", pos + QVector3D(0.0f, 3.0f, 0.0f));
            new_img1->SetDir(dir);
            new_img1->SetV("scale", QVector3D(0.15f, 0.15f, 0.15f));
            AddRoomObject(new_img1);

            QPointer <RoomObject> new_img2 = RoomObject::CreateImage("", "img_up_arrow", QColor(255,255,255), false);
            new_img2->SetV("pos", pos + QVector3D(0.0f, 4.25f, 0.0f));
            new_img2->SetDir(dir);
            new_img2->SetV("scale", QVector3D(0.15f, 0.15f, 0.15f));
            AddRoomObject(new_img2);

            QPointer <RoomObject> new_text1(new RoomObject());
            new_text1->SetType("text");
            new_text1->SetText("submitted");
            new_text1->SetV("pos", pos + QVector3D(0.0f, 0.6f, 0.0f));
            new_text1->SetDir(dir);
            new_text1->SetV("scale", QVector3D(1.0f, 1.0f, 1.0f));
            new_text1->SetC("col", QColor(50, 50, 50));
            AddRoomObject(new_text1);

            QPointer <RoomObject> new_text2(new RoomObject());
            new_text2->SetType("text");
            new_text2->SetText(things[i].time_str + QString(" ago by"));
            new_text2->SetV("pos", pos + QVector3D(0.0f, 0.4f, 0.0f));
            new_text2->SetDir(dir);
            new_text2->SetV("scale", QVector3D(1.7f, 1.7f, 1.7f));
            new_text2->SetC("col", QColor(50, 50, 50));
            AddRoomObject(new_text2);

            QPointer <RoomObject> new_text3(new RoomObject());
            new_text3->SetType("text");
            new_text3->SetText(things[i].user_str);
            new_text3->SetV("pos", pos + QVector3D(0.0f, 0.2f, 0.0f));
            new_text3->SetDir(dir);
            new_text3->SetV("scale", QVector3D(1.0f, 1.0f, 1.0f));
            new_text3->SetC("col", QColor(100, 100, 200));
            AddRoomObject(new_text3);

            QPointer <RoomObject> new_text4(new RoomObject());
            new_text4->SetType("text");
            new_text4->SetText(things[i].rank_str);
            new_text4->SetV("pos", pos + QVector3D(0.0f, 3.5f, 0.0f));
            new_text4->SetDir(dir);
            new_text4->SetV("scale", QVector3D(1.0f, 1.0f, 1.0f));
            new_text4->SetC("col", QColor(255, 139, 96));
            new_text4->SetFixedSize(true, 0.25f);
            AddRoomObject(new_text4);
        }
        else {
            QPointer <RoomObject> new_text1(new RoomObject());
            new_text1->SetType("text");
            new_text1->SetText(things[i].user_str);
            new_text1->SetV("pos", pos + QVector3D(0.0f, 3.5f, 0.0f));
            new_text1->SetDir(dir);
            new_text1->SetV("scale", QVector3D(1.0f, 1.0f, 1.0f));
            new_text1->SetC("col", QColor(100, 100, 200));
            AddRoomObject(new_text1);

            QPointer <RoomObject> new_text2(new RoomObject());
            new_text2->SetType("text");
            new_text2->SetText(things[i].score_str);
            new_text2->SetV("pos", pos + QVector3D(0.0f, 3.25f, 0.0f));
            new_text2->SetDir(dir);
            new_text2->SetV("scale", QVector3D(1.0f, 1.0f, 1.0f));
            new_text2->SetC("col", QColor(50, 50, 50));
            AddRoomObject(new_text2);

            QPointer <RoomObject> new_text3(new RoomObject());
            new_text3->SetType("text");
            new_text3->SetText(things[i].time_str + QString(" ago"));
            new_text3->SetV("pos", pos + QVector3D(0.0f, 3.0f, 0.0f));
            new_text3->SetDir(dir);
            new_text3->SetV("scale", QVector3D(1.7f, 1.7f, 1.7f));
            new_text3->SetC("col", QColor(50, 50, 50));
            AddRoomObject(new_text3);
        }
    }
}

void Room::Create_Default_Workspace()
{
    SetS("use_local_asset", "room_plane");
    SetStarted(true);
    SetLoaded(true);
    SetProcessing(true);
    SetProcessed(true);
}

void Room::Create_Vimeo()
{
    //TODO: perform conversion
//    [4:13 PM] (Channel) Fire_Fox: https://vimeo.com/108650530
//    [4:13 PM] (Channel) Fire_Fox: https://player->vimeo.com/video/108650530
    const QString translator_path = MathUtil::GetTranslatorPath();

    //check if URL contains numeric suffix, if so, use player
    QString replace_url = GetS("url");
    const QString suffix = replace_url.right(replace_url.length() - replace_url.lastIndexOf("/") -1);

    bool num_found;
    suffix.toInt(&num_found);

    if (num_found) {
        replace_url = "https://player->vimeo.com/video/" + suffix;
    }

    QPointer <AssetObject> assetobj0(new AssetObject());
    assetobj0->SetSrc(translator_path, "vimeo/spincube.dae.gz");
    assetobj0->SetS("id", "spin");
    AddAssetObject(assetobj0);

    QPointer <AssetObject> assetobj1(new AssetObject());
    assetobj1->SetSrc(translator_path, "vimeo/Flynn.dae.gz");
    assetobj1->SetS("id", "Main");
    AddAssetObject(assetobj1);

    QPointer <AssetObject> assetobj2(new AssetObject());
    assetobj2->SetSrc(translator_path, "vimeo/FlynnL.dae.gz");
    assetobj2->SetS("id", "MainL");
    AddAssetObject(assetobj2);

    QPointer <AssetObject> assetobj3(new AssetObject());
    assetobj3->SetSrc(translator_path, "vimeo/screen.obj");
    assetobj3->SetS("id", "screen");
    AddAssetObject(assetobj3);

    QPointer <AssetObject> assetobj4(new AssetObject());
    assetobj4->SetSrc(translator_path, "vimeo/VimeoCol.obj");
    assetobj4->SetS("id", "Vcol");
    AddAssetObject(assetobj4);

    QPointer <AssetImage> assetimg = new AssetImage();
    assetimg->SetSrc(translator_path, "vimeo/vimeo_radiance256.dds");
    assetimg->SetS("id", "Annotated_skybox_radiance");
    AddAssetImage(assetimg);

    assetimg = new AssetImage();
    assetimg->SetSrc(translator_path, "vimeo/vimeo_irradiance64.dds");
    assetimg->SetS("id", "Annotated_skybox_Irradiance");
    AddAssetImage(assetimg);

    assetimg = new AssetImage();
    assetimg->SetSrc(translator_path, "vimeo/down.png");
    assetimg->SetB("tex_clamp", true);
    assetimg->SetS("id", "sky_down");
    AddAssetImage(assetimg);

    assetimg = new AssetImage();
    assetimg->SetSrc(translator_path, "vimeo/left.png");
    assetimg->SetB("tex_clamp", true);
    assetimg->SetS("id", "sky_left");
    AddAssetImage(assetimg);

    assetimg = new AssetImage();
    assetimg->SetSrc(translator_path, "vimeo/up.png");
    assetimg->SetB("tex_clamp", true);
    assetimg->SetS("id", "sky_up");
    AddAssetImage(assetimg);

    assetimg = new AssetImage();
    assetimg->SetSrc(translator_path, "vimeo/right.png");
    assetimg->SetB("tex_clamp", true);
    assetimg->SetS("id", "sky_right");
    AddAssetImage(assetimg);

    assetimg = new AssetImage();
    assetimg->SetSrc(translator_path, "vimeo/front.png");
    assetimg->SetB("tex_clamp", true);
    assetimg->SetS("id", "sky_front");
    AddAssetImage(assetimg);

    assetimg = new AssetImage();
    assetimg->SetSrc(translator_path, "vimeo/back.png");
    assetimg->SetB("tex_clamp", true);
    assetimg->SetS("id", "sky_back");
    AddAssetImage(assetimg);

    QPointer <AbstractWebSurface> new_asset_websurface = (AbstractWebSurface*)new AssetWebSurface();
    new_asset_websurface->SetSrc(replace_url, replace_url);
    new_asset_websurface->SetS("id", "web1");
    new_asset_websurface->SetWidth(1300);
    new_asset_websurface->SetHeight(768);
    AddAssetWebSurface(new_asset_websurface);

    QVector <QString> r0;
    QVector <QString> r1;
    QVector <QString> r2;
    r0.push_back("Annotated_skybox_radiance");
    r1.push_back("Annotated_skybox_Irradiance");
    r2.push_back("sky_right");
    r2.push_back("sky_left");
    r2.push_back("sky_up");
    r2.push_back("sky_down");
    r2.push_back("sky_front");
    r2.push_back("sky_back");
    SetCubemap(r0, CUBEMAP_TYPE::RADIANCE);
    SetCubemap(r1, CUBEMAP_TYPE::IRRADIANCE);
    SetCubemap(r2, CUBEMAP_TYPE::DEFAULT);

    SetB("visible", false);

    QPointer <RoomObject> obj = new RoomObject();
    obj->SetV("pos", QVector3D(11,0,0));
    obj->SetDir(QVector3D(-1,0,0));
    SetEntranceObject(obj);

    obj = RoomObject::CreateObject("", "Main", QColor(255,255,255), true);
    obj->SetS("collision_id", "Vcol");
    obj->SetS("cull_face", "none");
    AddRoomObject(obj);

    obj = RoomObject::CreateObject("", "MainL", QColor(255,255,255), false);
    obj->SetS("cull_face", "none");
    AddRoomObject(obj);

    obj = RoomObject::CreateObject("", "spin", QColor(255,255,255), true);
    obj->SetS("cull_face", "none");
    obj->SetV("pos", QVector3D(-12,2,8));
    obj->SetF("rotate_deg_per_sec", -10.0f);
    AddRoomObject(obj);

    obj = RoomObject::CreateObject("", "spin", QColor(255,255,255), true);
    obj->SetS("cull_face", "none");
    obj->SetV("pos", QVector3D(-12,2,-8));
    obj->SetF("rotate_deg_per_sec", 10.0f);
    AddRoomObject(obj);

    obj = RoomObject::CreateObject("", "screen", QColor(255,255,255), false);
    obj->SetS("cull_face", "none");
    obj->SetS("websurface_id", "web1");
    AddRoomObject(obj);

}

void Room::Create_Reddit()
{    
    const QString translator_path = MathUtil::GetTranslatorPath();

    QPointer <AbstractWebSurface> new_asset_websurface = (AbstractWebSurface*)new AssetWebSurface();
    new_asset_websurface->SetSrc(GetS("url"), GetS("url"));
    new_asset_websurface->SetS("id", "web1");
    new_asset_websurface->SetWidth(1280);
    new_asset_websurface->SetHeight(768);
    AddAssetWebSurface(new_asset_websurface);

    QPointer <AssetObject> assetobj0(new AssetObject());
    assetobj0->SetSrc(translator_path, "reddit/redditPBR.dae.gz");
    assetobj0->SetS("id", "class");
    AddAssetObject(assetobj0);

    QPointer <AssetObject> assetobj1(new AssetObject());
    assetobj1->SetSrc(translator_path, "reddit/orbitsphere.dae.gz");
    assetobj1->SetS("id", "orbitsphere");
    assetobj1->SetTextureMipmap(false);
    AddAssetObject(assetobj1);

    QPointer <AssetObject> assetobj5(new AssetObject());
    assetobj5->SetSrc(translator_path, "reddit/screensingle.obj");
    assetobj5->SetS("id", "screen1");
    AddAssetObject(assetobj5);

    QPointer <AssetObject> assetobj6(new AssetObject());
    assetobj6->SetSrc(translator_path, "reddit/redditPBR.obj");
    assetobj6->SetS("id", "col");
    AddAssetObject(assetobj6);

    QPointer <AssetImage> assetirrad0(new AssetImage());
    assetirrad0->SetSrc(translator_path, "reddit/RedditIrRadience.dds");
    assetirrad0->SetS("id", "Annotated_skybox_Irradiance");
    AddAssetImage(assetirrad0);

    QPointer <AssetImage> assetrad0(new AssetImage());
    assetrad0->SetSrc(translator_path, "reddit/RedditRadience.dds");
    assetrad0->SetS("id", "Annotated_skybox_radiance");
    AddAssetImage(assetrad0);

    QPointer <RoomObject> obj0 = RoomObject::CreateObject("", "class", QColor(255,255,255), true);
    obj0->SetS("collision_id", "col");
    obj0->SetDir(QVector3D(-1,0,0));
    obj0->SetF("collision_radius", 2.0f);
    AddRoomObject(obj0);

    QPointer <RoomObject> obj1 = RoomObject::CreateObject("p0", "orbitsphere", QColor(255,255,255), true);
    obj1->SetV("scale", QVector3D(0.5f,0.5f,0.5f));
    AddRoomObject(obj1);

    QPointer <RoomObject> obj2 = RoomObject::CreateObject("p1", "orbitsphere", QColor(255,255,255), true);
    obj2->SetV("scale", QVector3D(0.5f,0.5f,0.5f));
    AddRoomObject(obj2);

    QPointer <RoomObject> obj3 = RoomObject::CreateObject("p2", "sphere", QColor(255,255,255), false);
    obj3->SetB("visible", false);
    AddRoomObject(obj3);

    QPointer <RoomObject> obj5 = RoomObject::CreateObject("", "screen1", QColor(255,255,255), false);
    obj5->SetS("websurface_id", "web1");
    obj5->SetV("pos", QVector3D(-12.5f, 0.3f, -12.5f));
    obj5->SetDir(QVector3D(-1.0f, 0.0f, 0.0f));
    AddRoomObject(obj5);

    QPointer <RoomObject> obj6 = RoomObject::CreateObject("", "screen1", QColor(255,255,255), false);
    obj6->SetS("websurface_id", "web1");
    obj6->SetV("pos", QVector3D(-12.5f, 0.3f, 12.5f));
    obj6->SetDir(QVector3D(0.0f, 0.0f, 1.0f));
    AddRoomObject(obj6);

    QPointer <RoomObject> obj7 = RoomObject::CreateObject("", "screen1", QColor(255,255,255), false);
    obj7->SetS("websurface_id", "web1");
    obj7->SetV("pos", QVector3D(12.5f, 0.3f, -12.5f));
    obj7->SetDir(QVector3D(0.0f, 0.0f, -1.0f));
    AddRoomObject(obj7);

    QPointer <RoomObject> obj8 = RoomObject::CreateObject("", "screen1", QColor(255,255,255), false);
    obj8->SetS("websurface_id", "web1");
    obj8->SetV("pos", QVector3D(12.5f, 0.3f, 12.5f));
    obj8->SetDir(QVector3D(1.0f, 0.0f, 0.0f));
    AddRoomObject(obj8);

    QPointer <RoomObject> l0 = new RoomObject();
    l0->SetType("light");
    l0->SetS("js_id", "Light1");
    l0->SetV("pos", QVector3D(0,16,0));
    l0->SetF("light_intensity", 20.0f);
    l0->SetF("light_cone_angle", 0.0f);
    l0->SetF("light_cone_exponent", 10.0f);
    l0->SetF("light_range", 100.0f);
    l0->SetC("col", QColor("#FF9900"));
    AddRoomObject(l0);

    QPointer <RoomObject> l1 = new RoomObject();
    l1->SetType("light");
    l1->SetS("js_id", "Light2");
    l1->SetV("pos", QVector3D(0,16,0));
    l1->SetF("light_intensity", 20.0f);
    l1->SetF("light_cone_angle", 0.0f);
    l1->SetF("light_cone_exponent", 10.0f);
    l1->SetF("light_range", 100.0f);
    l1->SetC("col", QColor("#FF9900"));
    AddRoomObject(l1);

    SetB("visible", false);

    QVector <QString> rad0;
    QVector <QString> irrad0;

    rad0.push_back("Annotated_skybox_radiance");
    irrad0.push_back("Annotated_skybox_Irradiance");

    SetCubemap(rad0, CUBEMAP_TYPE::RADIANCE);
    SetCubemap(irrad0, CUBEMAP_TYPE::IRRADIANCE);

    const QList <RedditData> & things = page->RedditThings();

    for (int i=0; i<things.size(); ++i) {

        QVector3D pos, dir;
        if (!GetMountPoint(pos, dir)) {
            return;
        }

        QPointer <RoomObject> new_text1(new RoomObject());
        new_text1->SetType("text");
        new_text1->SetText(things[i].rank_str);
        new_text1->SetV("pos", pos + QVector3D(0.0f, 3.5f, 0.0f));
        new_text1->SetDir(dir);
        new_text1->SetC("col", QColor(50,50,50));
        AddRoomObject(new_text1);

        QPointer <RoomObject> new_text2(new RoomObject());
        new_text2->SetType("text");
        new_text2->SetText(things[i].comment_str);
        new_text2->SetV("pos", pos + QVector3D(0.0f, 1.0f, 0.0f));
        new_text2->SetDir(dir);
        new_text2->SetC("col", QColor(50,50,50));
        new_text2->SetV("scale", QVector3D(1.7f, 1.7f, 1.7f));
        AddRoomObject(new_text2);

        QPointer <RoomObject> new_text4(new RoomObject());
        new_text4->SetType("text");
        new_text4->SetText(things[i].time_str + QString(" by"));
        new_text4->SetV("pos", pos + QVector3D(0.0f, 2.5f, 0.0f));
        new_text4->SetDir(dir);
        new_text4->SetC("col", QColor(50,50,50));
        new_text4->SetV("scale", QVector3D(1.7f, 1.7f, 1.7f));
        AddRoomObject(new_text4);

        QPointer <RoomObject> new_text5(new RoomObject());
        new_text5->SetType("text");
        new_text5->SetText(things[i].user_str);
        new_text5->SetV("pos", pos + QVector3D(0.0f, 2.25f, 0.0f));
        new_text5->SetDir(dir);
        new_text5->SetV("scale", QVector3D(1.0f, 1.0f, 1.0f));
        new_text5->SetC("col", QColor(100,100,200));
        AddRoomObject(new_text5);

        QString img_id = QString("image") + QString::number(i);

        QPointer <AssetImage> new_asset_image(new AssetImage());
        new_asset_image->SetSrc(GetS("url"), things[i].img_url);
        new_asset_image->SetS("id", img_id);
        AddAssetImage(new_asset_image);

        if (!GetMountPoint(pos, dir)) {
            return;
        }

        QPointer <RoomObject> new_text6(new RoomObject());
        new_text6->SetType("text");
        new_text6->SetText(things[i].score_str);
        new_text6->SetV("pos", pos + QVector3D(0.0f, 3.25f, 0.0f));
        new_text6->SetDir(dir);
        new_text6->SetV("scale", QVector3D(1.0f, 1.0f, 1.0f));
        new_text6->SetC("col", QColor(255,139,96));
        new_text6->SetFixedSize(true, 0.25f);
        AddRoomObject(new_text6);

        QPointer <RoomObject> new_portal = new RoomObject();
        new_portal->SetType("link");
        new_portal->SetTitle(things[i].text_str);
        new_portal->SetURL(GetS("url"), things[i].link_url);
        new_portal->SetV("pos", pos);
        new_portal->SetDir(dir);
        new_portal->SetC("col", GetC("col"));
        new_portal->SetS("thumb_id", img_id);
        AddRoomObject(new_portal);
    }

    //finally add some navigation stuff
    const QList <QString> & nav_links = page->NavLinks();

    if (!nav_links.empty()) {

        QVector3D pos, dir;
        for (int i=0; i<10; ++i) {
            if (!GetMountPoint(pos, dir)) {
                return;
            }
        }

        QPointer <RoomObject> new_text(new RoomObject());
        new_text->SetType("text");
        new_text->SetText("Next >");
        new_text->SetV("pos", pos + QVector3D(0.0f, 3.0f, 0.0f));
        new_text->SetDir(dir);
        new_text->SetC("col", QColor(100,100,200));
        new_text->SetV("scale", QVector3D(2.0f, 2.0f, 2.0f));
        AddRoomObject(new_text);

        QPointer <RoomObject> new_portal = new RoomObject();
        new_portal->SetType("link");
        new_portal->SetTitle("Next>");
        new_portal->SetURL(GetS("url"), nav_links.last());
        new_portal->SetV("pos", pos);
        new_portal->SetDir(dir);
        new_portal->SetC("col", GetC("col"));
        AddRoomObject(new_portal);
    }

    QPointer <AssetScript> assetscript0(new AssetScript(this));
    assetscript0->SetSrc(translator_path, "reddit/3BodyOrbits.js");
    assetscript0->Load();
    AddAssetScript(assetscript0);

    SetAllObjects("locked", true);
}

void Room::Create_WebSurface()
{
    const QString translator_path = MathUtil::GetTranslatorPath();
    //49.89 - new PBR websurface room by FireFoxG
    SetS("use_local_asset", "room_plane");

    const QString url = GetS("url");

    QPointer <AbstractWebSurface> new_asset_websurface = (AbstractWebSurface*)new AssetWebSurface();
    new_asset_websurface->SetSrc(url, url);
    new_asset_websurface->SetS("id", "web1");
    new_asset_websurface->SetWidth(1300);
    new_asset_websurface->SetHeight(768);
    AddAssetWebSurface(new_asset_websurface);

    QPointer <AssetImage> assetrad(new AssetImage());
    assetrad->SetS("id", "Annotated_skybox_radiance");
    assetrad->SetSrc(translator_path, "web/paralleogramEqui_radiance256.dds");
    assetrad->SetB("tex_clamp", false);
    assetrad->SetB("tex_linear", true);
    AddAssetImage(assetrad);

    QPointer <AssetImage> assetirrad(new AssetImage());
    assetirrad->SetS("id", "Annotated_skybox_Irradiance");
    assetirrad->SetSrc(translator_path, "web/paralleogramEqui_irradiance64.dds");
    assetirrad->SetB("tex_clamp", false);
    assetirrad->SetB("tex_linear", true);
    AddAssetImage(assetirrad);

    QVector <QString> r0;
    QVector <QString> r1;
    r0.push_back(assetrad->GetS("id"));
    r1.push_back(assetirrad->GetS("id"));
    SetCubemap(r0, CUBEMAP_TYPE::RADIANCE);
    SetCubemap(r1, CUBEMAP_TYPE::IRRADIANCE);

    QPointer <AssetObject> assetobj0(new AssetObject());
    assetobj0->SetSrc(translator_path, "web/bamboo.dae.gz");
    assetobj0->SetS("id", "class");
    AddAssetObject(assetobj0);

    QPointer <AssetObject> assetobjcol(new AssetObject());
    assetobjcol->SetSrc(translator_path, "web/Collisionmesh.obj");
    assetobjcol->SetS("id", "collision");
    AddAssetObject(assetobjcol);

    QPointer <AssetObject> assetobj3(new AssetObject());
    assetobj3->SetSrc(translator_path, "web/ParallelogramFinal.dae.gz");
    assetobj3->SetS("id", "class3");
    AddAssetObject(assetobj3);

    QPointer <AssetObject> assetobj3s(new AssetObject());
    assetobj3s->SetSrc(translator_path, "web/ParallelogramFinalNolight.dae.gz");
    assetobj3s->SetS("id", "class3s");
    AddAssetObject(assetobj3s);

    QPointer <AssetObject> assetobj2(new AssetObject());
    assetobj2->SetSrc(translator_path, "web/screen3.obj");
    assetobj2->SetS("id", "screen2");
    AddAssetObject(assetobj2);

    QPointer <RoomObject> obj0 = RoomObject::CreateObject("", "class3", QColor(255,255,255), true);
    obj0->SetS("collision_id", "collision");
    obj0->SetV("pos", QVector3D(0.0f, 0.0f, 0.0f));
    obj0->SetDir(QVector3D(0.0f, 0.0f, 1.0f));
    obj0->SetV("scale", QVector3D(1.1f, 1.1f, 1.1f));
    obj0->SetB("locked", true);
    AddRoomObject(obj0);

    QPointer <RoomObject> obj1 = RoomObject::CreateObject("", "class3s", QColor(255,255,255), false);
    obj1->SetV("pos", QVector3D(0.0f, 0.0f, 0.0f));
    obj1->SetDir(QVector3D(0.0f, 0.0f, 1.0f));
    obj1->SetV("scale", QVector3D(1.1f, 1.1f, 1.1f));
    obj1->SetB("locked", true);
    AddRoomObject(obj1);

    QPointer <RoomObject> obj5 = RoomObject::CreateObject("", "screen2", QColor(255,255,255), false);
    obj5->SetS("websurface_id", "web1");
    obj5->SetV("pos", QVector3D(0.0f, 0.0f, 0.0f));
    obj5->SetDir(QVector3D(0.0f, 0.0f, 1.0f));
    obj5->SetV("scale", QVector3D(1.09f,1.09f,1.09f));
    obj5->SetB("lighting", false);
    obj5->SetB("locked", true);
    AddRoomObject(obj5);

    QPointer <RoomObject> obj6 = RoomObject::CreateObject("", "class", QColor(255,255,255), true);
    obj6->SetV("pos", QVector3D(-3.62f, 0.0f, 0.0f));
    obj6->SetDir(QVector3D(0.0f, 0.0f, 1.0f));
    obj6->SetV("scale", QVector3D(1.1f, 1.1f, 1.1f));
    obj6->SetS("cull_face", "none");
    obj6->SetB("locked", true);
    AddRoomObject(obj6);

    QPointer <RoomObject> obj7 = RoomObject::CreateObject("", "class", QColor(255,255,255), true);
    obj7->SetV("pos", QVector3D(-1.7f, 0.0f, 0.0f));
    obj7->SetDir(QVector3D(0.0f, 0.0f, 1.0f));
    obj7->SetV("scale", QVector3D(1.1f, 1.1f, 1.1f));
    obj7->SetS("cull_face", "none");
    obj7->SetB("locked", true);
    AddRoomObject(obj7);

    QPointer <RoomObject> obj8 = RoomObject::CreateObject("", "class", QColor(255,255,255), true);
    obj8->SetV("pos", QVector3D(-5.521f, 0.0f, 0.0f));
    obj8->SetDir(QVector3D(0.0f, 0.0f, 1.0f));
    obj8->SetV("scale", QVector3D(1.1f ,1.1f, 1.1f));
    obj8->SetS("cull_face", "none");
    obj8->SetB("locked", true);
    AddRoomObject(obj8);

    QPointer <RoomObject> obj9 = RoomObject::CreateObject("", "class", QColor(255,255,255), true);
    obj9->SetV("pos", QVector3D(0,0,0));
    obj9->SetDir(QVector3D(0.0f, 0.0f, 1.0f));
    obj9->SetV("scale", QVector3D(1.1f ,1.1f, 1.1f));
    obj9->SetS("cull_face", "none");
    obj9->SetB("locked", true);
    AddRoomObject(obj9);

    SetB("visible", false);
}

void Room::Create_Default_Helper(const QVariantMap & d, QPointer <RoomObject> p)
{
    QVariantMap::const_iterator it;
    for (it=d.begin(); it!=d.end(); ++it) {
        const QString t = it.key();
//            qDebug() << "NAME" << name;
        if (!t.isEmpty()) {
            QVariantList l = d[t].toList();
            for (int i=0; i<l.size(); ++i) {
                QVariantMap d = l[i].toMap();

                QPointer <RoomObject> o = new RoomObject();
                o->SetType(t);
                o->SetProperties(d);
                if (d.contains("js_id")) {
                    o->SetS("js_id", d["js_id"].toString());
                }

                //60.0 - resolve links relative to this room's path
                if (t == "link") {
                    o->SetURL(this->GetS("url"), o->GetS("url"));
                }

                //                qDebug() << "Room::Create_Default_Helper" << new_thing->GetID();
                if (t == "link" && !page->FoundFireBoxContent()) {
                    QVector3D pos, dir;
                    if (GetMountPoint(pos, dir)) {
                        o->SetV("pos", pos);
                        o->SetDir(dir);
                    }
                }

                AddRoomObject(o);
                //qDebug() << "Parentobject" << (&parent_object) << "now has children:" << parent_object.GetChildObjects().size() << parent_object.GetID() << new_thing.GetID();

                o->SetParentObject(p);
                if (p) {
                    if (p->GetType() == "room") {
                        o->SetParentObject(NULL);
                    }
                    p->GetChildObjects().push_back(o);
                }

                Create_Default_Helper(d, o);
            }
        }
    }
}

void Room::Create_Error(const int code)
{
    const QString translator_path = MathUtil::GetTranslatorPath();
    const QString code_str = QString::number(code);

    QPointer <AssetImage> new_asset_image(new AssetImage());
    new_asset_image->SetS("id", "static");
    new_asset_image->SetB("tex_linear", false);
    new_asset_image->SetSrc(translator_path, "errors/static.gif");
    AddAssetImage(new_asset_image);

    QPointer <AssetObject> obj(new AssetObject());
    obj->SetSrc(translator_path, "errors/error.obj");
    obj->SetS("id", "stand");
    obj->SetTextureFile("errors/lightmap.png", 0);
    AddAssetObject(obj);

    QPointer <RoomObject> stand(new RoomObject());
    stand->SetType("object");
    stand->SetS("id", "stand");
    stand->SetB("locked", true);
    stand->SetV("pos", QVector3D(1,-0.1f, 4));
    stand->SetXDirs(QVector3D(-1,0,0), QVector3D(0,1,0), QVector3D(0,0,-1));
    stand->SetB("lighting", false);
    stand->SetS("collision_id", "stand");
    AddRoomObject(stand);

    QPointer <RoomObject> sphere(new RoomObject());
    sphere->SetType("object");
    sphere->SetS("id", "sphere");
    sphere->SetB("locked", true);
    sphere->SetV("pos", QVector3D(4, 0, -1));
    sphere->SetXDirs(QVector3D(0,0,1), QVector3D(0,-1,0), QVector3D(1,0,0));
    sphere->SetV("scale", QVector3D(400,400,400));
    sphere->SetS("image_id", "static");
    sphere->SetS("cull_face", "front");
    sphere->SetB("lighting", false);
    AddRoomObject(sphere);

    QPointer <RoomObject> text(new RoomObject());
    text->SetType("text");
    text->SetV("pos", QVector3D(1.3f, 1.4f, 12.5f));
    text->SetXDirs(QVector3D(-1,0,0), QVector3D(0,1,0), QVector3D(0,0,-1));
    text->SetV("scale", QVector3D(6,6,1));
    AddRoomObject(text);

    switch (code) {
    case 400:
        sphere->SetC("col", QColor(25,0,25));
        stand->SetC("col", QColor(255,0,255));
        text->SetC("col", QColor(255,0,255));
        text->SetText("400 - Bad Request");
        break;

    case 401:
        sphere->SetC("col", QColor(0,25,0));
        stand->SetC("col", QColor(0,255,0));
        text->SetC("col", QColor(0,255,0));
        text->SetText("401 - Unauthorized");
        break;

    case 402:
        sphere->SetC("col", QColor(25,25,0));
        stand->SetC("col", QColor(255,255,0));
        text->SetC("col", QColor(255,255,0));
        text->SetText("402 - Payment Required");
        break;

    case 403:
        sphere->SetC("col", QColor(0,0,25));
        stand->SetC("col", QColor(0,0,255));
        text->SetC("col", QColor(0,0,255));
        text->SetText("403 - Forbidden");
        break;

    case 404:
        sphere->SetC("col", QColor(25,0,0));
        stand->SetC("col", QColor(255,0,0));
        text->SetC("col", QColor(255,0,0));
        text->SetText("404 - Are you lost?");
        break;

    case 408:
        sphere->SetC("col", QColor(0,25,25));
        stand->SetC("col", QColor(0,255,255));
        text->SetC("col", QColor(0,255,255));
        text->SetText("408 - Timeout");
        break;

    case 500:
        sphere->SetC("col", QColor(25, 12, 0));
        stand->SetC("col", QColor(255,128,0));
        text->SetC("col", QColor(255,128,0));
        text->SetText("500 - Internal Server Error");
        break;

    default:
        sphere->SetC("col", QColor(25,25,25));
        text->SetText("Server not found");
        break;
    }

}

void Room::Create_DirectoryListing()
{
//    qDebug() << "Room::Create_DirectoryListing()" << GetURL() << QUrl::fromLocalFile(GetURL()).toString();
    QString local_path = QUrl(GetS("url")).toLocalFile();
    QDir d(local_path);
    //const QFileInfoList l = d.entryInfoList(0, QDir::Type | QDir::DirsFirst);
    d.setFilter(QDir::AllEntries | QDir::NoDot);
    d.setSorting(QDir::Type | QDir::DirsFirst | QDir::IgnoreCase);
    QFileInfoList l = d.entryInfoList();

    for (int i=0; i<l.size(); ++i) {
        if (l[i].absoluteFilePath().endsWith("~")) {
            l.removeAt(i);
            --i;
        }
    }

    const float circ_radius = qMax(2.0f, ((l.size() + 1)* 4.0f) / (2.0f * MathUtil::_2_PI));
    const QVector3D circ_centre(0,0,circ_radius);

    QPointer <RoomObject> loc_text = RoomObject::CreateText("loc_text", 0.3f, GetS("url"), QColor(255,255,255), QVector3D(1,1,1));
    loc_text->SetV("pos", circ_centre + QVector3D(0,6,0));
    loc_text->SetDir(QVector3D(0,0,-1));
    AddRoomObject(loc_text);

    QString extension;
    for (int i=0; i<l.size(); ++i) {

        QUrl f = QUrl::fromLocalFile(l[i].absoluteFilePath());
        QUrl p = QUrl::fromLocalFile(l[i].absolutePath());

        const float theta = float(i+1)/float(l.size()+1) * MathUtil::_2_PI;
        const QVector3D zdir = QVector3D(sinf(theta),0,-cosf(theta));
        const QVector3D pos = circ_centre + zdir * circ_radius;

        if (MathUtil::img_extensions.contains(l[i].fileName().right(3))) {
            QString img_id = QString("image") + QString::number(i);

            QPointer <AssetImage> new_asset_image(new AssetImage());
            new_asset_image->SetSrc(p.toString(), f.toString());
            new_asset_image->SetS("id", img_id);
            AddAssetImage(new_asset_image);

            QPointer <RoomObject> new_img = RoomObject::CreateImage("", img_id, QColor(255,255,255), false);
            new_img->SetV("pos", pos + QVector3D(0,1.25f,0));
            new_img->SetDir(-zdir);
            AddRoomObject(new_img);
        }
        else {
            QColor c;
            c.setHsl(GetRoomObjects().size() * 30, 128, 128);

            QPointer <RoomObject> new_portal = new RoomObject();
            new_portal->SetType("link");
            new_portal->SetURL(p.toString(), f.toString());
            new_portal->SetV("pos", pos);
            new_portal->SetDir(-zdir);
            new_portal->SetC("col", c);
            AddRoomObject(new_portal);
        }

        //add extension labels
        const QString suffix = l[i].suffix();
        if (suffix.length() >= 2 && suffix.length() <=6 && QString::compare(suffix, extension) != 0) {
            extension = l[i].suffix();
            QPointer <RoomObject> new_text = RoomObject::CreateText(extension + QString::number(i), 0.15f, extension, QColor(255,255,255), QVector3D(1,1,1));
            new_text->SetV("pos", pos + QVector3D(0,3,0));
            new_text->SetDir(-zdir);
            AddRoomObject(new_text);
        }
        else if (i == 0 && l[i].isDir()) {
            QPointer <RoomObject> new_text = RoomObject::CreateText(extension + QString::number(i), 0.15f, "DIR", QColor(192,255,192), QVector3D(1,1,1));
            new_text->SetV("pos", pos + QVector3D(0,3,0));
            new_text->SetDir(-zdir);
            AddRoomObject(new_text);
        }
    }
}

void Room::LoadTemplates()
{
    if (room_templates.isEmpty()) {
        QFile file(MathUtil::GetApplicationPath() + "assets/rooms.txt");
        if (file.open( QIODevice::ReadOnly | QIODevice::Text )) {
            //set base path (for loading resources in same dir)
            QTextStream ifs(&file);

            while (!ifs.atEnd()) {
                const QString room_name = ifs.readLine().trimmed();

                if (room_name.length() > 4) {                    
                    room_templates[room_name] = new RoomTemplate();
                    room_templates[room_name]->Load(room_name);
                }
            }
            file.close();
        }
    }
}

void Room::LoadSkyboxes()
{
    if (SettingsManager::GetDemoModeEnabled() && !SettingsManager::GetDemoModeBuiltinSkyboxes()) {
        return;
    }

    if (skyboxes.isEmpty()) {
        QList <QString> skybox_image_urls;
        QList <QString> skybox_cubemap_image_urls;

        skybox_image_urls.push_back("assets/skybox/Day/Dayright.jpg");
        skybox_image_urls.push_back("assets/skybox/Day/Dayleft.jpg");
        skybox_image_urls.push_back("assets/skybox/Day/Daytop.jpg");
        skybox_image_urls.push_back("assets/skybox/Day/Daybottom.jpg");
        skybox_image_urls.push_back("assets/skybox/Day/Dayfront.jpg");
        skybox_image_urls.push_back("assets/skybox/Day/Dayback.jpg");
        skybox_cubemap_image_urls.push_back("assets/skybox/Day/Day_Radiance.dds");
        skybox_cubemap_image_urls.push_back("assets/skybox/Day/Day_Irradiance.dds");

        const QString base = MathUtil::GetApplicationURL();
        QList <QPointer <AssetImage> > skybox_images;
        for (int i = 0; i < skybox_image_urls.size(); ++i) {
            QPointer <AssetImage> a = new AssetImage();
            a->SetB("tex_clamp", true);
            a->SetB("tex_mipmap", true);
            a->SetSrc(base, skybox_image_urls[i]);
            a->Load();
            skybox_images.push_back(a);
        }

        QList <QPointer <AssetImage> > skybox_cubemaps;
        for (int i = 0; i < skybox_cubemap_image_urls.size(); ++i) {
            QPointer <AssetImage> a = new AssetImage();
            a->SetB("tex_clamp", true);
            a->SetB("tex_mipmap", true);
            a->SetSrc(base, skybox_cubemap_image_urls[i]);
            bool const is_rad = ((i % 2) == 0);
            if (is_rad) {
                a->SetS("id", "__CUBEMAP_RADIANCE");
            }
            else {
                a->SetS("id", "__CUBEMAP_IRRADIANCE");
            }
            a->Load();
            skybox_cubemaps.push_back(a);
        }

        int cubemap_index = 0;
        const int cubemap_face_count = 6;
        for (int i = 0; i < skybox_image_urls.size(); i += cubemap_face_count) {
            QVector <QPointer <AssetImage> > images;
            images.resize(cubemap_face_count);
            for (int j = 0; j < cubemap_face_count; ++j)
            {
                images[j] = skybox_images[i+j];
            }

            QPointer <AssetSkybox> s = new AssetSkybox();
            s->SetAssetImages(images);
            skyboxes.push_back(s);

            QVector<QPointer <AssetImage> > radiance_images;
            radiance_images.push_back(skybox_cubemaps[cubemap_index++]);
            QPointer <AssetSkybox> s_rad = new AssetSkybox();
            s_rad->SetAssetImages(radiance_images);
            skyboxes.push_back(s_rad);

            QVector<QPointer <AssetImage> > irradiance_images;
            irradiance_images.push_back(skybox_cubemaps[cubemap_index++]);
            QPointer <AssetSkybox> s_irrad = new AssetSkybox();
            s_irrad->SetAssetImages(irradiance_images);
            skyboxes.push_back(s_irrad);
        }
    }
}

void Room::AddAssetGhost(QPointer <AssetGhost> a)
{
    if (a.isNull()) {
        return;
    }

    QString asset_id = a->GetS("id");
    if (asset_id.isEmpty()) {
        asset_id = a->GetS("_src_url");
    }
    assetghosts[asset_id] = a;
}

void Room::AddAssetImage(QPointer <AssetImage> a)
{
    if (a.isNull()) {
        return;
    }

    QString asset_id = a->GetS("id");
    if (asset_id.isEmpty()) {
        asset_id = a->GetS("_src_url");
    }
    assetimages[asset_id] = a;
}

void Room::AddAssetObject(QPointer <AssetObject> a)
{
    if (a.isNull()) {
        return;
    }

    QString asset_id = a->GetS("id");
    if (asset_id.isEmpty()) {
        asset_id = a->GetS("_src_url");
    }
    assetobjects[asset_id] = a;
}

void Room::AddAssetRecording(QPointer <AssetRecording> a)
{
    if (a.isNull()) {
        return;
    }

    QString asset_id = a->GetS("id");
    if (asset_id.isEmpty()) {
        asset_id = a->GetS("_src_url");
    }
    assetrecordings[asset_id] = a;
}

void Room::AddAssetScript(QPointer <AssetScript> a)
{
    if (a.isNull()) {
        return;
    }

    QString asset_id = a->GetS("id");
    if (asset_id.isEmpty()) {
        asset_id = a->GetS("_src_url");
    }
    assetscripts[asset_id] = a;
}

void Room::AddAssetShader(QPointer <AssetShader> a)
{
    if (a.isNull()) {
        return;
    }

    QString asset_id = a->GetS("id");
    if (asset_id.isEmpty()) {
        asset_id = a->GetS("_src_url");
    }
    assetshaders[asset_id] = a;
}

void Room::AddAssetSound(QPointer <AssetSound> a)
{
    if (a.isNull()) {
        return;
    }

    QString asset_id = a->GetS("id");
    if (asset_id.isEmpty()) {
        asset_id = a->GetS("_src_url");
    }
    assetsounds[asset_id] = a;
}

void Room::AddAssetVideo(QPointer <AssetVideo> a)
{
    if (a.isNull()) {
        return;
    }

    QString asset_id = a->GetS("id");
    if (asset_id.isEmpty()) {
        asset_id = a->GetS("_src_url");
    }
    assetvideos[asset_id] = a;
}

void Room::AddAssetWebSurface(QPointer <AbstractWebSurface> a)
{
    if (a.isNull()) {
        return;
    }

    QString asset_id = a->GetS("id");
    if (asset_id.isEmpty()) {
        asset_id = a->GetS("_src_url");
    }
    assetwebsurfaces[asset_id] = a;
}

void Room::RemoveAsset(QPointer <Asset> a)
{
    if (a.isNull()) {
        return;
    }

    QString asset_id = a->GetS("id");
    if (asset_id.isEmpty()) {
        asset_id = a->GetS("_src_url");
    }

    if (assetghosts.contains(asset_id)) {
        assetghosts.remove(asset_id);
    }
    if (assetimages.contains(asset_id)) {
        assetimages.remove(asset_id);
    }
    if (assetobjects.contains(asset_id)) {
        assetobjects.remove(asset_id);
    }
    if (assetrecordings.contains(asset_id)) {
        assetrecordings.remove(asset_id);
    }
    if (assetscripts.contains(asset_id)) {
        assetscripts.remove(asset_id);
    }
    if (assetshaders.contains(asset_id)) {
        assetshaders.remove(asset_id);
    }
    if (assetsounds.contains(asset_id)) {
        assetsounds.remove(asset_id);
    }
    if (assetvideos.contains(asset_id)) {
        assetvideos.remove(asset_id);
    }    
    if (assetwebsurfaces.contains(asset_id)) {
        assetwebsurfaces.remove(asset_id);
    }
}

QList <QPointer <Asset> > Room::GetAllAssets()
{
    QList <QPointer <Asset> > as;
    for (QPointer <AssetGhost> & a: assetghosts) {
        as.push_back((Asset *)a.data());
    }
    for (QPointer <AssetImage> & a: assetimages) {
        as.push_back((Asset *)a.data());
    }
    for (QPointer <AssetObject> & a: assetobjects) {
        as.push_back((Asset *)a.data());
    }
    for (QPointer <AssetRecording> & a: assetrecordings) {
        as.push_back((Asset *)a.data());
    }
    for (QPointer <AssetScript> & a: assetscripts) {
        as.push_back((Asset *)a.data());
    }
    for (QPointer <AssetShader> & a: assetshaders) {
        as.push_back((Asset *)a.data());
    }
    for (QPointer <AssetSound> & a: assetsounds) {
        as.push_back((Asset *)a.data());
    }
    for (QPointer <AssetVideo> & a: assetvideos) {
        as.push_back((Asset *)a.data());
    }    
    for (QPointer <AbstractWebSurface> & a: assetwebsurfaces) {
        as.push_back((Asset *)a.data());
    }
    return as;
}

QHash <QString, QPointer <AssetGhost> > & Room::GetAssetGhosts()
{
    return assetghosts;
}

QHash <QString, QPointer <AssetImage> > & Room::GetAssetImages()
{
    return assetimages;
}

QHash <QString, QPointer <AssetObject> > & Room::GetAssetObjects()
{
    return assetobjects;
}

QHash <QString, QPointer <AssetRecording> > & Room::GetAssetRecordings()
{
    return assetrecordings;
}

QHash <QString, QPointer <AssetScript> > & Room::GetAssetScripts()
{
    return assetscripts;
}

QHash <QString, QPointer <AssetShader> > & Room::GetAssetShaders()
{
    return assetshaders;
}

QHash <QString, QPointer <AssetSound> > & Room::GetAssetSounds()
{
    return assetsounds;
}

QHash <QString, QPointer <AssetVideo> > & Room::GetAssetVideos()
{
    return assetvideos;
}

QHash <QString, QPointer <AbstractWebSurface> > & Room::GetAssetWebSurfaces()
{
    return assetwebsurfaces;
}

QPointer <Asset> Room::GetAsset(const QString id) const
{
    if (GetAssetGhost(id)) {
        return (Asset *)GetAssetGhost(id);
    }
    if (GetAssetImage(id)) {
        return (Asset *)GetAssetImage(id);
    }
    if (GetAssetObject(id)) {
        return (Asset *)GetAssetObject(id);
    }
    if (GetAssetRecording(id)) {
        return (Asset *)GetAssetRecording(id);
    }
    if (GetAssetScript(id)) {
        return (Asset *)GetAssetScript(id);
    }
    if (GetAssetShader(id)) {
        return (Asset *)GetAssetShader(id);
    }
    if (GetAssetSound(id)) {
        return (Asset *)GetAssetSound(id);
    }
    if (GetAssetVideo(id)) {
        return (Asset *)GetAssetVideo(id);
    }    
    if (GetAssetWebSurface(id)) {
        return (Asset *)GetAssetWebSurface(id);
    }
    return QPointer <Asset> ();
}

QPointer <AssetGhost> Room::GetAssetGhost(const QString id) const
{
    if (assetghosts.contains(id)) {
        return assetghosts[id];
    }
    return QPointer <AssetGhost> ();
}

QPointer <AssetImage> Room::GetAssetImage(const QString id) const
{    
    if (assetimages.contains(id)) {
        return assetimages[id];
    }
    return QPointer <AssetImage> ();
}

QPointer <AssetObject> Room::GetAssetObject(const QString id) const
{
    if (assetobjects.contains(id)) {
        return assetobjects[id];
    }
    return QPointer <AssetObject> ();
}

QPointer <AssetRecording> Room::GetAssetRecording(const QString id) const
{
    if (assetrecordings.contains(id)) {
        return assetrecordings[id];
    }
    return QPointer <AssetRecording> ();
}

QPointer <AssetScript> Room::GetAssetScript(const QString id) const
{
    if (assetscripts.contains(id)) {
       return assetscripts[id];
    }
    return QPointer <AssetScript> ();
}

QPointer <AssetShader> Room::GetAssetShader(const QString id) const
{
    if (assetshaders.contains(id)) {
        return assetshaders[id];
    }
    return QPointer <AssetShader> ();
}

QPointer <AssetSound> Room::GetAssetSound(const QString id) const
{
    if (assetsounds.contains(id)) {
        return assetsounds[id];
    }
    return QPointer <AssetSound> ();
}

QPointer <AssetVideo> Room::GetAssetVideo(const QString id) const
{
    if (assetvideos.contains(id)) {
        return assetvideos[id];
    }
    return QPointer <AssetVideo> ();
}

QPointer <AbstractWebSurface> Room::GetAssetWebSurface(const QString id) const
{
    if (assetwebsurfaces.contains(id)) {
        return assetwebsurfaces[id];
    }
    return QPointer <AbstractWebSurface> ();
}

void Room::AddAsset(const QString asset_type, const QVariantMap & property_list, const bool do_sync)
{    
//    qDebug() << "Room::AddAsset" << asset_type << property_list;
    const QString t = asset_type.toLower().trimmed();
    const QString url = GetS("url");
    if (t == "assetghost") {
        QPointer <AssetGhost> a = new AssetGhost();
        a->SetSrc(url, property_list["src"].toString());
        a->SetProperties(property_list);
        a->SetB("sync", do_sync);
        a->Load();
        AddAssetGhost(a);
    }
    else if (t == "assetimage") {
        QPointer <AssetImage> a = new AssetImage();
        a->SetSrc(url, property_list["src"].toString());
        a->SetProperties(property_list);
        a->SetB("sync", do_sync);
        a->Load();
        AddAssetImage(a);
    }
    else if (t == "assetobject") {
        QPointer <AssetObject> a = new AssetObject();
        if (!property_list.contains("src")) {
            a->GetGeom()->SetMesh(property_list);
        }
        else {
            a->SetSrc(url, property_list["src"].toString());
        }
        a->SetProperties(property_list);
        a->SetB("sync", do_sync);
        a->Load();
        AddAssetObject(a);
    }
    else if (t == "assetrecording") {
        QPointer <AssetRecording> a = new AssetRecording();
        a->SetSrc(url, property_list["src"].toString());
        a->SetProperties(property_list);
        a->SetB("sync", do_sync);
        a->Load();
        AddAssetRecording(a);
    }
     else if (t == "assetscript") {
        QPointer <AssetScript> a = new AssetScript(this);
        a->SetSrc(url, property_list["src"].toString());
        a->SetProperties(property_list);
        a->SetB("sync", do_sync);
        a->Load();
        AddAssetScript(a);
    }
    else if (t == "assetshader") {
        QPointer <AssetShader> a = new AssetShader();
        a->SetSrc(url, property_list["src"].toString(), property_list["vertex_src"].toString());
        a->SetProperties(property_list);
        a->SetB("sync", do_sync);
        a->Load();
        AddAssetShader(a);
    }
    else if (t == "assetsound") {
        QPointer <AssetSound> a = new AssetSound();
        a->SetSrc(url, property_list["src"].toString());
        a->SetProperties(property_list);
        a->SetB("sync", do_sync);
        AddAssetSound(a);
    }
    else if (t == "assetvideo") {
        QPointer <AssetVideo> a = new AssetVideo();
        a->SetSrc(url, property_list["src"].toString());
        a->SetProperties(property_list);
        a->SetB("sync", do_sync);
        AddAssetVideo(a);
    }    
    else if (t == "assetwebsurface") {
        QPointer <AbstractWebSurface> a;

#if !defined(__APPLE__) && !defined(__ANDROID__)
        a = (AbstractWebSurface*)new AssetWebSurface();
        a->SetTextureAlpha(true);
        a->GetWebView()->initializeNonMenu();

        //adjust background colour palette based on if menu is in focus or not
        QPalette palette = a->GetWebView()->palette();
        palette.setBrush(QPalette::Window, Qt::white);
        palette.setBrush(QPalette::Base, QColor(0,0,0,0));
        a->GetWebView()->setPalette(palette);
#else
        a = (AbstractWebSurface*)new AssetWebSurface();
        a->SetTextureAlpha(true);
        a->GetWebView()->initializeNonMenu();

        //adjust background colour palette based on if menu is in focus or not
        QPalette palette = a->GetWebView()->palette();
        palette.setBrush(QPalette::Window, Qt::white);
        palette.setBrush(QPalette::Base, QColor(0,0,0,0));
        a->GetWebView()->setPalette(palette);
#endif   

        a->SetSrc(url, property_list["src"].toString());
        a->SetProperties(property_list);
        a->SetB("sync", do_sync);
//        a->Load();
        AddAssetWebSurface(a);
    }
}

void Room::Create_Custom_Translator_Loaded()
{
    //Load up the translator code
    QString translator_code;
    const QString translator_fullpath = QDir::currentPath() + "/assets/translators/" + room_jsengine_translator_filename;
    QFile f(translator_fullpath);
    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << "Room::LoadCustomTranslators() - ERROR: Failed to open translator:" << translator_fullpath;
        return;
    }
    else {        
        translator_code = f.readAll();
    }
    f.close();

    room_jsengine->GetWebView()->addToJavaScriptWindowObject("janus", this);

    //execute builtin function window.janus.create_room() for translator generation    
    room_jsengine->GetWebView()->evaluateJavaScript(translator_code);
    room_jsengine->GetWebView()->evaluateJavaScript("window.janus.createroom();");

    //set room Property from the qvariantmap read
//    qDebug() << "Room::Create_Custom_Translator_Loaded() roomattribs" << room_jsoperations->GetRoomAttribs();
    SetProperties(GetJSProperties());

    //set entrance portal position to room's Property
    if (entrance_object) {
        entrance_object->SetProperties(GetJSProperties());
        entrance_object->SetB("visible", true); //53.16 don't set portal invisible (room may have invis geom, and thus visible="false")
    }

    SetB("_translator_busy", false);
}

void Room::Create_Custom_Translator(const QString translator_name)
{
    const QString url = GetS("url");
    SetB("_translator_busy", true);

    room_jsengine = new AssetWebSurface();
    room_jsengine->SetSrc(url, url);
    room_jsengine->Load();   

    room_jsengine_translator_filename = translator_name;

    //NOTE: connect to the mainFrame otherwise loadFinished is triggered numerous times
    connect(room_jsengine->GetWebView(), SIGNAL(loadFinishedFrame(bool)), this, SLOT(Create_Custom_Translator_Loaded()));

    QPointer <RoomObject> o(new RoomObject);
    SetEntranceObject(o);
}

void Room::RenameJSID(const QString & old_js_id, const QString & new_js_id)
{    
    if (envobjects.contains(old_js_id) && !envobjects.contains(new_js_id)) {
        envobjects[old_js_id]->SetS("js_id", new_js_id);
        envobjects[new_js_id] = envobjects[old_js_id];
        envobjects.remove(old_js_id);        
    }
}

PerformanceLogger & Room::GetPerformanceLogger()
{
    return perf_logger;
}

void Room::AddPrimitiveAssetObjects()
{
    //load up primitives shared by all instances
    if (object_primitives.empty()) {
        object_primitives["capsule"] = QPointer<AssetObject>(new AssetObject());
        object_primitives["cone"] = QPointer<AssetObject>(new AssetObject());
        object_primitives["cube"] = QPointer<AssetObject>(new AssetObject());
        object_primitives["cylinder"] = QPointer<AssetObject>(new AssetObject());
        object_primitives["pipe"] = QPointer<AssetObject>(new AssetObject());
        object_primitives["plane"] = QPointer<AssetObject>(new AssetObject());
        object_primitives["pyramid"] = QPointer<AssetObject>(new AssetObject());
        object_primitives["sphere"] = QPointer<AssetObject>(new AssetObject());
        object_primitives["torus"] = QPointer<AssetObject>(new AssetObject());

        QHash <QString, QPointer <AssetObject> >::iterator i;
        for (i = object_primitives.begin(); i != object_primitives.end(); ++i) {
            i.value()->SetSrc(MathUtil::GetApplicationURL(), QString("assets/primitives/") + i.key() + ".obj");
            i.value()->SetS("id", i.key());
            i.value()->SetB("_primitive", true);
            i.value()->SetB("_save_to_markup", false);
            i.value()->Load();
        }
    }

    //add them
    for (QPointer <AssetObject> & o: object_primitives) {
        AddAssetObject(o);
    }
}

QPointer <AssetShader> Room::GetTransparencyShader()
{
    return transparency_shader;
}

QPointer <AssetShader> Room::GetPortalShader()
{
    return portal_shader;
}

QPointer <AssetShader> Room::GetCubemapShader()
{
    return cubemap_shader;
}

QPointer <AssetShader> Room::GetCubemapShader2()
{
    return cubemap_shader2;
}

QPointer <AssetShader> Room::GetSkyboxShader()
{
    return skybox_shader;
}

QString Room::GetURL() const
{
    return GetS("url");
}

QVariantMap Room::GetJSProperties()
{
    return room_jsproperties;
}

QPair <QVector3D, QVector3D> Room::GetResetVolume()
{
    QPair <QVector3D, QVector3D> p;
    QStringList s = props->property("reset_volume").toString().trimmed().split(" ");
    if (s.size() >= 6) {
        p.first.setX(s[0].toFloat());
        p.first.setY(s[1].toFloat());
        p.first.setZ(s[2].toFloat());
        p.second.setX(s[3].toFloat());
        p.second.setY(s[4].toFloat());
        p.second.setZ(s[5].toFloat());
    }
    return p;
}

void Room::createasset(QString asset_type, QVariantMap property_list)
{
//        qDebug() << "Room::createasset" << asset_type;// << property_list;
    AddAsset(asset_type, property_list, false);
}

void Room::createobject(QString object_type, QVariantMap property_list)
{
//        qDebug() << "Room::createobject" << object_type;// << property_list;        
    QPointer <RoomObject> o = new RoomObject();
    o->SetType(object_type);
    o->SetProperties(property_list);
    o->SetB("sync", false);
    AddRoomObject(o);
}

void Room::SetParent(QPointer <Room> r)
{
    parent = r;
}

QPointer <Room> Room::GetParent() const {
    return parent;
}

QList <QPointer <Room> > & Room::GetChildren() {
    return children;
}

QList <QPointer <Room> > Room::GetAllChildren() {
    QList <QPointer <Room> > cs;
    cs.push_back(this);
    for (int i=0; i<children.size(); ++i) {
        cs += children[i]->GetAllChildren();
    }
    return cs;
}

QPointer <Room> Room::GetLastChild() const {
    return last_child;
}

void Room::SetLastChild(QPointer <Room> r) {
    last_child = r;
}

void Room::AddChild(QPointer <Room> r) {
    children.push_back(r);
}

void Room::RemoveChild(QPointer <Room> r) {
    children.removeAll(r);
}

QList <QPointer <Room> > Room::GetVisibleRooms() {
    QList <QPointer <Room> > l;

    //me
    l.push_back(this);

    //parent
    if (parent && entrance_object && entrance_object->GetB("open") && parent_object && parent_object->GetB("open")) {
        l.push_back(parent);
    }

    //children
    for (QPointer <Room> & r : children) {
        if (r &&
                r->GetEntranceObject() && r->GetEntranceObject()->GetB("open") &&
                r->GetParentObject() && r->GetParentObject()->GetB("open")) {
            l.push_back(r);
        }
    }    

    return l;
}

QPointer <Room> Room::GetConnectedRoom(QPointer <RoomObject> p)
{
    //I am at curnode...
    //1) p leads specifically to my parent room
    //    - p is my entrance_object
    //
    // OR
    //
    //2) p leads to a child room
    //    - p is some child room's parent_object
    //
    // OR
    //
    //3) Something is amiss, return NULL
    if (p) {
        if (p == entrance_object) {
            return parent;
        }
        for (QPointer <Room> & r : children) {
            if (r && p == r->GetParentObject()) {
                return r;
            }
        }
    }
    return QPointer <Room> ();
}

QPointer <RoomObject> Room::GetConnectedPortal(QPointer <RoomObject> p)
{
    //I am at curnode...
    //1) p leads specifically to my parent room
    //    - p is my entrance_object
    //
    // OR
    //
    //2) p leads to a child room
    //    - p is some child room's parent_object
    //
    // OR
    //
    //3) Something is amiss, return NULL
    if (p) {
        if (p == entrance_object) {
            return parent_object;
        }
        for (QPointer <Room> & r : children) {
            if (r && p == r->GetParentObject()) {
                return r->GetEntranceObject();
            }
        }
    }
    return QPointer <RoomObject> ();
}

QString Room::AddText_Interactive(const QVector3D & pos, const QVector3D & xdir, const QVector3D & ydir, const QVector3D & zdir, const QString & js_id)
{
    QPointer <RoomObject> new_text(new RoomObject());
    new_text->SetType("text");
    new_text->SetText("T");
    new_text->SetV("pos", pos);
    new_text->SetV("xdir", xdir);
    new_text->SetV("ydir", ydir);
    new_text->SetV("zdir", zdir);
    new_text->SetS("js_id", js_id);
    return AddRoomObject(new_text);
}

QString Room::AddImage_Interactive(const QVector3D & pos, const QVector3D & xdir, const QVector3D & ydir, const QVector3D & zdir, const QString & js_id)
{
    if (!assetimages.isEmpty()) {
        QPointer <AssetImage> assetimg = assetimages.begin().value();
        if (assetimg) {
            QPointer <RoomObject> envimg(new RoomObject());
            envimg->SetType("image");
            envimg->SetS("id", assetimg->GetS("id"));
            envimg->SetV("pos", pos);
            envimg->SetXDirs(xdir, ydir, zdir);
            envimg->SetV("scale", QVector3D(0.9f, 0.9f, 0.9f));
            envimg->SetS("js_id", js_id);
            return AddRoomObject(envimg);
        }
    }
    return "";
}

QString Room::AddObject_Interactive(const QVector3D & pos, const QVector3D & xdir, const QVector3D & ydir, const QVector3D & zdir, const QString & js_id)
{
    if (!assetobjects.isEmpty()) {
        const QPointer <AssetObject> a = assetobjects.begin().value();
        if (a) {
            const QString id = a->GetS("id");
            QPointer <RoomObject> envobj(new RoomObject());
            envobj->SetType("object");
            envobj->SetS("id", id);
            envobj->SetS("collision_id", id);
            envobj->SetCollisionAssetObject(a);
            envobj->SetV("pos", pos);
            envobj->SetXDirs(xdir, ydir, zdir);
            envobj->SetS("js_id", js_id);

            //fade object in
            envobj->SetV("scale", QVector3D(0,0,0));
            envobj->SetInterpolate(true);
            envobj->SetInterpolation();
            envobj->SetV("scale", QVector3D(1,1,1));
    //        qDebug() << "Room::AddObject_Interactive" << envobj->GetV("pos") << envobj->GetS("collision_id") << envobj->GetS("js_id");
    //        qDebug() << pos << xdir << ydir << zdir;
            return AddRoomObject(envobj);
        }
    }
    return "";
}

void Room::SetAllObjects(const char * name, const bool b)
{
    for (QPointer <RoomObject> & o : envobjects) {
        if (o) {
            o->SetB(name, b);
        }
    }
}
