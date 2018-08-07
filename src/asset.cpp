#include "asset.h"

Asset::Asset()
{        
    props = new DOMNode();
    SetS("_type", "asset");        
//    qDebug() << "Asset::Asset()" << props;
}

void Asset::SetV(const char * name, const QVector3D p)
{
    if (props) {
        props->SetV(name, p);
    }
}

QVector3D Asset::GetV(const char * name) const
{
    if (props) {
        return props->GetV(name);
    }
    return QVector3D();
}

void Asset::SetV4(const char * name, const QVector4D p)
{
    if (props) {
        props->SetV4(name, p);
    }

}

QVector4D Asset::GetV4(const char * name) const
{
    if (props) {
        return props->GetV4(name);
    }
    return QVector4D();
}

void Asset::SetF(const char * name, const float f)
{
    if (props) {
        props->setProperty(name, QString::number(f));
    }
}

float Asset::GetF(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return props->property(name).toFloat();
    }
    return 0.0f;
}

void Asset::SetI(const char * name, const int i)
{
    if (props) {
        props->setProperty(name, QString::number(i));
    }
}

int Asset::GetI(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return props->property(name).toInt();
    }
    return 0;
}

void Asset::SetB(const char * name, const bool b)
{
    if (props) {
        props->setProperty(name, b ? "true" : "false");
    }
}

bool Asset::GetB(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return props->property(name).toString().toLower() == "true";
    }
    return false;
}

void Asset::SetS(const char * name, const QString s)
{
    if (props) {
        props->setProperty(name, s);
    }
}

QString Asset::GetS(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return props->property(name).toString();
    }
    return QString();
}

void Asset::SetC(const char * name, const QColor c)
{
    if (props) {
        props->setProperty(name, MathUtil::GetColourAsString(c, false));
    }
}

QColor Asset::GetC(const char * name) const
{
    if (props && props->property(name).isValid()) {
        return MathUtil::GetStringAsColour(props->property(name).toString());
    }
    return QColor(255,255,255);
}

void Asset::SetSrc(const QString & base, const QString & src_str)
{
//    qDebug() << "Asset::SetSrc" << base << src_str;
    SetS("_base_url", base);
    if (src_str.right(5) == "data:") {
        SetS("src", src_str);
        SetS("_src_url", "");
        SetURL(src_str);
    }
    else {
        SetS("src", src_str.trimmed());
        SetS("_src_url", QUrl(base).resolved(src_str).toString());
        SetURL(GetS("_src_url"));
    }
}

void Asset::SetProperties(const QVariantMap & d)
{
    QVariantMap::const_iterator it;
    for (it=d.begin(); it!=d.end(); ++it) {
//        qDebug() << " " << it.key() << it.value().toString();
        if (!it.key().isEmpty()) {
            props->setProperty(it.key().trimmed().toLower().toLatin1().data(), it.value());
        }
    }
}

QString Asset::GetXMLCode() const
{
    QString s = QString("<") + GetS("_type");
    QList <QByteArray> p = props->dynamicPropertyNames();
    for (int i=0; i<p.size(); ++i) {
        if (props->GetSaveAttribute(p[i].data(), false)) {
            s += QString(" ") + QString(p[i]) + "=\"" + props->property(p[i]).toString() + "\"";
        }
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
