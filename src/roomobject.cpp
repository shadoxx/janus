#include "roomobject.h"

QPointer <AssetImage> RoomObject::linear_gradient_img;
QPointer <AssetImage> RoomObject::sound_img;
QPointer <AssetImage> RoomObject::light_img;
QPointer <AssetImage> RoomObject::object_img;
QPointer <AssetImage> RoomObject::particle_img;
QPointer <AssetObject> RoomObject::cursor_arrow_obj;
QPointer <AssetObject> RoomObject::cursor_crosshair_obj;
QPointer <AssetObject> RoomObject::cursor_hand_obj;
QPointer <AssetObject> RoomObject::avatar_obj;
QPointer <AssetObject> RoomObject::avatar_head_obj;
#ifdef __ANDROID__
QPointer <AssetImage> RoomObject::internet_connected_img;
QPointer <AssetImage> RoomObject::remove_headset_img;
#endif

int RoomObject::objects_allocated = 0;
float RoomObject::portal_spacing = 0.1f;
bool RoomObject::draw_assetobject_teleport = false;

//Network-related rates, in seconds
float RoomObject::rate = 0.2f;
float RoomObject::logoff_rate = 3.0f;

RoomObject::RoomObject() :
    props(new DOMNode()),
    save_to_markup(true),
    grabbed(false)
{
    ++objects_allocated;

    SetType("object");
    last_rotation = QVector3D(0,0,0);
    selected = false;    
    tex_width = 1024;
    tex_height = 1024;    
    playing = false;    
    do_multiplayer_timeout = false;    
    play_sounds = true;        
    time_elapsed = 0.0f;
    head_avatar_pos = QVector3D(0,1,0);

    eye_ipd = 0.0f;  
    error_reported = false;
    openal_stream_source = 0;

    assetghost = 0;
    assetimage = 0;
    assetsound = 0;
    assetobject_anim = 0;
    assetobject = 0;
    assetobject_collision = 0;    
    assetobject_emitter = 0;
    assetshader = 0;
    assetvideo = 0;
    assetwebsurface = 0;
    assetobject_teleport = 0;    
    ghost_frame_index = -1; //49.50 ensures ghosts always load, even for single-frame animations where the frameindex = 0

    interpolate = false;
    interp_val = 1.0f;
    interp_time = 0.2f;
    interp_timer.start();

    particle_system = new ParticleSystem();

    assetobject_blendshapes = QVector<QPointer <AssetObject> >(4);

    textgeom = QPointer <TextGeom> (new TextGeom());
    textgeom_player_id = QPointer <TextGeom> (new TextGeom());
    textgeom_player_id->SetFixedSize(true, 0.075f);

    textgeom_url = QPointer <TextGeom> (new TextGeom());
    textgeom_title = QPointer <TextGeom> (new TextGeom());

    player_in_room = false;
    player_in_adjacent_room = false;

    m_cubemap_radiance = 0;
    m_cubemap_irradiance = 0;
    assetimage_lmap = 0;

    rescale_on_load = false;
    rescale_on_load_aspect = false;
    rescale_on_load_done = false;

    sound_buffers_sample_rate = 44100;
}

RoomObject::~RoomObject()
{
//    qDebug() << "RoomObject::~RoomObject()" << this;
    --objects_allocated;
    Clear();

    if (particle_system) {
        delete particle_system;
    }

    if (portal_text) {
        delete portal_text;
    }

    if (props) {
        delete props;
    }
}

void RoomObject::Clear()
{
    //Note: envobject does not do any deallocation - it only holds pointers to things
    collision_set.clear();

    if (assetsound) {
        AssetSound::ClearOutput(&media_ctx);
    }

    if (assetvideo) {
        AssetVideo::ClearOutput(&media_ctx);
    }

    if (!sound_buffers.isEmpty()) {
        sound_buffers.clear();
    }

    if (openal_stream_source > 0) {
        ALint is_playing;
        alGetSourcei(openal_stream_source, AL_SOURCE_STATE, &is_playing);
        if (is_playing == AL_PLAYING) {
            alSourceStop(openal_stream_source);
        }

        //Dequeue buffers
        int buffers_to_dequeue = 0;
        alGetSourcei(openal_stream_source, AL_BUFFERS_QUEUED, &buffers_to_dequeue);
        if (buffers_to_dequeue > 0)
        {
            QVector<ALuint> buffHolder;
            buffHolder.resize(buffers_to_dequeue);
            alSourceUnqueueBuffers(openal_stream_source, buffers_to_dequeue, buffHolder.data());
            for (int i=0;i<buffers_to_dequeue;++i) {
                // Push the recovered buffers back on the queue
                SoundManager::buffer_input_queue.push_back(buffHolder[i]);
            }
        }

        alSourcei(openal_stream_source, AL_BUFFER, 0);

        alDeleteSources(1, &openal_stream_source);
        openal_stream_source = 0;
    }

    assetshader.clear();
    // TYPE_PARAGRAPH creates it's own assetimage separate from the usual asset creation pipeline
    // so in this edge-case we need to delete the assetimage to stop it leaking.
    if (GetType() == "paragraph" && assetimage) {
        delete assetimage;
    }
    assetimage.clear();
    assetsound.clear();
    assetvideo.clear();
    assetwebsurface.clear();

    assetvideo.clear();
    assetobject.clear();
    assetobject_collision.clear();
    assetobject_emitter.clear();
    assetobject_teleport.clear();
    assetobject_anim.clear();
    assetghost.clear();
    assetobject_blendshapes.clear();

    ghost_assetobjs.clear();
    ghost_asset_shaders.clear();

    for (int i=0; i<child_objects.size(); ++i) {
        if (child_objects[i]) {
            delete child_objects[i];
        }
    }
    child_objects.clear();

    if (textgeom) {
        delete textgeom;
    }
    if (textgeom_player_id) {
        delete textgeom_player_id;
    }
    if (textgeom_url) {
        delete textgeom_url;
    }
    if (textgeom_title) {
        delete textgeom_title;
    }

    player_in_room = false;
    player_in_adjacent_room = false;
}

void RoomObject::initializeGL()
{
    if (linear_gradient_img.isNull()) {
        linear_gradient_img = QPointer<AssetImage>(new AssetImage());
        linear_gradient_img->SetB("tex_clamp", true);
        linear_gradient_img->SetSrc(MathUtil::GetApplicationURL(), "assets/linear_gradient.png");
        linear_gradient_img->Load();
    }

    if (sound_img.isNull()) {
        sound_img = new AssetImage();
        sound_img = QPointer<AssetImage>(new AssetImage());
        sound_img->SetSrc(MathUtil::GetApplicationURL(), "assets/sound.png");
        sound_img->Load();
    }

    if (light_img.isNull()) {
        light_img = new AssetImage();
        light_img = QPointer<AssetImage>(new AssetImage());
        light_img->SetSrc(MathUtil::GetApplicationURL(), "assets/light.png");
        light_img->Load();
    }

    if (object_img.isNull()) {
        object_img = new AssetImage();
        object_img = QPointer<AssetImage>(new AssetImage());
        object_img->SetSrc(MathUtil::GetApplicationURL(), "assets/object.png");
        object_img->Load();
    }

    if (particle_img.isNull()) {
        particle_img = new AssetImage();
        particle_img = QPointer<AssetImage>(new AssetImage());
        particle_img->SetSrc(MathUtil::GetApplicationURL(), "assets/particle.png");
        particle_img->Load();
    }

    if (cursor_arrow_obj.isNull()) {
        cursor_arrow_obj = new AssetObject();
        cursor_arrow_obj->SetSrc(MathUtil::GetApplicationURL(), "assets/primitives/plane.obj");
        cursor_arrow_obj->SetTextureClamp(true);
        cursor_arrow_obj->SetTextureAlphaType("blended");
        cursor_arrow_obj->SetTextureFile("./assets/cursor/arrow.png", 0);
        cursor_arrow_obj->Load();
    }

    if (cursor_crosshair_obj.isNull()) {
        cursor_crosshair_obj = new AssetObject();
        cursor_crosshair_obj->SetSrc(MathUtil::GetApplicationURL(), "assets/primitives/plane.obj");
        cursor_crosshair_obj->SetTextureClamp(true);
        cursor_crosshair_obj->SetTextureLinear(true);
        cursor_crosshair_obj->SetTextureMipmap(true);
        cursor_crosshair_obj->SetTextureAlphaType("blended");
        cursor_crosshair_obj->SetTextureFile("./assets/cursor/crosshair.png", 0);
        cursor_crosshair_obj->Load();
    }

    if (cursor_hand_obj.isNull()) {
        cursor_hand_obj = new AssetObject();
        cursor_hand_obj->SetSrc(MathUtil::GetApplicationURL(), "assets/primitives/plane.obj");
        cursor_hand_obj->SetTextureClamp(true);
        cursor_hand_obj->SetTextureLinear(true);
        cursor_hand_obj->SetTextureMipmap(true);
        cursor_hand_obj->SetTextureAlphaType("blended");
        cursor_hand_obj->SetTextureFile("./assets/cursor/hand.png", 0);
        cursor_hand_obj->Load();
    }

    if (avatar_obj.isNull()) {
        avatar_obj = new AssetObject();
        avatar_obj->SetSrc(MathUtil::GetApplicationURL(), "assets/avatar.obj");
        avatar_obj->SetTextureFile("./assets/avatar.png", 0);
        avatar_obj->Load();
    }

    if (avatar_head_obj.isNull()) {
        avatar_head_obj = new AssetObject();
        avatar_head_obj->SetSrc(MathUtil::GetApplicationURL(), "assets/avatar_head.obj");
        avatar_head_obj->SetTextureFile("./assets/avatar_head.png", 0);
        avatar_head_obj->Load();
    }

#ifdef __ANDROID__
    if (internet_connected_img.isNull()) {
        internet_connected_img = new AssetImage();
        internet_connected_img = QPointer<AssetImage>(new AssetImage());
        internet_connected_img->SetSrc(MathUtil::GetApplicationURL(), "assets/splash/internet-connection.png");
        internet_connected_img->Load();
    }

    if (remove_headset_img.isNull()) {
        remove_headset_img = new AssetImage();
        remove_headset_img = QPointer<AssetImage>(new AssetImage());
        remove_headset_img->SetSrc(MathUtil::GetApplicationURL(), "assets/splash/remove-headset.png");
        remove_headset_img->Load();
    }
#endif
}

QPointer<DOMNode> RoomObject::GetProperties()
{
    return props;
}

void RoomObject::SetType(const QString t)
{
//    qDebug() << "RoomObject::SetType" << t;
    SetS("_type", t);   

    if (t == "link") {
        SetV("scale", QVector3D(1.8f, 2.5f, 1.0f));

        //portals
        if (portal_text.isNull()) {
            portal_text = new RoomObject();
            portal_text->SetType("paragraph");
            portal_text->SetF("back_alpha", 0.0f);
            portal_text->SetI("font_size", 32);
            portal_text->SetV("pos", QVector3D(0.0f, 0.0f, 0.0f));
            portal_text->SetDir(QVector3D(0.0f, 0.0f, 1.0f));
            portal_text->SetB("lighting", false);
        }
    }
//    qDebug() << "RoomObject::SetType"  << GetJSID() << GetID() << "allocated" << objects_allocated << "type" << GetType();
}

QString RoomObject::GetTagName() const
{
    const QString t = GetType();

    if (t == "image") {
        return "Image";
    }
    else if (t == "object") {
        return "Object";
    }
    else if (t == "video") {
        return "Video";
    }
    else if (t == "text") {
        return "Text";
    }
    else if (t == "ghost") {
        return "Ghost";
    }
    else if (t == "paragraph") {
        return "Paragraph";
    }
    else if (t == "sound") {
        return "Sound";
    }
    else if (t == "image3d") {
        return "Image3D";
    }
    else if (t == "link") {
        return "Link";
    }
    else if (t == "particle") {
        return "Particle";
    }
    else if (t == "light") {
        return "Light";
    }
}

QString RoomObject::GetType() const
{
    return GetS("_type");
}

void RoomObject::SetProperties(QPointer<DOMNode> props)
{
    this->props = props;
}

void RoomObject::SetProperties(QVariantMap d)
{
    if (d.contains("js_id") && d["js_id"].toString().isEmpty()) {
        d.remove("js_id");
    }

//    qDebug() << "RoomObject::SetProperties" << this << d;
    QVariantMap::const_iterator it;
    for (it=d.begin(); it!=d.end(); ++it) {
//        qDebug() << " " << it.key() << it.value().toString();
        const QString key = it.key().trimmed().toLower();
        if (!key.isEmpty()) {
            //59.13 - hack to support ScriptableVector
            if (props) {
                if (!props->property(key.toLatin1().data()).isNull() && props->property(key.toLatin1().data()).value<ScriptableVector*>()) {
                    const QString values = it.value().toString().trimmed();
                    const QStringList splitvals = values.split(" ");
                    if (splitvals.size() >= 4) {
                        SetV4(key.toLatin1().data(), MathUtil::GetStringAsVector4(values));
                    }
                    else if (splitvals.size() >= 3) {
                        SetV(key.toLatin1().data(), MathUtil::GetStringAsVector(values));
                    }
                }
                else {
                    props->setProperty(key.toLatin1().data(), it.value());
                }
            }
        }
    }

    if (d.contains("fwd")) {
        SetDir(MathUtil::GetStringAsVector(d["fwd"].toString()));
    }
    SetXDirs(GetV("xdir"), GetV("ydir"), GetV("zdir"));
    if (d.contains("text") || d.contains("innertext")) {
        SetB("_text_changed", true);
    }
}

