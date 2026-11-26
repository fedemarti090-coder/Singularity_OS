#pragma once
#include <stddef.h>
#include <stdint.h>

enum class TaskState : uint8_t {
    Unused = 0,
    Ready,
    Running,
    Dead
};

struct Task {
    uint32_t id;
    TaskState state;
    uint32_t esp;
    uint32_t* stack_base;
    size_t stack_size;
    const char* name;
};

class Scheduler {
public:
    static constexpr size_t MAX_TASKS = 16;
    static constexpr size_t DEFAULT_STACK_SIZE = 4096;

    static void init();
    static int32_t create(const char* name, void (*entry)(void*), void* arg = nullptr, size_t stack_size = DEFAULT_STACK_SIZE);
    static void yield();
    static void exit_current();
    static void tick();

    static size_t task_count();
    static const Task* get_task(size_t index);
    static int32_t current_id();

private:
    static Task tasks[MAX_TASKS];
    static size_t current_idx;
    static uint32_t next_pid;
    static bool initialized;

    static size_t pick_next_ready(size_t from_idx);
    static void switch_to(size_t next_idx);
};