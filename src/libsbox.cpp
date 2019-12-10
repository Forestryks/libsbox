/*
 * Copyright (c) 2019 Andrei Odintsov <forestryks1@gmail.com>
 */

#include "utils.h"
#include "context_manager.h"
#include "schema_validator.h"
#include "generated/response_schema.h"

#include <libsbox.h>
#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <rapidjson/error/en.h>

using namespace libsbox;

BindRule::BindRule(const std::string &inside, const std::string &outside)
    : inside_(inside), outside_(outside), flags_(0) {}

BindRule::BindRule(const std::string &inside, const std::string &outside, int flags)
    : inside_(inside), outside_(outside), flags_(flags) {}

BindRule &BindRule::allow_write() {
    flags_ |= WRITABLE;
    return (*this);
}

BindRule &BindRule::allow_dev() {
    flags_ |= DEV_ALLOWED;
    return (*this);
}

BindRule &BindRule::forbid_exec() {
    flags_ |= EXEC_FORBIDDEN;
    return (*this);
}

BindRule &BindRule::allow_suid() {
    flags_ |= SUID_ALLOWED;
    return (*this);
}

BindRule &BindRule::make_optional() {
    flags_ |= OPTIONAL;
    return (*this);
}

const std::string &BindRule::get_inside_path() const {
    return inside_;
}

const std::string &BindRule::get_outside_path() const {
    return outside_;
}

int BindRule::get_flags() const {
    return flags_;
}

Pipe::Pipe() {
    static int32_t counter = 0;
    name_ = "pipe-" + std::to_string(counter++);
}

const std::string &Pipe::get_name() const {
    return name_;
}

void Stream::disable() {
    filename_.clear();
}

void Stream::use_pipe(const Pipe &pipe) {
    filename_ = "@" + pipe.get_name();
}

void Stream::use_file(const std::string &filename) {
    filename_ = filename;
}

const std::string &Stream::get_filename() const {
    return filename_;
}

void StderrStream::use_stdout() {
    filename_ = "@_stdout";
}

time_ms_t Task::get_time_limit_ms() const {
    return time_limit_ms_;
}

void Task::set_time_limit_ms(time_ms_t time_limit_ms) {
    time_limit_ms_ = time_limit_ms;
}

time_ms_t Task::get_wall_time_limit_ms() const {
    return wall_time_limit_ms_;
}

void Task::set_wall_time_limit_ms(time_ms_t wall_time_limit_ms) {
    wall_time_limit_ms_ = wall_time_limit_ms;
}

memory_kb_t Task::get_memory_limit_kb() const {
    return memory_limit_kb_;
}

void Task::set_memory_limit_kb(memory_kb_t memory_limit_kb) {
    memory_limit_kb_ = memory_limit_kb;
}

memory_kb_t Task::get_fsize_limit_kb() const {
    return fsize_limit_kb_;
}

void Task::set_fsize_limit_kb(memory_kb_t fsize_limit_kb) {
    fsize_limit_kb_ = fsize_limit_kb;
}

int32_t Task::get_max_files() const {
    return max_files_;
}

void Task::set_max_files(int32_t max_files) {
    max_files_ = max_files;
}

int32_t Task::get_max_threads() const {
    return max_threads_;
}

void Task::set_max_threads(int32_t max_threads) {
    max_threads_ = max_threads;
}

bool Task::get_need_ipc() const {
    return need_ipc_;
}

void Task::set_need_ipc(bool need_ipc) {
    need_ipc_ = need_ipc;
}

bool Task::get_use_standard_binds() const {
    return use_standard_binds_;
}

void Task::set_use_standard_binds(bool use_standard_binds) {
    use_standard_binds_ = use_standard_binds;
}

Stream &Task::get_stdin() {
    return stdin_;
}

Stream &Task::get_stdout() {
    return stdout_;
}

StderrStream &Task::get_stderr() {
    return stderr_;
}

std::vector<std::string> &Task::get_env() {
    return env_;
}

const std::vector<std::string> &Task::get_env() const {
    return env_;
}

std::vector<BindRule> &Task::get_binds() {
    return binds_;
}

const std::vector<BindRule> &Task::get_binds() const {
    return binds_;
}

const std::vector<std::string> &Task::get_argv() const {
    return argv_;
}

void Task::set_argv(const std::vector<std::string> &argv) {
    argv_ = argv;
}

time_ms_t Task::get_time_usage_ms() const {
    return time_usage_ms_;
}

