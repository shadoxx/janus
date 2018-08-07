#include "renderergl33_loadingthread.h"

RendererGL33_LoadingThread::RendererGL33_LoadingThread()
{
    qDebug() << "RendererGL33_LoadingThread::RendererGL33_LoadingThread()";
}

RendererGL33_LoadingThread::~RendererGL33_LoadingThread()
{
    qDebug() << "RendererGL33_LoadingThread::~RendererGL33_LoadingThread() start";
    emit EndChildThread();
    m_loading_thread->quit();
    while (m_loading_thread->isFinished() == false)
    {
//        qDebug() << "caught here";
       QThread::sleep(0.1);
    }
    // end decoupled-thread here so that it doesn't try to render
    // with an invalid main-thread parter renderer.
    qDebug() << "RendererGL33_LoadingThread::~RendererGL33_LoadingThread() done";
}

void RendererGL33_LoadingThread::InitializeRenderingThread()
{
//    qDebug() << "RendererGL33_LoadingThread::InitializeRenderingThread()";
    m_loading_thread = new QThread();
    m_loading_thread->setObjectName("RendererGL33RenderThread");
    m_loading_worker = new RendererGL33RenderThread(this);
    m_loading_worker->moveToThread(m_loading_thread);

    // These signals are asynconous
    QObject::connect(m_loading_worker, SIGNAL(finished()),
                     m_loading_thread, SLOT(quit()));
    QObject::connect(m_loading_worker, SIGNAL(finished()),
                     m_loading_worker, SLOT(deleteLater()));

    // This will make the worker emit it's finished signal, which
    // will queue the worker for deletion on the child-thread's next event loop cycle.
    // This will also make the thread quit() which will emit it's finished event
    // triggering it's own deletion on the main-thread's next event loop cycle.
    // This is a blocking event
    QObject::connect(this, SIGNAL(EndChildThread()),
                     m_loading_worker, SLOT(FinishThread()),
                     Qt::ConnectionType::BlockingQueuedConnection);

    // These signals are syncronous will block until all SLOT callbacks are complete
    QObject::connect(this,             SIGNAL(InitializeGLContext(QOpenGLContext*)),
                     m_loading_worker, SLOT(InitializeGLContext(QOpenGLContext*)),
                     Qt::ConnectionType::BlockingQueuedConnection);

    QObject::connect(this,             SIGNAL(CreateMeshHandleForGeomVBODataMIRRORCOPY(AbstractRenderer *, GeomVBOData *)),
                     m_loading_worker, SLOT(CreateMeshHandleForGeomVBODataMIRRORCOPY(AbstractRenderer *, GeomVBOData *)),
                     Qt::ConnectionType::BlockingQueuedConnection);

    QObject::connect(this,             SIGNAL(CreateMeshHandle(AbstractRenderer *, std::shared_ptr<MeshHandle> *,
                                                               VertexAttributeLayout)),
                     m_loading_worker, SLOT(CreateMeshHandle(AbstractRenderer *, std::shared_ptr<MeshHandle> *,
                                                             VertexAttributeLayout)),
                     Qt::ConnectionType::BlockingQueuedConnection);

    QObject::connect(this,             SIGNAL(InitializeGLObjectsMIRROR(AbstractRenderer *)),
                     m_loading_worker, SLOT(InitializeGLObjectsMIRROR(AbstractRenderer *)),
                     Qt::ConnectionType::BlockingQueuedConnection);

    QObject::connect(this,             SIGNAL(Process(AbstractRenderer *, QHash<size_t, QVector<AbstractRenderCommand>> *, QHash<StencilReferenceValue, LightContainer> *)),
                     m_loading_worker, SLOT(Process(AbstractRenderer *, QHash<size_t, QVector<AbstractRenderCommand>> *, QHash<StencilReferenceValue, LightContainer> *)),
                     Qt::ConnectionType::BlockingQueuedConnection);

    QOpenGLContext * current_context = QOpenGLContext::currentContext();
    auto current_surface = current_context->surface();
    current_context->doneCurrent();

    auto main_format = current_context->format();
    QOpenGLContext * new_context = new QOpenGLContext();
    new_context->setFormat(main_format);
    new_context->setShareContext(current_context);
    new_context->create();
    new_context->doneCurrent();
    current_context->setShareContext(new_context);
    new_context->moveToThread(m_loading_thread);
    current_context->makeCurrent(current_surface);

    m_loading_thread->start();
    emit InitializeGLContext(new_context);
    m_loading_worker->Initialize();
}

