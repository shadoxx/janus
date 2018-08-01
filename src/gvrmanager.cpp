#include "gvrmanager.h"

GVRManager::GVRManager()
{
    last_controller_xform = QMatrix4x4();
    controller_xform = QMatrix4x4();

    hmd_xform = QMatrix4x4();

    m_eye_view_matrices.resize(2);
    m_eye_projection_matrices.resize(4);

    m_eye_view_matrices[0] = QMatrix4x4();
    m_eye_view_matrices[1] = QMatrix4x4();

    m_eye_projection_matrices[0] = QMatrix4x4();
    m_eye_projection_matrices[1] = QMatrix4x4();
    m_eye_projection_matrices[2] = QMatrix4x4();
    m_eye_projection_matrices[3] = QMatrix4x4();

    gl_initialized = false;
    initialized = false;
}

QSize GVRManager::GetTextureSize() const
{
    return QSize(idealTextureSize.width/2, idealTextureSize.height);
}

bool GVRManager::Initialize()
{
    JNIUtil::SetupGVR();

    // The Java GvrLayout owns the gvr_context.
    //qDebug() << "GVRManager::Initialize() - Context" << JNIUtil::GetGVRContext();

    context = reinterpret_cast<gvr_context*>(JNIUtil::GetGVRContext());
    gvr_api = gvr::GvrApi::WrapNonOwned(context);

    if (context == NULL || gvr_api == NULL) return false;

    idealTextureSize = gvr_api->GetMaximumEffectiveRenderTargetSize();
    idealTextureSize.width = ((5 * idealTextureSize.width) / 10) + 8;
    idealTextureSize.height = (5 * idealTextureSize.height) / 10;

    //qDebug() << "TEXTURESIZE" << idealTextureSize.width << idealTextureSize.height;

    controller_api.reset(new gvr::ControllerApi);
    int32_t options = gvr::ControllerApi::DefaultOptions();
    //options |= GVR_CONTROLLER_ENABLE_GYRO;
    //options |= GVR_CONTROLLER_ENABLE_ACCEL;
    options |= GVR_CONTROLLER_ENABLE_POSE_PREDICTION;
    options |= GVR_CONTROLLER_ENABLE_POSITION;
    options |= GVR_CONTROLLER_ENABLE_ARM_MODEL;
    controller_api->Init(options, context);
    controller_api->Resume();

    initialized = true;

    return true;
}

void GVRManager::PostPresent()
{

}

void GVRManager::InitializeGL()
{
    if (!initialized) return;

    //qDebug() << "GVR::InitializeGL()";

    // This must be called on the thread in which the app will submit OpenGL
    // rendering commands.
    gvr_api->InitializeGl();

    // Create a swap chain with one framebuffer per frame, and a set
    // of buffer viewports to be updated each frame.
    buffer_viewports = gvr_api->CreateEmptyBufferViewportList();
    left_viewport = gvr_api->CreateBufferViewport();
    right_viewport = gvr_api->CreateBufferViewport();

    buffer_viewports.SetToRecommendedBufferViewports();
    buffer_viewports.GetBufferViewport(GVR_LEFT_EYE, &left_viewport);
    buffer_viewports.GetBufferViewport(GVR_RIGHT_EYE, &right_viewport);

    const gvr::Rectf r = {qMax(left_viewport.GetSourceFov().left, right_viewport.GetSourceFov().left),
                       qMax(left_viewport.GetSourceFov().right, right_viewport.GetSourceFov().right),
                       (left_viewport.GetSourceFov().bottom + right_viewport.GetSourceFov().bottom) / 2,
                       (left_viewport.GetSourceFov().top + right_viewport.GetSourceFov().top) / 2};
    left_viewport.SetSourceFov(r);
    right_viewport.SetSourceFov(r);

    buffer_viewports.SetBufferViewport((size_t) 0, left_viewport);
    buffer_viewports.SetBufferViewport((size_t) 1, right_viewport);

    std::vector<gvr::BufferSpec> specs;

    specs.push_back(gvr_api->CreateBufferSpec());
    specs[0].SetSize(idealTextureSize);
    specs[0].SetColorFormat(GVR_COLOR_FORMAT_RGBA_8888);
    specs[0].SetDepthStencilFormat(GVR_DEPTH_STENCIL_FORMAT_NONE); //GVR_DEPTH_STENCIL_FORMAT_DEPTH_24_STENCIL_8
    specs[0].SetSamples(0);
    swap_chain.reset(new gvr::SwapChain(gvr_api->CreateSwapChain(specs)));

    this->ReCentre();

    m_eye_viewports.resize(2);
    m_eye_viewports[0] = QVector4D(0, 0, (idealTextureSize.width / 2) - 4, idealTextureSize.height);
    m_eye_viewports[1] = QVector4D((idealTextureSize.width / 2) + 4, 0, (idealTextureSize.width / 2) - 4, idealTextureSize.height);

    gl_initialized = true;
}

