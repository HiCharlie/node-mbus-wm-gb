#include <string.h>
#include <stdio.h>
#include "CMbus.h"
#include "mbus-serial.h"

#if 0
#define  tr_err(_msg_)              napi_throw_error(env, NULL, _msg_)
#else
#define  tr_err(_msg_)              printf("%s", _msg_)
#endif

napi_ref CMbus::_constructor;

CMbus::CMbus() {
    _databits = 8;
    _stopbits = 1;
    _parity   = 'e';

    _mbus.fd = -1;
    _mbus.open  = mbus_serial_connect;
    _mbus.close = mbus_serial_disconnect;
    _mbus.send  = mbus_serial_send_frame;
    _mbus.recv  = mbus_serial_recv_frame;
    _mbus.data_free  = mbus_serial_data_free;
    mbus_serial_data_init(&_mbus);
    mbus_serial_set_baudrate(&_mbus, 9600);
    mbus_serial_set_format(&_mbus, _databits, _stopbits, _parity);

    _tx.meter_type = MBUS_DEFAULT_METER_TYPE;
    memset(_tx.addr, 0xAA, MBUS_ADDR_LEN);
}

CMbus::~CMbus() {
    _mbus.close(&_mbus);
    _mbus.data_free(&_mbus);
}

void CMbus::Init(napi_env *env, napi_value *exports) {
    napi_property_descriptor properties[] = {
        { "device",   NULL,  NULL,   getString, setString,  0, napi_default, (void*)CMBUS_FLAG_SERIAL_DEVICE },
        { "baudrate", NULL,  NULL,   getInt32,  setInt32,   0, napi_default, (void*)CMBUS_FLAG_SERIAL_BAUDRATE },
        { "databits", NULL,  NULL,   getInt32,  setInt32,   0, napi_default, (void*)CMBUS_FLAG_SERIAL_DATABITS },
        { "stopbits", NULL,  NULL,   getInt32,  setInt32,   0, napi_default, (void*)CMBUS_FLAG_SERIAL_STOPBITS },
        { "parity",   NULL,  NULL,   getString, setString,  0, napi_default, (void*)CMBUS_FLAG_SERIAL_PARITY },
        { "addr",     NULL,  NULL,   getBuffer, setBuffer,  0, napi_default, (void*)CMBUS_FLAG_ADDR },
        { "metertype",NULL,  NULL,   getInt32,  setInt32,   0, napi_default, (void*)CMBUS_FLAG_METERTYPE },
        { "connect",    NULL,  connect,    NULL, NULL,  0, napi_default, NULL },
        { "disconnect", NULL,  disconnect, NULL, NULL,  0, napi_default, NULL },
        { "isConnect",  NULL,  isconnect,  NULL, NULL,  0, napi_default, NULL },
        { "recv",       NULL,  recv,       NULL, NULL,  0, napi_default, NULL },
        { "send",      NULL,   send,      NULL, NULL,  0, napi_default, NULL },
    };

    napi_value cons;
    napi_define_class(*env, "Mbus", NAPI_AUTO_LENGTH, structure, NULL, sizeof(properties) / sizeof(napi_property_descriptor), properties, &cons);
    napi_create_reference(*env, cons, 1, &_constructor);
    napi_set_named_property(*env, *exports, "Mbus", cons);
}

napi_value CMbus::structure(napi_env env, napi_callback_info info) {
    napi_status status;
    napi_value new_target;
    napi_value _this;

    status = napi_get_cb_info(env, info, NULL, NULL, &_this, NULL);
    if (status != napi_ok) {
        tr_err("structure: napi_get_cb_info error.\r\n");
        return NULL;
    }

    napi_get_new_target(env, info, &new_target);
    if (new_target != NULL) {
        // Invoked as constructor: `new CMbus(...)`
        CMbus* obj = new CMbus();
        obj->_env = env;
        napi_wrap(env, _this, obj, CMbus::destructor, NULL, &obj->_wrapper);
        return _this;
    } else {
        // Invoked as plain function_wrap `CMbus(...)`, turn into construct call.
        napi_value cons;
        napi_value instance;
        status = napi_get_reference_value(env, _constructor, &cons);
        if (status != napi_ok) {
            tr_err("structure: napi_get_reference_value error.\r\n");
            return NULL;
        }

        status = napi_new_instance(env, cons, NULL, NULL, &instance);
        if (status != napi_ok) {
            tr_err("structure: napi_new_instance error.\r\n");
            return NULL;
        }
        return instance;
    }
}

