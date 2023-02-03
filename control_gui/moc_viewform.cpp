/****************************************************************************
** Meta object code from reading C++ file 'viewform.h'
**
** Created by: The Qt Meta Object Compiler version 63 (Qt 4.8.7)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "viewform.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'viewform.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.7. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ViewForm[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      10,    9,    9,    9, 0x08,
      34,    9,    9,    9, 0x08,
      61,    9,    9,    9, 0x08,
      86,    9,    9,    9, 0x08,
     109,    9,    9,    9, 0x08,
     142,  136,    9,    9, 0x08,
     181,    9,    9,    9, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ViewForm[] = {
    "ViewForm\0\0slotBtnConnectPressed()\0"
    "slotBtnDeviceListPressed()\0"
    "slotBtnChangeIpPressed()\0"
    "slotBtnStreamPressed()\0"
    "slotBtnTakeScreenPressed()\0index\0"
    "slotDeviceListItemClicked(QModelIndex)\0"
    "timerScreenRefreshSlot()\0"
};

void ViewForm::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        ViewForm *_t = static_cast<ViewForm *>(_o);
        switch (_id) {
        case 0: _t->slotBtnConnectPressed(); break;
        case 1: _t->slotBtnDeviceListPressed(); break;
        case 2: _t->slotBtnChangeIpPressed(); break;
        case 3: _t->slotBtnStreamPressed(); break;
        case 4: _t->slotBtnTakeScreenPressed(); break;
        case 5: _t->slotDeviceListItemClicked((*reinterpret_cast< QModelIndex(*)>(_a[1]))); break;
        case 6: _t->timerScreenRefreshSlot(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData ViewForm::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject ViewForm::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_ViewForm,
      qt_meta_data_ViewForm, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ViewForm::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ViewForm::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ViewForm::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ViewForm))
        return static_cast<void*>(const_cast< ViewForm*>(this));
    return QWidget::qt_metacast(_clname);
}

int ViewForm::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
