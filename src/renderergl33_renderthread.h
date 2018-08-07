#ifndef RENDERERGL33_RENDERTHREAD_H
#define RENDERERGL33_RENDERTHREAD_H

#include <QObject>
#include <QImage>
#include <QWindow>

#include "abstractrenderer.h"
#include "jniutil.h"

class RendererGL33RenderThread : public QObject, public AbstractRenderer
{
    Q_OBJECT
    QThread workerThread;
public:
    RendererGL33RenderThread(AbstractRenderer* p_main_thread_renderer);
    virtual ~RendererGL33RenderThread();

    virtual void Initialize();
    virtual void Render(QHash<size_t, QVector<AbstractRenderCommand>> * p_scoped_render_commands,
                        QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers);
    virtual void PreRender(QHash<size_t, QVector<AbstractRenderCommand> > * p_scoped_render_commands,
                           QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers);
    virtual void PostRender(QHash<size_t, QVector<AbstractRenderCommand> > * p_scoped_render_commands,
                            QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers);
    virtual void UpgradeShaderSource(QByteArray & p_shader_source, bool p_is_vertex_shader);

    void SaveScreenshot();

    //QMutex m_mesh_deletion_guard;

#ifdef __ANDROID__
    void Pause();
    void Resume();
#endif

public slots:
    void PrintFPS();
    void InitializeGLContext(QOpenGLContext* p_gl_context);
    void CreateMeshHandleForGeomVBODataMIRRORCOPY(AbstractRenderer * p_main_thread_renderer, GeomVBOData * p_VBO_data);
    void CreateMeshHandle(AbstractRenderer * p_main_thread_renderer, std::shared_ptr<MeshHandle> * p_handle,
                            VertexAttributeLayout p_layout);
    void Process(AbstractRenderer * p_main_thread_renderer, QHash<size_t, QVector<AbstractRenderCommand>> * p_scoped_object_render_commands, QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers);
    void InitializeGLObjectsMIRROR(AbstractRenderer * p_renderer);
    void DecoupledRender();
    void FinishThread();

signals:
    void finished();
    void error(QString err);

private:
    void UpdatePerObjectData(QHash<size_t, QVector<AbstractRenderCommand> > * p_scoped_render_commands);
    QOffscreenSurface *  m_gl_surface;
    QOpenGLContext * m_gl_context;
#ifndef __ANDROID__
    QOpenGLFunctions_3_3_Core * m_gl_funcs;
#else
    QOpenGLExtraFunctions * m_gl_funcs;
    bool paused;
#endif
    GLuint m_main_thread_fbo;
    std::shared_ptr<TextureHandle> m_equi_cubemap_handle;
    uint32_t m_equi_cubemap_face_size;
    void RenderEqui();
    bool m_is_rendering;
    bool m_is_initialized;
    bool m_hmd_initialized;
    bool m_capture_frame;
    QThread* m_thread;
    QTimer* m_timer;
    QTimer* m_fps_timer;
    qint64  m_frame_time;
    bool m_screenshot_pbo_pending;
    GLuint m_screenshot_pbo;
    bool m_frame_vector_sorted;
    QVector<AbstractRenderCommand_sort> m_sorted_command_indices;
    bool m_shutting_down;   
};

#endif // RendererGL33RenderThread_H
