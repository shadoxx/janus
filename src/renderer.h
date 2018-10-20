#ifndef RENDERER_H
#define RENDERER_H

#include <QDebug>
#include <vector>
#include <QVector>
#include <QString>
#include <QImage>
#include <algorithm>
#include "rendererinterface.h"
#include "assetimagedata.h"
#include "renderergl33_loadingthread.h"
#ifndef __ANDROID__
#include "renderergl44_loadingthread.h"
#else
#include "jniutil.h"
#endif

class Renderer : public RendererInterface
{
public:
	Renderer();
    ~Renderer();

	static Renderer * GetSingleton()
	{
		static Renderer * const singleton = new Renderer();
		return singleton;
	}

    uint32_t CreateVAO();
    uint32_t DeleteVAO();

    void Initialize(const QString& p_requested_gl_version);
    void InitializeScopes();
    void InitializeState();
    void InitializeLightUBOs();
    void InitializeHMDManager(QPointer <AbstractHMDManager> p_hmd_manager);
    std::shared_ptr<ProgramHandle> CompileAndLinkShaderProgram(QByteArray * p_vertex_shader, QString p_vertex_shader_path,
                                                                      QByteArray * p_fragment_shader, QString p_fragment_shader_path);

    std::shared_ptr<ProgramHandle> GetDefaultObjectShaderProgram();
    std::shared_ptr<ProgramHandle> GetDefaultSkyboxShaderProgram();
    std::shared_ptr<ProgramHandle> GetDefaultPortalShaderProgram();

    void SetCameras(QVector<VirtualCamera> * p_cameras);
	void SetDefaultFontGlyphAtlas(std::shared_ptr<TextureHandle> p_handle);
	TextureHandle* GetDefaultFontGlyphAtlas();

    std::shared_ptr<TextureHandle> CreateCubemapTextureHandleFromAssetImages(QVector<QPointer<AssetImageData>>& p_skybox_image_data, const bool tex_mipmap, const bool tex_linear, const bool tex_clamp, const TextureHandle::ALPHA_TYPE tex_alpha, const TextureHandle::COLOR_SPACE tex_colorspace);
    std::shared_ptr<TextureHandle> CreateTextureFromAssetImageData(QPointer<AssetImageData> data, bool is_left, const bool tex_mipmap, const bool tex_linear, const bool tex_clamp, const TextureHandle::ALPHA_TYPE tex_alpha, const TextureHandle::COLOR_SPACE tex_colorspace);
    std::shared_ptr<TextureHandle> CreateTextureFromGLIData(const QByteArray & ba, const bool tex_mipmap, const bool tex_linear, const bool tex_clamp, const TextureHandle::ALPHA_TYPE tex_alpha, const TextureHandle::COLOR_SPACE tex_colorspace);
    std::shared_ptr<TextureHandle> CreateTextureQImage(const QImage & img, const bool tex_mipmap, const bool tex_linear, const bool tex_clamp, const TextureHandle::ALPHA_TYPE tex_alpha, const TextureHandle::COLOR_SPACE tex_colorspace);
    std::shared_ptr<TextureHandle> CreateCubemapTextureHandle(const uint32_t p_width, const uint32_t p_height, const TextureHandle::COLOR_SPACE p_color_space, const int32_t p_internal_texture_format, const bool tex_mipmap, const bool tex_linear, const bool tex_clamp, const TextureHandle::ALPHA_TYPE tex_alpha, const TextureHandle::COLOR_SPACE tex_colorspace);
    std::shared_ptr<TextureHandle> CreateCubemapTextureHandleFromTextureHandles(QVector<QPointer<AssetImageData> > &p_skybox_image_data, QVector<TextureHandle *> &p_skybox_image_handles, const bool tex_mipmap, const bool tex_linear, const bool tex_clamp, const TextureHandle::ALPHA_TYPE tex_alpha, const TextureHandle::COLOR_SPACE tex_colorspace);
    void GenerateEnvMapsFromCubemapTextureHandle(Cubemaps &p_cubemaps);
#ifdef WIN32
    QVector<std::shared_ptr<TextureHandle>> CreateSlugTextureHandles(uint32_t const p_curve_texture_width,
                                                                           uint32_t const p_curve_texture_height,
                                                                           void const * p_curve_texture,
                                                                           uint32_t const p_band_texture_width,
                                                                           uint32_t const p_band_texture_height,
                                                                           void const * p_band_texture);
#endif
    void UpdateTextureHandleData(TextureHandle* p_handle, uint const p_level, uint const p_x_offset, uint const p_y_offset, uint const p_width, uint const p_height, uint const p_pixel_size, void* const p_pixel_data);
    void UpdateTextureHandleData(TextureHandle* p_handle, uint const p_level, uint const p_x_offset, uint const p_y_offset, uint const p_width, uint const p_height, int const p_pixel_format, int const p_pixel_type, void* const p_pixel_data, uint32_t const p_data_size);
    void GenerateTextureHandleMipMap(TextureHandle* p_handle);