bool RoomObject::GetRaycastIntersection(const QMatrix4x4 transform, QList <QVector3D> & int_verts, QList <QVector3D> & int_normals, QList <QVector2D> & int_texcoords)
{
//    qDebug() << "RoomObject::GetRaycastIntersection" << this->GetID() << this->GetCollisionID();
    const bool visible = GetB("visible");
    const bool edit_mode_enabled = SettingsManager::GetEditModeEnabled();
    const bool edit_mode_icons_enabled = SettingsManager::GetEditModeIconsEnabled();
    const QVector3D ray_p = transform.column(3).toVector3D();
    const QVector3D ray_d = transform.column(2).toVector3D();

    QList <QVector3D> int_pts;

    const QString t = GetType();
    if (t == "sound" || t == "light" || t == "particle") {
        if (edit_mode_enabled && edit_mode_icons_enabled) {
            //manip handle sphere test
            Sphere3D s;
            s.cent = GetPos();
            s.rad = SpinAnimation::GetIconScale() * 0.5f;

            int nbInter;
            float inter1, inter2;
            if (MathUtil::testIntersectionSphereLine(s, ray_p, ray_p + ray_d, nbInter, inter1, inter2)) {

                if (nbInter >= 1) {
                    int_verts.push_back(ray_p + ray_d * inter1);
                    int_normals.push_back(-ray_d);
                    int_texcoords.push_back(QVector2D(0,0));
                }
                if (nbInter >= 2) {
                    int_verts.push_back(ray_p + ray_d * inter2);
                    int_normals.push_back(-ray_d);
                    int_texcoords.push_back(QVector2D(0,0));
                }
            }
        }
    }
    else if (t == "object") {
        //other tests
        QPointer <AssetObject> obj;

        bool do_collider_transform = false;
        //use the teleport assetobject if it exists, otherwise use the geometry itself
        if (assetobject_teleport) {
            obj = assetobject_teleport;
        }
        else if (assetobject && visible && (GetS("websurface_id").length() > 0 || GetS("video_id").length() > 0)) {
            obj = assetobject;
        }
        else if (assetobject_collision) {
            obj = assetobject_collision;
            do_collider_transform = true;
        }

        if (obj && obj->GetFinished())
        {
            const QVector3D bbox_min = obj->GetBBoxMin();
            const QVector3D bbox_max = obj->GetBBoxMax();

            QMatrix4x4 model_matrix = model_matrix_global;

            if (do_collider_transform)
            {
                const QVector3D cp = GetV("collision_pos");
                const QVector3D cs = GetV("collision_scale");
                model_matrix.translate(cp.x(), cp.y(), cp.z());
                model_matrix.scale(cs.x(), cs.y(), cs.z());
            }

            const QMatrix4x4 model_matrix_inv = model_matrix.inverted();     
            const QVector3D ray_pos_object_space = model_matrix_inv.map(ray_p);
            const QVector3D ray_dir_object_space = model_matrix_inv.mapVector(ray_d);

            //determine if the ray crosses the bounding sphere which encapsulates this bounding box
            Sphere3D s;
            s.cent = (bbox_min + bbox_max) * 0.5f;
            s.rad = (bbox_max - bbox_min).length() * 0.5f;

            int nbInter;
            float inter1, inter2;

            // If the ray intersects the bounding sphere for this AssetObject
            if (MathUtil::testIntersectionSphereLine(s, ray_pos_object_space, ray_pos_object_space + ray_dir_object_space, nbInter, inter1, inter2))
            {
                const QList <QString> materials = obj->GetGeom()->GetData().GetMaterialNames();

                // For each material used in this AssetObject
                for (int k=0; k<materials.size(); ++k)
                {
                    // Get the unordered_map of Meshes(QVector <GeomTriangle>) that use this material
                    GeomMaterial & material = obj->GetGeom()->GetData().GetMaterial(materials[k]);
                    QVector<QVector<GeomTriangle>>& triangles_map = material.triangles;
                    QVector<GeomVBOData>& vbo_map = material.vbo_data;
                    QVector<QPair<uint32_t, size_t>>& mesh_keys = material.mesh_keys;
                    size_t const mesh_count = mesh_keys.size();

                    // For each mesh that uses this material of this AssetObject
                    for (unsigned int mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
                    {
                        const QVector<GeomTriangle> & triangles = triangles_map[mesh_keys[mesh_index].second];
                        const GeomVBOData & vbo_data = vbo_map[mesh_keys[mesh_index].second];
                        const QVector <QMatrix4x4>& mesh_instance_transforms = vbo_data.m_instance_transforms;

                        // For each instance of this mesh
                        QVector3D ray_pos_instance_space;
                        QVector3D ray_dir_instance_space;
                        QMatrix4x4 inv_instance_position_transform;
                        QMatrix4x4 instance_normal_transform;
                        QMatrix4x4 inv_instance_normal_transform;

                        size_t const instance_count = mesh_instance_transforms.size();
                        for (size_t instance_index = 0; instance_index < instance_count; ++instance_index)
                        {
                            // Transform ray into instance-local space
                            inv_instance_position_transform = mesh_instance_transforms[instance_index].inverted();
                            instance_normal_transform = mesh_instance_transforms[instance_index].inverted().transposed();
                            inv_instance_normal_transform = instance_normal_transform.inverted();
                            ray_pos_instance_space = inv_instance_position_transform.map(ray_pos_object_space);
                            ray_dir_instance_space = inv_instance_position_transform.mapVector(ray_dir_object_space);
                            ray_dir_instance_space.normalize();

                            // Raycast against the first 16k triangles of a mesh
                            const size_t triangle_loop_count = qMin(triangles.size(), 16384);
                            for (size_t i = 0; i < triangle_loop_count; ++i)
                            {
                                // per-triangle test
                                QVector3D p0 = QVector3D(triangles[i].p[0][0],
                                                         triangles[i].p[0][1],
                                                         triangles[i].p[0][2]);

                                QVector3D p1 = QVector3D(triangles[i].p[1][0],
                                                         triangles[i].p[1][1],
                                                         triangles[i].p[1][2]);

                                QVector3D p2 = QVector3D(triangles[i].p[2][0],
                                                         triangles[i].p[2][1],
                                                         triangles[i].p[2][2]);

                                QVector3D int_pt;
                                if (MathUtil::GetRayTriIntersect(ray_pos_instance_space, ray_dir_instance_space, p0, p1, p2, int_pt)
                                    && QVector3D::dotProduct(int_pt - ray_pos_instance_space, ray_dir_instance_space) > 0.0f)
                                {
                                    // If sucess, compute barycentric to get interpolated pos, normal, and UV for the intersection point.
                                    float f0, f1, f2;
                                    MathUtil::ComputeBarycentric3D(int_pt, p0, p1, p2, f0, f1, f2);

                                    // Map the barycentric position back into instance-space then back into room-space
                                    QVector4D room_space_intersection_pos = model_matrix.map(mesh_instance_transforms[instance_index].map(int_pt));
                                    int_verts.push_back(room_space_intersection_pos.toVector3D());

                                    // Map the barycentric normal back into instance-space then back into room-space
                                    QVector3D barycentric_normal = QVector3D(triangles[i].n[0][0], triangles[i].n[0][1], triangles[i].n[0][2]) * f0 +
                                                                   QVector3D(triangles[i].n[1][0], triangles[i].n[1][1], triangles[i].n[1][2]) * f1 +
                                                                   QVector3D(triangles[i].n[2][0], triangles[i].n[2][1], triangles[i].n[2][2]) * f2;
                                    barycentric_normal.normalize();
                                    QVector3D room_space_intersection_normal = model_matrix.mapVector(instance_normal_transform.mapVector(barycentric_normal).normalized());
                                    room_space_intersection_normal.normalize();
                                    int_normals.push_back(room_space_intersection_normal);

                                    // UV
                                    QVector2D int_uv = QVector2D(triangles[i].t[0][0], triangles[i].t[0][1]) * f0 +
                                                       QVector2D(triangles[i].t[1][0], triangles[i].t[1][1]) * f1 +
                                                       QVector2D(triangles[i].t[2][0], triangles[i].t[2][1]) * f2;
                                    int_uv.setY(1.0f - int_uv.y()); //UVs are flipped vertically
                                    int_texcoords.push_back(int_uv);
                                }
                            }
                        }
                    }
                }
            }
        }

        //manip handle sphere test 57.1 - note, it should not get in way of normal interaction
        if (edit_mode_enabled && edit_mode_icons_enabled && int_verts.empty() && GetS("js_id").left(6) != "__menu") {
            Sphere3D s;
            s.cent = GetPos();
            s.rad = SpinAnimation::GetIconScale() * 0.5f;

            int nbInter;
            float inter1, inter2;
            if (MathUtil::testIntersectionSphereLine(s, ray_p, ray_p + ray_d, nbInter, inter1, inter2)) {

                if (nbInter >= 1) {
                    int_verts.push_back(ray_p + ray_d * inter1);
                    int_normals.push_back(-ray_d);
                    int_texcoords.push_back(QVector2D(0,0));
                }
                if (nbInter >= 2) {
                    int_verts.push_back(ray_p + ray_d * inter2);
                    int_normals.push_back(-ray_d);
                    int_texcoords.push_back(QVector2D(0,0));
                }
            }
        }
    }
    else if (t == "ghost") {
        //compute cursor intersection for ghost
        //billboard plane
        if (visible) {
            QVector3D p = GetPos();
            QVector3D z = ray_p - p;
            z.setY(0.0f);
            z.normalize();
            QVector3D y(0,1,0);
            QVector3D x = QVector3D::crossProduct(y,z).normalized();

            QVector3D int_pt;
            if (MathUtil::LinePlaneIntersection(p, z, ray_p, ray_p + ray_d, int_pt) && QVector3D::dotProduct(int_pt - ray_p, ray_d) > 0.0f) {
                const float x_dot = fabsf(QVector3D::dotProduct(int_pt - p, x));
                const float y_dot = QVector3D::dotProduct(int_pt - p, y);

                if (x_dot < 0.5f && y_dot > 0.0f && y_dot < 1.8f) {
                    int_verts.push_back(int_pt);
                    int_normals.push_back(z);
                    int_texcoords.push_back(QVector2D(x_dot, y_dot));
                }
            }
        }
    }
    else if (t == "image" || t =="video" || t == "link" || t == "text" || t == "paragraph") {
//        qDebug() << "TESTING PTAL FOR INTERSECTION" << this->GetID() << this->GetJSID();
        if (visible) {
            QVector4D bbox_min(-1,-1,0,1);
            QVector4D bbox_max(1,1,0.1f,1);
            QMatrix4x4 m = model_matrix_local;

            if (t == "image") {
                if (assetimage) {
                    m.scale(1.0f, assetimage->GetAspectRatio(), 1.0f);
                }
            }
            else if (t == "video") {
                if (assetimage) {
                    m.scale(1.0f, assetimage->GetAspectRatio(), 1.0f);
                }
                else if (assetvideo) {
                    m.scale(1.0f, assetvideo->GetAspectRatio(&media_ctx), 1.0f);
                }
            }
            else if (t == "text") {
                if (textgeom) {
                    const float f = float(textgeom->GetText().length());
                    m.scale(f * 0.5f, 1, 1);
                    m.translate(1, 0, 0);
                }
            }
            else {
                const float w = GetScale().x() * 0.5f;
                const float h = GetScale().y() * 0.5f;
                m.translate(0, h, 0);
                m.scale(w, h, 1.0f);
            }

            const QVector4D transform_bbox_min = (m * bbox_min);
            const QVector4D transform_bbox_max = (m * bbox_max);
            const QVector3D cent = (transform_bbox_min.toVector3D() + transform_bbox_max.toVector3D()) * 0.5f;

            QVector3D int_pt;
            if (MathUtil::LinePlaneIntersection(cent, GetZDir(), ray_p, ray_p + ray_d, int_pt) && QVector3D::dotProduct(int_pt - ray_p, ray_d) > 0.0f) {

                const QVector3D x = m.column(0).toVector3D();
                const QVector3D y = m.column(1).toVector3D();

                const float x_dot = fabsf(QVector3D::dotProduct(int_pt - cent, x.normalized()));
                const float y_dot = fabsf(QVector3D::dotProduct(int_pt - cent, y.normalized()));

                if (x_dot < x.length() && y_dot < y.length()) {
    //                qDebug() << "DOT PRODS ARE LESS THAN 1!" << x << y << x_dot << y_dot;
                    int_verts.push_back(int_pt);
                    int_normals.push_back(m.column(2).toVector3D().normalized());
                    int_texcoords.push_back(QVector2D(x_dot, y_dot));
                }
            }
        }
    }   

    return !(int_verts.isEmpty());
}

void RoomObject::SetAttributeModelMatrix(const QMatrix4x4 & m)
{
    SetV("xdir", m.column(0).toVector3D().normalized());
    SetV("ydir", m.column(1).toVector3D().normalized());
    SetV("zdir", m.column(2).toVector3D().normalized());
    SetV("pos", m.column(3).toVector3D());
    SetV("scale", QVector3D(m.column(0).toVector3D().length(),
                              m.column(1).toVector3D().length(),
                              m.column(2).toVector3D().length()));
}

QMatrix4x4 RoomObject::GetAttributeModelMatrix() const
{
    const QVector3D s = GetV("scale");
    QMatrix4x4 m;
    m.setColumn(0, GetV("xdir") * s.x());
    m.setColumn(1, GetV("ydir") * s.y());
    m.setColumn(2, GetV("zdir") * s.z());
    m.setColumn(3, GetV("pos"));
    m.setRow(3, QVector4D(0,0,0,1));
    return m;
}

void RoomObject::SetAttributeModelMatrixNoScaling(const QMatrix4x4 & m)
{
    SetV("xdir", m.column(0).toVector3D().normalized());
    SetV("ydir", m.column(1).toVector3D().normalized());
    SetV("zdir", m.column(2).toVector3D().normalized());
    SetV("pos", m.column(3).toVector3D());
}

QMatrix4x4 RoomObject::GetAttributeModelMatrixNoScaling() const
{
    QMatrix4x4 m;
    m.setColumn(0, GetV("xdir"));
    m.setColumn(1, GetV("ydir"));
    m.setColumn(2, GetV("zdir"));
    m.setColumn(3, GetV("pos"));
    m.setRow(3, QVector4D(0,0,0,1));
    return m;
}

QVector3D RoomObject::GetPos() const
{
    QVector3D pos = GetV("pos");
    if (interpolate && interp_val < 1.0f) {
        return interp_pos * (1.0f - interp_val) + pos * interp_val;
    }
    else {
        return pos;
    }
}

QVector3D RoomObject::GetVel() const
{
    return GetV("vel");
}

QVector3D RoomObject::GetAccel() const
{
    return GetV("accel");
}

void RoomObject::SetDir(const QVector3D & p)
{
    const QVector3D z = p.normalized();
    const QVector3D x = QVector3D::crossProduct(QVector3D(0, 1, 0), z).normalized();
    const QVector3D y = QVector3D::crossProduct(z, x).normalized();

    SetV("xdir", x);
    SetV("ydir", y);
    SetV("zdir", z);
}

QVector3D RoomObject::GetDir() const
{
    return GetV("zdir");
}

void RoomObject::SetXDirs(const QString & x, const QString & y, const QString & z)
{
    if (!x.isEmpty()) {
        SetV("xdir", MathUtil::GetStringAsVector(x));
    }
    if (!y.isEmpty()) {
        SetV("ydir", MathUtil::GetStringAsVector(y));
    }
    if (!z.isEmpty()) {
        SetV("zdir", MathUtil::GetStringAsVector(z).normalized());
    }

    const QVector3D dy = GetV("ydir");
    const QVector3D dz = GetV("zdir");

    SetV("ydir", (dy - dz * QVector3D::dotProduct(dy, dz)).normalized());
    SetV("xdir", QVector3D::crossProduct(dy, dz));
}

void RoomObject::SetXDirs(const QVector3D & x, const QVector3D & y, const QVector3D & z)
{
    SetV("xdir", x);
    SetV("ydir", y);
    SetV("zdir", z.normalized());

    const QVector3D dy = GetV("ydir");
    const QVector3D dz = GetV("zdir");

    //re-orthgonalize/ortho-normalize
    SetV("ydir", (dy - dz * QVector3D::dotProduct(dy, dz)).normalized());
    SetV("xdir", QVector3D::crossProduct(dy, dz));
//    qDebug() << "setting xdirs" << x << y << z << props->GetXDir()->GetQVector3D() << props->GetYDir()->GetQVector3D() << props->GetZDir()->GetQVector3D();
}

void RoomObject::SetSaveToMarkup(const bool b)
{
    save_to_markup = b;
}

bool RoomObject::GetSaveToMarkup() const
{
    return save_to_markup;
}

void RoomObject::SnapXDirsToMajorAxis()
{
    QVector3D x[3];
    x[0] = this->GetXDir();
    x[1] = this->GetYDir();
    x[2] = this->GetZDir();

    QList <QVector3D> a;
    a.push_back(QVector3D(1,0,0));
    a.push_back(QVector3D(-1,0,0));
    a.push_back(QVector3D(0,1,0));
    a.push_back(QVector3D(0,-1,0));
    a.push_back(QVector3D(0,0,1));
    a.push_back(QVector3D(0,0,-1));

    //get closest major axes for x and y
    for (int j=0; j<2; ++j) {
        int best_index = 0;
        float best_prod = -FLT_MAX;
        for (int i=0; i<a.size(); ++i) {
            const float each_prod = QVector3D::dotProduct(x[j], a[i]);
            if (each_prod > best_prod) {
                best_prod = each_prod;
                best_index = i;
            }
        }
        x[j] = a[best_index];
        a.removeAt(best_index);
    }

    x[2] = QVector3D::crossProduct(x[0], x[1]);

    SetV("xdir", x[0]);
    SetV("ydir", x[1]);
    SetV("zdir", x[2]);
}

void RoomObject::Update(const double dt_sec)
{
    const QString obj_type = GetType();

    if (assetimage) {
        assetimage->UpdateGL();
    }
    for (QPointer <AssetObject> a : ghost_assetobjs) {
        if (a) {
            a->Update();
            a->UpdateGL();
        }
    }

    if (obj_type != "ghost" || playing) {
        time_elapsed += dt_sec;
    }

    if (!interpolate) {
        interp_pos = GetV("pos");
        interp_xdir = GetV("xdir");
        interp_ydir = GetV("ydir");
        interp_zdir = GetV("zdir");
        interp_scale = GetV("scale");
    }

    UpdateMatrices();   

    if (obj_type == "particle") {
        if (assetobject_emitter) {
            particle_system->SetEmitterMesh(assetobject_emitter);
        }
        else {
            particle_system->SetEmitterMesh(NULL);
        }

        particle_system->Update(props, dt_sec);
    }

    //update child objects
    for (int i=0; i<child_objects.size(); ++i) {        
        if (child_objects[i]) {
            child_objects[i]->model_matrix_parent = model_matrix_parent * model_matrix_local;
            child_objects[i]->Update(dt_sec);            
        }
    }

    //rescale objects (generally used for drag and drop where scale can be crazy)
    if (rescale_on_load && !rescale_on_load_done && assetobject && assetobject->GetFinished()) {
        QVector3D bmin, bmax;
        QPointer <AssetObject> a = assetobject;
        bmin = a->GetBBoxMin();
        bmax = a->GetBBoxMax();

        const float d  = 1.0f / (bmax - bmin).length();
        SetV("scale", QVector3D(d,d,d));

        SetV("collision_pos", (bmin+bmax)*0.5f);
        SetV("collision_scale", bmax - bmin);
//        SetCollisionStatic(false);

        rescale_on_load_done = true;
    }

    if (rescale_on_load_aspect && !rescale_on_load_done && assetobject && assetobject->GetFinished()) {
        float aspect_ratio = 1.0;
        if (assetimage && assetimage->GetFinished()){
            aspect_ratio = assetimage->GetAspectRatio();
            SetV("scale", QVector3D(1.0f,aspect_ratio,0.1f));
            rescale_on_load_done = true;
        }
        else if (assetvideo){
            aspect_ratio = assetvideo->GetAspectRatio(&media_ctx);
            SetV("scale", QVector3D(1.0f,aspect_ratio,0.1f));
            rescale_on_load_done = true;
        }
    }

    //update the object's current_time and total_time members for sound/video playback for JS reads
    if (assetsound) {
        SetF("current_time", assetsound->GetCurTime(&media_ctx));
        SetF("total_time", assetsound->GetTotalTime(&media_ctx));
    }
    else if (assetvideo) {
        SetF("current_time", assetvideo->GetCurTime(&media_ctx));
        SetF("total_time", assetvideo->GetTotalTime(&media_ctx));
    }

    if (assetsound && !media_ctx.setup) {
        assetsound->SetupOutput(&media_ctx, GetB("loop"));
    }

    if (assetvideo && !media_ctx.setup) {
        assetvideo->SetupOutput(&media_ctx);
    }

    // HACKS: Trigger side-effects on setters.
    // will be fixed when logic is ported over to properties.
    if (obj_type == "text" && textgeom) { //60.0 - bugfix update text changed by JS
        if (GetS("text") != textgeom->GetText()) {
            SetB("_text_changed", true);
        }
    }
    if (GetB("_text_changed")) {
        SetText(GetS("text"));
        SetB("_text_changed", false);
    }

    if (interpolate && interp_time > 0.0f && obj_type != "link" && obj_type != "sound") {
        interp_val = float(interp_timer.elapsed()) / (interp_time * 1000.0f);
        interp_val = qMax(0.0f, qMin(interp_val, 1.0f));
    }
    else {
        interp_val = 1.0f;
    }

    if (obj_type == "ghost") {
//        qDebug() << "RoomObject::Update() time_elapsed" << time.elapsed() << this->GetID() << ghost_packet_index;

        const QString id = GetS("id");
        if (id != textgeom_player_id->GetText()) {
            textgeom_player_id->SetText(id);
        }

        if (ghost_frame.hands.first.is_active || ghost_frame.hands.second.is_active) {
            textgeom_player_id->SetColour(QColor(0,0,255));
        }
        else if (ghost_frame.editing) {
            textgeom_player_id->SetColour(QColor(255,128,255));
        }
        else if (ghost_frame.speaking) {
            const int v = qMin(255, int(512.0f*ghost_frame.current_sound_level));
            textgeom_player_id->SetColour(QColor(255-v,255,255-v));
        }
        else if (ghost_frame.typing) {
            textgeom_player_id->SetColour(QColor(255,255,0));
        }
        else {
            textgeom_player_id->SetColour(QColor(255,255,255));
        }

        //update active animation for avatar
        const QString body_id = GetS("body_id");
        QPointer <AssetObject> assetobj = ghost_assetobjs[body_id];
        if (assetobj) {
            QPointer <Geom> body = assetobj->GetGeom();
            const QString anim_id = GetS("anim_id");

            if (body && body->GetReady() &&
                    ghost_assetobjs.contains(anim_id) &&
                    ghost_assetobjs[anim_id] &&
                    ghost_assetobjs[anim_id]->GetFinished() &&
                    ghost_assetobjs[anim_id]->GetGeom()->GetReady()) {
                body->SetLinkAnimation(ghost_assetobjs[anim_id]->GetGeom());
            }

            if (assetobj->GetGeom()) {
                assetobj->GetGeom()->SetAnimSpeed(GetF("anim_speed"));
                assetobj->GetGeom()->SetLoop(true);
            }
        }

        QPointer <AssetGhost> g = assetghost;
        if (g && g->GetProcessed()) { //56.0 - ensure ghost data is processed before doing this

            ghost_frame = GhostFrame();

            const int cur_ghost_frame_index = g->GetFrameIndex(time_elapsed);
            const bool sequence_end = g->GetGhostFrame(time_elapsed, ghost_frame);

            g->ClearEditsDeletes();

            //54.8 - auto start/stop only for non-multiplayer avatars
            if (playing && sequence_end && !do_multiplayer_timeout) {
                if (GetB("loop")) {
                    time_elapsed = 0.0f;
                }
                else {
                    playing = false;
                }
            }

            if (do_multiplayer_timeout) {
                if (SettingsManager::GetUpdateVOIP()) {
                    for (int i=0; i<g->GetNumFrames(); ++i) {
                        GhostFrame & f2 = g->GetFrameByIndex(i);
                        if (!f2.sound_buffers.isEmpty()) {
//                            qDebug() << "Room::Update voip1" << this->GetS("id") << f2.sound_buffers.size();
                            sound_buffers += f2.sound_buffers;
                            f2.sound_buffers.clear();
                            //59.0 - commented out intentionally, play VOIP packets even if player is in another room
                            if (!player_in_room && !player_in_adjacent_room) {
                                sound_buffers.clear();
                            }
                        }
                    }
                }
            }

            //do all the things we want to do only ONCE for each new frame
            if (cur_ghost_frame_index != ghost_frame_index) {
                if (SettingsManager::GetUpdateVOIP()) {
                    if (cur_ghost_frame_index+1 < g->GetNumFrames()) {
                        GhostFrame & f2 = g->GetFrameByIndex(cur_ghost_frame_index+1);
                        if (!f2.sound_buffers.isEmpty()) {
//                            qDebug() << "Room::Update voip2" << this->GetS("id")<< f2.sound_buffers.size();
                            sound_buffers += f2.sound_buffers;
                            //59.0 - commented out intentionally, play VOIP packets even if player is in another room
                            if (!player_in_room && !player_in_adjacent_room) {
                                sound_buffers.clear();
                            }
                        }
                    }
                }
                //                qDebug() << "Got sound buffers" << sound_buffers.size();

                //make the ghost chat
                SetChatMessage(ghost_frame.chat_message);

                //change the ghost's avatar
                if (SettingsManager::GetUpdateCustomAvatars()) {
                    if (!ghost_frame.avatar_data.isEmpty()
                            && QString::compare(ghost_frame.avatar_data.left(13), "<FireBoxRoom>") == 0
                            && QString::compare(GetAvatarCode(), ghost_frame.avatar_data) != 0) {
                        const QString id = GetS("id");
                        const QString js_id = GetS("js_id");
                        const bool loop = GetB("loop");
//                        qDebug() << this << "setting data:" << ghost_frame.avatar_data;
                        LoadGhost(ghost_frame.avatar_data);
                        SetAvatarCode(ghost_frame.avatar_data);

                        //49.50 - Do not let id, js_id, or loop attributes be overwritten
                        SetS("id", id);
                        SetS("js_id", js_id);
                        SetB("loop", loop);
                    }
                }

                //room edits/deletes
                if (!ghost_frame.room_edits.isEmpty()) {
                    RoomObjectEdit e;
                    e.data = ghost_frame.room_edits;
                    e.room = ghost_frame.roomid;
                    e.user = ghost_frame.userid;
                    room_edits_incoming += e;
                }

                if (!ghost_frame.room_deletes.isEmpty()) {
                    RoomObjectEdit e;
                    e.data = ghost_frame.room_deletes;
                    e.room = ghost_frame.roomid;
                    e.user = ghost_frame.userid;
                    room_deletes_incoming += e;
                }

                //send portals
                if (ghost_frame.send_portal_url.length() > 0) {
                    send_portal_url += ghost_frame.send_portal_url;
                    send_portal_jsid += ghost_frame.send_portal_jsid;
                    send_portal_pos += ghost_frame.send_portal_pos;
                    send_portal_fwd += ghost_frame.send_portal_fwd;
                }

                ghost_frame_index = cur_ghost_frame_index;
            }

            //change the ghost's animation and other attributes
            SetS("anim_id", ghost_frame.anim_id);

            //re-orthogonalize view and up dirs (they get unorthogonalized through slerp)
            ghost_frame.head_xform.setColumn(1, ghost_frame.head_xform.column(1).toVector3D().normalized());
            ghost_frame.head_xform.setColumn(2, ghost_frame.head_xform.column(2).toVector3D().normalized());
            ghost_frame.head_xform.setColumn(0, QVector3D::crossProduct(ghost_frame.head_xform.column(1).toVector3D(),
                                                                        ghost_frame.head_xform.column(2).toVector3D()).normalized());
            ghost_frame.head_xform.setColumn(1, QVector3D::crossProduct(ghost_frame.head_xform.column(2).toVector3D(),
                                                                        ghost_frame.head_xform.column(0).toVector3D()).normalized());

            SetV("pos", ghost_frame.pos);
            SetDir(ghost_frame.dir);
            SetHMDType(ghost_frame.hmd_type);
        }

        //modify the transformation of the neck joint based on HMD transform
        QPointer <AssetObject> ghost_body = 0;
        if (ghost_assetobjs.contains(body_id)) {
            ghost_body = ghost_assetobjs[body_id];
        }
        if (ghost_body &&
                ghost_body->GetFinished() &&
                ghost_body->GetGeom() &&
                ghost_body->GetGeom()->GetReady()) {
            const QPointer <Geom> body = ghost_body->GetGeom();
            body->ClearTransforms();
        }

//            const QVector3D s = GetScale();
//            const QVector3D zdir = GetZDir();

//            const float angle = -(90.0f - atan2f(zdir.z(), zdir.x()) * MathUtil::_180_OVER_PI);

//            QMatrix4x4 m0;
////            m0.rotate(angle, 0, 1, 0);
//////            m0.scale(s.x(), s.y(), s.z());
////            m0 *= ghost_frame.head_xform;

//            m0 = ghost_frame.head_xform;
//            m0.rotate(angle, 0, 1, 0);
////            m0.scale(1.0f / s.x(), 1.0f / s.y(), 1.0f / s.z());
//            m0.setColumn(3, QVector4D(m0.column(3).x() / s.x(),
//                         m0.column(3).y() / s.y(),
//                         m0.column(3).z() / s.z(),
//                         1));

////            qDebug() << "xform" << m0;
////            m0.translate(0,5,0);
////            m0.scale(1.0f / s.x(), 1.0f / s.y(), 1.0f / s.z());
//            const QVector3D head_pos = ghost_frame.head_xform.column(3).toVector3D();

//            if (QString::compare(hmd_type, QStringLiteral("vive")) == 0) {
////                qDebug() << "global xform" << m0.column(3);
//                body_fbx->SetGlobalTransform("Head", m0);

//                QMatrix4x4 m1;
//                m1.setToIdentity();

//                m1.rotate(angle, 0, 1, 0);
//                m1.translate(QVector3D(head_pos.x() / s.x(),  (head_pos.y() - 0.55f) / s.y(), head_pos.z() / s.z()));

//                const float head_angle = (90.0f - atan2f(ghost_frame.head_xform.column(2).z(), ghost_frame.head_xform.column(2).x()) * MathUtil::_180_OVER_PI);
//                m1.rotate(head_angle, 0, 1, 0);
//                body_fbx->SetGlobalTransform("Hips", m1);
//            }
//            else {
//                body_fbx->SetRelativeTransform("Head", m0);
//            }

//            QList <QString> finger_names;
//            finger_names.push_back("Thumb");
//            finger_names.push_back("Index");
//            finger_names.push_back("Middle");
//            finger_names.push_back("Ring");
//            finger_names.push_back("Pinky");

//            for (int i=0; i<2; ++i) {
//                LeapHand & hand = ((i == 0) ? ghost_frame.hands.first : ghost_frame.hands.second);
//                if (hand.is_active) {

//                    QMatrix4x4 head_basis;

//                    if ((QString::compare(hmd_type, QStringLiteral("vive")) == 0 ||
//                            QString::compare(hmd_type, QStringLiteral("rift")) == 0)
//                            && !hand.finger_tracking) { //55.2 - override if using Leap Motion (i.e. finger tracking)
////                        head_basis = m0;
//                        //use identity matrix for head basis
//                        //for some reason finger tracking can be true??
//                    }
//                    else {
//                        if (!hand.finger_tracking) {
//                            head_basis.setColumn(3, QVector4D(eye_pos, 1));
//                        }
//                        else {
//                            head_basis = ghost_frame.head_xform;
////                            head_basis.setColumn(3, QVector4D(head_pos + eye_pos, 1));
////                            head_basis.rotate(angle, 0, 1, 0);
//                        }

//                        //56.0 - commented all this arm stuff out until it can be given a proper pass.... bugs out vive and rift with gamepad or non-tracked controllers
//                        if (hand.finger_tracking) {

//                            head_basis = QMatrix4x4();
//                            if (hand.finger_tracking_leap_hmd) {
//                                head_basis.setColumn(3, QVector4D(eye_pos, 1));
//                            }
//                            else {
//                                head_basis.setColumn(3, QVector4D(head_pos + eye_pos, 1));
//                            }

//                            const QString prefix = ((i == 0) ? "Left" : "Right");

//                            QVector3D b0, b1, b2;
//                            b0 = body_fbx->GetGlobalBindPose(prefix+"Arm").column(3).toVector3D();
//                            b1 = body_fbx->GetGlobalBindPose(prefix+"ForeArm").column(3).toVector3D();
//                            b2 = body_fbx->GetGlobalBindPose(prefix+"Hand").column(3).toVector3D();

//                            QVector3D p0, p2;
//                            p0 = body_fbx->GetFinalPose(prefix+"Arm").column(3).toVector3D();
//                            p2 = head_basis * hand.basis.column(3).toVector3D();
//                            p2.setX(p2.x() / s.x());
//                            p2.setY(p2.y() / s.y());
//                            p2.setZ(p2.z() / s.z());

//                            const float l0 = (b1-b0).lengthSquared();
//                            const float l1 = (b2-b1).lengthSquared();

//                            //set for forearm
//                            double a0 = (1.0 - ((l0-l1)) / (p2-p0).lengthSquared()) * 0.5;
//                            QVector3D c0 = p0 * a0 + p2 * (1.0 - a0); //circle centre point
//                            const float r = sqrtf(l0 - (c0-p0).lengthSquared()); //circle radius of intersecting spheres on p0,p2 with rads l0,l1

//                            QVector3D p1;
//                            if (sqrtf(l0) + sqrtf(l1) < (p2-p0).length()) { //in case hands are further than the arms' reaech
//                                p1 = (p0 + p2) * 0.5;
//                            }
//                            else {
//                                p1 = c0 - MathUtil::GetOrthoVec((p2-p0).normalized()) * r;
//                            }

//                            //setup arm joint
//                            QVector3D m00 = (p1 - p0).normalized();
//                            QVector3D m01 = MathUtil::GetOrthoVec(m00);
//                            QVector3D m02 = QVector3D::crossProduct(m00, m01).normalized();
//                            QVector3D m03 = p0;

//                            if (i == 1) {
//                                m00 *= -1.0f;
//                                m02 *= -1.0f;
//                            }

//                            QMatrix4x4 m0;
//                            m0.setColumn(0, m00);
//                            m0.setColumn(1, m01);
//                            m0.setColumn(2, m02);
//                            m0.setColumn(3, m03);
//                            m0.setRow(3, QVector4D(0,0,0,1));

//                            body_fbx->SetGlobalTransform(prefix + "Arm", m0);

//                            //setup forearm joint
//                            QVector3D m10 = (p2 - p1).normalized();
//                            QVector3D m11 = MathUtil::GetOrthoVec(m10);
//                            QVector3D m12 = QVector3D::crossProduct(m10, m11).normalized();
//                            QVector3D m13 = p1;

//                            if (i == 1) {
//                                m10 *= -1.0f;
//                                m11 *= -1.0f;
//                            }

//                            QMatrix4x4 m1;
//                            m1.setColumn(0, m10);
//                            m1.setColumn(1, m11);
//                            m1.setColumn(2, m12);
//                            m1.setColumn(3, m13);
//                            m1.setRow(3, QVector4D(0,0,0,1));

//                            body_fbx->SetGlobalTransform(prefix + "ForeArm", m1);

//                            //set for hand
//                            QMatrix4x4 m = head_basis * hand.basis;
//                            m.data()[12] /= s.x();
//                            m.data()[13] /= s.y();
//                            m.data()[14] /= s.z();
//                            body_fbx->SetGlobalTransform(prefix + "Hand", m);

//                            //do fingers only if it's leap motion
//                            for (int j=0; j<finger_names.size(); ++j) {
//                                for (int k=0; k<4; ++k) {
//                                    QMatrix4x4 m = head_basis * hand.fingers[j][k];
//                                    m.data()[12] /= s.x();
//                                    m.data()[13] /= s.y();
//                                    m.data()[14] /= s.z();
//                                    body_fbx->SetGlobalTransform(prefix+"Hand"+finger_names[j]+QString::number(k), m);
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//        }
    }
    else if (obj_type == "object") {
        if (assetobject && assetobject_anim) {
            QPointer <Geom> body = assetobject->GetGeom();
            QPointer <Geom> body_anim = assetobject_anim->GetGeom();
            if (body && body->GetReady() && body_anim && body_anim->GetReady()) {
                body->SetLinkAnimation(body_anim);
            }
        }

        if (assetobject && assetobject->GetGeom()) {
            assetobject->GetGeom()->SetAnimSpeed(GetF("anim_speed"));
            assetobject->GetGeom()->SetLoop(GetB("loop"));
        }
    }
    else if (obj_type == "link") {
        if (GetB("draw_text") && (GetURL() != portal_last_url || GetTitle() != portal_last_title)) {
    //        qDebug() << "Portal::DrawDecorationsGL() updating text" << object->GetURL() << last_url;
            if (portal_text) {
                portal_text->SetText(QString("<p align=\"center\">") + GetTitle() + QString("<br><font color=\"#0000ff\">") + GetURL() + QString("</font></p>"), false);
            }
            portal_last_url = GetURL();
            portal_last_title = GetTitle();
        }
    }

    UpdateMedia();
}

void RoomObject::UpdateMedia()
{
    //are we playing a streaming sound?  generate a source
    if (!sound_buffers.isEmpty() && openal_stream_source == 0) {
        alGenSources(1, &openal_stream_source);
    }

    const QString obj_type = GetType();

    const bool positional_env = SettingsManager::GetPositionalEnvEnabled();
    const bool positional_voip = SettingsManager::GetPositionalVOIPEnabled();
    const float gain_env = SettingsManager::GetVolumeEnv()/100.0f;
    const float gain_voip = SettingsManager::GetVolumeVOIP()/100.0f;

    if (openal_stream_source > 0) {
        const QVector3D pos = GetPos() + QVector3D(0, 1.6f, 0);
        const QVector3D vel = GetVel();
        const QVector3D dir = GetZDir();
        const QVector3D scale = GetScale();
        const float dist = qMax(scale.x(), qMax(scale.y(), scale.z()));

        if (positional_voip) {
            alSourcei(openal_stream_source, AL_SOURCE_RELATIVE, AL_FALSE);
            alSource3f(openal_stream_source, AL_POSITION, pos.x(), pos.y(), pos.z()); //set source position
            alSource3f(openal_stream_source, AL_VELOCITY, vel.x(), vel.y(), vel.z()); //set source velocity
        }
        else{
            alSourcei(openal_stream_source, AL_SOURCE_RELATIVE, AL_TRUE);
            alSource3f(openal_stream_source, AL_POSITION, 0, 0, 0); //set source position
            alSource3f(openal_stream_source, AL_VELOCITY, 0, 0, 0); //set source velocity
        }

        alSourcei(openal_stream_source, AL_LOOPING, GetB("loop") ? AL_TRUE : AL_FALSE);
        //60.0 - commented these out as they are problematic for VOIP
//        alSourcef(openal_stream_source, AL_PITCH, GetF("pitch"));
//        alSourcef(openal_stream_source, AL_GAIN, 2.0f * gain_voip * GetF("gain"));
//        alSourcef(openal_stream_source, AL_ROLLOFF_FACTOR, 3.0f);
//        alSourcef(openal_stream_source, AL_REFERENCE_DISTANCE, dist);

        if (dir != QVector3D(0,0,1) && positional_voip) {
            alSource3f(openal_stream_source, AL_DIRECTION, dir.x(), dir.y(), dir.z());
            alSourcef(openal_stream_source, AL_CONE_INNER_ANGLE, GetF("inner_angle"));
            alSourcef(openal_stream_source, AL_CONE_OUTER_ANGLE, GetF("outer_angle"));
            alSourcef(openal_stream_source, AL_CONE_OUTER_GAIN, GetF("outer_gain"));
        }
        else if (dir != QVector3D(0,0,1) && !positional_voip){
            alSource3f(openal_stream_source, AL_DIRECTION, 0, 0, 0);
            alSourcef(openal_stream_source, AL_CONE_INNER_ANGLE, 360);
            alSourcef(openal_stream_source, AL_CONE_OUTER_ANGLE, 360);
            alSourcef(openal_stream_source, AL_CONE_OUTER_GAIN, 0);
        }

        // Poll for recoverable buffers
        ALint availBuffers=0; // Buffers to be recovered
        alGetSourcei(openal_stream_source, AL_BUFFERS_PROCESSED, &availBuffers);
        //qDebug() << availBuffers;
        if (availBuffers > 0) {
            QVector<ALuint> buffHolder;
//                buffHolder.reserve(SoundManager::buffer_input_pool.size()); // An array to hold catch the unqueued buffers
            buffHolder.resize(availBuffers); //49.50 crash with audio on Windows
            alSourceUnqueueBuffers(openal_stream_source, availBuffers, buffHolder.data());
            for (int i=0;i<availBuffers;++i) {
                // Push the recovered buffers back on the queue
                SoundManager::buffer_input_queue.push_back(buffHolder[i]);
            }
        }

        if (!sound_buffers.isEmpty()) {

            if (!SoundManager::buffer_input_queue.isEmpty()) { // We just drop the data if no buffers are available

                ALuint myBuff = SoundManager::buffer_input_queue.front();

                SoundManager::buffer_input_queue.pop_front();
                //49.8 bugfix - use the buffer size (which can vary), not a fixed size causing portions of audio not to play
                QByteArray b = AudioUtil::decode(QByteArray::fromBase64(sound_buffers.front()));
                //ghost_frame.current_sound_level = MathUtil::GetSoundLevel(b);

                alBufferData(myBuff, AL_FORMAT_MONO16, b.data(), b.size(), sound_buffers_sample_rate);

                // Queue the buffer
                alSourceQueueBuffers(openal_stream_source, 1, &myBuff);

                // Restart the source if needed
                ALint sState=0;
                alGetSourcei(openal_stream_source, AL_SOURCE_STATE, &sState);
                if (sState != AL_PLAYING) {
                    alSourcePlay(openal_stream_source);
//                    qDebug() << "RoomObject::UpdateMedia() playing" << this->GetS("id") << openal_stream_source;
                }

//                qDebug() << "RoomObject::UpdateMedia() buffer input queue" << this->GetS("id") << sound_buffers.size() << SoundManager::buffer_input_queue.size();

                sound_buffers.pop_front();
            }
        }
        else {
            //If no more buffers in queue, delete source
            ALint queued_buffers = 0;
            alGetSourcei(openal_stream_source, AL_BUFFERS_QUEUED, &queued_buffers);
            //qDebug() << "queued_buffers" << queued_buffers;
            if (queued_buffers == 0)
            {
                //Delete source if not in use
                //qDebug() << "deleting source";
                alSourcei(openal_stream_source, AL_BUFFER, 0);
                alDeleteSources(1, &openal_stream_source);
                openal_stream_source = 0;
            }
        }
    }


    //play a sound if we need to
    if (assetsound) {
        //Update gain, pos, vel, dist, pitch, positional sound
        media_ctx.audio_lock.lock();
        media_ctx.pos = model_matrix_parent.map(GetPos());
        media_ctx.vel = GetVel();
        const QVector3D scale = GetScale();
        media_ctx.dist = qMax(scale.x(), qMax(scale.y(), scale.z()));
        media_ctx.gain = gain_env * GetF("gain");
        media_ctx.pitch = GetF("pitch");
        media_ctx.positional_sound = (positional_env && media_ctx.pos != QVector3D(0,0,0));
        media_ctx.audio_lock.unlock();
    }

    //volume adjustment for videos
    if (assetvideo) {
        //Update gain, pos, vel, dist, pitch, positional sound
        media_ctx.audio_lock.lock();
        media_ctx.pos = model_matrix_parent.map(GetPos());
        media_ctx.vel = GetVel();
        const QVector3D scale = GetScale();
        media_ctx.dist = qMax(scale.x(), qMax(scale.y(), scale.z()));
        media_ctx.gain = gain_env * GetF("gain");
        media_ctx.pitch = GetF("pitch");
        media_ctx.positional_sound = (positional_env && media_ctx.pos != QVector3D(0,0,0));
        media_ctx.audio_lock.unlock();
    }
}

qsreal RoomObject::NextUUID()
{
    return DOMNode::NextUUID();
}

void RoomObject::SetCubemapRadiance(const QPointer <AssetImage> a)
{
    m_cubemap_radiance = a;
}

QPointer <AssetImage> RoomObject::GetCubemapRadiance() const
{
    return m_cubemap_radiance;
}

void RoomObject::SetCubemapIrradiance(const QPointer <AssetImage> a)
{
   m_cubemap_irradiance = a;
}

QPointer <AssetImage> RoomObject::GetCubemapIrradiance() const
{
    return m_cubemap_irradiance;
}

QVector3D RoomObject::GetBBoxMin()
{    
    if (assetobject) {
        return assetobject->GetBBoxMin();
    }
    return QVector3D();
}

QVector3D RoomObject::GetBBoxMax()
{
    if (assetobject) {
        return assetobject->GetBBoxMax();
    }
    return QVector3D();
}

void RoomObject::SetUserIDPos(const QVector3D & p)
{
    userid_pos = p;
}

QVector3D RoomObject::GetUserIDPos() const
{
    return userid_pos;
}

void RoomObject::SetHMDType(const QString & s)
{
    hmd_type = s;
}

QString RoomObject::GetHMDType() const
{
    return hmd_type;
}

void RoomObject::SetHeadAvatarPos(const QVector3D & p)
{
    head_avatar_pos = p;
}

QVector3D RoomObject::GetHeadAvatarPos() const
{
    return head_avatar_pos;
}

void RoomObject::SetGhostAssetShaders(const QHash <QString, QPointer <AssetShader> > & a)
{
    ghost_asset_shaders = a;
}

QHash <QString, QPointer <AssetShader> > RoomObject::GetGhostAssetShaders()
{
    return ghost_asset_shaders;
}

void RoomObject::SetGhostAssetObjects(const QHash <QString, QPointer <AssetObject> > & a)
{
//    qDebug() << "RoomObject::SetGhostAssetObjects()" << a;
    ghost_assetobjs = a;
}

QHash <QString, QPointer <AssetObject> > RoomObject::GetGhostAssetObjects()
{
    //59.3 - bugfix for child object AssetObjects appearing in userid.txt file
    //Note!  This function takes the ghost_assetobjects member at root, but only the assetobject of child_objects
    QHash <QString, QPointer <AssetObject> > gas = ghost_assetobjs;
    QList <QPointer <RoomObject> > co = child_objects;

    while (!co.isEmpty()) {
        QPointer <RoomObject> o = co.first();
        co.pop_front();

        const QPointer <AssetObject> a = o->GetAssetObject();
        if (a) {
            gas[a->GetS("id")] = a;
        }
        //add this object's children to list to iterate over
        co += o->GetChildObjects();
    }

    return gas;
}

void RoomObject::SetChildObjects(QList <QPointer <RoomObject> > & e)
{
    child_objects = e;
}

QList <QPointer <RoomObject> > & RoomObject::GetChildObjects()
{
    return child_objects;
}

void RoomObject::SetAvatarCode(const QString & s)
{
    avatar_code = s;
}

QString RoomObject::GetAvatarCode() const
{
    return avatar_code;
}

void RoomObject::SetSelected(const bool b)
{
    selected = b;

    for (int i=0; i<child_objects.size(); ++i) {
        if (child_objects[i]) {
            child_objects[i]->SetSelected(b);
        }
    }
}

bool RoomObject::GetSelected() const
{
    return selected;
}

void RoomObject::SetCollisionSet(const QSet <QString> s)
{
    collision_set = s;
}

QSet <QString> RoomObject::GetCollisionSet() const
{
    return collision_set;
}

void RoomObject::SetPlaySounds(const bool b)
{
    play_sounds = b;
}

bool RoomObject::GetPlaySounds() const
{
    return play_sounds;
}

void RoomObject::SetAssetGhost(const QPointer <AssetGhost> a)
{
    assetghost = a;
}

QPointer<AssetGhost> RoomObject::GetAssetGhost()
{
    return assetghost;
}

void RoomObject::SetDoMultiplayerTimeout(const bool b)
{
    do_multiplayer_timeout = b;
}

void RoomObject::SetChatMessage(const QString & s)
{
    if (s.isEmpty()) {
        return;
    }

    QStringList message_list = s.split(" ");
    QList <QString> messages;
    QString each_message;

    //break apart long sentence into smaller pieces
    for (int i=0; i<message_list.size(); ++i) {

        const QString & eachword = message_list[i];

        each_message += eachword + QString(" ");
        if (each_message.length() > 25) {
            messages.push_back(each_message);
            each_message = QString("");
        }
    }
    messages.push_back(each_message);

    //add this message if it's new
    if (textgeom_chatmessages.empty() || QString::compare(textgeom_chatmessages.last()->GetText(), messages.last()) != 0) {

        //add the messages
        for (int i=0; i<messages.size(); ++i) {

            chat_message = s;
            chat_message_time_offsets.push_back(-float(i)*2.0f);

            QTime new_time;
            new_time.start();
            chat_message_times.push_back(new_time);

            QPointer <TextGeom> new_textgeom = new TextGeom();
            new_textgeom->SetLeftJustify(false);
            new_textgeom->SetFixedSize(true, 0.075f);
            new_textgeom->SetText(messages[i], QColor(255,255,255));
            textgeom_chatmessages.push_back(new_textgeom);
        }

        //play a pop sound
        if (play_sounds) {
            SoundManager::Play(SOUND_POP1, false, GetPos()+QVector3D(0,1,0), 1.0f);
        }
    }
}

QString RoomObject::GetChatMessage() const
{
    return chat_message;
}

void RoomObject::SetAssetImage(const QPointer <AssetImage> a)
{
    assetimage = a;
}

QPointer <AssetImage> RoomObject::GetAssetImage()
{
    return assetimage;
}

void RoomObject::SetAssetLightmap(const QPointer <AssetImage> a)
{
    assetimage_lmap = a;
}

QPointer <AssetImage> RoomObject::GetAssetLightmap()
{
    return assetimage_lmap;
}

void RoomObject::SetAssetObject(const QPointer <AssetObject> a)
{
//    qDebug() << "RoomObject::SetAssetObject" << this << a;
    assetobject = a;
}

QPointer <AssetObject> RoomObject::GetAssetObject()
{
    return assetobject;
}

void RoomObject::SetAnimAssetObject(const QPointer <AssetObject> a)
{
    assetobject_anim = a;
}

QPointer <AssetObject> RoomObject::GetAnimAssetObject() const
{
    return assetobject_anim;
}

QPointer<AssetSound> RoomObject::GetAssetSound() const
{
    return assetsound;
}

void RoomObject::SetBlendAssetObject(const int i, const QPointer <AssetObject> a)
{
    //qDebug() << "EnvObject::SetBlend0AssetObject()!" << a->GetID();
    if (i >= 0 && i < 4) {        
        assetobject_blendshapes[i] = a;
//        QPointer <AssetObject> o = assetobject;
//        QPointer <AssetObject> b = assetobject_blendshapes[i];
//        if (o && o->GetGeomOBJ() && b && b->GetGeomOBJ()) {
//            o->GetGeomOBJ()->SetBlendShape(i, b->GetGeomOBJ());
//        }
    }
}

QPointer <AssetObject> RoomObject::GetBlendAssetObject(const int i)
{
    return assetobject_blendshapes[i];
}

void RoomObject::SetCollisionAssetObject(const QPointer <AssetObject> a)
{
    assetobject_collision = a;
}

QPointer <AssetObject> RoomObject::GetCollisionAssetObject()
{
    return assetobject_collision;
}

void RoomObject::SetEmitterAssetObject(const QPointer <AssetObject> a)
{
    assetobject_emitter = a;
}

QPointer <AssetObject> RoomObject::GetEmitterAssetObject()
{
    return assetobject_emitter;
}

void RoomObject::SetTeleportAssetObject(const QPointer <AssetObject> a)
{
    assetobject_teleport = a;
}

QPointer <AssetObject> RoomObject::GetTeleportAssetObject()
{
    return assetobject_teleport;
}

void RoomObject::SetInterpolation()
{
    interp_pos = GetV("pos");
    interp_xdir = GetV("xdir");
    interp_ydir = GetV("ydir");
    interp_zdir = GetV("zdir");
    interp_scale = GetV("scale");

    interp_val = 0.0f;
    interp_timer.restart();

    UpdateMatrices();
}

void RoomObject::Copy(const QPointer <RoomObject> o)
{
    if (o.isNull()) {
        return;
    }

    selected = o->selected;
    tex_width = o->tex_width;
    tex_height = o->tex_height;

    //image
    assetimage = o->assetimage;

    //sound
    assetsound = o->assetsound;    

    //websurface
    assetwebsurface = o->assetwebsurface;

    //video
    assetvideo = o->assetvideo;

    //object
    assetobject = o->assetobject;
    assetobject_collision = o->assetobject_collision;
    assetobject_anim = o->assetobject_anim;
    assetobject_teleport = o->assetobject_teleport;
    assetobject_blendshapes = o->assetobject_blendshapes;
    assetobject_emitter = o->assetobject_emitter;

    assetshader = o->assetshader; //optional shader

    //ghost
    assetghost = o->assetghost;
    time_elapsed = o->time_elapsed;
    playing = o->playing;
    chat_message = o->chat_message;
    chat_message_times = o->chat_message_times;
    head_avatar_pos = o->head_avatar_pos;
    ghost_assetobjs = o->ghost_assetobjs;
    ghost_asset_shaders = o->ghost_asset_shaders;
    do_multiplayer_timeout = o->do_multiplayer_timeout;
    avatar_code = o->avatar_code;
    play_sounds = o->play_sounds;

    child_objects = o->child_objects;

    textgeom_chatmessages = o->textgeom_chatmessages;
    chat_message_time_offsets = o->chat_message_time_offsets;

    error_reported = o->error_reported;

    // Do not set model matrices, always defer to calls to UpdateMatrices();
    save_to_markup = o->save_to_markup;   

    props->Copy(o->props);    
}

void RoomObject::SetV(const char * name, const QVector3D p)
{
    if (props) {
        props->SetV(name, p);
    }
}

QVector3D RoomObject::GetV(const char * name) const
{
    if (props) {
        return props->GetV(name);
    }
    return QVector3D();
}

void RoomObject::SetV4(const char * name, const QVector4D p)
{
    if (props) {
        props->SetV4(name, p);
    }

}

QVector4D RoomObject::GetV4(const char * name) const
{
    if (props) {
        return props->GetV4(name);
    }
    return QVector4D();
}

void RoomObject::SetF(const char * name, const float f)
{
    if (props) {
        props->setProperty(name, QString::number(f));
    }
}

float RoomObject::GetF(const char * name) const
{    
    if (props && props->property(name).isValid()) {
        return props->property(name).toFloat();
    }
    return 0.0f;
}

void RoomObject::SetI(const char * name, const int i)
{
    if (props) {
        props->setProperty(name, QString::number(i));
    }
}

int RoomObject::GetI(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return props->property(name).toInt();
    }
    return 0;
}

