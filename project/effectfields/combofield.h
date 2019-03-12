#ifndef COMBOFIELD_H
#define COMBOFIELD_H

#include "effectfield.h"

struct ComboFieldItem {
  QString name;
  QVector<QVariant> data;
};

class ComboField : public EffectField
{
  Q_OBJECT
public:
  ComboField(EffectRow* parent, const QString& id);

  void AddItem(const QString& text, const QVariant& data);

  virtual QVariant ConvertStringToValue(const QString& s);
  virtual QString ConvertValueToString(const QVariant& v);

signals:
  void IndexChanged(int i);

private:
  QVector<ComboFieldItem> items_;
};

#endif // COMBOFIELD_H
