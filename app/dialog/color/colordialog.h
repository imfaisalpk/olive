#ifndef COLORDIALOG_H
#define COLORDIALOG_H

#include <QDialog>

#include "colorvalueswidget.h"
#include "render/color.h"
#include "render/colormanager.h"
#include "widget/colorwheel/colorwheelwidget.h"
#include "widget/colorwheel/colorgradientwidget.h"

class ColorDialog : public QDialog
{
  Q_OBJECT
public:
  ColorDialog(ColorManager* color_manager, Color start = Color(1.0f, 1.0f, 1.0f), QWidget* parent = nullptr);

  const Color& GetSelectedColor() const;

private:
  ColorManager* color_manager_;

  ColorWheelWidget* color_wheel_;

  ColorGradientWidget* hsv_value_gradient_;

};

#endif // COLORDIALOG_H