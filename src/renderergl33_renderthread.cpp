#include "renderergl33_renderthread.h"

RendererGL33RenderThread::RendererGL33RenderThread(AbstractRenderer* p_main_thread_renderer)
    : AbstractRenderer(p_main_thread_renderer),
      m_gl_surface(nullptr),
      m_gl_context(nullptr),
      m_gl_funcs(nullptr),
      m_main_thread_fbo(0),
      m_is_rendering(false),
      m_is_initialized(false),
      m_hmd_initialized(false),
      m_capture_frame(false),
      m_thread(nullptr),
      m_timer(nullptr),
      m_fps_timer(nullptr),
      m_frame_time(0),
      m_screenshot_pbo_pending(false),
      m_screenshot_pbo(0),
      m_frame_vector_sorted(false),
      m_shutting_down(false)
{
#ifdef __ANDROID__
    paused = false;
#endif
}

RendererGL33RenderThread::~RendererGL33RenderThread()
{

}

void RendererGL33RenderThread::Initialize()
{
    PostConstructorInitialize();
    m_meshes_pending_deletion.reserve(1024);
}

void RendererGL33RenderThread::CreateMeshHandleForGeomVBODataMIRRORCOPY(AbstractRenderer * p_main_thread_renderer, GeomVBOData * p_VBO_data)
{
    // Calling CreateMeshHandleForGeomVBOData here will cause the VAO to be created on the render-thread's GL Context, any attempt to bind a
    // VAO obtained from the MeshHandles stored in GeomVBOData on the main-thread will cause wierd behaviour
    // as those VAO ID are not guaranteed to even exist in the main-thread GL context
    //CreateMeshHandleForGeomVBOData(p_VBO_data);
	if (p_main_thread_renderer == nullptr || p_VBO_data == nullptr)
	{
		qDebug("Crash Catch");
		return;
	}


/*#ifdef __ANDROID__
    int32_t float_type = GL_HALF_FLOAT;
    int32_t float_size = sizeof(half_float::half);
#else*/
    int32_t float_type = GL_FLOAT;
    int32_t float_size = sizeof(float);
//#endif

    VertexAttributeLayout layout;
    layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use = true;
    layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_count = 3;
    layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_type = float_type;
    layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].stride_in_bytes = 4 * float_size;
    layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].offset_in_bytes = 0;

    layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].in_use = true;
    layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_count = 3;
    layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_type = float_type;
    layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].stride_in_bytes = 4 * float_size;
    layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].offset_in_bytes = 0;

    layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].in_use = true;
    layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_count = 2;
    layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_type = float_type;
    layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].stride_in_bytes = 4 * float_size;
    layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].offset_in_bytes = 0;

    layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD1].in_use = true;
    layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD1].buffer_id = VAO_ATTRIB::TEXCOORD0;
    layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD1].element_count = 2;
    layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD1].element_type = float_type;
    layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD1].stride_in_bytes = 4 * float_size;
    layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD1].offset_in_bytes = 2 * float_size;

    layout.attributes[(uint32_t)VAO_ATTRIB::COLOR].in_use = true;
    layout.attributes[(uint32_t)VAO_ATTRIB::COLOR].element_count = 4;
    layout.attributes[(uint32_t)VAO_ATTRIB::COLOR].element_type = float_type;
    layout.attributes[(uint32_t)VAO_ATTRIB::COLOR].stride_in_bytes = 4 * float_size;
    layout.attributes[(uint32_t)VAO_ATTRIB::COLOR].offset_in_bytes = 0;

    layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMINDICES].in_use = p_VBO_data->use_skelanim ? true : false;
    layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMINDICES].element_count = 4;
    layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMINDICES].element_type = GL_UNSIGNED_BYTE;
    layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMINDICES].is_normalized = false;
    layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMINDICES].is_float_attrib = false; // We want a uvec4 not a vec4
    layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMINDICES].stride_in_bytes = 4 * sizeof(uint8_t);
    layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMINDICES].offset_in_bytes = 0;

    layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMWEIGHTS].in_use = p_VBO_data->use_skelanim ? true : false;
    layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMWEIGHTS].element_count = 4;
    layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMWEIGHTS].element_type = float_type;
    layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMWEIGHTS].stride_in_bytes = 4 * float_size;
    layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMWEIGHTS].offset_in_bytes = 0;

    layout.attributes[(uint32_t)VAO_ATTRIB::INDICES].in_use = true;
    layout.attributes[(uint32_t)VAO_ATTRIB::INDICES].element_count = 1;
    layout.attributes[(uint32_t)VAO_ATTRIB::INDICES].element_type = GL_UNSIGNED_INT;
    layout.attributes[(uint32_t)VAO_ATTRIB::INDICES].stride_in_bytes = 1 * sizeof(uint32_t);
    layout.attributes[(uint32_t)VAO_ATTRIB::INDICES].offset_in_bytes = 0;

    CreateMeshHandle(p_main_thread_renderer, &p_VBO_data->m_mesh_handle,
                     layout);
    p_main_thread_renderer->BindMeshHandle(p_VBO_data->m_mesh_handle.get());

    auto buffer_handles = p_main_thread_renderer->GetBufferHandlesForMeshHandle(p_VBO_data->m_mesh_handle.get());

    p_main_thread_renderer->BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::INDICES].get());
    MathUtil::glFuncs->glBufferData(GL_ELEMENT_ARRAY_BUFFER, p_VBO_data->m_indices.size() * sizeof(uint32_t), p_VBO_data->m_indices.data(), GL_STATIC_DRAW);

    p_main_thread_renderer->BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::POSITION].get());
    MathUtil::glFuncs->glBufferData(GL_ARRAY_BUFFER, p_VBO_data->m_positions.size() * float_size, p_VBO_data->m_positions.data(), GL_STATIC_DRAW);

    p_main_thread_renderer->BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::NORMAL].get());
    MathUtil::glFuncs->glBufferData(GL_ARRAY_BUFFER, p_VBO_data->m_normals.size() * float_size, p_VBO_data->m_normals.data(), GL_STATIC_DRAW);

    p_main_thread_renderer->BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::TEXCOORD0].get());
    MathUtil::glFuncs->glBufferData(GL_ARRAY_BUFFER, p_VBO_data->m_tex_coords.size() * float_size, p_VBO_data->m_tex_coords.data(), GL_STATIC_DRAW);

    p_main_thread_renderer->BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::COLOR].get());
    MathUtil::glFuncs->glBufferData(GL_ARRAY_BUFFER, p_VBO_data->m_colors.size() * float_size, p_VBO_data->m_colors.data(), GL_STATIC_DRAW);

    if (p_VBO_data->use_skelanim)
    {
        p_main_thread_renderer->BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::SKELANIMINDICES].get());
        MathUtil::glFuncs->glBufferData(GL_ARRAY_BUFFER, p_VBO_data->m_skel_anim_indices.size() * sizeof(uint8_t), p_VBO_data->m_skel_anim_indices.data(), GL_STATIC_DRAW);

        p_main_thread_renderer->BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::SKELANIMWEIGHTS].get());
        MathUtil::glFuncs->glBufferData(GL_ARRAY_BUFFER, p_VBO_data->m_skel_anim_weights.size() * float_size, p_VBO_data->m_skel_anim_weights.data(), GL_STATIC_DRAW);
    }

    // I call this to ensure that multiple queued up calls of this function don't cause large gaps between DecoupledRender calls.
    DecoupledRender();
}