void RendererGL33_LoadingThread::Initialize()
{
//    qDebug() << "RendererGL33_LoadingThread::Initialize()";
    m_main_thread_renderer = this;
    PostConstructorInitialize();
#ifdef __ANDROID__
    m_name = QString("OpenGL ES 3.1");
#else
    m_name = QString("OpenGL 3.3");
#endif

    InitializeRenderingThread();

    // Object Shader
    QString default_object_vertex_shader_path("assets/shaders/vertex.txt");
    QByteArray default_object_vertex_shader_bytes = MathUtil::LoadAssetFile(default_object_vertex_shader_path);

    QString default_no_alpha_fragment_shader_path("assets/shaders/default_no_alpha.txt");
    QByteArray default_no_alpha_fragment_shader_bytes = MathUtil::LoadAssetFile(default_no_alpha_fragment_shader_path);

    m_default_object_shader = CompileAndLinkShaderProgram(&default_object_vertex_shader_bytes, default_object_vertex_shader_path,
                                                          &default_no_alpha_fragment_shader_bytes, default_no_alpha_fragment_shader_path);

    QString default_binary_alpha_fragment_shader_path("assets/shaders/default_binary_alpha.txt");
    QByteArray default_binary_alpha_fragment_shader_bytes = MathUtil::LoadAssetFile(default_binary_alpha_fragment_shader_path);

    m_default_object_shader_binary_alpha = CompileAndLinkShaderProgram(&default_object_vertex_shader_bytes, default_object_vertex_shader_path,
                                                                     &default_binary_alpha_fragment_shader_bytes, default_binary_alpha_fragment_shader_path);

    QString default_linear_alpha_fragment_shader_path("assets/shaders/default_linear_alpha.txt");
    QByteArray ddefault_linear_alpha_fragment_shader_bytes = MathUtil::LoadAssetFile(default_linear_alpha_fragment_shader_path);

    m_default_object_shader_linear_alpha = CompileAndLinkShaderProgram(&default_object_vertex_shader_bytes, default_object_vertex_shader_path,
                                                                     &ddefault_linear_alpha_fragment_shader_bytes, default_linear_alpha_fragment_shader_path);

    // Skybox Shader
    QString default_cubemap_vertex_shader_path("assets/shaders/cubemap_vert.txt");
    QByteArray default_cubemap_vertex_shader_bytes = MathUtil::LoadAssetFile(default_cubemap_vertex_shader_path);

    QString default_cubemap_fragment_shader_path("assets/shaders/cubemap_frag.txt");
    QByteArray default_cubemap_fragment_shader_bytes = MathUtil::LoadAssetFile(default_cubemap_fragment_shader_path);

    m_default_skybox_shader = CompileAndLinkShaderProgram(&default_cubemap_vertex_shader_bytes, default_cubemap_vertex_shader_path,
                                                          &default_cubemap_fragment_shader_bytes, default_cubemap_fragment_shader_path);
    // Portal Shader
    QString default_portal_vertex_shader_path("assets/shaders/vertex.txt");
    QByteArray default_portal_vertex_shader_bytes = MathUtil::LoadAssetFile(default_portal_vertex_shader_path);

    QString default_portal_fragment_shader_path("assets/shaders/portal_frag.txt");
    QByteArray default_portal_fragment_shader_bytes = MathUtil::LoadAssetFile(default_portal_fragment_shader_path);

    m_default_portal_shader = CompileAndLinkShaderProgram(&default_portal_vertex_shader_bytes, default_portal_vertex_shader_path,
                                                          &default_portal_fragment_shader_bytes, default_portal_fragment_shader_path);

    // Cubemap to equi shader
    QString default_equi_vertex_shader_path("assets/shaders/vertex.txt");
    QByteArray default_equi_vertex_shader_bytes = MathUtil::LoadAssetFile(default_equi_vertex_shader_path);

    QString default_equi_fragment_shader_path("assets/shaders/cubemap_to_equi_frag2.txt");
    QByteArray default_equi_fragment_shader_bytes = MathUtil::LoadAssetFile(default_equi_fragment_shader_path);

    m_default_equi_shader = CompileAndLinkShaderProgram(&default_equi_vertex_shader_bytes, default_equi_vertex_shader_path,
                                                          &default_equi_fragment_shader_bytes, default_equi_fragment_shader_path);
}

