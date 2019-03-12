#ifndef COLORFIELD_H
#define COLORFIELD_H

#include "effectfield.h"

class ColorField : public EffectField
{
  Q_OBJECT
public:
  ColorField(EffectRow* parent, const QString& id);

  QColor GetColorAt(double timecode);

  virtual QVariant ConvertStringToValue(const QString& s);
  virtual QString ConvertValueToString(const QVariant& v);
};

#endif // COLORFIELD_H