void RendererGL33RenderThread::CreateMeshHandle(AbstractRenderer *p_main_thread_renderer, std::shared_ptr<MeshHandle> *p_handle, VertexAttributeLayout p_layout)
{
    // Calling CreateMeshHandle here will cause the VAO to be created on the render-thread's GL Context, any attempt to bind a
    // VAO obtained from the MeshHandles stored in GeomVBOData on the main-thread will cause wierd behaviour
    // as those VAO ID are not guaranteed to even exist in the main-thread GL context
    uint32_t VAO_id = 0;
    MathUtil::glFuncs->glGenVertexArrays(1, &VAO_id);

    *p_handle = p_main_thread_renderer->CreateMeshHandle(p_layout, VAO_id);

    // I call this to ensure that multiple queued up calls of this function don't cause large gaps between DecoupledRender calls.
    DecoupledRender();
}

void RendererGL33RenderThread::Render(QHash<size_t, QVector<AbstractRenderCommand>> * p_scoped_render_commands,
                                 QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers)
{
    Q_UNUSED(p_scoped_render_commands)
    Q_UNUSED(p_scoped_light_containers)
}

void RendererGL33RenderThread::PreRender(QHash<size_t, QVector<AbstractRenderCommand> > * p_scoped_render_commands, QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers)
{
    Q_UNUSED(p_scoped_light_containers)
    UpdatePerObjectData(p_scoped_render_commands);
}

void RendererGL33RenderThread::PostRender(QHash<size_t, QVector<AbstractRenderCommand> > * p_scoped_render_commands, QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers)
{
    Q_UNUSED(p_scoped_render_commands)
    Q_UNUSED(p_scoped_light_containers)
}

