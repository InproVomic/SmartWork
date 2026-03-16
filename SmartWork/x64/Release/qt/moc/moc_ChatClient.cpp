/****************************************************************************
** Meta object code from reading C++ file 'ChatClient.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.12)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../ChatClient.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ChatClient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.12. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ChatClient_t {
    QByteArrayData data[29];
    char stringdata0[404];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ChatClient_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ChatClient_t qt_meta_stringdata_ChatClient = {
    {
QT_MOC_LITERAL(0, 0, 10), // "ChatClient"
QT_MOC_LITERAL(1, 11, 11), // "messageSent"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 7), // "message"
QT_MOC_LITERAL(4, 32, 13), // "onSendClicked"
QT_MOC_LITERAL(5, 46, 15), // "onReturnPressed"
QT_MOC_LITERAL(6, 62, 15), // "onConfigClicked"
QT_MOC_LITERAL(7, 78, 21), // "onAvatarConfigClicked"
QT_MOC_LITERAL(8, 100, 15), // "onExportClicked"
QT_MOC_LITERAL(9, 116, 13), // "onApiResponse"
QT_MOC_LITERAL(10, 130, 11), // "ApiResponse"
QT_MOC_LITERAL(11, 142, 8), // "response"
QT_MOC_LITERAL(12, 151, 16), // "onRequestStarted"
QT_MOC_LITERAL(13, 168, 14), // "onTcpConnected"
QT_MOC_LITERAL(14, 183, 17), // "onTcpDisconnected"
QT_MOC_LITERAL(15, 201, 21), // "onTcpResponseReceived"
QT_MOC_LITERAL(16, 223, 7), // "content"
QT_MOC_LITERAL(17, 231, 7), // "success"
QT_MOC_LITERAL(18, 239, 10), // "onTcpError"
QT_MOC_LITERAL(19, 250, 5), // "error"
QT_MOC_LITERAL(20, 256, 22), // "onPythonProcessStarted"
QT_MOC_LITERAL(21, 279, 23), // "onPythonProcessFinished"
QT_MOC_LITERAL(22, 303, 8), // "exitCode"
QT_MOC_LITERAL(23, 312, 20), // "QProcess::ExitStatus"
QT_MOC_LITERAL(24, 333, 10), // "exitStatus"
QT_MOC_LITERAL(25, 344, 20), // "onPythonProcessError"
QT_MOC_LITERAL(26, 365, 16), // "onReconnectTimer"
QT_MOC_LITERAL(27, 382, 15), // "onScrollChanged"
QT_MOC_LITERAL(28, 398, 5) // "value"

    },
    "ChatClient\0messageSent\0\0message\0"
    "onSendClicked\0onReturnPressed\0"
    "onConfigClicked\0onAvatarConfigClicked\0"
    "onExportClicked\0onApiResponse\0ApiResponse\0"
    "response\0onRequestStarted\0onTcpConnected\0"
    "onTcpDisconnected\0onTcpResponseReceived\0"
    "content\0success\0onTcpError\0error\0"
    "onPythonProcessStarted\0onPythonProcessFinished\0"
    "exitCode\0QProcess::ExitStatus\0exitStatus\0"
    "onPythonProcessError\0onReconnectTimer\0"
    "onScrollChanged\0value"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ChatClient[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   99,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,  102,    2, 0x08 /* Private */,
       5,    0,  103,    2, 0x08 /* Private */,
       6,    0,  104,    2, 0x08 /* Private */,
       7,    0,  105,    2, 0x08 /* Private */,
       8,    0,  106,    2, 0x08 /* Private */,
       9,    1,  107,    2, 0x08 /* Private */,
      12,    0,  110,    2, 0x08 /* Private */,
      13,    0,  111,    2, 0x08 /* Private */,
      14,    0,  112,    2, 0x08 /* Private */,
      15,    2,  113,    2, 0x08 /* Private */,
      18,    1,  118,    2, 0x08 /* Private */,
      20,    0,  121,    2, 0x08 /* Private */,
      21,    2,  122,    2, 0x08 /* Private */,
      25,    1,  127,    2, 0x08 /* Private */,
      26,    0,  130,    2, 0x08 /* Private */,
      27,    1,  131,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,   16,   17,
    QMetaType::Void, QMetaType::QString,   19,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, 0x80000000 | 23,   22,   24,
    QMetaType::Void, QMetaType::QString,   19,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   28,

       0        // eod
};

void ChatClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChatClient *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->messageSent((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->onSendClicked(); break;
        case 2: _t->onReturnPressed(); break;
        case 3: _t->onConfigClicked(); break;
        case 4: _t->onAvatarConfigClicked(); break;
        case 5: _t->onExportClicked(); break;
        case 6: _t->onApiResponse((*reinterpret_cast< const ApiResponse(*)>(_a[1]))); break;
        case 7: _t->onRequestStarted(); break;
        case 8: _t->onTcpConnected(); break;
        case 9: _t->onTcpDisconnected(); break;
        case 10: _t->onTcpResponseReceived((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 11: _t->onTcpError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 12: _t->onPythonProcessStarted(); break;
        case 13: _t->onPythonProcessFinished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QProcess::ExitStatus(*)>(_a[2]))); break;
        case 14: _t->onPythonProcessError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 15: _t->onReconnectTimer(); break;
        case 16: _t->onScrollChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ChatClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChatClient::messageSent)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ChatClient::staticMetaObject = { {
    &QWidget::staticMetaObject,
    qt_meta_stringdata_ChatClient.data,
    qt_meta_data_ChatClient,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ChatClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChatClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ChatClient.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ChatClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 17;
    }
    return _id;
}

// SIGNAL 0
void ChatClient::messageSent(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