void RoomObject::SetB(const char * name, const bool b)
{
    if (props) {
        props->setProperty(name, b ? "true" : "false");
    }
}

bool RoomObject::GetB(const char * name) const
{    
    if (props && props->property(name).isValid()) {
        return props->property(name).toString().toLower() == "true";
    }
    return false;
}

void RoomObject::SetS(const char * name, const QString s)
{
    if (props) {
        props->SetS(name, s);
    }
}

QString RoomObject::GetS(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return props->GetS(name);
    }
    return QString();
}

void RoomObject::SetC(const char * name, const QColor c)
{
    if (props) {
        props->SetC(name, c);
    }
}

QColor RoomObject::GetC(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return props->GetC(name);
    }
    return QColor(255,255,255);
}

QMatrix4x4 RoomObject::GetModelMatrixLocal() const
{
    return model_matrix_local;
}

QMatrix4x4 RoomObject::GetModelMatrixGlobal() const
{
    return model_matrix_global;
}

void RoomObject::DrawLoadingGL(QPointer <AssetShader> shader)
{
    if (anim == NULL) {
        return;
    }

    const bool edit_mode_enabled = SettingsManager::GetEditModeEnabled();
    const bool edit_mode_icons_enabled = SettingsManager::GetEditModeIconsEnabled();

    const QString t = GetType();

    if (t == "object") {
        if (assetobject && !assetobject->GetFinished()) {
            const float value = assetobject->GetProgress();
            SpinAnimation::DrawGL(shader, value, true);
        }
        else { //59.0 - don't draw the cube icon, it interferes with stuff
            if (edit_mode_enabled && edit_mode_icons_enabled && GetS("js_id").left(6) != "__menu") {
                DrawIconGL(shader, object_img);
            }
        }
    }
    else if (t == "video") {
        if (assetvideo) {
            if (assetimage && !assetimage->GetFinished() && !GetPlaying()) {
                const float value = assetimage->GetProgress();
                SpinAnimation::DrawGL(shader, value, true);
            }
        }
    }
    else if (t == "image") {
        if (assetimage && !assetimage->GetFinished()) {
            const float value = assetimage->GetProgress();
            SpinAnimation::DrawGL(shader, value, true);
        }
    }
    else if (t == "particle") {
        if (edit_mode_enabled && edit_mode_icons_enabled && GetS("js_id").left(6) != "__menu") {
            DrawIconGL(shader, particle_img);
        }
    }
}