void RendererGL33RenderThread::UpgradeShaderSource(QByteArray & p_shader_source, bool p_is_vertex_shader)
{
#ifdef __ANDROID__
    p_shader_source.replace("uniform lowp vec4 iUseSkelAnim;",  "uniform lowp vec4 iUseFlags;");
    p_shader_source.replace("uniform lowp vec4 iUseLighting;",  "");
#else
    p_shader_source.replace("uniform vec4 iUseSkelAnim;",  "uniform vec4 iUseFlags;");
    p_shader_source.replace("uniform vec4 iUseLighting;",  "");
#endif
    p_shader_source.replace("iUseSkelAnim.x",                      "iUseFlags.x");
    p_shader_source.replace("iUseSkelAnim[0]",                     "iUseFlags.x");
    p_shader_source.replace("iUseLighting.x",                      "iUseFlags.y");
    p_shader_source.replace("iUseLighting[0]",                     "iUseFlags.y");

    if (!p_is_vertex_shader)
    {
        // Add gamma correction step as we output to a linear texture
        if (MathUtil::m_linear_framebuffer == false)
        {
            prependDataInShaderMainFunction(p_shader_source, g_gamma_correction_GLSL);
        }
    }
}
/*
glm::dmat4x4 dmatFromFloatArray(float * m_data)
{
    glm::dmat4x4 high_precision_matrix = glm::dmat4x4(1.0); // Identity 64-bit float matrix
    high_precision_matrix[0][0] = double(m_data[0]);
    high_precision_matrix[0][1] = double(m_data[1]);
    high_precision_matrix[0][2] = double(m_data[2]);
    high_precision_matrix[0][3] = double(m_data[3]);
    high_precision_matrix[1][0] = double(m_data[4]);
    high_precision_matrix[1][1] = double(m_data[5]);
    high_precision_matrix[1][2] = double(m_data[6]);
    high_precision_matrix[1][3] = double(m_data[7]);
    high_precision_matrix[2][0] = double(m_data[8]);
    high_precision_matrix[2][1] = double(m_data[9]);
    high_precision_matrix[2][2] = double(m_data[10]);
    high_precision_matrix[2][3] = double(m_data[11]);
    high_precision_matrix[3][0] = double(m_data[12]);
    high_precision_matrix[3][1] = double(m_data[13]);
    high_precision_matrix[3][2] = double(m_data[14]);
    high_precision_matrix[3][3] = double(m_data[15]);
    return high_precision_matrix;
}

glm::dmat4x4 dmatFromQMatrix(QMatrix4x4 const & m)
{
    glm::dmat4x4 high_precision_matrix = glm::dmat4x4(1.0); // Identity 64-bit float matrix
    float const * m_data = m.constData();
    high_precision_matrix[0][0] = double(m_data[0]);
    high_precision_matrix[0][1] = double(m_data[1]);
    high_precision_matrix[0][2] = double(m_data[2]);
    high_precision_matrix[0][3] = double(m_data[3]);
    high_precision_matrix[1][0] = double(m_data[4]);
    high_precision_matrix[1][1] = double(m_data[5]);
    high_precision_matrix[1][2] = double(m_data[6]);
    high_precision_matrix[1][3] = double(m_data[7]);
    high_precision_matrix[2][0] = double(m_data[8]);
    high_precision_matrix[2][1] = double(m_data[9]);
    high_precision_matrix[2][2] = double(m_data[10]);
    high_precision_matrix[2][3] = double(m_data[11]);
    high_precision_matrix[3][0] = double(m_data[12]);
    high_precision_matrix[3][1] = double(m_data[13]);
    high_precision_matrix[3][2] = double(m_data[14]);
    high_precision_matrix[3][3] = double(m_data[15]);
    return high_precision_matrix;
}

glm::mat4x4 matFromDmat(glm::dmat4x4 const & m)
{
    glm::mat4x4 matrix = glm::mat4x4(1.0); // Identity 64-bit float matrix
    double const * m_data = (&m[0][0]);
    matrix[0][0] = float(m_data[0]);
    matrix[0][1] = float(m_data[1]);
    matrix[0][2] = float(m_data[2]);
    matrix[0][3] = float(m_data[3]);
    matrix[1][0] = float(m_data[4]);
    matrix[1][1] = float(m_data[5]);
    matrix[1][2] = float(m_data[6]);
    matrix[1][3] = float(m_data[7]);
    matrix[2][0] = float(m_data[8]);
    matrix[2][1] = float(m_data[9]);
    matrix[2][2] = float(m_data[10]);
    matrix[2][3] = float(m_data[11]);
    matrix[3][0] = float(m_data[12]);
    matrix[3][1] = float(m_data[13]);
    matrix[3][2] = float(m_data[14]);
    matrix[3][3] = float(m_data[15]);
    return matrix;
}*/