void CMbus::destructor(napi_env env, void* nativeObject, void*) {
    CMbus* obj = static_cast<CMbus*>(nativeObject);
    delete obj;
}

bool CMbus::getParm(napi_env &env, napi_callback_info &info, CMbus **obj, napi_value *args, size_t *argc, int *flag) {
    napi_status status;
    napi_value  thisValue;

    if (flag) {
        status = napi_get_cb_info(env, info, argc, args, &thisValue,  (void**)(flag));
    } else {
        status = napi_get_cb_info(env, info, argc, args, &thisValue,  NULL);
    }
    if (status != napi_ok) {
        tr_err("napi_get_cb_info error.\r\n");
        return false;
    }

    napi_unwrap(env, thisValue, reinterpret_cast<void**>(obj));
    if (status != napi_ok) {
        tr_err("napi_get_cb_info error.\r\n");
        return false;
    }
    return true;
}

napi_value CMbus::getInt32(napi_env env, napi_callback_info info) {
    napi_value  ret;
    CMbus      *obj;
    int         flag;
    int         val;

    if (!getParm(env, info, &obj, NULL, 0, &flag)) {
        return NULL;
    } 

    switch(flag) {
        case CMBUS_FLAG_SERIAL_BAUDRATE:
            val = mbus_serial_get_baudrate(&obj->_mbus);
            break;
        case CMBUS_FLAG_SERIAL_DATABITS:
            val = obj->_databits;
            break;
        case CMBUS_FLAG_SERIAL_STOPBITS:
            val = obj->_stopbits;
            break;
        case CMBUS_FLAG_METERTYPE:
            val = obj->_tx.meter_type;
            break;
        default:
            tr_err("CMBUS::getInt32: Invaild param flag.\r\n");
            return NULL;
    }
    napi_create_int32(env, val, &ret);
    return ret;
}

napi_value CMbus::setInt32(napi_env env, napi_callback_info info) {
    napi_status status;
    size_t      argc = 1;
    napi_value  args[1];
    CMbus*      obj;
    int         flag;
    int32_t     val;

    if (!getParm(env, info, &obj, args, &argc, &flag)) {
        return NULL;
    }

    status = napi_get_value_int32(env, args[0], &val);
    if (status != napi_ok) {
        tr_err("CMbus::setInt32: napi_get_value_int32 error.\r\n");
        return NULL;
    }

    switch(flag) {
        case CMBUS_FLAG_SERIAL_BAUDRATE:
            if (0 != mbus_serial_set_baudrate(&obj->_mbus, val)) 
                tr_err("setInt32: mbus_serial_set_baudrate error.\r\n");
            break;
        case CMBUS_FLAG_SERIAL_DATABITS:
            obj->_databits = val;
            break;
        case CMBUS_FLAG_SERIAL_STOPBITS:
            obj->_stopbits = val;
            break;
        case CMBUS_FLAG_METERTYPE:
            if (val < 0 || val > 255) {
                tr_err("setInt32: Invaild Meter Type.\r\n");
            } else {
                val = obj->_tx.meter_type;
            }
            break;
        default:
            tr_err("CMBUS::setInt32: Invaild param flag.\r\n");
            return NULL;
    }

    if (CMBUS_FLAG_SERIAL_DATABITS == flag ||  CMBUS_FLAG_SERIAL_STOPBITS == flag) {
        if (0 != mbus_serial_set_format(&obj->_mbus, obj->_databits, obj->_stopbits, obj->_parity)) 
            tr_err("setDataBits: mbus_serial_set_format error.\r\n");
    }
    return NULL;
}

napi_value CMbus::getString(napi_env env, napi_callback_info info) {
    napi_status status;
    napi_value  ret;
    CMbus      *obj;
    int         flag;
    char       *val;
    size_t      len = NAPI_AUTO_LENGTH;

    if (!getParm(env, info, &obj, NULL, 0, &flag)) {
        return NULL;
    }

    switch(flag) {
        case CMBUS_FLAG_SERIAL_DEVICE:
            val = ((mbus_serial_data*)obj->_mbus.auxdata)->device;
            break;
        case CMBUS_FLAG_SERIAL_PARITY:
            val = &obj->_parity;
            len = 1;
            break;
        default:
            tr_err("CMBUS::getInt32: Invaild param flag.\r\n");
            break;
    }

    status = napi_create_string_utf8(env, val, len, &ret);
    if (status != napi_ok) {
        tr_err("CMbus::getString: napi_create_string_utf8 error.\r\n");
        return NULL;
    }
    return ret;
}

