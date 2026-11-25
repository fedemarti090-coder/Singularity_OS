#include "task.hpp"
#include "heap.hpp"
#include "memory.hpp"
#include "serial.hpp"

extern "C" void switch_context(uint32_t* old_esp, uint32_t new_esp);
extern "C" void task_trampoline();

Task Scheduler::tasks[Scheduler::MAX_TASKS];
size_t Scheduler::current_idx = 0;
uint32_t Scheduler::next_pid = 1;
bool Scheduler::initialized = false;

namespace {

struct TaskLaunchArgs {
    void (*entry)(void*);
    void* arg;
};

} // namespace

extern "C" void task_trampoline_c(TaskLaunchArgs* launch) {
    if (launch != nullptr) {
        auto entry = launch->entry;
        auto arg = launch->arg;
        Heap::free(launch);
        if (entry != nullptr) {
            entry(arg);
        }
    }
    Scheduler::exit_current();
}

void Scheduler::init() {
    for (size_t i = 0; i < MAX_TASKS; ++i) {
        tasks[i].id = 0;
        tasks[i].state = TaskState::Unused;
        tasks[i].esp = 0;
        tasks[i].stack_base = nullptr;
        tasks[i].stack_size = 0;
        tasks[i].name = nullptr;
    }

    // Task 0: rappresenta il contesto kernel corrente già in esecuzione
    tasks[0].id = next_pid++;
    tasks[0].state = TaskState::Running;
    tasks[0].esp = 0;
    tasks[0].stack_base = nullptr;
    tasks[0].stack_size = 0;
    tasks[0].name = "kernel";

    current_idx = 0;
    initialized = true;

}

int32_t Scheduler::create(const char* name, void (*entry)(void*), void* arg, size_t stack_size) {
    if (!initialized || entry == nullptr) {
        return -1;
    }

    size_t slot = MAX_TASKS;
    for (size_t i = 0; i < MAX_TASKS; ++i) {
        if (tasks[i].state == TaskState::Unused || tasks[i].state == TaskState::Dead) {
            if (tasks[i].stack_base != nullptr) {
                Heap::free(tasks[i].stack_base);
                tasks[i].stack_base = nullptr;
            }
            slot = i;
            break;
        }
    }

    if (slot == MAX_TASKS) {
        return -1;
    }

    if (stack_size < 1024) {
        stack_size = DEFAULT_STACK_SIZE;
    }
    stack_size = (stack_size + 15u) & ~15u;

    uint32_t* stack_mem = static_cast<uint32_t*>(Heap::alloc(stack_size));
    if (stack_mem == nullptr) {
        return -1;
    }
    memset(stack_mem, 0, stack_size);

    auto* launch = static_cast<TaskLaunchArgs*>(Heap::alloc(sizeof(TaskLaunchArgs)));
    if (launch == nullptr) {
        Heap::free(stack_mem);
        return -1;
    }
    launch->entry = entry;
    launch->arg = arg;

    uint32_t* sp = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(stack_mem) + stack_size);

    *(--sp) = reinterpret_cast<uint32_t>(launch);
    *(--sp) = reinterpret_cast<uint32_t>(&task_trampoline);
    *(--sp) = 0; // ebp
    *(--sp) = 0; // edi
    *(--sp) = 0; // esi
    *(--sp) = 0; // ebx

    tasks[slot].id = next_pid++;
    tasks[slot].state = TaskState::Ready;
    tasks[slot].esp = reinterpret_cast<uint32_t>(sp);
    tasks[slot].stack_base = stack_mem;
    tasks[slot].stack_size = stack_size;
    tasks[slot].name = (name != nullptr) ? name : "task";

    Serial::print("[SCHED] Created task '", Serial::COM1);
    Serial::print(tasks[slot].name, Serial::COM1);
    Serial::print("' PID=", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(tasks[slot].id), Serial::COM1);
    Serial::print_line("", Serial::COM1);

    return static_cast<int32_t>(tasks[slot].id);
}

size_t Scheduler::pick_next_ready(size_t from_idx) {
    for (size_t offset = 1; offset <= MAX_TASKS; ++offset) {
        size_t idx = (from_idx + offset) % MAX_TASKS;
        if (tasks[idx].state == TaskState::Ready) {
            return idx;
        }
    }
    return from_idx;
}

void Scheduler::switch_to(size_t next_idx) {
    if (next_idx == current_idx) {
        return;
    }

    size_t prev_idx = current_idx;
    if (tasks[prev_idx].state == TaskState::Running) {
        tasks[prev_idx].state = TaskState::Ready;
    }

    tasks[next_idx].state = TaskState::Running;
    current_idx = next_idx;

    /*
    Serial::print_dec(static_cast<int32_t>(prev_idx), Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(next_idx), Serial::COM1);
    Serial::print_hex32(tasks[next_idx].esp, Serial::COM1);
    Serial::print_line("", Serial::COM1);
    */

    switch_context(&tasks[prev_idx].esp, tasks[next_idx].esp);
}

void Scheduler::yield() {
    if (!initialized) {
        return;
    }
    size_t next_idx = pick_next_ready(current_idx);
    switch_to(next_idx);
}

void Scheduler::exit_current() {
    if (!initialized) {
        return;
    }
    tasks[current_idx].state = TaskState::Dead;
    size_t next_idx = pick_next_ready(current_idx);
    if (next_idx != current_idx) {
        switch_to(next_idx);
    }
    while (true) {
        asm volatile("hlt");
    }
}

void Scheduler::tick() {
    // Puoi attivare qui la preemption dopo aver verificato yield()
}

size_t Scheduler::task_count() {
    size_t count = 0;
    for (size_t i = 0; i < MAX_TASKS; ++i) {
        if (tasks[i].state != TaskState::Unused && tasks[i].state != TaskState::Dead) {
            ++count;
        }
    }
    return count;
}

const Task* Scheduler::get_task(size_t index) {
    if (index >= MAX_TASKS) {
        return nullptr;
    }
    return &tasks[index];
}

int32_t Scheduler::current_id() {
    if (!initialized) {
        return -1;
    }
    return static_cast<int32_t>(tasks[current_idx].id);
}