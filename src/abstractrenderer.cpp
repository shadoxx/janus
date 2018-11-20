#include "abstractrenderer.h"

char const * AbstractRenderer::g_gamma_correction_GLSL = "out_color = pow(out_color, vec4(0.45454545454, 0.45454545454, 0.45454545454, 1.0));";
//char const * AbstractRenderer::g_gamma_correction_GLSL = "bvec4 cutoff = lessThan(out_color, vec4(0.0031308)); vec4 higher = vec4(1.055) * pow(out_color, vec4(1.0 / 2.4)) - vec4(0.055); vec4 lower = out_color * vec4(12.92); out_color = mix(higher, lower, cutoff);";

AbstractRenderer::AbstractRenderer()
    : m_update_GPU_state(false),
      m_allow_color_mask(true),
      m_active_texture_slot(0),
      m_active_texture_slot_render(0),
      m_num_deleted_textures(0),
      m_fullScreenQuadShaderProgram(0),
      m_window_width(1280),
      m_window_height(720),
      m_msaa_count(4),
      m_framebuffer_requires_initialization(true),
      m_framebuffer_initialized(false),
      m_frame_rate_limiter(1),      
      m_current_submission_index(0),
      m_completed_submission_index(1),  // This always starts 1 ahead of m_rendering_index, so we don't try to push into the initial m_rendering_index value
      m_rendering_index(2),
      m_submitted_frame_id(1),
      m_current_frame_id(0),
      m_draw_id(0),
      m_screenshot_requested(false),
      m_screenshot_width(m_window_width),
      m_screenshot_height(m_window_height),
      m_screenshot_sample_count(m_msaa_count),
      m_screenshot_is_equi(false),
      m_screenshot_frame_index(0),
      m_enhanced_depth_precision_used(false),
      m_enhanced_depth_precision_supported(false),      
      m_glClipControl(nullptr),
      m_GPUTimeMin(UINT64_MAX),
      m_GPUTimeMax(0),
      m_GPUTimeAvg(0),
      m_frame_counter(0),
      m_texture_UUID(1),
      m_mesh_UUID(1),
      m_buffer_UUID(1),
      m_shader_UUID(1),
      m_max_anisotropy(0.0f),
      m_bound_VAO(0),
      m_face_cull_mode(FaceCullMode::BACK),
      m_current_face_cull_mode(FaceCullMode::BACK),
      m_default_face_cull_mode(FaceCullMode::BACK),
      m_mirror_mode(false),
      m_depth_func(DepthFunc::LEQUAL),
      m_current_depth_func(DepthFunc::LEQUAL),      
      m_depth_mask(DepthMask::DEPTH_WRITES_ENABLED),
      m_current_depth_mask(DepthMask::DEPTH_WRITES_ENABLED),                  
      m_active_light_UBO_index(0)
{
}

AbstractRenderer::~AbstractRenderer()
{

}

void AbstractRenderer::PostConstructorInitialize()
{
    m_bound_texture_handles = QVector<TextureHandle *>(ASSETSHADER_MAX_TEXTURE_UNITS, nullptr);
    m_bound_texture_handles_render = QVector<TextureHandle *>(ASSETSHADER_MAX_TEXTURE_UNITS, nullptr);

    for (size_t i = 0; i <BUFFER_TYPE_COUNT; ++i) {
        m_bound_buffers[i] = 0;
    }
    for (size_t i = 0; i <BUFFER_CHUNK_COUNT; ++i) {
        m_light_UBOs[i] = 0;
    }

    m_bound_VAO = 0;

    // Pre allocated our mapping buffers to avoid frequent reallocations during initial loading.
    m_texture_handle_to_GL_ID.reserve(1024);
    m_texture_handle_to_width.reserve(1024);
    m_texture_handle_to_height.reserve(1024);
    m_buffer_handle_to_GL_ID.reserve(1024);
    m_mesh_handle_to_GL_ID.reserve(1024);
    m_mesh_handle_to_GL_ID.reserve(1024);
    m_program_handle_to_GL_ID.reserve(1024);
    m_uniform_locs.resize(1024);

    UpdateFramebuffer();
    InitializeState();
    InitializeLightUBOs();

    MathUtil::glFuncs->glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &m_max_anisotropy);

    m_scopes.reserve(static_cast<size_t>(RENDERER::RENDER_SCOPE::SCOPE_COUNT));
    for (size_t scope_enum = 0; scope_enum < static_cast<size_t>(RENDERER::RENDER_SCOPE::SCOPE_COUNT); ++scope_enum)
    {
         m_scopes.push_back(static_cast<RENDERER::RENDER_SCOPE>(scope_enum));
    }

    m_scoped_cameras_cache.resize(3);
}

void AbstractRenderer::InitializeHMDManager(QPointer<AbstractHMDManager> p_hmd_manager)
{
    m_hmd_manager = p_hmd_manager;
}

QString AbstractRenderer::GetRendererName()
{
    return m_name;
}

void AbstractRenderer::RequestScreenShot(uint32_t const p_width, uint32_t const p_height, uint32_t const p_sample_count, bool const p_is_equi, uint64_t p_frame_index)
{
    m_screenshot_requested = true;
    m_screenshot_width = p_width;
    m_screenshot_height = p_height;
    m_screenshot_sample_count = p_sample_count;
    m_screenshot_is_equi = p_is_equi;
    m_screenshot_frame_index = p_frame_index;
}

void AbstractRenderer::InitializeGLObjects()
{
    //setup "slab"
    const int vbo_size = 288;
    const float vbo_data[vbo_size] = {
        0,0,1,0,0,-1,-1,0,
        0,0,1,1,0,1,-1,0,
        0,0,1,1,1,1,1,0,

        0,0,1,0,0,-1,-1,0,
        0,0,1,1,1,1,1,0,
        0,0,1,0,1,-1,1,0,

        0,0,-1,0,0,-1,-1,-0.1f,
        0,0,-1,0,1,-1,1,-0.1f,
        0,0,-1,1,1,1,1,-0.1f,

        0,0,-1,0,0,-1,-1,-0.1f,
        0,0,-1,1,1,1,1,-0.1f,
        0,0,-1,1,0,1,-1,-0.1f,

        0,-1,0,0,0,-1,-1,0,
        0,-1,0,0,0,-1,-1,-0.1f,
        0,-1,0,1,0,1,-1,-0.1f,

        0,-1,0,0,0,-1,-1,0,
        0,-1,0,1,0,1,-1,-0.1f,
        0,-1,0,1,0,1,-1,0,

        0,1,0,0,1,-1,1,0,
        0,1,0,1,1,1,1,0,
        0,1,0,1,1,1,1,-0.1f,

        0,1,0,0,1,-1,1,0,
        0,1,0,1,1,1,1,-0.1f,
        0,1,0,0,1,-1,1,-0.1f,

        -1,0,0,0,0,-1,-1,0,
        -1,0,0,0,1,-1,1,0,
        -1,0,0,0,1,-1,1,-0.1f,

        -1,0,0,0,0,-1,-1,0,
        -1,0,0,0,1,-1,1,-0.1f,
        -1,0,0,0,0,-1,-1,-0.1f,

        1,0,0,1,0,1,-1,0,
        1,0,0,1,0,1,-1,-0.1f,
        1,0,0,1,1,1,1,-0.1f,

        1,0,0,1,0,1,-1,0,
        1,0,0,1,1,1,1,-0.1f,
        1,0,0,1,1,1,1,0};

    //setup "cube"
    const int vbo_size2 = 288;
    const float vbo_data2[vbo_size2] = {
        0,0,1,1,0,-1,-1,1,
        0,0,1,0,0,1,-1,1,
        0,0,1,0,1,1,1,1,

        0,0,1,1,0,-1,-1,1,
        0,0,1,0,1,1,1,1,
        0,0,1,1,1,-1,1,1,

        0,0,-1,0,0,-1,-1,-1,
        0,0,-1,0,1,-1,1,-1,
        0,0,-1,1,1,1,1,-1,

        0,0,-1,0,0,-1,-1,-1,
        0,0,-1,1,1,1,1,-1,
        0,0,-1,1,0,1,-1,-1,

        0,-1,0,0,0,-1,-1,1,
        0,-1,0,0,1,-1,-1,-1,
        0,-1,0,1,1,1,-1,-1,

        0,-1,0,0,0,-1,-1,1,
        0,-1,0,1,1,1,-1,-1,
        0,-1,0,1,0,1,-1,1,

        0,1,0,0,1,-1,1,1,
        0,1,0,1,1,1,1,1,
        0,1,0,1,0,1,1,-1,

        0,1,0,0,1,-1,1,1,
        0,1,0,1,0,1,1,-1,
        0,1,0,0,0,-1,1,-1,

        -1,0,0,0,0,-1,-1,1,
        -1,0,0,0,1,-1,1,1,
        -1,0,0,1,1,-1,1,-1,

        -1,0,0,0,0,-1,-1,1,
        -1,0,0,1,1,-1,1,-1,
        -1,0,0,1,0,-1,-1,-1,

        1,0,0,1,0,1,-1,1,
        1,0,0,0,0,1,-1,-1,
        1,0,0,0,1,1,1,-1,

        1,0,0,1,0,1,-1,1,
        1,0,0,0,1,1,1,-1,
        1,0,0,1,1,1,1,1};

    //setup "skybox cube"
    const int vbo_skysize = 108;
    const float vbo_skydata[vbo_skysize] = {
        1,1,1,
        1,-1,1,
        -1,-1,1,

        -1,1,1,
        1,1,1,
        -1,-1,1,

        1,1,-1,
        -1,1,-1,
        -1,-1,-1,

        1,-1,-1,
        1,1,-1,
        -1,-1,-1,

        1,-1,-1,
        -1,-1,-1,
        -1,-1,1,

        1,-1,1,
        1,-1,-1,
        -1,-1,1,

        1,1,-1,
        1,1,1,
        -1,1,1,

        -1,1,-1,
        1,1,-1,
        -1,1,1,

        -1,1,-1,
        -1,1,1,
        -1,-1,1,

        -1,-1,-1,
        -1,1,-1,
        -1,-1,1,

        1,1,-1,
        1,-1,-1,
        1,-1,1,

        1,1,1,
        1,1,-1,
        1,-1,1,
    };


    //setup "cube3"
    const int vbo_size8 = 288;
    const float vbo_data8[vbo_size8] = {
        0,0,1,0,0,-1,-1,1,
        0,0,1,1,0,1,-1,1,
        0,0,1,1,1,1,1,1,

        0,0,1,0,0,-1,-1,1,
        0,0,1,1,1,1,1,1,
        0,0,1,0,1,-1,1,1,

        0,0,-1,1,0,-1,-1,-1,
        0,0,-1,1,1,-1,1,-1,
        0,0,-1,0,1,1,1,-1,

        0,0,-1,1,0,-1,-1,-1,
        0,0,-1,0,1,1,1,-1,
        0,0,-1,0,0,1,-1,-1,

        0,-1,0,1,0,-1,-1,1,
        0,-1,0,1,1,-1,-1,-1,
        0,-1,0,0,1,1,-1,-1,

        0,-1,0,1,0,-1,-1,1,
        0,-1,0,0,1,1,-1,-1,
        0,-1,0,0,0,1,-1,1,

        0,1,0,1,1,-1,1,1,
        0,1,0,0,1,1,1,1,
        0,1,0,0,0,1,1,-1,

        0,1,0,1,1,-1,1,1,
        0,1,0,0,0,1,1,-1,
        0,1,0,1,0,-1,1,-1,

        -1,0,0,1,0,-1,-1,1,
        -1,0,0,1,1,-1,1,1,
        -1,0,0,0,1,-1,1,-1,

        -1,0,0,1,0,-1,-1,1,
        -1,0,0,0,1,-1,1,-1,
        -1,0,0,0,0,-1,-1,-1,

        1,0,0,0,0,1,-1,1,
        1,0,0,1,0,1,-1,-1,
        1,0,0,1,1,1,1,-1,

        1,0,0,0,0,1,-1,1,
        1,0,0,1,1,1,1,-1,
        1,0,0,0,1,1,1,1};

    //setup "plane"
    const int vbo_size3 = 48;
    const float vbo_data3[vbo_size3] = {
        0,0,1,0,0,-1,-1,0,
        0,0,1,1,0,1,-1,0,
        0,0,1,1,1,1,1,0,

        0,0,1,0,0,-1,-1,0,
        0,0,1,1,1,1,1,0,
        0,0,1,0,1,-1,1,0
        };

    //setup "disc"
    const int vbo_size4 = 72 * 8;
    float vbo_data4[vbo_size4];

    int i2 = 0;
    for (int i=0; i < 360; i+=5)
    {
        const float degInRad = MathUtil::DegToRad(i);
        const float c = cosf(degInRad);
        const float s = sinf(degInRad);

        vbo_data4[i2+0] = 0.0f;
        vbo_data4[i2+1] = 0.0f;
        vbo_data4[i2+2] = 1.0f;
        vbo_data4[i2+3] = c; // / aspect;
        vbo_data4[i2+4] = s;
        vbo_data4[i2+5] = c; // * width;
        vbo_data4[i2+6] = s;
        vbo_data4[i2+7] = 0.0f;

        i2 += 8;
    }

    //setup "cone"
    const int vbo_size5 = 72 * 8 * 6;
    float vbo_data5[vbo_size5];
    i2 = 0;
    for (int i=0; i < 360; i+=5)
    {
        const float degInRad1 = MathUtil::DegToRad(i);
        const float degInRad2 = MathUtil::DegToRad(i+5);
        const float c1 = cosf(degInRad1);
        const float s1 = sinf(degInRad1);
        const float c2 = cosf(degInRad2);
        const float s2 = sinf(degInRad2);

        const QVector3D v1(c1, s1, 0);
        const QVector3D v2(c1 * 1.1f, s1 * 1.1f, 1);
        const QVector3D v3(c2 * 1.1f, s2 * 1.1f, 1);
        const QVector3D v4(c2, s2, 0);

        const QVector3D n1 = v1.normalized();
        const QVector3D n2 = v3.normalized();

        const float mx1 = float(i) / 30.0f;
        const float mx2 =float(i+5) / 30.0f;
        const float my1 = 0.0f;
        const float my2 = 1.0f;

        vbo_data5[i2+0] = n1.x();
        vbo_data5[i2+1] = n1.y();
        vbo_data5[i2+2] = n1.z();
        vbo_data5[i2+3] = mx1;
        vbo_data5[i2+4] = my1;
        vbo_data5[i2+5] = v1.x();
        vbo_data5[i2+6] = v1.y();
        vbo_data5[i2+7] = v1.z();
        i2 += 8;

        vbo_data5[i2+0] = n1.x();
        vbo_data5[i2+1] = n1.y();
        vbo_data5[i2+2] = n1.z();
        vbo_data5[i2+3] = mx1;
        vbo_data5[i2+4] = my2;
        vbo_data5[i2+5] = v2.x();
        vbo_data5[i2+6] = v2.y();
        vbo_data5[i2+7] = v2.z();
        i2 += 8;

        vbo_data5[i2+0] = n2.x();
        vbo_data5[i2+1] = n2.y();
        vbo_data5[i2+2] = n2.z();
        vbo_data5[i2+3] = mx2;
        vbo_data5[i2+4] = my2;
        vbo_data5[i2+5] = v3.x();
        vbo_data5[i2+6] = v3.y();
        vbo_data5[i2+7] = v3.z();
        i2 += 8;

        vbo_data5[i2+0] = n1.x();
        vbo_data5[i2+1] = n1.y();
        vbo_data5[i2+2] = n1.z();
        vbo_data5[i2+3] = mx1;
        vbo_data5[i2+4] = my1;
        vbo_data5[i2+5] = v1.x();
        vbo_data5[i2+6] = v1.y();
        vbo_data5[i2+7] = v1.z();
        i2 += 8;

        vbo_data5[i2+0] = n2.x();
        vbo_data5[i2+1] = n2.y();
        vbo_data5[i2+2] = n2.z();
        vbo_data5[i2+3] = mx2;
        vbo_data5[i2+4] = my2;
        vbo_data5[i2+5] = v3.x();
        vbo_data5[i2+6] = v3.y();
        vbo_data5[i2+7] = v3.z();
        i2 += 8;

        vbo_data5[i2+0] = n2.x();
        vbo_data5[i2+1] = n2.y();
        vbo_data5[i2+2] = n2.z();
        vbo_data5[i2+3] = mx2;
        vbo_data5[i2+4] = my1;
        vbo_data5[i2+5] = v4.x();
        vbo_data5[i2+6] = v4.y();
        vbo_data5[i2+7] = v4.z();
        i2 += 8;

    }

    const int vbo_size6 = 4 * 8 * 6;
    float vbo_data6[vbo_size6];
    i2 = 0;
    for (int i=0; i < 4; ++i)
    {

        QVector3D v1;
        QVector3D v2;
        QVector3D v3;
        QVector3D v4;
        QVector3D n1;
        QVector3D n2;

        if (i == 0) {
            v1 = QVector3D(-1.0f, -1.0f, 0.0f);
            v2 = QVector3D(-1.1f, -1.1f, 1.0f);
            v3 = QVector3D(1.1f, -1.1f, 1.0f);
            v4 = QVector3D(1.0f, -1.0f, 0.0f);

            n1 = QVector3D(0.0f, 1.0f, 0.0f);
            n2 = QVector3D(0.0f, 1.0f, 0.0f);
        }
        else if (i == 1) {
            v1 = QVector3D(1.0f, -1.0f, 0.0f);
            v2 = QVector3D(1.1f, -1.1f, 1.0f);
            v3 = QVector3D(1.1f, 1.1f, 1.0f);
            v4 = QVector3D(1.0f, 1.0f, 0.0f);

            n1 = QVector3D(1.0f, 0.0f, 0.0f);
            n2 = QVector3D(1.0f, 0.0f, 0.0f);
        }
        else if (i == 2) {
            v1 = QVector3D(1.0f, 1.0f, 0.0f);
            v2 = QVector3D(1.1f, 1.1f, 1.0f);
            v3 = QVector3D(-1.1f, 1.1f, 1.0f);
            v4 = QVector3D(-1.0f, 1.0f, 0.0f);

            n1 = QVector3D(0.0f, -1.0f ,0.0f);
            n2 = QVector3D(0.0f, -1.0f ,0.0f);
        }
        else if (i == 3) {
            v1 = QVector3D(-1.0f, 1.0f, 0.0f);
            v2 = QVector3D(-1.1f, 1.1f, 1.0f);
            v3 = QVector3D(-1.1f, -1.1f, 1.0f);
            v4 = QVector3D(-1.0f, -1.0f, 0.0f);


            n1 = QVector3D(-1.0f, 0.0f, 0.0f);
            n2 = QVector3D(-1.0f, 0.0f, 0.0f);
        }

        vbo_data6[i2+0] = n1.x();
        vbo_data6[i2+1] = n1.y();
        vbo_data6[i2+2] = n1.z();
        vbo_data6[i2+3] = 1.0f; // / aspect;
        vbo_data6[i2+4] = 0.0f;
        vbo_data6[i2+5] = v1.x(); // * width;
        vbo_data6[i2+6] = v1.y();
        vbo_data6[i2+7] = v1.z();
        i2 += 8;

        vbo_data6[i2+0] = n1.x();
        vbo_data6[i2+1] = n1.y();
        vbo_data6[i2+2] = n1.z();
        vbo_data6[i2+3] = 1.0f; // / aspect;
        vbo_data6[i2+4] = 1.0f;
        vbo_data6[i2+5] = v2.x(); // * width;
        vbo_data6[i2+6] = v2.y();
        vbo_data6[i2+7] = v2.z();
        i2 += 8;

        vbo_data6[i2+0] = n2.x();
        vbo_data6[i2+1] = n2.y();
        vbo_data6[i2+2] = n2.z();
        vbo_data6[i2+3] = 0.0f; // / aspect;
        vbo_data6[i2+4] = 1.0f;
        vbo_data6[i2+5] = v3.x(); // * width;
        vbo_data6[i2+6] = v3.y();
        vbo_data6[i2+7] = v3.z();
        i2 += 8;

        vbo_data6[i2+0] = n1.x();
        vbo_data6[i2+1] = n1.y();
        vbo_data6[i2+2] = n1.z();
        vbo_data6[i2+3] = 1.0f; // / aspect;
        vbo_data6[i2+4] = 0.0f;
        vbo_data6[i2+5] = v1.x(); // * width;
        vbo_data6[i2+6] = v1.y();
        vbo_data6[i2+7] = v1.z();
        i2 += 8;

        vbo_data6[i2+0] = n2.x();
        vbo_data6[i2+1] = n2.y();
        vbo_data6[i2+2] = n2.z();
        vbo_data6[i2+3] = 0.0f; // / aspect;
        vbo_data6[i2+4] = 1.0f;
        vbo_data6[i2+5] = v3.x(); // * width;
        vbo_data6[i2+6] = v3.y();
        vbo_data6[i2+7] = v3.z();
        i2 += 8;

        vbo_data6[i2+0] = n2.x();
        vbo_data6[i2+1] = n2.y();
        vbo_data6[i2+2] = n2.z();
        vbo_data6[i2+3] = 0.0f; // / aspect;
        vbo_data6[i2+4] = 0.0f;
        vbo_data6[i2+5] = v4.x(); // * width;
        vbo_data6[i2+6] = v4.y();
        vbo_data6[i2+7] = v4.z();
        i2 += 8;
    }

    //setup "cone"
    const int vbo_size7 = 72 * 8 * 3;
    float vbo_data7[vbo_size7];
    i2 = 0;
    for (int i=0; i < 360; i+=5)
    {
        const float degInRad1 = MathUtil::DegToRad(i);
        const float degInRad2 = MathUtil::DegToRad(i+5);
        const float c1 = cosf(degInRad1);
        const float s1 = sinf(degInRad1);
        const float c2 = cosf(degInRad2);
        const float s2 = sinf(degInRad2);

        const QVector3D v1(c1, s1, 0);
        const QVector3D v2(c2, s2, 0);
        const QVector3D v3(0, 0, 1);

        const QVector3D n1 = -v1.normalized();
        const QVector3D n2 = -v3.normalized();

        const float tiling = 8.0f;
        const float mx1 = float(i) / 360.0f * tiling;
        const float mx2 = float(i+5) / 360.0f * tiling;
        const float my1 = 0.0f * tiling;
        const float my2 = 1.0f * tiling;

        vbo_data7[i2+0] = n1.x();
        vbo_data7[i2+1] = n1.y();
        vbo_data7[i2+2] = n1.z();
        vbo_data7[i2+3] = mx1;
        vbo_data7[i2+4] = my1;
        vbo_data7[i2+5] = v1.x();
        vbo_data7[i2+6] = v1.y();
        vbo_data7[i2+7] = v1.z();
        i2 += 8;

        vbo_data7[i2+0] = n2.x();
        vbo_data7[i2+1] = n2.y();
        vbo_data7[i2+2] = n2.z();
        vbo_data7[i2+3] = mx1;
        vbo_data7[i2+4] = my2;
        vbo_data7[i2+5] = v2.x();
        vbo_data7[i2+6] = v2.y();
        vbo_data7[i2+7] = v2.z();
        i2 += 8;

        vbo_data7[i2+0] = n2.x();
        vbo_data7[i2+1] = n2.y();
        vbo_data7[i2+2] = n2.z();
        vbo_data7[i2+3] = mx2;
        vbo_data7[i2+4] = my1;
        vbo_data7[i2+5] = v3.x();
        vbo_data7[i2+6] = v3.y();
        vbo_data7[i2+7] = v3.z();
        i2 += 8;
    }

    //setup "cone"
    const int vbo_sizec = 72 * 8 * 3;
    float vbo_datac[vbo_sizec];
    i2 = 0;
    for (int i=0; i < 360; i+=5)
    {
        const float degInRad1 = MathUtil::DegToRad(i);
        const float degInRad2 = MathUtil::DegToRad(i+5);
        const float c1 = cosf(degInRad1);
        const float s1 = sinf(degInRad1);
        const float c2 = cosf(degInRad2);
        const float s2 = sinf(degInRad2);

        const QVector3D v1(c1, s1, 0);
        const QVector3D v2(0, 0, -0.3f);
        const QVector3D v3(c2, s2, 0);

        const QVector3D n1 = -v1.normalized();
        const QVector3D n2 = -v3.normalized();

        const float tiling = 8.0f;
        const float mx1 = float(i) / 360.0f * tiling;
        const float mx2 = float(i+5) / 360.0f * tiling;
        const float my1 = 0.0f * tiling;
        const float my2 = 1.0f * tiling;

        vbo_datac[i2+0] = n1.x();
        vbo_datac[i2+1] = n1.y();
        vbo_datac[i2+2] = n1.z();
        vbo_datac[i2+3] = mx1;
        vbo_datac[i2+4] = my1;
        vbo_datac[i2+5] = v1.x();
        vbo_datac[i2+6] = v1.y();
        vbo_datac[i2+7] = v1.z();
        i2 += 8;

        vbo_datac[i2+0] = n1.x();
        vbo_datac[i2+1] = n1.y();
        vbo_datac[i2+2] = n1.z();
        vbo_datac[i2+3] = mx1;
        vbo_datac[i2+4] = my2;
        vbo_datac[i2+5] = v2.x();
        vbo_datac[i2+6] = v2.y();
        vbo_datac[i2+7] = v2.z();
        i2 += 8;

        vbo_datac[i2+0] = n2.x();
        vbo_datac[i2+1] = n2.y();
        vbo_datac[i2+2] = n2.z();
        vbo_datac[i2+3] = mx2;
        vbo_datac[i2+4] = my1;
        vbo_datac[i2+5] = v3.x();
        vbo_datac[i2+6] = v3.y();
        vbo_datac[i2+7] = v3.z();
        i2 += 8;
    }

    //setup portal_stencil_cube_vbo
    //shaped like a cube without the front face, a box you are looking into
    const int vbo_size9 = 240;
    const float vbo_data9[vbo_size9] = {
        0,0,-1,0,0,-1,-0.9f,-0.09f,
        0,0,-1,0,1,-1,0.9f,-0.09f,
        0,0,-1,1,1,1,0.9f,-0.09f,

        0,0,-1,0,0,-1,-0.9f,-0.09f,
        0,0,-1,1,1,1,0.9f,-0.09f,
        0,0,-1,1,0,1,-0.9f,-0.09f,

        0,-1,0,0,0,-1,-1,0,
        0,-1,0,0,0,-1,-0.9f,-0.09f,
        0,-1,0,1,0,1,-0.9f,-0.09f,

        0,-1,0,0,0,-1,-1,0,
        0,-1,0,1,0,1,-0.9f,-0.09f,
        0,-1,0,1,0,1,-1,0,

        0,1,0,0,1,-1,1,0,
        0,1,0,1,1,1,1,0,
        0,1,0,1,1,1,0.9f,-0.09f,

        0,1,0,0,1,-1,1,0,
        0,1,0,1,1,1,0.9f,-0.09f,
        0,1,0,0,1,-1,0.9f,-0.09f,

        -1,0,0,0,0,-1,-1,0,
        -1,0,0,0,1,-1,1,0,
        -1,0,0,0,1,-1,0.9f,-0.09f,

        -1,0,0,0,0,-1,-1,0,
        -1,0,0,0,1,-1,0.9f,-0.09f,
        -1,0,0,0,0,-1,-0.9f,-0.09f,

        1,0,0,1,0,1,-1,0,
        1,0,0,1,0,1,-0.9f,-0.09f,
        1,0,0,1,1,1,0.9f,-0.09f,

        1,0,0,1,0,1,-1,0,
        1,0,0,1,1,1,0.9f,-0.09f,
        1,0,0,1,1,1,1,0};

    VertexAttributeLayout slab_layout;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].in_use = true;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].buffer_id = VAO_ATTRIB::POSITION;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_count = 3;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_type = GL_FLOAT;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].stride_in_bytes = 8 * sizeof(GLfloat);
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].offset_in_bytes = 0 * sizeof(GLfloat);

    slab_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].in_use = true;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].buffer_id = VAO_ATTRIB::POSITION;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_count = 2;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_type = GL_FLOAT;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].stride_in_bytes = 8 * sizeof(GLfloat);
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].offset_in_bytes = 3 * sizeof(GLfloat);

    slab_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use = true;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].buffer_id = VAO_ATTRIB::POSITION;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_count = 3;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_type = GL_FLOAT;
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].stride_in_bytes = 8 * sizeof(GLfloat);
    slab_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].offset_in_bytes = 5 * sizeof(GLfloat);

    m_slab_vao = CreateMeshHandle(slab_layout);
    auto buffers = GetBufferHandlesForMeshHandle(m_slab_vao.get());
    m_slab_vbo = (*buffers)[(GLuint)VAO_ATTRIB::POSITION];
    BindBufferHandle(m_slab_vbo.get());
    ConfigureBufferHandleData(m_slab_vbo, vbo_size * sizeof(GLfloat), (void* const)&vbo_data[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);

    VertexAttributeLayout sky_cube_layout;
    sky_cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use = true;
    sky_cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].buffer_id =VAO_ATTRIB::POSITION;
    sky_cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_count = 3;
    sky_cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_type = GL_FLOAT;
    sky_cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].stride_in_bytes = 3 * sizeof(GLfloat);

    m_skycube_vao = CreateMeshHandle(sky_cube_layout);
    buffers = GetBufferHandlesForMeshHandle(m_skycube_vao.get());
    m_skycube_vbo = (*buffers)[(GLuint)VAO_ATTRIB::POSITION];
    BindBufferHandle(m_skycube_vbo.get());
    ConfigureBufferHandleData(m_skycube_vbo, vbo_skysize * sizeof(GLfloat), (void* const)&vbo_skydata[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);

    VertexAttributeLayout cube_layout;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].in_use = true;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].buffer_id = VAO_ATTRIB::POSITION;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_count = 3;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_type = GL_FLOAT;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].stride_in_bytes = 8 * sizeof(GLfloat);
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].offset_in_bytes = 0 * sizeof(GLfloat);

    cube_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].in_use = true;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].buffer_id = VAO_ATTRIB::POSITION;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_count = 2;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_type = GL_FLOAT;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].stride_in_bytes = 8 * sizeof(GLfloat);
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].offset_in_bytes = 3 * sizeof(GLfloat);

    cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use = true;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].buffer_id = VAO_ATTRIB::POSITION;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_count = 3;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_type = GL_FLOAT;
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].stride_in_bytes = 8 * sizeof(GLfloat);
    cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].offset_in_bytes = 5 * sizeof(GLfloat);

    m_cube_vao = CreateMeshHandle(cube_layout);
    buffers = GetBufferHandlesForMeshHandle(m_cube_vao.get());
    m_cube_vbo = (*buffers)[(GLuint)VAO_ATTRIB::POSITION];
    BindBufferHandle(m_cube_vbo.get());
    ConfigureBufferHandleData(m_cube_vbo, vbo_size2 * sizeof(GLfloat), (void* const)&vbo_data2[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);

    VertexAttributeLayout plane_layout;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].in_use = true;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].buffer_id = VAO_ATTRIB::POSITION;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_count = 3;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_type = GL_FLOAT;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].stride_in_bytes = 8 * sizeof(GLfloat);
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].offset_in_bytes = 0 * sizeof(GLfloat);

    plane_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].in_use = true;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].buffer_id = VAO_ATTRIB::POSITION;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_count = 2;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_type = GL_FLOAT;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].stride_in_bytes = 8 * sizeof(GLfloat);
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].offset_in_bytes = 3 * sizeof(GLfloat);

    plane_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use = true;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].buffer_id = VAO_ATTRIB::POSITION;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_count = 3;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_type = GL_FLOAT;
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].stride_in_bytes = 8 * sizeof(GLfloat);
    plane_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].offset_in_bytes = 5 * sizeof(GLfloat);

    m_plane_vao = CreateMeshHandle(plane_layout);
    buffers = GetBufferHandlesForMeshHandle(m_plane_vao.get());
    m_plane_vbo = (*buffers)[(GLuint)VAO_ATTRIB::POSITION];
    BindBufferHandle(m_plane_vbo.get());
    ConfigureBufferHandleData(m_plane_vbo, vbo_size3 * sizeof(GLfloat), (void* const)&vbo_data3[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);

    VertexAttributeLayout disc_layout;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].in_use = true;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].buffer_id = VAO_ATTRIB::POSITION;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_count = 3;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_type = GL_FLOAT;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].stride_in_bytes = 8 * sizeof(GLfloat);
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].offset_in_bytes = 0 * sizeof(GLfloat);

    disc_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].in_use = true;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].buffer_id = VAO_ATTRIB::POSITION;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_count = 2;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_type = GL_FLOAT;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].stride_in_bytes = 8 * sizeof(GLfloat);
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].offset_in_bytes = 3 * sizeof(GLfloat);

    disc_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use = true;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].buffer_id = VAO_ATTRIB::POSITION;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_count = 3;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_type = GL_FLOAT;
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].stride_in_bytes = 8 * sizeof(GLfloat);
    disc_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].offset_in_bytes = 5 * sizeof(GLfloat);

    m_disc_vao = CreateMeshHandle(disc_layout);
    buffers = GetBufferHandlesForMeshHandle(m_disc_vao.get());
    m_disc_vbo = (*buffers)[(GLuint)VAO_ATTRIB::POSITION];
    BindBufferHandle(m_disc_vbo.get());
    ConfigureBufferHandleData(m_disc_vbo, vbo_size4 * sizeof(GLfloat), (void* const)&vbo_data4[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);

    VertexAttributeLayout cone_layout;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].in_use = true;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].buffer_id = VAO_ATTRIB::POSITION;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_count = 3;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_type = GL_FLOAT;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].stride_in_bytes = 8 * sizeof(GLfloat);
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].offset_in_bytes = 0 * sizeof(GLfloat);

    cone_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].in_use = true;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].buffer_id = VAO_ATTRIB::POSITION;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_count = 2;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_type = GL_FLOAT;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].stride_in_bytes = 8 * sizeof(GLfloat);
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].offset_in_bytes = 3 * sizeof(GLfloat);

    cone_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use = true;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].buffer_id = VAO_ATTRIB::POSITION;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_count = 3;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_type = GL_FLOAT;
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].stride_in_bytes = 8 * sizeof(GLfloat);
    cone_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].offset_in_bytes = 5 * sizeof(GLfloat);

    m_cone_vao = CreateMeshHandle(cone_layout);
    buffers = GetBufferHandlesForMeshHandle(m_cone_vao.get());
    m_cone_vbo = (*buffers)[(GLuint)VAO_ATTRIB::POSITION];
    BindBufferHandle(m_cone_vbo.get());
    ConfigureBufferHandleData(m_cone_vbo, vbo_size5 * sizeof(GLfloat), (void* const)&vbo_data5[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);

    VertexAttributeLayout pyramid_layout;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].in_use = true;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].buffer_id = VAO_ATTRIB::POSITION;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_count = 3;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_type = GL_FLOAT;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].stride_in_bytes = 8 * sizeof(GLfloat);
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].offset_in_bytes = 0 * sizeof(GLfloat);

    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].in_use = true;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].buffer_id = VAO_ATTRIB::POSITION;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_count = 2;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_type = GL_FLOAT;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].stride_in_bytes = 8 * sizeof(GLfloat);
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].offset_in_bytes = 3 * sizeof(GLfloat);

    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use = true;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].buffer_id = VAO_ATTRIB::POSITION;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_count = 3;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_type = GL_FLOAT;
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].stride_in_bytes = 8 * sizeof(GLfloat);
    pyramid_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].offset_in_bytes = 5 * sizeof(GLfloat);

    m_pyramid_vao = CreateMeshHandle(pyramid_layout);
    buffers = GetBufferHandlesForMeshHandle(m_pyramid_vao.get());
    m_pyramid_vbo = (*buffers)[(GLuint)VAO_ATTRIB::POSITION];
    BindBufferHandle(m_pyramid_vbo.get());
    ConfigureBufferHandleData(m_pyramid_vbo, vbo_size6 * sizeof(GLfloat), (void* const)&vbo_data6[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);

    VertexAttributeLayout cone2_layout;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].in_use = true;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].buffer_id = VAO_ATTRIB::POSITION;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_count = 3;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_type = GL_FLOAT;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].stride_in_bytes = 8 * sizeof(GLfloat);
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].offset_in_bytes = 0 * sizeof(GLfloat);

    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].in_use = true;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].buffer_id = VAO_ATTRIB::POSITION;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_count = 2;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_type = GL_FLOAT;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].stride_in_bytes = 8 * sizeof(GLfloat);
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].offset_in_bytes = 3 * sizeof(GLfloat);

    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use = true;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].buffer_id = VAO_ATTRIB::POSITION;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_count = 3;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_type = GL_FLOAT;
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].stride_in_bytes = 8 * sizeof(GLfloat);
    cone2_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].offset_in_bytes = 5 * sizeof(GLfloat);

    m_cone2_vao = CreateMeshHandle(cone2_layout);
    buffers = GetBufferHandlesForMeshHandle(m_cone2_vao.get());
    m_cone2_vbo = (*buffers)[(GLuint)VAO_ATTRIB::POSITION];
    BindBufferHandle(m_cone2_vbo.get());
    ConfigureBufferHandleData(m_cone2_vbo, vbo_size7 * sizeof(GLfloat), (void* const)&vbo_data7[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);

    VertexAttributeLayout cube3_layout;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].in_use = true;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].buffer_id = VAO_ATTRIB::POSITION;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_count = 3;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_type = GL_FLOAT;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].stride_in_bytes = 8 * sizeof(GLfloat);
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].offset_in_bytes = 0 * sizeof(GLfloat);

    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].in_use = true;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].buffer_id = VAO_ATTRIB::POSITION;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_count = 2;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].element_type = GL_FLOAT;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].stride_in_bytes = 8 * sizeof(GLfloat);
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].offset_in_bytes = 3 * sizeof(GLfloat);

    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use = true;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].buffer_id = VAO_ATTRIB::POSITION;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_count = 3;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_type = GL_FLOAT;
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].stride_in_bytes = 8 * sizeof(GLfloat);
    cube3_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].offset_in_bytes = 5 * sizeof(GLfloat);

    m_cube3_vao = CreateMeshHandle(cube3_layout);
    buffers = GetBufferHandlesForMeshHandle(m_cube3_vao.get());
    m_cube3_vbo = (*buffers)[(GLuint)VAO_ATTRIB::POSITION];
    BindBufferHandle(m_cube3_vbo.get());
    ConfigureBufferHandleData(m_cube3_vbo, vbo_size8 * sizeof(GLfloat), (void* const)&vbo_data8[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);

    VertexAttributeLayout portal_stencil_cylinder_layout;
    portal_stencil_cylinder_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].in_use = true;
    portal_stencil_cylinder_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].buffer_id = VAO_ATTRIB::POSITION;
    portal_stencil_cylinder_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_count = 3;
    portal_stencil_cylinder_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_type = GL_FLOAT;
    portal_stencil_cylinder_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].stride_in_bytes = 8 * sizeof(GLfloat);
    portal_stencil_cylinder_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].offset_in_bytes = 0 * sizeof(GLfloat);

    portal_stencil_cylinder_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use = true;
    portal_stencil_cylinder_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].buffer_id = VAO_ATTRIB::POSITION;
    portal_stencil_cylinder_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_count = 3;
    portal_stencil_cylinder_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_type = GL_FLOAT;
    portal_stencil_cylinder_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].stride_in_bytes = 8 * sizeof(GLfloat);
    portal_stencil_cylinder_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].offset_in_bytes = 5 * sizeof(GLfloat);

    m_portal_stencil_cylinder_vao = CreateMeshHandle(portal_stencil_cylinder_layout);
    buffers = GetBufferHandlesForMeshHandle(m_portal_stencil_cylinder_vao.get());
    m_portal_stencil_cylinder_vbo = (*buffers)[(GLuint)VAO_ATTRIB::POSITION];
    BindBufferHandle(m_portal_stencil_cylinder_vbo.get());
    ConfigureBufferHandleData(m_portal_stencil_cylinder_vbo, vbo_sizec * sizeof(GLfloat), (void* const)&vbo_datac[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);

    VertexAttributeLayout portal_stencil_cube_layout;
    portal_stencil_cube_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].in_use = true;
    portal_stencil_cube_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].buffer_id = VAO_ATTRIB::POSITION;
    portal_stencil_cube_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_count = 3;
    portal_stencil_cube_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].element_type = GL_FLOAT;
    portal_stencil_cube_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].stride_in_bytes = 8 * sizeof(GLfloat);
    portal_stencil_cube_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].offset_in_bytes = 0 * sizeof(GLfloat);

    portal_stencil_cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use = true;
    portal_stencil_cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].buffer_id = VAO_ATTRIB::POSITION;
    portal_stencil_cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_count = 3;
    portal_stencil_cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].element_type = GL_FLOAT;
    portal_stencil_cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].stride_in_bytes = 8 * sizeof(GLfloat);
    portal_stencil_cube_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].offset_in_bytes = 5 * sizeof(GLfloat);

    m_portal_stencil_cube_vao = CreateMeshHandle(portal_stencil_cube_layout);
    buffers = GetBufferHandlesForMeshHandle(m_portal_stencil_cube_vao.get());
    m_portal_stencil_cube_vbo = (*buffers)[(GLuint)VAO_ATTRIB::POSITION];
    BindBufferHandle(m_portal_stencil_cube_vbo.get());
    ConfigureBufferHandleData(m_portal_stencil_cube_vbo, vbo_size9 * sizeof(GLfloat), (void* const)&vbo_data9[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);

    //InitScreenQuadShaderProgram();
    //InitScreenAlignedQuad();
}