napi_value CMbus::setString(napi_env env, napi_callback_info info) {
    napi_status status;
    size_t      argc = 1;
    napi_value  args[1];
    CMbus*      obj;
    int         flag;
    size_t      size;
    char        val[256];

    if (!getParm(env, info, &obj, args, &argc, &flag)) {
        return NULL;
    }

    status = napi_get_value_string_utf8(env, args[0], val, 256 - 1, &size);
    if (status != napi_ok) {
        tr_err("CMbus::getString: napi_get_value_string_utf8 error.\r\n");
        return NULL;
    }
    val[size] = 0;

    switch(flag) {
        case CMBUS_FLAG_SERIAL_DEVICE:
            if (mbus_serial_set_device(&obj->_mbus, val) != 0) 
                tr_err("CMbus::getString: mbus_serial_set_device error.\r\n");
            break;
        case CMBUS_FLAG_SERIAL_PARITY:
            if (size != 1) {
                tr_err("CMbus::getString: Invaild parity lenght.\r\n");
                return NULL;
            }
            obj->_parity = val[0];
            if (0 != mbus_serial_set_format(&obj->_mbus, obj->_databits, obj->_stopbits, obj->_parity)) 
                tr_err("CMbus::getString: mbus_serial_set_format error.\r\n");
            break;
        default:
            tr_err("CMbus::getString: Invaild param flag.\r\n");
            return NULL;
    }
    return NULL;
}

napi_value CMbus::getBuffer(napi_env env, napi_callback_info info) {
    napi_status status;
    napi_value  ret;
    CMbus      *obj;
    int         flag;
    char       *val;
    size_t      len;

    if (!getParm(env, info, &obj, NULL, 0, &flag)) {
        return NULL;
    }

    switch(flag) {
        case CMBUS_FLAG_ADDR:
            val = (char*)obj->_tx.addr;
            len = MBUS_ADDR_LEN;
            break;
        default:
            tr_err("CMBUS::getBuffer: Invaild param flag.\r\n");
            break;
    }

    status = napi_create_buffer_copy(env, len, val, NULL, &ret);
    if (status != napi_ok) {
        tr_err("CMbus::getBuffer: napi_create_buffer error.\r\n");
        return NULL;
    }
    return ret;
}

napi_value CMbus::setBuffer(napi_env env, napi_callback_info info) {
    napi_status status;
    size_t      argc = 1;
    napi_value  args[1];
    CMbus*      obj;
    int         flag;
    size_t      len;
    char       *val;

    if (!getParm(env, info, &obj, args, &argc, &flag)) {
        return NULL;
    }

    status = napi_get_buffer_info(env, args[0], (void**)&val, &len);
    if (status != napi_ok) {
        tr_err("CMbus::setBuffer: napi_get_buffer_info error.\r\n");
        return NULL;
    }

    switch(flag) {
        case CMBUS_FLAG_ADDR:
            if (len != MBUS_ADDR_LEN) {
                tr_err("CMbus::setBuffer: Invaild addr len.\r\n");
                return NULL;
            }
            memcpy(obj->_tx.addr, val, MBUS_ADDR_LEN);
            break;
        default:
            tr_err("CMbus::setBuffer: Invaild param flag.\r\n");
            return NULL;
    }
    return NULL;
}

napi_value CMbus::connect(napi_env env, napi_callback_info info) {
    napi_value  ret;
    int32_t     errCode = -1;
    CMbus      *obj;


    if (!getParm(env, info, &obj, NULL, 0, NULL)) {
        goto exit;
    } 

    if (-1 != obj->_mbus.fd) {
        //tr_err("CMbus::connect: device is Connected.\r\n");
        goto exit;
    }

    if (0 != obj->_mbus.open(&obj->_mbus)) {
        tr_err("CMbus::connect: device open failure.\r\n");
        goto exit;
    }
    errCode = 0;
exit:
    napi_create_int32(env, errCode, &ret);
    return ret;
}

