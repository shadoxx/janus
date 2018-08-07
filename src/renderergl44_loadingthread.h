#ifndef RENDERERGL44_LOADINGTHREAD_H
#define RENDERERGL44_LOADINGTHREAD_H

#if !defined(__APPLE__)
#include <QThread>
#include <QObject>
#include <memory>
#include "renderergl44_renderthread.h"

class RendererGL44_LoadingThread : public QObject, public AbstractRenderer
{
    Q_OBJECT
public:
    RendererGL44_LoadingThread();
    virtual ~RendererGL44_LoadingThread();
    virtual void Initialize();
    virtual void Render(QHash<size_t, QVector<AbstractRenderCommand>> * p_scoped_render_commands,
                        QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers);
    virtual void PreRender(QHash<size_t, QVector<AbstractRenderCommand> > * p_scoped_render_commands,
                           QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers);
    virtual void PostRender(QHash<size_t, QVector<AbstractRenderCommand> > * p_scoped_render_commands,
                            QHash<StencilReferenceValue, LightContainer> * p_scoped_light_containers);
    virtual void UpgradeShaderSource(QByteArray & p_shader_source, bool p_is_vertex_shader);
    virtual std::shared_ptr<ProgramHandle> CompileAndLinkShaderProgram(QByteArray * p_vertex_shader, QString p_vertex_shader_path,
                                                                              QByteArray * p_fragment_shader, QString p_fragment_shader_path);
    virtual void CreateMeshHandleForGeomVBOData(GeomVBOData * p_VBO_data);
    virtual std::shared_ptr<MeshHandle> CreateMeshHandle(VertexAttributeLayout p_layout);
    virtual void InitializeGLObjects();
    void CompileAndLinkShaderProgram2(std::shared_ptr<ProgramHandle> *p_abstract_program, QByteArray *p_vertex_shader, QString p_vertex_shader_path, QByteArray *p_fragment_shader, QString p_fragment_shader_path, QVector<QVector<GLint> > *p_map);
    //virtual void RemoveMeshHandleFromMap(MeshHandle *p_handle);
    //virtual void RemoveProgramHandleFromMap(ProgramHandle *p_handle);
public slots:

signals:
    void EndChildThread();
    void InitializeGLContext(QOpenGLContext *);
    void CreateMeshHandleForGeomVBODataMIRRORCOPY(AbstractRenderer *, GeomVBOData *);
    void CreateMeshHandle(AbstractRenderer *, std::shared_ptr<MeshHandle> *,
                          VertexAttributeLayout);
    void InitializeGLObjectsMIRROR(AbstractRenderer*);
    void Process(AbstractRenderer *, QHash<size_t, QVector<AbstractRenderCommand>> *, QHash<StencilReferenceValue, LightContainer> *);
private:
    QThread* m_loading_thread;
    RendererGL44_RenderThread * m_loading_worker;
    void InitializeRenderingThread();
    std::shared_ptr<ProgramHandle> CompileAndLinkShaderProgram(QByteArray * p_compute_shader, QString p_compute_shader_path);
};
#endif
#endif // RENDERERGL44_LOADINGTHREAD_H
