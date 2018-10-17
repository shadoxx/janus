#include "asset.h"

Asset::Asset()
{        
    props = new DOMNode();
    props->SetType(TYPE_ASSET);
//    qDebug() << "Asset::Asset()" << props;

    //60.0 - make js_id property invalid for Assets
    props->SetJSID("");
}

void Asset::SetSrc(const QString & base, const QString & src_str)
{
//    qDebug() << "Asset::SetSrc" << base << src_str;
    props->SetBaseURL(base);
    if (src_str.right(5) == "data:") {
        props->SetSrc(src_str);
        props->SetSrcURL("");
        SetURL(src_str);
    }
    else {
        props->SetSrc(src_str.trimmed());
        props->SetSrcURL(QUrl(base).resolved(src_str).toString());
//        qDebug() << " resolved" << QUrl(base).resolved(src_str).toString();
        SetURL(props->GetSrcURL());
    }
}

void Asset::SetProperties(const QVariantMap & d)
{
    props->SetProperties(d);
}

QString Asset::GetXMLCode() const
{
    QString s = QString("<") + DOMNode::ElementTypeToTagName(props->GetType());

    if (props->GetID().length() > 0) {
        s += " id=\"" + props->GetID() + "\"";
    }
    if (props->GetSrc().length() > 0) {
        s += " src=\"" + props->GetSrc() + "\"";
    }
    if (props->GetVertexSrc().length() > 0) {
        s += " vertex_src=\"" + props->GetVertexSrc() + "\"";
    }
    if (props->GetMTL().length() > 0) {
        s += " mtl=\"" + props->GetMTL() + "\"";
    }
    if (props->GetSBS3D()) {
        s += " sbs3d=\"true\"";
    }
    if (props->GetOU3D()) {
        s += " ou3d=\"true\"";
    }
    if (props->GetReverse3D()) {
        s += " reverse3d=\"true\"";
    }
    if (props->GetTexClamp()) {
        s += " tex_clamp=\"true\"";
    }
    if (!props->GetTexLinear()) {
        s += " tex_linear=\"false\"";
    }
    if (props->GetTexCompress()) {
        s += " tex_compress=\"true\"";
    }
    if (!props->GetTexPreMultiply()) {
        s += " tex_premultiply=\"false\"";
    }
    if (!props->GetTexMipmap()) {
        s += " tex_mipmap=\"false\"";
    }
    if (props->GetTexAlpha() != "undefined") {
        s += " tex_alpha=\"" + props->GetTexAlpha() + "\"";
    }
    if (props->GetTexColorspace() != "sRGB") {
        s += " tex_colorspace=\"" + props->GetTexColorspace() + "\"";
    }

    s += " />";
    return s;
}

QVariantMap Asset::GetJSONCode() const
{
    QVariantMap m;
    QList <QByteArray> p = props->dynamicPropertyNames();
    for (int i=0; i<p.size(); ++i) {
        if (props->GetSaveAttribute(p[i].data(), false)) {
            m[p[i]] = props->property(p[i]);
        }
    }
    return m;
}

QPointer <DOMNode> Asset::GetProperties()
{
    return props;
}