void RendererGL33_LoadingThread::PreRender(QHash<size_t, QVector<AbstractRenderCommand> > * p_scoped_render_commands, QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers)
{
    Q_UNUSED(p_scoped_render_commands)
    Q_UNUSED(p_scoped_light_containers)
}

void RendererGL33_LoadingThread::PostRender(QHash<size_t, QVector<AbstractRenderCommand> > * p_scoped_render_commands, QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers)
{
    Q_UNUSED(p_scoped_render_commands)
    Q_UNUSED(p_scoped_light_containers)
}

void RendererGL33_LoadingThread::UpgradeShaderSource(QByteArray & p_shader_source, bool p_is_vertex_shader)
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
        prependDataInShaderMainFunction(p_shader_source, g_gamma_correction_GLSL);
    }
}

std::shared_ptr<ProgramHandle> RendererGL33_LoadingThread::CompileAndLinkShaderProgram(QByteArray * p_vertex_shader, QString p_vertex_shader_path, QByteArray * p_fragment_shader, QString p_fragment_shader_path)
{
//    qDebug() << "RendererGL33_LoadingThread::CompileAndLinkShaderProgram" << this;
    std::shared_ptr<ProgramHandle> handle_id = nullptr;
    CompileAndLinkShaderProgram2(&handle_id, p_vertex_shader, p_vertex_shader_path, p_fragment_shader, p_fragment_shader_path, &m_uniform_locs);
    return handle_id;
}

void RendererGL33_LoadingThread::CompileAndLinkShaderProgram2(std::shared_ptr<ProgramHandle> * p_abstract_program, QByteArray * p_vertex_shader,
                                                            QString p_vertex_shader_path, QByteArray * p_fragment_shader, QString p_fragment_shader_path,
                                                            QVector<QVector<GLint>> *p_map)
{
//    qDebug() << "RendererGL33_LoadingThread::CompileAndLinkShaderProgram2";
    GLuint program_id;
    *p_abstract_program = CreateProgramHandle(&program_id);

    GLuint vertex_shader_id = MathUtil::glFuncs->glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader_id = MathUtil::glFuncs->glCreateShader(GL_FRAGMENT_SHADER);
    GLint vertex_compile_result = GL_FALSE;
    GLint fragment_compile_result = GL_FALSE;
    GLint program_link_result = GL_FALSE;

    bool shader_failed = false;
    bool vertex_empty = false;
    bool fragment_empty = false;

#ifndef __ANDROID__
    if (p_vertex_shader->contains("#version 330 core"))
#else
    if (p_vertex_shader->contains("#version 310 es"))
#endif
    {
        UpgradeShaderSource(*p_vertex_shader, true);
#ifdef __ANDROID__
        const char * shader_data = p_vertex_shader->data();
#else
        GLchar * shader_data = p_vertex_shader->data();
#endif
        GLint shader_data_size = p_vertex_shader->size();
        MathUtil::glFuncs->glShaderSource(vertex_shader_id, 1, &shader_data, &shader_data_size);
        MathUtil::glFuncs->glCompileShader(vertex_shader_id);
        MathUtil::glFuncs->glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &vertex_compile_result);

        if (vertex_compile_result == GL_FALSE)
        {
            shader_failed = true;
            int log_length;
            MathUtil::glFuncs->glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &log_length);
            QVector<char> vertex_shader_log((log_length > 1) ? log_length : 1);
            MathUtil::glFuncs->glGetShaderInfoLog(vertex_shader_id, log_length, NULL, &vertex_shader_log[0]);
            MathUtil::ErrorLog(QString("Compilation of vertex shader ") + p_vertex_shader_path + QString("failed:")+vertex_shader_log.data());
        }
    }
    else
    {
        vertex_empty = true;
        QString default_object_vertex_shader_path(MathUtil::GetApplicationPath() + "assets/shaders/vertex.txt");
        QFile default_object_vertex_shader_file(default_object_vertex_shader_path);
        default_object_vertex_shader_file.open(QIODevice::ReadOnly | QIODevice::Text);
        QByteArray default_object_vertex_shader_bytes = default_object_vertex_shader_file.readAll();
        default_object_vertex_shader_file.close();

        UpgradeShaderSource(default_object_vertex_shader_bytes, true);
#ifdef __ANDROID__
        const char * shader_data = default_object_vertex_shader_bytes.data();
#else
        GLchar * shader_data = default_object_vertex_shader_bytes.data();
#endif
        GLint shader_data_size = default_object_vertex_shader_bytes.size();
        MathUtil::glFuncs->glShaderSource(vertex_shader_id, 1, &shader_data, &shader_data_size);
        MathUtil::glFuncs->glCompileShader(vertex_shader_id);
        MathUtil::glFuncs->glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &vertex_compile_result);

        if (vertex_compile_result == GL_FALSE)
        {
            shader_failed = true;
            int log_length;
            MathUtil::glFuncs->glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &log_length);
            QVector<char> vertex_shader_log((log_length > 1) ? log_length : 1);
            MathUtil::glFuncs->glGetShaderInfoLog(vertex_shader_id, log_length, NULL, &vertex_shader_log[0]);
            MathUtil::ErrorLog(QString("Compilation of vertex shader ") + default_object_vertex_shader_path + QString(" failed:")+vertex_shader_log.data());
        }
    }