void AbstractRenderer::InitializeState()
{
//    qDebug() << "AbstractRenderer::InitializeState()";
    // Misc Enables
#ifndef __ANDROID__
    MathUtil::glFuncs->glEnable(GL_MULTISAMPLE);
    MathUtil::glFuncs->glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    //MathUtil::glFuncs->glEnable(GL_FRAMEBUFFER_SRGB);
#endif

    // Blending State: Pre-multiplied alpha blending
    MathUtil::glFuncs->glEnable(GL_BLEND);
    MathUtil::glFuncs->glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    MathUtil::glFuncs->glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
                                           GL_ONE, GL_ZERO);

    m_blend_src = BlendFunction::ONE;
    m_current_blend_src = m_blend_src;
    m_blend_dest = BlendFunction::ONE_MINUS_SRC_ALPHA;
    m_current_blend_dest = m_blend_dest;

    // Data Transfer State
    MathUtil::glFuncs->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    MathUtil::glFuncs->glPixelStorei(GL_PACK_ALIGNMENT, 1);

    // Depth Test State
    MathUtil::glFuncs->glEnable(GL_DEPTH_TEST);
    m_depth_mask = DepthMask::DEPTH_WRITES_ENABLED;
    m_current_depth_mask = m_depth_mask;
    MathUtil::glFuncs->glDepthMask(GL_TRUE);
    m_depth_func = DepthFunc::LEQUAL;
    m_current_depth_func = m_depth_func;
    MathUtil::glFuncs->glClearDepth(1.0);
    MathUtil::glFuncs->glDepthFunc(GL_LEQUAL);

    // Custom clip planes
#ifndef __ANDROID__
    MathUtil::glFuncs->glEnable(GL_CLIP_DISTANCE0);
#endif

    // Optional Depth Precision improvements
#ifndef __ANDROID__
    if (QOpenGLContext::currentContext()->hasExtension(QByteArrayLiteral("GL_ARB_clip_control")))
    {
        m_glClipControl = (PFNGLCLIPCONTROLPROC)QOpenGLContext::currentContext()->getProcAddress(QByteArrayLiteral("glClipControl"));

        if (m_glClipControl != nullptr)
        {
            qDebug() << "glClipControl extension found, enabling improved depth precision";
            m_enhanced_depth_precision_supported = true;
        }
        else
        {
            qDebug () << "glClipControl NOT SUPPORTED!";
            m_enhanced_depth_precision_supported = false;
        }
    }
#else
    m_enhanced_depth_precision_supported = false;
#endif

    // Color and Raster state
    MathUtil::glFuncs->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    m_color_mask = ColorMask::COLOR_WRITES_ENABLED;
    m_current_color_mask = m_color_mask;
    MathUtil::glFuncs->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    m_poly_mode = PolyMode::FILL;
    m_current_poly_mode = m_poly_mode;
#ifndef __ANDROID__
    MathUtil::glFuncs->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
    MathUtil::glFuncs->glEnable(GL_CULL_FACE);
    m_face_cull_mode = FaceCullMode::BACK;
    m_current_face_cull_mode = m_face_cull_mode;
    MathUtil::glFuncs->glCullFace(GL_BACK);

    // Stencil Test State
    MathUtil::glFuncs->glEnable(GL_STENCIL_TEST);
    MathUtil::glFuncs->glClearStencil(0);
    m_stencil_func = StencilFunc(StencilTestFuncion::ALWAYS, 0, 0xffffffff);
    m_current_stencil_func = m_stencil_func;
    MathUtil::glFuncs->glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
    m_stencil_op = StencilOp(StencilOpAction::KEEP, StencilOpAction::KEEP, StencilOpAction::KEEP);
    m_current_stencil_op = m_stencil_op;
    MathUtil::glFuncs->glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

    // Create sync objects to use for frame resource fencing
    m_syncObjects.resize(BUFFER_CHUNK_COUNT);

    // Initialize GPU Frame timers
#ifndef __ANDROID__
    m_GPUTimeQuery.resize(BUFFER_CHUNK_COUNT);
    for (unsigned int i = 0; i < m_GPUTimeQuery.size(); ++i)
    {
        MathUtil::glFuncs->glGenQueries(1, &m_GPUTimeQuery[i]);
        MathUtil::glFuncs->glBeginQuery(GL_TIME_ELAPSED, m_GPUTimeQuery[i]);
        MathUtil::glFuncs->glEndQuery(GL_TIME_ELAPSED);
    }
#endif

    m_GPUTimeQueryResults.resize(180);
    m_CPUTimeQueryResults.resize(180);
    m_frame_time_timer.start();
}

void AbstractRenderer::InitScreenAlignedQuad()
{
    /*GLfloat vertices[] = {	-1.0f, -1.0f,
                            -1.0f,  1.0f,
                             1.0f,  1.0f,
                             1.0f, -1.0f};

    GLuint indices[] = { 0,3,1,
                         3,2,1};

    m_fullScreenQuadVAO = CreateMeshHandle(true, false, false, false, false, false, false, false, false, false, false, false, false, true);
    BindMeshHandle(m_fullScreenQuadVAO.get());

    auto buffers = GetBufferHandlesForMeshHandle(m_fullScreenQuadVAO.get());
    m_fullScreenQuadVBOs.resize(2);
    m_fullScreenQuadVBOs[0] = (*buffers)[(GLuint)VAO_ATTRIB::POSITION];
    BindBufferHandle(m_fullScreenQuadVBOs[0].get());
    ConfigureBufferHandleData(m_fullScreenQuadVBOs[0], sizeof(GLfloat) * 8, (void* const)&vertices[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);
    MathUtil::glFuncs->glVertexAttribPointer((GLuint)VAO_ATTRIB::POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);
    MathUtil::glFuncs->glVertexAttribDivisor((GLuint)VAO_ATTRIB::POSITION, 0);
    MathUtil::glFuncs->glEnableVertexAttribArray((GLuint)VAO_ATTRIB::POSITION);

    m_fullScreenQuadVBOs[1] = (*buffers)[(GLuint)VAO_ATTRIB::INDICES];
    BindBufferHandle(m_fullScreenQuadVBOs[1].get());
    ConfigureBufferHandleData(m_fullScreenQuadVBOs[1], sizeof(GLuint) * 6, (void* const)&indices[0], BufferHandle::BUFFER_USAGE::STATIC_DRAW);*/
}

void AbstractRenderer::InitScreenQuadShaderProgram()
{
    const QString full_screen_quad_vertex_shader_path = MathUtil::GetApplicationPath() + "assets/shaders/fullScreenQuad_vert.txt";
    const QString full_screen_quad_frag_shader_path = MathUtil::GetApplicationPath() + "assets/shaders/fullScreenQuad_frag.txt";

    m_fullScreenQuadShaderProgram = 0;
    MathUtil::loadGLShaderFromFile(&m_fullScreenQuadShaderProgram, full_screen_quad_vertex_shader_path, full_screen_quad_frag_shader_path);

    MathUtil::glFuncs->glUseProgram(m_fullScreenQuadShaderProgram);
    GLint texture0Loc = MathUtil::glFuncs->glGetUniformLocation(m_fullScreenQuadShaderProgram, "iTexture0");
    GLint texture1Loc = MathUtil::glFuncs->glGetUniformLocation(m_fullScreenQuadShaderProgram, "iTexture1");
    GLint texture2Loc = MathUtil::glFuncs->glGetUniformLocation(m_fullScreenQuadShaderProgram, "iTexture2");
    GLint texture3Loc = MathUtil::glFuncs->glGetUniformLocation(m_fullScreenQuadShaderProgram, "iTexture3");

    MathUtil::glFuncs->glUniform1i(texture0Loc, 0);
    MathUtil::glFuncs->glUniform1i(texture1Loc, 1);
    MathUtil::glFuncs->glUniform1i(texture2Loc, 2);
    MathUtil::glFuncs->glUniform1i(texture3Loc, 3);
}

void AbstractRenderer::InitializeLightUBOs()
{
    m_dummyLights.m_lights.resize(MAX_LIGHTS);

    for (uint32_t ubo_index = 0; ubo_index < BUFFER_CHUNK_COUNT; ++ubo_index)
    {
        MathUtil::glFuncs->glGenBuffers(1, &m_light_UBOs[ubo_index]);
        MathUtil::glFuncs->glBindBuffer(GL_UNIFORM_BUFFER, m_light_UBOs[ubo_index]);
        MathUtil::glFuncs->glBufferData(GL_UNIFORM_BUFFER, sizeof(Light) * static_cast<GLsizeiptr>(MAX_LIGHTS), &(m_dummyLights.m_lights[0]), GL_DYNAMIC_DRAW);
    }
    // Attach active UBO to block index 0
    MathUtil::glFuncs->glBindBuffer(GL_UNIFORM_BUFFER, m_light_UBOs[0]);
    MathUtil::glFuncs->glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_light_UBOs[0]);
}

void AbstractRenderer::PushNewLightData(LightContainer const * p_lightContainer)
{
    // Bind UBO, update it's contents, attach to block index 0
    MathUtil::glFuncs->glBindBuffer(GL_UNIFORM_BUFFER, m_light_UBOs[m_active_light_UBO_index]);

    auto const light_count = p_lightContainer->m_lights.size();

    // Check that we have the appropriate amount of data in the container
    // This can happen when we try to draw a new portal
    if (light_count == 0)
    {
        // Push a single dummy light with defaults so that lighting is disabled.
        MathUtil::glFuncs->glBufferData(GL_UNIFORM_BUFFER, sizeof(Light) * 1, &(m_dummyLights.m_lights[0]), GL_DYNAMIC_DRAW);
    }
    else
    {
        MathUtil::glFuncs->glBufferData(GL_UNIFORM_BUFFER, sizeof(Light) * light_count, &(p_lightContainer->m_lights[0]), GL_DYNAMIC_DRAW);
    }

    MathUtil::glFuncs->glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_light_UBOs[m_active_light_UBO_index]);

    // Update Active light index
    m_active_light_UBO_index = (m_active_light_UBO_index + 1) % static_cast<GLuint>(BUFFER_CHUNK_COUNT);
}

TextureSet AbstractRenderer::GetCurrentlyBoundTextures()
{
    TextureSet texture_set;

    for (size_t i = 0; i < ASSETSHADER_NUM_COMBINED_TEXURES; ++i)
    {
        texture_set.SetTextureHandle(i, m_bound_texture_handles[i]);
    }
    return texture_set;
}



bool AbstractRenderer::GetIsEnhancedDepthPrecisionSupported() const
{
    return m_enhanced_depth_precision_supported;
}

bool AbstractRenderer::GetIsUsingEnhancedDepthPrecision() const
{
    return (m_enhanced_depth_precision_used);
}

void AbstractRenderer::SetIsUsingEnhancedDepthPrecision(bool const p_is_using)
{
    m_framebuffer_requires_initialization = (m_framebuffer_requires_initialization == false) ? (m_enhanced_depth_precision_supported && (m_enhanced_depth_precision_used != p_is_using)) : m_framebuffer_requires_initialization;
    m_enhanced_depth_precision_used = p_is_using;
}

