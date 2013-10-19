#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "zend_variables.h"
#include "zend_exceptions.h"
#include "zend_API.h"
#include "ext/standard/info.h"
#include "php_mosquitto.h"

zend_class_entry *mosquitto_ce_client;
zend_class_entry *mosquitto_ce_exception;
zend_object_handlers mosquitto_std_object_handlers;
zend_error_handling mosquitto_original_error_handling;

/* {{{ */
PHP_METHOD(Mosquitto_Client, __construct)
{
	mosquitto_client_object *object;
	char *id = NULL;
	int id_len = 0;
	zend_bool clean_session = 1;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sb", &id, &id_len, &clean_session) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	object->client = mosquitto_new(id, clean_session, object);

	if (!object->client) {
		char *message = php_mosquitto_strerror_wrapper(errno);
		zend_throw_exception(mosquitto_ce_exception, message, 1 TSRMLS_CC);
	}
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, setCredentials)
{
	mosquitto_client_object *object;
	char *username = NULL, *password = NULL;
	int username_len, password_len, retval;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &username, &username_len, &password, &password_len) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	retval = mosquitto_username_pw_set(object->client, username, password);
	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, setWill)
{
	mosquitto_client_object *object;
	int topic_len, payload_len, retval;
	long qos;
	zend_bool retain;
	char *topic, *payload;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sslb",
				&topic, &topic_len, &payload, &payload_len, &qos, &retain) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	retval = mosquitto_will_set(object->client, topic, payload_len, (void *) payload, qos, retain);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, clearWill)
{
	mosquitto_client_object *object;
	int retval;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters_none() == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	retval = mosquitto_will_clear(object->client);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, setReconnectDelay)
{
	mosquitto_client_object *object;
	int retval;
	long reconnect_delay = 0, reconnect_delay_max = 0;
	zend_bool exponential_backoff = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|lb",
				&reconnect_delay, &reconnect_delay_max, &exponential_backoff)  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	retval = mosquitto_reconnect_delay_set(object->client, reconnect_delay, reconnect_delay_max, exponential_backoff);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, connect)
{
	mosquitto_client_object *object;
	char *host = NULL, *interface = NULL;
	int host_len, interface_len, retval;
	long port = 1883;
	long keepalive = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|lls!",
				&host, &host_len, &port, &keepalive,
				&interface, &interface_len)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	if (interface == NULL) {
		retval = mosquitto_connect(object->client, host, port, keepalive);
	} else {
		retval = mosquitto_connect_bind(object->client, host, port, keepalive, interface);
	}

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, disconnect)
{
	mosquitto_client_object *object;
	char *host = NULL, *interface = NULL;
	int host_len, interface_len, retval;
	long port = 1883;
	long keepalive = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters_none() == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	retval = mosquitto_disconnect(object->client);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, onConnect)
{
	mosquitto_client_object *object;
	zend_fcall_info connect_callback = empty_fcall_info;
	zend_fcall_info_cache connect_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&connect_callback, &connect_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!ZEND_FCI_INITIALIZED(connect_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
	}

	object->connect_callback = connect_callback;
	object->connect_callback_cache = connect_callback_cache;
	Z_ADDREF_P(connect_callback.function_name);

	if (connect_callback.object_ptr != NULL) {
		Z_ADDREF_P(connect_callback.object_ptr);
	}

	mosquitto_connect_callback_set(object->client, php_mosquitto_connect_callback);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, onDisconnect)
{
	mosquitto_client_object *object;
	zend_fcall_info disconnect_callback = empty_fcall_info;
	zend_fcall_info_cache disconnect_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&disconnect_callback, &disconnect_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!ZEND_FCI_INITIALIZED(disconnect_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
	}

	object->disconnect_callback = disconnect_callback;
	object->disconnect_callback_cache = disconnect_callback_cache;
	Z_ADDREF_P(disconnect_callback.function_name);

	if (disconnect_callback.object_ptr != NULL) {
		Z_ADDREF_P(disconnect_callback.object_ptr);
	}

	mosquitto_disconnect_callback_set(object->client, php_mosquitto_disconnect_callback);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, onSubscribe)
{
	mosquitto_client_object *object;
	zend_fcall_info subscribe_callback = empty_fcall_info;
	zend_fcall_info_cache subscribe_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&subscribe_callback, &subscribe_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!ZEND_FCI_INITIALIZED(subscribe_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
	}

	object->subscribe_callback = subscribe_callback;
	object->subscribe_callback_cache = subscribe_callback_cache;
	Z_ADDREF_P(subscribe_callback.function_name);

	if (subscribe_callback.object_ptr != NULL) {
		Z_ADDREF_P(subscribe_callback.object_ptr);
	}

	mosquitto_subscribe_callback_set(object->client, php_mosquitto_subscribe_callback);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, onMessage)
{
	mosquitto_client_object *object;
	zend_fcall_info message_callback = empty_fcall_info;
	zend_fcall_info_cache message_callback_cache = empty_fcall_info_cache;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "f!",
				&message_callback, &message_callback_cache)  == FAILURE) {

		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);

	if (!ZEND_FCI_INITIALIZED(message_callback)) {
		zend_throw_exception(mosquitto_ce_exception, "Need a valid callback", 0 TSRMLS_CC);
	}

	object->message_callback = message_callback;
	object->message_callback_cache = message_callback_cache;
	Z_ADDREF_P(message_callback.function_name);

	if (message_callback.object_ptr != NULL) {
		Z_ADDREF_P(message_callback.object_ptr);
	}

	mosquitto_message_callback_set(object->client, php_mosquitto_message_callback);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, setMaxInFlightMessages)
{
	mosquitto_client_object *object;
	int retval;
	long max = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &max)  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	retval = mosquitto_max_inflight_messages_set(object->client, max);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, setMessageRetry)
{
	mosquitto_client_object *object;
	int retval;
	long retry = 0;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &retry)  == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	mosquitto_message_retry_set(object->client, retry);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, publish)
{
	mosquitto_client_object *object;
	int mid, topic_len, payload_len, retval;
	long qos;
	zend_bool retain;
	char *topic, *payload;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sslb",
				&topic, &topic_len, &payload, &payload_len, &qos, &retain) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	retval = mosquitto_publish(object->client, &mid, topic, payload_len, (void *) payload, qos, retain);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);

	RETURN_LONG(mid);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, subscribe)
{
	mosquitto_client_object *object;
	char *sub;
	int sub_len, retval, mid;
	long qos;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
				&sub, &sub_len, &qos) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	retval = mosquitto_subscribe(object->client, &mid, sub, qos);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);

	RETURN_LONG(mid);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, unsubscribe)
{
	mosquitto_client_object *object;
	char *sub;
	int sub_len, retval, mid;
	long qos;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
				&sub, &sub_len, &qos) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	retval = mosquitto_unsubscribe(object->client, &mid, sub);

	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);

	RETURN_LONG(mid);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, loop)
{
	mosquitto_client_object *object;
	long timeout = 1000, max_packets = 1, retval = 0;
	char *message = NULL;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll",
				&timeout, &max_packets) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	retval = mosquitto_loop(object->client, timeout, max_packets);
	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* {{{ */
PHP_METHOD(Mosquitto_Client, loopForever)
{
	mosquitto_client_object *object;
	long timeout = 1000, max_packets = 1, retval = 0;
	char *message = NULL;

	PHP_MOSQUITTO_ERROR_HANDLING();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ll",
				&timeout, &max_packets) == FAILURE) {
		PHP_MOSQUITTO_RESTORE_ERRORS();
		return;
	}
	PHP_MOSQUITTO_RESTORE_ERRORS();

	object = (mosquitto_client_object *) zend_object_store_get_object(getThis() TSRMLS_CC);
	retval = mosquitto_loop_forever(object->client, timeout, max_packets);
	php_mosquitto_handle_errno(retval, errno TSRMLS_CC);
}
/* }}} */

/* Internal functions */

PHP_MOSQUITTO_API char *php_mosquitto_strerror_wrapper(int err)
{
	char *buf = ecalloc(256, sizeof(char));
	POSSIBLY_UNUSED char *bbuf = buf;
#ifdef STRERROR_R_CHAR_P
	bbuf =
#endif
		strerror_r(err, buf, 256);
	return bbuf;
}

static void mosquitto_client_object_destroy(void *object TSRMLS_DC)
{
	mosquitto_client_object *client = (mosquitto_client_object *) object;

	/* Disconnect cleanly, but disregard an error if it wasn't connected */
	/* We must loop here so that the disconnect packet is sent and acknowledged */
	mosquitto_disconnect(client->client);
	mosquitto_loop(client->client, 100, 1);

	zend_hash_destroy(client->std.properties);
	FREE_HASHTABLE(client->std.properties);
	mosquitto_destroy(client->client);
	efree(object);
}

static zend_object_value mosquitto_client_object_new(zend_class_entry *ce TSRMLS_DC) {

	zend_object_value retval;
	mosquitto_client_object *client;
#if PHP_VERSION_ID < 50399
	zval *temp;
#endif

	client = ecalloc(1, sizeof(mosquitto_client_object));
	client->std.ce = mosquitto_ce_client;
	client->client = NULL;

#ifdef ZTS
	client->TSRMLS_C = TSRMLS_C;
#endif

	ALLOC_HASHTABLE(client->std.properties);
	zend_hash_init(client->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
#if PHP_VERSION_ID < 50399
	zend_hash_copy(client->std.properties, &mosquitto_ce_client->default_properties, (copy_ctor_func_t) zval_add_ref,(void *) &temp, sizeof(zval *));
#else
	object_properties_init(&client->std, mosquitto_ce_client);
#endif
	retval.handle = zend_objects_store_put(client, NULL, (zend_objects_free_object_storage_t) mosquitto_client_object_destroy, NULL TSRMLS_CC);
	retval.handlers = &mosquitto_std_object_handlers;
	return retval;
}

void php_mosquitto_handle_errno(int retval, int err TSRMLS_DC) {
	char *message = NULL;

	switch (retval) {
		case MOSQ_ERR_SUCCESS:
		default:
			return;

		case MOSQ_ERR_INVAL:
			message = estrdup("Invalid input parameter");
			break;

		case MOSQ_ERR_NOMEM:
			message = estrdup("Insufficient memory");
			break;

		case MOSQ_ERR_NO_CONN:
			message = estrdup("The client is not connected to a broker");
			break;

		case MOSQ_ERR_CONN_LOST:
			message = estrdup("Connection lost");
			break;

		case MOSQ_ERR_PROTOCOL:
			message = estrdup("There was a protocol error communicating with the broker.");
			break;

		case MOSQ_ERR_PAYLOAD_SIZE:
			message = estrdup("Payload is too large");
			break;

		case MOSQ_ERR_ERRNO:
			message = php_mosquitto_strerror_wrapper(errno);
			break;
	}

	zend_throw_exception(mosquitto_ce_exception, message, 0 TSRMLS_CC);
}

PHP_MOSQUITTO_API void php_mosquitto_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	mosquitto_client_object *object = (mosquitto_client_object *) obj;
	zval *retval_ptr = NULL;
	zval **params = ecalloc(1, sizeof (zval));
#ifdef ZTS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->connect_callback)) {
		return;
	}

	ALLOC_INIT_ZVAL(params[0]);
	ZVAL_LONG(params[0], rc);

	object->connect_callback.params = &params;
	object->connect_callback.param_count = 1;
	object->connect_callback.retval_ptr_ptr = &retval_ptr;
	object->connect_callback.no_separation = 1;

	if (zend_call_function(&object->connect_callback, &object->connect_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke connect callback %s()", Z_STRVAL_P(object->connect_callback.function_name));
		}
	}

	zval_ptr_dtor(params);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
}

