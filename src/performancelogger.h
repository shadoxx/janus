#ifndef PERFORMANCELOGGER_H
#define PERFORMANCELOGGER_H

#ifdef __ANDROID__
//#define GLdouble float
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>
#else
#include <qopengl.h>
#include <qopenglext.h>
#endif

#include <time.h>
#include <QtCore>

#include "mathutil.h"
#include "rendererinterface.h"

struct PerformanceLoggerSample
{
    float render_thread_gpu_time;
    float render_thread_cpu_time;
};

class PerformanceLogger
{
public:
    PerformanceLogger();   

    void StartFrameSample();    
    void EndFrameSample();

    void SetGPUTimeQueryResults(QVector<uint64_t>& v);
    void SetCPUTimeQueryResults(QVector<uint64_t>& v);

    void SetNumFrameSamples(const int i);
    int GetNumFrameSamples() const;    
    TextureHandle* GetFrameSamplesTextureHandle();
    double GetAverageMainThreadCPUTime() const;
    double GetAverageRenderThreadCPUTime() const;
    double GetAverageRenderThreadGPUTime() const;    

private:   

    int samples_num;
    std::shared_ptr<TextureHandle> m_texture_handle;
    int64_t m_total_main_CPU_time;   

    // This is nanoseconds per frame
    double m_average_render_GPU_time;
    double m_average_render_CPU_time;
    double m_average_main_CPU_time;

    QElapsedTimer m_frame_time_timer;
    QVector <int64_t> main_thread_cpu_time_samples;

    static QVector <PerformanceLoggerSample> frame_samples;            
    static int max_frame_samples;
    static clock_t last_clock;
};

#endif // PERFORMANCELOGGER_H
