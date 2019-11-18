/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#ifndef LIBSBOX_SCHEMA_VALIDATOR_H
#define LIBSBOX_SCHEMA_VALIDATOR_H

#include <rapidjson/schema.h>
#include <memory>

class SchemaValidator {
public:
    explicit SchemaValidator(const char *json_schema);

    bool validate(const rapidjson::Document &document);
    std::string get_error();
private:
    std::unique_ptr<rapidjson::SchemaDocument> schema_document_;
    std::unique_ptr<rapidjson::SchemaValidator> schema_validator_;
    std::string error_;
};

#endif //LIBSBOX_SCHEMA_VALIDATOR_H