void AbstractRenderer::SetFaceCullMode(FaceCullMode p_face_cull_mode)
{        
    if ((m_current_face_cull_mode != p_face_cull_mode) && m_update_GPU_state)
    {
        // If we are currently disabled, enable face culling before updating the value
        if (m_current_face_cull_mode == FaceCullMode::DISABLED)
        {
            MathUtil::glFuncs->glEnable(GL_CULL_FACE);
            MathUtil::glFuncs->glCullFace(static_cast<GLenum>(p_face_cull_mode));
        }
        // Else, we are already enabled, so disable if needed
        else if (p_face_cull_mode == FaceCullMode::DISABLED)
        {
            MathUtil::glFuncs->glDisable(GL_CULL_FACE);
        }
        // otherwise just change the face culling mode
        else
        {
            MathUtil::glFuncs->glCullFace(static_cast<GLenum>(p_face_cull_mode));
        }

        m_current_face_cull_mode = p_face_cull_mode;
    }
}

FaceCullMode AbstractRenderer::GetFaceCullMode() const
{
    return m_face_cull_mode;
}

void AbstractRenderer::SetDefaultFaceCullMode(FaceCullMode p_face_cull_mode)
{
    m_default_face_cull_mode = p_face_cull_mode;
}

FaceCullMode AbstractRenderer::GetDefaultFaceCullMode() const
{
    return m_default_face_cull_mode;
}

void AbstractRenderer::SetMirrorMode(bool p_mirror_mode)
{
    m_mirror_mode = p_mirror_mode;
}

bool AbstractRenderer::GetMirrorMode() const
{
    return m_mirror_mode;
}

void AbstractRenderer::SetDepthFunc(DepthFunc p_depth_func)
{
	m_depth_func = p_depth_func;
	// Floating point depth requires that we invert the usual 0 to 1 depth range to be 1 to 0
    // which means we need to also flip the depth compare functions
	if (GetIsUsingEnhancedDepthPrecision() == true
		&& GetIsEnhancedDepthPrecisionSupported() == true)
    {
        if (p_depth_func == DepthFunc::LEQUAL)
        {
            p_depth_func = DepthFunc::GEQUAL;
        }
        else if (p_depth_func == DepthFunc::LESS)
        {
            p_depth_func = DepthFunc::GREATER;
        }
		else if (p_depth_func == DepthFunc::GEQUAL)
		{
			p_depth_func = DepthFunc::LEQUAL;
		}
		else if (p_depth_func == DepthFunc::GREATER)
		{
			p_depth_func = DepthFunc::LESS;
		}
    }

	if (m_current_depth_func != p_depth_func && m_update_GPU_state)
    {
		MathUtil::glFuncs->glDepthFunc(static_cast<GLenum>(p_depth_func));
		m_current_depth_func = p_depth_func;
    }
}

DepthFunc AbstractRenderer::GetDepthFunc() const
{
    return m_depth_func;
}

void AbstractRenderer::SetDepthMask(DepthMask p_depth_mask)
{
    m_depth_mask = p_depth_mask;

    if (m_current_depth_mask != m_depth_mask && m_update_GPU_state)
    {
        MathUtil::glFuncs->glDepthMask(static_cast<GLboolean>(m_depth_mask));
        m_current_depth_mask = m_depth_mask;
    }
}

DepthMask AbstractRenderer::GetDepthMask() const
{
    return m_depth_mask;
}

void AbstractRenderer::SetStencilFunc(StencilFunc p_stencil_func)
{
    m_stencil_func = p_stencil_func;

    if (m_current_stencil_func != m_stencil_func && m_update_GPU_state)
    {
        MathUtil::glFuncs->glStencilFunc(static_cast<GLenum>(m_stencil_func.GetStencilTestFunction()),
                                         static_cast<GLint>(m_stencil_func.GetStencilReferenceValue()),
                                         static_cast<GLuint>(m_stencil_func.GetStencilMask()));
        m_current_stencil_func = m_stencil_func;
    }
}



StencilFunc AbstractRenderer::GetStencilFunc() const
{
    return m_stencil_func;
}


void AbstractRenderer::SetStencilOp(StencilOp p_stencil_op)
{
    m_stencil_op = p_stencil_op;

    if (m_current_stencil_op != m_stencil_op && m_update_GPU_state)
    {
        MathUtil::glFuncs->glStencilOp(static_cast<GLenum>(m_stencil_op.GetStencilFailAction()),
                                         static_cast<GLenum>(m_stencil_op.GetDepthFailAction()),
                                         static_cast<GLenum>(m_stencil_op.GetPassAction()));
        m_current_stencil_op = m_stencil_op;
    }
}

StencilOp AbstractRenderer::GetStencilOp() const
{
    return m_stencil_op;
}

void AbstractRenderer::SetPolyMode(PolyMode p_poly_mode)
{
    m_poly_mode = p_poly_mode;

    if (m_current_poly_mode != m_poly_mode && m_update_GPU_state)
    {
#ifndef __ANDROID__
        MathUtil::glFuncs->glPolygonMode(GL_FRONT_AND_BACK,
                                         static_cast<GLenum>(m_poly_mode));
        m_current_poly_mode = m_poly_mode;
#endif
    }
}

PolyMode AbstractRenderer::GetPolyMode() const
{
    return m_poly_mode;
}

void AbstractRenderer::SetColorMask(ColorMask p_color_mask)
{
    m_color_mask = p_color_mask;

    if (m_current_color_mask != m_color_mask && m_update_GPU_state && m_allow_color_mask)
    {
        MathUtil::glFuncs->glColorMask(static_cast<GLboolean>(m_color_mask),
                                       static_cast<GLboolean>(m_color_mask),
                                       static_cast<GLboolean>(m_color_mask),
                                       static_cast<GLboolean>(m_color_mask));
        m_current_color_mask = m_color_mask;
    }
}

ColorMask AbstractRenderer::GetColorMask() const
{
    return m_color_mask;
}

void AbstractRenderer::GetViewportsAndCameraCount(QVector<float> & viewports, RENDERER::RENDER_SCOPE const p_scope, uint32_t & camera_count)
{
    QVector4D viewport;
    camera_count = m_scoped_cameras_cache[m_rendering_index][static_cast<size_t>(p_scope)].size();
    for (size_t camera_index = 0; camera_index < camera_count; camera_index++)
    {
        VirtualCamera& camera = m_scoped_cameras_cache[m_rendering_index][static_cast<size_t>(p_scope)][camera_index];
        viewport = camera.GetViewport();
        viewports.push_back(viewport.x());
        viewports.push_back(viewport.y());
        viewports.push_back(viewport.z());
        viewports.push_back(viewport.w());
    }
}

void AbstractRenderer::GenerateEnvMapsFromCubemapTextureHandle(Cubemaps& p_cubemaps)
{
// STAGE 1: Generate and save to memory, dds files containing the 6 faces of the cubemap
    // Bind Texture Handle
    BindTextureHandle(&m_texture_handle_to_GL_ID, 15, p_cubemaps.m_cubemap);

    // Get pixel channel swizzles
    int gl_tex_swizzle_mask[4] = {GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
    MathUtil::glFuncs->glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_R, &(gl_tex_swizzle_mask[0]));
    MathUtil::glFuncs->glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_G, &(gl_tex_swizzle_mask[1]));
    MathUtil::glFuncs->glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_B, &(gl_tex_swizzle_mask[2]));
    MathUtil::glFuncs->glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SWIZZLE_A, &(gl_tex_swizzle_mask[3]));

    // Calculate optimal mip level (Face size of at least 256)
    int gl_max_level = -2;
    MathUtil::glFuncs->glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, &gl_max_level);
    int const target_mip_level = (gl_max_level < 9) ? 0 : gl_max_level - 8;

    // Get width of chosen mip level
    int gl_tex_width = -2;
    MathUtil::glFuncs->glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, target_mip_level, GL_TEXTURE_WIDTH, &gl_tex_width);

    // Get pixel channel sizes in bytes
    int gl_tex_red_size = -2;
    int gl_tex_green_size = -2;
    int gl_tex_blue_size = -2;
    int gl_tex_alpha_size = -2;
    MathUtil::glFuncs->glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, target_mip_level, GL_TEXTURE_RED_SIZE, &gl_tex_red_size);
    MathUtil::glFuncs->glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, target_mip_level, GL_TEXTURE_GREEN_SIZE, &gl_tex_green_size);
    MathUtil::glFuncs->glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, target_mip_level, GL_TEXTURE_BLUE_SIZE, &gl_tex_blue_size);
    MathUtil::glFuncs->glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, target_mip_level, GL_TEXTURE_ALPHA_SIZE, &gl_tex_alpha_size);

    // Allocate memory for holding raw face data
    size_t const pixel_data_size = (gl_tex_red_size / 8 * ((gl_tex_red_size == 8) ? 3 : 4));
    QVector<uint8_t> pixel_data(pixel_data_size * gl_tex_width * gl_tex_width, 0);
    // For each face
    gli::format format = (gl_tex_red_size == 8) ? gli::FORMAT_BGR8_UNORM_PACK8 : gli::FORMAT_RGBA16_SFLOAT_PACK16;
    gli::extent2d extent = { gl_tex_width, gl_tex_width };
    size_t const levels = 1;
    // Allocate Texture
    gli::texture2d texture2d(format, extent, levels);
    for (int face_index = 0; face_index < 6; ++face_index)
    {
        // Load face data into memory via glReadPixels
#ifndef __ANDROID__
        MathUtil::glFuncs->glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index, target_mip_level, (gl_tex_red_size == 8) ? GL_BGR : GL_RGBA, (gl_tex_red_size == 8) ? GL_UNSIGNED_BYTE : GL_HALF_FLOAT, pixel_data.data());
#else
        GLint draw_fbo = 0;
        GLint read_fbo = 0;
        GLuint dummy_fbo = 0;

        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fbo);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fbo);

        MathUtil::glFuncs->glGenFramebuffers(1, &dummy_fbo);
        MathUtil::glFuncs->glBindFramebuffer(GL_FRAMEBUFFER, dummy_fbo);

        MathUtil::glFuncs->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index, GetTextureFromHandle(&m_texture_handle_to_GL_ID, p_cubemaps.m_cubemap).second, 0);
        MathUtil::glFuncs->glReadPixels(0, 0, gl_tex_width, gl_tex_width, (gl_tex_red_size == 8) ? GL_RGB : GL_RGBA, (gl_tex_red_size == 1) ? GL_UNSIGNED_BYTE : GL_HALF_FLOAT, pixel_data.data());

        MathUtil::glFuncs->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fbo);
        MathUtil::glFuncs->glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo);

        MathUtil::glFuncs->glDeleteFramebuffers(1, &dummy_fbo);
#endif

        // Create and save gli based dds file into memory
        memcpy(texture2d.data(0, 0, 0), pixel_data.data(), pixel_data.size());
        gli::save_dds(texture2d, p_cubemaps.m_dds_data[face_index]);
    }

    p_cubemaps.m_channel_size = gl_tex_red_size;

}

void AbstractRenderer::RenderObjectsStereoViewportInstanced(const RENDERER::RENDER_SCOPE p_scope,
                                                            const QVector<AbstractRenderCommand> &p_object_render_commands,
                                                            const QHash<StencilReferenceValue, LightContainer> &p_scoped_light_containers)
{
#ifndef __ANDROID__
    const size_t camera_count_this_scope = m_per_frame_scoped_cameras_view_matrix[static_cast<size_t>(p_scope)].size();

    QVector<float> viewports;
    viewports.reserve(camera_count_this_scope * 4);

    QVector4D viewport;
    for (size_t camera_index = 0; camera_index < camera_count_this_scope; camera_index++)
    {
        VirtualCamera& camera = m_scoped_cameras_cache[m_rendering_index][static_cast<size_t>(p_scope)][camera_index];
        viewport = camera.GetViewport();
        viewports.push_back(viewport.x());
        viewports.push_back(viewport.y());
        viewports.push_back(viewport.z());
        viewports.push_back(viewport.w());
    }

    const size_t cmd_array_limit = p_object_render_commands.size();
    const size_t cmd_count = cmd_array_limit;
    const float viewport_count_float = camera_count_this_scope;
    const float viewport_count = camera_count_this_scope;

    float encompassing_viewport[4] = { 0.0, 0.0, 0.0, 0.0};
    for (size_t viewport_index = 0; viewport_index < viewport_count; ++ viewport_index)
    {
        encompassing_viewport[0] = (viewports[viewport_index * 4 + 0] < encompassing_viewport[0]) ? viewports[viewport_index * 4 + 0] : encompassing_viewport[0];
        encompassing_viewport[1] = (viewports[viewport_index * 4 + 1] < encompassing_viewport[1]) ? viewports[viewport_index * 4 + 1] : encompassing_viewport[1];
        encompassing_viewport[2] = (viewports[viewport_index * 4 + 2] > encompassing_viewport[2]) ? viewports[viewport_index * 4 + 2] : encompassing_viewport[2];
        encompassing_viewport[3] = (viewports[viewport_index * 4 + 3] > encompassing_viewport[3]) ? viewports[viewport_index * 4 + 3] : encompassing_viewport[3];
    }

    if (p_scope == RENDERER::RENDER_SCOPE::CURRENT_ROOM_SKYBOX)
    {
        if (camera_count_this_scope == 0)
        {
            MathUtil::glFuncs->glViewport(0, 0, m_window_width, m_window_height);
        }
        else
        {
            MathUtil::glFuncs->glViewport(encompassing_viewport[0], encompassing_viewport[1], encompassing_viewport[2], encompassing_viewport[3]);
        }
        m_update_GPU_state = true;
        SetDepthMask(DepthMask::DEPTH_WRITES_ENABLED);
        SetColorMask(ColorMask::COLOR_WRITES_ENABLED);
        MathUtil::glFuncs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //clear
        m_update_GPU_state = false;
    }

    if (camera_count_this_scope == 0)
    {
        return;
    }

    StencilReferenceValue current_stencil_ref = -1;

    bool is_left_eye = true;
    GLuint current_programID = 0;
    ProgramHandle* current_program_handle = nullptr;

    QVector<GLfloat> default_color;
    default_color.resize(4);
    default_color[0] = 1.0f;
    default_color[1] = 1.0f;
    default_color[2] = 1.0f;
    default_color[3] = 1.0f;
    QVector<GLfloat> default_normal;
    default_normal.resize(4);
    default_normal[0] = 0.0f;
    default_normal[1] = 0.0f;
    default_normal[2] = 0.0f;
    default_normal[3] = 0.0f;
    AssetShader_Frame const * current_frame_uniforms = nullptr;
    AssetShader_Room const * current_room_uniforms = nullptr;
    AssetShader_Material const * current_material_uniforms = nullptr;
    AssetShader_Object const * current_object_uniforms = nullptr;

    m_update_GPU_state = true;

    m_active_texture_slot_render = 0;
    MathUtil::glFuncs->glActiveTexture(GL_TEXTURE0);

    float base_viewport = 0.0f;
    float current_base_viewport = 0.0f;

    // If we have at least one command to draw in this scope
    if (cmd_count != 0)
    {
        // For each command in this scope
        size_t cmd_stride = 0;
        for (size_t cmd_index = 0; cmd_index < cmd_count;)
        {
            cmd_stride = camera_count_this_scope;
            MathUtil::glFuncs->glViewportArrayv(0, camera_count_this_scope, viewports.data());

            AbstractRenderCommand const & current_render_command = p_object_render_commands[cmd_index];

            // Change face culling state if necessary,
            // SetFaceCullMode() will only change state if needed so we call it either way.
            //FaceCullMode face_cull_mode = current_render_command.GetFaceCullMode();
            SetFaceCullMode(current_render_command.m_active_face_cull_mode);

            // Push polymode if necessary,
            // SetPolyMode() will only change state if needed so we call it either way.
            //PolyMode poly_mode = current_render_command.GetPolyMode();
            SetPolyMode(current_render_command.m_poly_mode);

            // Push depth state if necessary,
            // SetDepthFunc() and SetDepthMask() will only change state if needed so we call it either way.
            //DepthFunc depth_func = current_render_command.GetDepthFunc();
            SetDepthFunc(current_render_command.m_depth_func);
            //DepthMask depth_mask = current_render_command.GetDepthMask();
            SetDepthMask(current_render_command.m_depth_mask);

            // Push color mask state if necessary,
            // SetColorMask() will only change state if needed so we call it either way.
            //ColorMask color_mask = current_render_command.GetColorMask();
            SetColorMask(current_render_command.m_color_mask);

            // Push stencil test state if necessary,
            // SetStencilFunc() will only change state if needed so we call it either way.
            //StencilFunc stencil_func = current_render_command.GetStencilFunc();
            StencilReferenceValue stencil_ref = current_render_command.m_stencil_func.m_ref_value;
            SetStencilFunc(current_render_command.m_stencil_func);
            //StencilOp stencil_op = current_render_command.GetStencilOp();
            SetStencilOp(current_render_command.m_stencil_op);

            // Bind new shader if necessary
            bool shader_changed = false;
            ProgramHandle* program_handle = current_render_command.m_shader;
            if (program_handle != nullptr) // Skip when an invalid program ID is passed
            {
                if (current_program_handle == nullptr || program_handle != current_program_handle)
                {
                    GLuint programID = GetProgramHandleID(program_handle);
                    MathUtil::glFuncs->glUseProgram(programID);
                    current_programID = programID;
                    current_program_handle = program_handle;
                    shader_changed = true;
                }

                // Special case for textures that vary per-viewport
                base_viewport = cmd_index % camera_count_this_scope;
                is_left_eye = m_per_frame_scoped_cameras_is_left_eye[static_cast<size_t>(p_scope)][base_viewport];
                bool is3DTexture = current_render_command.m_texture_set.GetIs3DTexture();

                // Push new frame uniforms if necessary
                AssetShader_Frame const * frame_uniforms = current_render_command.GetFrameUniformsPointer();
				if (frame_uniforms != nullptr 
					&& (current_frame_uniforms == nullptr 
					|| *frame_uniforms != *current_frame_uniforms
					|| shader_changed == true
					|| current_base_viewport != base_viewport)
					)
                {
                    ((AssetShader_Frame *)frame_uniforms)->iViewportCount = QVector4D(viewport_count_float, base_viewport, 0.0f, 0.0f);
                    UpdateFrameUniforms(current_programID, frame_uniforms);
                    current_frame_uniforms = frame_uniforms;
                    current_base_viewport = base_viewport;
                }

                // Push new room uniforms if necessary
                AssetShader_Room const * room_uniforms = current_render_command.GetRoomUniformsPointer();
                if(current_room_uniforms == nullptr || *room_uniforms != *current_room_uniforms || shader_changed == true)
                {
                    UpdateRoomUniforms(current_programID, room_uniforms);
                    current_room_uniforms = room_uniforms;
                }

                // Push new light data if necessary
                if (current_stencil_ref != stencil_ref)
                {
                    current_stencil_ref = stencil_ref;
                    QHash<StencilReferenceValue, LightContainer>::const_iterator itr = p_scoped_light_containers.find(current_stencil_ref);
                    if (itr != p_scoped_light_containers.end())
                    {
                        //p_main_thread_renderer->PushNewLightData(&(itr->second));
                        PushNewLightData(&(itr.value()));
                    }
                    else
                    {
                        PushNewLightData(&m_empty_light_container);
                    }
                }

                // Push new object uniforms if necessary
                AssetShader_Object const * object_uniforms = current_render_command.GetObjectUniformsPointer();
                if (current_object_uniforms == nullptr || *object_uniforms != *current_object_uniforms || shader_changed == true)
                {
                    UpdateObjectUniforms(current_programID, object_uniforms);
                    current_object_uniforms = object_uniforms;
                }

                // Push new material uniforms if necessary
                AssetShader_Material const * material_uniforms = current_render_command.GetMaterialUniformsPointer();
                if (current_material_uniforms == nullptr || *material_uniforms != *current_material_uniforms || shader_changed == true)
                {
                    UpdateMaterialUniforms(current_programID, material_uniforms);
                    current_material_uniforms = material_uniforms;
                }

                // Push texture state if necessary
                for (uint32_t texture_slot = 0; texture_slot < ASSETSHADER_NUM_COMBINED_TEXURES; ++texture_slot)
                {
                    TextureHandle * texture_handle_ref = current_render_command.m_texture_set.GetTextureHandleRef(texture_slot, is_left_eye);
                    if (!texture_handle_ref || (texture_handle_ref->m_UUID.m_in_use_flag == 0))
                    {
                        continue;
                    }

                    BindTextureHandleRef(&(m_texture_handle_to_GL_ID), texture_slot, texture_handle_ref);
                }

                // Render object
                GLenum current_mode = static_cast<GLenum>(current_render_command.GetPrimitiveType());
                GLuint current_primitive_count = current_render_command.GetPrimitiveCount();
                GLuint instance_count = (is3DTexture) ? 1 : camera_count_this_scope * current_render_command.GetInstanceCount();
                //GLuint instance_count = (is3DTexture) ? 1 : camera_count_this_scope;
                cmd_stride = instance_count;
                if (current_primitive_count != 0)
                {
                    auto current_mesh_handle = current_render_command.m_mesh_handle;
                    if (current_mesh_handle != nullptr)
                    {
                        BindMeshHandle(current_mesh_handle);
                        if (current_mesh_handle->m_UUID.m_has_INDICES == 1)
                        {
                            MathUtil::glFuncs->glDrawElementsInstanced(current_mode, current_primitive_count, GL_UNSIGNED_INT, 0, instance_count);
                        }
                        else
                        {
                            MathUtil::glFuncs->glDrawArraysInstanced(current_mode, 0, current_primitive_count, instance_count);
                        }
                        // Vertex atrributes that are disabled during a draw by a VAO bind annoyingly need their value refreshed
                        // as they default to undefined every time they are disabled.
                        MathUtil::glFuncs->glVertexAttrib4fv((uint32_t)VAO_ATTRIB::COLOR, default_color.data());
                        MathUtil::glFuncs->glVertexAttrib4fv((uint32_t)VAO_ATTRIB::NORMAL, default_normal.data());
                    }
                }
            }
            cmd_index += cmd_stride;
        }
    }
    m_update_GPU_state = false;
#endif
}