void Task::set_time_usage_ms(time_ms_t time_usage_ms) {
    time_usage_ms_ = time_usage_ms;
}

time_ms_t Task::get_time_usage_sys_ms() const {
    return time_usage_sys_ms_;
}

void Task::set_time_usage_sys_ms(time_ms_t time_usage_sys_ms) {
    time_usage_sys_ms_ = time_usage_sys_ms;
}

time_ms_t Task::get_time_usage_user_ms() const {
    return time_usage_user_ms_;
}

void Task::set_time_usage_user_ms(time_ms_t time_usage_user_ms) {
    time_usage_user_ms_ = time_usage_user_ms;
}

time_ms_t Task::get_wall_time_usage_ms() const {
    return wall_time_usage_ms_;
}

void Task::set_wall_time_usage_ms(time_ms_t wall_time_usage_ms) {
    wall_time_usage_ms_ = wall_time_usage_ms;
}

memory_kb_t Task::get_memory_usage_kb() const {
    return memory_usage_kb_;
}

void Task::set_memory_usage_kb(memory_kb_t memory_usage_kb) {
    memory_usage_kb_ = memory_usage_kb;
}

bool Task::is_time_limit_exceeded() const {
    return time_limit_exceeded_;
}

void Task::set_time_limit_exceeded(bool time_limit_exceeded) {
    time_limit_exceeded_ = time_limit_exceeded;
}

bool Task::is_wall_time_limit_exceeded() const {
    return wall_time_limit_exceeded_;
}

void Task::set_wall_time_limit_exceeded(bool wall_time_limit_exceeded) {
    wall_time_limit_exceeded_ = wall_time_limit_exceeded;
}

bool Task::exited() const {
    return exited_;
}

void Task::set_exited(bool exited) {
    exited_ = exited;
}

int Task::get_exit_code() const {
    return exit_code_;
}

void Task::set_exit_code(int exit_code) {
    exit_code_ = exit_code;
}

bool Task::signaled() const {
    return signaled_;
}

void Task::set_signaled(bool signaled) {
    signaled_ = signaled;
}

int Task::get_term_signal() const {
    return term_signal_;
}

void Task::set_term_signal(int term_signal) {
    term_signal_ = term_signal;
}

bool Task::is_oom_killed() const {
    return oom_killed_;
}

void Task::set_oom_killed(bool oom_killed) {
    oom_killed_ = oom_killed;
}

bool Task::is_memory_limit_hit() const {
    return memory_limit_hit_;
}

void Task::set_memory_limit_hit(bool memory_limit_hit) {
    memory_limit_hit_ = memory_limit_hit;
}

#define KEY(s) writer.Key(s)
#define STRING(s) writer.String(s.c_str(), s.size(), true)
#define INT(x) writer.Int(x)
#define INT64(x) writer.Int64(x)
#define BOOL(x) writer.Bool(x)
#define ARRAY(var, statement) do { writer.StartArray(); for (const auto &it : var) {statement;} writer.EndArray(); } while (0)

template<>
void BindRule::serialize_request(rapidjson::Writer<rapidjson::StringBuffer> &writer) const {
    writer.StartObject();
    KEY("inside");
    STRING(inside_);
    KEY("outside");
    STRING(outside_);
    KEY("flags");
    INT(flags_);
    writer.EndObject();
}

template<>
void Stream::serialize_request(rapidjson::Writer<rapidjson::StringBuffer> &writer) const {
    STRING(filename_);
}

template<>
void Task::serialize_request(rapidjson::Writer<rapidjson::StringBuffer> &writer) const {
    writer.StartObject();
    KEY("time_limit_ms");
    INT64(time_limit_ms_);
    KEY("wall_time_limit_ms");
    INT64(wall_time_limit_ms_);
    KEY("memory_limit_kb");
    INT64(memory_limit_kb_);
    KEY("fsize_limit_kb");
    INT64(fsize_limit_kb_);
    KEY("max_files");
    INT(max_files_);
    KEY("max_threads");
    INT(max_threads_);
    KEY("need_ipc");
    BOOL(need_ipc_);
    KEY("use_standard_binds");
    BOOL(use_standard_binds_);
    KEY("stdin");
    stdin_.serialize_request(writer);
    KEY("stdout");
    stdout_.serialize_request(writer);
    KEY("stderr");
    stderr_.serialize_request(writer);
    KEY("argv");
    ARRAY(argv_, STRING(it));
    KEY("env");
    ARRAY(env_, STRING(it));
    KEY("binds");
    ARRAY(binds_, it.serialize_request(writer));
    writer.EndObject();
}