#ifndef __ANDROID__
    if (!shader_failed && p_fragment_shader->contains("#version 330 core"))
#else
    if (!shader_failed && p_fragment_shader->contains("#version 310 es"))
#endif
    {
        UpgradeShaderSource(*p_fragment_shader, false);
#ifdef __ANDROID__
        const char * shader_data = p_fragment_shader->data();
#else
        GLchar * shader_data = p_fragment_shader->data();
#endif
        GLint shader_data_size = p_fragment_shader->size();
        MathUtil::glFuncs->glShaderSource(fragment_shader_id, 1, &shader_data, &shader_data_size);
        MathUtil::glFuncs->glCompileShader(fragment_shader_id);
        MathUtil::glFuncs->glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &fragment_compile_result);

        if (fragment_compile_result == GL_FALSE)
        {
            shader_failed = true;
            int log_length;
            MathUtil::glFuncs->glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &log_length);
            QVector<char> fragment_shader_lod((log_length > 1) ? log_length : 1);
            MathUtil::glFuncs->glGetShaderInfoLog(fragment_shader_id, log_length, NULL, &fragment_shader_lod[0]);
            MathUtil::ErrorLog(QString("Compilation of fragment shader ") + p_fragment_shader_path + QString(" failed:")+fragment_shader_lod.data());
        }
    }
    else
    {
        fragment_empty = true;
        QString default_object_fragment_shader_path("assets/shaders/default_no_alpha.txt");
        QByteArray default_object_fragment_shader_bytes = MathUtil::LoadAssetFile(default_object_fragment_shader_path);

        UpgradeShaderSource(default_object_fragment_shader_bytes, false);
#ifdef __ANDROID__
        const char * shader_data = default_object_fragment_shader_bytes.data();
#else
        GLchar * shader_data = default_object_fragment_shader_bytes.data();
#endif
        GLint shader_data_size = default_object_fragment_shader_bytes.size();
        MathUtil::glFuncs->glShaderSource(fragment_shader_id, 1, &shader_data, &shader_data_size);
        MathUtil::glFuncs->glCompileShader(fragment_shader_id);
        MathUtil::glFuncs->glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &fragment_compile_result);

        if (fragment_compile_result == GL_FALSE)
        {
            shader_failed = true;
            int log_length;
            MathUtil::glFuncs->glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &log_length);
            QVector<char> fragment_shader_lod((log_length > 1) ? log_length : 1);
            MathUtil::glFuncs->glGetShaderInfoLog(fragment_shader_id, log_length, NULL, &fragment_shader_lod[0]);
            MathUtil::ErrorLog(QString("Compilation of fragment shader ") + default_object_fragment_shader_path + QString(" failed:") + fragment_shader_lod.data());
        }
    }