void AbstractRenderer::RenderObjectsNaive(RENDERER::RENDER_SCOPE const , QVector<AbstractRenderCommand> const & , QHash<StencilReferenceValue, LightContainer> const & )
{
    /*uint32_t camera_count = 0;

    QVector<float> viewports;
    viewports.reserve(m_cameras_cache[m_main_thread_renderer->m_rendering_index].size() * 4);

    GetViewportsAndCameraCount(viewports, p_scope, camera_count);

    if (camera_count == 0)
    {
        return;
    }

    const size_t cmd_array_limit = p_object_render_commands.size();
    const size_t cmd_count = cmd_array_limit;
    const float viewport_count_float = 1.0f;
    const float viewport_count = 1.0f;
    const float instancing_SSBO_stride = static_cast<float>(camera_count);

    float encompassing_viewport[4] = { 0.0, 0.0, 0.0, 0.0};
    for (size_t viewport_index = 0; viewport_index < viewport_count; ++ viewport_index)
    {
        encompassing_viewport[0] = (viewports[viewport_index * 4 + 0] < encompassing_viewport[0]) ? viewports[viewport_index * 4 + 0] : encompassing_viewport[0];
        encompassing_viewport[1] = (viewports[viewport_index * 4 + 1] < encompassing_viewport[1]) ? viewports[viewport_index * 4 + 1] : encompassing_viewport[1];
        encompassing_viewport[2] = (viewports[viewport_index * 4 + 2] > encompassing_viewport[2]) ? viewports[viewport_index * 4 + 2] : encompassing_viewport[2];
        encompassing_viewport[3] = (viewports[viewport_index * 4 + 3] > encompassing_viewport[3]) ? viewports[viewport_index * 4 + 3] : encompassing_viewport[3];
    }

    StencilReferenceValue current_stencil_ref = 0;

    int cmd_index = 0;
    GLuint instance_arg = 0;
    GLuint draw_count = 0;
    bool is_left_eye = true;
    GLuint current_programID = 0;

    m_update_GPU_state = true;

    if (p_scope == RENDERER::RENDER_SCOPE::CURRENT_ROOM_PORTAL_STENCILS)
    {
        MathUtil::glFuncs->glViewport(encompassing_viewport[0], encompassing_viewport[1], encompassing_viewport[2], encompassing_viewport[3]);
        SetDepthMask(DepthMask::DEPTH_WRITES_ENABLED);
        SetColorMask(ColorMask::COLOR_WRITES_ENABLED);
        MathUtil::glFuncs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //clear
    }

    if (cmd_count != 0)
    {
        for (uint32_t camera_index = 0; camera_index < camera_count; ++camera_index)
        {
            draw_count = 0;
            MathUtil::glFuncs->glViewport(viewports[camera_index * 4], viewports[camera_index * 4 + 1], viewports[camera_index * 4 + 2], viewports[camera_index * 4 + 3]);
            cmd_index = camera_index;

            AbstractRenderCommand const & current_render_command = p_object_render_commands[cmd_index];

            // Push face culling state
            FaceCullMode current_face_cull_mode = current_render_command.GetFaceCullMode();
            SetFaceCullMode(current_face_cull_mode);

            // Push polymode
            PolyMode current_poly_mode = current_render_command.GetPolyMode();
            SetPolyMode(current_poly_mode);

            // Push depth test state
            DepthFunc current_depth_func = current_render_command.GetDepthFunc();
            SetDepthFunc(current_depth_func);
            DepthMask current_depth_mask = current_render_command.GetDepthMask();
            SetDepthMask(current_depth_mask);

            // Push color mask state
            ColorMask current_color_mask = current_render_command.GetColorMask();
            SetColorMask(current_color_mask);

            // Push stencil test state
            StencilFunc current_stencil_func = current_render_command.GetStencilFunc();
            SetStencilFunc(current_stencil_func);
            current_stencil_ref = current_stencil_func.GetStencilReferenceValue();
            StencilOp current_stencil_op = current_render_command.GetStencilOp();
            SetStencilOp(current_stencil_op);

            // Bind initial shader
            current_programID = GetProgramHandleID(current_render_command.m_shader.get());
            MathUtil::glFuncs->glUseProgram(current_programID);

            // Push frame uniforms
            AssetShader_Frame const * current_frame_uniforms = current_render_command.GetFrameUniformsPointer();
            ((AssetShader_Frame *)current_frame_uniforms)->iViewportCount = QVector4D(viewport_count_float, instancing_SSBO_stride, 0.0f, 0.0f);
            UpdateFrameUniforms(current_programID, current_frame_uniforms);

            // Push room uniforms
            AssetShader_Room const * current_room_uniforms = current_render_command.GetRoomUniformsPointer();
            UpdateRoomUniforms(current_programID, current_room_uniforms);

            // Push Light data to UBO
            auto itr = p_scoped_light_containers.find(current_stencil_ref);
            if (itr != p_scoped_light_containers.end())
            {
                PushNewLightData(itr->second);
            }
            else
            {
                PushNewLightData(LightContainer());
            }

            // Push object uniforms
            AssetShader_Object const * current_object_uniforms = current_render_command.GetObjectUniformsPointer();
            UpdateObjectUniforms(current_programID, current_object_uniforms);

            // Push material uniforms
            AssetShader_Material const * current_material_uniforms = current_render_command.GetMaterialUniformsPointer();
            UpdateMaterialUniforms(current_programID, current_material_uniforms);

            // Push texture state
            for (uint32_t texture_slot = 0; texture_slot < ASSETSHADER_NUM_COMBINED_TEXURES; ++texture_slot)
            {
                BindTextureHandle(&(this->m_texture_handle_to_GL_ID), texture_slot, current_render_command.m_texture_set.GetTextureHandle(texture_slot));
            }

            // Bind initial VAO
            auto current_mesh_handle = current_render_command.m_mesh_handle.get();
            if (current_mesh_handle != nullptr)
            {
                BindMeshHandle(current_mesh_handle);
            }

            // Render object
            GLenum mode = static_cast<GLenum>(current_render_command.GetPrimitiveType());
            GLuint first_index = current_render_command.GetFirstIndex();
            GLuint primitive_count = current_render_command.GetPrimitiveCount();
            GLuint instance_count = current_render_command.GetInstanceCount();
            instance_arg = viewport_count * instance_count;
            if (primitive_count != 0 && current_mesh_handle != nullptr)
            {
				if (instance_arg == 1)
				{
					if (current_mesh_handle->m_UUID.m_has_INDICES)
					{
                        MathUtil::glFuncs->glDrawElements(mode, primitive_count, GL_UNSIGNED_INT, 0);
					}
					else
					{
						MathUtil::glFuncs->glDrawArrays(mode, first_index, primitive_count);
					}
				}
				else
				{
					if (current_mesh_handle->m_UUID.m_has_INDICES)
					{
                        MathUtil::glFuncs->glDrawElementsInstanced(mode, primitive_count, GL_UNSIGNED_INT, 0, instance_arg);
					}
					else
					{
						MathUtil::glFuncs->glDrawArraysInstanced(mode, first_index, primitive_count, instance_arg);
					}
				}
            }

            cmd_index += instance_arg * camera_count;

            for (; cmd_index < cmd_array_limit;)
            {
                AbstractRenderCommand const & current_render_command = p_object_render_commands[cmd_index];

                // Change face culling state if necessary,
                // SetFaceCullMode() will only change state if needed so we call it either way.
                FaceCullMode face_cull_mode = current_render_command.GetFaceCullMode();
                SetFaceCullMode(face_cull_mode);

                // Push polymode if necessary,
                // SetPolyMode() will only change state if needed so we call it either way.
                PolyMode poly_mode = current_render_command.GetPolyMode();
                SetPolyMode(poly_mode);

                // Push depth state if necessary,
                // SetDepthFunc() and SetDepthMask() will only change state if needed so we call it either way.
                DepthFunc depth_func = current_render_command.GetDepthFunc();
                SetDepthFunc(depth_func);
                DepthMask depth_mask = current_render_command.GetDepthMask();
                SetDepthMask(depth_mask);

                // Push color mask state if necessary,
                // SetColorMask() will only change state if needed so we call it either way.
                ColorMask color_mask = current_render_command.GetColorMask();
                SetColorMask(color_mask);

                // Push stencil test state if necessary,
                // SetStencilFunc() will only change state if needed so we call it either way.
                StencilFunc stencil_func = current_render_command.GetStencilFunc();
                StencilReferenceValue stencil_ref = stencil_func.GetStencilReferenceValue();
                SetStencilFunc(stencil_func);
                StencilOp stencil_op = current_render_command.GetStencilOp();
                SetStencilOp(stencil_op);

                // Bind new shader if necessary
                GLuint programID = GetProgramHandleID(current_render_command.m_shader.get());
                bool shader_changed = false;
                if (programID != current_programID)
                {
                    MathUtil::glFuncs->glUseProgram(programID);
                    current_programID = programID;
                    shader_changed = true;
                }

                // Push new frame uniforms if necessary
                AssetShader_Frame const * frame_uniforms = current_render_command.GetFrameUniformsPointer();
                if(*frame_uniforms != *current_frame_uniforms || shader_changed == true)
                {
                    ((AssetShader_Frame *)frame_uniforms)->iViewportCount = QVector4D(viewport_count_float, instancing_SSBO_stride, 0.0f, 0.0f);
                    UpdateFrameUniforms(current_programID, frame_uniforms);
                    current_frame_uniforms = frame_uniforms;
                }

                // Push new room uniforms if necessary
                AssetShader_Room const * room_uniforms = current_render_command.GetRoomUniformsPointer();
                if(*room_uniforms != *current_room_uniforms || shader_changed == true)
                {
                    UpdateRoomUniforms(current_programID, room_uniforms);
                    current_room_uniforms = room_uniforms;
                }

                // Push new light data if necessary
                if (current_stencil_ref != stencil_ref)
                {
                    current_stencil_ref = stencil_ref;
                    auto itr = p_scoped_light_containers.find(current_stencil_ref);
                    if (itr != p_scoped_light_containers.end())
                    {
                        PushNewLightData(itr->second);
                    }
                    else
                    {
                        PushNewLightData(LightContainer());
                    }
                }

                // Push new object uniforms if necessary
                AssetShader_Object const * object_uniforms = current_render_command.GetObjectUniformsPointer();
                if (object_uniforms != current_object_uniforms || shader_changed == true)
                {
                    UpdateObjectUniforms(current_programID, object_uniforms);
                    current_object_uniforms = object_uniforms;
                }

                // Push new material uniforms if necessary
                AssetShader_Material const * material_uniforms = current_render_command.GetMaterialUniformsPointer();
                if (*material_uniforms != *current_material_uniforms || shader_changed == true)
                {
                    UpdateMaterialUniforms(current_programID, material_uniforms);
                    current_material_uniforms = material_uniforms;
                }

                // Push texture state if necessary
                for (uint32_t texture_slot = 0; texture_slot < ASSETSHADER_NUM_COMBINED_TEXURES; ++texture_slot)
                {
                    BindTextureHandle(&(this->m_texture_handle_to_GL_ID), texture_slot, current_render_command.m_texture_set.GetTextureHandle(texture_slot));
                }

                // Bind new VAO if necessary
                auto current_mesh_handle = current_render_command.m_mesh_handle.get();
                if (current_mesh_handle != nullptr)
                {
					BindMeshHandle(current_mesh_handle);
                }

                // Render object
                GLenum current_mode = static_cast<GLenum>(current_render_command.GetPrimitiveType());
                GLuint current_first_index = current_render_command.GetFirstIndex();
                GLuint current_primitive_count = current_render_command.GetPrimitiveCount();
                GLuint current_instance_count = current_render_command.GetInstanceCount();
                instance_arg = viewport_count * current_instance_count;

                if (primitive_count != 0 && current_mesh_handle != nullptr)
                {
                    if (instance_arg == 1)
                    {
                        if (current_mesh_handle->m_UUID.m_has_INDICES == 1)
                        {
                            MathUtil::glFuncs->glDrawElements(current_mode, current_primitive_count, GL_UNSIGNED_INT, 0);
                        }
                        else
                        {
                            MathUtil::glFuncs->glDrawArrays(current_mode, current_first_index, current_primitive_count);
                        }
                    }
                    else
                    {
                        if (current_mesh_handle->m_UUID.m_has_INDICES == 1)
                        {
                            MathUtil::glFuncs->glDrawElementsInstanced(current_mode, current_primitive_count, GL_UNSIGNED_INT, 0, instance_arg);
                        }
                        else
                        {
                            MathUtil::glFuncs->glDrawArraysInstanced(current_mode, current_first_index, current_primitive_count, instance_arg);
                        }
                    }
                }
                // We don't remove the commands when we create the batched draws, so we need to skip over them for now.
                cmd_index += instance_arg * camera_count;
            }

            // Only the first camera of each scope is counted as the "left eye camera"
            // this is to ensure sbs or ou3d images disaplay as expected
            is_left_eye = false;
        }
    }
    m_update_GPU_state = false;*/
}

void AbstractRenderer::RenderObjectsNaiveDecoupled(const RENDERER::RENDER_SCOPE p_scope,
                                                   const QVector<AbstractRenderCommand> &p_object_render_commands,
                                                   const QHash<StencilReferenceValue, LightContainer> &p_scoped_light_containers)
{
    const size_t camera_count_this_scope = m_per_frame_scoped_cameras_view_matrix[static_cast<size_t>(p_scope)].size();

    QVector<float> viewports;
    viewports.reserve(camera_count_this_scope);

    QVector4D viewport;
    for (size_t camera_index = 0; camera_index < camera_count_this_scope; camera_index++)
    {
        VirtualCamera& camera = m_scoped_cameras_cache[m_rendering_index][static_cast<size_t>(p_scope)][camera_index];
        viewport = camera.GetViewport();
        viewports.push_back(viewport.x());
        viewports.push_back(viewport.y());
        viewports.push_back(viewport.z());
        viewports.push_back(viewport.w());
    }

    const size_t cmd_array_limit = p_object_render_commands.size();
    const size_t cmd_count = cmd_array_limit;
    const float viewport_count_float = 1.0f;
    const float viewport_count = 1.0f;
    const float instancing_SSBO_stride = static_cast<float>(camera_count_this_scope);

    float encompassing_viewport[4] = { 0.0, 0.0, 0.0, 0.0};
    for (size_t viewport_index = 0; viewport_index < viewport_count; ++ viewport_index)
    {
        encompassing_viewport[0] = (viewports[viewport_index * 4 + 0] < encompassing_viewport[0]) ? viewports[viewport_index * 4 + 0] : encompassing_viewport[0];
        encompassing_viewport[1] = (viewports[viewport_index * 4 + 1] < encompassing_viewport[1]) ? viewports[viewport_index * 4 + 1] : encompassing_viewport[1];
        encompassing_viewport[2] = (viewports[viewport_index * 4 + 2] > encompassing_viewport[2]) ? viewports[viewport_index * 4 + 2] : encompassing_viewport[2];
        encompassing_viewport[3] = (viewports[viewport_index * 4 + 3] > encompassing_viewport[3]) ? viewports[viewport_index * 4 + 3] : encompassing_viewport[3];
    }

    if (p_scope == RENDERER::RENDER_SCOPE::CURRENT_ROOM_SKYBOX)
    {
        if (camera_count_this_scope == 0)
        {
            MathUtil::glFuncs->glViewport(0, 0, m_window_width, m_window_height);
        }
        else
        {
            MathUtil::glFuncs->glViewport(encompassing_viewport[0], encompassing_viewport[1], encompassing_viewport[2], encompassing_viewport[3]);
        }
        m_update_GPU_state = true;
        SetDepthMask(DepthMask::DEPTH_WRITES_ENABLED);
        SetColorMask(ColorMask::COLOR_WRITES_ENABLED);
        MathUtil::glFuncs->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //clear
        m_update_GPU_state = false;
    }

    /*if (p_scope == RENDERER::RENDER_SCOPE::CURRENT_ROOM_OBJECTS_CUTOUT
        || p_scope == RENDERER::RENDER_SCOPE::CHILD_ROOM_OBJECTS_CUTOUT)
    {
        MathUtil::glFuncs->glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        MathUtil::glFuncs->glEnable(GL_SAMPLE_ALPHA_TO_ONE);
    }
    else
    {
        MathUtil::glFuncs->glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        MathUtil::glFuncs->glDisable(GL_SAMPLE_ALPHA_TO_ONE);
    }*/

    if (camera_count_this_scope == 0)
    {
        return;
    }

    StencilReferenceValue current_stencil_ref = -1;

    bool is_left_eye = true;
    GLuint current_programID = 0;
    QVector<GLfloat> default_color;
    default_color.resize(4);
    default_color[0] = 1.0f;
    default_color[1] = 1.0f;
    default_color[2] = 1.0f;
    default_color[3] = 1.0f;
    QVector<GLfloat> default_normal;
    default_normal.resize(4);
    default_normal[0] = 0.0f;
    default_normal[1] = 0.0f;
    default_normal[2] = 0.0f;
    default_normal[3] = 0.0f;
    AssetShader_Frame const * current_frame_uniforms = nullptr;
    AssetShader_Room const * current_room_uniforms = nullptr;
    AssetShader_Material const * current_material_uniforms = nullptr;
    AssetShader_Object const * current_object_uniforms = nullptr;

    m_update_GPU_state = true;

    m_active_texture_slot_render = 0;
    MathUtil::glFuncs->glActiveTexture(GL_TEXTURE0);

    /*if (p_scope == RENDERER::RENDER_SCOPE::CURRENT_ROOM_OBJECTS_CUTOUT
        || p_scope == RENDERER::RENDER_SCOPE::CHILD_ROOM_OBJECTS_CUTOUT)
    {
        // This allows cutout alpha using the hashed-alpha test
        // to set it's coverage to it's alpha, allowing for correct blending
        // with background pixels
        MathUtil::glFuncs->glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        //MathUtil::glFuncs->glEnable(GL_SAMPLE_ALPHA_TO_ONE);
    }
    else
    {
        MathUtil::glFuncs->glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        //MathUtil::glFuncs->glDisable(GL_SAMPLE_ALPHA_TO_ONE);
    }*/

    // If we have at least one command to draw in this scope
    if (cmd_count != 0)
    {
        // For each command in this scope
        for (size_t cmd_index = 0; cmd_index < cmd_count; cmd_index += camera_count_this_scope)
        {
            // For each camera that affects this scope
            for (size_t camera_index = 0; camera_index < camera_count_this_scope; camera_index++)
            {
                MathUtil::glFuncs->glViewport(viewports[camera_index * 4], viewports[camera_index * 4 + 1], viewports[camera_index * 4 + 2], viewports[camera_index * 4 + 3]);
                is_left_eye = m_per_frame_scoped_cameras_is_left_eye[static_cast<size_t>(p_scope)][camera_index];

                AbstractRenderCommand const & current_render_command = p_object_render_commands[cmd_index + camera_index];

                // Change face culling state if necessary,
                // SetFaceCullMode() will only change state if needed so we call it either way.
                FaceCullMode face_cull_mode = current_render_command.GetFaceCullMode();
                SetFaceCullMode(face_cull_mode);

                // Push polymode if necessary,
                // SetPolyMode() will only change state if needed so we call it either way.
                PolyMode poly_mode = current_render_command.GetPolyMode();
                SetPolyMode(poly_mode);

                // Push depth state if necessary,
                // SetDepthFunc() and SetDepthMask() will only change state if needed so we call it either way.
                DepthFunc depth_func = current_render_command.GetDepthFunc();
                SetDepthFunc(depth_func);
                DepthMask depth_mask = current_render_command.GetDepthMask();
                SetDepthMask(depth_mask);

                // Push color mask state if necessary,
                // SetColorMask() will only change state if needed so we call it either way.
                ColorMask color_mask = current_render_command.GetColorMask();
                SetColorMask(color_mask);

                // Push stencil test state if necessary,
                // SetStencilFunc() will only change state if needed so we call it either way.
                StencilFunc stencil_func = current_render_command.GetStencilFunc();
                StencilReferenceValue stencil_ref = stencil_func.GetStencilReferenceValue();
                SetStencilFunc(stencil_func);
                StencilOp stencil_op = current_render_command.GetStencilOp();
                SetStencilOp(stencil_op);

                // Bind new shader if necessary
                GLuint programID = GetProgramHandleID(current_render_command.m_shader);
                bool shader_changed = false;
                if (programID != 0) // Skip when an invalid program ID is passed
                {
                    if (programID != current_programID)
                    {
                        MathUtil::glFuncs->glUseProgram(programID);
                        current_programID = programID;
                        shader_changed = true;
                    }

                    // Push new frame uniforms if necessary
                    AssetShader_Frame const * frame_uniforms = current_render_command.GetFrameUniformsPointer();
					if (frame_uniforms != nullptr
						&& (current_frame_uniforms == nullptr 
						|| *frame_uniforms != *current_frame_uniforms 
						|| shader_changed == true)
						)
                    {
                        ((AssetShader_Frame *)frame_uniforms)->iViewportCount = QVector4D(viewport_count_float, instancing_SSBO_stride, 0.0f, 0.0f);
                        UpdateFrameUniforms(current_programID, frame_uniforms);
                        current_frame_uniforms = frame_uniforms;
                    }

                    // Push new room uniforms if necessary
                    AssetShader_Room const * room_uniforms = current_render_command.GetRoomUniformsPointer();
                    if(current_room_uniforms == nullptr || *room_uniforms != *current_room_uniforms || shader_changed == true)
                    {
                        UpdateRoomUniforms(current_programID, room_uniforms);
                        current_room_uniforms = room_uniforms;
                    }

                    // Push new light data if necessary
                    if (current_stencil_ref != stencil_ref)
                    {
                        current_stencil_ref = stencil_ref;
                        QHash<StencilReferenceValue, LightContainer>::const_iterator itr = p_scoped_light_containers.find(current_stencil_ref);
                        if (itr != p_scoped_light_containers.end())
                        {
//                            p_main_thread_renderer->PushNewLightData(&(itr->second));
                            PushNewLightData(&(itr.value()));
                        }
                        else
                        {
                            PushNewLightData(&m_empty_light_container);
                        }
                    }

                    // Push new object uniforms if necessary
                    AssetShader_Object const * object_uniforms = current_render_command.GetObjectUniformsPointer();
                    if (current_object_uniforms == nullptr || *object_uniforms != *current_object_uniforms || shader_changed == true)
                    {
                        UpdateObjectUniforms(current_programID, object_uniforms);
                        current_object_uniforms = object_uniforms;
                    }

                    // Push new material uniforms if necessary
                    AssetShader_Material const * material_uniforms = current_render_command.GetMaterialUniformsPointer();
                    if (current_material_uniforms == nullptr || *material_uniforms != *current_material_uniforms || shader_changed == true)
                    {
                        UpdateMaterialUniforms(current_programID, material_uniforms);
                        current_material_uniforms = material_uniforms;
                    }

                    // Push texture state if necessary
                    for (uint32_t texture_slot = 0; texture_slot < ASSETSHADER_NUM_COMBINED_TEXURES; ++texture_slot)
                    {
                        TextureHandle * texture_handle_ref = current_render_command.m_texture_set.GetTextureHandleRef(texture_slot, is_left_eye);
                        if (!texture_handle_ref || (texture_handle_ref->m_UUID.m_in_use_flag == 0))
                        {
                            continue;
                        }

                        BindTextureHandleRef(&(m_texture_handle_to_GL_ID), texture_slot, texture_handle_ref);
                    }

                    // Render object
                    GLenum current_mode = static_cast<GLenum>(current_render_command.GetPrimitiveType());
                    GLuint current_first_index = current_render_command.GetFirstIndex();
                    GLuint current_primitive_count = current_render_command.GetPrimitiveCount();
                    if (current_primitive_count != 0)
                    {
                        auto current_mesh_handle = current_render_command.m_mesh_handle;
                        if (current_mesh_handle != nullptr)
                        {
                            BindMeshHandle(current_mesh_handle);
                            if (current_mesh_handle->m_UUID.m_has_INDICES == 1)
                            {
                                MathUtil::glFuncs->glDrawElements(current_mode, current_primitive_count, GL_UNSIGNED_INT, 0);
                            }
                            else
                            {
                                MathUtil::glFuncs->glDrawArrays(current_mode, current_first_index, current_primitive_count);
                            }

                            // Vertex atrributes that are disabled during a draw by a VAO bind annoyingly need their value refreshed
                            // as they default to undefined every time they are disabled.
                            MathUtil::glFuncs->glVertexAttrib4fv((uint32_t)VAO_ATTRIB::COLOR, default_color.data());
                            MathUtil::glFuncs->glVertexAttrib4fv((uint32_t)VAO_ATTRIB::NORMAL, default_normal.data());
                        }
                    }
                }
            }
        }
    }
    m_update_GPU_state = false;
}

GLuint AbstractRenderer::InitGLBuffer(GLsizeiptr p_dataSizeInBytes, void* p_data, GLenum p_bufferType, GLenum p_bufferUse)
{
    GLuint ID;
    MathUtil::glFuncs->glGenBuffers(1, &ID);
    MathUtil::glFuncs->glBindBuffer(p_bufferType, ID);
    MathUtil::glFuncs->glBufferData(p_bufferType, p_dataSizeInBytes, p_data, p_bufferUse);
    return ID;
}

GLuint AbstractRenderer::InitGLVertexAttribBuffer(GLenum p_dataType, GLboolean p_normalised, GLint p_dataTypeCount, GLsizeiptr p_dataSizeInBytes
                                                        , GLuint p_attribID, GLuint p_attribDivisor, GLsizei p_stride, GLenum p_bufferUse, void* p_data)
{
    GLuint ID = InitGLBuffer(p_dataSizeInBytes, p_data, GL_ARRAY_BUFFER, p_bufferUse);
    MathUtil::glFuncs->glVertexAttribPointer(p_attribID, p_dataTypeCount, p_dataType, p_normalised, p_stride, 0);
    MathUtil::glFuncs->glVertexAttribDivisor(p_attribID, p_attribDivisor);
    MathUtil::glFuncs->glEnableVertexAttribArray(p_attribID);
    return ID;
}

#if !defined(__APPLE__) && !defined(__ANDROID__)
void AbstractRenderer::initPersistentlyMappedGLBuffer(GLsizeiptr* p_bufferPtr, GLuint* p_bufferID
                                                            , GLsizeiptr* p_alignedChunkSizeInBytes, GLuint p_bufferIndex
                                                            , GLsizeiptr p_chunkSizeInBytes, GLuint p_chunkCount
                                                            , GLenum p_bufferType)
{
    MathUtil::glFuncs->glGenBuffers(1, p_bufferID);
    MathUtil::glFuncs->glBindBuffer(p_bufferType, *p_bufferID);
    MathUtil::glFuncs->glBindBufferBase(p_bufferType, p_bufferIndex, *p_bufferID);
    GLint bufferAlignment;
    MathUtil::glFuncs->glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &bufferAlignment);
    if (bufferAlignment < 16) bufferAlignment = 16;
    *p_alignedChunkSizeInBytes = static_cast<GLsizeiptr>(std::ceil((double)p_chunkSizeInBytes / (double)bufferAlignment) * bufferAlignment);
    GLsizeiptr totalBufferSize = *p_alignedChunkSizeInBytes * p_chunkCount;
    MathUtil::glFuncs->glBufferStorage(p_bufferType, totalBufferSize, 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    *p_bufferPtr = (GLsizeiptr)MathUtil::glFuncs->glMapBufferRange(p_bufferType, 0, totalBufferSize,  GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}
#endif

void AbstractRenderer::CopyDataBetweenBuffers(GLuint p_src, GLuint p_dst, GLsizeiptr p_size, GLintptr p_srcOffset, GLintptr p_dstOffset)
{
    if (p_size > 0)
    {
        MathUtil::glFuncs->glBindBuffer(GL_COPY_READ_BUFFER, p_src);
        MathUtil::glFuncs->glBindBuffer(GL_COPY_WRITE_BUFFER, p_dst);
        MathUtil::glFuncs->glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, p_srcOffset, p_dstOffset, p_size);
        MathUtil::glFuncs->glBindBuffer(GL_COPY_READ_BUFFER, 0);
        MathUtil::glFuncs->glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    }
    else
    {
        qDebug() << "WARNING: AbstractRenderer::copyDataBetweenBuffers tried to copy data of 0 size, no copy has occured.";
    }
}

void AbstractRenderer::CreateShadowVAO(GLuint p_VAO, QVector<GLuint> p_VBOs)
{
    Q_UNUSED(p_VAO);
    Q_UNUSED(p_VBOs);
}

void AbstractRenderer::CreateShadowFBO(GLuint p_FBO, QVector<GLuint> p_texture_ids)
{
    Q_UNUSED(p_FBO);
    Q_UNUSED(p_texture_ids);
}

#if !defined(__APPLE__) && !defined(__ANDROID__)
void AbstractRenderer::CreateMatrixSSBO(std::size_t p_ssbo_size, GLuint* p_ssbo_handle, GLintptr& p_ssbo_GPU_ptr, std::size_t* p_aligned_size)
{
    // Create the ssbo handle
    MathUtil::glFuncs->glGenBuffers(1, p_ssbo_handle);

    // Bind the handle to the correct target
    MathUtil::glFuncs->glBindBuffer(GL_SHADER_STORAGE_BUFFER, *p_ssbo_handle);

    // Bind the live portion of the buffer as accessable for the GPU
    MathUtil::glFuncs->glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, *p_ssbo_handle, 0, p_ssbo_size);

    // Flags for persistent buffer mapping to avoid any further glMap* calls
    const GLbitfield mapFlags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    const GLbitfield createFlags = mapFlags | GL_DYNAMIC_STORAGE_BIT;

    // Ensure alignement is at least SSE2 friendly
    GLint bufferAlignment;
    MathUtil::glFuncs->glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &bufferAlignment);
    if (bufferAlignment < 16)
    {
        bufferAlignment = 16; // Minimum alignment of 16 bytes to allow for future SSE2 use.
    }

    // Compute the size needed for one chink of triple buffered SSBO with each chunk aligned as expected
    *p_aligned_size = static_cast<std::size_t>(ceil((double)p_ssbo_size / (double)bufferAlignment) * bufferAlignment); // This is to ensure that the buffer is aligned correctly

    // Allocate enough immutable GPU storage for 3 aligned chunks
    MathUtil::glFuncs->glBufferStorage(GL_SHADER_STORAGE_BUFFER, *p_aligned_size * 3, 0, createFlags);

    // Get a CPU addressable pointer to the GPU buffer so we can directly memcpy the data to the GPU.
    // Because of the flags used this will be a persistent pointer that remains valid even while the buffer is bound for GPU reading
    // as opposed to the normal glMap behaviour that causes pointers to become invalid when the buffer is bound.
    p_ssbo_GPU_ptr = (GLintptr)MathUtil::glFuncs->glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, *p_aligned_size * 3, mapFlags);
}
#endif

void AbstractRenderer::WaitforFrameSyncObject()
{
    GLuint frame_index = m_frame_counter % BUFFER_CHUNK_COUNT;
    if (m_syncObjects[frame_index] != NULL)
    {
        GLenum waitRet = GL_UNSIGNALED;
        while (waitRet != GL_ALREADY_SIGNALED && waitRet != GL_CONDITION_SATISFIED)
        {
            waitRet = MathUtil::glFuncs->glClientWaitSync(m_syncObjects[frame_index], GL_SYNC_FLUSH_COMMANDS_BIT, 1);
        }
        MathUtil::glFuncs->glDeleteSync(m_syncObjects[frame_index]);
        m_syncObjects[frame_index] = NULL;
    }
}

