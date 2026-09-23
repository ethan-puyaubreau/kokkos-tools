#include <atomic>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <nvml.h>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>

namespace KokkosTools::EnergyProfiler {

namespace {
bool g_running = false;
std::thread g_sampler_thread;

std::string g_out_dir = "./";
int g_mpi_rank = -1;

std::ofstream g_events_file;
std::ofstream g_power_file;

std::vector<nvmlDevice_t> g_nvml_devices;
bool g_nvml_ok = false;
std::atomic<uint64_t> g_next_event_id{1};

struct ActiveEvent {
  uint64_t id;
  uint64_t parent_id;
  std::string name;
  std::string category;
  uint64_t start_ns;
};

struct FinishedEvent {
  uint64_t id;
  uint64_t parent_id;
  std::string name;
  std::string category;
  uint64_t start_ns;
  uint64_t end_ns;
};

struct ThreadEventBuffer {
  std::vector<ActiveEvent> active_stack;
  std::vector<FinishedEvent> finished_events;

  ThreadEventBuffer() {
    active_stack.reserve(64);
    finished_events.reserve(4096);
  }
};

std::mutex g_registry_mutex;
std::vector<ThreadEventBuffer*> g_thread_buffers;
thread_local ThreadEventBuffer* t_buffer = nullptr;

inline ThreadEventBuffer& get_thread_buffer() {
  if (!t_buffer) {
    t_buffer = new ThreadEventBuffer();
    std::lock_guard<std::mutex> lock(g_registry_mutex);
    g_thread_buffers.push_back(t_buffer);
  }
  return *t_buffer;
}

inline uint64_t now_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

std::string get_current_hostname() {
  char buf[256] = {0};
  if (gethostname(buf, sizeof(buf) - 1) == 0) {
    return std::string(buf);
  }
  return "unknown_host";
}

std::string get_current_app_name() {
  char buf[PATH_MAX] = {0};
  ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (len > 0) {
    std::string path(buf, len);
    auto pos = path.find_last_of('/');
    return (pos != std::string::npos) ? path.substr(pos + 1) : path;
  }
  return "kokkos_app";
}

int detect_mpi_rank() {
  const char *rank_vars[] = {
      "OMPI_COMM_WORLD_RANK",
      "PMI_RANK",
      "MV2_COMM_WORLD_RANK",
      "SLURM_PROCID",
  };
  for (const char *var : rank_vars) {
    const char *val = std::getenv(var);
    if (val && *val) {
      try {
        return std::stoi(val);
      } catch (...) {
        return -1;
      }
    }
  }
  return -1;
}

void sampler_loop() {
  const auto interval = std::chrono::milliseconds(20); // 50 Hz
  auto next_tick = std::chrono::steady_clock::now();

  while (g_running) {
    next_tick += interval;

    if (g_nvml_ok && g_power_file.is_open()) {
      uint64_t ts = now_ns();
      for (size_t i = 0; i < g_nvml_devices.size(); ++i) {
        unsigned int power_mw = 0;
        if (nvmlDeviceGetPowerUsage(g_nvml_devices[i], &power_mw) == NVML_SUCCESS) {
          double watts = static_cast<double>(power_mw) / 1000.0;
          g_power_file << ts << ",GPU," << i << "," << watts << ",\n";
        }
      }
      g_power_file.flush();
    }

    std::this_thread::sleep_until(next_tick);
  }
}

void write_metadata() {
  std::ofstream meta(g_out_dir + "/metadata.json");
  if (meta.is_open()) {
    meta << "{\n"
         << "  \"spec_version\": \"1.0\",\n"
         << "  \"app_name\": \"" << get_current_app_name() << "\",\n"
         << "  \"hostname\": \"" << get_current_hostname() << "\",\n"
         << "  \"kokkos_backend\": \"CUDA\",\n"
         << "  \"start_epoch_ns\": " << now_ns();
    if (g_mpi_rank >= 0) {
      meta << ",\n  \"mpi_rank\": " << g_mpi_rank;
    }
    meta << "\n}\n";
  }
}

} // namespace

void init() {
  const char *out_env = std::getenv("KOKKOS_TOOLS_OUTPUT_PATH");
  if (out_env && *out_env) {
    g_out_dir = out_env;
  }

  g_mpi_rank = detect_mpi_rank();
  if (g_mpi_rank >= 0) {
    g_out_dir = (std::filesystem::path(g_out_dir) / ("rank_" + std::to_string(g_mpi_rank))).string();
  }

  std::error_code ec;
  std::filesystem::create_directories(g_out_dir, ec);
  if (ec) {
    std::cerr << "[kokkos-energy-profiler] Error creating output directory "
              << g_out_dir << ": " << ec.message() << "\n";
  }

  g_events_file.open(g_out_dir + "/events.csv");
  g_power_file.open(g_out_dir + "/power_samples.csv");

  if (g_events_file.is_open()) {
    g_events_file << "id,parent_id,name,category,start_ns,end_ns\n";
  }
  if (g_power_file.is_open()) {
    g_power_file << "timestamp_ns,domain,device_id,power_watts,energy_joules\n";
  }

  write_metadata();

  // Init NVML
  if (nvmlInit() == NVML_SUCCESS) {
    unsigned int dev_count = 0;
    if (nvmlDeviceGetCount(&dev_count) == NVML_SUCCESS && dev_count > 0) {
      for (unsigned int i = 0; i < dev_count; ++i) {
        nvmlDevice_t dev;
        if (nvmlDeviceGetHandleByIndex(i, &dev) == NVML_SUCCESS) {
          g_nvml_devices.push_back(dev);
        }
      }
      if (!g_nvml_devices.empty()) {
        g_nvml_ok = true;
        std::cout << "[kokkos-energy-profiler] NVML initialized with "
                  << g_nvml_devices.size() << " GPU device(s)\n";
      }
    }
  } else {
    std::cerr << "[kokkos-energy-profiler] Warning: NVML init failed, running without GPU telemetry\n";
  }

  g_running = true;
  g_sampler_thread = std::thread(sampler_loop);
}

std::string escape_csv_field(const std::string &field) {
  bool needs_quotes = false;
  std::string escaped;
  escaped.reserve(field.size() + 8);

  for (char c : field) {
    if (c == '"') {
      needs_quotes = true;
      escaped += "\"\"";
    } else {
      if (c == ',' || c == '\n' || c == '\r') {
        needs_quotes = true;
      }
      escaped += c;
    }
  }

  if (needs_quotes) {
    return "\"" + escaped + "\"";
  }
  return escaped;
}

void finalize() {
  g_running = false;
  if (g_sampler_thread.joinable()) {
    g_sampler_thread.join();
  }

  if (g_nvml_ok) {
    nvmlShutdown();
  }

  // Flush all buffered thread events to events.csv
  if (g_events_file.is_open()) {
    std::lock_guard<std::mutex> lock(g_registry_mutex);
    for (ThreadEventBuffer *buf : g_thread_buffers) {
      if (!buf) continue;
      // Close any unclosed active events
      uint64_t t_now = now_ns();
      while (!buf->active_stack.empty()) {
        ActiveEvent &ev = buf->active_stack.back();
        buf->finished_events.push_back({ev.id, ev.parent_id, std::move(ev.name), std::move(ev.category), ev.start_ns, t_now});
        buf->active_stack.pop_back();
      }

      for (const auto &ev : buf->finished_events) {
        g_events_file << ev.id << "," << ev.parent_id << ","
                      << escape_csv_field(ev.name) << ","
                      << ev.category << "," << ev.start_ns << ","
                      << ev.end_ns << "\n";
      }
      delete buf;
    }
    g_thread_buffers.clear();
    g_events_file.close();
  }

  if (g_power_file.is_open()) g_power_file.close();

  std::cout << "[kokkos-energy-profiler] Profiling complete. Traces written to: " << g_out_dir << "\n";
}

void push_event(const char *name, const char *cat, uint64_t *kID) {
  uint64_t id = g_next_event_id.fetch_add(1, std::memory_order_relaxed);
  if (kID) *kID = id;

  ThreadEventBuffer &buf = get_thread_buffer();
  uint64_t parent_id = buf.active_stack.empty() ? 0 : buf.active_stack.back().id;
  uint64_t t0 = now_ns();

  buf.active_stack.push_back({id, parent_id, name ? name : "unnamed", cat ? cat : "", t0});
}

void pop_event(uint64_t kID) {
  uint64_t t1 = now_ns();
  ThreadEventBuffer &buf = get_thread_buffer();

  if (buf.active_stack.empty()) return;

  if (kID == 0 || buf.active_stack.back().id == kID) {
    ActiveEvent ev = std::move(buf.active_stack.back());
    buf.active_stack.pop_back();
    buf.finished_events.push_back({ev.id, ev.parent_id, std::move(ev.name), std::move(ev.category), ev.start_ns, t1});
    return;
  }

  for (auto it = buf.active_stack.rbegin(); it != buf.active_stack.rend(); ++it) {
    if (it->id == kID) {
      ActiveEvent ev = std::move(*it);
      auto forward_it = it.base() - 1;
      buf.active_stack.erase(forward_it);
      buf.finished_events.push_back({ev.id, ev.parent_id, std::move(ev.name), std::move(ev.category), ev.start_ns, t1});
      break;
    }
  }
}

} // namespace KokkosTools::EnergyProfiler

