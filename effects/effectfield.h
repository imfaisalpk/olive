/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef EFFECTFIELD_H
#define EFFECTFIELD_H

#include <QObject>
#include <QVariant>
#include <QVector>

#include "effects/keyframe.h"
#include "undo/undostack.h"
#include "nodes/nodedatatypes.h"
#include "global/rational.h"

class NodeIO;
class ComboAction;

/**
 * @brief The EffectField class
 *
 * Any user-interactive element of an Effect. Usually a parameter that modifies the effect output, but sometimes just
 * a UI object that performs some other function (e.g. LabelField and ButtonField).
 *
 * EffectField provides a largely abstract interface for Effect classes to pull information from. The class itself
 * handles keyframing between linear, bezier, and hold interpolation accessible through GetValueAt(). This class
 * is abstract, and therefore never intended to be used on its own. Instead you should always use a derived class.
 *
 * EffectField objects are *not* UI objects on their own. Instead, they're largely a system of values that can change
 * over time. For a widget that the user can use to edit/modify these values, use CreateWidget().
 *
 * Derived classes are expected to override at least CreateWidget() to create a visual interactive widget corresponding
 * to the field. If this
 * field is a value used in the Effect (as most will be), UpdateWidgetValue() should also be overridden to display the
 * correct value for this field as the user moves around the Timeline.
 *
 * If the field is intended to be saved and loaded from Olive project files (as most will be), ConvertStringToValue()
 * and ConvertValueToString() may also have to be overridden depending on how the derived class's data works.
 */
class EffectField : public QObject {
  Q_OBJECT
public:

  /**
   * @brief The EffectFieldType enum
   *
   * Predetermined types of fields. Used throughout Olive to identify what kind of data to expect from GetValueAt().
   *
   * This enum is also currently used to match an external XML effect's fields with the correct derived class (e.g.
   * EFFECT_FIELD_DOUBLE matches to DoubleField).
   */
  enum EffectFieldType {
    /** Values are doubles. Also corresponds to DoubleField. */
    EFFECT_FIELD_DOUBLE,

    /** Values are colors. Also corresponds to ColorField. */
    EFFECT_FIELD_COLOR,

    /** Values are strings. Also corresponds to StringField. */
    EFFECT_FIELD_STRING,

    /** Values are booleans. Also corresponds to BoolField. */
    EFFECT_FIELD_BOOL,

    /** Values are arbitrary data. Also corresponds to ComboField. */
    EFFECT_FIELD_COMBO,

    /** Values are font family names (in string). Also corresponds to FontField. */
    EFFECT_FIELD_FONT,

    /** Values are filenames (in string). Also corresponds to FileField. */
    EFFECT_FIELD_FILE,

    /** Values is a UI object with no data. Corresponds to nothing. */
    EFFECT_FIELD_UI
  };

  /**
   * @brief EffectField Constructor
   *
   * Creates a new EffectField object.
   *
   * @param parent
   *
   * The EffectRow to add this field to. This must be a valid EffectRow. The EffectRow takes ownership of the field
   * using the QObject parent/child system to automate memory management. EffectFields are never expected
   * to change parent during their lifetime.
   *
   * @param t
   *
   * The type of data contained within this field. This is expected to be filled by a derived class.
   */
  EffectField(NodeIO* parent, EffectFieldType t);

  /**
   * @brief Get the EffectRow that this field is a member of.
   *
   * Equivalent to `static_cast<EffectRow*>(EffectField::parent())`
   *
   * @return
   *
   * The EffectRow that this field is a member of.
   */
  NodeIO* GetParentRow();

  /**
   * @brief Get the type of data to expect from this field
   *
   * @return
   *
   * A member of the EffectFieldType enum.
   */
  const EffectFieldType& type();

  /**
   * @brief Get the value of this field at a given timecode
   *
   * EffectFields are designed to be keyframable, meaning the user can make the values change over the course of the
   * Sequence. This is the main function used through Olive to retrieve what value this field will be at a given time.
   *
   * A common use case for this function would be EffectField::GetValueAt(EffectField::Now()), which will automatically
   * retrieve the timecode at the current playhead.
   *
   * If the parent EffectRow is NOT keyframing, this function will simply return persistent_data_. If it IS keyframing,
   * this will use the values in `keyframes` to determine what value should be specifically at this time.
   * (bezier or linear interpolating it between values if necessary). Therefore this function should almost always be
   * used to retrieve data from this field as the value will always be correct for the given time.
   *
   * @param timecode
   *
   * The time to retrieve the value in clip/media seconds (e.g. 0.0 is the very start of the media, 1.0 is one second
   * into the media).
   *
   * @return
   *
   * A QVariant representation of the value at the given timecode.
   */
  QVariant GetValueAt(const rational &timecode);