void RoomObject::UpdateMatrices()
{
    //if any components of the rotation value were updated in JS, modify the xdir/ydir/zdir
    const QVector3D r = GetV("rotation");
    if (r != last_rotation)
    {
        last_rotation = r;
        SetV("rotation", r);
    }

    const QVector3D xd = GetXDir();
    const QVector3D yd = GetYDir();
    const QVector3D zd = GetZDir();
    const QVector3D s = GetScale();
    const QVector3D p = GetPos();
    const QString t = GetType();

    if (t == "ghost") {
        //compute rotation angle for ghost
        GhostFrame frame;
        if (assetghost) {
            assetghost->GetGhostFrame(time_elapsed, frame);
        }

        const float angle = MathUtil::RadToDeg(MathUtil::_PI_OVER_2 - atan2f(frame.dir.z(), frame.dir.x()));
        model_matrix_local.setToIdentity();
        model_matrix_local.translate(p);
        model_matrix_local.rotate(angle, 0, 1, 0);
        model_matrix_local.scale(s);
    }
    else if (t == "text" || t == "paragraph" || t == "image" || t == "video") {
        model_matrix_local.setColumn(0, xd);
        model_matrix_local.setColumn(1, yd);
        model_matrix_local.setColumn(2, zd);
        model_matrix_local.setColumn(3, QVector4D(p, 1));
        model_matrix_local.translate(0,0,portal_spacing);
        model_matrix_local.scale(s);

        if (t == "text") {
            model_matrix_local *= textgeom->GetModelMatrix();
        }
    }
    else if (t == "link") {
        const QVector3D p2 = p + zd * portal_spacing;

        model_matrix_local.setColumn(0, QVector4D(xd.x(), xd.y(), xd.z(), 0.0f));
        model_matrix_local.setColumn(1, QVector4D(yd.x(), yd.y(), yd.z(), 0.0f));
        model_matrix_local.setColumn(2, QVector4D(zd.x(), zd.y(), zd.z(), 0.0f));
        model_matrix_local.setColumn(3, QVector4D(p2.x(), p2.y(), p2.z(), 1.0f));
    }
    else if (t == "particle") {
        //model_matrix_local.setToIdentity();
        if (GetB("emit_local")) {
            model_matrix_local.setColumn(0, xd);
            model_matrix_local.setColumn(1, yd);
            model_matrix_local.setColumn(2, zd);
            model_matrix_local.setColumn(3, QVector4D(p, 1));
        }
        else {
            model_matrix_local.setToIdentity();
        }
    }
    else {
        model_matrix_local.setColumn(0, xd);
        model_matrix_local.setColumn(1, yd);
        model_matrix_local.setColumn(2, zd);
        model_matrix_local.setColumn(3, QVector4D(p, 1));
        model_matrix_local.scale(s);
    }

    if (GetF("rotate_deg_per_sec") != 0.0f) {
        const float t = time_elapsed * GetF("rotate_deg_per_sec");
        const QVector3D sa = GetV("rotate_axis");
        model_matrix_local.rotate(t, sa);
    }

    model_matrix_global = model_matrix_parent * model_matrix_local * MathUtil::ModelMatrix();

//    model_matrix_local.optimize();
//    model_matrix_global.optimize();

//    if (this->GetParentObject() || selected) {
//        qDebug() << "id" << this->GetID()
//                 << "model" << MathUtil::ModelMatrix()
//                 << "local" << model_matrix_local
//                 << "global" << model_matrix_global;
//        qDebug() << "done";
//    }
}

void RoomObject::DrawCursorGL(QPointer <AssetShader> shader)
{
    const bool visible = GetB("visible");
    if (!visible) {
        return;
    }

    const QString t = GetType();

    if (t == "ghost") {
        if (assetghost && assetghost->GetProcessed()) {

            //scalar value that affects rendering/disconnect
            float scale_fac = 1.0f;
            const float disc_time_sec = logoff_rate;
            if (do_multiplayer_timeout && time_elapsed > disc_time_sec) {
                const float time_disconnect = time_elapsed - disc_time_sec;
                scale_fac = qMax(0.0f, 1.0f - time_disconnect*0.25f);
                if (scale_fac <= 0.0f) {
                    return;
                }
            }

            RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::DISABLED);

            //draw ghost's cursor if active
            if (ghost_frame.cursor_active && cursor_arrow_obj) {

                bool use_lighting = shader->GetUseLighting();
                shader->SetUseLighting(false);
                shader->UpdateObjectUniforms();

                MathUtil::PushModelMatrix();
                MathUtil::LoadModelMatrix(ghost_frame.cursor_xform);
                MathUtil::ModelMatrix().translate(0,0,0.01f);
                MathUtil::ModelMatrix().scale(ghost_frame.cscale * scale_fac);

                cursor_arrow_obj->DrawGL(shader, QColor(255,255,255));

                MathUtil::PopModelMatrix();
                shader->SetUseLighting(use_lighting);
            }

            RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::BACK);
        }
    }    
}

void RoomObject::DrawGL(QPointer <AssetShader> shader, const bool left_eye, const QVector3D & player_pos)
{    
//        qDebug() << "RoomObject::DrawGL()" << GetID() << GetJSID() << GetScale() << GetType() << assetobject << assetobject->GetFinished();
    if (shader.isNull() || !shader->GetCompiled() || !GetB("visible")) {
        return;
    }

    const QString t = GetType();
    const QColor col = GetC("col");
//    qDebug() << "draw" << this << col << t;
    const QColor chromakey_col = GetC("chromakey_col");
    const bool edit_mode_enabled = SettingsManager::GetEditModeEnabled();
    const bool edit_mode_icons_enabled = SettingsManager::GetEditModeIconsEnabled();

    const QVector3D pos = GetPos();
//    qDebug() << "RoomObject::DrawGL" << GetS("_type") << GetS("id") << GetV("pos") << GetV("xdir") << GetV("ydir") << GetV("zdir") << GetV("scale");

    const QString cf = GetS("cull_face");
    if (cf == "front") {
        RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::FRONT);
    }
    else if (cf == "none") {
        RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::DISABLED);
    }
    else {
        RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::BACK);        
    }