void AbstractRenderer::LockFrameSyncObject()
{
    GLuint frame_index = m_frame_counter % BUFFER_CHUNK_COUNT;
    m_syncObjects[frame_index] = MathUtil::glFuncs->glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void AbstractRenderer::StartFrame()
{
    GLuint frame_index = m_frame_counter % BUFFER_CHUNK_COUNT;
#ifndef __ANDROID__
    MathUtil::glFuncs->glBeginQuery(GL_TIME_ELAPSED, m_GPUTimeQuery[frame_index]);
#endif
}

void AbstractRenderer::EndFrame()
{
#ifndef __ANDROID__
    MathUtil::glFuncs->glEndQuery(GL_TIME_ELAPSED);
#endif

    // Read frames back BUFFER_CHUNK_COUNT frames late to ensure the timer
    // queries are complete and wont stall the thread.
    if (m_frame_counter >= (BUFFER_CHUNK_COUNT-1))
    {
        GLuint frame_index = m_frame_counter - (BUFFER_CHUNK_COUNT-1);
        GLuint query_index = frame_index % BUFFER_CHUNK_COUNT;
        GLuint results_index = frame_index % m_GPUTimeQueryResults.size();
#ifdef __ANDROID__
        GLuint available = GL_TRUE; // 0
        //MathUtil::glFuncs->glGetQueryObjectuiv(m_GPUTimeQuery[query_index], GL_QUERY_RESULT_AVAILABLE, &available);
#else
        GLint available = 0;
        MathUtil::glFuncs->glGetQueryObjectiv(m_GPUTimeQuery[query_index], GL_QUERY_RESULT_AVAILABLE, &available);
#endif

        if (available == GL_TRUE)
        {
#ifdef __ANDROID__
            //GLuint query_result;
            //MathUtil::glFuncs->glGetQueryObjectuiv(m_GPUTimeQuery[query_index], GL_QUERY_RESULT, &query_result);
            m_GPUTimeQueryResults[results_index] = (uint64_t) 0; //query_result
#else
            MathUtil::glFuncs->glGetQueryObjectui64v(m_GPUTimeQuery[query_index], GL_QUERY_RESULT_NO_WAIT, &m_GPUTimeQueryResults[results_index]);
#endif
        }
    }
    GLuint cpu_results_index = m_frame_counter % m_GPUTimeQueryResults.size();
    m_CPUTimeQueryResults[cpu_results_index] = m_frame_time_timer.nsecsElapsed();
    m_frame_time_timer.restart();
    m_frame_counter++;
}

int64_t AbstractRenderer::GetFrameCounter()
{
    return m_frame_counter;
}

size_t AbstractRenderer::GetNumTextures() const
{
    return m_texture_handle_to_GL_ID.size() - m_num_deleted_textures;
}

QVector<uint64_t> & AbstractRenderer::GetGPUTimeQueryResults()
{
    return m_GPUTimeQueryResults;
}

QVector<uint64_t> & AbstractRenderer::GetCPUTimeQueryResults()
{
    return m_CPUTimeQueryResults;
}

void AbstractRenderer::SetCameras(QVector<VirtualCamera> *p_cameras)
{
    for (size_t scope_enum = 0; scope_enum < static_cast<size_t>(RENDERER::RENDER_SCOPE::SCOPE_COUNT); ++scope_enum)
    {
        size_t const cam_count = p_cameras->size();
        m_scoped_cameras_cache[m_current_submission_index][scope_enum].clear();
        m_scoped_cameras_cache[m_current_submission_index][scope_enum].reserve(cam_count);

        for (size_t cam_index = 0; cam_index < cam_count; ++cam_index)
        {
            if ((*p_cameras)[cam_index].GetScopeMask(static_cast<RENDERER::RENDER_SCOPE>(scope_enum)) == true)
            {
                m_scoped_cameras_cache[m_current_submission_index][scope_enum].push_back((*p_cameras)[cam_index]);
            }
        }
    }
}

void AbstractRenderer::SetDefaultFontGlyphAtlas(std::shared_ptr<TextureHandle> p_handle)
{
	m_default_font_glyph_atlas = p_handle;
}

TextureHandle* AbstractRenderer::GetDefaultFontGlyphAtlas()
{
	return m_default_font_glyph_atlas.get();
}

/*QVector<VirtualCamera> const & AbstractRenderer::GetCameras() const
{
    return  m_cameras_cache[m_rendering_index];
}

uint32_t AbstractRenderer::GetCamerasPerScope(const RENDERER::RENDER_SCOPE p_scope) const
{
    return m_cameras_per_scope_cache[m_rendering_index][static_cast<size_t>(p_scope)];
}*/

std::shared_ptr<ProgramHandle> AbstractRenderer::CompileAndLinkShaderProgram(QByteArray *p_vertex_shader, QString p_vertex_shader_path,
                                                                                QByteArray *p_fragment_shader, QString p_fragment_shader_path)
{
//    qDebug() << "AbstractRenderer::CompileAndLinkShaderProgram()";
    GLuint program_id;
    std::shared_ptr<ProgramHandle> abstract_shader_program = CreateProgramHandle(&program_id);

    GLuint vertex_shader_id = MathUtil::glFuncs->glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader_id = MathUtil::glFuncs->glCreateShader(GL_FRAGMENT_SHADER);
    GLint vertex_compile_result = GL_FALSE;
    GLint fragment_compile_result = GL_FALSE;
    GLint program_link_result = GL_FALSE;

    bool shader_failed = false;
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
            MathUtil::ErrorLog(QString("Compilation of vertex shader \"") + p_vertex_shader_path + QString("\" failed:") + QString("\n") + vertex_shader_log.data());
        }
    }
    else
    {
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
            MathUtil::ErrorLog(QString("Compilation of vertex shader \"") + default_object_vertex_shader_path + QString("\" failed:") + QString("\n") + vertex_shader_log.data());
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
            MathUtil::ErrorLog(QString("Compilation of fragment shader \"") + p_fragment_shader_path + QString("\" failed:") + QString("\n") + fragment_shader_lod.data());
        }
    }
    else
    {
        QString default_object_fragment_shader_path(MathUtil::GetApplicationPath() + "assets/shaders/trans_frag.txt");
        QFile default_object_fragment_shader_file(default_object_fragment_shader_path);
        default_object_fragment_shader_file.open(QIODevice::ReadOnly | QIODevice::Text);
        QByteArray default_object_fragment_shader_bytes = default_object_fragment_shader_file.readAll();
        default_object_fragment_shader_file.close();

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
            MathUtil::ErrorLog(QString("Error: Compilation of fragment shader \"") + default_object_fragment_shader_path + QString("\" failed:") + QString("\n") + fragment_shader_lod.data());
        }
	}

	if (!shader_failed)
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
            MathUtil::ErrorLog(QString("Linking of shaders \"") + program_id + QString("\" & \"") + p_fragment_shader_path + QString("\" failed:") + QString("\n") + program_log.data());
        }
    }

	// If we failed just return the default object shader
	if (shader_failed)
	{
		abstract_shader_program = m_default_object_shader;
	}
	else
	{
        int log_length;
        MathUtil::glFuncs->glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
        QVector<char> program_log( (log_length > 1) ? log_length : 1 );
        MathUtil::glFuncs->glGetProgramInfoLog(program_id, log_length, NULL, &program_log[0]);;
//        MathUtil::ErrorLog(QString("Linking of shaders \"") + p_vertex_shader_path + QString("\" & \"") + p_fragment_shader_path + QString("\" successful:") + QString("\n") + program_log.data());

        MathUtil::glFuncs->glUseProgram(program_id);
        CacheUniformLocations(program_id, &m_uniform_locs);

        MathUtil::glFuncs->glDeleteShader(vertex_shader_id);
        MathUtil::glFuncs->glDeleteShader(fragment_shader_id);
	}

	return abstract_shader_program;
}

std::shared_ptr<TextureHandle> AbstractRenderer::CreateTextureHandle(TextureHandle::TEXTURE_TYPE p_texture_type,
    TextureHandle::COLOR_SPACE p_color_space,
    TextureHandle::ALPHA_TYPE p_alpha_type,
    uint32_t p_width,
    uint32_t p_height,
    GLuint p_GL_texture_ID)
{    
    QPair<TextureHandle*, GLuint> texture_pair;
	texture_pair.first = new TextureHandle(m_texture_UUID, p_texture_type, p_color_space, p_alpha_type);
    texture_pair.second = p_GL_texture_ID;

    m_texture_UUID++;

    QPair<TextureHandle*, uint32_t> width_pair;
    width_pair.first = texture_pair.first;
    width_pair.second = p_width;

    QPair<TextureHandle*, uint32_t> height_pair;
    height_pair.first = texture_pair.first;
    height_pair.second = p_height;

    if (m_texture_handle_to_GL_ID.capacity() == m_texture_handle_to_GL_ID.size())
    {        
        m_mesh_handle_to_buffers.reserve(m_mesh_handle_to_buffers.size() + 128);
        m_texture_handle_to_width.reserve(m_texture_handle_to_width.size() + 128);
        m_texture_handle_to_height.reserve(m_texture_handle_to_height.size() + 128);        
    }

    m_texture_handle_to_GL_ID.push_back(texture_pair);
    texture_pair.first->m_last_known_index = (m_texture_handle_to_GL_ID.size() - 1);
    m_texture_handle_to_width.push_back(width_pair);
    m_texture_handle_to_height.push_back(height_pair);

    TextureHandle* tex_address = m_texture_handle_to_GL_ID.back().first;

	std::shared_ptr<TextureHandle> texture_handle = std::shared_ptr<TextureHandle>(tex_address,
		[this](TextureHandle* ptr)
	{
        //59.3 - TODO fix crash here on exit when uncommented
        this->RemoveTextureHandleFromMap(ptr);
	});

#ifdef QT_DEBUG
    auto texture_UUID = texture_pair.first->GetUUID();
	qDebug() << QString("CreateTextureHandle:: ")
        + "UUID: " + QString::number(texture_UUID.m_UUID) + " "
        + "GPUHandle: " + QString::number((uint32_t)texture_pair.second);
#endif
	return texture_handle;
}

void AbstractRenderer::RemoveTextureHandleFromMap(TextureHandle* p_handle)
{    
    if (p_handle == nullptr)
    {
        qDebug() << QString("ERROR!: RemoveTextureHandleFromMap:: p_handle was nullptr");
        return;
    }

    // We keep handles in the vector but clear their contents and add the address to a free list so that it can be reused by a later creation call;
    const size_t texture_count = m_texture_handle_to_GL_ID.size();
    for (size_t itr = 0; itr < texture_count; ++itr)
    {
        if (m_texture_handle_to_GL_ID[itr].first == p_handle && m_texture_handle_to_GL_ID[itr].first->m_UUID.m_UUID == p_handle->m_UUID.m_UUID)
        {
            QPair<TextureHandle*, GLuint>& texture_pair = m_texture_handle_to_GL_ID[itr];
            texture_pair.first->m_last_known_index = itr;
            m_texture_deletion_guard.lock();

#ifdef QT_DEBUG
    auto texture_UUID = texture_pair.first->GetUUID();
    qDebug() << QString("RemoveTextureHandleFromMap:: (queue for deletion)")
        + "UUID: " + QString::number(texture_UUID.m_UUID) + " "
        + "GPUHandle: " + QString::number((uint32_t)texture_pair.second);
#endif
            m_textures_pending_deletion.push_back(texture_pair.first);
            m_texture_deletion_guard.unlock();
            return;
        }
    }
}

void AbstractRenderer::FreeTextureHandles()
{
    m_texture_deletion_guard.lock();
    const size_t texture_count = m_textures_pending_deletion.size();
    for (size_t i = 0; i < texture_count; ++i)
    {
        size_t const last_index = m_textures_pending_deletion[i]->m_last_known_index;
        QPair<TextureHandle*, GLuint>& texture_pair = m_texture_handle_to_GL_ID[last_index];

#ifdef QT_DEBUG
    auto texture_UUID = texture_pair.first->GetUUID();
    qDebug() << QString("FreeTextureHandles::")
        + "UUID: " + QString::number(texture_UUID.m_UUID) + " "
        + "GPUHandle: " + QString::number((uint32_t)texture_pair.second);
#endif

        // Null m_texture_handle_to_GL_ID ptr so that nothing new can grab the about to be deleted data
        texture_pair.first = nullptr;

        // Delete GL resource
        MathUtil::glFuncs->glDeleteTextures(1, &(texture_pair.second));
        texture_pair.second = 0;

        // Free memory used by MeshHandle #TODO (Leaking this for now to see if deleting it was causing a crash
        //delete m_textures_pending_deletion[i];
    }
    m_textures_pending_deletion.clear();
    m_num_deleted_textures += texture_count;
    m_texture_deletion_guard.unlock();
}

QVector<std::shared_ptr<BufferHandle>>* AbstractRenderer::GetBufferHandlesForMeshHandle(MeshHandle * p_mesh_handle)
{
    const size_t mesh_count = m_mesh_handle_to_buffers.size();
    for (size_t itr = 0; itr < mesh_count; ++itr)
	{
        if (m_mesh_handle_to_buffers[itr].first == p_mesh_handle)
		{
            return &(m_mesh_handle_to_buffers[itr].second);
		}
	}
	return nullptr;
}

void AbstractRenderer::CreateMeshHandleForGeomVBOData(GeomVBOData* p_VBO_data)
{
	GLuint VAO_ID = 0;
    MathUtil::glFuncs->glGenVertexArrays(1, &VAO_ID);

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

    p_VBO_data->m_mesh_handle = CreateMeshHandle(layout, VAO_ID);
    BindMeshHandle(p_VBO_data->m_mesh_handle.get());

    auto buffer_handles = GetBufferHandlesForMeshHandle(p_VBO_data->m_mesh_handle.get());

    BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::INDICES].get());
    MathUtil::glFuncs->glBufferData(GL_ELEMENT_ARRAY_BUFFER, p_VBO_data->m_indices.size() * sizeof(uint32_t), p_VBO_data->m_indices.data(), GL_STATIC_DRAW);

    BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::POSITION].get());
    MathUtil::glFuncs->glBufferData(GL_ARRAY_BUFFER, p_VBO_data->m_positions.size() * float_size, p_VBO_data->m_positions.data(), GL_STATIC_DRAW);

    BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::NORMAL].get());
    MathUtil::glFuncs->glBufferData(GL_ARRAY_BUFFER, p_VBO_data->m_normals.size() * float_size, p_VBO_data->m_normals.data(), GL_STATIC_DRAW);

    BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::TEXCOORD0].get());
    MathUtil::glFuncs->glBufferData(GL_ARRAY_BUFFER, p_VBO_data->m_tex_coords.size() * float_size, p_VBO_data->m_tex_coords.data(), GL_STATIC_DRAW);

    BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::COLOR].get());
    MathUtil::glFuncs->glBufferData(GL_ARRAY_BUFFER, p_VBO_data->m_colors.size() * float_size, p_VBO_data->m_colors.data(), GL_STATIC_DRAW);

	if (p_VBO_data->use_skelanim)
	{
        BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::SKELANIMINDICES].get());
        MathUtil::glFuncs->glBufferData(GL_ARRAY_BUFFER, p_VBO_data->m_skel_anim_indices.size() * sizeof(uint8_t), p_VBO_data->m_skel_anim_indices.data(), GL_STATIC_DRAW);
		
        BindBufferHandle((*buffer_handles)[(uint32_t)VAO_ATTRIB::SKELANIMWEIGHTS].get());
        MathUtil::glFuncs->glBufferData(GL_ARRAY_BUFFER, p_VBO_data->m_skel_anim_weights.size() * float_size, p_VBO_data->m_skel_anim_weights.data(), GL_STATIC_DRAW);
	}
}


std::shared_ptr<MeshHandle> AbstractRenderer::CreateMeshHandle(VertexAttributeLayout p_layout)
{
    uint32_t VAO_id = 0;
    MathUtil::glFuncs->glGenVertexArrays(1, &VAO_id);

    return CreateMeshHandle(p_layout,
                            VAO_id);
}

std::shared_ptr<MeshHandle> AbstractRenderer::CreateMeshHandle(VertexAttributeLayout p_layout, GLuint p_GL_VAO_ID)
{    
    QPair<MeshHandle*, GLuint> mesh_pair;
    mesh_pair.first = new MeshHandle(m_mesh_UUID,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::POSITION].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::NORMAL].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD0].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::TEXCOORD1].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::COLOR].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::BLENDV0].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::BLENDV1].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::BLENDV2].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::BLENDN0].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::BLENDN1].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::BLENDN2].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMINDICES].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::SKELANIMWEIGHTS].in_use,
        p_layout.attributes[(uint32_t)VAO_ATTRIB::INDICES].in_use);
    mesh_pair.second = p_GL_VAO_ID;

    m_mesh_UUID++;

    if (m_mesh_handle_to_GL_ID.capacity() == m_mesh_handle_to_GL_ID.size())
    {        
        m_mesh_handle_to_GL_ID.reserve(m_mesh_handle_to_GL_ID.size() + 128);
    }

    m_mesh_handle_to_GL_ID.push_back(mesh_pair);
    mesh_pair.first->m_last_known_index = (m_mesh_handle_to_GL_ID.size() - 1);

    MeshHandle* mesh_address = m_mesh_handle_to_GL_ID.back().first;

    std::shared_ptr<MeshHandle> mesh_handle = std::shared_ptr<MeshHandle>(mesh_address,
        [this](MeshHandle* ptr)
    {
        //59.3 - TODO fix crash here on exit when uncommented
        this->RemoveMeshHandleFromMap(ptr);
    });

    BindMeshHandle(mesh_handle.get());

    uint32_t const num_attribs = (uint32_t)VAO_ATTRIB::NUM_ATTRIBS;

    QVector<std::shared_ptr<BufferHandle>> buffer_handles;
    buffer_handles.resize(num_attribs);

    for (uint32_t attrib_index = 0; attrib_index < num_attribs; ++ attrib_index)
    {
        if (p_layout.attributes[attrib_index].in_use == true)
        {
            if (attrib_index == (uint32_t)VAO_ATTRIB::INDICES)
            {
                buffer_handles[attrib_index] = (CreateBufferHandle(BufferHandle::BUFFER_TYPE::ELEMENT_ARRAY_BUFFER, BufferHandle::BUFFER_USAGE::STATIC_DRAW));
                BindBufferHandle(buffer_handles[attrib_index].get());
            }
            else
            {
                if (buffer_handles[(uint32_t)p_layout.attributes[(uint32_t)attrib_index].buffer_id] == nullptr)
                {

                    buffer_handles[(uint32_t)p_layout.attributes[(uint32_t)attrib_index].buffer_id] = (CreateBufferHandle(BufferHandle::BUFFER_TYPE::ARRAY_BUFFER, BufferHandle::BUFFER_USAGE::STATIC_DRAW));
                }
                else
                {
                    buffer_handles[attrib_index] = buffer_handles[(uint32_t)p_layout.attributes[(uint32_t)attrib_index].buffer_id];
                }

                BindBufferHandle(buffer_handles[attrib_index].get());
                MathUtil::glFuncs->glEnableVertexAttribArray((GLuint)attrib_index);
                if (p_layout.attributes[attrib_index].is_float_attrib == true)
                {
                    MathUtil::glFuncs->glVertexAttribPointer((GLuint)attrib_index,
                                                             (GLint)p_layout.attributes[attrib_index].element_count,
                                                             (GLenum)p_layout.attributes[attrib_index].element_type,
                                                             (GLboolean)p_layout.attributes[attrib_index].is_normalized,
                                                             (GLsizei)p_layout.attributes[attrib_index].stride_in_bytes,
                                                             (void*)p_layout.attributes[attrib_index].offset_in_bytes);
                }
                else
                {
                    MathUtil::glFuncs->glVertexAttribIPointer((GLuint)attrib_index,
                                                             (GLint)p_layout.attributes[attrib_index].element_count,
                                                             (GLenum)p_layout.attributes[attrib_index].element_type,
                                                             (GLsizei)p_layout.attributes[attrib_index].stride_in_bytes,
                                                             (void*)p_layout.attributes[attrib_index].offset_in_bytes);
                }
            }
        }
    }

    if (m_mesh_handle_to_buffers.capacity() == m_mesh_handle_to_buffers.size())
    {        
        m_mesh_handle_to_buffers.reserve(m_mesh_handle_to_buffers.size() + 128);        
    }
    m_mesh_handle_to_buffers.push_back(QPair<MeshHandle*, QVector<std::shared_ptr<BufferHandle>>>(mesh_address, buffer_handles));



#ifdef QT_DEBUG
    auto mesh_UUID = mesh_pair.first->m_UUID;
    qDebug() << QString("CreateMeshHandle:: ")
        + "UUID: " + QString::number(mesh_UUID.m_UUID) + ' '
        + "GPUHandle: " + QString::number((uint32_t)mesh_pair.second);
#endif

    return mesh_handle;
}

void AbstractRenderer::FreeMeshHandles()
{
    m_mesh_deletion_guard.lock();
    size_t const mesh_count = m_meshes_pending_deletion.size();
    for (size_t i = 0; i < mesh_count; ++i)
    {
        size_t const last_index = m_meshes_pending_deletion[i]->m_last_known_index;
        QPair<MeshHandle*, GLuint>& mesh_pair = m_mesh_handle_to_GL_ID[last_index];

#ifdef QT_DEBUG
    auto mesh_UUID = mesh_pair.first->m_UUID;
    qDebug() << QString("FreeMeshHandles:: ")
        + "UUID: " + QString::number(mesh_UUID.m_UUID) + ' '
        + "GPUHandle: " + QString::number((uint32_t)mesh_pair.second);
#endif

        // Null m_mesh_handle_to_GL_ID ptr
        mesh_pair.first = nullptr;

        // Delete GL resource
        MathUtil::glFuncs->glDeleteVertexArrays(1, &mesh_pair.second);
        mesh_pair.second = 0;

        // Deref VBOs assosiated with VAO
        m_mesh_handle_to_buffers[last_index].first = nullptr;
        m_mesh_handle_to_buffers[last_index].second.clear();

        // Free memory used by MeshHandle
        delete m_meshes_pending_deletion[i];
    }
    m_meshes_pending_deletion.clear();
    m_mesh_deletion_guard.unlock();
}

void AbstractRenderer::RemoveMeshHandleFromMap(MeshHandle* p_handle)
{    
    if (p_handle == nullptr)
    {
        qDebug() << QString("ERROR!: RemoveMeshHandleFromMap:: p_handle was nullptr");
        return;
    }

    // We keep handles in the vector but clear their contents and add the address to a free list so that it can be reused by a later creation call;
    const size_t mesh_count = m_mesh_handle_to_GL_ID.size();
    for (size_t itr = 0; itr < mesh_count; ++itr)
    {
        if (m_mesh_handle_to_GL_ID[itr].first == p_handle && m_mesh_handle_to_GL_ID[itr].first->m_UUID.m_UUID == p_handle->m_UUID.m_UUID)
        {
            QPair<MeshHandle*, GLuint>& mesh_pair = m_mesh_handle_to_GL_ID[itr];
            mesh_pair.first->m_last_known_index = itr;

#ifdef QT_DEBUG
    auto mesh_UUID = mesh_pair.first->m_UUID;
    qDebug() << QString("RemoveMeshHandleFromMap:: (queue for deletion) ")
        + "UUID: " + QString::number(mesh_UUID.m_UUID) + ' '
        + "GPUHandle: " + QString::number((uint32_t)mesh_pair.second);
#endif

            m_mesh_deletion_guard.lock();
            m_meshes_pending_deletion.push_back(mesh_pair.first);
            m_mesh_deletion_guard.unlock();
            return;
        }
    }
}

void AbstractRenderer::FreeProgramHandles()
{
    m_program_deletion_guard.lock();
    const size_t program_count = m_programs_pending_deletion.size();
    for (size_t i = 0; i < program_count; ++i)
    {
        size_t const last_index = m_programs_pending_deletion[i]->m_last_known_index;
        QPair<ProgramHandle*, GLuint>& program_pair = m_program_handle_to_GL_ID[last_index];

        // Null m_mesh_handle_to_GL_ID ptr
        program_pair.first = nullptr;

        // Delete GL resource
        // This line crashes AMD cards, have not figured out why yet.
        // This means we leak programs but they are not large and are very infrequently
        // created so the inpact to the user is minimal in comparison to the crashing.
        //MathUtil::glFuncs->glDeleteProgram(program_pair.second);
        program_pair.second = 0;

        // Free memory used by MeshHandle
        delete m_programs_pending_deletion[i];
    }
    m_programs_pending_deletion.clear();
    m_program_deletion_guard.unlock();
}

void AbstractRenderer::RemoveProgramHandleFromMap(ProgramHandle* p_handle)
{    
    // We keep handles in the vector but clear their contents and add the address to a free list so that it can be reused by a later creation call;
    const size_t program_count = m_program_handle_to_GL_ID.size();
    for (size_t itr = 0; itr < program_count; ++itr)
    {
        if (m_program_handle_to_GL_ID[itr].first == p_handle && m_program_handle_to_GL_ID[itr].first->m_UUID.m_UUID == p_handle->m_UUID.m_UUID)
        {
            QPair<ProgramHandle*, GLuint>& program_pair = m_program_handle_to_GL_ID[itr];
            program_pair.first->m_last_known_index = itr;

            m_program_deletion_guard.lock();
            m_programs_pending_deletion.push_back(program_pair.first);
            m_program_deletion_guard.unlock();
            return;
        }
    }
}

int AbstractRenderer::GetTextureWidth(TextureHandle* p_handle)
{
	int width = 0;

    TextureHandle* tex_handle = p_handle;
    const size_t tex_count = m_texture_handle_to_width.size();
    for (size_t itr = 0; itr < tex_count; ++itr)
	{
        if (tex_handle == m_texture_handle_to_width[itr].first)
		{
            width = m_texture_handle_to_width[itr].second;
		}
	}
	return width;
}

int AbstractRenderer::GetTextureHeight(TextureHandle* p_handle)
{
	int height = 0;
    TextureHandle* tex_handle = p_handle;
    const size_t tex_count = m_texture_handle_to_height.size();
    for (size_t itr = 0; itr < tex_count; ++itr)
	{
        if (tex_handle == m_texture_handle_to_height[itr].first)
		{
            height = m_texture_handle_to_height[itr].second;
		}
	}
	return height;
}