PHP_MOSQUITTO_API void php_mosquitto_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	mosquitto_client_object *object = (mosquitto_client_object *) obj;
	zval *retval_ptr = NULL;
	zval **params = ecalloc(1, sizeof (zval));
#ifdef ZTS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->disconnect_callback)) {
		return;
	}

	ALLOC_INIT_ZVAL(params[0]);
	ZVAL_LONG(params[0], rc);

	object->disconnect_callback.params = &params;
	object->disconnect_callback.param_count = 1;
	object->disconnect_callback.retval_ptr_ptr = &retval_ptr;
	object->disconnect_callback.no_separation = 1;

	if (zend_call_function(&object->disconnect_callback, &object->disconnect_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke disconnect callback %s()", Z_STRVAL_P(object->disconnect_callback.function_name));
		}
	}

	zval_ptr_dtor(params);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
}

PHP_MOSQUITTO_API void php_mosquitto_message_callback(struct mosquitto *mosq, void *client_obj, const struct mosquitto_message *message)
{
	mosquitto_client_object *object = (mosquitto_client_object *) client_obj;
	mosquitto_message_object *message_object;
	zval *retval_ptr = NULL, *message_zval = NULL;
	zval **params[1];
#ifdef ZTS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->message_callback)) {
		return;
	}

	ALLOC_INIT_ZVAL(message_zval);
	object_init_ex(message_zval, mosquitto_ce_message);
	message_object = (mosquitto_message_object *) zend_object_store_get_object(message_zval TSRMLS_CC);
	mosquitto_message_copy(&message_object->message, message);
	params[0] = &message_zval;

	object->message_callback.params = params;
	object->message_callback.param_count = 1;
	object->message_callback.retval_ptr_ptr = &retval_ptr;
	object->message_callback.no_separation = 1;

	if (zend_call_function(&object->message_callback, &object->message_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke message callback %s()", Z_STRVAL_P(object->message_callback.function_name));
		}
	}

	zval_ptr_dtor(&message_zval);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
}

