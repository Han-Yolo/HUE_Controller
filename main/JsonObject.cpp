#include "JsonObject.h"

#include <esp_log.h>

#include <stdio.h>


#define LOG_TAG "JsonObject"


JsonObject::JsonObject(const char* jsonString)
{
    m_Root = json_parse(jsonString, strlen(jsonString));

    if(m_Root == nullptr)
    {
        ESP_LOGE(LOG_TAG, "Unable to parse JSON!");
    }
}


JsonObject::~JsonObject()
{
    json_value_free(m_Root);
}


bool JsonObject::getBool(const char** path, const uint32_t depth, 
        bool* returnValue)
{
    json_value* jsonValue;

    if(getObject(path, depth, &jsonValue) == false) return false;

    if(jsonValue->type != json_boolean) return false;

    *returnValue = jsonValue->u.boolean;
    return true;
}


bool JsonObject::getInt(const char** path, const uint32_t depth, 
        int64_t* returnValue)
{
    json_value* jsonValue;

    if(getObject(path, depth, &jsonValue) == false) return false;

    if(jsonValue->type != json_integer) return false;

    *returnValue = jsonValue->u.integer;
    return true;
}


bool JsonObject::getDouble(const char** path, const uint32_t depth, 
        double* returnValue)
{
    json_value* jsonValue;

    if(getObject(path, depth, &jsonValue) == false) return false;

    if(jsonValue->type != json_double) return false;

    *returnValue = jsonValue->u.dbl;
    return true;
}


bool JsonObject::getString(const char** path, const uint32_t depth, 
        char** returnValue)
{
    json_value* jsonValue;

    if(getObject(path, depth, &jsonValue) == false) return false;

    if(jsonValue->type != json_string) return false;

    *returnValue = jsonValue->u.string.ptr;
    return true;
}


bool JsonObject::getNumObjects(const char** path, 
        const uint32_t depth, uint32_t* returnValue)
{
    json_value* jsonValue;

    if(getObject(path, depth, &jsonValue) == false) return false;

    if(jsonValue->type != json_object) return false;

    *returnValue = jsonValue->u.object.length;
    return true;
}


void JsonObject::print(void)
{
    printValue(m_Root, 0);
}


bool JsonObject::getObject(const char** path, const uint32_t depth, 
        json_value** returnValue)
{
    json_value* jsonValue = m_Root;

    for(uint32_t currentDepth = 0; currentDepth < depth; currentDepth++)
    {
        if(jsonValue == nullptr) return false;

        if(jsonValue->type != json_object) return false;

        uint32_t objLength = jsonValue->u.object.length;

        bool nextValueFound = false;
        for(uint32_t i = 0; i < objLength; i++)
        {
            if(strcmp(path[currentDepth], 
                jsonValue->u.object.values[i].name) == 0)
            {
                jsonValue = jsonValue->u.object.values[i].value;
                nextValueFound = true;
                break;
            }
        }

        if(nextValueFound == false) return false;
    }

    *returnValue = jsonValue;
    return true;
}


void JsonObject::printValue(json_value* value, uint32_t depth)
{
    if(value == nullptr) return;

    if(value->type != json_object) 
    {
        printDepthShift(depth);
    }

    switch(value->type)
    {
        case json_none:
            printf("none\n");
            break;

        case json_object:
            printObject(value, depth+1);
            break;

        case json_array:
            printArray(value, depth+1);
            break;

        case json_integer:
            printf("int: %d\n", (int32_t)value->u.integer);
            break;

        case json_double:
            printf("double: %f\n", value->u.dbl);
            break;

        case json_string:
            printf("string: %s\n", value->u.string.ptr);
            break;

        case json_boolean:
            printf("bool: %d\n", value->u.boolean);
            break;

        default:
            break;
    }
}


void JsonObject::printObject(json_value* value, uint32_t depth)
{
    if(value == NULL) return;

    for(uint32_t i = 0; i < value->u.object.length; i++)
    {
        printDepthShift(depth);
        printf("object[%d].name = %s\n", i, value->u.object.values[i].name);
        printValue(value->u.object.values[i].value, depth+1);
    }
}


void JsonObject::printArray(json_value* value, uint32_t depth)
{
    if (value == NULL) return;

    printf("array\n");

    for(uint32_t i = 0; i < value->u.array.length; i++)
    {
        printValue(value->u.array.values[i], depth);
    }
}


void JsonObject::printDepthShift(uint32_t depth)
{
    for(uint32_t i = 0; i < depth; i++)
    {
        printf(" ");
    }
}