void AbstractRenderer::externalFormatAndTypeFromSize(GLenum* p_pixel_format, GLenum* p_pixel_type, uint const p_pixel_size)
{
	switch (p_pixel_size)
	{
	case 3: // RGB8
        *p_pixel_type = GL_UNSIGNED_BYTE;
        *p_pixel_format = GL_RGB;
		break;
	case 4: // RBGA8
        *p_pixel_type = GL_UNSIGNED_BYTE;
        *p_pixel_format = GL_RGBA;
		break;
	case 6: // RGB16F
        *p_pixel_type = GL_HALF_FLOAT;
        *p_pixel_format = GL_RGB;
		break;
	case 8: // RGBA16F
        *p_pixel_type = GL_HALF_FLOAT;
        *p_pixel_format = GL_RGBA;
		break;
	case 12: // RGB32F
        *p_pixel_type = GL_FLOAT;
        *p_pixel_format = GL_RGB;
		break;
	case 16: // RGBA32F
        *p_pixel_type = GL_FLOAT;
        *p_pixel_format = GL_RGBA;
		break;
	default:
		// ERROR some craziness is afoot.
		break;
	}
}

void AbstractRenderer::InternalFormatFromSize(GLenum* p_pixel_format, uint const p_pixel_size)
{
    switch (p_pixel_size)
    {
    case 3: // RGB8
        *p_pixel_format = GL_RGB8;
        break;
    case 4: // RBGA8
        *p_pixel_format = GL_RGBA8;
        break;
    case 6: // RGB16F
        *p_pixel_format = GL_RGB16F;
        break;
    case 8: // RGBA16F
        *p_pixel_format = GL_RGBA16F;
        break;
    case 12: // RGB32F
        *p_pixel_format = GL_RGB32F;
        break;
    case 16: // RGBA32F
        *p_pixel_format = GL_RGBA32F;
        break;
    default:
        // ERROR some craziness is afoot.
        break;
    }
}

std::shared_ptr<TextureHandle> AbstractRenderer::CreateCubemapTextureHandleFromTextureHandles(QVector<QPointer<AssetImageData>>& p_skybox_image_data, QVector<TextureHandle*>& p_skybox_image_handles, const bool tex_mipmap, const bool tex_linear, const bool tex_clamp, const TextureHandle::ALPHA_TYPE tex_alpha, const TextureHandle::COLOR_SPACE )
{
    int w = 0;
    int h = 0;
    GLint max_dim = 0;

    const unsigned int imageCount = p_skybox_image_data.size();
    uchar max_pixel_size_in_bytes = 0;
    QVector<uchar> pixel_sizes_in_bytes;
    pixel_sizes_in_bytes.resize(6);

    QVector<GLenum> external_pixel_types;
    external_pixel_types.resize(imageCount);
    QVector<GLenum> external_pixel_formats;
    external_pixel_formats.resize(imageCount);

#ifdef __ANDROID__
    GLenum internal_format;
#endif
    GLenum current_internal_format;
    GLenum current_external_format;
    GLenum current_external_pixel_size;

    TextureHandle::COLOR_SPACE color_space = TextureHandle::LINEAR;

    //get max size needed
    for (unsigned int i = 0; i < imageCount; i++)
    {
        if (p_skybox_image_data[i])
        {
            auto data = p_skybox_image_data[i];
            pixel_sizes_in_bytes[i] = data->GetPixelSize();
            w = data->GetWidth();
            h = data->GetHeight();

            max_dim = qMax(max_dim, w);
            max_dim = qMax(max_dim, h);

            InternalFormatFromSize(&current_internal_format, pixel_sizes_in_bytes[i]);
            externalFormatAndTypeFromSize(&current_external_format, &current_external_pixel_size, pixel_sizes_in_bytes[i]);
            external_pixel_types[i] = current_external_pixel_size;
            external_pixel_formats[i] = current_external_format;

            // If this image has a larger pixel size than the current max, update the max and internal format to be able to hold it.
            if (max_pixel_size_in_bytes < pixel_sizes_in_bytes[i])
            {
                max_pixel_size_in_bytes = pixel_sizes_in_bytes[i];
#ifdef __ANDROID__
                internal_format = current_internal_format;
#endif
            }
        }
    }


    GLuint texture_id = 0;
    MathUtil::glFuncs->glGenTextures(1, &texture_id);
    MathUtil::glFuncs->glActiveTexture(GL_TEXTURE15);
    m_active_texture_slot = 15;
    MathUtil::glFuncs->glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

    //mipmap levels
    auto const max_mip_level = log(max_dim)/log(2);
    if (tex_mipmap)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, max_mip_level);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, max_mip_level);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, 0);
    }

    //minification filter
    if (tex_mipmap)
    {
        if (tex_linear)
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        else
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        }
    }
    else
    {
        if (tex_linear)
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        else
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
    }

    //magnification filter
    if (tex_linear)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    //clamping
    if (tex_clamp)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
    }

    //aniso
    MathUtil::glFuncs->glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_max_anisotropy);

#ifndef __ANDROID__
    // Allocate Texture
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_SRGB8, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_SRGB8, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_SRGB8, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_SRGB8, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_SRGB8, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_SRGB8, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
#else
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internal_format, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internal_format, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internal_format, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internal_format, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internal_format, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internal_format, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

#endif


    // Create src fbo
    GLuint src_fbo = 0;
    MathUtil::glFuncs->glGenFramebuffers(1, &src_fbo);
    // Bind src fbo to READ FRAMEBUFFER
    MathUtil::glFuncs->glBindFramebuffer(GL_READ_FRAMEBUFFER, src_fbo);
    // Create dst fbo
    GLuint dst_fbo = 0;
    MathUtil::glFuncs->glGenFramebuffers(1, &dst_fbo);
    // Bind src fbo to READ FRAMEBUFFER
    MathUtil::glFuncs->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_fbo);
    // For each face
    for (size_t face_index = 0; face_index < imageCount; ++face_index)
    {
        // Attach src face to src fbo
        GLuint face_id = 0;
        TextureHandle * ref_handle = p_skybox_image_handles[face_index];
        auto const tex_count = m_texture_handle_to_GL_ID.size();
        if (m_texture_handle_to_GL_ID[ref_handle->m_last_known_index].first->m_UUID.m_UUID == ref_handle->m_UUID.m_UUID)
        {
            face_id =  m_texture_handle_to_GL_ID[ref_handle->m_last_known_index].second;
        }
        else
        {
            for (size_t itr = 0; itr < tex_count; ++itr)
            {
                if (m_texture_handle_to_GL_ID[itr].first->m_UUID.m_UUID == ref_handle->m_UUID.m_UUID)
                {
                    ref_handle->m_last_known_index = itr;
                    face_id =  m_texture_handle_to_GL_ID[itr].second;
                }
            }
        }
        MathUtil::glFuncs->glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, face_id, 0);
        // Attach dst face to dst fbo
        MathUtil::glFuncs->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face_index, texture_id, 0);
        // Blit src to dst which will auto-scale the src image if needed using LINEAR filtering
        MathUtil::glFuncs->glBlitFramebuffer(0, 0, p_skybox_image_data[face_index].data()->GetWidth(), p_skybox_image_data[face_index].data()->GetWidth(),
                                             0, 0, max_dim, max_dim, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    // Bind 0 fbo to READ FRAMEBUFFER
    MathUtil::glFuncs->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    // Bind 0 fbo to DRAW FRAMEBUFFER
    MathUtil::glFuncs->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    // Delete src fbo
    MathUtil::glFuncs->glDeleteFramebuffers(1, &src_fbo);
    // Delete dst fbo
    MathUtil::glFuncs->glDeleteFramebuffers(1, &dst_fbo);

    if (tex_mipmap)
    {
        MathUtil::glFuncs->glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }

    return CreateTextureHandle(TextureHandle::TEXTURE_TYPE::TEXTURE_CUBEMAP, color_space, tex_alpha, max_dim, max_dim, texture_id);
}

std::shared_ptr<TextureHandle> AbstractRenderer::CreateCubemapTextureHandleFromAssetImages(QVector<QPointer<AssetImageData>>& p_skybox_image_data, const bool tex_mipmap, const bool tex_linear, const bool tex_clamp, const TextureHandle::ALPHA_TYPE tex_alpha, const TextureHandle::COLOR_SPACE )
{
	int w = 0;
	int h = 0;
	GLint max_dim = 0;

	const int imageCount = p_skybox_image_data.size();
	uchar max_pixel_size_in_bytes = 0;
	QVector<uchar> pixel_sizes_in_bytes;
	pixel_sizes_in_bytes.resize(6);

    QVector<GLenum> external_pixel_types;
	external_pixel_types.resize(imageCount);
    QVector<GLenum> external_pixel_formats;
	external_pixel_formats.resize(imageCount);

#ifdef __ANDROID__
    GLenum internal_format;
#endif
    GLenum current_internal_format;
    GLenum current_external_format;
    GLenum current_external_pixel_size;

    TextureHandle::COLOR_SPACE color_space = TextureHandle::LINEAR;

	//get max size needed
	for (int i = 0; i < imageCount; i++)
	{
		if (p_skybox_image_data[i])
		{
			auto data = p_skybox_image_data[i];
			pixel_sizes_in_bytes[i] = data->GetPixelSize();
			w = data->GetWidth();
			h = data->GetHeight();

			max_dim = qMax(max_dim, w);
			max_dim = qMax(max_dim, h);

            InternalFormatFromSize(&current_internal_format, pixel_sizes_in_bytes[i]);
            externalFormatAndTypeFromSize(&current_external_format, &current_external_pixel_size, pixel_sizes_in_bytes[i]);
            external_pixel_types[i] = current_external_pixel_size;
            external_pixel_formats[i] = current_external_format;

			// If this image has a larger pixel size than the current max, update the max and internal format to be able to hold it.
			if (max_pixel_size_in_bytes < pixel_sizes_in_bytes[i])
			{
				max_pixel_size_in_bytes = pixel_sizes_in_bytes[i];
#ifdef __ANDROID__
                internal_format = current_internal_format;
#endif
			}
		}
    }


    GLuint texture_id = 0;
    MathUtil::glFuncs->glGenTextures(1, &texture_id);
    MathUtil::glFuncs->glGenTextures(1, &texture_id);
    MathUtil::glFuncs->glActiveTexture(GL_TEXTURE15);
    m_active_texture_slot = 15;
    MathUtil::glFuncs->glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

    //mipmap levels
    auto const max_mip_level = log(max_dim)/log(2);
    if (tex_mipmap)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, max_mip_level);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, max_mip_level);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, 0);
    }

    //minification filter
    if (tex_mipmap)
    {
        if (tex_linear)
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        else
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        }
    }
    else
    {
        if (tex_linear)
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        else
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
    }

    //magnification filter
    if (tex_linear)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    //clamping
    if (tex_clamp)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
    }

    //aniso
    MathUtil::glFuncs->glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_max_anisotropy);

    // Allocate Texture
#ifndef __ANDROID__
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_SRGB8, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_SRGB8, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_SRGB8, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_SRGB8, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_SRGB8, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_SRGB8, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
#else
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internal_format, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internal_format, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internal_format, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internal_format, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internal_format, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internal_format, max_dim, max_dim, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

#endif

    if (tex_mipmap)
    {
        MathUtil::glFuncs->glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }

    // Populate Texture
	for (int i = 0; i < imageCount; i++)
	{
		if (p_skybox_image_data[i].isNull())
		{
			QImage null_image(max_dim, max_dim, QImage::Format_RGB32);
			null_image.fill(QColor(64, 64, 64));
			for (int column = 0; column < max_dim; ++column)
			{
				for (int row = 0; row < max_dim; ++row)
				{
					if ((column / 16 + row / 16) % 2 == 0)
					{
						null_image.setPixel(column, row, 0xff999999);
					}
				}
			}
            MathUtil::glFuncs->glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, max_dim, max_dim, GL_RGBA, GL_UNSIGNED_BYTE, (const void*)null_image.constBits());;
        }
		else
		{
			auto data = p_skybox_image_data[i];
				
			if (data.isNull())
			{
				continue;
			}

            QByteArray pData = data->GetLeftFrameData(0);
            if (pData.isEmpty()) {
				continue;
			}

			int width = data->GetWidth();
			int height = data->GetHeight();
			bool madeData = false;
			int pixelSize = data->GetPixelSize();

            if (width != max_dim || height != max_dim) {
				madeData = true;				
                pData = MathUtil::ScalePixelData(pData, QSize(width, height), pixelSize, QSize(max_dim, max_dim));
			}

            if (!pData.isEmpty())
			{
				GLenum external_format = GL_RGB;
				GLenum external_pixel_size = GL_UNSIGNED_BYTE;
				bool format_invalid = false;
				switch (data->format)
				{
				case QImage::Format_RGB888:
					external_format = GL_RGB;
					external_pixel_size = GL_UNSIGNED_BYTE;
					break;
				case QImage::Format_RGBA8888:
				case QImage::Format_RGBX8888:
				case QImage::Format_RGBA8888_Premultiplied:
					external_format = GL_RGBA;
					external_pixel_size = GL_UNSIGNED_BYTE;
					break;
				case QImage::Format_RGB32:
				case QImage::Format_ARGB32:
				case QImage::Format_ARGB32_Premultiplied:
#ifdef __ANDROID__
					external_format = GL_RGBA; // Format_RGB32 = (0xffGGRRBB) which is the same as BGRA
#else
					external_format = GL_BGRA; // Format_RGB32 = (0xffGGRRBB) which is the same as BGRA
#endif
					external_pixel_size = GL_UNSIGNED_BYTE;
					break;
				case QImage::Format_Invalid:
					format_invalid = true;
					break; // This acts like a flag for HDR textures which have higher bits per pixel
				default:
					break;
				}

                if (!format_invalid) {
					MathUtil::glFuncs->glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, max_dim, max_dim, external_format, external_pixel_size, pData);
				}

                if (madeData) {
                    data->ClearPixelData();
				}
			}
		}
	}

    if (tex_mipmap)
    {
        MathUtil::glFuncs->glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }

    return CreateTextureHandle(TextureHandle::TEXTURE_TYPE::TEXTURE_CUBEMAP, color_space, tex_alpha, max_dim, max_dim, texture_id);
}

std::shared_ptr<TextureHandle> AbstractRenderer::CreateTextureFromAssetImageData(QPointer<AssetImageData> data, bool is_left, const bool tex_mipmap, const bool tex_linear, const bool tex_clamp, const TextureHandle::ALPHA_TYPE tex_alpha, const TextureHandle::COLOR_SPACE tex_colorspace)
{
    QByteArray data_ptr = (is_left)
            ? data->GetLeftFrameData(data->GetUploadedTextures())
            : data->GetRightFrameData(data->GetUploadedTextures());

    // If we have no data, just early out.
    if (data_ptr.length() == 0)
    {
         return nullptr;
    }

    uchar pixelSize = data->GetPixelSize();
    GLenum internal_format;
    GLenum external_format;
    GLenum external_pixel_size;
    InternalFormatFromSize(&internal_format, pixelSize);
    externalFormatAndTypeFromSize(&external_format, &external_pixel_size, pixelSize);
    bool data_has_alpha = false;

    switch(data->format)
    {
    case QImage::Format_RGB888:
        internal_format = (tex_colorspace == TextureHandle::COLOR_SPACE::LINEAR) ? GL_RGB8 : GL_SRGB8;
        external_format = GL_RGB;
        external_pixel_size = GL_UNSIGNED_BYTE;
        break;
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888_Premultiplied:
        data_has_alpha = true;
        internal_format = (tex_colorspace == TextureHandle::COLOR_SPACE::LINEAR) ? GL_RGBA8 : GL_SRGB8_ALPHA8;
        external_format = GL_RGBA;
        external_pixel_size = GL_UNSIGNED_BYTE;
        break;
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        data_has_alpha = true;
        internal_format = (tex_colorspace == TextureHandle::COLOR_SPACE::LINEAR) ? GL_RGBA8 : GL_SRGB8_ALPHA8;
#ifndef __ANDROID__
        external_format = GL_BGRA; // Format_RGB32 = (0xffGGRRBB) which is the same as BGRA
#else
        external_format = GL_RGBA;
#endif
        external_pixel_size = GL_UNSIGNED_BYTE;
        break;
    case QImage::Format_RGB32:
        internal_format = (tex_colorspace == TextureHandle::COLOR_SPACE::LINEAR) ? GL_RGB8 : GL_SRGB8;
#ifndef __ANDROID__
        external_format = GL_BGRA;
#else
        external_format = GL_RGBA;
#endif
        external_pixel_size = GL_UNSIGNED_BYTE;
        break;
    case QImage::Format_Invalid:
        break; // This acts like a flag for HDR textures which have higher bits per pixel
    default:
        break;
    }

	// Defaults for the texture handle
	TextureHandle::TEXTURE_TYPE texture_type = TextureHandle::TEXTURE_TYPE::TEXTURE_2D;
    TextureHandle::COLOR_SPACE color_space = tex_colorspace;
    TextureHandle::ALPHA_TYPE alpha_type = (internal_format == GL_RGB8 || internal_format == GL_SRGB8) ? TextureHandle::ALPHA_TYPE::NONE : tex_alpha;
    int const width_in_pixels = data->GetWidth();
    if (data_has_alpha && alpha_type == TextureHandle::ALPHA_TYPE::UNDEFINED)
	{
        QByteArray data_ptr = (is_left)
                ? data->GetLeftFrameData(data->GetUploadedTextures())
                : data->GetRightFrameData(data->GetUploadedTextures());

        if (!data_ptr.isEmpty()) {
            int const width = width_in_pixels * pixelSize;
			int const height = data->GetHeight();
            //int const first_red_offset = (external_format == GL_RGBA) ? 0 : 2;
            //int const first_green_offset = (external_format == GL_RGBA) ? 1 : 1;
            //int const first_blue_offset = (external_format == GL_RGBA) ? 2 : 0;
            int const first_alpha_offset = (external_format == GL_RGBA) ? 3 : 3;
            bool found_zero = false;
            bool found_one = false;
            bool found_blended = false;

            for (int row = 0; row < height; row++)
			{
                for (int column = 0; column < width; column += pixelSize)
				{
                    //uchar this_red = data_ptr[row * width + column + first_red_offset];
                    //uchar this_green = data_ptr[row * width + column + first_green_offset];
                    //uchar this_blue = data_ptr[row * width + column + first_blue_offset];
                    uchar & this_alpha = ((uchar *)data_ptr.data())[row * width + column + first_alpha_offset];
                    found_zero = (found_zero) ? found_zero : (this_alpha == 0x00);
                    found_one = (found_one) ? found_one : (this_alpha == 0xff);
                    found_blended = (found_blended) ? found_blended : ((this_alpha != 0xff) && (this_alpha != 0x00));
				}
			}

            if (!found_zero && !found_blended)
            {
                alpha_type = TextureHandle::ALPHA_TYPE::NONE;
            }
            else if (found_zero && !found_blended)
            {
                alpha_type = TextureHandle::ALPHA_TYPE::CUTOUT;
            }
            else if (found_one && found_blended)
            {
                alpha_type = TextureHandle::ALPHA_TYPE::MIXED;
            }
            else if (!found_one && found_blended)
            {
                alpha_type = TextureHandle::ALPHA_TYPE::BLENDED;
            }
		}
	}

    GLuint texture_id = 0;
    MathUtil::glFuncs->glGenTextures(1, &texture_id);
    MathUtil::glFuncs->glActiveTexture(GL_TEXTURE15);
    m_active_texture_slot = 15;
    MathUtil::glFuncs->glBindTexture(GL_TEXTURE_2D, texture_id);

    // Mipmaping
    int max_mip_level = log(qMax(data->GetWidth(), data->GetHeight()))/log(2);
    if (tex_mipmap)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, max_mip_level);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, max_mip_level);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
    }
    // Minification filter
    if (tex_mipmap)
    {
        if (tex_linear)
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        else
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        }
    }
    else
    {
        if (tex_linear)
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        else
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
    }

    // Magnification filter
    if (tex_linear)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    // Clamping
    if (tex_clamp)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    }

    // Anisotropy
    MathUtil::glFuncs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_max_anisotropy);

    // Allocate and Populate Texture
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_2D, 0, internal_format, data->GetWidth(), data->GetHeight(), 0,  external_format, external_pixel_size,
                                    (is_left)
                                        ? (const void*)data->GetLeftFrameData(data->GetUploadedTextures())
                                        : (const void*)data->GetRightFrameData(data->GetUploadedTextures())
                                    );

    if (tex_mipmap)
    {
        MathUtil::glFuncs->glGenerateMipmap(GL_TEXTURE_2D);
    }

    return CreateTextureHandle(texture_type, color_space, alpha_type, data->GetWidth(), data->GetHeight(), texture_id);
}

std::shared_ptr<TextureHandle> AbstractRenderer::CreateTextureFromGLIData(const QByteArray & ba, const bool tex_mipmap, const bool tex_linear, const bool tex_clamp, const TextureHandle::ALPHA_TYPE , const TextureHandle::COLOR_SPACE tex_colorspace)
{
    if (ba.length() == 0)
    {
        return nullptr;
    }

    gli::texture Texture = gli::load(ba.data(), ba.size());
    MathUtil::glFuncs->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//56.0 - early out here if load failed (prevents crash for ASSERT calls made in gli)
	if (Texture == gli::texture()) {
		return std::shared_ptr<TextureHandle>(nullptr);
	}

#ifdef __ANDROID__
    gli::gl GL(gli::gl::PROFILE_ES30);
#else
    gli::gl GL(gli::gl::PROFILE_GL33);
#endif
	gli::gl::format const Format = GL.translate(Texture.format(), Texture.swizzles());
	glm::tvec3<GLsizei> const Extent(Texture.extent());

	auto texture_target_gli = Texture.target();
    TextureHandle::TEXTURE_TYPE texture_type = (texture_target_gli == gli::TARGET_2D) ? TextureHandle::TEXTURE_TYPE::TEXTURE_2D : TextureHandle::TEXTURE_TYPE::TEXTURE_CUBEMAP;
	// TODO make this detect alpha via a loop like in the other creation functions
    TextureHandle::ALPHA_TYPE alpha_type = TextureHandle::ALPHA_TYPE::NONE;

	if ((Texture.target() != gli::TARGET_2D && Texture.target() != gli::TARGET_CUBE) || Texture.empty())
	{
		qDebug("ERROR: AssetImage::load_gli_compatible_image() failed due to invalid texture target or no texture data");
		return 0;
	}

    GLenum targetType;
	switch (Texture.target())
	{
	case gli::TARGET_2D:
        targetType = GL_TEXTURE_2D;
		break;
	case gli::TARGET_CUBE:
        targetType = GL_TEXTURE_CUBE_MAP;
		break;
	default:
		return 0;
		break;
    }


    GLenum internal_format = (tex_colorspace == TextureHandle::COLOR_SPACE::LINEAR)
            ? (static_cast<GLenum>(Format.Internal))
            : ((static_cast<GLenum>(Format.Internal) == GL_RGB8)
                ? GL_SRGB8
                : (static_cast<GLenum>(Format.Internal))
            );


    GLuint texture_id = 0;
    MathUtil::glFuncs->glGenTextures(1, &texture_id);
    MathUtil::glFuncs->glActiveTexture(GL_TEXTURE15);
    m_active_texture_slot = 15;
    MathUtil::glFuncs->glBindTexture(targetType, texture_id);

    // Mipmaping
    int max_mip_level = log(qMax(Extent.x, Extent.y))/log(2);
    if (tex_mipmap)
    {
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_BASE_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_MAX_LEVEL, max_mip_level);
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_MIN_LOD, 0);
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_MAX_LOD, max_mip_level);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_BASE_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_MAX_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_MIN_LOD, 0);
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_MAX_LOD, 0);
    }

    // Minification filter
    if (tex_mipmap)
    {
        if (tex_linear)
        {
            MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        else
        {
            MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        }
    }
    else
    {
        if (tex_linear)
        {
            MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        else
        {
            MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
    }

    // Magnification filter
    if (tex_linear)
    {
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    // Clamping
    if (tex_clamp)
    {
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_WRAP_S, GL_REPEAT);
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_WRAP_T, GL_REPEAT);
        MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_WRAP_R, GL_REPEAT);
    }

    // Anisotropy
    MathUtil::glFuncs->glTexParameterf(targetType, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_max_anisotropy);

    // Swizzles
    MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_SWIZZLE_R, Format.Swizzles[0]);
    MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_SWIZZLE_G, Format.Swizzles[1]);
    MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_SWIZZLE_B, Format.Swizzles[2]);
    MathUtil::glFuncs->glTexParameteri(targetType, GL_TEXTURE_SWIZZLE_A, Format.Swizzles[3]);

    // Allocate Data
#ifndef __ANDROID__
    if (targetType == GL_TEXTURE_CUBE_MAP)
    {
        MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internal_format, Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
        MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internal_format, Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
        MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internal_format, Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
        MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internal_format, Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
        MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internal_format, Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
        MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internal_format, Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
    }
    else
    {
        MathUtil::glFuncs->glTexImage2D(targetType, 0, internal_format, Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
    }
#else
    if (targetType == GL_TEXTURE_CUBE_MAP)
    {
        MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, static_cast<GLenum>(Format.Internal), Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
        MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, static_cast<GLenum>(Format.Internal), Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
        MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, static_cast<GLenum>(Format.Internal), Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
        MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, static_cast<GLenum>(Format.Internal), Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
        MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, static_cast<GLenum>(Format.Internal), Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
        MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, static_cast<GLenum>(Format.Internal), Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
    }
    else
    {
        MathUtil::glFuncs->glTexImage2D(targetType, 0, static_cast<GLenum>(Format.Internal), Extent.x, Extent.y, 0, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), 0);
    }