PHP_MOSQUITTO_API void php_mosquitto_subscribe_callback(struct mosquitto *mosq, void *client_obj, int mid, int qos_count, const int *granted_qos)
{
	mosquitto_client_object *object = (mosquitto_client_object *) client_obj;
	zval *retval_ptr = NULL;
	zval *mid_zval, *qos_count_zval;
	zval **params[2];
#ifdef ZTS
	TSRMLS_D = object->TSRMLS_C;
#endif

	if (!ZEND_FCI_INITIALIZED(object->subscribe_callback)) {
		return;
	}

	MAKE_STD_ZVAL(mid_zval);
	MAKE_STD_ZVAL(qos_count_zval);
	ZVAL_LONG(mid_zval, mid);
	ZVAL_LONG(qos_count_zval, qos_count);
	/* TODO: handle granted_qos */
	params[0] = &mid_zval;
	params[1] = &qos_count_zval;

	object->subscribe_callback.params = params;
	object->subscribe_callback.param_count = 2;
	object->subscribe_callback.retval_ptr_ptr = &retval_ptr;
	object->subscribe_callback.no_separation = 1;

	if (zend_call_function(&object->subscribe_callback, &object->subscribe_callback_cache TSRMLS_CC) == FAILURE) {
		if (!EG(exception)) {
			zend_throw_exception_ex(mosquitto_ce_exception, 0 TSRMLS_CC, "Failed to invoke subscribe callback %s()", Z_STRVAL_P(object->subscribe_callback.function_name));
		}
	}

	zval_ptr_dtor(params[0]);
	zval_ptr_dtor(params[1]);

	if (retval_ptr != NULL) {
		zval_ptr_dtor(&retval_ptr);
	}
}

