#ifndef TEXTGEOM_H
#define TEXTGEOM_H

#include <QtGui>

#include "mathutil.h"
#include "assetshader.h"
#include "texturemanager.h"
#include "assetimage.h"

struct TextGeomLine
{
    TextGeomLine():
        glyph_count(0)
    {
    }

    QString text;
    uint32_t glyph_count;
    QColor col;
    QVector<float> m_positions;
    QVector<float> m_texcoords;
    QVector<uint32_t> m_indices;
    std::shared_ptr<MeshHandle> m_mesh_handle;
    std::shared_ptr<BufferHandle> m_position_handle;
    std::shared_ptr<BufferHandle> m_texcoord_handle;
    std::shared_ptr<BufferHandle> m_index_handle;
};

class TextGeom : public QObject
{
public:

    TextGeom();
    ~TextGeom();

    static void initializeGL();

    void SetMaxSize(float x, float y);
    float GetMaxSizeX() const;
    float GetMaxSizeY() const;

    void AddText(const QString & s, const QColor col = QColor(255,255,255));
    void SetText(const QString & s, const QColor col = QColor(255,255,255));
    QString GetText() const;
    float GetTextLength() const;   

    QVector <TextGeomLine> GetAllText() const;

    void SetColour(const QColor & c);
    QColor GetColour() const;

    void SetFixedSize(const bool b, const float f);
    float GetFixedSize() const;

    void SetLeftJustify(const bool b);
    bool GetLeftJustify() const;

    float GetScale() const;
    float GetHeight() const;

    void DrawGL(QPointer <AssetShader> shader);
    void DrawSelectedGL(QPointer <AssetShader> shader);

    QMatrix4x4 GetModelMatrix() const;

    void Clear();   

private:

    void CreateVBO();    

    QVector <TextGeomLine> texts;
    QVector <TextGeomLine> last_texts;

    bool do_fixed_size;
    float fixed_size;

    float maxx;
    float maxy;
    float len;
    float height;    

    QVector3D pos;
    QVector3D dir;

    bool vbo_rebuild;
	bool left_justify;
};

#endif // TEXTGEOM_H