  /**
   * @brief Set the value of this field at a given timecode
   *
   * EffectFields are designed to be keyframable, meaning the user can make the values change over the course of the
   * Sequence. This is the main function used through Olive to set what value this field will be at a given time.
   *
   * If the parent EffectRow is keyframing, this function will determine whether a keyframe exists at this time already.
   * If it does, it will change the value at that keyframe to `value`. Otherwise, it'll create a new keyframe at the
   * specified `time` with the specified `value`.
   *
   * If the parent EffectRow is not keyframing, the data is simply stored in `persistent_data_`.
   *
   * When constructing an Effect
   *
   * @param time
   *
   * The time to retrieve the value at in clip/media seconds (e.g. 0.0 is the very start of the media, 1.0 is one
   * second into the media).
   *
   * @param value
   *
   * The QVariant value to set at this time.
   */
  void SetValueAt(const rational& time, const QVariant& value);

  /**
   * @brief Set up keyframing on this field
   *
   * This should always be called if the user is enabling/disabling keyframing on the parent row. This function will
   * move data between persistent_data_ and keyframes depending on whether keyframing is being enabled or disabled.
   *
   * If keyframing is getting ENABLED, this function will create the first keyframe automatically at the current time
   * using the current value in persistent_data_.
   *
   * If keyframing is getting DISABLED, persistent_data_ is set to the current value at this time (GetValueAt(Now()))
   * and delete all current keyframes.
   *
   * @param enabled
   *
   * TRUE if keyframing is getting enabled.
   *
   * @param ca
   *
   * A valid ComboAction object. It's expected that this function will be part of a larger action to enable/disable
   * keyframing on the parent EffectRow, so this function will add commands to this ComboAction.
   */
  void PrepareDataForKeyframing(bool enabled, ComboAction* ca);

  /**
   * @brief Convert a value from this field to a string
   *
   * When saving effect data to a project file, the data needs to be converted to a string format for saving in XML.
   * The needs of this string representation may differ depending on the needs of the derived class, therefore you
   * derived classes may need to override it.
   *
   * Default behavior is a simple QVariant <-> QString conversion, which should suffice in most cases.
   *
   * @param v
   *
   * The QVariant data (retrieved from this field) to convert to string
   *
   * @return
   *
   * A string representation of the QVariant data provided.
   */
  virtual QString ConvertValueToString(const QVariant& v);

  /**
   * @brief Convert a string to a value appropriate for this field
   *
   * This function is the inverse of ConvertValueToString(), converting a string back to field data.
   *
   * @param s
   *
   * The string to convert to data.
   *
   * @return
   *
   * QVariant data converted from the provided string.
   */
  virtual QVariant ConvertStringToValue(const QString& s);


  /**
   * @brief Create a widget for the user to interact with this field
   *
   * EffectField objects are *not* UI objects on their own. Instead, they're largely a system of values that can change
   * over time. This function creates a QWidget object that can be placed somewhere in the UI so the user can
   * interact with and change the data in this field.
   *
   * This function must be overridden by derived classes in order to create a widget that appropriate for that field's
   * data. The derived class is also responsible for
   * connecting signals like EnabledChanged(), Clicked(), and any other data that needs to be transferred between the
   * widget and the field (setting up the signals and slots to do so). The field does NOT retain ownership (or any
   * reference for that matter) to widgets it creates,
   * so keeping the widget and field up to date with each other relies solely on setting up signals and slots.
   * Infinite widgets can be created from a single field and used throughout Olive this way.
   *
   * Ownership is passed to the caller, and therefore the caller is responsible for freeing it.
   *
   * @param existing
   *
   * Olive allows multiple effects to attach to one UI layout. Pass a QWidget to this parameter (instead of nullptr)
   * to attach this field additionally to the widget's signals/slots without creating a new one. The QWidget must be a
   * widget previously created from the same derived class type or the result is undefined.
   *
   * @return
   *
   * A new QWidget object for this EffectField, or the same QWidget passed to `existing` if one was specified.
   */
  virtual QWidget* CreateWidget(QWidget* existing = nullptr) = 0;