QString GVRManager::GetHMDString() const
{
    return QString("");
}

QString GVRManager::GetHMDType() const
{
    return (gvr_api->GetViewerType() == GVR_VIEWER_TYPE_CARDBOARD) ? QString("cardboard") : QString("daydream");
}

bool GVRManager::GetEnabled() const
{
    return (context != NULL && gvr_api != NULL && JNIUtil::GetShowingVR());
}

float GVRManager::GetIPD() const
{
    return 0;
    //return 0.064f;
}

void GVRManager::EnterVR()
{
    JNIUtil::ShowGVR(true);
    JNIUtil::HideSplash();
}

void GVRManager::ExitVR()
{
    JNIUtil::ShowGVR(false);
}

void GVRManager::Update()
{
    if (!gl_initialized) return;

    //qDebug() << "GVRManager - Acquiring frame";
    frame = swap_chain->AcquireFrame();
    //gvr_frame * frame = gvr_swap_chain_acquire_frame(swap_chain->cobj());

    //Update hmd xform
    gvr::ClockTimePoint pred_time = gvr::GvrApi::GetTimePointNow();
    pred_time.monotonic_system_time_nanos += 50000000;
    gvr_hmd_xform = gvr_api->ApplyNeckModel(gvr_api->GetHeadSpaceFromStartSpaceRotation(pred_time), 1.0f);

    QMatrix4x4 h = GVRMatrixToMatrix(gvr_hmd_xform);

    QQuaternion q = MathUtil::QuaternionFromQMatrix4x4(h);

    QVector3D x, y, z;
    x = QVector3D(1,0,0);
    y = QVector3D(0,1,0);
    z = QVector3D(0,0,-1);

    x = q.rotatedVector(x);
    y = q.rotatedVector(y);
    z = q.rotatedVector(z);

    QMatrix4x4 m;
    m.setColumn(0, x);
    m.setColumn(1, y);
    m.setColumn(2, z);
    m.setColumn(3, QVector3D(h.column(3).x(), h.column(3).y()+1.6,h.column(3).z()));
    m.setRow(3, QVector4D(0,0,0,1));
    m.scale(1,1,-1);

    hmd_xform = m;

    //Set up projection matrix for specific eye
    //qDebug() << "GVRManager - Loading matrices";

    // m_eye_view_matrices contains the compelte transform from tracking origin to each eye.
    // This is what we use when rendering, hmd_xform is used purely for positiong the player capsule
    QMatrix4x4 l = GVRMatrixToMatrix(gvr_api->GetEyeFromHeadMatrix(GVR_LEFT_EYE))*h;
    QMatrix4x4 r = GVRMatrixToMatrix(gvr_api->GetEyeFromHeadMatrix(GVR_RIGHT_EYE))*h;
    QMatrix4x4 v = (l+r)/2;
    m_eye_view_matrices[GVR_LEFT_EYE] = v;
    m_eye_view_matrices[GVR_RIGHT_EYE] = v;

    QMatrix4x4 p = SymmetricPerspectiveMatrixFromView(left_viewport.GetSourceFov(), right_viewport.GetSourceFov(), m_avatar_near_clip, m_avatar_far_clip);
    QMatrix4x4 p2 = SymmetricPerspectiveMatrixFromView(left_viewport.GetSourceFov(), right_viewport.GetSourceFov(), m_avatar_near_clip, m_avatar_far_clip);
    m_eye_projection_matrices[GVR_LEFT_EYE + 2] = p;
    m_eye_projection_matrices[GVR_RIGHT_EYE + 2] = p;
    m_eye_projection_matrices[GVR_LEFT_EYE] = p2;
    m_eye_projection_matrices[GVR_RIGHT_EYE] = p2;

    //qDebug() << "gvr::hmd_xform "<< hmd_xform;

    controller_api->ApplyArmModel(gvr_api->GetUserPrefs().GetControllerHandedness(),
                       GVR_ARM_MODEL_FOLLOW_GAZE, //GVR_ARM_MODEL_SYNC_GAZE,
                       gvr_hmd_xform);

    //Update last controller xform
    last_controller_xform = controller_xform;

    //Update controller xform
    x = QVector3D(1,0,0);
    y = QVector3D(0,1,0);
    z = QVector3D(0,0,1);

    controller_state.Update(*controller_api);

    gvr_vec3f p_gvr = controller_state.GetPosition(); //Simulated position
    //qDebug() <<  "gvr::controller_pos " << pos;

    const gvr::ControllerQuat & rot = controller_state.GetOrientation();
    QQuaternion qt_quat = QQuaternion(rot.qw, rot.qx, rot.qy, rot.qz);
    x = qt_quat.rotatedVector(x);
    y = qt_quat.rotatedVector(y);
    z = qt_quat.rotatedVector(z);
    //qDebug() <<  "gvr::controller_quat " << qt_quat;

    m.setColumn(0, x);
    m.setColumn(1, y);
    m.setColumn(2, z);
    m.setColumn(3, QVector3D(p_gvr.x, 1.6 + p_gvr.y, p_gvr.z - 0.25));
    m.setRow(3, QVector4D(0,0,0,1));

    controller_xform = (controller_state.GetConnectionState() == GVR_CONTROLLER_CONNECTED) ?  m : QMatrix4x4();

    //qDebug() <<  "gvr::controller_xform " << controller_xform;

    if (controller_state.GetRecentered()){
        this->ReCentre();
    }
}