//    qDebug() << "cull_face"<< this << cf;

    const QString body_id = GetS("body_id");
    const QString head_id = GetS("head_id");

    //get the model matrix (not including the view portion)            
    //    qDebug() << "setting colour" << QVector4D(col.redF(), col.greenF(), col.blueF(), col.alphaF());
    shader->SetAmbient(QVector3D(1,1,1));
    shader->SetDiffuse(QVector3D(1,1,1));
    shader->SetEmission(QVector3D(0,0,0));
    shader->SetSpecular(QVector3D(0.04f,0.04f,0.04f));
    shader->SetShininess(20.0f);
    shader->SetConstColour(QVector4D(col.redF(), col.greenF(), col.blueF(), col.alphaF()));
    shader->SetChromaKeyColour(QVector4D(chromakey_col.redF(), chromakey_col.greenF(), chromakey_col.blueF(), chromakey_col.alphaF()));
    shader->SetUseLighting(GetB("lighting"));
    shader->SetUseSkelAnim(false); //53.10 - don't know why this solves the problem but it does, skeletal animation is disabled after FBX draw call but this is still needed
    shader->SetTiling(GetV4("tile")); //59.13 - what is this?
    shader->SetDrawLayer(GetI("draw_layer"));

//    qDebug() << "draw layer" << this << GetI("draw_layer");

    // This allows us to sort objects based on their bounding spheres in the Ren
    float scale_fac = 1.0f;
    const float size = (GetBBoxMax() - GetBBoxMin()).length() * 0.5f;
    const float distance = qAbs(((pos - player_pos).length() - size));
    // Transform the position to be relative to the current_room origin so that it can be transformed into view_space by each camera's viewmatrix
    QVector4D position(pos, 1.0f);
    position = MathUtil::RoomMatrix() * position;

    shader->SetRoomSpacePositionAndDistance(QVector4D(position.x(), position.y(), position.z(), distance));

    //do the draw
    if (t == "particle") {
        bool override_texture = false;

        shader->SetUseTextureAll(false);        

        if (assetimage && assetimage->GetFinished()) {
            auto tex_id = assetimage->GetTextureHandle(left_eye);
            RendererInterface::m_pimpl->BindTextureHandle(0, tex_id);
            override_texture = true;

			shader->SetUseTexture(0, true);
        }

        if (assetobject.isNull()) {
            RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::DISABLED);
            RendererInterface::m_pimpl->SetDepthMask(DepthMask::DEPTH_WRITES_DISABLED);
        }

        MathUtil::PushModelMatrix();
        MathUtil::MultModelMatrix(model_matrix_local);
        particle_system->DrawGL(shader, player_pos, assetobject, override_texture);
        MathUtil::PopModelMatrix();

#ifndef __ANDROID__
        if (selected && edit_mode_enabled) {
            shader->SetConstColour(QVector4D(0.5f, 1.0f, 0.5f, 1.0f));
            shader->SetChromaKeyColour(QVector4D(0.0f, 0.0f, 0.0f, 0.0f));
            shader->SetUseTextureAll(false);
            shader->SetUseLighting(false);
            shader->SetAmbient(QVector3D(1.0f, 1.0f, 1.0f));
            shader->SetDiffuse(QVector3D(1.0f, 1.0f, 1.0f));
            shader->SetSpecular(QVector3D(0.04f, 0.04f, 0.04f));
            shader->SetShininess(20.0f);

            RendererInterface::m_pimpl->SetPolyMode(PolyMode::LINE);
            particle_system->DrawGL(shader, player_pos, assetobject, true);

            shader->SetConstColour(QVector4D(1,1,1,1));

            RendererInterface::m_pimpl->SetPolyMode(PolyMode::FILL);
        }
#endif

        if (assetobject.isNull()) {
            RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::BACK);
            RendererInterface::m_pimpl->SetDepthMask(DepthMask::DEPTH_WRITES_ENABLED);
        }
    }
    else if (t == "text") {
        RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::DISABLED);
        textgeom->SetColour(col);
        MathUtil::PushModelMatrix();
        MathUtil::MultModelMatrix(model_matrix_local);        
        textgeom->DrawGL(shader);

#ifndef __ANDROID__
        if (selected && edit_mode_enabled) {
            shader->SetConstColour(QVector4D(0.5f, 1.0f, 0.5f, 1.0f));
            shader->SetChromaKeyColour(QVector4D(0.0f, 0.0f, 0.0f, 0.0f));
            shader->SetUseTextureAll(false);
            shader->SetUseLighting(false);
            shader->SetAmbient(QVector3D(1.0f, 1.0f, 1.0f));
            shader->SetDiffuse(QVector3D(1.0f, 1.0f, 1.0f));
            shader->SetSpecular(QVector3D(0.04f, 0.04f, 0.04f));
            shader->SetShininess(20.0f);

            RendererInterface::m_pimpl->SetPolyMode(PolyMode::LINE);
            textgeom->DrawSelectedGL(shader);

            shader->SetConstColour(QVector4D(1,1,1,1));

            RendererInterface::m_pimpl->SetPolyMode(PolyMode::FILL);
        }
#endif

        MathUtil::PopModelMatrix();
        RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::BACK);
    }
    else if (t == "paragraph") {

        if (assetimage) {
            assetimage->UpdateGL();
        }
        if (assetimage && assetimage->GetFinished()) {
            shader->SetUseTextureAll(false);
            shader->SetUseTexture(0, true);

            const float iw = 1.0f;
            const float ih = iw * assetimage->GetAspectRatio();

            MathUtil::PushModelMatrix();
            MathUtil::MultModelMatrix(model_matrix_local);
            MathUtil::ModelMatrix().scale(iw, ih, 1.0f);

            auto tex_id = assetimage->GetTextureHandle(true);
            RendererInterface::m_pimpl->BindTextureHandle(0, tex_id);

            shader->UpdateObjectUniforms();

            RendererInterface * renderer = RendererInterface::m_pimpl;
            AbstractRenderCommand a(PrimitiveType::TRIANGLES,
                                    renderer->GetPlanePrimCount(),
                                    1,
                                    0,
                                    0,
                                    0,
                                    renderer->GetPlaneVAO().get(),
                                    shader->GetProgramHandle(),
                                    shader->GetFrameUniforms(),
                                    shader->GetRoomUniforms(),
                                    shader->GetObjectUniforms(),
                                    shader->GetMaterialUniforms(),
                                    renderer->GetCurrentlyBoundTextures(),
                                    renderer->GetDefaultFaceCullMode(),
                                    renderer->GetDepthFunc(),
                                    renderer->GetDepthMask(),
                                    renderer->GetStencilFunc(),
                                    renderer->GetStencilOp(),
                                    renderer->GetPolyMode(),
                                    renderer->GetColorMask());
            renderer->PushAbstractRenderCommand(a);

            MathUtil::PopModelMatrix();
        }
    }
    else if (t == "ghost") {

        if (ghost_assetobjs.contains(head_id) && ghost_assetobjs[head_id]) {

        }        
        if (assetghost && assetghost->GetProcessed()) {

            //scalar value that affects rendering/disconnect
            const float disc_time_sec = logoff_rate;
            if (do_multiplayer_timeout && time_elapsed > disc_time_sec) {
                const float time_disconnect = time_elapsed - disc_time_sec;
                scale_fac = qMin(1.0f, qMax(0.0f, 1.0f - time_disconnect*0.25f));
            }

            if (scale_fac > 0.0f) {

                MathUtil::PushModelMatrix();
                MathUtil::MultModelMatrix(model_matrix_local);
                MathUtil::ModelMatrix().scale(scale_fac, scale_fac, scale_fac);

                const QVector3D z = GetZDir();
                const QVector3D s = GetScale();
                const float angle = 90.0f - atan2f(z.z(), z.x()) * MathUtil::_180_OVER_PI;

                //draw body
                if (SettingsManager::GetUpdateCustomAvatars()) {
                    if (!body_id.isEmpty()) {
                        //enter here if body_id is empty, this allows blank id's to be "nodraw" for this part
                        if (ghost_assetobjs.contains(body_id) && ghost_assetobjs[body_id]) {
                            ghost_assetobjs[body_id]->Update();
                            ghost_assetobjs[body_id]->UpdateGL();

                            if (ghost_assetobjs[body_id]->GetFinished()) {
                                ghost_assetobjs[body_id]->DrawGL(shader, col);
                            }
                        }
                        else if (avatar_obj && avatar_obj->GetFinished()) {
                            avatar_obj->DrawGL(shader, col);
                        }
                    }
                }
                else if (avatar_obj && avatar_obj->GetFinished()) {
                    avatar_obj->DrawGL(shader, col);
                }

                //draw head
                MathUtil::PushModelMatrix();
                MathUtil::ModelMatrix().rotate(-angle, 0, 1, 0);

                QMatrix4x4 m = ghost_frame.head_xform;
                m.setColumn(3, QVector4D(head_avatar_pos, 1));
                MathUtil::MultModelMatrix(m);
                MathUtil::ModelMatrix().translate(-head_avatar_pos.x(), -head_avatar_pos.y(), -head_avatar_pos.z());

                if (SettingsManager::GetUpdateCustomAvatars()) {
                    if (!head_id.isEmpty()) {
                        //enter here if head_id is empty, this allows blank id's to be "nodraw" for this part
                        if (ghost_assetobjs.contains(head_id) && ghost_assetobjs[head_id]) {
                            ghost_assetobjs[head_id]->Update();
                            ghost_assetobjs[head_id]->UpdateGL();

                            if (ghost_assetobjs[head_id]->GetFinished()) {
                                ghost_assetobjs[head_id]->DrawGL(shader, col);
                            }
                        }
                        else if (avatar_head_obj && avatar_head_obj->GetFinished()) {
                            avatar_head_obj->DrawGL(shader, col);
                        }
                    }
                }
                MathUtil::PopModelMatrix();

                RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::DISABLED);

                DrawGhostUserIDChat(shader);

                RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::BACK);

                //draw controllers (if present)
                if (ghost_frame.hands.first.is_active || ghost_frame.hands.second.is_active) {
                    MathUtil::PushModelMatrix();
                    MathUtil::ModelMatrix().rotate(180,0,1,0);
                    MathUtil::ModelMatrix().scale(1.0f/s.x(), 1.0f/s.y(), 1.0f/s.z());

    //                qDebug() << MathUtil::ModelMatrix() << ghost_frame.hands.first.basis << ghost_frame.hands.second.basis;
                    if (ghost_frame.hands.first.is_active) {
                        ControllerManager::DrawGhostGL(shader, 0, ghost_frame.hmd_type, ghost_frame.hands.first.basis);
                    }
                    if (ghost_frame.hands.second.is_active) {
                        ControllerManager::DrawGhostGL(shader, 1, ghost_frame.hmd_type, ghost_frame.hands.second.basis);
                    }

                    MathUtil::PopModelMatrix();
                }

                MathUtil::PopModelMatrix();
            }
        }
    }
    else if (t == "video") {
        if (assetvideo) {

            MathUtil::PushModelMatrix();
            MathUtil::MultModelMatrix(model_matrix_local);

#ifndef __ANDROID__
            if (selected && edit_mode_enabled) {
                MathUtil::PushModelMatrix();
                if (assetimage && assetimage->GetFinished() && !assetvideo->GetPlaying(&media_ctx) && !GetPlaying() && !assetvideo->GetCursorActive(&media_ctx)) {
                    MathUtil::ModelMatrix().scale(1.0f, assetimage->GetAspectRatio(), 1.0f);
                    assetimage->DrawSelectedGL(shader);
                }
                else {
                    MathUtil::ModelMatrix().scale(1.0f, assetvideo->GetAspectRatio(&media_ctx), 1.0f);
                    assetvideo->DrawSelectedGL(shader);
                }
                MathUtil::PopModelMatrix();
            }
#endif

            if (assetimage && assetimage->GetFinished() && !assetvideo->GetPlaying(&media_ctx) && !GetPlaying() && !assetvideo->GetCursorActive(&media_ctx)) {
                shader->SetUseTexture(0, true);
                assetimage->DrawGL(shader, left_eye);
            }
            else {
                //shader->SetUseTexture(0, true);
                float aspect = 0.7f;
                if (assetimage && assetimage->GetFinished()) {
                    aspect = assetimage->GetAspectRatio();
                }
                else {
                    aspect = assetvideo->GetAspectRatio(&media_ctx);
                }
                assetvideo->DrawGL(&media_ctx, shader, left_eye, aspect);
            }

            MathUtil::PopModelMatrix();
        }
    }
    else if (t == "image") {
        if (assetimage && assetimage->GetFinished()) {
            MathUtil::PushModelMatrix();
            MathUtil::MultModelMatrix(model_matrix_local);

#ifndef __ANDROID__
            if (selected && edit_mode_enabled) {
                MathUtil::PushModelMatrix();
                MathUtil::ModelMatrix().scale(1.0f, assetimage->GetAspectRatio(), 1.0f);
                assetimage->DrawSelectedGL(shader);
                MathUtil::PopModelMatrix();
            }
#endif

            shader->SetUseTexture(0, true);
            shader->SetUseLighting(GetB("lighting"));
            assetimage->DrawGL(shader, left_eye);

            MathUtil::PopModelMatrix();
        }       
    }
    else if (t == "light") {
        if (edit_mode_icons_enabled) {
            DrawIconGL(shader, light_img);
        }
    }
    else if (t == "sound") {
        if (edit_mode_icons_enabled) {
            DrawIconGL(shader, sound_img);
        }
    }
    else {
        bool override_texture = false;
		bool is_hdr = false;
        bool is_flipped = false;

        QPointer <AssetObject> obj = assetobject;
        QPointer <AbstractWebSurface> aw = assetwebsurface;
        QPointer <AssetImage> ai = assetimage;
        QPointer <AssetVideo> av = assetvideo;

        if (aw && (aw->GetFocus() || ai.isNull())) {
            RendererInterface::m_pimpl->BindTextureHandle(0, aw->GetTextureHandle());
            override_texture = true;
        }
        else if (av && av->GetPlaying(&media_ctx) && av->GetTextureHandle(&media_ctx, left_eye)
                 && av->GetTextureHandle(&media_ctx, left_eye) != AssetImage::null_image_tex_handle.get()) {
            RendererInterface::m_pimpl->BindTextureHandle(0, av->GetTextureHandle(&media_ctx, left_eye));
            override_texture = true;
            is_flipped = false;
        }
        else if (ai && ai->GetFinished()) {
            auto tex_id = ai->GetTextureHandle(left_eye);
            RendererInterface::m_pimpl->BindTextureHandle(0, tex_id);
            override_texture = true;
            is_hdr = ai->GetIsHDR();
        }

        shader->SetUseTextureAll(false);
        shader->SetUseTexture(0, override_texture, is_hdr, is_flipped);

        if (m_cubemap_radiance && m_cubemap_radiance->GetFinished()) {
            RendererInterface::m_pimpl->BindTextureHandle(11, m_cubemap_radiance->GetTextureHandle(left_eye));
            shader->SetUseCubeTexture1(true);
        }

        if (m_cubemap_irradiance && m_cubemap_irradiance->GetFinished()) {
            RendererInterface::m_pimpl->BindTextureHandle(12, m_cubemap_irradiance->GetTextureHandle(left_eye));
            shader->SetUseCubeTexture2(true);
        }

        if (assetimage_lmap && assetimage_lmap->GetFinished())
        {
            RendererInterface::m_pimpl->BindTextureHandle(8, assetimage_lmap->GetTextureHandle(left_eye));
            shader->SetUseTexture(8, true, assetimage_lmap->GetIsHDR());
            QVector4D lmapScale = GetV4("lmap_scale");
            shader->SetLightmapScale(lmapScale);
            override_texture = true;
        }

        MathUtil::PushModelMatrix();
        MathUtil::MultModelMatrix(model_matrix_local);

        if (obj && obj->GetFinished()) {
            obj->DrawGL(shader, col, override_texture);
        }

        if (assetobject_teleport && draw_assetobject_teleport && assetobject_teleport->GetFinished()) {
            assetobject_teleport->DrawGL(shader, col, override_texture);
        }

        if (override_texture) {
            RendererInterface::m_pimpl->BindTextureHandle(0, AssetImage::null_image_tex_handle.get());
        }
        shader->SetUseTextureAll(false);

#ifndef __ANDROID__
        if (obj && selected && edit_mode_enabled) {
            shader->SetChromaKeyColour(QVector4D(0.0f, 0.0f, 0.0f, 0.0f));
            shader->SetUseLighting(false);
            shader->SetAmbient(QVector3D(1.0f, 1.0f, 1.0f));
            shader->SetDiffuse(QVector3D(1.0f, 1.0f, 1.0f));
            shader->SetSpecular(QVector3D(0.04f, 0.04f, 0.04f));
            shader->SetShininess(20.0f);

            RendererInterface::m_pimpl->SetPolyMode(PolyMode::LINE);
            obj->DrawGL(shader, QColor(128, 255, 128, 255), true);

            if (assetobject_collision && assetobject_collision->GetFinished()) {
                const QVector3D p = GetV("collision_pos");
                const QVector3D s = GetV("collision_scale");

                MathUtil::PushModelMatrix();
                MathUtil::ModelMatrix().translate(p);
                MathUtil::ModelMatrix().scale(s);
                assetobject_collision->DrawGL(shader, QColor(255, 0, 0, 255), true);
                MathUtil::PopModelMatrix();
            }

            shader->SetConstColour(QVector4D(1,1,1,1));

            RendererInterface::m_pimpl->SetPolyMode(PolyMode::FILL);
        }
#endif
        MathUtil::PopModelMatrix();       
    }    

    //draw child objects, and if ghost, don't continue to draw child objects if faded away
    if (!child_objects.isEmpty() && scale_fac > 0.0f) {
        MathUtil::PushModelMatrix();
        MathUtil::MultModelMatrix(model_matrix_local);
        MathUtil::ModelMatrix().scale(scale_fac);

        for (int i=0; i<child_objects.size(); ++i) {
            if (child_objects[i]) {

                QPointer <AssetObject> a;
                if (ghost_assetobjs.contains(body_id) && ghost_assetobjs[body_id]) {
                    a = ghost_assetobjs[body_id];
                }
                else {
                    a = assetobject;
                }

                QPointer <AssetShader> s = shader;
                if (child_objects[i]->GetAssetShader()) {
                    s = child_objects[i]->GetAssetShader();
                }

                //apply a secondary bone xform
                const QString bone = child_objects[i]->GetS("bone_id");
                if (a && a->GetGeom() && bone.length() > 0) {
                    const QMatrix4x4 bone_xform = a->GetGeom()->GetFinalPose(bone);

                    MathUtil::PushModelMatrix();
                    MathUtil::MultModelMatrix(bone_xform);
                    child_objects[i]->DrawGL(s, left_eye, player_pos);
                    MathUtil::PopModelMatrix();
                }
                else {
                    child_objects[i]->DrawGL(s, left_eye, player_pos);
                }
            }
        }
        MathUtil::PopModelMatrix();
    }

    //56.0 - Karan's request: do not use clip plane on loading icons
    const bool clip_plane = shader->GetUseClipPlane();
    shader->SetUseClipPlane(false);
    MathUtil::PushModelMatrix();
    MathUtil::MultModelMatrix(model_matrix_local);
    DrawLoadingGL(shader);
    MathUtil::PopModelMatrix();
    shader->SetUseClipPlane(clip_plane);

    RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::BACK);

    shader->SetUseSkelAnim(false);
    shader->SetConstColour(QVector4D(1,1,1,1));
    shader->SetChromaKeyColour(QVector4D(0,0,0,0));
    shader->SetUseTextureAll(false);
}