  /**
   * @brief Update a widget created by CreateWidget() using the value at a given time
   *
   * Use this function to update a QWidget (obtained from CreateWidget()) with the correct value from the field at a
   * given time.
   *
   * Since only the derived classes know what type of QWidget it created in CreateWidget() and how to work with them,
   * derived classes are also expected to override this function if the field is an active value used in the Effect
   * that should visually update as the user moves around the Timeline. However if the field does NOT need to update
   * live (e.g. the field is just a UI wrapper like LabelField or ButtonField), this function does not need to be
   * overridden as the default behavior (to do nothing) will suffice in those cases.
   *
   * @param widget
   *
   * The QWidget to set the value of (must be a QWidget obtained from CreateWidget() or the behavior is undefined).
   *
   * @param timecode
   *
   * The time in clip/media seconds to retrieve data from.
   */
  virtual void UpdateWidgetValue(QWidget* widget, const rational& timecode);

  /**
   * @brief Get the correct X position/time value of a bezier keyframe's handles
   *
   * Retrieves the X value (time value) of a bezier keyframe's handles. Internally, the handles' X values are allowed
   * to be arbitrary values. This however can lead to inadvertently creating impossible bezier curves (ones that, for
   * example, mathematically loop over each other, but obviously a field can't have two values at the same time).
   *
   * This function returns the keyframe handles' X values adjusted to prevent this from happening. All calculations
   * are consistent (i.e. the post handle of one keyframe will be adjusted the same way as the pre handle of the
   * keyframe before it). It's recommended to always use this function to retrieve keyframe handle X values.
   *
   * @param key
   *
   * Index of the keyframe (in `keyframes`) to retrieve the handle position from.
   *
   * @param post
   *
   * FALSE to retrieve the "pre" handle (handle to the left of the keyframe), TRUE to retrieve the "post" handle
   * (handle to the right of the keyframe).
   *
   * @return
   *
   * The adjusted X value of that keyframe handle.
   */
  double GetValidKeyframeHandlePosition(int key, bool post);

  /**
   * @brief Return whether this field is enabled or not
   *
   * @return
   *
   * TRUE if this field is enabled.
   */
  bool IsEnabled();

  /**
   * @brief Set the enabled state of this field
   * @param e
   *
   * TRUE to enable this field, FALSE to disable it.
   */
  void SetEnabled(bool e);

  /**
   * @brief Persistent data object
   *
   * If the parent EffectRow is not keyframing, all field data is stored and retrieved here. If the row IS keyframing,
   * this variable goes basically unused unless `keyframes` is empty.
   *
   * NOTE: It is NOT recommended to access this variable directly. Use GetValueAt() instead.
   */
  QVariant persistent_data_;

  /**
   * @brief Keyframe array
   *
   * Contains all data about this field's keyframes, from the keyframe times, to their data, to their type (linear,
   * bezier, or hold), to the bezier handles (if using bezier). If the row is not keyframing, this array is never used
   * (`persistent_data_` is used instead). If it is, this array will always be used unless the array is empty, in which
   * case `persistent_data_` will be used again.
   */
  QVector<EffectKeyframe> keyframes;

signals:
  /**
   * @brief Changed signal
   *
   * Emitted whenever SetValueAt() is called in order to trigger a UI update and Viewer repaint. Note this is NOT
   * triggered as the value changes from keyframing. Only when the user themselves triggers a change.
   */
  void Changed();

  /**
   * @brief Clicked signal
   *
   * Emitted when the user clicks on a QWidget attached to this field. Derived classes should connect this to the
   * clicked signal of any QWidget's created (or attached) in CreateWidget().
   */
  void Clicked();

  /**
   * @brief Enable change state signal
   *
   * Emitted when the field's enabled state is changed through SetEnabled(). Derived classes should connect this to the
   * setEnabled() slot of any QWidget's created (or attached) in CreateWidget().
   */
  void EnabledChanged(bool);

private:
  /**
   * @brief Internal type variable set in the constructor. Access with type().
   */
  EffectFieldType type_;

  /**
   * @brief Used by GetValueAt() to determine whether to use keyframe data or persistent data
   * @return
   *
   * TRUE if this keyframe data should be retrieved, FALSE if persistent data should be retrieved
   */
  bool HasKeyframes();

  /**
   * @brief Internal function for determining where we are between the available keyframes
   *
   * @param timecode
   *
   * Timecode to get keyframe data at
   *
   * @param before
   *
   * The index (in the keyframes array) in the keyframe prior to this timecode.
   *
   * @param after
   *
   * The index (in the keyframes array) in the keyframe after this timecode.
   *
   * @param d
   *
   * The progress between the `before` keyframe and `after` keyframe from 0.0 to 1.0.
   */
  void GetKeyframeData(const rational &timecode, int& before, int& after, double& d);

  /**
   * @brief Internal enabled value
   */
  bool enabled_;

};

#endif // EFFECTFIELD_H