#endif

    if (tex_mipmap)
    {
        MathUtil::glFuncs->glGenerateMipmap(targetType);
    }

    // Populate Data
    for (std::size_t Layer = 0; Layer < Texture.layers(); ++Layer)
    {
		for (std::size_t Face = 0; Face < Texture.faces(); ++Face)
		{
			for (std::size_t Level = 0; Level < Texture.levels(); ++Level)
			{
				// If we've disabled mipmaps skip over any in the file.
				if (tex_mipmap == false && Level != 0)
				{
					break;
				}

                glm::tvec3<GLsizei> level_extent(Texture.extent(Level));

				switch (Texture.target())
				{
				case gli::TARGET_1D:
					break;
				case gli::TARGET_1D_ARRAY:
					break;
				case gli::TARGET_2D:
					if (gli::is_compressed(Texture.format()))
					{
						GLsizei blockCounts[2];
                        GLsizei blockCountX = static_cast<GLsizei>(std::ceil(static_cast<double>(level_extent.x) / Texture.Storage->block_extent().x));
                        GLsizei blockCountY = static_cast<GLsizei>(std::ceil(static_cast<double>(level_extent.y) / Texture.Storage->block_extent().y));
						blockCounts[0] = std::max(blockCountX, 1);
						blockCounts[1] = std::max(blockCountY, 1);
						GLsizei manLevelSize = static_cast<GLsizei>(blockCounts[0] * blockCounts[1] * Texture.Storage->block_size());

                        MathUtil::glFuncs->glCompressedTexSubImage2D(targetType, static_cast<int>(Level), 0, 0, level_extent.x, level_extent.y, static_cast<GLenum>(Format.External), manLevelSize, (const void*)Texture.data(Layer, Face, Level));
					}
					else
					{
                        MathUtil::glFuncs->glTexSubImage2D(targetType, static_cast<int>(Level), 0, 0, level_extent.x, level_extent.y, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), (const void*)Texture.data(Layer, Face, Level));
					}
					break;
				case gli::TARGET_CUBE:
					if (gli::is_compressed(Texture.format()))
					{
						GLsizei blockCounts[2];
                        GLsizei blockCountX = static_cast<GLsizei>(std::ceil(static_cast<double>(level_extent.x) / Texture.Storage->block_extent().x));
                        GLsizei blockCountY = static_cast<GLsizei>(std::ceil(static_cast<double>(level_extent.y) / Texture.Storage->block_extent().y));
						blockCounts[0] = std::max(blockCountX, 1);
						blockCounts[1] = std::max(blockCountY, 1);
						GLsizei manLevelSize = static_cast<GLsizei>(blockCounts[0] * blockCounts[1] * Texture.Storage->block_size());

                        MathUtil::glFuncs->glCompressedTexSubImage2D(static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<int>(Face)), static_cast<int>(Level), 0, 0, level_extent.x, level_extent.y, static_cast<GLenum>(Format.External), manLevelSize, (const void*)Texture.data(Layer, Face, Level));
					}
					else
					{
                        MathUtil::glFuncs->glTexSubImage2D(static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<int>(Face)), static_cast<int>(Level), 0, 0, level_extent.x, level_extent.y, static_cast<GLenum>(Format.External), static_cast<GLenum>(Format.Type), (const void*)Texture.data(Layer, Face, Level));
                    }
					break;
				case gli::TARGET_2D_ARRAY:
					break;
				case gli::TARGET_3D:
					break;
				case gli::TARGET_CUBE_ARRAY:
					break;
				default:
					break;
				} // switch(Texture.target())
			} // ++Level
		} // ++Face
	} // ++Layer

	// Generate mipmaps if we didn't have any in the file
    if (Texture.levels() == 1 && tex_mipmap)
    {
        MathUtil::glFuncs->glGenerateMipmap(targetType);
    }
    MathUtil::glFuncs->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    return CreateTextureHandle(texture_type, tex_colorspace, alpha_type, Extent.x, Extent.y, texture_id);
}

std::shared_ptr<TextureHandle> AbstractRenderer::CreateCubemapTextureHandle(uint32_t const p_width, uint32_t const p_height, TextureHandle::COLOR_SPACE const p_color_space, const int32_t , const bool tex_mipmap, const bool tex_linear, const bool tex_clamp, const TextureHandle::ALPHA_TYPE , const TextureHandle::COLOR_SPACE tex_colorspace)
{
    GLuint texture_id = 0;
    MathUtil::glFuncs->glGenTextures(1, &texture_id);
    MathUtil::glFuncs->glActiveTexture(GL_TEXTURE15);
    m_active_texture_slot = 15;
    MathUtil::glFuncs->glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

    // Mipmaping
    int max_mip_level = log(qMax(p_width, p_height))/log(2);
    if (tex_mipmap)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, max_mip_level);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, max_mip_level);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_LOD, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LOD, 0);
    }

    // Minification filter
    if (tex_mipmap)
    {
        if (tex_linear)
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        else
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
        }
    }
    else
    {
        if (tex_linear)
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        else
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
    }

    // Magnification filter
    if (tex_linear)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    // Clamping
    if (tex_clamp)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_REPEAT);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_REPEAT);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_REPEAT);
    }

    // Anisotropy
    MathUtil::glFuncs->glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_max_anisotropy);

    // Allocate Texture
    GLenum internal_format = (tex_colorspace == TextureHandle::COLOR_SPACE::LINEAR) ? GL_RGB8 : GL_SRGB8;

    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internal_format, p_width, p_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internal_format, p_width, p_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internal_format, p_width, p_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internal_format, p_width, p_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internal_format, p_width, p_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internal_format, p_width, p_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

    if (tex_mipmap)
    {
        MathUtil::glFuncs->glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }

    TextureHandle::TEXTURE_TYPE texture_type =  TextureHandle::TEXTURE_TYPE::TEXTURE_CUBEMAP;
    TextureHandle::COLOR_SPACE color_space = p_color_space;
    TextureHandle::ALPHA_TYPE alpha_type = TextureHandle::ALPHA_TYPE::NONE;

    return CreateTextureHandle(texture_type, color_space, alpha_type,p_width, p_height, texture_id);
}
#ifdef WIN32
QVector<std::shared_ptr<TextureHandle>> AbstractRenderer::CreateSlugTextureHandles(uint32_t const p_curve_texture_width,
                                                                                       uint32_t const p_curve_texture_height,
                                                                                       void const * p_curve_texture,
                                                                                       uint32_t const p_band_texture_width,
                                                                                       uint32_t const p_band_texture_height,
                                                                                       void const * p_band_texture)
{
    // Create TextureHandles for the font textures
    GLuint texture_ids[2] = {0, 0};
    MathUtil::glFuncs->glActiveTexture(GL_TEXTURE15);
    m_active_texture_slot = 15;
    MathUtil::glFuncs->glGenTextures(2, &(texture_ids[0]));

    MathUtil::glFuncs->glBindTexture(GL_TEXTURE_RECTANGLE, texture_ids[0]);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA16F, p_curve_texture_width, p_curve_texture_height, 0, GL_RGBA, GL_HALF_FLOAT, p_curve_texture);

    MathUtil::glFuncs->glBindTexture(GL_TEXTURE_RECTANGLE, texture_ids[1]);
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA16UI, p_band_texture_width, p_band_texture_height, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, p_band_texture);

    // The OpenGL spec requires that the filtering modes for integer textures be set to GL_NEAREST,
    // or else the results of a texture fetch are undefined. Nvidia and AMD drivers still return the
    // expected texel values, but the Intel driver returns zeros if the default modes aren't changed.
    MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Defaults for the texture handle
    TextureHandle::TEXTURE_TYPE texture_type = TextureHandle::TEXTURE_TYPE::TEXTURE_RECTANGLE;
    TextureHandle::COLOR_SPACE color_space = TextureHandle::COLOR_SPACE::LINEAR;
    TextureHandle::ALPHA_TYPE alpha_type = TextureHandle::ALPHA_TYPE::CUTOUT;

    QVector<std::shared_ptr<TextureHandle>> handles;
    handles.reserve(2);
    handles.push_back(CreateTextureHandle(texture_type, color_space, alpha_type,
                                          p_curve_texture_width, p_curve_texture_height, texture_ids[0]));
    handles.push_back(CreateTextureHandle(texture_type, color_space, alpha_type,
                                          p_band_texture_width, p_band_texture_height, texture_ids[1]));

    return handles;

}
#endif
std::shared_ptr<TextureHandle> AbstractRenderer::CreateTextureQImage(const QImage & img, const bool tex_mipmap, const bool tex_linear, const bool tex_clamp, const TextureHandle::ALPHA_TYPE tex_alpha, const TextureHandle::COLOR_SPACE tex_colorspace)
{
    GLuint texture_id = 0;
    MathUtil::glFuncs->glGenTextures(1, &texture_id);
    MathUtil::glFuncs->glActiveTexture(GL_TEXTURE15);
    m_active_texture_slot = 15;
    MathUtil::glFuncs->glBindTexture(GL_TEXTURE_2D, texture_id);

	//mipmap levels
    int max_mip_level = log(qMax(img.width(), img.height()))/log(2);
    if (tex_mipmap)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, max_mip_level);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, max_mip_level);
	}
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
	}

	//minification filter
    if (tex_mipmap)
    {
        if (tex_linear)
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
        else
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		}
	}
    else
    {
        if (tex_linear)
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
        else
        {
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
	}

	//magnification filter
    if (tex_linear)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	//clamping
    if (tex_clamp)
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	}
    else
    {
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	}

	//aniso
    MathUtil::glFuncs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_max_anisotropy);

    GLenum internal_format = GL_RGBA8;
    GLenum external_format = GL_RGB;
    GLenum external_pixel_size = GL_UNSIGNED_BYTE;
    switch(img.format())
    {
    case QImage::Format_RGB888:
        // Allocate Texture
        internal_format = (tex_colorspace == TextureHandle::COLOR_SPACE::LINEAR) ? GL_RGB8 : GL_SRGB8;
        external_format = GL_RGB;
        external_pixel_size = GL_UNSIGNED_BYTE;
        break;
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888_Premultiplied:
        internal_format = (tex_colorspace == TextureHandle::COLOR_SPACE::LINEAR) ? GL_RGBA8 : GL_SRGB8_ALPHA8;
        external_format = GL_RGBA;
        external_pixel_size = GL_UNSIGNED_BYTE;
        break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        internal_format = (tex_colorspace == TextureHandle::COLOR_SPACE::LINEAR) ? GL_RGBA8 : GL_SRGB8_ALPHA8;
#ifdef __ANDROID__
        external_format = GL_RGBA; // Format_RGB32 = (0xffGGRRBB) which is the same as BGRA
#else
        external_format = GL_BGRA; // Format_RGB32 = (0xffGGRRBB) which is the same as BGRA
#endif
        external_pixel_size = GL_UNSIGNED_BYTE;
        break;
    case QImage::Format_Invalid:
        break; // This acts like a flag for HDR textures which have higher bits per pixel
    default:
        break;
    }

    // Allocate Texture
    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_2D, 0, internal_format, img.width(), img.height(), 0, external_format, external_pixel_size, (const void*)img.constBits());

    if (tex_mipmap)
    {
        MathUtil::glFuncs->glGenerateMipmap(GL_TEXTURE_2D);
    }

    // Defaults for the texture handle
    TextureHandle::TEXTURE_TYPE texture_type = TextureHandle::TEXTURE_TYPE::TEXTURE_2D;
    TextureHandle::COLOR_SPACE color_space = TextureHandle::COLOR_SPACE::LINEAR;
    TextureHandle::ALPHA_TYPE alpha_type = tex_alpha;

    uchar const * img_data = img.constBits();

    if (alpha_type == TextureHandle::ALPHA_TYPE::UNDEFINED)
    {
#ifdef __ANDROID__
        if (external_format == GL_RGBA)
#else
        if (external_format == GL_RGBA || external_format == GL_BGRA)
#endif
        {
            if (img_data != nullptr)
            {
                int const width = img.width();
                int const height = img.height();
                int const first_alpha_offset = (external_format == GL_RGBA) ? 3 : 0;
                int const pixel_offset = 4;
                bool found_zero = false;
                bool found_one = false;
                bool found_blended = false;

                for (int row = 0; row < height; row++)
                {
					for (int column = first_alpha_offset; column < width; column += pixel_offset)
					{
						uchar const * this_alpha_ptr = &(img_data[row * width + column]);
						if (this_alpha_ptr != nullptr)
						{
							found_zero = (found_zero) ? found_zero : (*this_alpha_ptr == 0x00);
							found_one = (found_one) ? found_one : (*this_alpha_ptr == 0xff);
							found_blended = (found_blended) ? found_blended : ((*this_alpha_ptr != 0xff) && (*this_alpha_ptr != 0x00));
						}
                    }
                }

                if (!found_zero && !found_blended)
                {
                    alpha_type = TextureHandle::ALPHA_TYPE::NONE;
                }
                else if (found_zero && !found_blended)
                {
                    alpha_type = TextureHandle::ALPHA_TYPE::CUTOUT;
                }
                else if (found_one && found_blended)
                {
                    alpha_type = TextureHandle::ALPHA_TYPE::MIXED;
                }
                else if (!found_one && found_blended)
                {
                    alpha_type = TextureHandle::ALPHA_TYPE::BLENDED;
                }
            }
        }
        else
        {
            alpha_type = TextureHandle::ALPHA_TYPE::NONE;
        }
    }

    return CreateTextureHandle(texture_type, color_space, alpha_type, img.width(), img.height(), texture_id);
}

void AbstractRenderer::UpdateTextureHandleData(TextureHandle* p_handle, uint const p_level, uint const p_x_offset, uint const p_y_offset, uint const p_width, uint const p_height, uint const p_pixel_size, void* const p_pixel_data)
{
	GLenum pixel_format = 0;
	GLenum pixel_type = 0;
    externalFormatAndTypeFromSize(&pixel_format, &pixel_type, p_pixel_size);
    uint32_t const data_size = p_width * p_height * p_pixel_size;
    UpdateTextureHandleData(p_handle, p_level, p_x_offset, p_y_offset, p_width, p_height, pixel_format, pixel_type, p_pixel_data, data_size);
}

void AbstractRenderer::UpdateTextureHandleData(TextureHandle* p_handle, uint const p_level, uint const p_x_offset, uint const p_y_offset, uint const p_width, uint const p_height, int const p_pixel_format, int const p_pixel_type, void* const p_pixel_data, uint32_t const p_data_size)
{
	GLenum target = (p_handle->GetTextureType() == TextureHandle::TEXTURE_2D) ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;

    // async CPU to GPU trasnfer of data
    GLuint pbo = 0;
    MathUtil::glFuncs->glGenBuffers(1, &pbo);
    MathUtil::glFuncs->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    MathUtil::glFuncs->glBufferData(GL_PIXEL_UNPACK_BUFFER, p_data_size, nullptr, GL_STREAM_DRAW);
    MathUtil::glFuncs->glBufferData(GL_PIXEL_UNPACK_BUFFER, p_data_size, p_pixel_data, GL_STREAM_DRAW);

    // async PBO to texture transfer of data, we use offset of 0 rather than p_pixel_data as the last param since we have
    // a PBO bound to the PIXEL_UNPACK_BUFFER slot
    BindTextureHandle(&(this->m_texture_handle_to_GL_ID), 0, p_handle);
    MathUtil::glFuncs->glTexSubImage2D(target, p_level, p_x_offset, p_y_offset, p_width, p_height, p_pixel_format, p_pixel_type, 0);

    MathUtil::glFuncs->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    MathUtil::glFuncs->glDeleteBuffers(1, &pbo);
}

void AbstractRenderer::GenerateTextureHandleMipMap(TextureHandle* p_handle)
{
	GLenum target = (p_handle->GetTextureType() == TextureHandle::TEXTURE_2D) ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
    BindTextureHandle(&(this->m_texture_handle_to_GL_ID), 0, p_handle);
    MathUtil::glFuncs->glGenerateMipmap(target);
}

void AbstractRenderer::CopyTextureHandleDataToTextureHandle(TextureHandle* p_src_handle, const int32_t p_src_level, const int32_t p_src_x, const int32_t p_src_y, const int32_t p_src_z, TextureHandle* p_dst_handle, const int32_t p_dst_level, const int32_t p_dst_x, const int32_t p_dst_y, const int32_t p_dst_z, const int32_t p_src_width, const int32_t p_src_height, const int32_t p_src_depth)
{
    if (!p_src_handle || !p_dst_handle)
    {
        return;
    }

    GLuint src_id = 0;
    GLenum src_target = 0;
    GLuint dst_id = 0;
    GLenum dst_target = 0;
    bool found_src = false;
    bool found_dst = false;
    TextureHandle * p_src_pointer = p_src_handle;
    TextureHandle * p_dst_pointer = p_dst_handle;
    TextureHandle * itr_pointer = nullptr;

    const size_t tex_count = m_texture_handle_to_GL_ID.size();
    for (size_t itr = 0; itr < tex_count; ++itr)
    {
        QPair<TextureHandle*, GLuint>& pair_ref = m_texture_handle_to_GL_ID[itr];
        itr_pointer = pair_ref.first;

        if (itr_pointer->m_UUID.m_UUID == p_src_pointer->m_UUID.m_UUID)
        {
            src_id = pair_ref.second;
            src_target = static_cast<GLenum>(pair_ref.first->GetTextureType() == TextureHandle::TEXTURE_2D ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP);
            found_src = true;
        }

        if (itr_pointer->m_UUID.m_UUID == p_dst_pointer->m_UUID.m_UUID)
        {
            dst_id = pair_ref.second;
            dst_target = static_cast<GLenum>(pair_ref.first->GetTextureType() == TextureHandle::TEXTURE_2D ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP);
            found_dst = true;
        }
    }

    if (found_src && found_dst)
    {
#ifndef __ANDROID__
        // This is GL 4.3+, GL33Renderer will need to override this method with a FBO blitting based approach
        MathUtil::glFuncs->glCopyImageSubData(src_id, src_target, p_src_level, p_src_x, p_src_y, p_src_z,
                                              dst_id, dst_target, p_dst_level, p_dst_x, p_dst_y, p_dst_z,
                                              p_src_width, p_src_height, p_src_depth);
#endif
    }
}

void AbstractRenderer::CopyReadBufferToTextureHandle(QVector<QPair<TextureHandle *, GLuint>> * const , TextureHandle* p_handle, uint32_t p_target, int32_t p_level, int32_t p_dst_x, int32_t p_dst_y, int32_t p_src_x, int32_t p_src_y, int32_t p_src_width, int32_t p_src_height)
{
    if (p_handle) {
        MathUtil::glFuncs->glCopyTexSubImage2D(p_target, p_level, p_dst_x, p_dst_y, p_src_x, p_src_y, p_src_width, p_src_height);
    }    
}

std::shared_ptr<BufferHandle> AbstractRenderer::CreateBufferHandle(
	BufferHandle::BUFFER_TYPE const p_buffer_type,
	BufferHandle::BUFFER_USAGE const p_buffer_usage)
{     
    GLuint buffer_GL_ID;
	MathUtil::glFuncs->glGenBuffers(1, &buffer_GL_ID);
    QPair<BufferHandle*, GLuint> buffer_pair;
	buffer_pair.first = new BufferHandle(m_buffer_UUID, p_buffer_type, p_buffer_usage);
	buffer_pair.second = buffer_GL_ID;

    m_buffer_UUID++;

    if (m_buffer_handle_to_GL_ID.capacity() == m_buffer_handle_to_GL_ID.size())
    {
        m_buffer_handle_to_GL_ID.reserve(m_buffer_handle_to_GL_ID.size() + 128);
    }

    m_buffer_handle_to_GL_ID.push_back(buffer_pair);
    buffer_pair.first->m_last_known_index = (m_buffer_handle_to_GL_ID.size() - 1);

	BufferHandle* mesh_address = m_buffer_handle_to_GL_ID.back().first;

	std::shared_ptr<BufferHandle> buffer_handle = std::shared_ptr<BufferHandle>(mesh_address,
		[this](BufferHandle* ptr)
	{
		this->RemoveBufferHandleFromMap(ptr);
	});

#ifdef QT_DEBUG
    auto buffer_UUID = buffer_pair.first->m_UUID;
	qDebug() << QString("CreateBufferHandle:: ")
		+ "UUID: " + QString::number(buffer_UUID.m_UUID) + ' '
		+ "GPUHandle: " + QString::number((uint32_t)buffer_pair.second);
#endif
	return buffer_handle;
}

void AbstractRenderer::FreeBufferHandles()
{
    m_buffer_deletion_guard.lock();
    size_t const buffer_count = m_buffers_pending_deletion.size();
    for (size_t i = 0; i < buffer_count; ++i)
    {
        size_t const last_index = m_buffers_pending_deletion[i]->m_last_known_index;
        QPair<BufferHandle*, GLuint>& buffer_pair = m_buffer_handle_to_GL_ID[last_index];

#ifdef QT_DEBUG
    auto mesh_UUID = buffer_pair.first->m_UUID;
    qDebug() << QString("FreeBufferHandles:: ")
        + "UUID: " + QString::number(mesh_UUID.m_UUID) + ' '
        + "GPUHandle: " + QString::number((uint32_t)buffer_pair.second);
#endif

        // Null m_mesh_handle_to_GL_ID ptr
        buffer_pair.first = nullptr;

        // Delete GL resource
        MathUtil::glFuncs->glDeleteBuffers(1, &buffer_pair.second);
        buffer_pair.second = 0;

        // Free memory used by BufferHandle
        delete m_buffers_pending_deletion[i];
    }
    m_buffers_pending_deletion.clear();
    m_buffer_deletion_guard.unlock();
}

void AbstractRenderer::RemoveBufferHandleFromMap(BufferHandle* p_handle)
{    
    if (p_handle == nullptr)
    {
        qDebug() << QString("ERROR!: RemoveBufferHandleFromMap:: p_handle was nullptr");
        return;
    }

    // We keep handles in the vector but clear their contents and add the address to a free list so that it can be reused by a later creation call;
    const size_t buffer_count = m_buffer_handle_to_GL_ID.size();
    for (size_t itr = 0; itr < buffer_count; ++itr)
	{
        if (m_buffer_handle_to_GL_ID[itr].first == p_handle && m_buffer_handle_to_GL_ID[itr].first->m_UUID.m_UUID == p_handle->m_UUID.m_UUID)
		{
            QPair<BufferHandle*, GLuint>& buffer_pair = m_buffer_handle_to_GL_ID[itr];
            buffer_pair.first->m_last_known_index = itr;
            buffer_pair.first->m_UUID.m_in_use_flag = 0;
#ifdef QT_DEBUG
    auto buffer_UUID = buffer_pair.first->m_UUID;
    qDebug() << QString("RemoveBufferHandleFromMap:: (queue for deletion) ")
        + "UUID: " + QString::number(buffer_UUID.m_UUID) + ' '
        + "GPUHandle: " + QString::number((uint32_t)buffer_pair.second);
#endif
            m_buffer_deletion_guard.lock();
            m_buffers_pending_deletion.push_back(buffer_pair.first);
            m_buffer_deletion_guard.unlock();
            return;
		}
	}
}

std::shared_ptr<ProgramHandle> AbstractRenderer::CreateProgramHandle(uint32_t *p_GPU_ID)
{   
    *p_GPU_ID = MathUtil::glFuncs->glCreateProgram();
    QPair<ProgramHandle*, GLuint> buffer_pair;
    buffer_pair.first = new ProgramHandle(m_shader_UUID);
    buffer_pair.second = *p_GPU_ID;

    m_shader_UUID++;

    if (m_program_handle_to_GL_ID.capacity() == m_program_handle_to_GL_ID.size())
    {
        m_program_handle_to_GL_ID.reserve(m_program_handle_to_GL_ID.size() + 128);
    }

    m_program_handle_to_GL_ID.push_back(buffer_pair);

    ProgramHandle* program_address = m_program_handle_to_GL_ID.back().first;

    std::shared_ptr<ProgramHandle> program_handle = std::shared_ptr<ProgramHandle>(program_address,
        [this](ProgramHandle* ptr)
    {
        this->RemoveProgramHandleFromMap(ptr);
    });

/*#ifdef QT_DEBUG
    auto program_UUID = buffer_pair.first->m_UUID;
    qDebug() << QString("CreateProgramHandle:: ")
        + "UUID: " + QString::number(program_UUID.m_UUID) + ' '
        + "InUseFlag: " + ((program_UUID.m_in_use_flag == 0) ? "NOT_IN_USE" : "IN_USE") + ' '
        + "GPUHandle: " + QString::number((uint32_t)buffer_pair.second);
#endif*/
    return program_handle;
}

GLuint AbstractRenderer::GetProgramHandleID(ProgramHandle * p_handle)
{
    const size_t program_count = m_program_handle_to_GL_ID.size();
    for (size_t itr = 0; itr < program_count; ++itr) {
        if (m_program_handle_to_GL_ID[itr].first == p_handle && m_program_handle_to_GL_ID[itr].first->m_UUID.m_UUID == p_handle->m_UUID.m_UUID) {
            return m_program_handle_to_GL_ID[itr].second;
        }
    }
/*#ifdef QT_DEBUG
    qDebug() << QString("ERROR: called GetProgramHandleID with invalid ProgramHandle") + QString::number((uint64_t)p_handle);
#endif*/
    return 0;
}

void AbstractRenderer::ConfigureBufferHandleData(std::shared_ptr<BufferHandle> p_buffer_handle, uint32_t const p_data_size, void* const p_data, BufferHandle::BUFFER_USAGE const p_buffer_usage)
{
	p_buffer_handle->m_UUID.m_buffer_usage = p_buffer_usage;
	GLenum const buffer_target = p_buffer_handle->GetBufferTypeEnum();
	GLenum const buffer_usage = p_buffer_handle->GetBufferUsageEnum();
    BindBufferHandle(p_buffer_handle.get());
	MathUtil::glFuncs->glBufferData(buffer_target, p_data_size, p_data, buffer_usage);
}

void AbstractRenderer::UpdateBufferHandleData(std::shared_ptr<BufferHandle> p_buffer_handle, uint32_t const p_offset, uint32_t const p_data_size, void* const p_data)
{
    BindBufferHandle(p_buffer_handle.get());
	GLenum const buffer_target = p_buffer_handle->GetBufferTypeEnum();
    MathUtil::glFuncs->glBufferSubData(buffer_target, p_offset, p_data_size, p_data);
    MathUtil::glFuncs->glFlush();
}

void AbstractRenderer::prependDataInShaderMainFunction(QByteArray& p_shader_source, char const * p_insertion_string)
{
    auto data_itr = p_shader_source.begin();
    auto end_itr = p_shader_source.end();
    auto main_index = p_shader_source.indexOf("void main");
    data_itr += main_index;
    int current_index = main_index;
    int stack_count = 0;
    bool found_last_brace = false;
    int insertion_index = 0;
    bool ignore_current_line = false;
    bool ignore_current_block = false;
    bool found_forward_slash = false;
    bool found_asterix = false;
    while (found_last_brace == false && data_itr != end_itr)
    {
        switch(*(data_itr++))
        {
        case '*':
            found_asterix = true;
            if (found_forward_slash)
            {
                ignore_current_block = true;
            }
            break;
        case '\n':
            found_asterix = false;
            found_forward_slash = false;
            ignore_current_line = false;
            break;
        case '/':
            if (found_asterix)
            {
                ignore_current_block = false;
            }
            found_asterix = false;
            if (found_forward_slash == true)
            {
                ignore_current_line = true;
            }
            found_forward_slash = true;
            break;
        case '{':
            found_asterix = false;
            if (ignore_current_line == false && ignore_current_block == false)
            {
                stack_count++;
            }
            found_forward_slash = false;
            break;
        case '}':
            found_asterix = false;
            if (ignore_current_line == false && ignore_current_block == false)
            {
                stack_count--;
                if (stack_count == 0)
                {
                    found_last_brace = true;
                    insertion_index = current_index;
                    data_itr -= 5;
                }
            }
            found_forward_slash = false;
            break;
        default:
            found_asterix = false;
            found_forward_slash = false;
            break;
        }

        current_index++;
    }

    p_shader_source.insert(insertion_index, p_insertion_string);
}