// C Interface expected by KokkosP runtime
extern "C" {

void kokkosp_init_library(const int, const uint64_t, const uint32_t, void*) {
  KokkosTools::EnergyProfiler::init();
}

void kokkosp_finalize_library() {
  KokkosTools::EnergyProfiler::finalize();
}

void kokkosp_push_profile_region(const char *name) {
  KokkosTools::EnergyProfiler::push_event(name, "USER_REGION", nullptr);
}

void kokkosp_pop_profile_region() {
  KokkosTools::EnergyProfiler::pop_event(0);
}

void kokkosp_begin_parallel_for(const char *name, uint32_t, uint64_t *kID) {
  KokkosTools::EnergyProfiler::push_event(name, "PARALLEL_FOR", kID);
}

void kokkosp_end_parallel_for(uint64_t kID) {
  KokkosTools::EnergyProfiler::pop_event(kID);
}

void kokkosp_begin_parallel_reduce(const char *name, uint32_t, uint64_t *kID) {
  KokkosTools::EnergyProfiler::push_event(name, "PARALLEL_REDUCE", kID);
}

void kokkosp_end_parallel_reduce(uint64_t kID) {
  KokkosTools::EnergyProfiler::pop_event(kID);
}

void kokkosp_begin_parallel_scan(const char *name, uint32_t, uint64_t *kID) {
  KokkosTools::EnergyProfiler::push_event(name, "PARALLEL_SCAN", kID);
}

void kokkosp_end_parallel_scan(uint64_t kID) {
  KokkosTools::EnergyProfiler::pop_event(kID);
}

} // extern "C"