template<>
void Task::serialize_response(rapidjson::Writer<rapidjson::StringBuffer> &writer) const {
    writer.StartObject();
    KEY("time_usage_ms");
    INT64(time_usage_ms_);
    KEY("time_usage_sys_ms");
    INT64(time_usage_sys_ms_);
    KEY("time_usage_user_ms");
    INT64(time_usage_user_ms_);
    KEY("wall_time_usage_ms");
    INT64(wall_time_usage_ms_);
    KEY("memory_usage_kb");
    INT64(memory_usage_kb_);
    KEY("time_limit_exceeded");
    BOOL(time_limit_exceeded_);
    KEY("wall_time_limit_exceeded");
    BOOL(wall_time_limit_exceeded_);
    KEY("exited");
    BOOL(exited_);
    KEY("exit_code");
    INT(exit_code_);
    KEY("signaled");
    BOOL(signaled_);
    KEY("term_signal");
    INT(term_signal_);
    KEY("oom_killed");
    BOOL(oom_killed_);
    KEY("memory_limit_hit");
    BOOL(memory_limit_hit_);
    writer.EndObject();
}

#undef ARRAY
#undef BOOL
#undef INT64
#undef INT
#undef STRING
#undef KEY

#define ERR() die("JSON is incorrect")
#define CHECK_MEMBER(obj, key) do { if (!obj.HasMember(key)) ERR(); } while (0)
#define CHECK_TYPE(obj, type) do { if (!obj.Is##type()) ERR(); } while (0)
#define GET(to, obj, type) do { CHECK_TYPE(obj, type); to = obj.Get##type(); } while (0)
#define GET_MEMBER(to, obj, key, type) do { CHECK_MEMBER(obj, key); GET(to, obj[key], type); } while (0)

template<>
BindRule::BindRule(const rapidjson::Value &value) {
    CHECK_TYPE(value, Object);
    GET_MEMBER(inside_, value, "inside", String);
    GET_MEMBER(outside_, value, "outside", String);
    GET_MEMBER(flags_, value, "flags", Int);
}

template<>
void Stream::deserialize_request(const rapidjson::Value &value) {
    if (value.IsNull()) {
        filename_ = "";
    } else {
        CHECK_TYPE(value, String);
        GET(filename_, value, String);
    }
}

template<>
void Task::deserialize_request(const rapidjson::Value &value) {
    CHECK_TYPE(value, Object);
    GET_MEMBER(time_limit_ms_, value, "time_limit_ms", Int64);
    GET_MEMBER(wall_time_limit_ms_, value, "wall_time_limit_ms", Int64);
    GET_MEMBER(memory_limit_kb_, value, "memory_limit_kb", Int64);
    GET_MEMBER(fsize_limit_kb_, value, "fsize_limit_kb", Int64);
    GET_MEMBER(max_files_, value, "max_files", Int);
    GET_MEMBER(max_threads_, value, "max_threads", Int);
    GET_MEMBER(need_ipc_, value, "need_ipc", Bool);
    GET_MEMBER(use_standard_binds_, value, "use_standard_binds", Bool);
    CHECK_MEMBER(value, "stdin");
    stdin_.deserialize_request(value["stdin"]);
    CHECK_MEMBER(value, "stdout");
    stdout_.deserialize_request(value["stdout"]);
    CHECK_MEMBER(value, "stderr");
    stderr_.deserialize_request(value["stderr"]);
    CHECK_MEMBER(value, "argv");
    CHECK_TYPE(value["argv"], Array);
    argv_.clear();
    for (size_t i = 0; i < value["argv"].Size(); ++i) {
        CHECK_TYPE(value["argv"][i], String);
        argv_.emplace_back(value["argv"][i].GetString());
    }
    CHECK_MEMBER(value, "env");
    CHECK_TYPE(value["env"], Array);
    env_.clear();
    for (size_t i = 0; i < value["env"].Size(); ++i) {
        CHECK_TYPE(value["env"][i], String);
        env_.emplace_back(value["env"][i].GetString());
    }
    CHECK_MEMBER(value, "binds");
    CHECK_TYPE(value["binds"], Array);
    binds_.clear();
    for (size_t i = 0; i < value["binds"].Size(); ++i) {
        CHECK_TYPE(value["binds"][i], Object);
        binds_.push_back(BindRule(value["binds"][i]));
    }
}

