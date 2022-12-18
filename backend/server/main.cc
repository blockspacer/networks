#include "backend/backend_config.h"
#include "backend/server.h"
#include "core/backtrace.h"
#include "core/exception.h"
#include "storage/api.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include <fstream>

#include <csignal>
#include <unistd.h>

static backend::RpcServer* server_ptr = nullptr;

void SignalHandler(int /* signal */) {
  if (server_ptr != nullptr) {
    server_ptr->Stop();
  }
}

void RegisterSignalHandlers(backend::RpcServer* server) {
  server_ptr = server;
  std::signal(SIGTERM, SignalHandler);
  std::signal(SIGINT, SignalHandler);
}

ABSL_FLAG(bool, daemon, false, "run as daemon");
ABSL_FLAG(std::string, config, "config.ini", "config file");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);

  if (absl::GetFlag(FLAGS_daemon)) {
    daemon(0, 0);
  }

  backend::Config config;
  try {
    config = backend::LoadFromFile(absl::GetFlag(FLAGS_config));
  } catch (const core::Exception& e) {
    std::cerr << e.what() << "\n";
    core::FormatBackTrace(std::cerr);
    return 1;
  }

  std::unique_ptr<storage::IStorage> storage;
  try {
    storage = storage::CreateStorage(config.storage_config);
  } catch (const core::Exception& e) {
    std::cerr << e.what() << "\n";
    core::FormatBackTrace(std::cerr);
    return 1;
  }

  try {
    backend::InitializeLogger(config.log_config);
  } catch (const core::Exception& e) {
    std::cerr << e.what() << "\n";
    core::FormatBackTrace(std::cerr);
    return 1;
  }

  chat_server_log("Finished loading configs");

  std::ofstream pid_out(config.pid_file);
  pid_out << getpid() << std::endl;
  pid_out.close();

  chat_server_log("Server pid: " + std::to_string(getpid()));
  chat_server_log("Finished writing pidfile");

  auto server = backend::RpcServer(config.threads_num, std::move(storage));

  server.Start("localhost:" + std::to_string(config.port));
  chat_server_log("Server is listening on port: " + std::to_string(config.port));

  RegisterSignalHandlers(&server);
  server.WaitForStop();
  return 0;
}