bool AbstractRenderer::IsFramebufferConfigurationValid() const
{
    return (m_window_width != 0 && m_window_height != 0);
}

void AbstractRenderer::checkFrameBufferCompleteness(const uint32_t p_target) const
{
    GLenum error = MathUtil::glFuncs->glCheckFramebufferStatus((GLenum)p_target);
    if (error != GL_FRAMEBUFFER_COMPLETE)
    {
        switch(error)
        {
            case GL_FRAMEBUFFER_UNDEFINED : qDebug("ERROR: Framebuffer incomplete, GL_FRAMEBUFFER_UNDEFINED."); break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT : qDebug("ERROR: Framebuffer incomplete, GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT."); break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT : qDebug("ERROR: Framebuffer incomplete, GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT."); break;
            case GL_FRAMEBUFFER_UNSUPPORTED : qDebug("ERROR: Framebuffer incomplete, GL_FRAMEBUFFER_UNSUPPORTED."); break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE : qDebug("ERROR: Framebuffer incomplete, GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE."); break;
#ifndef __ANDROID__
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER : qDebug("ERROR: Framebuffer incomplete, GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER."); break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER : qDebug("ERROR: Framebuffer incomplete, GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER."); break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS : qDebug("ERROR: Framebuffer incomplete, GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS."); break;
#endif
            case GL_INVALID_ENUM : qDebug("ERROR: Framebuffer incomplete, GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS."); break;
            default : qDebug("ERROR: Framebuffer incomplete, error not recognised: "); break;
        }
    }
}

void AbstractRenderer::UpdateFramebuffer()
{
    if (IsFramebufferConfigurationValid() && m_framebuffer_requires_initialization)
    {
        MathUtil::glFuncs->glActiveTexture(GL_TEXTURE15);
        m_active_texture_slot = 15;
        bool const do_multi_fbos = (m_msaa_count > 0);
        size_t const fbo_count = (do_multi_fbos ? 2 : 1);
        m_FBOs.resize(fbo_count);
        m_textures.resize(fbo_count * FBO_TEXTURE::COUNT);
        m_texture_handles = QVector<std::shared_ptr<TextureHandle>>(fbo_count * FBO_TEXTURE::COUNT, std::make_shared<TextureHandle>());

        // For each fbo
        for (uint32_t fbo_index = 0; fbo_index < fbo_count; ++fbo_index)
        {
            GLenum const texture_type = (fbo_index == 0) ? GL_TEXTURE_2D : GL_TEXTURE_2D_MULTISAMPLE;

            // GL_RGBA8 colour
            GLuint colour_texture = 0;
            MathUtil::glFuncs->glGenTextures(1, &colour_texture);
            m_textures[fbo_index * FBO_TEXTURE::COUNT + FBO_TEXTURE::COLOR] = colour_texture;
            MathUtil::glFuncs->glBindTexture(texture_type, colour_texture);
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_NEAREST));
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_NEAREST));
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));
            if (texture_type == GL_TEXTURE_2D_MULTISAMPLE)
            {
#ifndef __ANDROID__
                MathUtil::glFuncs->glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaa_count, GL_RGBA8, m_window_width, m_window_height, GL_TRUE);
#else
                MathUtil::glFuncs->glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaa_count, GL_RGBA8, m_window_width, m_window_height, GL_TRUE);
#endif
            }
            else
            {
                MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_RGBA8), m_window_width,  m_window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            }

            m_texture_handles[fbo_index * FBO_TEXTURE::COUNT + FBO_TEXTURE::COLOR] = CreateTextureHandle(TextureHandle::TEXTURE_TYPE::TEXTURE_2D,
                                                                                                         TextureHandle::COLOR_SPACE::LINEAR,
                                                                                                         TextureHandle::ALPHA_TYPE::NONE,
                                                                                                         m_window_width, m_window_height,
                                                                                                         colour_texture);

            /*// GL_RGB16F AO
            GLuint ao_texture = 0;
            MathUtil::glFuncs->glGenTextures(1, &ao_texture);
            m_textures[fbo_index * FBO_TEXTURE::COUNT + FBO_TEXTURE::AO] = ao_texture;
            MathUtil::glFuncs->glBindTexture(texture_type, ao_texture);
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_LINEAR));
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_LINEAR));
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));
            if (texture_type == GL_TEXTURE_2D_MULTISAMPLE)
            {
#ifndef __ANDROID__
                MathUtil::glFuncs->glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaa_count, GL_RGB16F, m_window_width, m_window_height, GL_TRUE);
#else
                MathUtil::glFuncs->glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaa_count, GL_RGB16F, m_window_width, m_window_height, GL_TRUE);
#endif
            }
            else
            {
                MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_RGB16F), m_window_width,  m_window_height, 0, GL_RGB, GL_HALF_FLOAT, NULL);
            }

            m_texture_handles[fbo_index * FBO_TEXTURE::COUNT + FBO_TEXTURE::AO] = CreateTextureHandle(TextureHandle::TEXTURE_TYPE::TEXTURE_2D,
                                                                                                         TextureHandle::COLOR_SPACE::LINEAR,
                                                                                                         TextureHandle::ALPHA_TYPE::NONE,
                                                                                                         m_window_width, m_window_height,
                                                                                                         ao_texture);*/

            // GL_DEPTH24_STENCIL8 (On most modern cards this is emulated by using 24-bits of a 32-bit int texture for depth and an 8-bit texture for stencil
            GLuint depth_stencil_texture = 0;
            MathUtil::glFuncs->glGenTextures(1, &depth_stencil_texture);
            m_textures[fbo_index * FBO_TEXTURE::COUNT + FBO_TEXTURE::DEPTH_STENCIL] = depth_stencil_texture;
            MathUtil::glFuncs->glBindTexture(texture_type, depth_stencil_texture);
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(GL_NEAREST));
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(GL_NEAREST));
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, static_cast<GLint>(GL_CLAMP_TO_EDGE));
            MathUtil::glFuncs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, static_cast<GLint>(GL_CLAMP_TO_EDGE));
            if(GetIsUsingEnhancedDepthPrecision() && GetIsEnhancedDepthPrecisionSupported())
            {
				m_glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
				m_depth_func = DepthFunc::LEQUAL;
				MathUtil::glFuncs->glDepthFunc(GL_GEQUAL);
                m_current_depth_func = DepthFunc::GEQUAL;
                MathUtil::glFuncs->glClearDepth(0.0);
				
				if (texture_type == GL_TEXTURE_2D_MULTISAMPLE)
                {
#ifndef __ANDROID__
                MathUtil::glFuncs->glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaa_count, GL_DEPTH32F_STENCIL8, m_window_width, m_window_height, GL_TRUE);
#else
                MathUtil::glFuncs->glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaa_count, GL_DEPTH32F_STENCIL8, m_window_width, m_window_height, GL_TRUE);
#endif
                }
                else
                {
                    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_DEPTH32F_STENCIL8), m_window_width,  m_window_height, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, NULL);
                }
            }
            else
            {
				// If it it supported we may have altered the ClipControl so set it back to default
				if (GetIsEnhancedDepthPrecisionSupported())
				{ 
					m_glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
				}
				
				m_depth_func = DepthFunc::LEQUAL;
				MathUtil::glFuncs->glDepthFunc(GL_LEQUAL);
                m_current_depth_func = DepthFunc::LEQUAL;
                MathUtil::glFuncs->glClearDepth(1.0);
				
				if (texture_type == GL_TEXTURE_2D_MULTISAMPLE)
                {
#ifndef __ANDROID__
                MathUtil::glFuncs->glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaa_count, GL_DEPTH24_STENCIL8, m_window_width, m_window_height, GL_TRUE);
#else
                MathUtil::glFuncs->glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_msaa_count, GL_DEPTH24_STENCIL8, m_window_width, m_window_height, GL_TRUE);
#endif
                }
                else
                {

                    MathUtil::glFuncs->glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_DEPTH24_STENCIL8), m_window_width,  m_window_height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
                }
            }

            m_texture_handles[fbo_index * FBO_TEXTURE::COUNT + FBO_TEXTURE::DEPTH_STENCIL] = CreateTextureHandle(TextureHandle::TEXTURE_TYPE::TEXTURE_2D,
                                                                                                         TextureHandle::COLOR_SPACE::LINEAR,
                                                                                                         TextureHandle::ALPHA_TYPE::NONE,
                                                                                                         m_window_width, m_window_height,
                                                                                                         depth_stencil_texture);

            // Attach layers and check completeness
            GLuint FBO;
            MathUtil::glFuncs->glGenFramebuffers(1, &FBO);
            m_FBOs[fbo_index] = FBO;
            MathUtil::glFuncs->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
            MathUtil::glFuncs->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_type, colour_texture, 0);
            //MathUtil::glFuncs->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + FBO_TEXTURE::AO, texture_type, ao_texture, 0);
            MathUtil::glFuncs->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, texture_type, depth_stencil_texture, 0);
            checkFrameBufferCompleteness(GL_DRAW_FRAMEBUFFER);
            GLenum drawBuffers[1];
            drawBuffers[0] = GL_COLOR_ATTACHMENT0;
            //drawBuffers[1] = GL_COLOR_ATTACHMENT0 + FBO_TEXTURE::AO;
            GLsizei drawBufferCount = 1;
            MathUtil::glFuncs->glDrawBuffers(drawBufferCount, &drawBuffers[0]);
        }

        m_framebuffer_requires_initialization = false;
        m_framebuffer_initialized = true;
    }
}

void AbstractRenderer::ConfigureFramebuffer(const uint32_t p_window_width, const uint32_t p_window_height, const uint32_t p_msaa_count)
{
	m_framebuffer_requires_initialization = (m_framebuffer_requires_initialization == false) 
												?	(m_window_width != p_window_width) ||
													(m_window_height != p_window_height) ||
													(m_msaa_count    != p_msaa_count)
												:	(m_framebuffer_requires_initialization);
    m_window_width = p_window_width;
    m_window_height = p_window_height;
    m_msaa_count = p_msaa_count;
}

void AbstractRenderer::ConfigureWindowSize(const uint32_t p_window_width, const uint32_t p_window_height)
{
	m_framebuffer_requires_initialization = (m_framebuffer_requires_initialization == false)
											?	(m_window_width != p_window_width) ||
												(m_window_height != p_window_height)
												: m_framebuffer_requires_initialization;

    m_window_width = p_window_width;
    m_window_width = p_window_height;
}

void AbstractRenderer::ConfigureSamples(const uint32_t p_msaa_count)
{
	m_framebuffer_requires_initialization = (m_framebuffer_requires_initialization == false)
											?	(m_msaa_count != p_msaa_count)
											:	m_framebuffer_requires_initialization;
    m_msaa_count = p_msaa_count;
}

uint32_t AbstractRenderer::GetTextureID(const FBO_TEXTURE_ENUM p_texture_index, const bool p_multisampled) const
{
    size_t const offset = (p_multisampled) ? FBO_TEXTURE::COUNT : 0;
    uint32_t texture_id = m_textures[p_texture_index + offset];
    return texture_id;
}

void AbstractRenderer::BindFBOAndTextures(QVector<uint32_t> &p_bound_buffers, const uint32_t p_texture_type, const uint32_t p_framebuffer_target, const uint32_t p_fbo, const size_t p_texture_offset, const FBO_TEXTURE_BITFIELD_ENUM p_textures_bitmask) const
{
    MathUtil::glFuncs->glBindFramebuffer((GLenum)p_framebuffer_target, (GLuint)p_fbo);

    if ((p_textures_bitmask & FBO_TEXTURE_BITFIELD::COLOR) != 0)
    {
        MathUtil::glFuncs->glFramebufferTexture2D(p_framebuffer_target,
                                                  GL_COLOR_ATTACHMENT0,
                                                  p_texture_type,
                                                  m_textures[FBO_TEXTURE::COLOR + p_texture_offset],
                                                  0);
        p_bound_buffers.push_back(GL_COLOR_ATTACHMENT0);
    }

    if ((p_textures_bitmask & FBO_TEXTURE_BITFIELD::AO) != 0)
    {
        MathUtil::glFuncs->glFramebufferTexture2D(p_framebuffer_target,
                                                  GL_COLOR_ATTACHMENT1,
                                                  p_texture_type,
                                                  m_textures[FBO_TEXTURE::AO + p_texture_offset],
                                                  0);
        p_bound_buffers.push_back(GL_COLOR_ATTACHMENT1);
    }

    if ((p_textures_bitmask & FBO_TEXTURE_BITFIELD::TRANSMISSION) != 0)
    {
        MathUtil::glFuncs->glFramebufferTexture2D(p_framebuffer_target,
                                                  GL_COLOR_ATTACHMENT2,
                                                  p_texture_type,
                                                  m_textures[FBO_TEXTURE::TRANSMISSION + p_texture_offset],
                                                  0);
        p_bound_buffers.push_back(GL_COLOR_ATTACHMENT2);
    }

    if ((p_textures_bitmask & FBO_TEXTURE_BITFIELD::MODULATION_DIFFUSION) != 0)
    {
        MathUtil::glFuncs->glFramebufferTexture2D(p_framebuffer_target,
                                                  GL_COLOR_ATTACHMENT3,
                                                  p_texture_type,
                                                  m_textures[FBO_TEXTURE::MODULATION_DIFFUSION + p_texture_offset],
                                                  0);
        p_bound_buffers.push_back(GL_COLOR_ATTACHMENT3);
    }

    if ((p_textures_bitmask & FBO_TEXTURE_BITFIELD::DELTA) != 0)
    {
        MathUtil::glFuncs->glFramebufferTexture2D(p_framebuffer_target,
                                                  GL_COLOR_ATTACHMENT4,
                                                  p_texture_type,
                                                  m_textures[FBO_TEXTURE::DELTA + p_texture_offset],
                                                  0);
        p_bound_buffers.push_back(GL_COLOR_ATTACHMENT4);
    }

    if ((p_textures_bitmask & FBO_TEXTURE_BITFIELD::COMPOSITED) != 0)
    {
        MathUtil::glFuncs->glFramebufferTexture2D(p_framebuffer_target,
                                                  GL_COLOR_ATTACHMENT5,
                                                  p_texture_type,
                                                  m_textures[FBO_TEXTURE::COMPOSITED + p_texture_offset],
                                                  0);
        p_bound_buffers.push_back(GL_COLOR_ATTACHMENT5);
    }


    if ((p_textures_bitmask & FBO_TEXTURE_BITFIELD::DEPTH_STENCIL) != 0)
    {
        MathUtil::glFuncs->glFramebufferTexture2D(p_framebuffer_target,
                                                  GL_DEPTH_STENCIL_ATTACHMENT,
                                                  p_texture_type,
                                                  m_textures[FBO_TEXTURE::DEPTH_STENCIL + p_texture_offset],
                                                  0);
    }
}

void AbstractRenderer::BlitMultisampledFramebuffer(const FBO_TEXTURE_BITFIELD_ENUM p_textures_bitmask, int32_t srcX0, int32_t srcY0, int32_t srcX1, int32_t srcY1, int32_t dstX0, int32_t dstY0, int32_t dstX1, int32_t dstY1)
{
    if(m_framebuffer_initialized)
    {
        if (m_msaa_count > 0)
        {
            QVector<GLenum> read_buffers = BindFBOToRead(p_textures_bitmask, true);
            BindFBOToDraw(p_textures_bitmask, false);

            size_t const read_buffers_size = read_buffers.size();
            for (size_t read_buffer_index = 0; read_buffer_index < read_buffers_size; ++read_buffer_index)
            {
                MathUtil::glFuncs->glReadBuffer(read_buffers[read_buffer_index]);
                MathUtil::glFuncs->glDrawBuffers(1, &read_buffers[read_buffer_index]);
                MathUtil::glFuncs->glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1,
                                                     dstX0, dstY0, dstX1, dstY1,
                                                     GL_COLOR_BUFFER_BIT, GL_NEAREST);
            }

            BindFBOToRead(p_textures_bitmask, false);
            BindFBOToDraw(p_textures_bitmask, true);
        }
    }
    else
    {
        qDebug("ERROR: AbstractRenderer::BlitMultisampledFramebuffer() called while framebuffer not initialized.");
    }
}

void AbstractRenderer::BlitMultisampledFramebuffer(const FBO_TEXTURE_BITFIELD_ENUM p_textures_bitmask)
{
    BlitMultisampledFramebuffer(p_textures_bitmask, 0, 0, m_window_width, m_window_height, 0, 0, m_window_width, m_window_height);
}

QVector<uint32_t> AbstractRenderer::BindFBOToRead(FBO_TEXTURE_BITFIELD_ENUM const p_textures_bitmask, bool const p_bind_multisampled/* = true*/)
{
    QVector<uint32_t> read_buffers;

    UpdateFramebuffer();
    if (m_framebuffer_initialized)
    {
        bool is_multisampled = (p_bind_multisampled) ? ((m_msaa_count > 0) ? true : false) : p_bind_multisampled;
        if ((is_multisampled && m_FBOs.size() >= 2) || (!is_multisampled && m_FBOs.size() >= 1))
        {
            GLuint const fbo = m_FBOs[(is_multisampled) ? 1 : 0];
            //MathUtil::glFuncs->glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

            read_buffers.reserve(FBO_TEXTURE::COUNT);
            GLenum const texture_type = (is_multisampled) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
            GLenum const framebuffer_target = GL_READ_FRAMEBUFFER;
            size_t const texture_offset = (is_multisampled) ? FBO_TEXTURE::COUNT : 0;

            BindFBOAndTextures(read_buffers, texture_type, framebuffer_target, fbo, texture_offset, p_textures_bitmask);

            MathUtil::glFuncs->glReadBuffer((read_buffers.size() != 0) ? read_buffers[0] : GL_COLOR_ATTACHMENT0);
        }
    }
    else
    {
        qDebug("ERROR: AbstractRenderer::BindFBOToRead() called while not configured.");
    }

    return read_buffers;
}

QVector<uint32_t> AbstractRenderer::BindFBOToDraw(FBO_TEXTURE_BITFIELD_ENUM const p_textures_bitmask, bool const p_bind_multisampled/* = true*/)
{
    QVector<uint32_t> draw_buffers;

    UpdateFramebuffer();
    if (m_framebuffer_initialized)
    {
        bool is_multisampled = (p_bind_multisampled) ? ((m_msaa_count > 0) ? true : false) : p_bind_multisampled;
        if ((is_multisampled && m_FBOs.size() >= 2) || (!is_multisampled && m_FBOs.size() >= 1))
        {
            GLuint const fbo = m_FBOs[(is_multisampled) ? 1 : 0];
            draw_buffers.reserve(FBO_TEXTURE::COUNT);
            GLenum const texture_type = (is_multisampled) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
            GLenum const framebuffer_target = GL_DRAW_FRAMEBUFFER;
            size_t const texture_offset = (is_multisampled) ? FBO_TEXTURE::COUNT : 0;

            BindFBOAndTextures(draw_buffers, texture_type, framebuffer_target, fbo, texture_offset, p_textures_bitmask);

            size_t const draw_buffers_size = draw_buffers.size();
            if (draw_buffers_size == 0)
            {
                GLenum buf = GL_NONE;
                MathUtil::glFuncs->glDrawBuffers(1, &buf);
            }
            else
            {
                MathUtil::glFuncs->glDrawBuffers(static_cast<GLsizei>(draw_buffers_size), draw_buffers.data());
            }
        }
    }
    else
    {
        qDebug("ERROR: FramebufferManager::bindFBOToDraw() called while not configured.");
    }

    return draw_buffers;
}

void AbstractRenderer::CacheUniformLocations(GLuint p_program, QVector<QVector<GLint>> * const p_map)
{
    GLint loc = -1;

    if ((*p_map).size() <= p_program)
    {
        (*p_map).resize(p_program + 128); // Allocate 128 here so that we aren't reallocating frequently
    }
    (*p_map)[p_program] = QVector<GLint>(count, -1);

    // Frame Uniforms
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iLeftEye");
    (*p_map)[p_program][ShaderUniformEnum::iLeftEye] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iResolution");
    (*p_map)[p_program][ShaderUniformEnum::iResolution] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iGlobalTime");
    (*p_map)[p_program][ShaderUniformEnum::iGlobalTime] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iViewportCount");
    (*p_map)[p_program][ShaderUniformEnum::iViewportCount] = loc;

    // Room Uniforms
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iMiscRoomData");
    (*p_map)[p_program][ShaderUniformEnum::iMiscRoomData] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iPlayerPosition");
    (*p_map)[p_program][ShaderUniformEnum::iPlayerPosition] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iUseClipPlane");
    (*p_map)[p_program][ShaderUniformEnum::iUseClipPlane] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iClipPlane");
    (*p_map)[p_program][ShaderUniformEnum::iClipPlane] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iFogEnabled");
    (*p_map)[p_program][ShaderUniformEnum::iFogEnabled] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iFogMode");
    (*p_map)[p_program][ShaderUniformEnum::iFogMode] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iFogDensity");
    (*p_map)[p_program][ShaderUniformEnum::iFogDensity] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iFogStart");
    (*p_map)[p_program][ShaderUniformEnum::iFogStart] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iFogEnd");
    (*p_map)[p_program][ShaderUniformEnum::iFogEnd] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iFogCol");
    (*p_map)[p_program][ShaderUniformEnum::iFogCol] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iRoomMatrix");
    (*p_map)[p_program][ShaderUniformEnum::iRoomMatrix] = loc;

    // Object Uniforms
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iMiscObjectData");
    (*p_map)[p_program][ShaderUniformEnum::iMiscObjectData] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iConstColour");
    (*p_map)[p_program][ShaderUniformEnum::iConstColour] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iChromaKeyColour");
    (*p_map)[p_program][ShaderUniformEnum::iChromaKeyColour] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iUseFlags");
    (*p_map)[p_program][ShaderUniformEnum::iUseFlags] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iModelMatrix");
    (*p_map)[p_program][ShaderUniformEnum::iModelMatrix] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iViewMatrix");
    (*p_map)[p_program][ShaderUniformEnum::iViewMatrix] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iProjectionMatrix");
    (*p_map)[p_program][ShaderUniformEnum::iProjectionMatrix] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iInverseViewMatrix");
    (*p_map)[p_program][ShaderUniformEnum::iInverseViewMatrix] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iModelViewMatrix");
    (*p_map)[p_program][ShaderUniformEnum::iModelViewMatrix] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iModelViewProjectionMatrix");
    (*p_map)[p_program][ShaderUniformEnum::iModelViewProjectionMatrix] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iTransposeInverseModelMatrix");
    (*p_map)[p_program][ShaderUniformEnum::iTransposeInverseModelMatrix] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iTransposeInverseModelViewMatrix");
    (*p_map)[p_program][ShaderUniformEnum::iTransposeInverseModelViewMatrix] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iSkelAnimJoints");
    (*p_map)[p_program][ShaderUniformEnum::iSkelAnimJoints] = loc;

    // Material Uniforms
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iAmbient");
    (*p_map)[p_program][ShaderUniformEnum::iAmbient] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iDiffuse");
    (*p_map)[p_program][ShaderUniformEnum::iDiffuse] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iSpecular");
    (*p_map)[p_program][ShaderUniformEnum::iSpecular] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iShininess");
    (*p_map)[p_program][ShaderUniformEnum::iShininess] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iEmission");
    (*p_map)[p_program][ShaderUniformEnum::iEmission] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iLightmapScale");
    (*p_map)[p_program][ShaderUniformEnum::iLightmapScale] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iTiling");
    (*p_map)[p_program][ShaderUniformEnum::iTiling] = loc;
    loc = MathUtil::glFuncs->glGetUniformLocation(p_program, "iUseTexture");
    (*p_map)[p_program][ShaderUniformEnum::iUseTexture] = loc;


    for (uint8_t i = 0 ; i < ASSETSHADER_NUM_TEXTURES; ++i)
    {
        QString uniformName = "iTexture" + QString::number(i);
        uint8_t tex_enum = (uint8_t)ShaderUniformEnum::iTexture0 + i;
        GLint tex_loc = MathUtil::glFuncs->glGetUniformLocation(p_program, uniformName.toStdString().c_str());
        (*p_map)[p_program][tex_enum] = tex_loc;

        if(tex_loc != -1)
        {
            MathUtil::glFuncs->glUniform1i(tex_loc, static_cast<GLint>(i));
        }
    }

    for (uint8_t i = 0 ; i < ASSETSHADER_NUM_CUBEMAPS; ++i)
    {
        QString uniformName = "iCubeTexture" + QString::number(i);
        uint8_t cube_tex_enum = (uint8_t)ShaderUniformEnum::iCubeTexture0 + i;
        GLint cube_tex_loc = MathUtil::glFuncs->glGetUniformLocation(p_program, uniformName.toStdString().c_str());
        (*p_map)[p_program][cube_tex_enum] = cube_tex_loc;

        if(cube_tex_loc != -1)
        {
            MathUtil::glFuncs->glUniform1i(cube_tex_loc, static_cast<GLint>(i) + static_cast<GLint>(ASSETSHADER_NUM_TEXTURES));
        }
    }

    QString uniformName = "iTexture10";
    GLint hbao_tex_loc = MathUtil::glFuncs->glGetUniformLocation(p_program, uniformName.toStdString().c_str());
    if (hbao_tex_loc != -1)
    {
        MathUtil::glFuncs->glUniform1i(hbao_tex_loc, 14);
    }
}

std::shared_ptr<ProgramHandle> AbstractRenderer::GetDefaultObjectShaderProgram()
{
    return m_default_object_shader;
}

std::shared_ptr<ProgramHandle> AbstractRenderer::GetDefaultSkyboxShaderProgram()
{
    return m_default_skybox_shader;
}

std::shared_ptr<ProgramHandle> AbstractRenderer::GetDefaultPortalShaderProgram()
{
    return m_default_portal_shader;
}