#undef ERR
#undef CHECK_MEMBER
#undef CHECK_TYPE
#undef GET
#undef GET_MEMBER

#define ERR() do { return Error("Response JSON is incorrect"); } while (0)
#define CHECK_MEMBER(obj, key) do { if (!obj.HasMember(key)) ERR(); } while (0)
#define CHECK_TYPE(obj, type) do { if (!obj.Is##type()) ERR(); } while (0)
#define GET(to, obj, type) do { CHECK_TYPE(obj, type); to = obj.Get##type(); } while (0)
#define GET_MEMBER(to, obj, key, type) do { CHECK_MEMBER(obj, key); GET(to, obj[key], type); } while (0)

template<>
Error Task::deserialize_response(const rapidjson::Value &value) {
    CHECK_TYPE(value, Object);
    GET_MEMBER(time_usage_ms_, value, "time_usage_ms", Int64);
    GET_MEMBER(time_usage_sys_ms_, value, "time_usage_sys_ms", Int64);
    GET_MEMBER(time_usage_user_ms_, value, "time_usage_user_ms", Int64);
    GET_MEMBER(wall_time_usage_ms_, value, "wall_time_usage_ms", Int64);
    GET_MEMBER(memory_usage_kb_, value, "memory_usage_kb", Int64);
    GET_MEMBER(time_limit_exceeded_, value, "time_limit_exceeded", Bool);
    GET_MEMBER(wall_time_limit_exceeded_, value, "wall_time_limit_exceeded", Bool);
    GET_MEMBER(exited_, value, "exited", Bool);
    GET_MEMBER(exit_code_, value, "exit_code", Int);
    GET_MEMBER(signaled_, value, "signaled", Bool);
    GET_MEMBER(term_signal_, value, "term_signal", Int);
    GET_MEMBER(oom_killed_, value, "oom_killed", Bool);
    GET_MEMBER(memory_limit_hit_, value, "memory_limit_hit", Bool);
    return Error();
}

#undef ERR
#undef CHECK_MEMBER
#undef CHECK_TYPE
#undef GET
#undef GET_MEMBER

Error libsbox::run_together(const std::vector<Task *> &tasks, const std::string &socket_path) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    writer.StartObject();
    writer.Key("tasks");
    writer.StartArray();
    for (auto task : tasks) {
        task->serialize_request(writer);
    }
    writer.EndArray();
    writer.EndObject();

    std::string message = buffer.GetString();

    fd_t socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        return Error(format("Cannot create socket: %m"));
    }

    std::shared_ptr<fd_t> ptr(&socket_fd, [](const fd_t *fd) { close(*fd); });

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path.c_str(), std::min(sizeof(addr.sun_path) - 1, socket_path.size()));

    int status = connect(socket_fd, reinterpret_cast<const struct sockaddr *>(&addr), sizeof(struct sockaddr_un));
    if (status != 0) {
        return Error(format("Cannot connect to socket: %m"));
    }

    int cnt = send(socket_fd, message.c_str(), message.size() + 1, 0);
    if (cnt < 0 || static_cast<size_t>(cnt) != message.size() + 1) {
        return Error(format("Cannot send request: %m"));
    }

    std::string res;
    char buf[1024];
    while (true) {
        cnt = recv(socket_fd, buf, 1023, 0);
        if (cnt < 0) {
            return Error(format("Cannot receive response: %m"));
        }
        if (cnt == 0) {
            break;
        }
        buf[cnt] = 0;
        res += buf;
    }

    static SchemaValidator response_validator(response_schema_data);
    if (!response_validator.get_error().empty()) {
        return Error(response_validator.get_error());
    }

    rapidjson::Document document;
    if (document.Parse(res.c_str()).HasParseError()) {
        return Error(
            format(
                "Cannot parse response (offset %zi): %s",
                document.GetErrorOffset(),
                rapidjson::GetParseError_En(document.GetParseError())
            )
        );
    }

    if (!response_validator.validate(document)) {
        return Error(response_validator.get_error());
    }

    if (document.HasMember("error") && document["error"].IsString()) {
        return Error(format("[remote] %s", document["error"].GetString()));
    }

    if (document["tasks"].GetArray().Size() != tasks.size()) {
        return Error("Response JSON object 'tasks' array size is not equal to tasks count");
    }

    for (size_t i = 0; i < tasks.size(); ++i) {
        auto error = tasks[i]->deserialize_response(document["tasks"][i]);
        if (error) {
            return error;
        }
    }

    return Error();
}