	void CreateMeshHandleForGeomVBOData(GeomVBOData* p_VBO_data);
    std::shared_ptr<MeshHandle> CreateMeshHandle(VertexAttributeLayout p_layout);
    void BindMeshHandle(MeshHandle * p_mesh_handle);
    QVector<std::shared_ptr<BufferHandle>>* GetBufferHandlesForMeshHandle(MeshHandle * p_mesh_handle);
	
	std::shared_ptr<BufferHandle> CreateBufferHandle(BufferHandle::BUFFER_TYPE const p_buffer_type, BufferHandle::BUFFER_USAGE const p_buffer_usage);
    void BindBufferHandle(BufferHandle * p_buffer_handle, BufferHandle::BUFFER_TYPE const p_buffer_type);
    void BindBufferHandle(BufferHandle * p_buffer_handle);
	void ConfigureBufferHandleData(std::shared_ptr<BufferHandle> p_buffer_handle, uint32_t const p_data_size, void* const p_data, BufferHandle::BUFFER_USAGE const p_buffer_usage);
	void UpdateBufferHandleData(std::shared_ptr<BufferHandle> p_buffer_handle, uint32_t const p_offset, uint32_t const p_data_size, void* const p_data);

    //AbstractRenderCommand CreateDefaultAbstractRenderCommand();
    bool GetIsEnhancedDepthPrecisionSupported() const;
    bool GetIsUsingEnhancedDepthPrecision() const;
    void SetIsUsingEnhancedDepthPrecision(bool const p_is_using);

    void SetDefaultFaceCullMode(FaceCullMode p_face_cull_mode);
    FaceCullMode GetDefaultFaceCullMode() const;

    void SetMirrorMode(bool p_mirror_mode);
    bool GetMirrorMode() const;

    void SetDepthFunc(DepthFunc p_depth_func);
    DepthFunc GetDepthFunc() const;

    void SetDepthMask(DepthMask p_depth_mask);
    DepthMask GetDepthMask() const;

    void SetStencilFunc(StencilFunc p_stencil_func);
    StencilFunc GetStencilFunc() const;

    void SetStencilOp(StencilOp p_stencil_op);
    StencilOp GetStencilOp() const;

    void SetPolyMode(PolyMode p_poly_mode);
    PolyMode GetPolyMode() const;

    void SetColorMask(ColorMask p_color_mask);
    ColorMask GetColorMask() const;

    void BindTextureHandle(uint32_t p_slot_index, TextureHandle* p_id);
    TextureSet GetCurrentlyBoundTextures();

    int GetTextureWidth(TextureHandle* p_handle);
    int GetTextureHeight(TextureHandle* p_handle);

    void Render();
    void PushAbstractRenderCommand(AbstractRenderCommand& p_object_render_command);
    void RenderObjects();
	void RenderObjectsDebug();
    void ClearObjectRenderQueue();

    void PushLightContainer(LightContainer const * p_lightContainer, StencilReferenceValue p_room_stencil_ref);

    void BeginScope(RENDERER::RENDER_SCOPE p_scope);
    void EndCurrentScope();
    RENDERER::RENDER_SCOPE GetCurrentScope();

    bool AreObjectsVisible() const;
    bool AreBoundingBoxesVisible() const;
    bool ArePhysicsProxiesVisible() const;
    bool AreWireframesVisible() const;
    bool AreNormalsVisible() const;
    bool AreFramebufferLayersVisible() const;
    void ToggleImmediateMode();

	void ClearRenderQueues();

    QVector<uint64_t> & GetGPUTimeQueryResults();
    QVector<uint64_t> & GetCPUTimeQueryResults();

    int64_t GetFrameCounter();
    size_t GetNumTextures() const;
    QString GetRendererName();

