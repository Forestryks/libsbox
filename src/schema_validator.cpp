/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "schema_validator.h"
#include "utils.h"
#include "context_manager.h"

#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>

SchemaValidator::SchemaValidator(const char *json_schema) {
    rapidjson::Document document;
    document.Parse(json_schema);
    if (document.HasParseError()) {
        error_ = format(
            "Failed to parse JSON Schema: %s (at %zi)",
            GetParseError_En(document.GetParseError()),
            document.GetErrorOffset()
        );
        return;
    }

    schema_document_ = std::make_unique<rapidjson::SchemaDocument>(document);
    schema_validator_ = std::make_unique<rapidjson::SchemaValidator>(*schema_document_);

    if (!schema_validator_->IsValid()) {
        rapidjson::StringBuffer string_buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(string_buffer);
        schema_validator_->GetError().Accept(writer);
        error_ = format(
            "Invalid schema: %s",
            string_buffer.GetString());
        return;
    }
}

bool SchemaValidator::validate(const rapidjson::Document &document) {
    error_ = "";
    rapidjson::StringBuffer string_buffer1;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer1(string_buffer1);
    document.Accept(writer1);
    schema_validator_->Reset();
    if (!document.Accept(*schema_validator_)) {
        rapidjson::StringBuffer string_buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(string_buffer);
        schema_validator_->GetError().Accept(writer);
        error_ = format("Invalid document: %s", string_buffer.GetString());
        return false;
    }
    return true;
}

std::string SchemaValidator::get_error() {
    return error_;
}
