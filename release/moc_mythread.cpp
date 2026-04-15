/****************************************************************************
** Meta object code from reading C++ file 'mythread.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.14.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../mythread.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mythread.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.14.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MyThread_t {
    QByteArrayData data[16];
    char stringdata0[175];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MyThread_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MyThread_t qt_meta_stringdata_MyThread = {
    {
QT_MOC_LITERAL(0, 0, 8), // "MyThread"
QT_MOC_LITERAL(1, 9, 16), // "signal_messImage"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 8), // "cv::Mat*"
QT_MOC_LITERAL(4, 36, 5), // "image"
QT_MOC_LITERAL(5, 42, 23), // "signal_sendForDetection"
QT_MOC_LITERAL(6, 66, 10), // "cv::Rect2d"
QT_MOC_LITERAL(7, 77, 4), // "rect"
QT_MOC_LITERAL(8, 82, 17), // "signal_cleanlabel"
QT_MOC_LITERAL(9, 100, 20), // "signal_boxesSelected"
QT_MOC_LITERAL(10, 121, 12), // "detectionBox"
QT_MOC_LITERAL(11, 134, 11), // "trackingBox"
QT_MOC_LITERAL(12, 146, 8), // "received"
QT_MOC_LITERAL(13, 155, 4), // "data"
QT_MOC_LITERAL(14, 160, 12), // "receiveangle"
QT_MOC_LITERAL(15, 173, 1) // "a"

    },
    "MyThread\0signal_messImage\0\0cv::Mat*\0"
    "image\0signal_sendForDetection\0cv::Rect2d\0"
    "rect\0signal_cleanlabel\0signal_boxesSelected\0"
    "detectionBox\0trackingBox\0received\0"
    "data\0receiveangle\0a"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MyThread[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   44,    2, 0x06 /* Public */,
       5,    2,   47,    2, 0x06 /* Public */,
       8,    0,   52,    2, 0x06 /* Public */,
       9,    2,   53,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    1,   58,    2, 0x0a /* Public */,
      14,    1,   61,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3, 0x80000000 | 6,    4,    7,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6, 0x80000000 | 6,   10,   11,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void, QMetaType::Int,   15,

       0        // eod
};

void MyThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MyThread *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->signal_messImage((*reinterpret_cast< cv::Mat*(*)>(_a[1]))); break;
        case 1: _t->signal_sendForDetection((*reinterpret_cast< cv::Mat*(*)>(_a[1])),(*reinterpret_cast< cv::Rect2d(*)>(_a[2]))); break;
        case 2: _t->signal_cleanlabel(); break;
        case 3: _t->signal_boxesSelected((*reinterpret_cast< cv::Rect2d(*)>(_a[1])),(*reinterpret_cast< cv::Rect2d(*)>(_a[2]))); break;
        case 4: _t->received((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 5: _t->receiveangle((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MyThread::*)(cv::Mat * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MyThread::signal_messImage)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MyThread::*)(cv::Mat * , cv::Rect2d );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MyThread::signal_sendForDetection)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MyThread::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MyThread::signal_cleanlabel)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MyThread::*)(cv::Rect2d , cv::Rect2d );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MyThread::signal_boxesSelected)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MyThread::staticMetaObject = { {
    QMetaObject::SuperData::link<QThread::staticMetaObject>(),
    qt_meta_stringdata_MyThread.data,
    qt_meta_data_MyThread,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MyThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MyThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MyThread.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int MyThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void MyThread::signal_messImage(cv::Mat * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MyThread::signal_sendForDetection(cv::Mat * _t1, cv::Rect2d _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void MyThread::signal_cleanlabel()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void MyThread::signal_boxesSelected(cv::Rect2d _t1, cv::Rect2d _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
