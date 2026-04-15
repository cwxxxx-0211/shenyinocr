/****************************************************************************
** Meta object code from reading C++ file 'CameraThread.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.14.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../CameraThread.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CameraThread.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.14.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CameraThread_t {
    QByteArrayData data[19];
    char stringdata0[211];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CameraThread_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CameraThread_t qt_meta_stringdata_CameraThread = {
    {
QT_MOC_LITERAL(0, 0, 12), // "CameraThread"
QT_MOC_LITERAL(1, 13, 10), // "imageReady"
QT_MOC_LITERAL(2, 24, 0), // ""
QT_MOC_LITERAL(3, 25, 4), // "Mat*"
QT_MOC_LITERAL(4, 30, 5), // "image"
QT_MOC_LITERAL(5, 36, 14), // "threadFinished"
QT_MOC_LITERAL(6, 51, 16), // "signal_messImage"
QT_MOC_LITERAL(7, 68, 8), // "cv::Mat*"
QT_MOC_LITERAL(8, 77, 23), // "signal_sendForDetection"
QT_MOC_LITERAL(9, 101, 10), // "cv::Rect2d"
QT_MOC_LITERAL(10, 112, 4), // "bbox"
QT_MOC_LITERAL(11, 117, 17), // "signal_cleanlabel"
QT_MOC_LITERAL(12, 135, 20), // "signal_boxesSelected"
QT_MOC_LITERAL(13, 156, 12), // "detectionBox"
QT_MOC_LITERAL(14, 169, 11), // "trackingBox"
QT_MOC_LITERAL(15, 181, 8), // "received"
QT_MOC_LITERAL(16, 190, 4), // "data"
QT_MOC_LITERAL(17, 195, 13), // "receiveangle1"
QT_MOC_LITERAL(18, 209, 1) // "a"

    },
    "CameraThread\0imageReady\0\0Mat*\0image\0"
    "threadFinished\0signal_messImage\0"
    "cv::Mat*\0signal_sendForDetection\0"
    "cv::Rect2d\0bbox\0signal_cleanlabel\0"
    "signal_boxesSelected\0detectionBox\0"
    "trackingBox\0received\0data\0receiveangle1\0"
    "a"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CameraThread[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   54,    2, 0x06 /* Public */,
       5,    0,   57,    2, 0x06 /* Public */,
       6,    1,   58,    2, 0x06 /* Public */,
       8,    2,   61,    2, 0x06 /* Public */,
      11,    0,   66,    2, 0x06 /* Public */,
      12,    2,   67,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      15,    1,   72,    2, 0x0a /* Public */,
      17,    1,   75,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7,    4,
    QMetaType::Void, 0x80000000 | 7, 0x80000000 | 9,    4,   10,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 9, 0x80000000 | 9,   13,   14,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,   16,
    QMetaType::Void, QMetaType::Int,   18,

       0        // eod
};

void CameraThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CameraThread *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->imageReady((*reinterpret_cast< Mat*(*)>(_a[1]))); break;
        case 1: _t->threadFinished(); break;
        case 2: _t->signal_messImage((*reinterpret_cast< cv::Mat*(*)>(_a[1]))); break;
        case 3: _t->signal_sendForDetection((*reinterpret_cast< cv::Mat*(*)>(_a[1])),(*reinterpret_cast< cv::Rect2d(*)>(_a[2]))); break;
        case 4: _t->signal_cleanlabel(); break;
        case 5: _t->signal_boxesSelected((*reinterpret_cast< cv::Rect2d(*)>(_a[1])),(*reinterpret_cast< cv::Rect2d(*)>(_a[2]))); break;
        case 6: _t->received((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 7: _t->receiveangle1((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CameraThread::*)(Mat * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CameraThread::imageReady)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CameraThread::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CameraThread::threadFinished)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CameraThread::*)(cv::Mat * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CameraThread::signal_messImage)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CameraThread::*)(cv::Mat * , cv::Rect2d );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CameraThread::signal_sendForDetection)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CameraThread::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CameraThread::signal_cleanlabel)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CameraThread::*)(cv::Rect2d , cv::Rect2d );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CameraThread::signal_boxesSelected)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CameraThread::staticMetaObject = { {
    QMetaObject::SuperData::link<QThread::staticMetaObject>(),
    qt_meta_stringdata_CameraThread.data,
    qt_meta_data_CameraThread,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CameraThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CameraThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CameraThread.stringdata0))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int CameraThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void CameraThread::imageReady(Mat * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CameraThread::threadFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void CameraThread::signal_messImage(cv::Mat * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CameraThread::signal_sendForDetection(cv::Mat * _t1, cv::Rect2d _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void CameraThread::signal_cleanlabel()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void CameraThread::signal_boxesSelected(cv::Rect2d _t1, cv::Rect2d _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