void RendererGL33RenderThread::UpdatePerObjectData(QHash<size_t, QVector<AbstractRenderCommand>> * p_scoped_render_commands)
{
    QMatrix4x4 temp_matrix;
    QMatrix4x4 model_matrix;
    QMatrix4x4 model_view_matrix;

    QVector<float> misc_object_data;
    misc_object_data.resize(16);

    // Resize to this frame's camera vectors to fit the current frame's cameras for each scope
    for (const RENDERER::RENDER_SCOPE scope : m_scopes)
    {
        size_t const camera_count_this_scope = m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index][static_cast<size_t>(scope)].size();
        m_per_frame_scoped_cameras_view_matrix[static_cast<size_t>(scope)].resize(camera_count_this_scope);
        m_per_frame_scoped_cameras_is_left_eye[static_cast<size_t>(scope)].resize(camera_count_this_scope);
    }

    // Generate the view matrices and is_left_eye data for this frame's cameras
    if (m_main_thread_renderer->m_hmd_manager != nullptr && m_main_thread_renderer->m_hmd_manager->GetEnabled() == true)
    {
        QMatrix4x4 const eye_view_matrix_L = m_main_thread_renderer->m_hmd_manager->GetEyeViewMatrix(0);
        QMatrix4x4 const eye_view_matrix_R = m_main_thread_renderer->m_hmd_manager->GetEyeViewMatrix(1);

        for (const RENDERER::RENDER_SCOPE scope : m_scopes)
        {
            size_t const camera_count_this_scope = m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index][static_cast<size_t>(scope)].size();
            for (size_t camera_index = 0; camera_index < camera_count_this_scope; ++camera_index)
            {
                QMatrix4x4 composited_view_matrix = m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index][static_cast<size_t>(scope)][camera_index].GetViewMatrix();
                m_per_frame_scoped_cameras_is_left_eye[static_cast<size_t>(scope)][camera_index] = m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index][static_cast<size_t>(scope)][camera_index].GetLeftEye();
                composited_view_matrix = ((m_per_frame_scoped_cameras_is_left_eye[static_cast<size_t>(scope)][camera_index]) ? eye_view_matrix_L : eye_view_matrix_R) * composited_view_matrix;
                m_per_frame_scoped_cameras_view_matrix[static_cast<size_t>(scope)][camera_index] = composited_view_matrix;

                // Update camera viewports, this takes into account things like dynamic resolution scaling
                m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index][static_cast<size_t>(scope)][camera_index].SetViewport(
                            (m_per_frame_scoped_cameras_is_left_eye[static_cast<size_t>(scope)][camera_index] == true)
                        ? m_main_thread_renderer->m_hmd_manager->m_eye_viewports[0]
                        : m_main_thread_renderer->m_hmd_manager->m_eye_viewports[1]);
            }
        }
    }
    else
    {
        for (const RENDERER::RENDER_SCOPE scope : m_scopes)
        {
            size_t const camer_count_this_scope = m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index][static_cast<size_t>(scope)].size();
            for (size_t camera_index = 0; camera_index < camer_count_this_scope; ++camera_index)
            {
                m_per_frame_scoped_cameras_view_matrix[static_cast<size_t>(scope)][camera_index] = m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index][static_cast<size_t>(scope)][camera_index].GetViewMatrix();
                m_per_frame_scoped_cameras_is_left_eye[static_cast<size_t>(scope)][camera_index] = m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index][static_cast<size_t>(scope)][camera_index].GetLeftEye();
            }
        }
    }

    for (const RENDERER::RENDER_SCOPE scope : m_scopes)
    {
        QVector<AbstractRenderCommand> & render_command_vector = (*p_scoped_render_commands)[static_cast<size_t>(scope)];

        auto const command_count(render_command_vector.size());
        auto const camera_count_this_scope(m_per_frame_scoped_cameras_view_matrix[static_cast<size_t>(scope)].size());

        // For each command
        for (uint32_t command_index = 0; command_index < command_count; command_index += camera_count_this_scope)
        {
            // Recompute matrices for each camera affecting each command in this scope
            for (size_t camera_index = 0; camera_index < camera_count_this_scope; ++camera_index)
            {
                const VirtualCamera& camera = m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index][static_cast<size_t>(scope)][camera_index];

                AbstractRenderCommand & render_command = render_command_vector[command_index + camera_index];
                AssetShader_Object & new_object_uniforms = render_command.GetObjectUniformsReference();

                memcpy((char*)model_matrix.constData(), new_object_uniforms.iModelMatrix, 16 * sizeof(float));
                model_matrix.optimize(); //56.0 - call optimize so internal type is not identity and inverse does nothing

                QMatrix4x4 const & view_matrix = m_per_frame_scoped_cameras_view_matrix[static_cast<size_t>(scope)][camera_index];
                memcpy(new_object_uniforms.iViewMatrix, view_matrix.constData(), 16 * sizeof(float));

                memcpy(new_object_uniforms.iProjectionMatrix, camera.GetProjectionMatrix().constData(), 16 * sizeof(float));

                temp_matrix = view_matrix.inverted();
                memcpy(new_object_uniforms.iInverseViewMatrix, temp_matrix.constData(), 16 * sizeof(float));

                model_view_matrix = view_matrix * model_matrix;
                memcpy(new_object_uniforms.iModelViewMatrix, model_view_matrix.constData(), 16 * sizeof(float));

                temp_matrix = camera.GetProjectionMatrix() * model_view_matrix;
                memcpy(new_object_uniforms.iModelViewProjectionMatrix, temp_matrix.constData(), 16 * sizeof(float));

                temp_matrix = model_matrix.inverted().transposed();
                memcpy(new_object_uniforms.iTransposeInverseModelMatrix, temp_matrix.constData(), 16 * sizeof(float));

                temp_matrix = model_view_matrix.inverted().transposed();
                memcpy(new_object_uniforms.iTransposeInverseModelViewMatrix, temp_matrix.constData(), 16 * sizeof(float));
            }
        }
    }
}

void RendererGL33RenderThread::InitializeGLContext(QOpenGLContext * p_gl_context)
{
//    qDebug("RendererGL33RenderThread::InitializeGLContext");

    m_thread = QThread::currentThread();
    m_timer = new QTimer();
    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(DecoupledRender()));
    m_timer->start(0);

    m_fps_timer = new QTimer();
    QObject::connect(m_fps_timer, SIGNAL(timeout()), this, SLOT(PrintFPS()));
    m_fps_timer->start(500);

    m_gl_context = p_gl_context;

    m_gl_surface = new QOffscreenSurface();
    auto format = m_gl_context->format();
    m_gl_surface->setFormat(format);
    m_gl_surface->create();

    m_gl_context->makeCurrent(m_gl_surface);

#ifndef __ANDROID__
    m_gl_funcs = m_gl_context->versionFunctions<QOpenGLFunctions_3_3_Core>();
#else
    m_gl_funcs = m_gl_context->extraFunctions();
#endif

    // Create FBO to use for attaching main-thread FBO textures to for blitting
    m_main_thread_fbo = 0;
    MathUtil::glFuncs->glGenFramebuffers(1, &m_main_thread_fbo);

    AbstractRenderer::InitializeState();

#ifdef WIN32
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif
    QThread::currentThread()->setPriority(QThread::TimeCriticalPriority);

    m_gl_context->makeCurrent(m_gl_surface);

#if !defined(__APPLE__) && !defined(__ANDROID__)
    if (m_gl_context->hasExtension(QByteArrayLiteral("GL_ARB_debug_output")))
    {
        PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallbackARB = NULL;
        glDebugMessageCallbackARB = (PFNGLDEBUGMESSAGECALLBACKARBPROC)m_gl_context->getProcAddress(QByteArrayLiteral("glDebugMessageCallbackARB"));

        if (glDebugMessageCallbackARB != NULL)
        {
            qDebug() << "DEBUG OUTPUT SUPPORTED";

            glDebugMessageCallbackARB((GLDEBUGPROCARB)&MathUtil::DebugCallback, NULL);
            m_gl_funcs->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        }
        else
        {
            qDebug() << "DEBUG OUTPUT NOT SUPPORTED!";
        }
    }