//    qDebug() << "RendererGL33_LoadingThread::CompileAndLinkShaderProgram2" << shader_failed << vertex_empty << fragment_empty;

    if (!shader_failed && (!vertex_empty || !fragment_empty))
    {
        MathUtil::glFuncs->glAttachShader(program_id, vertex_shader_id);
        MathUtil::glFuncs->glAttachShader(program_id, fragment_shader_id);
        MathUtil::glFuncs->glLinkProgram(program_id);
        MathUtil::glFuncs->glGetProgramiv(program_id, GL_LINK_STATUS, &program_link_result);

        if (program_link_result == GL_FALSE)
        {
            int log_length;
            MathUtil::glFuncs->glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
            QVector<char> program_log( (log_length > 1) ? log_length : 1 );
            MathUtil::glFuncs->glGetProgramInfoLog(program_id, log_length, NULL, &program_log[0]);

            shader_failed = true;
            MathUtil::ErrorLog(QString("Linking of shaders ") + p_vertex_shader_path + QString(" & ") + p_fragment_shader_path + QString(" failed:") + program_log.data());
        }

//        qDebug() << "Vertex glDetachShader:" << program_id << "," << vertex_shader_id;
        MathUtil::glFuncs->glDetachShader(program_id, vertex_shader_id);

//        qDebug() << "Vertex glDeleteShader:" << vertex_shader_id;
        MathUtil::glFuncs->glDeleteShader(vertex_shader_id);

//        qDebug() << "Fragment glDetachShader:" << program_id << "," << fragment_shader_id;
        MathUtil::glFuncs->glDetachShader(program_id, fragment_shader_id);

//        qDebug() << "Fragment glDeleteShader:" << fragment_shader_id;
        MathUtil::glFuncs->glDeleteShader(fragment_shader_id);
    }

    // If we failed just return the default object shader
    if (shader_failed || (vertex_empty && fragment_empty))
    {
        (*p_abstract_program) = m_default_object_shader;
    }
    else
    {
        int log_length;
        MathUtil::glFuncs->glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
        QVector<char> program_log( (log_length > 1) ? log_length : 1 );
        MathUtil::glFuncs->glGetProgramInfoLog(program_id, log_length, NULL, &program_log[0]);
//        MathUtil::ErrorLog(QString("Linking of shaders ") + p_vertex_shader_path + QString(" & ") + p_fragment_shader_path + QString(" successful:"));
//        MathUtil::ErrorLog(program_log.data());

        MathUtil::glFuncs->glUseProgram(program_id);
        CacheUniformLocations(program_id, &m_uniform_locs);
        CacheUniformLocations(program_id, p_map);
    }
}

void RendererGL33_LoadingThread::CreateMeshHandleForGeomVBOData(GeomVBOData * p_VBO_data)
{
    emit CreateMeshHandleForGeomVBODataMIRRORCOPY((AbstractRenderer*)this, p_VBO_data);
}

std::shared_ptr<MeshHandle> RendererGL33_LoadingThread::CreateMeshHandle(VertexAttributeLayout p_layout)
{
    std::shared_ptr<MeshHandle> handle_id = nullptr;
    emit CreateMeshHandle((AbstractRenderer*)this, &handle_id, p_layout);
    return handle_id;
}

void RendererGL33_LoadingThread::InitializeGLObjects()
{
    AbstractRenderer::InitializeGLObjects();
}

void RendererGL33_LoadingThread::Render(QHash<size_t, QVector<AbstractRenderCommand>> * p_scoped_render_commands,
                          QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers)
{
    m_loading_worker->Process((AbstractRenderer*)this, p_scoped_render_commands, p_scoped_light_containers);
}

#ifdef __ANDROID__
void RendererGL33_LoadingThread::Pause()
{
    m_loading_worker->Pause();
}

void RendererGL33_LoadingThread::Resume()
{
    m_loading_worker->Resume();
}
#endif