QMatrix4x4 GVRManager::GetHMDTransform() const
{
    return hmd_xform;
}

QMatrix4x4 GVRManager::GetControllerTransform(const int i) const
{
    if (gvr_api->GetViewerType() == GVR_VIEWER_TYPE_DAYDREAM && i == 0)
        return controller_xform;
    return QMatrix4x4();
}

QMatrix4x4 GVRManager::GetLastControllerTransform(const int i) const
{
    if (gvr_api->GetViewerType() == GVR_VIEWER_TYPE_DAYDREAM && i == 0)
        return last_controller_xform;
    return QMatrix4x4();
}

int GVRManager::GetNumControllers() const
{
    if (gvr_api->GetViewerType() == GVR_VIEWER_TYPE_DAYDREAM){
        //qDebug() <<  "gvr::num_controllers " << QString((controller_state.GetConnectionState() == GVR_CONTROLLER_CONNECTED) ?"1":"0");
        return (controller_state.GetConnectionState() == GVR_CONTROLLER_CONNECTED)?1:0;
    }
    //qDebug() <<  "gvr::num_controllers" << "none";
    return 0;
}

bool GVRManager::GetControllerTracked(const int i)
{
    if (gvr_api->GetViewerType() == GVR_VIEWER_TYPE_DAYDREAM && i == 0){
        //qDebug() <<  "gvr::controller_connected " << QString((controller_state.GetConnectionState() == GVR_CONTROLLER_CONNECTED));
        return (controller_state.GetConnectionState() == GVR_CONTROLLER_CONNECTED);
    }
    //qDebug() <<  "gvr::controller_connected " << "none";
    return false;
}

QVector2D GVRManager::GetControllerThumbpad(const int i) const
{
    //Only report if thumb pad is not pressed
    if (gvr_api->GetViewerType() == GVR_VIEWER_TYPE_DAYDREAM && i == 0){
        //qDebug() <<  "gvr::controller_thumb_pos " << QVector2D(2*(controller_state.GetTouchPos().x - 0.5), 2*(controller_state.GetTouchPos().y - 0.5));
        return QVector2D(2*(controller_state.GetTouchPos().x - 0.5), -2*(controller_state.GetTouchPos().y - 0.5));
    }
    //qDebug() <<  "gvr::controller_thumb_pos " << "none";
    return QVector2D();
}