#elif __ANDROID__
    if (QOpenGLContext::currentContext()->hasExtension(QByteArrayLiteral("GL_KHR_debug")))
    {
        PFNGLDEBUGMESSAGECALLBACKKHRPROC glDebugMessageCallbackKHR = NULL;
        glDebugMessageCallbackKHR = (PFNGLDEBUGMESSAGECALLBACKKHRPROC)m_gl_context->getProcAddress(QByteArrayLiteral("glDebugMessageCallbackKHR"));

        if (glDebugMessageCallbackKHR != NULL)
        {
            qDebug() << "DEBUG OUTPUT SUPPORTED";

            glDebugMessageCallbackKHR((GLDEBUGPROCKHR)&MathUtil::DebugCallback, NULL);
            m_gl_funcs->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
        }
        else
        {
            qDebug() << "DEBUG OUTPUT NOT SUPPORTED!";
        }
    }
#endif

    m_is_initialized = true;
}

void RendererGL33RenderThread::Process(AbstractRenderer * p_main_thread_renderer,
                                        QHash<size_t, QVector<AbstractRenderCommand> > *,
                                        QHash<StencilReferenceValue, LightContainer> *)
{
     // Queued up data to allow Process to not block the main-thread longer than necessary
    m_main_thread_renderer = p_main_thread_renderer;

    if (m_is_rendering == false && m_is_initialized == true)
    {
        m_is_rendering = true;
    }

    const bool do_VR = (m_main_thread_renderer->m_hmd_manager != nullptr && m_main_thread_renderer->m_hmd_manager->GetEnabled() == true);

    if (do_VR == false && MathUtil::m_frame_limiter_render_thread != 0)
    {
        m_frame_rate_limiter.release(1);
    }
}