/* {{{ mosquitto_client_methods */
const zend_function_entry mosquitto_client_methods[] = {
	PHP_ME(Mosquitto_Client, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(Mosquitto_Client, onConnect, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onDisconnect, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onSubscribe, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, onMessage, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setCredentials, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setWill, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, clearWill, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setReconnectDelay, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setMessageRetry, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, connect, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, disconnect, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, setMaxInFlightMessages, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, publish, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, subscribe, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, loop, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Mosquitto_Client, loopForever, NULL, ZEND_ACC_PUBLIC)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ mosquitto_functions[] */
const zend_function_entry mosquitto_functions[] = {
	PHP_FE_END	/* Must be the last line in mosquitto_functions[] */
};
/* }}} */

/* {{{ mosquitto_module_entry */
zend_module_entry mosquitto_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"mosquitto",
	NULL,
	PHP_MINIT(mosquitto),
	PHP_MSHUTDOWN(mosquitto),
	NULL,
	NULL,
	PHP_MINFO(mosquitto),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_MOSQUITTO
ZEND_GET_MODULE(mosquitto)
#endif

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(mosquitto)
{
	memcpy(&mosquitto_std_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
	mosquitto_std_object_handlers.clone_obj = NULL;

	zend_class_entry client_ce, exception_ce;
	INIT_NS_CLASS_ENTRY(client_ce, "Mosquitto", "Client", mosquitto_client_methods);
	mosquitto_ce_client = zend_register_internal_class_ex(&client_ce, NULL, NULL TSRMLS_CC);
	mosquitto_ce_client->create_object = mosquitto_client_object_new;

	INIT_NS_CLASS_ENTRY(exception_ce, "Mosquitto", "Exception", NULL);
	mosquitto_ce_exception = zend_register_internal_class_ex(&exception_ce,
			zend_exception_get_default(TSRMLS_C), "Exception" TSRMLS_CC);

	mosquitto_lib_init();

	PHP_MINIT(mosquitto_message)(INIT_FUNC_ARGS_PASSTHRU);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(mosquitto)
{
	mosquitto_lib_cleanup();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(mosquitto)
{
	char tmp[16];
	snprintf(tmp, sizeof(tmp), "%d.%d.%d", LIBMOSQUITTO_MAJOR, LIBMOSQUITTO_MINOR, LIBMOSQUITTO_REVISION);

	php_info_print_table_start();
	php_info_print_table_header(2, "Mosquitto support", "enabled");
	php_info_print_table_row(2, "libmosquitto version", tmp);
	php_info_print_table_end();
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
