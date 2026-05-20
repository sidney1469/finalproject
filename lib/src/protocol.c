#include "protocol.h"

const struct json_obj_descr airplane_descr[] = {
    JSON_OBJ_DESCR_PRIM(airplane_t, icao,      JSON_TOK_STRING_BUF),
    JSON_OBJ_DESCR_PRIM(airplane_t, alt,       JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(airplane_t, lat,       JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(airplane_t, lon,       JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(airplane_t, spd,       JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(airplane_t, head,      JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(airplane_t, ts, JSON_TOK_STRING_BUF),
};

const struct json_obj_descr gps_descr[] = {
    JSON_OBJ_DESCR_PRIM(gps_location_t, lat,   JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(gps_location_t, lon,  JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(gps_location_t, alt, JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(gps_location_t, valid,      JSON_TOK_TRUE),
    JSON_OBJ_DESCR_PRIM(gps_location_t, fix_seq,    JSON_TOK_NUMBER),
};

const struct json_obj_descr sensor_message_descr[] = {
    JSON_OBJ_DESCR_OBJECT(sensor_message_t, gps, gps_descr),
    JSON_OBJ_DESCR_OBJECT(sensor_message_t, plane, airplane_descr),
};

const size_t airplane_descr_len = ARRAY_SIZE(airplane_descr);
const size_t gps_descr_len = ARRAY_SIZE(gps_descr);
const size_t sensor_message_descr_len = ARRAY_SIZE(sensor_message_descr);