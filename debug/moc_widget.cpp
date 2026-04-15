/****************************************************************************
** Meta object code from reading C++ file 'widget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.14.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../widget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'widget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.14.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Widget_t {
    QByteArrayData data[85];
    char stringdata0[1305];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Widget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Widget_t qt_meta_stringdata_Widget = {
    {
QT_MOC_LITERAL(0, 0, 6), // "Widget"
QT_MOC_LITERAL(1, 7, 12), // "captureFrame"
QT_MOC_LITERAL(2, 20, 0), // ""
QT_MOC_LITERAL(3, 21, 3), // "Mat"
QT_MOC_LITERAL(4, 25, 5), // "image"
QT_MOC_LITERAL(5, 31, 10), // "sendDataTo"
QT_MOC_LITERAL(6, 42, 5), // "pipei"
QT_MOC_LITERAL(7, 48, 8), // "imgmuban"
QT_MOC_LITERAL(8, 57, 4), // "Mat*"
QT_MOC_LITERAL(9, 62, 3), // "img"
QT_MOC_LITERAL(10, 66, 9), // "imgshibie"
QT_MOC_LITERAL(11, 76, 6), // "kernal"
QT_MOC_LITERAL(12, 83, 1), // "n"
QT_MOC_LITERAL(13, 85, 12), // "jiancestring"
QT_MOC_LITERAL(14, 98, 6), // "String"
QT_MOC_LITERAL(15, 105, 13), // "targetstring1"
QT_MOC_LITERAL(16, 119, 13), // "caijianchicun"
QT_MOC_LITERAL(17, 133, 9), // "width_min"
QT_MOC_LITERAL(18, 143, 9), // "width_max"
QT_MOC_LITERAL(19, 153, 10), // "height_min"
QT_MOC_LITERAL(20, 164, 10), // "height_max"
QT_MOC_LITERAL(21, 175, 11), // "block_size1"
QT_MOC_LITERAL(22, 187, 16), // "horizontalKernel"
QT_MOC_LITERAL(23, 204, 14), // "verticalKernel"
QT_MOC_LITERAL(24, 219, 4), // "ssim"
QT_MOC_LITERAL(25, 224, 1), // "s"
QT_MOC_LITERAL(26, 226, 6), // "rotate"
QT_MOC_LITERAL(27, 233, 5), // "angle"
QT_MOC_LITERAL(28, 239, 10), // "showscreen"
QT_MOC_LITERAL(29, 250, 21), // "slot_displayAndDetect"
QT_MOC_LITERAL(30, 272, 8), // "cv::Mat*"
QT_MOC_LITERAL(31, 281, 18), // "slot_readAndDetect"
QT_MOC_LITERAL(32, 300, 6), // "Rect2d"
QT_MOC_LITERAL(33, 307, 4), // "bbox"
QT_MOC_LITERAL(34, 312, 19), // "slot_readAndDetect3"
QT_MOC_LITERAL(35, 332, 19), // "slot_readAndDetect4"
QT_MOC_LITERAL(36, 352, 7), // "diffbox"
QT_MOC_LITERAL(37, 360, 21), // "on_VideoShoot_clicked"
QT_MOC_LITERAL(38, 382, 25), // "on_HandwareDetect_clicked"
QT_MOC_LITERAL(39, 408, 22), // "on_CloseCamera_clicked"
QT_MOC_LITERAL(40, 431, 21), // "onSpinBoxValueChanged"
QT_MOC_LITERAL(41, 453, 5), // "value"
QT_MOC_LITERAL(42, 459, 21), // "on_sureButton_clicked"
QT_MOC_LITERAL(43, 481, 20), // "on_Saveimage_clicked"
QT_MOC_LITERAL(44, 502, 11), // "setdatetime"
QT_MOC_LITERAL(45, 514, 17), // "on_plcbtn_clicked"
QT_MOC_LITERAL(46, 532, 28), // "on_ConnectpushButton_clicked"
QT_MOC_LITERAL(47, 561, 31), // "on_DisconnectpushButton_clicked"
QT_MOC_LITERAL(48, 593, 28), // "on_WriteVDpushButton_clicked"
QT_MOC_LITERAL(49, 622, 30), // "on_WriteVDpushButton_2_clicked"
QT_MOC_LITERAL(50, 653, 30), // "on_WriteVDpushButton_3_clicked"
QT_MOC_LITERAL(51, 684, 11), // "rightremove"
QT_MOC_LITERAL(52, 696, 11), // "wrongremove"
QT_MOC_LITERAL(53, 708, 23), // "on_textsure_btn_clicked"
QT_MOC_LITERAL(54, 732, 13), // "cvMatToQImage"
QT_MOC_LITERAL(55, 746, 7), // "cv::Mat"
QT_MOC_LITERAL(56, 754, 3), // "mat"
QT_MOC_LITERAL(57, 758, 11), // "QImageToMat"
QT_MOC_LITERAL(58, 770, 17), // "on_cancel_clicked"
QT_MOC_LITERAL(59, 788, 22), // "on_delayButton_clicked"
QT_MOC_LITERAL(60, 811, 21), // "slot_clearResultLabel"
QT_MOC_LITERAL(61, 833, 10), // "closeEvent"
QT_MOC_LITERAL(62, 844, 12), // "QCloseEvent*"
QT_MOC_LITERAL(63, 857, 5), // "event"
QT_MOC_LITERAL(64, 863, 24), // "slot_saveBoxesFromThread"
QT_MOC_LITERAL(65, 888, 10), // "cv::Rect2d"
QT_MOC_LITERAL(66, 899, 12), // "detectionBox"
QT_MOC_LITERAL(67, 912, 11), // "trackingBox"
QT_MOC_LITERAL(68, 924, 13), // "isChineseChar"
QT_MOC_LITERAL(69, 938, 1), // "c"
QT_MOC_LITERAL(70, 940, 16), // "isAlnumOrChinese"
QT_MOC_LITERAL(71, 957, 21), // "on_plcmodebtn_clicked"
QT_MOC_LITERAL(72, 979, 26), // "on_eliminatebutton_clicked"
QT_MOC_LITERAL(73, 1006, 23), // "on_pushButton_2_clicked"
QT_MOC_LITERAL(74, 1030, 21), // "on_pushButton_clicked"
QT_MOC_LITERAL(75, 1052, 23), // "on_pushButton_3_clicked"
QT_MOC_LITERAL(76, 1076, 23), // "on_pushButton_5_clicked"
QT_MOC_LITERAL(77, 1100, 23), // "on_pushButton_4_clicked"
QT_MOC_LITERAL(78, 1124, 23), // "on_pushButton_6_clicked"
QT_MOC_LITERAL(79, 1148, 23), // "on_pushButton_7_clicked"
QT_MOC_LITERAL(80, 1172, 23), // "on_pushButton_8_clicked"
QT_MOC_LITERAL(81, 1196, 23), // "on_pushButton_9_clicked"
QT_MOC_LITERAL(82, 1220, 29), // "on_cut_cancelButton_2_clicked"
QT_MOC_LITERAL(83, 1250, 29), // "on_cut_cancelButton_3_clicked"
QT_MOC_LITERAL(84, 1280, 24) // "on_pushButton_10_clicked"

    },
    "Widget\0captureFrame\0\0Mat\0image\0"
    "sendDataTo\0pipei\0imgmuban\0Mat*\0img\0"
    "imgshibie\0kernal\0n\0jiancestring\0String\0"
    "targetstring1\0caijianchicun\0width_min\0"
    "width_max\0height_min\0height_max\0"
    "block_size1\0horizontalKernel\0"
    "verticalKernel\0ssim\0s\0rotate\0angle\0"
    "showscreen\0slot_displayAndDetect\0"
    "cv::Mat*\0slot_readAndDetect\0Rect2d\0"
    "bbox\0slot_readAndDetect3\0slot_readAndDetect4\0"
    "diffbox\0on_VideoShoot_clicked\0"
    "on_HandwareDetect_clicked\0"
    "on_CloseCamera_clicked\0onSpinBoxValueChanged\0"
    "value\0on_sureButton_clicked\0"
    "on_Saveimage_clicked\0setdatetime\0"
    "on_plcbtn_clicked\0on_ConnectpushButton_clicked\0"
    "on_DisconnectpushButton_clicked\0"
    "on_WriteVDpushButton_clicked\0"
    "on_WriteVDpushButton_2_clicked\0"
    "on_WriteVDpushButton_3_clicked\0"
    "rightremove\0wrongremove\0on_textsure_btn_clicked\0"
    "cvMatToQImage\0cv::Mat\0mat\0QImageToMat\0"
    "on_cancel_clicked\0on_delayButton_clicked\0"
    "slot_clearResultLabel\0closeEvent\0"
    "QCloseEvent*\0event\0slot_saveBoxesFromThread\0"
    "cv::Rect2d\0detectionBox\0trackingBox\0"
    "isChineseChar\0c\0isAlnumOrChinese\0"
    "on_plcmodebtn_clicked\0on_eliminatebutton_clicked\0"
    "on_pushButton_2_clicked\0on_pushButton_clicked\0"
    "on_pushButton_3_clicked\0on_pushButton_5_clicked\0"
    "on_pushButton_4_clicked\0on_pushButton_6_clicked\0"
    "on_pushButton_7_clicked\0on_pushButton_8_clicked\0"
    "on_pushButton_9_clicked\0"
    "on_cut_cancelButton_2_clicked\0"
    "on_cut_cancelButton_3_clicked\0"
    "on_pushButton_10_clicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Widget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      54,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      10,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,  284,    2, 0x06 /* Public */,
       5,    1,  287,    2, 0x06 /* Public */,
       6,    0,  290,    2, 0x06 /* Public */,
       7,    1,  291,    2, 0x06 /* Public */,
      10,    1,  294,    2, 0x06 /* Public */,
      11,    1,  297,    2, 0x06 /* Public */,
      13,    1,  300,    2, 0x06 /* Public */,
      16,    7,  303,    2, 0x06 /* Public */,
      24,    1,  318,    2, 0x06 /* Public */,
      26,    1,  321,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      28,    0,  324,    2, 0x08 /* Private */,
      29,    1,  325,    2, 0x08 /* Private */,
      31,    2,  328,    2, 0x08 /* Private */,
      34,    2,  333,    2, 0x08 /* Private */,
      35,    2,  338,    2, 0x08 /* Private */,
      37,    0,  343,    2, 0x08 /* Private */,
      38,    0,  344,    2, 0x08 /* Private */,
      39,    0,  345,    2, 0x08 /* Private */,
      40,    1,  346,    2, 0x08 /* Private */,
      42,    0,  349,    2, 0x08 /* Private */,
      43,    0,  350,    2, 0x08 /* Private */,
      44,    0,  351,    2, 0x08 /* Private */,
      45,    0,  352,    2, 0x08 /* Private */,
      46,    0,  353,    2, 0x08 /* Private */,
      47,    0,  354,    2, 0x08 /* Private */,
      48,    0,  355,    2, 0x08 /* Private */,
      49,    0,  356,    2, 0x08 /* Private */,
      50,    0,  357,    2, 0x08 /* Private */,
      51,    0,  358,    2, 0x08 /* Private */,
      52,    0,  359,    2, 0x08 /* Private */,
      53,    0,  360,    2, 0x08 /* Private */,
      54,    1,  361,    2, 0x08 /* Private */,
      57,    1,  364,    2, 0x08 /* Private */,
      58,    0,  367,    2, 0x08 /* Private */,
      59,    0,  368,    2, 0x08 /* Private */,
      60,    0,  369,    2, 0x08 /* Private */,
      61,    1,  370,    2, 0x08 /* Private */,
      64,    2,  373,    2, 0x08 /* Private */,
      68,    1,  378,    2, 0x08 /* Private */,
      70,    1,  381,    2, 0x08 /* Private */,
      71,    0,  384,    2, 0x08 /* Private */,
      72,    0,  385,    2, 0x08 /* Private */,
      73,    0,  386,    2, 0x08 /* Private */,
      74,    0,  387,    2, 0x08 /* Private */,
      75,    0,  388,    2, 0x08 /* Private */,
      76,    0,  389,    2, 0x08 /* Private */,
      77,    0,  390,    2, 0x08 /* Private */,
      78,    0,  391,    2, 0x08 /* Private */,
      79,    0,  392,    2, 0x08 /* Private */,
      80,    0,  393,    2, 0x08 /* Private */,
      81,    0,  394,    2, 0x08 /* Private */,
      82,    0,  395,    2, 0x08 /* Private */,
      83,    0,  396,    2, 0x08 /* Private */,
      84,    0,  397,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    2,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, QMetaType::Int,   12,
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int, QMetaType::Int,   17,   18,   19,   20,   21,   22,   23,
    QMetaType::Void, QMetaType::Int,   25,
    QMetaType::Void, QMetaType::Int,   27,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 30,    4,
    QMetaType::Void, 0x80000000 | 30, 0x80000000 | 32,    4,   33,
    QMetaType::Void, 0x80000000 | 30, 0x80000000 | 32,    4,   33,
    QMetaType::Void, 0x80000000 | 30, 0x80000000 | 32,    4,   36,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   41,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::QString,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::QImage, 0x80000000 | 55,   56,
    0x80000000 | 8, QMetaType::QImage,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 62,   63,
    QMetaType::Void, 0x80000000 | 65, 0x80000000 | 65,   66,   67,
    QMetaType::Bool, QMetaType::UChar,   69,
    QMetaType::Bool, QMetaType::Char,   69,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void Widget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Widget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->captureFrame((*reinterpret_cast< Mat(*)>(_a[1]))); break;
        case 1: _t->sendDataTo((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 2: _t->pipei(); break;
        case 3: _t->imgmuban((*reinterpret_cast< Mat*(*)>(_a[1]))); break;
        case 4: _t->imgshibie((*reinterpret_cast< Mat*(*)>(_a[1]))); break;
        case 5: _t->kernal((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->jiancestring((*reinterpret_cast< String(*)>(_a[1]))); break;
        case 7: _t->caijianchicun((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5])),(*reinterpret_cast< int(*)>(_a[6])),(*reinterpret_cast< int(*)>(_a[7]))); break;
        case 8: _t->ssim((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: _t->rotate((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->showscreen(); break;
        case 11: _t->slot_displayAndDetect((*reinterpret_cast< cv::Mat*(*)>(_a[1]))); break;
        case 12: _t->slot_readAndDetect((*reinterpret_cast< cv::Mat*(*)>(_a[1])),(*reinterpret_cast< Rect2d(*)>(_a[2]))); break;
        case 13: _t->slot_readAndDetect3((*reinterpret_cast< cv::Mat*(*)>(_a[1])),(*reinterpret_cast< Rect2d(*)>(_a[2]))); break;
        case 14: _t->slot_readAndDetect4((*reinterpret_cast< cv::Mat*(*)>(_a[1])),(*reinterpret_cast< Rect2d(*)>(_a[2]))); break;
        case 15: _t->on_VideoShoot_clicked(); break;
        case 16: _t->on_HandwareDetect_clicked(); break;
        case 17: _t->on_CloseCamera_clicked(); break;
        case 18: _t->onSpinBoxValueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 19: _t->on_sureButton_clicked(); break;
        case 20: _t->on_Saveimage_clicked(); break;
        case 21: { QString _r = _t->setdatetime();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 22: _t->on_plcbtn_clicked(); break;
        case 23: _t->on_ConnectpushButton_clicked(); break;
        case 24: _t->on_DisconnectpushButton_clicked(); break;
        case 25: _t->on_WriteVDpushButton_clicked(); break;
        case 26: _t->on_WriteVDpushButton_2_clicked(); break;
        case 27: _t->on_WriteVDpushButton_3_clicked(); break;
        case 28: _t->rightremove(); break;
        case 29: _t->wrongremove(); break;
        case 30: _t->on_textsure_btn_clicked(); break;
        case 31: { QImage _r = _t->cvMatToQImage((*reinterpret_cast< const cv::Mat(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QImage*>(_a[0]) = std::move(_r); }  break;
        case 32: { Mat* _r = _t->QImageToMat((*reinterpret_cast< const QImage(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< Mat**>(_a[0]) = std::move(_r); }  break;
        case 33: _t->on_cancel_clicked(); break;
        case 34: _t->on_delayButton_clicked(); break;
        case 35: _t->slot_clearResultLabel(); break;
        case 36: _t->closeEvent((*reinterpret_cast< QCloseEvent*(*)>(_a[1]))); break;
        case 37: _t->slot_saveBoxesFromThread((*reinterpret_cast< cv::Rect2d(*)>(_a[1])),(*reinterpret_cast< cv::Rect2d(*)>(_a[2]))); break;
        case 38: { bool _r = _t->isChineseChar((*reinterpret_cast< unsigned char(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 39: { bool _r = _t->isAlnumOrChinese((*reinterpret_cast< char(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 40: _t->on_plcmodebtn_clicked(); break;
        case 41: _t->on_eliminatebutton_clicked(); break;
        case 42: _t->on_pushButton_2_clicked(); break;
        case 43: _t->on_pushButton_clicked(); break;
        case 44: _t->on_pushButton_3_clicked(); break;
        case 45: _t->on_pushButton_5_clicked(); break;
        case 46: _t->on_pushButton_4_clicked(); break;
        case 47: _t->on_pushButton_6_clicked(); break;
        case 48: _t->on_pushButton_7_clicked(); break;
        case 49: _t->on_pushButton_8_clicked(); break;
        case 50: _t->on_pushButton_9_clicked(); break;
        case 51: _t->on_cut_cancelButton_2_clicked(); break;
        case 52: _t->on_cut_cancelButton_3_clicked(); break;
        case 53: _t->on_pushButton_10_clicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Widget::*)(Mat );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Widget::captureFrame)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (Widget::*)(QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Widget::sendDataTo)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (Widget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Widget::pipei)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (Widget::*)(Mat * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Widget::imgmuban)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (Widget::*)(Mat * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Widget::imgshibie)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (Widget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Widget::kernal)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (Widget::*)(String );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Widget::jiancestring)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (Widget::*)(int , int , int , int , int , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Widget::caijianchicun)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (Widget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Widget::ssim)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (Widget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Widget::rotate)) {
                *result = 9;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject Widget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_Widget.data,
    qt_meta_data_Widget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *Widget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Widget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Widget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int Widget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 54)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 54;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 54)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 54;
    }
    return _id;
}

// SIGNAL 0
void Widget::captureFrame(Mat _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void Widget::sendDataTo(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void Widget::pipei()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void Widget::imgmuban(Mat * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void Widget::imgshibie(Mat * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void Widget::kernal(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void Widget::jiancestring(String _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void Widget::caijianchicun(int _t1, int _t2, int _t3, int _t4, int _t5, int _t6, int _t7)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t5))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t6))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t7))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void Widget::ssim(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void Widget::rotate(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