napi_value CMbus::disconnect(napi_env env, napi_callback_info info) {
    napi_value  ret;
    int32_t     errCode = -1;
    CMbus      *obj;


    if (!getParm(env, info, &obj, NULL, 0, NULL)) {
        goto exit;
    } 

    if (-1 == obj->_mbus.fd) {
        //tr_err("CMbus::disconnect: device not connect.\r\n");
        goto exit;
    }

    if (0 != obj->_mbus.close(&obj->_mbus)) {
        goto exit;
    }
    errCode = 0;
exit:
    napi_create_int32(env, errCode, &ret);
    return ret;
}

napi_value CMbus::isconnect(napi_env env, napi_callback_info info) {
    napi_value  ret;
    CMbus*      obj;

    if (!getParm(env, info, &obj, NULL, 0, NULL)) {
        return NULL;
    } 

    napi_create_int32(env, (obj->_mbus.fd != -1) ? 1 : 0, &ret);
    return ret;
}

napi_value CMbus::recv(napi_env env, napi_callback_info info) {
    napi_value  napiRet, napiCode, napiMeterType, napiAddr, napiCtrl, napiPayload;
    int32_t     errCode = -1;
    CMbus*      obj;

    if (!getParm(env, info, &obj, NULL, 0, NULL)) {
        return NULL;
    } 

    if (obj->_mbus.fd == -1) {
        errCode = -1;
        goto exit; 
    }
    
    if (0 != obj->_mbus.recv(&obj->_mbus, &obj->_rx)) {
        tr_err("CMbus::recv: recv packet failure.\r\n");
        errCode = -3;
        goto exit;   
    }

    napi_create_int32(env, obj->_rx.meter_type, &napiMeterType);
    napi_create_int32(env, obj->_rx.control, &napiCtrl);
    napi_create_buffer_copy(env, MBUS_ADDR_LEN, obj->_rx.addr, NULL, &napiAddr);
    napi_create_buffer_copy(env, obj->_rx.data_len, obj->_rx.data, NULL, &napiPayload);
    errCode = 0;

exit:
    napi_create_object(env, &napiRet);
    napi_create_int32(env, errCode, &napiCode);
    napi_set_named_property(env, napiRet, "code", napiCode);
    if (errCode == 0) {
        napi_set_named_property(env, napiRet, "metertype", napiMeterType);
        napi_set_named_property(env, napiRet, "addr",      napiAddr);
        napi_set_named_property(env, napiRet, "ctrl",      napiCtrl);
        napi_set_named_property(env, napiRet, "payload",   napiPayload);
    }
    return napiRet;
}

napi_value CMbus::send(napi_env env, napi_callback_info info) {
    napi_status status;
    napi_value  ret;
    int32_t     errCode = -1, control;
    size_t      argc = 2;
    napi_value  args[2];
    CMbus*      obj;
    size_t      payloadLen;
    char       *payload;

    if (!getParm(env, info, &obj, args, &argc, NULL)) {
        return NULL;
    } 

    if (obj->_mbus.fd == -1) {
        errCode = -1;
        goto exit; 
    }

    status = napi_get_value_int32(env, args[0], &control);
    if (status != napi_ok) {
        tr_err("CMbus::send: get param control error.\r\n");
        errCode = -2;
        goto exit; 
    }

    if (control < 0 || control > 0xFF) {
        tr_err("CMbus::send: Invaild param control.\r\n");
        errCode = -2;
        goto exit; 
    }

    status = napi_get_buffer_info(env, args[1], (void**)&payload, &payloadLen);
    if (status != napi_ok) {
        tr_err("CMbus::send: get param payload error.\r\n");
        errCode = -2;
        goto exit; 
    }

    if (!payload || payloadLen < 0 || payloadLen > MBUS_FRAME_DATA_LENGTH) {
        tr_err("CMbus::send: Invaild param payload.\r\n");
        errCode = -2;
        goto exit; 
    }

    obj->_tx.control = control;
    obj->_tx.data_len = payloadLen;
    memcpy(obj->_tx.data, payload, payloadLen);
    if (0 != obj->_mbus.send(&obj->_mbus, &obj->_tx)) {
        tr_err("CMbus::send: send packet failure.\r\n");
        errCode = -3;
        goto exit;   
    }
    errCode = 0;
exit:
    napi_create_int32(env, errCode, &ret);
    return ret; 
}