void RendererGL33RenderThread::DecoupledRender()
{
#ifdef __ANDROID__
    while (paused && m_main_thread_renderer->m_hmd_manager && !JNIUtil::GetShowingVR())
    {
        // Need to update to see mounted state of HMD
        /*const bool do_VR = (m_main_thread_renderer->m_hmd_manager != nullptr);
        if (do_VR)
        {
            m_main_thread_renderer->m_hmd_manager->Update();
        }*/

       QThread::sleep(0.5);
    }
#endif

    if (m_is_rendering == true && m_is_initialized == true && m_shutting_down == false)
    {
        // Clean up mesh handles that are pending deletion
        /*{
            m_main_thread_renderer->m_mesh_deletion_guard.lock();
            size_t const mesh_count = m_meshes_pending_deletion.size();
            if (mesh_count != 0)
            {
                MathUtil::glFuncs->glDeleteVertexArrays(GLsizei(mesh_count), (GLuint*)m_meshes_pending_deletion.data());
                m_meshes_pending_deletion.clear();
            }
            m_main_thread_renderer->m_mesh_deletion_guard.unlock();
        }*/

        if (m_main_thread_renderer->m_hmd_manager != nullptr
#ifndef __ANDROID__
                && m_main_thread_renderer->m_hmd_manager->GetEnabled() == true // GetEnabled() functions differently for mobile
#endif
                && m_hmd_initialized == false)
        {
            m_main_thread_renderer->m_hmd_manager->InitializeGL();
            m_main_thread_renderer->m_hmd_manager->ReCentre();
            m_hmd_initialized = true;
        }

        // Prevents main-thread from reallocating vectors read by render-thread during use
        m_reallocation_guard.lock();

        // Update rendering_index if needed
        if (m_main_thread_renderer->m_current_frame_id != m_main_thread_renderer->m_submitted_frame_id)
        {
            m_main_thread_renderer->m_rendering_index = m_main_thread_renderer->m_completed_submission_index.exchange(m_main_thread_renderer->m_rendering_index);
            m_main_thread_renderer->m_current_frame_id = m_main_thread_renderer->m_submitted_frame_id;
            m_frame_vector_sorted = false;

            // Clean up mesh handles that are pending deletion, we wait until the next unique frame
            // so that we don't delete objects that are in use for the previous one
            m_main_thread_renderer->FreeMeshHandles();
            m_main_thread_renderer->FreeBufferHandles();
            m_main_thread_renderer->FreeProgramHandles();
            m_main_thread_renderer->FreeTextureHandles();
        }

        const bool do_VR = (m_main_thread_renderer->m_hmd_manager != nullptr && m_main_thread_renderer->m_hmd_manager->GetEnabled() == true);
        bool render_limit = m_frame_rate_limiter.tryAcquire((do_VR == false && MathUtil::m_frame_limiter_render_thread != 0) ? 1 : 0);
        if (render_limit == true)
        {
            m_main_thread_renderer->StartFrame();
            auto texture_size = (m_main_thread_renderer->m_hmd_manager != nullptr && m_main_thread_renderer->m_hmd_manager->GetEnabled() == true)
                    ? m_main_thread_renderer->m_hmd_manager->GetTextureSize()
                    : QSize(m_main_thread_renderer->m_window_width / 2, m_main_thread_renderer->m_window_height);

            // Override texture_size if we are a screenshot frame
            texture_size = (m_main_thread_renderer->m_screenshot_requested) ? QSize(m_main_thread_renderer->m_screenshot_width / 2, m_main_thread_renderer->m_screenshot_height) : texture_size;
            auto msaa_count = m_main_thread_renderer->GetMSAACount();
            msaa_count = (m_main_thread_renderer->m_screenshot_requested) ? m_main_thread_renderer->m_screenshot_sample_count : msaa_count;

            SetIsUsingEnhancedDepthPrecision(m_main_thread_renderer->GetIsUsingEnhancedDepthPrecision());
            ConfigureFramebuffer(texture_size.width()*2, texture_size.height(), msaa_count);
            BindFBOToDraw(FBO_TEXTURE_BITFIELD::COLOR | FBO_TEXTURE_BITFIELD::DEPTH_STENCIL, true);
            UpdateFramebuffer();
            m_main_thread_renderer->WaitforFrameSyncObject();

            // This call allows the PreRender function to get recent pose data for the HMD for this frames render
            // this is important for reducing motion-to-photon latency in VR rendering and to ensure that the timers
            // from the various HMD perf overlays show correct latency values and return us accurate predicted pose values
            if (do_VR)
            {
                m_main_thread_renderer->m_hmd_manager->Update();
            }

            PreRender(&(m_main_thread_renderer->m_scoped_render_commands_cache[m_main_thread_renderer->m_rendering_index]), &(m_main_thread_renderer->m_scoped_light_containers_cache[m_main_thread_renderer->m_rendering_index]));

            if (do_VR)
            {
                m_main_thread_renderer->m_hmd_manager->BeginRendering();
                m_main_thread_renderer->m_hmd_manager->BeginRenderEye(0);
                m_main_thread_renderer->m_hmd_manager->BeginRenderEye(1);
            }

            for (size_t scope = 0; scope < (size_t)RENDERER::RENDER_SCOPE::SCOPE_COUNT; ++scope)
            {
                auto current_scope  = static_cast<RENDERER::RENDER_SCOPE>(scope);
                RenderObjectsNaiveDecoupled(m_main_thread_renderer,
                                            current_scope,
                                            m_main_thread_renderer->m_scoped_render_commands_cache[m_main_thread_renderer->m_rendering_index][scope],
                        m_main_thread_renderer->m_scoped_light_containers_cache[m_main_thread_renderer->m_rendering_index]);
            }

            BlitMultisampledFramebuffer(FBO_TEXTURE_BITFIELD::COLOR);
            MathUtil::glFuncs->glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // TODO:: Make this function create a mip-chain for the attached fbo color texture
            // this combined with a layer for the OVRSDK allows for better filtering
            // it is also useful if we want to create a mip-chain for the depth to use for frame n+1 reprojection or object culling techniques.
            PostRender(&(m_main_thread_renderer->m_scoped_render_commands_cache[m_main_thread_renderer->m_rendering_index]), &(m_main_thread_renderer->m_scoped_light_containers_cache[m_main_thread_renderer->m_rendering_index]));

            if (MathUtil::m_do_equi
                    || ((m_main_thread_renderer->m_screenshot_requested == true)
                        && m_main_thread_renderer->m_screenshot_is_equi == true
                        && m_main_thread_renderer->m_screenshot_frame_index == m_main_thread_renderer->m_current_frame_id))
            {
                RenderEqui();
            }

            BindFBOToRead(FBO_TEXTURE_BITFIELD::COLOR, false);
            QVector<uint32_t> draw_buffers;
            draw_buffers.reserve(FBO_TEXTURE::COUNT);
            m_main_thread_renderer->BindFBOAndTextures(draw_buffers, GL_TEXTURE_2D, GL_DRAW_FRAMEBUFFER, m_main_thread_fbo, 0, FBO_TEXTURE_BITFIELD::COLOR);

            // We don't copy the equi screenshot to the main-thread FBO as we wan't to be an
            // offscreen render that doesn't cause a 1 frame flicker
            if ((m_main_thread_renderer->m_screenshot_requested == false)
                    || (m_main_thread_renderer->m_screenshot_is_equi == false)
                    || (m_main_thread_renderer->m_screenshot_frame_index != m_main_thread_renderer->m_current_frame_id))
            {
                // Bind our current FBO as read, and the main-thread textures as our draw-FBO
                // This may have issues if those same main-thread textures are bound to a FBO on the main-thread context.
                MathUtil::glFuncs->glBlitFramebuffer(0,0, m_window_width, m_window_height,
                                                     0, 0, m_main_thread_renderer->m_window_width, m_main_thread_renderer->m_window_height,
                                                     GL_COLOR_BUFFER_BIT, GL_LINEAR);
            }

            if (m_main_thread_renderer->m_screenshot_requested == true
                    && (m_main_thread_renderer->m_current_frame_id >= m_main_thread_renderer->m_screenshot_frame_index))
            {
                SaveScreenshot();
            }

            if (do_VR)
            {
                // OpenVR binds textures inside of its submission step.
                // I change the active slot here to one we do not use for normal
                // rendering to avoid the call invalidating our GL state copy.
                //
                // OpenVR needs the OpenGL handle for the texture, so we update it here.
                if (m_main_thread_renderer->m_hmd_manager->m_using_openVR)
                {
                    MathUtil::glFuncs->glActiveTexture(GL_TEXTURE15);
                    m_active_texture_slot_render = 15;
                    m_main_thread_renderer->m_hmd_manager->m_color_texture_id =  GetTextureID(FBO_TEXTURE::COLOR, false);
                }

                m_main_thread_renderer->m_hmd_manager->EndRenderEye(0);
                m_main_thread_renderer->m_hmd_manager->EndRenderEye(1);
                m_main_thread_renderer->m_hmd_manager->EndRendering();
                MathUtil::glFuncs->glFlush();
            }

            m_main_thread_renderer->LockFrameSyncObject();
            m_main_thread_renderer->EndFrame();   
        }
        // Prevents main-thread from reallocating vectors read by render-thread during use
        m_reallocation_guard.unlock();
    }
}