void RoomObject::DrawIconGL(QPointer <AssetShader> shader, const QPointer <AssetImage> img)
{
    if (img) {
        MathUtil::PushModelMatrix();
        //MathUtil::MultModelMatrix(model_matrix_local);
        MathUtil::ModelMatrix().translate(GetPos());

        auto tex_id = img->GetTextureHandle(true);
        anim->DrawIconGL(shader, true, tex_id, QColor(255,255,255));

        if (selected) {
            RendererInterface::m_pimpl->SetPolyMode(PolyMode::LINE);
            anim->DrawIconGL(shader, true, 0, QColor(128,255,128));

            shader->SetConstColour(QVector4D(1,1,1,1));
            RendererInterface::m_pimpl->SetPolyMode(PolyMode::FILL);
        }

        MathUtil::PopModelMatrix();
    }
}

void RoomObject::DrawCollisionModelGL(QPointer <AssetShader> shader)
{
    if (shader == NULL || !shader->GetCompiled()) {
        return;
    }

    if (assetobject_collision.isNull() || !assetobject_collision->GetFinished()) {
        return;
    }

    const QString t = GetType();
    if (t == "object") {
        shader->SetConstColour(QVector4D(1,0.5f,0.5f,1));
        RendererInterface::m_pimpl->SetPolyMode(PolyMode::LINE);

        MathUtil::PushModelMatrix();
        MathUtil::MultModelMatrix(model_matrix_local);
        assetobject_collision->DrawGL(shader);
        MathUtil::PopModelMatrix();

        shader->SetConstColour(QVector4D(1,1,1,1));

        RendererInterface::m_pimpl->SetPolyMode(PolyMode::FILL);
    }          
}

void RoomObject::SetText(const QString & s)
{
    SetText(s, true);
}

void RoomObject::SetText(const QString & s, const bool add_markup)
{
//    qDebug() << "EnvObject::SetText" << s;
    SetS("text", s);

    const QString t = GetType();

    if (t == "text") {
        textgeom->SetText(s, GetC("col"));
        textgeom->SetMaxSize(1.0f, 1.0f);
    }
    else if (t == "paragraph") {
        if (assetimage.isNull()) {
            assetimage = new AssetImage();
        }
        assetimage->CreateFromText(s, GetI("font_size"), add_markup, GetC("text_color"), GetC("back_color"), GetF("back_alpha"), tex_width, tex_height, false);
    }
}

QString RoomObject::GetText() const
{
    return GetS("text");
}

void RoomObject::SetFixedSize(const bool fixed_size, const float size) {
    textgeom->SetFixedSize(fixed_size, size);
}

void RoomObject::SetAssetSound(const QPointer <AssetSound> a)
{
    if (a != assetsound) {
        if (assetsound) {
            assetsound->Stop(&media_ctx);
        }
        assetsound = a;
        media_ctx.setup = false;
    }
}

void RoomObject::SetAssetVideo(const QPointer <AssetVideo> a)
{
    if (a != assetvideo) {
        if (assetvideo) {
            assetvideo->Stop(&media_ctx);
        }
        assetvideo = a;
        media_ctx.setup = false;
    }
}

QPointer <AssetVideo> RoomObject::GetAssetVideo()
{
    return assetvideo;
}

void RoomObject::SetAssetWebSurface(const QPointer <AbstractWebSurface> a)
{
    assetwebsurface = a;
}

QPointer <AbstractWebSurface> RoomObject::GetAssetWebSurface()
{
    return assetwebsurface;
}

void RoomObject::SetAssetShader(const QPointer <AssetShader> a)
{
    assetshader = a;
}

QPointer <AssetShader> RoomObject::GetAssetShader()
{
    return assetshader;
}

bool RoomObject::TestPlayerPosition(const QPointF & p)
{
    const QRectF trigger_rect = MathUtil::GetStringAsRect(GetS("rect"));
    if (!GetB("_triggered") && GetType() == "sound" && assetsound && trigger_rect.contains(p)) {
        SetB("_triggered", true);
        return true;
    }
    return false;
}

void RoomObject::Seek(const float f)
{
    if (GetType() == "sound" && assetsound) {
        assetsound->Seek(&media_ctx, f);
    }
}

void RoomObject::Pause()
{
    if (GetType() == "sound" && assetsound) {
        assetsound->Pause(&media_ctx);
    }
}

void RoomObject::Play()
{
//    qDebug() << "RoomObject::Play()" << this->GetID() << this->GetJSID() << this->GetPos();
    const QString t = GetType();
    if (t == "sound" || t == "link" || t == "object") {
        if (assetsound) {
            assetsound->Play(&media_ctx);
            playing = true;
        }
    }
    else if (t == "ghost"){
        playing = true;

        //if not on a loop and finished, restart it
        if (!GetB("loop") && assetghost) {
            const bool sequence_end = assetghost->GetGhostFrame(time_elapsed, ghost_frame);
            if (sequence_end) {
                time_elapsed = 0.0f;
            }
        }
    }    
    else if (t == "video") {
        if (assetvideo) {
            assetvideo->Play(&media_ctx);
        }    
    }

}

void RoomObject::Stop()
{    
    const QString t = GetType();
    if (t == "sound") {
        if (assetsound) {
            assetsound->Stop(&media_ctx);
        }
    }
    else if (t == "video") {
        if (assetvideo) {
            assetvideo->Stop(&media_ctx);
        }
    }
    else if (t == "ghost") {
        playing = false;    
    }
}

bool RoomObject::GetPlaying() const
{
    const QString t = GetType();
    if (t == "sound") {
        if (assetsound) {
            return assetsound->GetPlaying((MediaContext *) &media_ctx);
        }
    }
    else if (t == "video") {
        if (assetvideo) {
            return assetvideo->GetPlaying((MediaContext *) &media_ctx);
        }
    }
    else if (t == "ghost") {
        return playing;    
    }

    return false;
}

void RoomObject::ReadXMLCode(const QString & str)
{
    QXmlStreamReader reader(str);

    while (!reader.atEnd()) {

        reader.readNext();

        QString tag_name = reader.name().toString().toLower();
        QXmlStreamAttributes attributes = reader.attributes();

        //qDebug() << reader.tokenString() << reader.text() << reader.name();
        if (!tag_name.isEmpty() && reader.tokenType() == QXmlStreamReader::StartElement) {

            for (int i=0; i<attributes.size(); ++i) {
                props->setProperty(attributes[i].name().toString().toLower().trimmed().toLatin1().data(),
                                   attributes[i].value().toString().trimmed());
            }
            SetType(tag_name);

            if (tag_name == "text") {
                reader.readNext();
                SetText(reader.text().toString());
            }
            else if (tag_name == "paragraph") {
                reader.readNext();
                SetText(reader.text().toString());
            }
        }
    }
}

QVariantMap RoomObject::GetJSONCode(const bool show_defaults) const
{
    QVariantMap m;
    QMap <QString, QVariantList> elementlistmap;   

    QList <QByteArray> p = props->dynamicPropertyNames();
    for (int i=0; i<p.size(); ++i) {
        if (props->GetSaveAttribute(p[i].data(), show_defaults)) {
            m[p[i]] = props->GetS(p[i]);
        }
    }

    //add text stuff if there is any in the middle
    const QString t = GetType();
    if (t == "text" || t == "paragraph") {
        m["innertext"] = GetText();        
    }

    //We have to add this node's children
    if (!child_objects.empty()) {
        //Add in all my child's tags recursively
        for (int i=0; i<child_objects.size(); ++i) {
            if (child_objects[i]) {
                elementlistmap[child_objects[i]->GetType()].push_back(child_objects[i]->GetJSONCode(show_defaults));
            }
        }
    }

    QMap <QString, QVariantList>::const_iterator ele_cit;
    for (ele_cit = elementlistmap.begin(); ele_cit != elementlistmap.end(); ++ele_cit) {
        m.insert(ele_cit.key(), ele_cit.value());
    }

    return m;
}

QString RoomObject::GetXMLCode(const bool show_defaults) const
{        
    const QString t = GetType();
    const QString tagname = GetTagName();

    QString code_str = QString("<") + tagname;

    if (props) {
        QList <QByteArray> p = props->dynamicPropertyNames();

        QList <QByteArray> list_order;
        list_order.push_back("id");
        list_order.push_back("js_id");
        list_order.push_back("pos");
        list_order.push_back("xdir");
        list_order.push_back("ydir");
        list_order.push_back("zdir");
        list_order.push_back("scale");
        list_order.push_back("col");
        list_order.push_back("collision_id");
        list_order.push_back("collision_scale");
        list_order.push_back("lighting");
        list_order.push_back("cull_face");
        list_order.push_back("draw_order");

        //60.0 - ensure specific ordering for a few key attributes
        while (!list_order.isEmpty()) {
            const QByteArray s = list_order.last();
            list_order.pop_back();
            if (p.contains(s)) {
                p.removeAll(s);
                p.push_front(s);
            }
        }

        for (int i=0; i<p.size(); ++i) {
            const bool save_attrib = props->GetSaveAttribute(p[i].data(), show_defaults) ;
    //        qDebug() << "attrib" << p[i] << save_attrib;
            if (save_attrib) {
                code_str += QString(" ") + QString(p[i]) + "=\"" + props->GetS(p[i]) + "\"";
            }
        }
    }

//    qDebug() << "RoomObject::GetXMLCode" << this << code_str;

    //add text stuff if there is any in the middle
    if (t == "text") {
        code_str += ">";
        code_str += GetText();
    }
    else if (t == "paragraph") {
        code_str += ">";
        code_str += GetText();        
    }

    //now close it off
    if (child_objects.empty()) {
        if (t == "text") {
            code_str += QString("</text>");
        }
        else if (t == "paragraph") {
            code_str += "</paragraph>";
        }
        else {
            code_str += " />";            
        }
    }
    else {
        code_str += " >\n";
        //Add in all my child's tags recursively
        for (int i=0; i<child_objects.size(); ++i) {
            if (child_objects[i]) {
                code_str += child_objects[i]->GetXMLCode(show_defaults) + "\n";
            }
        }

        //closing tag
        code_str += QString("</") + tagname + QString(">");
    }

    return code_str;
}

void RoomObject::SetURL(const QString & base, const QString & s)
{
//    qDebug() << "RoomObject::SetURL" << this << base << s;
    SetS("url", s);

    //do extra stuff - if this is a link, has an attached websurface, etc.
    if (GetType() == "link") {
        if (QString::compare(s, "workspaces") != 0 && QString::compare(s, "bookmarks") != 0) {
//            qDebug() << "RoomObject::SetURL setting url" << base << s;
            QUrl base_url(base);
            QString resolved_url = QUrl::fromPercentEncoding(base_url.resolved(s).toString().toLatin1());
            SetS("url", resolved_url);
            if (GetS("_original_url").isEmpty()) {
                SetS("_original_url", resolved_url);
            }
        }
        textgeom_url->SetText(ShortenString(GetS("url")));
    }
    else {
        if (GetS("websurface_id").length() > 0 && assetwebsurface && assetwebsurface->GetB("_save_to_markup")) {
            assetwebsurface->SetURL(s);
        }
    }
}

void RoomObject::SetURL(const QString & s)
{
    SetURL("", s);
}

QString RoomObject::GetURL() const
{
    return GetS("url");
}

void RoomObject::SetTitle(const QString & s)
{
    SetS("title", s);
    textgeom_title->SetText(ShortenString(s));
}

QString RoomObject::GetTitle() const
{
    return GetS("title");
}

QPointer <TextGeom> RoomObject::GetTextGeomURL()
{
    return textgeom_url;
}

QPointer <TextGeom> RoomObject::GetTextGeomTitle()
{
    return textgeom_title;
}

void RoomObject::DrawPortalGL(QPointer <AssetShader> shader)
{
    if (shader == NULL || !shader->GetCompiled()) {
        return;
    }

    const float w = GetScale().x() * 0.5f;
    const float h = GetScale().y() * 0.5f;

    //53.9 NOTE!    
    UpdateMatrices();

    MathUtil::PushModelMatrix();
    MathUtil::MultModelMatrix(model_matrix_local);

    MathUtil::ModelMatrix().translate(0,h,0);
    MathUtil::ModelMatrix().scale(w, h, 1.0f);

    shader->SetUseTextureAll(false);    
    shader->SetUseLighting(false);
    shader->SetUseSkelAnim(false); //53.11 - TODO: find out why is this necessary?

    if (selected) {
        shader->SetConstColour(QVector4D(0.5f, 1.0f, 0.5f, 1.0f));

        shader->UpdateObjectUniforms();

        RendererInterface::m_pimpl->SetPolyMode(PolyMode::LINE);
        RendererInterface * renderer = RendererInterface::m_pimpl;
        AbstractRenderCommand a(PrimitiveType::TRIANGLES,
                                renderer->GetTexturedCubePrimCount(),
                                1,
                                0,
                                0,
                                0,
                                renderer->GetTexturedCubeVAO().get(),
                                shader->GetProgramHandle(),
                                shader->GetFrameUniforms(),
                                shader->GetRoomUniforms(),
                                shader->GetObjectUniforms(),
                                shader->GetMaterialUniforms(),
                                renderer->GetCurrentlyBoundTextures(),
                                renderer->GetDefaultFaceCullMode(),
                                renderer->GetDepthFunc(),
                                renderer->GetDepthMask(),
                                renderer->GetStencilFunc(),
                                renderer->GetStencilOp(),
                                renderer->GetPolyMode(),
                                renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a);
        RendererInterface::m_pimpl->SetPolyMode(PolyMode::FILL);
    }

    shader->SetUseTexture(0, true);
    shader->SetConstColour(QVector4D(1,1,1,1));
    shader->SetUseLighting(GetB("lighting"));

    if (!GetB("open")) {
        DrawPortalInsideGL(shader);
    }
    if (GetB("_draw_back")) {
        DrawPortalBackGL(shader);
    }

    MathUtil::PopModelMatrix();
}


void RoomObject::DrawPortalDecorationsGL(QPointer <AssetShader> shader, const float anim_val)
{
    if (shader == NULL || !shader->GetCompiled()) {
        return;
    }

    const QVector3D s = GetScale();
    const float w = s.x() * 0.5f;
    const float h = s.y() * 0.5f;

    MathUtil::PushModelMatrix();
    MathUtil::MultModelMatrix(model_matrix_local);    
    MathUtil::ModelMatrix().translate(0, h, 0);
    MathUtil::ModelMatrix().scale(w, h, 1.0f);
    MathUtil::ModelMatrix().translate(0,0,portal_spacing);

    if (GetB("draw_text") && portal_text && portal_text->GetTextGeomTitle()) {
        shader->SetUseTexture(0, true);
        shader->SetConstColour(QVector4D(1,1,1,1));

        MathUtil::PushModelMatrix();
        MathUtil::ModelMatrix().translate(0, 0.9f, 0);
        MathUtil::ModelMatrix().scale(1.0f, s.x() / s.y(), 1.0f);
        portal_text->DrawGL(shader, true, QVector3D(0,0,0));
        MathUtil::PopModelMatrix();
        shader->SetUseTexture(0, false);
    }

    if (anim_val != 1.0f) {
        MathUtil::ModelMatrix().translate(0,0,0.1f);
        MathUtil::ModelMatrix().scale(1.0f/s.x(), 1.0f/s.y(), 1.0f);       
        SpinAnimation::DrawGL(shader, anim_val, false);

        shader->SetUseTextureAll(false);
    }

    MathUtil::PopModelMatrix();
}

void RoomObject::DrawPortalBackGL(QPointer <AssetShader> shader) const
{
    if (shader == NULL || !shader->GetCompiled()) {
        return;
    }   

    const bool use_thumb = (portal_thumb_img && portal_thumb_img->GetFinished() && !portal_thumb_img->GetError() && portal_thumb_img->GetTextureHandle(true) != nullptr);
    //qDebug() << "RoomObject::DrawPortalBackGL" << this->GetURL() << thumb_image;

    shader->SetUseLighting(false);
    shader->SetUseTextureAll(false);
    shader->SetUseTexture(0, use_thumb);

    const QColor col = GetC("col");
    if (use_thumb) {
        const float max_col = qMax(qMax(col.redF(), col.greenF()), col.blueF());
        shader->SetConstColour(QVector4D(max_col, max_col, max_col, 1));
        RendererInterface::m_pimpl->SetDepthMask(DepthMask::DEPTH_WRITES_ENABLED);
        RendererInterface::m_pimpl->BindTextureHandle(0, portal_thumb_img->GetTextureHandle(true));
    }
    else {
        shader->SetConstColour(QVector4D(col.redF(), col.greenF(), col.blueF(), 0.5f));
    }

    MathUtil::PushModelMatrix();
    MathUtil::ModelMatrix().scale(-1.0f, 1.0f, -1.0f); //scales and does 180 rotation

    shader->UpdateObjectUniforms();
    if (GetB("_circular")) {
        RendererInterface * renderer = RendererInterface::m_pimpl;
        AbstractRenderCommand a(PrimitiveType::TRIANGLE_FAN,
                                renderer->GetDiscPrimCount(),
                                1,
                                0,
                                0,
                                0,
                                renderer->GetDiscVAO().get(),
                                shader->GetProgramHandle(),
                                shader->GetFrameUniforms(),
                                shader->GetRoomUniforms(),
                                shader->GetObjectUniforms(),
                                shader->GetMaterialUniforms(),
                                renderer->GetCurrentlyBoundTextures(),
                                renderer->GetDefaultFaceCullMode(),
                                renderer->GetDepthFunc(),
                                renderer->GetDepthMask(),
                                renderer->GetStencilFunc(),
                                renderer->GetStencilOp(),
                                renderer->GetPolyMode(),
                                renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a);
    }
    else {        
        RendererInterface * renderer = RendererInterface::m_pimpl;
        AbstractRenderCommand a(PrimitiveType::TRIANGLES,
                                renderer->GetPlanePrimCount(),
                                1,
                                0,
                                0,
                                0,
                                renderer->GetPlaneVAO().get(),
                                shader->GetProgramHandle(),
                                shader->GetFrameUniforms(),
                                shader->GetRoomUniforms(),
                                shader->GetObjectUniforms(),
                                shader->GetMaterialUniforms(),
                                renderer->GetCurrentlyBoundTextures(),
                                renderer->GetDefaultFaceCullMode(),
                                renderer->GetDepthFunc(),
                                renderer->GetDepthMask(),
                                renderer->GetStencilFunc(),
                                renderer->GetStencilOp(),
                                renderer->GetPolyMode(),
                                renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a);
    }
    RendererInterface::m_pimpl->SetDepthMask(DepthMask::DEPTH_WRITES_DISABLED);
    MathUtil::PopModelMatrix();
}