bool GVRManager::GetControllerThumbpadTouched(const int i) const
{
    if (gvr_api->GetViewerType() == GVR_VIEWER_TYPE_DAYDREAM && i == 0){
        //qDebug() <<  "gvr::controller_thumb_touched " << controller_state.IsTouching();
        return controller_state.IsTouching();
    }
    //qDebug() <<  "gvr::controller_thumb_touched " << "none";
    return false;
}

bool GVRManager::GetControllerThumbpadPressed(const int i) const
{
    if (gvr_api->GetViewerType() == GVR_VIEWER_TYPE_DAYDREAM && i == 0){
        //qDebug() <<  "gvr::controller_thumb_pressed " << controller_state.GetButtonState(GVR_CONTROLLER_BUTTON_CLICK);
        return controller_state.GetButtonState(GVR_CONTROLLER_BUTTON_CLICK); //GVR_CONTROLLER_BUTTON_CLICK is used for touchpad clicks
    }
    //qDebug() <<  "gvr::controller_thumb_pressed " << "none";

    return false;
}

QVector2D GVRManager::GetControllerStick(const int i) const
{
    return QVector2D();
}

bool GVRManager::GetControllerStickTouched(const int i) const
{
    return false;
}

bool GVRManager::GetControllerStickPressed(const int i) const
{
    return false;
}

float GVRManager::GetControllerTrigger(const int i) const
{
    return 0.0f;
}

float GVRManager::GetControllerGrip(const int i) const
{
    return 0.0f;
}

bool GVRManager::GetControllerMenuPressed(const int i)
{
    if (gvr_api->GetViewerType() == GVR_VIEWER_TYPE_DAYDREAM && i == 0){
        //qDebug() <<  "gvr::app_pressed " << controller_state.GetButtonState(GVR_CONTROLLER_BUTTON_APP);
        return controller_state.GetButtonState(GVR_CONTROLLER_BUTTON_APP); //GVR_CONTROLLER_BUTTON_APP is used for app button clicks
    }
    //qDebug() <<  "gvr::app_pressed " << "none";
    return false;
}

bool GVRManager::GetControllerButtonAPressed() const
{
    return false;
}

bool GVRManager::GetControllerButtonBPressed() const
{
    return false;
}

bool GVRManager::GetControllerButtonXPressed() const
{
    return false;
}

bool GVRManager::GetControllerButtonYPressed() const
{
    return false;
}

bool GVRManager::GetControllerButtonATouched() const
{
    return false;
}

bool GVRManager::GetControllerButtonBTouched() const
{
    return false;
}

bool GVRManager::GetControllerButtonXTouched() const
{
    return false;
}

bool GVRManager::GetControllerButtonYTouched() const
{
    return false;
}

void GVRManager::EndRenderEye(const int )
{

}

void GVRManager::BeginRendering()
{

}

void GVRManager::BeginRenderEye(const int eye)
{

}

void GVRManager::EndRendering()
{
    if (!gl_initialized || !frame) return;

    //RendererInterface::m_pimpl->BlitMultisampledFramebuffer(FBO_TEXTURE_BITFIELD::COLOR);
    //MathUtil::glFuncs->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //qDebug() << "GVRManager - Binding buffer";
    //RendererInterface::m_pimpl->BindFBOToRead(FBO_TEXTURE_BITFIELD::COLOR, false);
    MathUtil::glFuncs->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frame.GetFramebufferObject(0));
    MathUtil::glFuncs->glBlitFramebuffer(0, 0, (idealTextureSize.width/2)-4, idealTextureSize.height, 0, 0, (idealTextureSize.width/2)-4, idealTextureSize.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    MathUtil::glFuncs->glBlitFramebuffer(0, 0, (idealTextureSize.width/2)-4, idealTextureSize.height, (idealTextureSize.width/2)+4, 0, idealTextureSize.width, idealTextureSize.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    //qDebug() << "GVRManager - Unbinding buffer";
    //MathUtil::glFuncs->glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //qDebug() << "GVRManager - Submitting frame";
    frame.Submit(buffer_viewports, gvr_hmd_xform);
    //gvr_frame_submit(&frame, buffer_viewports.cobj(), h);
    //qDebug() << "GVRManager - After submitting";

    MathUtil::glFuncs->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GVRManager::ReCentre()
{
    gvr_api->RecenterTracking();
}

void GVRManager::TriggerHapticPulse(const int i, const int val)
{
    Q_UNUSED(i)
    Q_UNUSED(val)
}

void GVRManager::Platform_ProcessMessages()
{

}

bool GVRManager::Platform_GetEntitled() const
{
    return true;
}

bool GVRManager::Platform_GetShouldQuit() const
{
    return false;
}

QMatrix4x4 GVRManager::GVRMatrixToMatrix(gvr::Mat4f m)
{
    return QMatrix4x4(
                m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3],
                m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3],
                m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3],
                m.m[3][0], m.m[3][1], m.m[3][2], m.m[3][3]);
}

