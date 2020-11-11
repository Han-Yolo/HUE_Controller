#ifndef JSONOBJECT_H
#define JSONOBJECT_H


#include "json.h"


class JsonObject
{
public:

    JsonObject(const char* jsonString);
    ~JsonObject();

    bool getBool(const char** path, const uint32_t depth, 
            bool* returnValue);

    bool getInt(const char** path, const uint32_t depth, 
            int64_t* returnValue);

    bool getDouble(const char** path, const uint32_t depth, 
            double* returnValue);

    bool getString(const char** path, const uint32_t depth, 
            char** returnValue);

    bool getNumObjects(const char** path, const uint32_t depth,
            uint32_t* returnValue);

    void print(void);

private:

    bool getObject(const char** path, const uint32_t depth, 
            json_value** returnValue);

    void printValue(json_value* value, uint32_t depth);
    void printObject(json_value* value, uint32_t depth);
    void printArray(json_value* value, uint32_t depth);
    void printDepthShift(uint32_t depth);

    json_value* m_Root;
};


#endif /* JSONOBJECT_H */