    std::shared_ptr<MeshHandle> GetSkyboxCubeVAO();
    GLuint GetSkyboxCubePrimCount() const;
    std::shared_ptr<MeshHandle> GetTexturedCubeVAO();
    GLuint GetTexturedCubePrimCount() const;
    std::shared_ptr<MeshHandle> GetTexturedCube2VAO();
    GLuint GetTexturedCube2PrimCount() const;
    std::shared_ptr<MeshHandle> GetTexturedCube3VAO();
    GLuint GetTexturedCube3PrimCount() const;
    std::shared_ptr<MeshHandle> GetPortalStencilCylinderVAO();
    GLuint GetPortalStencilCylinderPrimCount() const;
    std::shared_ptr<MeshHandle> GetPortalStencilCubeVAO();
    GLuint GetPortalStencilCubePrimCount() const;
    std::shared_ptr<MeshHandle> GetPlaneVAO();
    GLuint GetPlanePrimCount() const;
    std::shared_ptr<MeshHandle> GetDiscVAO();
    GLuint GetDiscPrimCount() const;
    std::shared_ptr<MeshHandle> GetConeVAO();
    GLuint GetConePrimCount() const;
    std::shared_ptr<MeshHandle> GetCone2VAO();
    GLuint GetCone2PrimCount() const;
    std::shared_ptr<MeshHandle> GetPyramidVAO();
    GLuint GetPyramidPrimCount() const;

    void ConfigureFramebuffer(uint32_t const p_window_width, uint32_t const p_window_height, uint32_t const p_msaa_count);
    void ConfigureWindowSize(uint32_t const p_window_width, uint32_t const p_window_height);
    void ConfigureSamples(uint32_t const p_msaa_count);
    uint32_t GetTextureID(FBO_TEXTURE_ENUM const p_texture_index, bool const p_multisampled) const;
    QVector<uint32_t> BindFBOToRead(FBO_TEXTURE_BITFIELD_ENUM const p_textures_bitmask, bool const p_bind_multisampled = true) const;
    QVector<uint32_t> BindFBOToDraw(FBO_TEXTURE_BITFIELD_ENUM const p_textures_bitmask, bool const p_bind_multisampled = true) const;
    void BlitMultisampledFramebuffer(FBO_TEXTURE_BITFIELD_ENUM const p_textures_bitmask,
                                     int32_t srcX0, int32_t srcY0, int32_t srcX1, int32_t srcY1,
                                     int32_t dstX0, int32_t dstY0, int32_t dstX1, int32_t dstY1) const;
    void BlitMultisampledFramebuffer(FBO_TEXTURE_BITFIELD_ENUM const p_textures_bitmask) const;
    uint32_t GetWindowWidth() const;
    uint32_t GetWindowHeight() const;
    uint32_t GetMSAACount() const;
    void SubmitFrame();
    void RequestScreenShot(uint32_t const p_width, uint32_t const p_height, uint32_t const p_sample_count, bool const p_is_equi, uint64_t p_frame_index);
    uint64_t GetLastSubmittedFrameID();

#ifdef __ANDROID__
    void Pause();
    void Resume();
#endif

private:

    void PreRender(QHash<size_t, QVector<AbstractRenderCommand> > & p_scoped_render_commands, QHash<StencilReferenceValue, LightContainer> & p_scoped_light_containers);
    void PostRender(QHash<size_t, QVector<AbstractRenderCommand> > & p_scoped_render_commands, QHash<StencilReferenceValue, LightContainer> & p_scoped_light_containers);

    RENDERER::RENDER_SCOPE m_current_scope;
    std::unique_ptr<AbstractRenderer> m_abstractRenderer;
    QString m_requested_version_string;
    // This notation causes the char to act like a bitfield with 6/8 used.
    // Reading these values will create new unsigned chars with the value
    // shifted to the correct place so this a nice reduction in size of the class
    // as well as a possible small perf gain as all the bits we care about are
    // read into a single register upon reading any one of them.
    unsigned char m_render_objects : 1;
    unsigned char m_render_bounding_boxes : 1;
    unsigned char m_render_physics_proxies : 1;
    unsigned char m_render_wireframes : 1;
    unsigned char m_render_normals : 1;
    unsigned char m_render_framebuffer_layers : 1;
    bool m_immediate_mode;
	RendererInterface* m_interface;
    QVector<AbstractRenderCommand_sort> m_sorted_command_indices;
    void SortRenderCommandsByDistance(QVector<AbstractRenderCommand>& render_command_vector, const bool p_is_transparent);
    void EnableRenderCommandInstancing(const RENDERER::RENDER_SCOPE p_scope);
    size_t m_collapsed_draws;
    size_t m_collapsable_draws;
};

#endif // RENDERER_H