gvr::Mat4f GVRManager::MatrixToGVRMatrix(QMatrix4x4 m)
{
    gvr::Mat4f g;
    for (int i = 0; i < 4; i++){
        for (int j = 0; i < 4; j++){
            g.m[i][j] = m(i,j);
        }
    }
    return g;
}

QMatrix4x4 GVRManager::PerspectiveMatrixFromView(const gvr::Rectf& fov, float near_clip, float far_clip)
{
    gvr::Mat4f result;
    const float x_left = -tan(fov.left * M_PI / 180.0f) * near_clip;
    const float x_right = tan(fov.right * M_PI / 180.0f) * near_clip;
    const float y_bottom = -tan(fov.bottom * M_PI / 180.0f) * near_clip;
    const float y_top = tan(fov.top * M_PI / 180.0f) * near_clip;
    const float zero = 0.0f;

    const float X = (2 * near_clip) / (x_right - x_left);
    const float Y = (2 * near_clip) / (y_top - y_bottom);
    const float A = (x_right + x_left) / (x_right - x_left);
    const float B = (y_top + y_bottom) / (y_top - y_bottom);
    const float C = (near_clip + far_clip) / (near_clip - far_clip);
    const float D = (2 * near_clip * far_clip) / (near_clip - far_clip);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = 0.0f;
        }
    }
    result.m[0][0] = X;
    result.m[0][2] = A;
    result.m[1][1] = Y;
    result.m[1][2] = B;
    result.m[2][2] = C;
    result.m[2][3] = D;
    result.m[3][2] = -1;

    return GVRMatrixToMatrix(result);
}

QMatrix4x4 GVRManager::SymmetricPerspectiveMatrixFromView(const gvr::Rectf& lfov, const gvr::Rectf& rfov, float near_clip, float far_clip)
{
    gvr::Mat4f result;

    //qDebug() << "LEFT" << lfov.left << lfov.right << lfov.bottom << lfov.top;
    //qDebug() << "RIGHT" << rfov.left << rfov.right << rfov.bottom << rfov.top;

    float left = qMax(lfov.left, rfov.left);
    float right = qMax(lfov.right, rfov.right);
    float bottom = (lfov.bottom + rfov.bottom)/2;
    float top = (lfov.top + rfov.top)/2;

    const float x_left = -tan(left * M_PI / 180.0f) * near_clip;
    const float x_right = tan(right * M_PI / 180.0f) * near_clip;
    const float y_bottom = -tan(bottom * M_PI / 180.0f) * near_clip;
    const float y_top = tan(top * M_PI / 180.0f) * near_clip;
    const float zero = 0.0f;

    const float X = (2 * near_clip) / (x_right - x_left);
    const float Y = (2 * near_clip) / (y_top - y_bottom);
    const float A = (x_right + x_left) / (x_right - x_left);
    const float B = (y_top + y_bottom) / (y_top - y_bottom);
    const float C = (near_clip + far_clip) / (near_clip - far_clip);
    const float D = (2 * near_clip * far_clip) / (near_clip - far_clip);

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = 0.0f;
        }
    }
    result.m[0][0] = X;
    //result.m[0][2] = A;
    result.m[1][1] = Y;
    //result.m[1][2] = B;
    result.m[2][2] = C;
    result.m[2][3] = D;
    result.m[3][2] = -1;

    return GVRMatrixToMatrix(result);
}
