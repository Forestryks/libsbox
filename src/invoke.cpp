/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include <json.hpp>
#include <libsbox.h>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

void print_error(const char *msg) {
    std::cout << json::object({{"error", msg}});
}

[[noreturn]]
void error(const char *msg) {
    print_error(msg);
    exit(1);
}

[[noreturn]]
void error(const std::string &msg) {
    print_error(msg.c_str());
    exit(1);
}

libsbox::execution_target *target_from_json(json &jtarget) {
    auto target = new libsbox::execution_target(jtarget["argv"].get<std::vector<std::string>>());
    target->time_limit = jtarget["time_limit"];
    target->memory_limit = jtarget["memory_limit"];
    target->fsize_limit = jtarget["fsize_limit"];
    target->max_files = jtarget["max_files"];
    target->max_threads = jtarget["max_threads"];
    if (jtarget["stdin"].is_string()) {
        target->stdin.freopen(jtarget["stdin"]);
    }
    if (jtarget["stdout"].is_string()) {
        target->stdout.freopen(jtarget["stdout"]);
    }
    if (jtarget["stderr"].is_string()) {
        target->stderr.freopen(jtarget["stderr"]);
    }
    if (jtarget["use_standard_rules"].is_boolean() && jtarget["use_standard_rules"].get<bool>()) {
        target->add_standard_rules();
    }
    for (auto &jrule : jtarget["bind_rules"]) {
        target->add_bind_rule(jrule["inside"], jrule["outside"], jrule["flags"]);
    }
    return target;
}

libsbox::execution_context *context_from_json(json &jcontext) {
    auto context = new libsbox::execution_context();
    context->wall_time_limit = jcontext["wall_time_limit"];
    context->first_uid = jcontext["first_uid"];

    std::vector<libsbox::execution_target *> targets;
    for (auto &jtarget : jcontext["targets"]) {
        auto target = target_from_json(jtarget);
        targets.push_back(target);
        context->register_target(target);
    }

    for (auto &jpipe : jcontext["pipes"]) {
        libsbox::out_stream *write_end;
        libsbox::in_stream *read_end;
        int flags = 0;

        if (!jpipe["flags"].is_null()) {
            flags = jpipe["flags"];
        }

        int id = jpipe["write_end"]["id"];
        if (jpipe["write_end"]["stream"] == "stdout") {
            write_end = &targets[id]->stdout;
        } else if (jpipe["write_end"]["stream"] == "stderr") {
            write_end = &targets[id]->stderr;
        } else {
            error("Unknown output stream: " + (std::string) jpipe["write_end"]["stream"]);
        }

        id = jpipe["read_end"]["id"];
        if (jpipe["read_end"]["stream"] == "stdin") {
            read_end = &targets[id]->stdin;
        } else {
            error("Unknown input stream: " + (std::string) jpipe["read_end"]["stream"]);
        }

        context->link(write_end, read_end, flags);
    }

    return context;
}

json json_from_target(libsbox::execution_target *target) {
    json jtarget = json::object({
        {"time_usage", target->time_usage},
        {"time_usage_sys", target->time_usage_sys},
        {"time_usage_user", target->time_usage_user},
        {"wall_time_usage", target->wall_time_usage},
        {"memory_usage", target->memory_usage},
        {"time_limit_exceeded", target->time_limit_exceeded},
        {"wall_time_limit_exceeded", target->wall_time_limit_exceeded},
        {"exited", target->exited},
        {"exit_code", target->exit_code},
        {"signaled", target->signaled},
        {"term_signal", target->term_signal},
        {"oom_killed", target->oom_killed}

    });
    return jtarget;
}

json json_from_context(libsbox::execution_context *context) {
    json jcontext = json::object({
        {"wall_time_usage", context->wall_time_usage},
        {"targets", json::array()}
    });
    for (auto target : context->targets) {
        jcontext["targets"].push_back(json_from_target(target));
    }
    return jcontext;
}

int main(int argc, char *argv[]) {
    libsbox::init(print_error);

    json obj;
    try {
        if (argc == 1) {
            std::cin >> obj;
        } else {
            std::ifstream in(argv[1]);
            if (!in.is_open()) {
                error(std::string() + "Cannot open file " + argv[1]);
            }
            in >> obj;
        }
    } catch (json::parse_error &e) {
        error(e.what());
    }

    libsbox::execution_context *context;
    try {
        context = context_from_json(obj);
    } catch (json::type_error &e) {
        error(e.what());
    }

    context->run();

    try {
        std::cout << json_from_context(context) << std::endl;
    } catch (json::type_error &e) {
        error(e.what());
    }
}