void RendererGL33RenderThread::SaveScreenshot()
{
    if (m_screenshot_pbo_pending == false)
    {
        GLsizei const pbo_size = m_main_thread_renderer->m_screenshot_width * m_main_thread_renderer->m_screenshot_height * sizeof(GL_UNSIGNED_BYTE) * 4; // RGBA8
        MathUtil::glFuncs->glGenBuffers(1, &m_screenshot_pbo);
        MathUtil::glFuncs->glBindBuffer(GL_PIXEL_PACK_BUFFER, m_screenshot_pbo);
        MathUtil::glFuncs->glBufferData(GL_PIXEL_PACK_BUFFER, pbo_size, 0, GL_STREAM_READ);
        MathUtil::glFuncs->glReadPixels(0, 0, m_main_thread_renderer->m_screenshot_width, m_main_thread_renderer->m_screenshot_height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        m_screenshot_pbo_pending = true;
    }
    else
    {
        GLsizei const pbo_size = m_main_thread_renderer->m_screenshot_width * m_main_thread_renderer->m_screenshot_height * sizeof(GL_UNSIGNED_BYTE) * 4; // RGBA8
        MathUtil::glFuncs->glBindBuffer(GL_PIXEL_PACK_BUFFER, m_screenshot_pbo);
        unsigned char* ptr = nullptr;
        ptr = (unsigned char*)MathUtil::glFuncs->glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, pbo_size, GL_MAP_READ_BIT);
        if (ptr != nullptr)
        {
            QString const out_filename = MathUtil::GetLastScreenshotPath();
            QImage img(ptr, m_main_thread_renderer->m_screenshot_width, m_main_thread_renderer->m_screenshot_height, QImage::Format_RGBX8888);
            img = img.mirrored();
            img.save(out_filename, "jpg", 95);
            //qDebug() << "GLWidget::SaveScreenShot() - image" << out_filename << "saved";
            MathUtil::glFuncs->glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        MathUtil::glFuncs->glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        MathUtil::glFuncs->glDeleteBuffers(1, &m_screenshot_pbo);
        m_screenshot_pbo_pending = false;
        m_main_thread_renderer->m_screenshot_requested = false;
    }
}


void RendererGL33RenderThread::PrintFPS()
{
    if (m_main_thread_renderer != nullptr && m_main_thread_renderer->m_GPUTimeQueryResults.size() != 0)
    {
        uint64_t sum_frame_time = 0;
        for (uint64_t frame_time_gpu : m_main_thread_renderer->m_GPUTimeQueryResults)
        {
            sum_frame_time += static_cast<uint64_t>(frame_time_gpu);
        }

        uint64_t sum_cpu_time = 0;
        for (uint64_t frame_time_cpu : m_main_thread_renderer->m_CPUTimeQueryResults)
        {
            sum_cpu_time += static_cast<uint64_t>(frame_time_cpu);
        }

        sum_frame_time /= m_main_thread_renderer->m_GPUTimeQueryResults.size();
        sum_cpu_time /= m_main_thread_renderer->m_CPUTimeQueryResults.size();

//        qDebug() << "GPUFPS: " << 1000000000.0 / static_cast<double>(sum_frame_time) << "CPUFPS: " << 1000000000.0 / static_cast<double>(sum_cpu_time);
    }
}

void RendererGL33RenderThread::RenderEqui()
{
    uint32_t const cube_cross_width = m_window_width;
    uint32_t const cube_cross_height = m_window_height;
    uint32_t const cube_face_dim = qMin(cube_cross_width / 3, cube_cross_height / 2);
    QVector<QVector4D> viewports;
    viewports.reserve(6);
    // This is a 3x2 grid layout to use all of the available framebuffer space
    viewports.push_back(QVector4D(cube_face_dim * 0.0f, cube_face_dim * 0.0f, cube_face_dim, cube_face_dim)); // X+
    viewports.push_back(QVector4D(cube_face_dim * 1.0f, cube_face_dim * 0.0f, cube_face_dim, cube_face_dim)); // X-
    viewports.push_back(QVector4D(cube_face_dim * 2.0f, cube_face_dim * 0.0f, cube_face_dim, cube_face_dim)); // Y+
    viewports.push_back(QVector4D(cube_face_dim * 0.0f, cube_face_dim * 1.0f, cube_face_dim, cube_face_dim)); // Y-
    viewports.push_back(QVector4D(cube_face_dim * 1.0f, cube_face_dim * 1.0f, cube_face_dim, cube_face_dim)); // Z+
    viewports.push_back(QVector4D(cube_face_dim * 2.0f, cube_face_dim * 1.0f, cube_face_dim, cube_face_dim)); // Z-

    // Create a new TextureHandle if our current is nullptr, this is either because it's the first
    // frame, or because the window changed size which nulls the existing TextureHandle.
    if (!m_equi_cubemap_handle || m_equi_cubemap_face_size != cube_face_dim)
    {
        m_equi_cubemap_handle = m_main_thread_renderer->CreateCubemapTextureHandle(cube_face_dim, cube_face_dim, TextureHandle::COLOR_SPACE::SRGB, GL_RGB, false, true, true, TextureHandle::ALPHA_TYPE::NONE, TextureHandle::COLOR_SPACE::SRGB);
        m_equi_cubemap_face_size = cube_face_dim;
    }

    BindFBOToRead(FBO_TEXTURE_BITFIELD::COLOR, false);
    BindFBOToDraw(FBO_TEXTURE_BITFIELD::NONE, false);
    m_main_thread_renderer->BindTextureHandle(&(m_main_thread_renderer->m_texture_handle_to_GL_ID), 13, m_equi_cubemap_handle.get(), true);
    for (uint32_t face_index = 0; face_index < 6; ++face_index)
    {

        uint32_t target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index;
        CopyReadBufferToTextureHandle(&(m_main_thread_renderer->m_texture_handle_to_GL_ID), m_equi_cubemap_handle.get(), target, 0, 0, 0,
                                      viewports[face_index].x(), viewports[face_index].y(),
                                      cube_face_dim, cube_face_dim);
    }


    // Use forward menu camera as we are drawing a full-screen quad
    QVector<VirtualCamera> overlay_camera;
    overlay_camera.reserve(1);
    overlay_camera.push_back(VirtualCamera(QVector3D(0.0f, 0.0f, 0.0f), QQuaternion(), QVector3D(1.0f, 1.0f, 1.0f),
                                    QVector4D(0, 0, cube_cross_width, cube_cross_height),
                                    float(cube_cross_width)/float(cube_cross_height), -1.0f, 0.1f, 10.0f));
    overlay_camera[0].SetScopeMask(RENDERER::RENDER_SCOPE::ALL, false);
    overlay_camera[0].SetScopeMask(RENDERER::RENDER_SCOPE::POST_PROCESS, true);

    // Cache existing cameras then set the cameras to our one camera needed for cubemap to equi rendering
    auto camera_cache = m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index];
    for (size_t scope_enum = 0; scope_enum < static_cast<size_t>(RENDERER::RENDER_SCOPE::SCOPE_COUNT); ++scope_enum)
    {
        m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index][scope_enum].clear();
        for (VirtualCamera& camera : overlay_camera)
        {
            if (camera.GetScopeMask(static_cast<RENDERER::RENDER_SCOPE>(scope_enum)) == true)
            {
                m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index][scope_enum].push_back(camera);
            }
        }
    }

    // Cache then erase any existing commands in the overlay scope
    QHash<size_t, QVector<AbstractRenderCommand>> post_process_commands;

    // Push the AbstractRenderCommand needed to convert the cubemap into an equi to the OVERLAYS scope
    AbstractRenderComandShaderData shader_data(m_main_thread_renderer->m_default_equi_shader.get(),
            AssetShader_Frame(),
            AssetShader_Room(),
            AssetShader_Object(),
            AssetShader_Material());

    shader_data.m_frame.iResolution = QVector4D(0, 0, cube_cross_width, cube_cross_height);
    QMatrix4x4 ident;
    ident.setToIdentity();
    memcpy(shader_data.m_object.iModelMatrix, ident.constData(), 16 * sizeof(float));

    post_process_commands[(size_t)RENDERER::RENDER_SCOPE::POST_PROCESS].push_back(
                AbstractRenderCommand(PrimitiveType::TRIANGLES,
                                       6,
                                       1,
                                       0,
                                       0,
                                       0,
                                       m_main_thread_renderer->m_plane_vao.get(),
                                       shader_data.m_program,
                                       shader_data.m_frame,
                                       shader_data.m_room,
                                       shader_data.m_object,
                                       shader_data.m_material,
                                       m_main_thread_renderer->GetCurrentlyBoundTextures(),
                                       FaceCullMode::DISABLED,
                                       DepthFunc::ALWAYS,
                                       DepthMask::DEPTH_WRITES_DISABLED,
                                       StencilFunc(StencilTestFuncion::ALWAYS, StencilReferenceValue(0), StencilMask(0xffffffff)),
                                       StencilOp(StencilOpAction::KEEP, StencilOpAction::KEEP, StencilOpAction::KEEP),
                                       PolyMode::FILL,
                                       ColorMask::COLOR_WRITES_ENABLED));

    // Do the second pass of rendering to convert cubemap to equi
    PreRender(&post_process_commands, &(m_main_thread_renderer->m_scoped_light_containers_cache[m_main_thread_renderer->m_rendering_index]));
    // This is just to trigger the clearing of the FBO
    //RenderObjectsNaiveDecoupled(m_main_thread_renderer, RENDERER::RENDER_SCOPE::CURRENT_ROOM_PORTAL_STENCILS, post_process_commands[(size_t)RENDERER::RENDER_SCOPE::POST_PROCESS], (m_scoped_light_containers));
    // This draws our full-screen quad with the cubemap-to-equi fragment shader
    BindFBOToRead(FBO_TEXTURE_BITFIELD::NONE, false);
    BindFBOToDraw(FBO_TEXTURE_BITFIELD::COLOR, false);
    RenderObjectsNaiveDecoupled(m_main_thread_renderer, RENDERER::RENDER_SCOPE::POST_PROCESS, post_process_commands[(size_t)RENDERER::RENDER_SCOPE::POST_PROCESS], m_main_thread_renderer->m_scoped_light_containers_cache[m_main_thread_renderer->m_rendering_index]);

    // Restore the cameras
    m_main_thread_renderer->m_scoped_cameras_cache[m_main_thread_renderer->m_rendering_index] = camera_cache;
}

void RendererGL33RenderThread::FinishThread()
{
    m_shutting_down = true;
    m_timer->stop();
    emit finished();
}

void RendererGL33RenderThread::InitializeGLObjectsMIRROR(AbstractRenderer * p_renderer)
{
    p_renderer->InitializeGLObjects2();
}

#ifdef __ANDROID__
void RendererGL33RenderThread::Pause()
{
    paused = true;
}

void RendererGL33RenderThread::Resume()
{
    paused = false;
}
#endif