void RoomObject::DrawPortalInsideGL(QPointer <AssetShader> shader) const
{
    if (shader == NULL || !shader->GetCompiled()) {
        return;
    }      

    //qDebug() << "EnvObject::DrawPortalInsideGL" << this->GetURL() << thumb_image << GetColour();
    const bool use_thumb = (portal_thumb_img && portal_thumb_img->GetFinished() && !portal_thumb_img->GetError() && portal_thumb_img->GetTextureHandle(true) != nullptr);
    QColor col = GetC("col");

    if (col == QColor(0,0,0,0) || col == QColor(0,0,0,255)) {
        col = QColor(255,255,255,255);
    }
    if (!GetB("_highlighted")) {
        col = col.darker(150);
    }

    shader->SetUseLighting(false);
    shader->SetUseTexture(0, use_thumb);

    if (use_thumb) {
        const float max_col = qMax(qMax(col.redF(), col.greenF()), col.blueF());
        shader->SetConstColour(QVector4D(max_col, max_col, max_col, 1));
        RendererInterface::m_pimpl->SetDepthMask(DepthMask::DEPTH_WRITES_ENABLED);
        RendererInterface::m_pimpl->BindTextureHandle(0, portal_thumb_img->GetTextureHandle(true));
    }
    else {
        shader->SetConstColour(QVector4D(col.redF(), col.greenF(), col.blueF(), 0.5f));
    }

    MathUtil::PushModelMatrix();

    shader->UpdateObjectUniforms();        
    if (GetB("_circular")) {
        RendererInterface * renderer = RendererInterface::m_pimpl;
        AbstractRenderCommand a(PrimitiveType::TRIANGLE_FAN,
                                renderer->GetDiscPrimCount(),
                                1,
                                0,
                                0,
                                0,
                                renderer->GetDiscVAO().get(),
                                shader->GetProgramHandle(),
                                shader->GetFrameUniforms(),
                                shader->GetRoomUniforms(),
                                shader->GetObjectUniforms(),
                                shader->GetMaterialUniforms(),
                                renderer->GetCurrentlyBoundTextures(),
                                renderer->GetDefaultFaceCullMode(),
                                renderer->GetDepthFunc(),
                                renderer->GetDepthMask(),
                                renderer->GetStencilFunc(),
                                renderer->GetStencilOp(),
                                renderer->GetPolyMode(),
                                renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a);
    }
    else {
        RendererInterface * renderer = RendererInterface::m_pimpl;
        AbstractRenderCommand a(PrimitiveType::TRIANGLES,
                                renderer->GetPlanePrimCount(),
                                1,
                                0,
                                0,
                                0,
                                renderer->GetPlaneVAO().get(),
                                shader->GetProgramHandle(),
                                shader->GetFrameUniforms(),
                                shader->GetRoomUniforms(),
                                shader->GetObjectUniforms(),
                                shader->GetMaterialUniforms(),
                                renderer->GetCurrentlyBoundTextures(),
                                renderer->GetDefaultFaceCullMode(),
                                renderer->GetDepthFunc(),
                                renderer->GetDepthMask(),
                                renderer->GetStencilFunc(),
                                renderer->GetStencilOp(),
                                renderer->GetPolyMode(),
                                renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a);
    }
    RendererInterface::m_pimpl->SetDepthMask(DepthMask::DEPTH_WRITES_DISABLED);
    MathUtil::PopModelMatrix();
}

void RoomObject::DrawPortalFrameGL(QPointer <AssetShader> shader)
{
    if (shader == NULL || !shader->GetCompiled()) {
        return;
    }

//    qDebug() << "RoomObject::DrawPortalFrameGL" << linear_gradient_img;
    if (linear_gradient_img.isNull()) {
        return;
    }   

    const QColor col = GetC("col");
    auto tex_id = linear_gradient_img->GetTextureHandle(true);
    RendererInterface::m_pimpl->BindTextureHandle(1, tex_id);

    shader->SetConstColour(QVector4D(col.redF(), col.greenF(), col.blueF(), 1));

    MathUtil::PushModelMatrix();

    if (GetB("_circular")) {
        MathUtil::ModelMatrix().scale(1.0f, 1.0f, !GetB("open") ? 0.2f : 0.1f);
        shader->UpdateObjectUniforms();
        RendererInterface * renderer = RendererInterface::m_pimpl;
        AbstractRenderCommand a(PrimitiveType::TRIANGLES,
                                renderer->GetConePrimCount(),
                                1,
                                0,
                                0,
                                0,
                                renderer->GetConeVAO().get(),
                                shader->GetProgramHandle(),
                                shader->GetFrameUniforms(),
                                shader->GetRoomUniforms(),
                                shader->GetObjectUniforms(),
                                shader->GetMaterialUniforms(),
                                renderer->GetCurrentlyBoundTextures(),
                                renderer->GetDefaultFaceCullMode(),
                                renderer->GetDepthFunc(),
                                renderer->GetDepthMask(),
                                renderer->GetStencilFunc(),
                                renderer->GetStencilOp(),
                                renderer->GetPolyMode(),
                                renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a);

        MathUtil::ModelMatrix().scale(1.0f, 1.0f, 1.5f);
        shader->UpdateObjectUniforms();
        AbstractRenderCommand a2(PrimitiveType::TRIANGLES,
                                 renderer->GetConePrimCount(),
                                 1,
                                 0,
                                 0,
                                 0,
                                 renderer->GetConeVAO().get(),
                                 shader->GetProgramHandle(),
                                 shader->GetFrameUniforms(),
                                 shader->GetRoomUniforms(),
                                 shader->GetObjectUniforms(),
                                 shader->GetMaterialUniforms(),
                                 renderer->GetCurrentlyBoundTextures(),
                                 renderer->GetDefaultFaceCullMode(),
                                 renderer->GetDepthFunc(),
                                 renderer->GetDepthMask(),
                                 renderer->GetStencilFunc(),
                                 renderer->GetStencilOp(),
                                 renderer->GetPolyMode(),
                                 renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a2);

        MathUtil::ModelMatrix().scale(1.0f, 1.0f, 1.5f);
        shader->UpdateObjectUniforms();
        AbstractRenderCommand a3(PrimitiveType::TRIANGLES,
                                 renderer->GetConePrimCount(),
                                 1,
                                 0,
                                 0,
                                 0,
                                 renderer->GetConeVAO().get(),
                                 shader->GetProgramHandle(),
                                 shader->GetFrameUniforms(),
                                 shader->GetRoomUniforms(),
                                 shader->GetObjectUniforms(),
                                 shader->GetMaterialUniforms(),
                                 renderer->GetCurrentlyBoundTextures(),
                                 renderer->GetDefaultFaceCullMode(),
                                 renderer->GetDepthFunc(),
                                 renderer->GetDepthMask(),
                                 renderer->GetStencilFunc(),
                                 renderer->GetStencilOp(),
                                 renderer->GetPolyMode(),
                                 renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a3);
    }
    else {
        MathUtil::ModelMatrix().scale(1.0f, 1.0f, 0.1f);
        shader->UpdateObjectUniforms();
        RendererInterface * renderer = RendererInterface::m_pimpl;
        AbstractRenderCommand a(PrimitiveType::TRIANGLES,
                                renderer->GetPyramidPrimCount(),
                                1,
                                0,
                                0,
                                0,
                                renderer->GetPyramidVAO().get(),
                                shader->GetProgramHandle(),
                                shader->GetFrameUniforms(),
                                shader->GetRoomUniforms(),
                                shader->GetObjectUniforms(),
                                shader->GetMaterialUniforms(),
                                renderer->GetCurrentlyBoundTextures(),
                                renderer->GetDefaultFaceCullMode(),
                                renderer->GetDepthFunc(),
                                renderer->GetDepthMask(),
                                renderer->GetStencilFunc(),
                                renderer->GetStencilOp(),
                                renderer->GetPolyMode(),
                                renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a);

        MathUtil::ModelMatrix().scale(1.0f, 1.0f, 1.5f);
        shader->UpdateObjectUniforms();
        AbstractRenderCommand a2(PrimitiveType::TRIANGLES,
                                 renderer->GetPyramidPrimCount(),
                                 1,
                                 0,
                                 0,
                                 0,
                                 renderer->GetPyramidVAO().get(),
                                 shader->GetProgramHandle(),
                                 shader->GetFrameUniforms(),
                                 shader->GetRoomUniforms(),
                                 shader->GetObjectUniforms(),
                                 shader->GetMaterialUniforms(),
                                 renderer->GetCurrentlyBoundTextures(),
                                 renderer->GetDefaultFaceCullMode(),
                                 renderer->GetDepthFunc(),
                                 renderer->GetDepthMask(),
                                 renderer->GetStencilFunc(),
                                 renderer->GetStencilOp(),
                                 renderer->GetPolyMode(),
                                 renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a2);

        MathUtil::ModelMatrix().scale(1.0f, 1.0f, 1.5f);
        shader->UpdateObjectUniforms();
        AbstractRenderCommand a3(PrimitiveType::TRIANGLES,
                                 renderer->GetPyramidPrimCount(),
                                 1,
                                 0,
                                 0,
                                 0,
                                 renderer->GetPyramidVAO().get(),
                                 shader->GetProgramHandle(),
                                 shader->GetFrameUniforms(),
                                 shader->GetRoomUniforms(),
                                 shader->GetObjectUniforms(),
                                 shader->GetMaterialUniforms(),
                                 renderer->GetCurrentlyBoundTextures(),
                                 renderer->GetDefaultFaceCullMode(),
                                 renderer->GetDepthFunc(),
                                 renderer->GetDepthMask(),
                                 renderer->GetStencilFunc(),
                                 renderer->GetStencilOp(),
                                 renderer->GetPolyMode(),
                                 renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a3);
    }

    MathUtil::PopModelMatrix();

}

void RoomObject::DrawPortalStencilGL(QPointer <AssetShader> shader, const bool draw_offset_stencil) const
{
    if (shader == NULL || !shader->GetCompiled()) {
        return;
    }

    QMatrix4x4 m = model_matrix_local;

    const float w = GetScale().x() * 0.5f;
    const float h = GetScale().y() * 0.5f;
    m.translate(0, h, 0);
    m.scale(w, h, 1.0f);

    shader->SetUseTextureAll(false);
    shader->SetUseLighting(false);
    shader->SetUseSkelAnim(false); //53.11 - TODO: find out why is this necessary?
    shader->SetConstColour(QVector4D(0,0,0,1));

    MathUtil::PushModelMatrix();
    MathUtil::MultModelMatrix(m);

    shader->UpdateObjectUniforms();   

    if (GetB("_circular")) {
        RendererInterface * renderer = RendererInterface::m_pimpl;
        AbstractRenderCommand a(PrimitiveType::TRIANGLE_FAN,
                                renderer->GetDiscPrimCount(),
                                1,
                                0,
                                0,
                                0,
                                renderer->GetDiscVAO().get(),
                                shader->GetProgramHandle(),
                                shader->GetFrameUniforms(),
                                shader->GetRoomUniforms(),
                                shader->GetObjectUniforms(),
                                shader->GetMaterialUniforms(),
                                renderer->GetCurrentlyBoundTextures(),
                                FaceCullMode::BACK,
                                DepthFunc::LEQUAL,
                                renderer->GetDepthMask(),
                                renderer->GetStencilFunc(),
                                renderer->GetStencilOp(),
                                PolyMode::FILL,
                                renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a);
        if (draw_offset_stencil)
        {
            RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::DISABLED);
            RendererInterface * renderer = RendererInterface::m_pimpl;
            AbstractRenderCommand a2(PrimitiveType::TRIANGLES,
                                     renderer->GetPortalStencilCylinderPrimCount(),
                                     1,
                                     0,
                                     0,
                                     0,
                                     renderer->GetPortalStencilCylinderVAO().get(),
                                     shader->GetProgramHandle(),
                                     shader->GetFrameUniforms(),
                                     shader->GetRoomUniforms(),
                                     shader->GetObjectUniforms(),
                                     shader->GetMaterialUniforms(),
                                     renderer->GetCurrentlyBoundTextures(),
                                     FaceCullMode::DISABLED,
                                     DepthFunc::LEQUAL,
                                     renderer->GetDepthMask(),
                                     renderer->GetStencilFunc(),
                                     renderer->GetStencilOp(),
                                     PolyMode::FILL,
                                     renderer->GetColorMask());
            renderer->PushAbstractRenderCommand(a2);
            RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::BACK);
        }
    }
    else
    {
        RendererInterface * renderer = RendererInterface::m_pimpl;
        AbstractRenderCommand a(PrimitiveType::TRIANGLES,
                                renderer->GetPlanePrimCount(),
                                1,
                                0,
                                0,
                                0,
                                renderer->GetPlaneVAO().get(),
                                shader->GetProgramHandle(),
                                shader->GetFrameUniforms(),
                                shader->GetRoomUniforms(),
                                shader->GetObjectUniforms(),
                                shader->GetMaterialUniforms(),
                                renderer->GetCurrentlyBoundTextures(),
                                FaceCullMode::BACK,
                                DepthFunc::LEQUAL,
                                renderer->GetDepthMask(),
                                renderer->GetStencilFunc(),
                                renderer->GetStencilOp(),
                                PolyMode::FILL,
                                renderer->GetColorMask());
        renderer->PushAbstractRenderCommand(a);
        if (draw_offset_stencil)
        {
            RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::DISABLED);
            RendererInterface * renderer = RendererInterface::m_pimpl;
            AbstractRenderCommand a2(PrimitiveType::TRIANGLES,
                                     renderer->GetPortalStencilCubePrimCount(),
                                     1,
                                     0,
                                     0,
                                     0,
                                     renderer->GetPortalStencilCubeVAO().get(),
                                     shader->GetProgramHandle(),
                                     shader->GetFrameUniforms(),
                                     shader->GetRoomUniforms(),
                                     shader->GetObjectUniforms(),
                                     shader->GetMaterialUniforms(),
                                     renderer->GetCurrentlyBoundTextures(),
                                     FaceCullMode::DISABLED,
                                     DepthFunc::LEQUAL,
                                     renderer->GetDepthMask(),
                                     renderer->GetStencilFunc(),
                                     renderer->GetStencilOp(),
                                     PolyMode::FILL,
                                     renderer->GetColorMask());
            renderer->PushAbstractRenderCommand(a2);
            RendererInterface::m_pimpl->SetDefaultFaceCullMode(FaceCullMode::BACK);
        }
    }

    MathUtil::PopModelMatrix();
    shader->SetConstColour(QVector4D(1,1,1,1));
}

bool RoomObject::Clicked(const QVector3D & p)
{
    const QVector3D p_local = GetLocal(p);

    const QString t = GetType();

    if (t == "video") {
        if (assetvideo) {
            if (fabsf(p_local.x()) < 1.0f && fabsf(p_local.y()) < assetvideo->GetAspectRatio(&media_ctx) && p_local.z() > 0.0f && p_local.z() < 1.0f) {
                return true;
            }
        }
    }
    else if (t == "object") {
        if (assetobject) {
            const QVector3D bbmin = assetobject->GetBBoxMin() - QVector3D(0.1f, 0.1f, 0.1f);
            const QVector3D bbmax = assetobject->GetBBoxMax() + QVector3D(0.1f, 0.1f, 0.1f);
            if (p_local.x() > bbmin.x() && p_local.x() < bbmax.x() &&
                    p_local.y() > bbmin.y() && p_local.y() < bbmax.y() &&
                    p_local.z() > bbmin.z() && p_local.z() < bbmax.z()) {
                return true;
            }
        }
    }
    else if (t == "ghost") {
        if (fabsf(p_local.x()) < 1.0f && fabsf(p_local.y()) < 2.0f && fabsf(p_local.z()) < 1.0f) {
            return true;
        }
    }
    else if (t == "image") {
        if (assetimage) {
            if (fabsf(p_local.x()) < 1.0f && fabsf(p_local.y()) < assetimage->GetAspectRatio() && p_local.z() > 0.0f && p_local.z() < 1.0f) {
                return true;
            }
        }
    }   

    return false;
}

QVector3D RoomObject::GetLocal(const QVector3D & p) const
{
    const QVector3D d = (p - (GetPos() + GetZDir() * RoomObject::portal_spacing));
    return QVector3D(QVector3D::dotProduct(d, GetXDir()) / GetScale().x(),
                     QVector3D::dotProduct(d, GetYDir()) / GetScale().y(),
                     QVector3D::dotProduct(d, GetZDir()) / GetScale().z());
}

QVector3D RoomObject::GetGlobal(const QVector3D & p) const
{
    return p.x() * GetXDir() * GetScale().x() +
            p.y() * GetYDir() * GetScale().y() +
            p.z() * GetZDir() * GetScale().z() +
            GetPos() + GetZDir() * RoomObject::portal_spacing;
}

int RoomObject::GetNumTris() const
{
    const QString t = GetType();
    if (t == "text" || t == "paragraph") {
        return 2;
    }
    else if (t == "image" || t == "video") {
        return 12;
    }
    else if (t == "link") {
        return 4;
    }
    else if (t == "object") {
        if (assetobject) {
            return assetobject->GetNumTris();
        }
        else {
            return 0;
        }
    }
    else if (t == "ghost") {
        int total = 0;        
        for (const QPointer <AssetObject> & a: ghost_assetobjs) {
            if (a) {
                total += a->GetNumTris();
            }
        }
        return total;
    }

    return 0;
}

void RoomObject::SetInterpTime(const float f)
{
    interp_time = f;
}

float RoomObject::GetInterpTime() const
{
    return interp_time;
}

void RoomObject::SetInterpolate(const bool b)
{
    interpolate = b;
}

bool RoomObject::GetInterpolate()
{
    return interpolate;
}

void RoomObject::SetDirty(const bool val)
{
    if (props) {
        props->SetDirty(val);
    }
}

bool RoomObject::IsDirty() const
{
    if (props) {
        return props->IsDirty();
    }
    return false;
}

QPointer <RoomObject> RoomObject::CreateText(const QString & js_id, const float fixed_size, const QString & text, const QColor col, QVector3D scale)
{
    QPointer <RoomObject> obj(new RoomObject());
    obj->SetType("text");
    obj->SetS("js_id", js_id);
    obj->SetFixedSize(true, fixed_size);
    obj->SetText(text);
    obj->SetC("col", col);
    obj->SetV("scale", scale);
    return obj;
}


QPointer <RoomObject> RoomObject::CreateImage(const QString & js_id, const QString & id, const QColor col, const bool lighting)
{
    QPointer <RoomObject> obj(new RoomObject());
    obj->SetType("image");
    obj->SetS("js_id", js_id);
    obj->SetS("id", id);
    obj->SetC("col", col);
    obj->SetB("lighting", lighting);
    return obj;
}

QPointer <RoomObject> RoomObject::CreateObject(const QString & js_id, const QString & id, const QColor col, const bool lighting)
{
    QPointer <RoomObject> obj(new RoomObject());
    obj->SetType("object");
    obj->SetS("js_id", js_id);
    obj->SetS("id", id);
    obj->SetC("col", col);
    obj->SetB("lighting", lighting);
    return obj;
}

QPointer <RoomObject> RoomObject::CreateParagraph(const QString & js_id, const QString & id, const QString & text, const QColor col, const bool lighting)
{
    QPointer <RoomObject> obj(new RoomObject());
    obj->SetType("paragraph");
    obj->SetS("js_id", js_id);
    obj->SetS("id", id);
    obj->SetText(text);    
    obj->SetC("col", col);
    obj->SetB("lighting", lighting);
    return obj;
}

void RoomObject::PlayCreateObject()
{
    switch (qrand() % 3) {
    case 0:
        SoundManager::Play(SOUND_SELECTOBJECT, false, GetPos(), 1.0f);
        break;
    case 1:
        SoundManager::Play(SOUND_SELECTIMAGE, false, GetPos(), 1.0f);
        break;
    default:
        SoundManager::Play(SOUND_NEWTEXT, false, GetPos(), 1.0f);
        break;
    }
}

void RoomObject::PlayDeleteObject()
{
    SoundManager::Play(SOUND_DELETING, false, GetPos(), 1.0f);
}

