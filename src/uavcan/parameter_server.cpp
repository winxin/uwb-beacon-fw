#include <uavcan/uavcan.hpp>
#include <uavcan/protocol/param_server.hpp>
#include <parameter_flash_storage/parameter_flash_storage.h>
#include "main.h"
#include "parameter_server.hpp"
#include "parameter_enumeration.hpp"

class ParamManager : public uavcan::IParamManager
{
    void construct_full_name_for_parameter(parameter_t *param, Name &name) const
    {
        int height = parameter_tree_height(param);
        name = "/";

        while (height > 1) {
            parameter_namespace_t *ns = param->ns;

            // height - 2 because we skip root node and leaf node
            for (int i = 0; i < height - 2; i++) {
                ns = ns->parent;
            }
            name += ns->id;
            name += "/";
            height --;
        }

        name += param->id;
    }
public:
/**
 * Copy the parameter name to @ref out_name if it exists, otherwise do nothing.
 */
    void getParamNameByIndex(Index index, Name& out_name) const
    {
        parameter_t *p;
        p = parameter_find_by_index(&parameter_root, index);

        if (p != nullptr) {
            construct_full_name_for_parameter(p, out_name);
        }
    }
/**
 * Assign by name if exists.
 */
    void assignParamValue(const Name& name, const Value& value)
    {
        parameter_t *p = parameter_find(&parameter_root, name.c_str());

        /* If the parameter was not found, abort. */
        if (p == nullptr) {
            return;
        }

        if (p->type == _PARAM_TYPE_SCALAR) {
            auto *val = value.as<Value::Tag::real_value>();
            if (val) {
                parameter_scalar_set(p, *val);
            }
        } else if (p->type == _PARAM_TYPE_INTEGER) {
            auto *val = value.as<Value::Tag::integer_value>();
            if (val && *val > INT32_MIN && *val < INT32_MAX) {
                parameter_integer_set(p, *val);
            }
        } else if (p->type == _PARAM_TYPE_BOOLEAN) {
            auto *val = value.as<Value::Tag::boolean_value>();
            if (val) {
                parameter_boolean_set(p, *val);
            }
        } else if (p->type == _PARAM_TYPE_STRING) {
            auto *val = value.as<Value::Tag::string_value>();
            if (val) {
                parameter_string_set(p, val->c_str());
            }
        }
    }

/**
 * Read by name if exists, otherwise do nothing.
 */
    void readParamValue(const Name& name, Value& out_value) const
    {
        parameter_t *p = parameter_find(&parameter_root, name.c_str());

        /* If the parameter was not found, abort. */
        if (p == nullptr) {
            return;
        }

        if (!parameter_defined(p)) {
            return;
        }

        if (p->type == _PARAM_TYPE_SCALAR) {
            out_value.to<Value::Tag::real_value>() = parameter_scalar_read(p);
        } else if (p->type == _PARAM_TYPE_INTEGER) {
            out_value.to<Value::Tag::integer_value>() = parameter_integer_read(p);
        } else if (p->type == _PARAM_TYPE_BOOLEAN) {
            out_value.to<Value::Tag::boolean_value>() = parameter_boolean_read(p);
        } else if (p->type == _PARAM_TYPE_STRING) {
            char name[93];
            parameter_string_read(p, name, sizeof(p));
            out_value.to<Value::Tag::string_value>() = name;
        }
    }

/**
 * Save all params to non-volatile storage.
 * @return Negative if failed.
 */
    int saveAllParams()
    {
        size_t len = (size_t)(&_config_end - &_config_start);
        // First write the config to flash
        parameter_flash_storage_save(&_config_start, len, &parameter_root);

        // Second try to read it back, see if we failed
        if(!parameter_flash_storage_load(&parameter_root, &_config_start)) {
            return -uavcan::ErrDriver;
        }

        return 0;
    }

/**
 * Clear the non-volatile storage.
 * @return Negative if failed.
 *
 * @note Not supported on motor board, always return -1.
 */
    int eraseAllParams()
    {
        parameter_flash_storage_erase(&_config_start);
        return 0;
    }
};

int parameter_server_start(Node &node)
{
    static uavcan::ParamServer server(node);
    static ParamManager manager;
    return server.start(&manager);
}