float RoomObject::GetTimeElapsed() const
{
    return time_elapsed;
}

void RoomObject::LoadGhost_Helper(const int depth, const QVariantMap & d, QPointer <RoomObject> parent_object, QHash <QString, QPointer <AssetObject> > & asset_obj_list,  QHash <QString, QPointer <AssetShader> > & asset_shader_list)
{    
//    qDebug() << "RoomObject::LoadGhost_Helper() - GOT HERE1" << this->GetJSID() << this->GetID();
    if (depth >= 32) {
        return;
    }

    if (d.contains("assetobject")) {
        QVariantList l = d["assetobject"].toList();
        for (int i=0; i<l.size(); ++i) {
            QVariantMap a = l[i].toMap();
            QPointer <AssetObject> new_asset_obj(new AssetObject());
            new_asset_obj->SetSrc("", a["src"].toString());
            new_asset_obj->SetProperties(a);
            new_asset_obj->Load();
            asset_obj_list[a["id"].toString()] = new_asset_obj;
        }
    }

    if (d.contains("assetshader"))
    {
        QVariantList l = d["assetshader"].toList();
        for (int i=0; i<l.size(); ++i)
        {
            QVariantMap a = l[i].toMap();
            QPointer <AssetShader> new_asset_shader(new AssetShader());
            new_asset_shader->SetSrc("", a["src"].toString(), "");
            new_asset_shader->SetProperties(a);
            new_asset_shader->Load();
            asset_shader_list[a["id"].toString()] = new_asset_shader;
        }
    }

    if (d.contains("ghost")) {
        QVariantList l = d["ghost"].toList();
        for (int i=0; i<l.size(); ++i) {
            QVariantMap o = l[i].toMap();
            SetType("ghost");
            SetProperties(o);
            LoadGhost_Helper(depth+1, o, this, asset_obj_list, asset_shader_list);
        }
    }

    if (d.contains("object")) {
        QVariantList l = d["object"].toList();
        for (int i=0; i<l.size(); ++i) {
            QVariantMap o = l[i].toMap();

            QPointer <RoomObject> new_thing(new RoomObject());
            new_thing->SetType("object");
            new_thing->SetProperties(o);

            //make sure we can link this Object to an AssetObject
            if (asset_obj_list.contains(o["id"].toString())) {
                new_thing->SetAssetObject(asset_obj_list[o["id"].toString()]);
            }

            LoadGhost_Helper(depth+1, o, new_thing, asset_obj_list, asset_shader_list);

            if (parent_object) {
                parent_object->GetChildObjects().push_back(new_thing);
            }
        }
    }

    if (d.contains("assets")) {
        LoadGhost_Helper(depth+1, d["assets"].toMap(), parent_object, asset_obj_list, asset_shader_list);
    }
    if (d.contains("room")) {
        LoadGhost_Helper(depth+1, d["room"].toMap(), parent_object, asset_obj_list, asset_shader_list);
    }
    if (d.contains("FireBoxRoom")) {
        LoadGhost_Helper(depth+1, d["FireBoxRoom"].toMap(), parent_object, asset_obj_list, asset_shader_list);
    }
}

void RoomObject::LoadGhost(const QString & data)
{
//    qDebug() << "RoomObject::LoadGhost" << data;
//    return;
    child_objects.clear(); //54.8 - prevent userid.txt from filling up with child objects

    HTMLPage avatar_page;
    avatar_page.ReadXMLContent(data);

    QHash <QString, QPointer <AssetObject> > asset_obj_list;
    QHash <QString, QPointer <AssetShader> > asset_shader_list;
    QPointer <RoomObject> root_object(new RoomObject());

    LoadGhost_Helper(0, avatar_page.GetData(), root_object, asset_obj_list, asset_shader_list);

    SetGhostAssetObjects(asset_obj_list);
    SetGhostAssetShaders(asset_shader_list);

    root_object->GetChildObjects().clear();
    delete root_object;
}

void RoomObject::DoGhostMoved(const QVariantMap & m)
{
    const QString userid = m["userId"].toString();
    const QString roomid = m["roomId"].toString();    
    GhostFrame frame0;
    GhostFrame frame1;

//    qDebug() << "RoomObject::DoGhostMoved doing ghost moved" << userid << roomid << GetURL();
    time_elapsed = 0.0f;   

    //Set sample rate
    if (m.contains("sample_rate")) {
        sound_buffers_sample_rate = m["sample_rate"].toUInt();
    }

    AssetGhost::ConvertPacketToFrame(m, frame1);
    frame1.time_sec = rate;

    QVector <GhostFrame> frames;

    if (assetghost.isNull() || QString::compare(GetURL(), roomid) != 0) { //if no ghost (new player), or this player changed URLs
        if (assetghost.isNull()) {
            assetghost = new AssetGhost();
        }

        //set first and last frames to be the same (current packet)
        SetV("pos", frame1.pos);
        SetDir(frame1.dir);

        ghost_frame = frame1;

        //49.12 - prevents interpolation between positions (immediately moves to next place)
        frame0 = frame1;
        frame0.time_sec = 0.0f;

        frames.push_back(frame0);
        frames.push_back(frame1);                        
    }
    else {
        //set new frames                
        frame0 = ghost_frame;
        frame0.time_sec = 0.0f;       
        frames.push_back(frame0);
        frames.push_back(frame1);        
    }

    if (assetghost) {
        assetghost->SetFromFrames(frames, rate);
    }

    ghost_frame_index = -1;

    SetURL(roomid);
}

QList <RoomObjectEdit> & RoomObject::GetRoomEditsIncoming()
{
    return room_edits_incoming;
}

QList <RoomObjectEdit> & RoomObject::GetRoomDeletesIncoming()
{
    return room_deletes_incoming;
}

QList <QString> & RoomObject::GetSendPortalURL()
{
    return send_portal_url;
}

QList <QString> & RoomObject::GetSendPortalJSID()
{
    return send_portal_jsid;
}

QList <QVector3D> & RoomObject::GetSendPortalPos()
{
    return send_portal_pos;
}

QList <QVector3D> & RoomObject::GetSendPortalFwd()
{
    return send_portal_fwd;
}

void RoomObject::SetPlayerInRoom(const bool b)
{
    player_in_room = b;
}

void RoomObject::SetPlayerInAdjacentRoom(const bool b)
{
    player_in_adjacent_room = b;
}

void RoomObject::SetRescaleOnLoad(const bool b)
{
    rescale_on_load = b;
}

void RoomObject::SetRescaleOnLoadAspect(const bool b)
{
    rescale_on_load_aspect = b;
}

bool RoomObject::GetRescaleOnLoad() const
{
    return rescale_on_load;
}

void RoomObject::SetParentObject(QPointer <RoomObject> p)
{
    parent_object = p;
}

void RoomObject::AppendChild(QPointer <RoomObject> child)
{
//    qDebug()<<"RoomObject::AppendChild() - Appended"<<child->GetJSID()<<"to"<<GetJSID();
    child_objects.push_back(child);
    child->SetParentObject(this);
}

void RoomObject::RemoveChildByJSID(QString jsid)
{
    for (int i=0; i<child_objects.size(); ++i)
    {
        if (child_objects[i]->GetS("js_id") == jsid)
        {
            child_objects[i]->SetParentObject(NULL);
            child_objects.removeAt(i);
            return;
        }
    }
}

void RoomObject::GetLight(LightContainer* p_container, QMatrix4x4* p_model_matrix)
{
    if (GetType() == "light") {
        float light_cone_angle = GetF("light_cone_angle");
        if (light_cone_angle == -1.0f) {
            return; // Skip over when light is disabled
        }
        float xScale = p_model_matrix->column(0).length();
        float yScale = p_model_matrix->column(1).length();
        float zScale = p_model_matrix->column(2).length();
        float averageScale = (xScale + yScale + zScale) / 3;

        QColor color = GetC("col");
        QVector3D light_intensity_color(color.redF(), color.greenF(), color.blueF());
        light_intensity_color *= GetF("light_intensity");
        light_intensity_color *= averageScale * averageScale;
        if (light_intensity_color == QVector3D(0.0f, 0.0f, 0.0f))
        {
            return; // Skip over when light has no intensity
        }

        float light_range = GetF("light_range") * averageScale;
        if (light_range == 0.0f)
        {
            return; // Skip over when light has no range
        }
        light_range *= light_range; // We square this to simplify the shader code

        // Get room-space position
        QVector3D light_pos = GetPos();
        // Apply roomMatrix to bring it into world-space for lighting
        light_pos = (MathUtil::RoomMatrix() * (*p_model_matrix) * QVector4D(light_pos.x(), light_pos.y(), light_pos.z(), 1)).toVector3D();

        // Get room-space direction
        QVector3D light_direction = GetModelMatrixLocal().column(2).toVector3D().normalized();

        // Calculate rotation only matrix to apply to it to bring it into world-space
        QMatrix4x4 room_matrix = MathUtil::RoomMatrix();
        room_matrix.setColumn(0, room_matrix.column(0).normalized());
        room_matrix.setColumn(1, room_matrix.column(1).normalized());
        room_matrix.setColumn(2, room_matrix.column(2).normalized());
        room_matrix.setColumn(3, QVector4D(0.0, 0.0, 0.0, 0.0));

        QMatrix4x4 model_matrix_normalized = (*p_model_matrix);
        model_matrix_normalized.setColumn(0, model_matrix_normalized.column(0).normalized());
        model_matrix_normalized.setColumn(1, model_matrix_normalized.column(1).normalized());
        model_matrix_normalized.setColumn(2, model_matrix_normalized.column(2).normalized());
        model_matrix_normalized.setColumn(3, QVector4D(0.0, 0.0, 0.0, 0.0));

        QMatrix4x4 rotation_matrix = room_matrix * model_matrix_normalized;
        light_direction = (rotation_matrix * QVector4D(light_direction, 0.0f)).toVector3D();

        float light_exponent = GetF("light_cone_exponent");
        p_container->m_lights.push_back(Light(light_intensity_color, light_cone_angle, light_pos, light_range, light_direction, light_exponent));
    }
}

void RoomObject::GetLights(LightContainer* p_container, QMatrix4x4* p_model_matrix)
{
    // Add our own Light to the container if we are of type light
    GetLight(p_container, p_model_matrix);
    QMatrix4x4 prev_model_matrix = *p_model_matrix;
    (*p_model_matrix) *= GetModelMatrixLocal();

    // Recusively call GetLights on any child RoomObjects with our model matrix applied
    QList<QPointer<RoomObject>> children = GetChildObjects();
    auto children_itr = children.begin();
    auto children_end = children.end();
    for (;children_itr != children_end; children_itr++)
    {
        if ((*children_itr)) {
            (*children_itr)->GetLights(p_container, p_model_matrix);
        }
    }

    // Restore model matrix back to its previous state
    (*p_model_matrix) = prev_model_matrix;
}

void RoomObject::SetDrawAssetObjectTeleport(const bool b)
{
    draw_assetobject_teleport = b;
}

bool RoomObject::GetDrawAssetObjectTeleport()
{
    return draw_assetobject_teleport;
}

float RoomObject::GetRate()
{
    return rate;
}

float RoomObject::GetLogoffRate()
{
    return logoff_rate;
}

void RoomObject::DrawGhostUserIDChat(QPointer <AssetShader> shader)
{
    //compute dialog dimensions
    const float message_duration_msec = 7000.0f;
    const float row_height = 0.16f;
    float width = textgeom_player_id->GetScale()*(textgeom_player_id->GetText().length()+2.0f);
    float height = row_height;

    if (!textgeom_chatmessages.isEmpty()) {
        height += row_height*0.5f; //space between userid and text msgs
    }

    for (int i=0; i<textgeom_chatmessages.size(); ++i) {
        const float interp = (chat_message_times[i].elapsed() + chat_message_time_offsets[i] * 1000.0f)/message_duration_msec;

        if (interp > 0.0f && interp < 1.0f) {
            const QVector <TextGeomLine> texts = textgeom_chatmessages[i]->GetAllText();
            height += float(texts.size()) * row_height + row_height * 0.25f;
            for (int j=0; j<texts.size(); ++j) {
                width = qMax(width, textgeom_player_id->GetScale()*float(texts[j].text.length()+2.0f));
            }
        }
    }

    //draw chat messages
    const QVector3D s = GetScale();
    MathUtil::PushModelMatrix();
    MathUtil::ModelMatrix().scale(1.0f/s.x(), 1.0f/s.y(), 1.0f/s.z());
    MathUtil::ModelMatrix().translate(userid_pos + QVector3D(0,2.0f,0));

    //billboard/auto-face
    const QVector3D p = MathUtil::ModelMatrix().column(3).toVector3D();
    QMatrix4x4 m = MathUtil::ViewMatrix().transposed();
    m.setColumn(3, QVector4D(p, 1));
    m.setRow(3, QVector4D(0,0,0,1));
    MathUtil::LoadModelMatrix(m);

    //draw grey box
    MathUtil::PushModelMatrix();
    MathUtil::ModelMatrix().scale(width, height, 1);
    SpinAnimation::DrawPlaneGL(shader, QColor(0,0,0,128));
    MathUtil::PopModelMatrix();

    MathUtil::ModelMatrix().translate(0,height*0.5f-row_height*0.5f,0);

    //draw userid slightly in front of back box
    MathUtil::PushModelMatrix();
    MathUtil::ModelMatrix().translate(0,0,0.02f);
    MathUtil::MultModelMatrix(textgeom_player_id->GetModelMatrix());
    textgeom_player_id->DrawGL(shader);

    //draw speaker if talking
    if (ghost_frame.speaking && sound_img) {
        MathUtil::ModelMatrix().translate(-width*0.5f -1.0f,0,0);
        MathUtil::ModelMatrix().scale(2.0f, 2.0f, 1.0f);

        RendererInterface::m_pimpl->BindTextureHandle(0, sound_img->GetTextureHandle(true));
        shader->SetUseTexture(0, true);
        SpinAnimation::DrawPlaneGL(shader, QColor(255,255,255,255));
        shader->SetUseTexture(0, false);
    }

    MathUtil::PopModelMatrix();

    MathUtil::PushModelMatrix();

    MathUtil::ModelMatrix().translate(0,-row_height*1.5f,0.02f);
    for (int i=0; i<textgeom_chatmessages.size(); ++i) {

        const float interp = (chat_message_times[i].elapsed() + chat_message_time_offsets[i] * 1000.0f)/message_duration_msec;

        float alpha = 1.0f;
        if (interp < 0.1f) {
            alpha = interp * 10.0f;
        }
        else if (interp > 0.9f) {
            alpha = 1.0f - (interp-0.9f) * 10.0f;
        }

        if (interp > 0.0f && interp < 1.0f) {
            //draw grey box
            MathUtil::PushModelMatrix();
            MathUtil::ModelMatrix().scale(float(textgeom_chatmessages[i]->GetTextLength()), row_height*textgeom_chatmessages[i]->GetAllText().size(), 1);
            SpinAnimation::DrawPlaneGL(shader, QColor(128,128,128,128*alpha));
            MathUtil::PopModelMatrix();

            MathUtil::PushModelMatrix();
            MathUtil::ModelMatrix().translate(0,0,0.02f);
            MathUtil::MultModelMatrix(textgeom_chatmessages[i]->GetModelMatrix());
            textgeom_chatmessages[i]->SetColour(QColor(255,255,255,255*alpha));
            textgeom_chatmessages[i]->DrawGL(shader);
            MathUtil::PopModelMatrix();
        }

        MathUtil::ModelMatrix().translate(0,-row_height*textgeom_chatmessages[i]->GetAllText().size() - row_height * 0.25f,0);

        if (interp > 1.0f) {
            textgeom_chatmessages.removeAt(i);
            chat_message_times.removeAt(i);
            chat_message_time_offsets.removeAt(i);
            --i;
        }
    }

    MathUtil::PopModelMatrix();

    MathUtil::PopModelMatrix();
}

void RoomObject::SetGrabbed(const bool b)
{
    grabbed = b;
}

bool RoomObject::GetGrabbed() const
{
    return grabbed;
}

QString RoomObject::ShortenString(const QString & s)
{
    const int maxlen = 40;
    if (s.length() < maxlen) {
       return s;
    }
    else {
        return s.left(maxlen/2) + QString("...") + s.right(maxlen/2);
    }
}

void RoomObject::DrawGL(QPointer <AssetShader> shader)
{ 
    DrawPortalGL(shader);
}

void RoomObject::DrawDecorationsGL(QPointer <AssetShader> shader, const float anim_val)
{
    DrawPortalDecorationsGL(shader, anim_val);
}

void RoomObject::DrawStencilGL(QPointer <AssetShader> shader, const QVector3D & player_pos) const
{    
    DrawPortalStencilGL(shader, GetPlayerAtSigned(player_pos));
}

float RoomObject::GetWidth() const
{
    return GetScale().x();
}

bool RoomObject::GetPlayerAtSigned(const QVector3D & player_pos) const
{
    const QVector3D local = GetLocal(player_pos);
    return (fabsf(local.x()) <= 0.5f && local.y() >= 0.0f && local.y() <= 1.6f && local.z() >= 0.0f && local.z() <= 1.0f);
}

bool RoomObject::GetPlayerCrossed(const QVector3D & player_pos, const QVector3D & player_last_pos) const
{
    QVector3D local1 = GetLocal(player_pos);
    QVector3D local2 = GetLocal(player_last_pos);    

    return (local1.z()) < 0.0f && (local2.z()) >= 0.0f && fabsf(local2.x()) <= 0.5f && local2.y() >= 0.0f && local2.y() <= 1.6f;
}

float RoomObject::GetSpacing()
{
    return portal_spacing;
}

void RoomObject::SetThumbAssetImage(const QPointer <AssetImage> a)
{
    portal_thumb_img = a;
}

QPointer <AssetImage> RoomObject::GetThumbAssetImage()
{
    return portal_thumb_img;
}

MediaContext * RoomObject::GetMediaContext()
{
    return &media_ctx;
}

QPointer <RoomObject> RoomObject::CreateFromProperties(QPointer<DOMNode> properties)
{
    QPointer <RoomObject> obj(new RoomObject());
    obj->SetProperties(properties);
    return obj;
}

void RoomObject::UpdateAssets()
{
    if (cursor_arrow_obj) {
        cursor_arrow_obj->Update();
        cursor_arrow_obj->UpdateGL();
    }
    if (cursor_crosshair_obj) {
        cursor_crosshair_obj->Update();
        cursor_crosshair_obj->UpdateGL();
    }
    if (cursor_hand_obj) {
        cursor_hand_obj->Update();
        cursor_hand_obj->UpdateGL();
    }
    if (linear_gradient_img) {
        linear_gradient_img->UpdateGL();
    }
    if (sound_img) {
        sound_img->UpdateGL();
    }
    if (light_img) {
        light_img->UpdateGL();
    }
    if (object_img) {
        object_img->UpdateGL();
    }
    if (particle_img) {
        particle_img->UpdateGL();
    }
    if (avatar_obj) {
        avatar_obj->Update();
        avatar_obj->UpdateGL();
    }
    if (avatar_head_obj) {
        avatar_head_obj->Update();
        avatar_head_obj->UpdateGL();
    }
#ifdef __ANDROID__
    if (internet_connected_img) {
        internet_connected_img->UpdateGL();
    }
    if (remove_headset_img) {
        remove_headset_img->UpdateGL();
    }
#endif
}

